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

#include "log_battery.h"
#include "symbian_auto_ptr.h"
#include "csd_battery.h"
#include "checkedactive.h"

#ifdef __S60V2__
#include <etelmm.h>
#endif

_LIT(KBattery, "battery");
_LIT(KBatteryImpl, "CLogBatteryImpl");

class CLogBatteryImpl : public CLogBattery, public CCheckedActive {
private:
	CLogBatteryImpl(MApp_context& Context);
	void ConstructL();
	virtual ~CLogBatteryImpl();

	void CheckedRunL();
	void DoCancel();

	TBBBattery			iValue;
#ifndef __S60V2__
	RPhoneType::TBatteryInfo	iBatteryInfo;
#else
	RMobilePhone::TMobilePhoneBatteryInfoV1 iBatteryInfo;
	RMobilePhone			iPhone;
#endif

	TInt				iErrorCount;
	const CBBSensorEvent& get_value();

	friend class CLogBattery;
	friend class auto_ptr<CLogBatteryImpl>;
};

CLogBattery* CLogBattery::NewL(MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CLogBattery"), _CL("NewL"),  &Context);

	auto_ptr<CLogBatteryImpl> ret(new (ELeave) CLogBatteryImpl(Context));
	ret->ConstructL();
	return ret.release();
}

CLogBattery::CLogBattery(MApp_context& Context) : Mlog_base_impl(Context, KBattery, KBatteryTuple)
{
	CALLSTACKITEM_N(_CL("CLogBattery"), _CL("CLogBattery"));

}

CLogBatteryImpl::CLogBatteryImpl(MApp_context& Context) : CLogBattery(Context),
	CCheckedActive(EPriorityNormal, KBatteryImpl), iValue(KBattery)
{
	CALLSTACKITEM_N(_CL("CLogBatteryImpl"), _CL("CLogBatteryImpl"));

}

void CLogBatteryImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLogBatteryImpl"), _CL("ConstructL"));

	Mlog_base_impl::ConstructL();
	iEvent.iData()=&iValue; iEvent.iData.SetOwnsValue(EFalse);

#ifndef __WINS__
	CActiveScheduler::Add(this);

#ifndef __S60V2__
	Phone().BatteryInfoNotification(iStatus, iBatteryInfo);
#else
	RTelServer::TPhoneInfo info;
	User::LeaveIfError( TelServer().GetPhoneInfo( 0, info ) );
	User::LeaveIfError( iPhone.Open( TelServer(), info.iName ) );
	iPhone.NotifyBatteryInfoChange(iStatus, iBatteryInfo);
#endif
#endif
	SetActive();
}

CLogBatteryImpl::~CLogBatteryImpl()
{
	CALLSTACKITEM_N(_CL("CLogBatteryImpl"), _CL("~CLogBatteryImpl"));

	Cancel();
}

void CLogBatteryImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CLogBatteryImpl"), _CL("CheckedRunL"));

	if (iStatus.Int()==KErrNone) {
		iValue.iLevel=iBatteryInfo.iChargeLevel;
		iValue.iState=iBatteryInfo.iStatus;
	} else {
		iErrorCount++;
		if (iErrorCount>5) User::Leave(iStatus.Int());
		get_value();
	}
	iEvent.iStamp()=GetTime();
	post_new_value(iEvent);

#ifndef __S60V2__
	Phone().BatteryInfoNotification(iStatus, iBatteryInfo);
#else
	iPhone.NotifyBatteryInfoChange(iStatus, iBatteryInfo);
#endif
	SetActive();
}

void CLogBatteryImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CLogBatteryImpl"), _CL("DoCancel"));

#ifndef __S60V2__
	Phone().BatteryInfoNotificationCancel();
#else
	iPhone.Close();
	if (iStatus.Int()==KRequestPending) {
		TRequestStatus *s=&iStatus;
		User::RequestComplete(s, KErrCancel);
	}
#endif
}

const CBBSensorEvent& CLogBatteryImpl::get_value()
{
	CALLSTACKITEM_N(_CL("CLogBatteryImpl"), _CL("get_value"));

#ifndef __WINS__
#  ifndef __S60V2__
	RPhoneType::TBatteryInfo	info;
	TInt err=Phone().GetBatteryInfo(info);
#  else
 	TRequestStatus s;
	RMobilePhone::TMobilePhoneBatteryInfoV1 info;
	iPhone.GetBatteryInfo(s, info);
	User::WaitForRequest(s);
	TInt err=s.Int();
#  endif

	if (err==KErrNone) {
		iValue.iLevel=info.iChargeLevel;
		iValue.iState=info.iStatus;
	} else {
		TBuf<100> msg=_L("failed to get battery status: ");
		msg.AppendNum(err);
		post_error(msg, err);
	}
#else
	iValue.iLevel=100;
	iValue.iState=1;
#endif
	return iEvent;
}
