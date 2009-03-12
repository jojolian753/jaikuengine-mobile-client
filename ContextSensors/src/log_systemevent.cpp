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

#include "log_systemevent.h"
#include "symbian_auto_ptr.h"
#include "checkedactive.h"
#include <saclient.h>


#define MAX_ERRORS	10

#include "csd_system.h"

class CLog_SystemEventImpl : public CLog_SystemEvent, public CCheckedActive {
public:
	virtual ~CLog_SystemEventImpl();
private:
	CLog_SystemEventImpl(MApp_context& Context, const TDesC& name, const TTupleName& aTupleName);
	void ConstructL(TUid Uid);

	void CheckedRunL();
	void DoCancel();

	RSystemAgent	iAgent;
	TSysAgentEvent	iSysEvent;
	TInt		iErrorCount;

	TBBSysEvent	iValue;
	enum TState { EIdle, EFirstValue, EChangeNotification };
	TState		iState;

	friend class CLog_SystemEvent;
};

EXPORT_C CLog_SystemEvent::~CLog_SystemEvent() { }

EXPORT_C CLog_SystemEvent* CLog_SystemEvent::NewL(MApp_context& Context, const TDesC& name, TUid Uid, const TTupleName& aTupleName)
{
	auto_ptr<CLog_SystemEventImpl> ret(new (ELeave) CLog_SystemEventImpl(Context, name, aTupleName));
	ret->ConstructL(Uid);
	return ret.release();
}

CLog_SystemEvent::CLog_SystemEvent(MApp_context& Context, const TDesC& name, const TTupleName& aTupleName) : 
	Mlog_base_impl(Context, name, aTupleName, 2*24*60*60)
{
}

CLog_SystemEventImpl::~CLog_SystemEventImpl()
{
	Cancel();
	iAgent.Close();
}

CLog_SystemEventImpl::CLog_SystemEventImpl(MApp_context& Context, const TDesC& name, const TTupleName& aTupleName) : 
	CLog_SystemEvent(Context, name, aTupleName), 
	CCheckedActive(EPriorityNormal, _L("CLog_SystemEventImpl")), iValue(name)
{
}

void CLog_SystemEventImpl::ConstructL(TUid Uid)
{
	iValue.iUid()=Uid.iUid;
	Mlog_base_impl::ConstructL();
	CActiveScheduler::Add(this);
	User::LeaveIfError(iAgent.Connect());

	iValue.iState=iAgent.GetState( Uid );
	iSysEvent.SetUid(Uid);
	iEvent.iData.SetValue(&iValue); iEvent.iData.SetOwnsValue(EFalse);

	iState=EFirstValue;
	TRequestStatus* s=&iStatus;
	User::RequestComplete(s, KErrNone);
	SetActive();

}

void CLog_SystemEventImpl::CheckedRunL()
{
	if (iStatus!=KErrNone) {
		TBuf<20> msg=_L("err: ");
		msg.AppendNum(iStatus.Int());
		post_error(msg, iStatus.Int());
		iErrorCount++;
		if (iErrorCount>MAX_ERRORS) User::Leave(iStatus.Int());
	} else {
		if (iState==EFirstValue) {
			post_new_value(&iValue);
		} else {
			iErrorCount=0;

			iValue.iState=iSysEvent.State();
			post_new_value(&iValue);
		}
	}
	iState=EChangeNotification;
	iSysEvent.SetRequestStatus(iStatus);
	iAgent.NotifyOnEvent(iSysEvent);

	SetActive();
}

void CLog_SystemEventImpl::DoCancel()
{
	iAgent.NotifyEventCancel();
}
