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

#include "log_idle.h"
#include "symbian_auto_ptr.h"
#include "csd_idle.h"

EXPORT_C CLog_idle::~CLog_idle()
{
	CALLSTACKITEM_N(_CL("CLog_idle"), _CL("~CLog_idle"));

	Cancel();
	iTimer.Close();
}

void CLog_idle::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CLog_idle"), _CL("CheckedRunL"));

	if (iStatus!=KErrNone) {
		if (User::InactivityTime().Int()>iTimeOut) {
			iCurrentState=EActive;
		} else {
			iCurrentState=EIdle;
		}
	}

	if (iCurrentState==EIdle) {
		iValue()=_L("active");
		iCurrentState=EActive;
		iTimer.Inactivity(iStatus, iTimeOut);
	} else {
		iValue()=_L("idle");
		iCurrentState=EIdle;
		iTimer.Inactivity(iStatus, 0);
	}
	iEvent.iData()=&iValue; iEvent.iStamp()=GetTime();
	post_new_value(iEvent);
	SetActive();
}

EXPORT_C CLog_idle* CLog_idle::NewL(MApp_context& Context)
{
	CALLSTACKITEMSTATIC_N(_CL("CLog_idle"), _CL("NewL"));

	auto_ptr<CLog_idle> ret(new (ELeave) CLog_idle(Context));
	ret->ConstructL();
	return ret.release();
}

void CLog_idle::DoCancel()
{
	CALLSTACKITEM_N(_CL("CLog_idle"), _CL("DoCancel"));

	iTimer.Cancel();
}

void CLog_idle::GetState()
{
	CALLSTACKITEM_N(_CL("CLog_idle"), _CL("GetState"));

	if (User::InactivityTime().Int()>iTimeOut) {
		iCurrentState=EIdle;
		iValue()=_L("idle");
	} else {
		iCurrentState=EActive;
		iValue()=_L("active");
	}
}

void CLog_idle::Start()
{
	CALLSTACKITEM_N(_CL("CLog_idle"), _CL("Start"));

	GetState();
	if (iCurrentState==EIdle) {
		iTimer.Inactivity(iStatus, 0);
	} else {
		iTimer.Inactivity(iStatus, iTimeOut);
	}
	SetActive();
}

TInt CLog_idle::CheckedRunError(TInt /*aError*/)
{
	CALLSTACKITEM_N(_CL("CLog_idle"), _CL("CheckedRunError"));

	Start();
	return KErrNone;
}

CLog_idle::CLog_idle(MApp_context& Context) : CCheckedActive(EPriorityHigh, _L("CLog_idle")), 
	Mlog_base_impl(Context, KIdle, KIdleTuple), iTimeOut(60), iValue(KIdle)
{
	CALLSTACKITEM_N(_CL("CLog_idle"), _CL("CLog_idle"));

}

void CLog_idle::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLog_idle"), _CL("ConstructL"));

	Mlog_base_impl::ConstructL();
	GetState();
	iTimer.CreateLocal();
	CActiveScheduler::Add(this);
	Start();

	post_new_value(get_value());
}
