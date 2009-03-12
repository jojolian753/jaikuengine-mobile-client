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

#include "log_alarm.h"

#include "symbian_auto_ptr.h"
#include "checkedactive.h"

#define MAX_ERRORS	10

#include "csd_presence.h"
#include <t32alm.h>

/*
 * Concepts:
 * !Accessing clock alarms!
 */

class CLogAlarmImpl : public CLogAlarm {
public:
	virtual ~CLogAlarmImpl();
private:
	CLogAlarmImpl(MApp_context& Context);
	void ConstructL();

	void CheckedRunL();
	void DoCancel();
	void GetAlarm();
	void NotifyOfChange();

#ifndef __WINS__
	RAlarmServer	iAlarmServer;
#endif
	TBBTime		iValue;
	TInt		iErrorCount;
	TTime		iPrevTime; TBBTime iPrevValue;

	friend class CLogAlarm;
};

EXPORT_C CLogAlarm* CLogAlarm::NewL(MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CLogAlarm"), _CL("NewL"), &Context);

	auto_ptr<CLogAlarmImpl> ret(new (ELeave) CLogAlarmImpl(Context));
	ret->ConstructL();
	return ret.release();
}

_LIT(KClass, "CLogAlarm");

CLogAlarm::CLogAlarm(MApp_context& Context) : CCheckedActive(EPriorityNormal, KClass),
	Mlog_base_impl(Context, KAlarm, KAlarmTuple, 2*24*60*60) { }

CLogAlarmImpl::~CLogAlarmImpl()
{
	CALLSTACKITEM_N(_CL("CLogAlarmImpl"), _CL("~CLogAlarmImpl"));
	Cancel();
#ifndef __WINS__
	iAlarmServer.Close();
#endif
}

_LIT(KAlarmTime, "alarm_due");

CLogAlarmImpl::CLogAlarmImpl(MApp_context& Context) : 
	CLogAlarm(Context), iValue(KAlarmTime), iPrevValue(KAlarmTime) { }

void CLogAlarmImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLogAlarmImpl"), _CL("ConstructL"));
	Mlog_base_impl::ConstructL();
	CActiveScheduler::Add(this);

#ifndef __WINS__
	User::LeaveIfError(iAlarmServer.Connect());
#endif

	iEvent.iData.SetValue(&iValue); iEvent.iData.SetOwnsValue(EFalse);

	GetAlarm();
	post_new_value(&iValue);
	NotifyOfChange();
}

void CLogAlarmImpl::GetAlarm()
{
#ifndef __WINS__
	CALLSTACKITEM_N(_CL("CLogAlarmImpl"), _CL("GetAlarm"));
	TAlarmInfo info;
	TAlarmSetState state;

	TInt i, err=-1;
	auto_ptr<CAlarmIdArray> ids(new (ELeave) CAlarmIdArray(8));
	iAlarmServer.AlarmArrayPopulateL(*ids.get(), RAlarmServer::EArrayNext, 8);
	iValue()=TTime(0);
	for (i=0; i<ids->Count(); i++) {
		err=iAlarmServer.AlarmInfo(info, RAlarmServer::EInfoClock, ids->At(i));
		TFullName owner;
		if (err==KErrNone) {
			iAlarmServer.AlarmOwner(owner, ids->At(i));
		}
		if (err==KErrNone ) {
			state=iAlarmServer.AlarmState(info.iAlarmId);
			if (state!=EAlarmNotSet && state!=EAlarmDisabled && info.iType==EAlarmTypeClock &&
					owner.FindF(_L("Agenda"))==KErrNotFound)
				break;
		}
		if (err!=KErrNotFound) {
			User::LeaveIfError(err);
		}
		err=KErrNotFound;
	}
	if (err==KErrNone) {
		/* AlarmTime is when the alarm will next go off,
		 * e.g., after a snooze
		 */
		iValue()=info.iAlarmTime;
	}
#endif
}

void CLogAlarmImpl::NotifyOfChange()
{
#ifndef __WINS__
	CALLSTACKITEM_N(_CL("CLogAlarmImpl"), _CL("NotifyOfChange"));
	iAlarmServer.NotifyOnChange(iStatus);
	SetActive();
#endif
}

void CLogAlarmImpl::DoCancel()
{
#ifndef __WINS__
	CALLSTACKITEM_N(_CL("CLogAlarmImpl"), _CL("DoCancel"));
	iAlarmServer.NotifyOnChangeCancel();
#endif
}

void CLogAlarmImpl::CheckedRunL()
{
#ifndef __WINS__
	CALLSTACKITEM_N(_CL("CLogAlarmImpl"), _CL("CheckedRunL"));
	if (iStatus.Int() < 0) {
		if (iErrorCount++ > MAX_ERRORS) User::Leave(iStatus.Int());
	} else {
		iErrorCount=0;
	}

	GetAlarm();
	if (iPrevValue==iValue) {
		TTime now; now.HomeTime();
		if (now>(iPrevTime+TTimeIntervalHours(1))) {
			post_unchanged_value(&iValue);
			iPrevTime.HomeTime();
		}
	} else {
		post_new_value(&iValue);
		iPrevValue()=iValue();
		iPrevTime.HomeTime();
	}
	NotifyOfChange();
#endif
}
