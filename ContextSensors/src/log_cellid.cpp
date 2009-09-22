// Copyright (c) 2007-2009 Google Inc.
// Copyright (c) 2006-2007 Jaiku Ltd.
// Copyright (c) 2002-2006 Mika Raento and Renaud Petit
//
// This software is licensed at your choice under either 1 or 2 below.
//
// 1. MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// 2. Gnu General Public license 2.0
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//
// This file is part of the JaikuEngine mobile client.

// Concepts:
// !Getting the current cell-id!

#include "log_cellid.h"
#include "csd_cell.h"
#include "cellmap.h"


#include <e32base.h>
#include "stripped_etelbgsm.h"
#include "app_context.h"
#include "operatormap.h"

#ifdef __WINS__
#include "bases.h"
#endif

#ifndef __WINS__
#  if defined(__S60V3__) || defined(__S60V2FP3__)
#define ETEL3RDPARTY
#  else
#    if defined(__S60V2__)
//#define MOBINFO
#define BASICGSM
#    else
#define BASICGSM
#    endif
#  endif //S60V3
#endif //WINS

#ifdef ETEL3RDPARTY
#include <etel3rdparty.h>
#endif
#ifdef MOBINFO
#include <mobileinfo.h>
#endif

#include "timeout.h"

class Clog_cellidImpl: public Clog_cellid, public MTimeOut
{
public:
	virtual const CBBSensorEvent& get_value();
private:
	virtual void DoCancel();
	virtual void CheckedRunL(); 
	virtual TInt CheckedRunError(TInt aError);
	virtual void expired(CBase*);
	void ResetMissing();
private:
	Clog_cellidImpl(MApp_context& Context, COperatorMap* aOpMap, CCellMap* aCellMap);
	void ConstructL();
	~Clog_cellidImpl();
	
#ifndef __WINS__

#ifdef BASICGSM
	MBasicGsmPhoneNetwork::TCurrentNetworkInfo async_info;
	MBasicGsmPhoneNetwork::TCurrentNetworkInfo prev_info;
#endif

#ifdef ETEL3RDPARTY
	CTelephony::TNetworkInfoV1Pckg async_infop;
	CTelephony::TNetworkInfoV1 async_info;
	CTelephony::TNetworkInfoV1 prev_info;
	enum T3rdpartyState { IDLE, GETTING_INFO, NOTIFYONCHANGE };
	T3rdpartyState	i3rdpartyState;
#endif

#ifdef MOBINFO
	CMobileNetworkInfo*	iMobInfo;
	TMobileCellIdBuf	async_infop;
	TMobileCellId		prev_info;
	enum TInfoState { IDLE, GETTING_FIRST_MCC, 
		GETTING_FIRST_CELL, GETTING_CELL, GETTING_NEW_MCC };
	TInfoState			iInfoState;
	TMobileNetwork		iNetworkInfo;
#endif

#endif //!WINS
	TInt	iErrorCount;
#ifdef __WINS__
	int test_data_i;
	RTimer timer;
	COperatorMap* iOpMap;
#endif
	class CCellMap* iCellMap;
	TBBCellId iCell;
	CTimeOut*	iTimeOut;
	TBool		iPostedMissing;
	friend class Clog_cellid;
};

#define MISSING_TIMEOUT	90

Clog_cellid::Clog_cellid(MApp_context& Context) : 
	CCheckedActive(EPriorityIdle, _L("Clog_cellid")), 
		Mlog_base_impl(Context, KCell, KCellIdTuple, 5*24*60*60) { }

Clog_cellidImpl::Clog_cellidImpl(MApp_context& Context, COperatorMap* aOpMap, CCellMap* aCellMap) : 
	Clog_cellid(Context),
#if defined(ETEL3RDPARTY)
async_infop(async_info),
#endif
	iCell(KCell), iCellMap(aCellMap) {
#ifdef __WINS__
	iOpMap=aOpMap;
	test_data_i=0;
#endif
}

EXPORT_C Clog_cellid* Clog_cellid::NewL(MApp_context& Context, 
											   COperatorMap* aOpMap,
	class CCellMap* aCellMap)
{
	CALLSTACKITEM2_N(_CL("Clog_cellid"), _CL("NewL"),  &Context);
	auto_ptr<Clog_cellidImpl> ret(new (ELeave) Clog_cellidImpl(Context, aOpMap, aCellMap));
	ret->ConstructL();
	return ret.release();
}

void Clog_cellidImpl::ConstructL() 
{
	CALLSTACKITEM_N(_CL("Clog_cellidImpl"), _CL("ConstructL"));

	Mlog_base_impl::ConstructL();

	CActiveScheduler::Add(this); // add to scheduler
	iTimeOut=CTimeOut::NewL(*this);
#ifndef __WINS__

#ifdef BASICGSM
	TInt err=Phone().GetCurrentNetworkInfo(async_info);
	TRequestStatus *s=&iStatus;
	User::RequestComplete(s, err);
#endif

#ifdef ETEL3RDPARTY
	Telephony().GetCurrentNetworkInfo(iStatus, async_infop);
	i3rdpartyState=GETTING_INFO;
#endif

#ifdef MOBINFO
	iMobInfo=CMobileNetworkInfo::NewL();
	iInfoState=GETTING_FIRST_MCC;
	iMobInfo->GetCurrentNetwork(iNetworkInfo, iStatus);
	//iMobInfo->GetCellId(async_infop, iStatus);
#endif

	SetActive();

#else
	iOpMap->AddRef();
	timer.CreateLocal();
	//TTimeIntervalMicroSeconds32 w(5*1000*1000);
	TTimeIntervalMicroSeconds32 w(1*1000*1000);
	timer.After(iStatus, w);
	SetActive();
#endif

#ifdef BASICGSM
	//post_new_value(get_value());
#endif
}

Clog_cellidImpl::~Clog_cellidImpl()
{
	CALLSTACKITEM_N(_CL("Clog_cellidImpl"), _CL("~Clog_cellidImpl"));
	Cancel();

#ifdef MOBINFO
	delete iMobInfo;
#endif

	delete iTimeOut;
#ifdef __WINS__
	if (iOpMap) iOpMap->Release();
	timer.Close();
#endif
}

void Clog_cellidImpl::expired(CBase*)
{
	iPostedMissing=ETrue;
	post_new_value(&iCell);
}

void Clog_cellidImpl::ResetMissing()
{
	iTimeOut->Reset();
	iPostedMissing=EFalse;
}

void Clog_cellidImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("Clog_cellidImpl"), _CL("DoCancel"));

#ifndef __WINS__

#  ifdef BASICGSM
#    ifndef __S60V2__
	Phone().NotifyChangeOfCurrentNetworkCancel();
#    else
	// The Cancel hangs on v2 :-(
	//Phone().NotifyChangeOfCurrentNetworkCancelNS();
	TRequestStatus* s=&iStatus;
	User::RequestComplete(s, KErrCancel);
#  endif
#  endif

#ifdef ETEL3RDPARTY
	if (i3rdpartyState==NOTIFYONCHANGE)
		Telephony().CancelAsync(CTelephony::ECurrentNetworkInfoChangeCancel);
	else 
		Telephony().CancelAsync(CTelephony::EGetCurrentNetworkInfoCancel);
#endif

#ifdef MOBINFO
	switch(iInfoState) {
	case GETTING_CELL:
		iMobInfo->CancelCellIdChangeNotification();
		break;
	case GETTING_FIRST_CELL:
		iMobInfo->CancelGetCellId();
		break;
	case GETTING_FIRST_MCC:
	case GETTING_NEW_MCC:
		iMobInfo->CancelGetCurrentNetwork();
		break;
	};
#endif
#else
	timer.Cancel();
#endif
}

TInt Clog_cellidImpl::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("Clog_cellidImpl"), _CL("CheckedRunError"));
	if (iErrorCount >= 10) return aError;

	TBuf<40> msg;
	msg.Format(_L("Clog_cellidImpl::CheckedRunError %d"), aError);

	post_error(msg, aError, GetTime());

#ifndef __WINS__
	Cancel();

#  ifdef BASICGSM
#    ifndef __S60V2__
	Phone().NotifyChangeOfCurrentNetwork(iStatus, async_info);
#    else
	Phone().NotifyChangeOfCurrentNetworkNS(iStatus, async_info);
#    endif
#  endif

#  ifdef ETEL3RDPARTY
	Telephony().NotifyChange(iStatus, CTelephony::ECurrentNetworkInfoChange, async_infop);
	i3rdpartyState=NOTIFYONCHANGE;
#  endif

#  ifdef MOBINFO

	if (iInfoState==GETTING_FIRST_MCC || iInfoState==GETTING_NEW_MCC) {
		iMobInfo->GetCurrentNetwork(iNetworkInfo, iStatus);
	} else {
		iMobInfo->NotifyCellIdChange(async_infop, iStatus);
	}
#  endif

	SetActive();
	return KErrNone;
#else // __WINS__
	return aError;
#endif
}

void Clog_cellidImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("Clog_cellidImpl"), _CL("CheckedRunL"));

#ifdef __WINS__
	TTimeIntervalMicroSeconds32 wait_time;
#endif
	if (iStatus!=KErrNone) {
		TBuf<40> msg;
		msg.Format(_L("NotifyChangeOfCurrentNetwork %d"), iStatus.Int());
		post_error(msg, iStatus.Int(), GetTime());
		if (iErrorCount++ >= 10) {
			User::Leave(-1005);
		}
	} else {
		iErrorCount=0;
#ifndef __WINS__
#  ifdef BASICGSM
		if (async_info.iNetworkInfo.iShortName.CompareF(_L("elisa"))==0) {
			async_info.iNetworkInfo.iShortName=_L("RADIOLINJA");
		}
		iCell.iMCC()=async_info.iNetworkInfo.iId.iMCC;
		iCell.iMNC()=async_info.iNetworkInfo.iId.iMNC;
		iCell.iShortName=async_info.iNetworkInfo.iShortName;
		iCell.iLocationAreaCode()=async_info.iLocationAreaCode;
		iCell.iCellId()=async_info.iCellId;
		iCell.iMappedId()=iCellMap->GetId(iCell);
#  endif
#  ifdef ETEL3RDPARTY
		iCell.iMCC.FromStringL(async_info.iCountryCode);
		iCell.iMNC.FromStringL(async_info.iNetworkId);
		iCell.iShortName=async_info.iShortName;
		iCell.iLocationAreaCode()=async_info.iLocationAreaCode;
		iCell.iCellId()=async_info.iCellId;
		iCell.iMappedId()=iCellMap->GetId(iCell);
#  endif
#  ifdef MOBINFO
		//TBuf<100> msg=_L("cell mcc ");

		if (iInfoState==GETTING_FIRST_MCC || iInfoState==GETTING_NEW_MCC) {
			iCell.iMCC.FromStringL(iNetworkInfo.iNetworkCountryCode);
		}
		if (iInfoState==GETTING_FIRST_MCC) {
			//post_error(msg, KErrGeneral);
			iInfoState=GETTING_FIRST_CELL;
			iMobInfo->GetCellId(async_infop, iStatus);
			SetActive();
			return;
		}
		/*
		msg.Append(iNetworkInfo.iNetworkCountryCode);
		if (iInfoState==GETTING_CELL) {
			msg.Append(async_infop().iCountryCode);
			msg.Append(_L(" mnc "));
			msg.Append(async_infop().iNetworkIdentity);
			msg.Append(_L(" lac "));
			msg.AppendNum(async_infop().iLocationAreaCode);
			msg.Append(_L(" cellid "));
			msg.AppendNum(async_infop().iCellId);
		}
		post_error(msg, KErrGeneral);
		return;
		*/
		
		iCell.iMNC.FromStringL(async_infop().iNetworkIdentity);
		iCell.iShortName()=iNetworkInfo.iNetworkShortName;
		iCell.iLocationAreaCode()=async_infop().iLocationAreaCode;
		iCell.iCellId()=async_infop().iCellId;
		iCell.iMappedId()=iCellMap->GetId(iCell);

		if (	(iInfoState==GETTING_CELL) && 
				(iCell.iMNC() != prev_info.iLocationAreaCode)) {
			iInfoState=GETTING_NEW_MCC;
			iMobInfo->GetCellId(async_infop, iStatus);
			SetActive();
			return;
		}
		async_infop().iCountryCode=iNetworkInfo.iNetworkCountryCode;
		iInfoState=GETTING_CELL;
#  endif

		if ( iCell.iLocationAreaCode()==0 && iCell.iCellId()==0) {
			if (!iPostedMissing ) {
				iTimeOut->WaitMax(MISSING_TIMEOUT);
			}
		} else {
			ResetMissing();
			post_new_value(&iCell);
		}

#  ifndef MOBINFO
		prev_info=async_info;
#  else
		prev_info=async_infop();
#  endif
	}
#ifdef BASICGSM
#  ifndef __S60V2__
	Phone().NotifyChangeOfCurrentNetwork(iStatus, async_info);
#  else
	Phone().NotifyChangeOfCurrentNetworkNS(iStatus, async_info);
#  endif
#endif //BASICGSM

#ifdef ETEL3RDPARTY
	Telephony().NotifyChange(iStatus, CTelephony::ECurrentNetworkInfoChange, async_infop);
	i3rdpartyState=NOTIFYONCHANGE;
#endif

#ifdef MOBINFO
	post_error(_L("NotifyCellIdChange"), KErrGeneral);
	iMobInfo->NotifyCellIdChange(async_infop, iStatus);
#endif
	SetActive();
#else //WINS
		TTime this_time;
		TBuf<40> val=bases::test_data[test_data_i][1];
		while (! val.Compare(_L("SWITCH"))) {
			val=bases::test_data[++test_data_i][1];
		}
		TPtrC dtd(bases::test_data[test_data_i][0]);
		_LIT(KTime, "t");
		TBBTime t(KTime);
		t.FromStringL(dtd);
		CCellMap::Parse(val, iCell.iCellId(), iCell.iLocationAreaCode(), iCell.iShortName());
		iOpMap->NameToMccMnc(iCell.iShortName(), iCell.iMCC(), iCell.iMNC());
		++test_data_i;
		if (test_data_i == 322) {
			TInt x=0;
		}
		iCell.iMappedId()=iCellMap->GetId(iCell);
		post_new_value(&iCell, t());
		this_time=t();
		TTime next_time;
		{
			val=bases::test_data[test_data_i][1];
			while (! val.Compare(_L("SWITCH"))) {
				val=bases::test_data[++test_data_i][1];
			}
			TPtrC dtd(bases::test_data[test_data_i][0]);
			t.FromStringL(dtd);
			next_time=t();
		}
		TTimeIntervalMicroSeconds w=next_time.MicroSecondsFrom(this_time);
		TInt64 ms=w.Int64();
		//ms/=(60*5);
		wait_time=I64LOW(ms);
	}
	//TTimeIntervalMicroSeconds32 w(120*1000*1000);
	//TTimeIntervalMicroSeconds32 w(100*1000);
	timer.After(iStatus, wait_time);
	SetActive();
	//iErrorCount=10; User::Leave(1);
	//User::Panic(_L("test panic"), 10);
	//RThread me;
	//me.Open(me.Id());
	//me.RaiseException(EExcAccessViolation);
#endif

}

const CBBSensorEvent& Clog_cellidImpl::get_value()
{
	CALLSTACKITEM_N(_CL("Clog_cellidImpl"), _CL("get_value"));

#ifndef __WINS__

#  ifdef BASICGSM
	MBasicGsmPhoneNetwork::TCurrentNetworkInfo ni;
	ni.iLocationAreaCode=ni.iCellId=0;
	// get network info

	User::LeaveIfError( Phone().GetCurrentNetworkInfo( ni ) );
	if (ni.iNetworkInfo.iShortName.CompareF(_L("elisa"))==0) {
		ni.iNetworkInfo.iShortName=_L("RADIOLINJA");
	}
	prev_info=ni;

	iCell.iMCC()=ni.iNetworkInfo.iId.iMCC;
	iCell.iMNC()=ni.iNetworkInfo.iId.iMNC;
	iCell.iShortName=ni.iNetworkInfo.iShortName;
	iCell.iLocationAreaCode()=ni.iLocationAreaCode;
	iCell.iCellId()=ni.iCellId;
	
	iCell.iMappedId()=iCellMap->GetId(iCell);
	iEvent.iData.SetOwnsValue(EFalse);
	iEvent.iData.SetValue(&iCell);
	iEvent.iStamp()=GetTime();
#  endif

#endif
	return iEvent;
}
