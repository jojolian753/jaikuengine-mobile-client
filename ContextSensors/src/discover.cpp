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
// !Bluetooth scan!
// !Own bluetooth MAC address!

#include "discover.h"
#include "cl_settings.h"

#include <e32base.h>
#include <f32file.h>
#include <e32std.h>

#include "log_base_impl.h"
#include "timeout.h"

#ifdef __WINS__
#define NO_BT 1
#endif

_LIT(KBtAccessMutex, "ContextBtMutex");

#include <btsdp.h>

#ifdef __USE_PLUGIN_APIS__
#include <bteng.h>
#else
#define NO_BTENG_H 1
#endif

#include <contextvariant.hrh>

//#ifdef __S60V3__
//#define NO_BTENG_H 1
//#endif
#ifdef __S60V2FP3__
#define NO_BTENG_H 1
#endif

#ifndef NO_BTENG_H
#include <btmanclient.h>
#include <bttypes.h>
#else
#include <btmanclient.h>
#include <bttypes.h>
// reverse engineered from BTENG.LIB:
enum TBTDiscoverabilityMode
    {
    EBTDiscModeHidden,
    EBTDiscModeLimited,
    EBTDiscModeGeneral
    };
enum TBTSearchMode { EBTSearchMode0, EBTSearchMode1 };
class CBTMCMSettings : public CBase {
public:
        IMPORT_C static int  GetAllSettings(int &, enum TBTDiscoverabilityMode &, enum TBTSearchMode &, class TDes16 &, int &);
        IMPORT_C static int  GetDiscoverabilityModeL(enum TBTDiscoverabilityMode &);
        IMPORT_C static int  GetLocalBDAddress(class TBTDevAddr &);
        IMPORT_C static int  GetLocalBTName(class TDes16 &);
#ifndef  __S60V3__
        IMPORT_C static int  GetPowerStateL(int &);
#else
        IMPORT_C static int  GetPowerState(int &);
#endif
        IMPORT_C static int  GetSearchModeL(enum TBTSearchMode &);
        IMPORT_C static int  IsLocalNameModified(int &);
        IMPORT_C static class CBTMCMSettings *  NewL(class MBTMCMSettingsCB *);
        IMPORT_C static class CBTMCMSettings *  NewLC(class MBTMCMSettingsCB *);
        IMPORT_C int  SetDefaultValuesL(void);
        IMPORT_C int  SetDiscoverabilityModeL(enum TBTDiscoverabilityMode, int);
        IMPORT_C int  SetLocalBTName(class TDesC16 const &);
#ifndef __S60V3__
        IMPORT_C int  SetPowerStateL(int, int);
#else
        IMPORT_C int  SetPowerState(int);
#endif
        IMPORT_C int  SetSearchModeL(enum TBTSearchMode);
};
#endif

#include <es_sock.h>
#include <bt_sock.h>

#include "csd_bluetooth.h"
#include "mutexrequest.h"
#include "reporting.h"

#ifdef __S60V2__

#  ifndef __E32PROPERTY_H__
/*
 * I don't want to create a new build variant just for this, so
 * here's a rip of the RProperty header. The header is only
 * in the 8.0a SDK, although it is supported on 7.0s. 
 */
class RProperty : public RHandleBase {
public:
	enum TType { EInt, EByteArray, EText = EByteArray, ETypeLimit, ETypeMask = 0xff };
	IMPORT_C static TInt Get(TUid aCategory, TUint aKey, TDes8& aValue);
};

const TUid KPropertyUidBluetoothCategory = {0x101FD916};
const TUint KPropertyKeyBluetoothLocalDeviceAddress = 0;
#  endif
#endif

_LIT(KName, "CDiscoverImpl");
_LIT(KFunctionality, "Information about other bluetooth devices around you");

class CDiscoverImpl: public CDiscover, public MTimeOut, public MSettingListener
{
public:
	~CDiscoverImpl();
	
	virtual void ComponentId(TUid& aComponentUid, TInt& aComponentId, TInt& aVersion) {
		aComponentUid=KBluetoothTuple.iModule;
		aComponentId=KBluetoothTuple.iId;
		aVersion=2;
	}
	virtual const TDesC& Name() { return KName; }
	virtual const TDesC& HumanReadableFunctionality() { return KFunctionality; }

	//static CDiscoverImpl* NewL(MApp_context& Context);
private:
	void StartL();
	void InnerConstructL();
	CDiscoverImpl(MApp_context& Context, TBool aSetToInvisible);
	virtual void ComponentCancel();
	virtual void ComponentRunL();
	virtual const TBTDevAddr& GetOwnAddress() { return own_address; }
	virtual void SettingChanged(TInt Setting);
private:
	enum comm_state { IDLE, WAITING_ON_MUTEX, GETTING_LOCAL, FIRST_SEARCH, NEXT_SEARCH,
		WAITING_ON_BT_BUSY };
	comm_state current_state;
	CBBBtDeviceList* names;
	CBBBtDeviceList* prev_env;

	RHostResolver hr; bool hr_is_open;
	RSocket socket; bool socket_is_open;
	TBTDevAddr own_address; TPckgBuf<TBTDevAddr> iDevAddrPckg;
	RSocketServ socket_server; bool socket_server_is_open;
	TInquirySockAddr addr;
	TNameEntry name;
	TProtocolDesc pInfo;
	TInt	iInterval;

	RTimer timer; bool timer_is_open;
	CTimeOut*	iTimeOut;
	TInt		iTimeOutCount;
	TBool		iFirst, iReseting;
	CMutexRequest*	iMutex;
	void ReleaseMutex();

	TTime		iPrevPost;
#ifndef NO_BTENG_H
	RBTDeviceHandler iBtHandler;
#endif
	TBool iSetToInvisible;

	void StopL();
	void finished();
	void expired(CBase* source);
	
	friend class CDiscover;
};

EXPORT_C CDiscover* CDiscover::NewL(MApp_context& Context, TBool aSetToInvisble)
{
	CALLSTACKITEM2_N(_CL("CDiscover"), _CL("NewL"),  &Context);

	auto_ptr<CDiscoverImpl> ret(new (ELeave) CDiscoverImpl(Context, aSetToInvisble));
	ret->CComponentBase::ConstructL();
	return ret.release();
}

EXPORT_C CDiscover::~CDiscover()
{
}

CDiscover::CDiscover(MApp_context& Context) : CComponentBase(KName), Mlog_base_impl(Context, KCSDBtList, KBluetoothTuple, 0)
{
}

CDiscoverImpl::CDiscoverImpl(MApp_context& Context, TBool aSetToInvisible) : CDiscover(Context),
	current_state(IDLE), iFirst(ETrue), iSetToInvisible(aSetToInvisible) { }

void CDiscoverImpl::InnerConstructL() 
{
	CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("ConstructL"));

	names=CBBBtDeviceList::NewL();
	prev_env=CBBBtDeviceList::NewL();

	Settings().NotifyOnChange(SETTING_BT_SCAN_INTERVAL, this);

	Mlog_base_impl::ConstructL();
	iTimeOut=CTimeOut::NewL(*this);
}

void CDiscoverImpl::StartL()
{
	CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("Restart"));

	if (prev_env->Count()>0) {
		CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("post empty"));
		prev_env->Reset();
		iEvent.iData.SetValue(prev_env);
		iEvent.iStamp()=GetTime();
		iPrevPost=GetTime();
		post_new_value(iEvent);
	}
	{
		CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("Reset"));
		Cancel();
		StopL();
		iTimeOut->Reset();
		names->Reset();
		current_state=IDLE;
	}

	iInterval=5*60;
	{
		CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("interval setting"));
		Settings().GetSettingL(SETTING_BT_SCAN_INTERVAL, iInterval);
		if (iInterval>2000) iInterval=2000;
	}

	if (iInterval==0) return;
	iLeaseTimeInSeconds=iInterval+60;

	{
		CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("timer"));
		User::LeaveIfError(timer.CreateLocal()); timer_is_open=true;
	}
	auto_ptr<CBTMCMSettings> BtSettings(0);
	TInt err; TBool got_own_address=EFalse;
#if !defined(__S60V2FP3__) && defined(__USE_PLUGIN_APIS__) && !defined(__WINS__)
	TRAP(err, BtSettings.reset(CBTMCMSettings::NewL(0)));
#else
	err=KErrNotSupported;
#endif
	if (err!=KErrNone || BtSettings.get()==0) {
		CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("no settings"));
#if !defined(__S60V2FP3__)
		post_error(_L("cannot create bt settings "), err);
#endif
	} else {
		CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("settings"));

#if !defined(__S60V2FP3__) && !defined(__WINS__)
		{
			CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("power"));
			TBool power;
#ifndef __S60V3__
			TRAP(err, BtSettings->GetPowerStateL(power));
#else
			err=BtSettings->GetPowerState(power);
#endif
			if (err!=KErrNone) {
				post_error(_L("cannot get bt power state "), err);
			}
			if (err==KErrNone && !power) {
				//FIXME: post_new_value(_L("Powering up Bt"), Mlogger::INFO);
				if (!iReseting) {
					post_error(_L("Bluetooth turned off "), err);
					return;
				}
#ifndef __S60V3__
				TRAP(err, BtSettings->SetPowerStateL(ETrue, EFalse));
#else
				err=BtSettings->SetPowerState(ETrue);
#endif
				iReseting=EFalse;
				if (err!=KErrNone) {
					post_error(_L("cannot set bt power state "), err);
				}
			}
#ifndef __S60V3__
			if (iSetToInvisible) {
			TRAP(err, 
				BtSettings->SetDiscoverabilityModeL(EBTDiscModeHidden, EFalse));
			}
#endif
		}
#endif

#if !defined(__WINS__)
		{
			CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("address"));
			TInt err1=0, err2=0;
			TBTDevAddr a;
#  if !defined(__S60V3__)
			TRAP(err2, err1=BtSettings->GetLocalBDAddress(a));
#  else
			err2=KErrNotSupported;
#  endif
			if (err2!=KErrNone) err1=err2;
			if (err1==KErrNone) {
				iDevAddrPckg()=a;
				got_own_address=ETrue;
			}
		}
#endif
	}

	if (!got_own_address) {
		TInt err1=KErrNone;	
#  if defined(__S60V2__)
		err1=RProperty::Get(KPropertyUidBluetoothCategory, 
			KPropertyKeyBluetoothLocalDeviceAddress, iDevAddrPckg);
#  endif
		if (err1<0) {
			post_error(_L("cannot get own address"), err1);
		} else {
			got_own_address=ETrue;
		}
	}

#ifndef NO_BT
	{
		CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("Socket conn"));

		User::LeaveIfError(socket_server.Connect()); socket_server_is_open=true;
		TProtocolName aName=_L("BTLinkManager");
		User::LeaveIfError(socket_server.FindProtocol(aName, pInfo));
		User::LeaveIfError(socket.Open(socket_server, KBTAddrFamily, KSockSeqPacket, KL2CAP));	
		User::LeaveIfError(hr.Open(socket_server,pInfo.iAddrFamily,pInfo.iProtocol)); hr_is_open=true;
	}


	{
		CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("async"));
		if (! got_own_address ) {
			socket.Ioctl(KHCILocalAddressIoctl, iStatus, &iDevAddrPckg, KSolBtHCI);
		} else {
			TRequestStatus *s=&iStatus;
			User::RequestComplete(s, KErrNone);
		}
		SetActive();
		current_state=GETTING_LOCAL;
	}
#else // __WINS__
	TTimeIntervalMicroSeconds32 w(5*1000*1000);
	timer.After(iStatus, w);
	SetActive();
#endif
}

/*
CDiscoverImpl* CDiscoverImpl::NewL(MApp_context& Context, TBool aSetToInvisible)
{
	CALLSTACKITEM2_N(_CL("CDiscoverImpl"), _CL("NewL"),  &Context);

	auto_ptr<CDiscoverImpl> ret(new (ELeave) CDiscoverImpl(Context, aSetToInvisible));
	ret->Const	ructL();
	return ret.release();
}
*/

void CDiscoverImpl::StopL()
{	
	CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("release"));

	if (socket_is_open) socket.Close();
	socket_is_open=false;

	if (timer_is_open) timer.Close();
	timer_is_open=false;
	if (hr_is_open) hr.Close();
	hr_is_open=false;
	if (socket_server_is_open) socket_server.Close();
	socket_server_is_open=false;
}

void CDiscoverImpl::ReleaseMutex()
{
	if (!iMutex) return;

	delete iMutex; iMutex=0;
	//Reporting().DebugLog(_L("discover::releasemutex"));
}

CDiscoverImpl::~CDiscoverImpl()
{
	CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("~CDiscoverImpl"));

	Settings().CancelNotifyOnChange(SETTING_BT_SCAN_INTERVAL, this);

	delete iTimeOut;
	Cancel();
	StopL();

	ReleaseMutex();

	delete names; delete prev_env;
}	

void CDiscoverImpl::ComponentCancel()
{
	CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("ComponentCancel"));

	switch(current_state) {
	case IDLE:
		timer.Cancel();
		break;
	case GETTING_LOCAL:
		socket.CancelIoctl();
		break;
	case WAITING_ON_MUTEX:
		ReleaseMutex();
		break;
	default:
		hr.Cancel();
		break;
	}
}


void CDiscoverImpl::ComponentRunL()
{
	CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("ComponentRunL"));

	iLeaseTimeInSeconds=iInterval+60;
	iTimeOut->Reset();
	TBuf<30> msg;
	TInt async_ret=iStatus.Int();
	if (async_ret!=KErrNone && async_ret!=KErrHostResNoMoreResults ) {
		_LIT(stat_err, "error: %d at state %d");
		msg.Format(stat_err, iStatus.Int(), current_state);
		//Reporting().DebugLog(msg);
		post_error(msg, iStatus.Int(), GetTime());
		if (current_state!=GETTING_LOCAL) {
			ReleaseMutex();
			StartL();
			return;
		}
	}
#ifndef NO_BT
	switch(current_state) {
	case GETTING_LOCAL:
		{
		if (socket_is_open) socket.Close();
		socket_is_open=false;

		if (iInterval>0) {
			TTimeIntervalMicroSeconds32 w;
			if (iFirst)
				w=TTimeIntervalMicroSeconds32(3*1000*1000);
			else
				w=TTimeIntervalMicroSeconds32(iInterval*1000*1000);
			iFirst=EFalse;
			timer.After(iStatus, w);
			SetActive();
			current_state=IDLE;
		}

		if (async_ret==KErrNone) {
			own_address=iDevAddrPckg();
			CBBSensorEvent e(KCSDBtOwnAddress, KOwnBluetoothTuple); e.iData.SetOwnsValue(EFalse);
			bb_auto_ptr<TBBBtDeviceInfo> di(new (ELeave) TBBBtDeviceInfo(iDevAddrPckg, _L(""),  0, 0, 0));
			names->AddItemL(di.get());
			di.release();
			e.iStamp()=GetTime();
			e.iData.SetValue(names);

			iPrevPost=GetTime();
			iLeaseTimeInSeconds=48*60*60;
			post_new_value(e);
		}
		break;
		}
	case FIRST_SEARCH:
		{
		if (iStatus.Int()!=KErrHostResNoMoreResults) {
			names->Reset();
			{
				bb_auto_ptr<TBBBtDeviceInfo> di(new (ELeave) TBBBtDeviceInfo(iDevAddrPckg, _L(""),  0, 0, 0));
				names->AddItemL(di.get());
				di.release();
			}
			{
				TInquirySockAddr btaddr(name().iAddr);
				bb_auto_ptr<TBBBtDeviceInfo> di(new (ELeave) TBBBtDeviceInfo(btaddr, name().iName));
				names->AddItemL(di.get());
				di.release();
			}

			hr.Next(name, iStatus);
			iTimeOut->Wait(3*60);
			SetActive();
			current_state=NEXT_SEARCH;
		} else {
			ReleaseMutex();
			//Reporting().DebugLog(_L("discover: finished scan"));
			if (prev_env->Count()>1 || iPrevPost+TTimeIntervalMinutes(5)<GetTime()) {
				prev_env->Reset();
				{
					bb_auto_ptr<TBBBtDeviceInfo> di(new (ELeave) TBBBtDeviceInfo(iDevAddrPckg, _L(""),  0, 0, 0));
					prev_env->AddItemL(di.get());
					di.release();
				}
				iPrevPost=GetTime();
				post_new_value(prev_env);
			}
			current_state=IDLE;
			iTimeOutCount=0;
			if (iInterval>0) {
				TTimeIntervalMicroSeconds32 w(iInterval*1000*1000);
				timer.After(iStatus, w);
				SetActive();
			}
		}
		}
		break;
	case NEXT_SEARCH:
		{
		if (iStatus.Int()!=KErrHostResNoMoreResults) {
			TInquirySockAddr btaddr(name().iAddr);
			bb_auto_ptr<TBBBtDeviceInfo> di(new (ELeave) TBBBtDeviceInfo(btaddr, name().iName));
			names->AddItemL(di.get());
			di.release();
			
			hr.Next(name, iStatus);
			iTimeOut->Wait(3*60);
			SetActive();
		} else {
			ReleaseMutex();
			//Reporting().DebugLog(_L("discover: finished scan"));
			if (prev_env->Equals(names)) {
				iEvent.iPriority()=CBBSensorEvent::UNCHANGED_VALUE;
				iEvent.iData.SetValue(names);
				iEvent.iStamp()=GetTime();
				iPrevPost=GetTime();
				post_new_value(iEvent);
			} else {
				iEvent.iData.SetValue(names);
				iEvent.iPriority()=CBBSensorEvent::VALUE;
				iEvent.iStamp()=GetTime();
				post_new_value(iEvent);
				CBBBtDeviceList* tmp=prev_env;
				prev_env=names; names=tmp;
			}
			current_state=IDLE;
			iTimeOutCount=0;
			if (iInterval>0) {
				TTimeIntervalMicroSeconds32 w(iInterval*1000*1000);
				timer.After(iStatus, w);
				SetActive();
			}
		}
		}
		break;
	case IDLE:
		{
		TTimeIntervalMicroSeconds32 w(120*1000*1000);
		current_state=WAITING_ON_MUTEX;
		//Reporting().DebugLog(_L("discover::wait_on_mutex"));
		iMutex=CMutexRequest::NewL(AppContext(), KBtAccessMutex, w, &iStatus);
		SetActive();
		break;
		}
	case WAITING_ON_MUTEX:
		if (iStatus==KErrNone) {
			//Reporting().DebugLog(_L("discover:got_mutex"));
			current_state=WAITING_ON_BT_BUSY;
			timer.After(iStatus, 
				TTimeIntervalMicroSeconds32(1*1000*1000));
			SetActive();
			break;
		}
	case WAITING_ON_BT_BUSY:
		{
		if (iStatus!=KErrNone) {
			ReleaseMutex();
			current_state=IDLE;
			if (iInterval>0) {
				TTimeIntervalMicroSeconds32 w(iInterval*1000*1000);
				timer.After(iStatus, w);
				SetActive();
			}
			return;
		}
		TBool connections=EFalse;
#ifndef NO_BTENG_H
		iBtHandler.AreExistingConnectionsL(connections);
#endif
		if (connections) {
			if (current_state==WAITING_ON_BT_BUSY) {
				ReleaseMutex();
				current_state=IDLE;
				//Reporting().DebugLog(_L("discover: BT connections in use"));
				if (iInterval>0) {
					TTimeIntervalMicroSeconds32 w(iInterval*1000*1000);
					timer.After(iStatus, w);
					SetActive();
				}
				post_error(_L("BT connections in use"), KErrInUse);
			} else {
				current_state=WAITING_ON_BT_BUSY;
				TTimeIntervalMicroSeconds32 w(5*1000*1000);
				timer.After(iStatus, w);
				SetActive();
			}			
		} else {
			if (iInterval>0) {
				//Reporting().DebugLog(_L("discover: starting scan"));
				addr.SetIAC(KGIAC);
				//addr.SetAction(KHostResInquiry|KHostResName|KHostResIgnoreCache);
				// don't ask for host names for quicker and more robust
				// inquiry
				addr.SetAction(KHostResInquiry|KHostResIgnoreCache);
				hr.GetByAddress(addr, name, iStatus);

				SetActive();
			
				iTimeOut->Wait(3*60);
				current_state=FIRST_SEARCH;
			}
		}
		break;
		}
	}
#else // !NO_BT
	names->Reset();
	CBBSensorEvent e(KCSDBtList, KBluetoothTuple); e.iData.SetOwnsValue(EFalse);
	//bb_auto_ptr<TBBBtDeviceInfo> di(new (ELeave) TBBBtDeviceInfo(_L8("\x01\x20\xe0\x4c\x71\xb9"), _L(""),  2, 3, 4));

	_LIT8(mac, "\x00\x12\x62\xe3\x53\xb1");
	bb_auto_ptr<TBBBtDeviceInfo> di(new (ELeave) 
		TBBBtDeviceInfo(mac, _L(""),  2, 3, 4));
	names->AddItemL(di.get());
	di.release();
	e.iData.SetValue(names);
	e.iStamp()=GetTime();

	post_new_value(e);

	TTimeIntervalMicroSeconds32 w(10*1000*1000);
	timer.After(iStatus, w);
	SetActive();
	//FIXME: post_new_value(_L("0020e04c71b8 [GLOMGOLD-25,1:1:2] 0002ee51c437 [Reno7650,2:3:4] 0002ee51c438 [Reno7651,2:3:4]"));

#endif

}

void CDiscoverImpl::expired(CBase* /*source*/)
{
	CALLSTACKITEM_N(_CL("CDiscoverImpl"), _CL("expired"));
	if (iTimeOutCount>5) {
		post_error(_L("too many timeouts in Bt discovery"), KErrTimedOut, GetTime());
		User::Leave(-1029);
	}
	iTimeOutCount++;
	auto_ptr<CBTMCMSettings> BtSettings(0);
	TInt err; TBool got_own_address=EFalse;
#if !defined(__S60V2FP3__) && defined(__USE_PLUGIN_APIS__) && !defined(__WINS__)
	TRAP(err, BtSettings.reset(CBTMCMSettings::NewL(0)));
	if (BtSettings.get()) {	
#ifndef __S60V3__
		BtSettings->SetPowerState(EFalse, EFalse);
#else
		BtSettings->SetPowerState(EFalse);
#endif
		iReseting=ETrue;
	}
#else
	err=KErrNotSupported;
#endif
	post_error(_L("timeout in Bt discover"), KErrTimedOut, GetTime());
	StartL();
}

void CDiscoverImpl::SettingChanged(TInt /*Setting*/)
{
	TInt prev=iInterval;
	iInterval=5*60;
	Settings().GetSettingL(SETTING_BT_SCAN_INTERVAL, iInterval);
	if (iInterval>2000) iInterval=2000;
	if (prev==0 && iInterval!=0 && current_state==IDLE) {
		iLeaseTimeInSeconds=iInterval+60;
		StartL();
	}
}
