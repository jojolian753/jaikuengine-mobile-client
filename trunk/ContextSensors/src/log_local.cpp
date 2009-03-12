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

#include "log_local.h"
#include "symbian_auto_ptr.h"
#include "csd_idle.h"
#include "break.h"

#define IDLE_TIME	60

EXPORT_C CLog_local* CLog_local::NewL(MApp_context& Context, const TDesC& name)
{
	CALLSTACKITEMSTATIC_N(_CL("CLog_local"), _CL("NewL"));

	auto_ptr<CLog_local> ret(new (ELeave) CLog_local(Context));
	ret->ConstructL(name);
	return ret.release();
}

EXPORT_C CLog_local::~CLog_local()
{
	CALLSTACKITEM_N(_CL("CLog_local"), _CL("~CLog_local"));

	Cancel();
	delete iTimer;
}

EXPORT_C TRequestStatus* CLog_local::RequestStatus()
{
	CALLSTACKITEM_N(_CL("CLog_local"), _CL("RequestStatus"));

	return &iStatus;
}

CLog_local::CLog_local(MApp_context& Context) : CCheckedActive(EPriorityNormal, _L("CLog_local")), 
	Mlog_base_impl(Context, KIdle, KIdleTuple, 5*24*60*60) { }

void CLog_local::ConstructL(const TDesC& /*name*/)
{
	CALLSTACKITEM_N(_CL("CLog_local"), _CL("ConstructL"));

	Mlog_base_impl::ConstructL();
	CActiveScheduler::Add(this); // add to scheduler
	iStatus=KRequestPending;
	SetActive();
	iTimer=CTimeOut::NewL(*this);
	iTimer->Wait(IDLE_TIME);

	MBBData *idle_event_d=0;
	CC_TRAPD(err, iBBSubSession->GetL(KIdleTuple, KNullDesC, idle_event_d, ETrue));
	bb_auto_ptr<MBBData> p(idle_event_d);
	const CBBSensorEvent *ev=bb_cast<CBBSensorEvent>(idle_event_d);
	if (ev) {
		const TBBUserActive* c=bb_cast<TBBUserActive>(ev->iData());
		if (c) {
			iValue=*c;
			post_new_value(&iValue);
		}
	}
}

void CLog_local::DoCancel()
{
	CALLSTACKITEM_N(_CL("CLog_local"), _CL("DoCancel"));

	if (iStatus==KRequestPending) {
		// this is unsafe: if we get pre-empted here by keycapture
		// this class should be made aware of the keycapture, zero
		// its reference to iStatus and only after that check it.
		// it's safe for the moment because of guaranteed destruction
		// order in context_log
		TRequestStatus *sp=&iStatus;
		User::RequestComplete(sp, KErrNone);
	}
}

void CLog_local::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CLog_local"), _CL("CheckedRunL"));

	if (! iTimer->IsActive() || iValue.iActive()==EFalse) {
		iValue.iSince()=GetTime();
		iValue.iActive()=ETrue;
		post_new_value(&iValue);
	}
	//RDebug::Print(_L("user active"));
	iStatus=KRequestPending;
	iTimer->Wait(IDLE_TIME);
	SetActive();
}

TInt CLog_local::CheckedRunError(TInt /*aError*/)
{
	CALLSTACKITEM_N(_CL("CLog_local"), _CL("CheckedRunError"));

	return KErrNone;
}

void CLog_local::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CLog_local"), _CL("expired"));

	iValue.iActive()=EFalse;
	iValue.iSince()=GetTime();
	post_new_value(&iValue);
}
