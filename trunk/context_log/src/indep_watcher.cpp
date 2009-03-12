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

#include "indep_watcher.h"
#include "cbbsession.h"
#include "app_context.h"
#include "timeout.h"
#include "app_context_impl.h"
#include "callstack.h"
#include "break.h"

class CIndependentWatcherImpl : public CIndependentWatcher, public MContextBase,
		public MBBObserver, MTimeOut {
public:
	CTimeOut *iTimeOut;
	CBBSubSession* iBBSession;
	worker_info& iInfo;
	TInt iWatchInterval;
	TInt iErrors;
	MAppEvents& iEvents;
	CApp_context* iIndepContext;
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData) {

		if (successes>0) { tries=0; }
		successes++;
		iTimeOut->Wait(iWatchInterval);
	}
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName) { }
	void ConstructL(const TDesC& aName, const TTupleName& aWatchTuple, TInt aWatchInterval) {
		CActiveScheduler::Add(this);
		iStatus=KRequestPending;
		SetActive();
		iIndepContext=CApp_context::NewL(true, aName);
		if ( !(aWatchTuple==KNoTuple) ) {
			iBBSession=BBSession()->CreateSubSessionL(this);
			iBBSession->AddNotificationL(aWatchTuple);
			iTimeOut=CTimeOut::NewL(*this);
			iTimeOut->Wait(aWatchInterval);
			iWatchInterval=aWatchInterval;
		}
	}
	TInt tries, successes;
	void expired(CBase*) {
		// allow to miss once (e.g., if the time is moved forward)
		if (tries==0) {
			tries++;
			successes=0;
			iTimeOut->Wait(iWatchInterval);
			return;
		}
			
		RThread t; 
		User::LeaveIfError(t.Open(iInfo.worker_threadid));
		t.Kill(KErrTimedOut);
		t.Close();
	}
	CIndependentWatcherImpl(worker_info& aInfo, MAppEvents& aEvents) : 
		iInfo(aInfo), iEvents(aEvents) { }
	TRequestStatus* GetStatus() { return &iStatus; }
	~CIndependentWatcherImpl() { 
		Cancel(); 
		delete iBBSession;
		delete iTimeOut;
		delete iIndepContext;
	}
	void RunL() {
		TInt err=iStatus.Int();
		if (err==KErrNone) err=KErrGeneral;
		iStatus=KRequestPending;
		SetActive();
		TBuf<150> msg=iInfo.name;
		msg.Append(_L(" stopped, error: "));
		msg.AppendNum(err);
		msg.Append(_L(" exit type: "));
		msg.AppendNum(iInfo.exit_type);
		msg.Append(_L(" "));
		msg.Append(iInfo.exit_cat);
		msg.AppendNum(iInfo.exit_reason);
		msg.Append(_L(" callstack:"));
		iEvents.LogAppEvent(msg);
		auto_ptr<HBufC> stack(0);
		CC_TRAPD(stack_err, stack.reset(iIndepContext->CallStackMgr().GetFormattedCallStack(KNullDesC)));
		if (stack_err==KErrNone && stack.get()!=0) {
			iEvents.LogAppEvent(*stack);
		}
		iErrors++;
		if (iErrors>3) User::Leave(err);
		iInfo.restart();
		if (iWatchInterval>0) 
			iTimeOut->Wait(iWatchInterval);
	}
	void DoCancel() {
		iStatus=KErrCancel;
		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrCancel);
	}
};


CIndependentWatcher::CIndependentWatcher() : CActive(EPriorityNormal) { }

CIndependentWatcher* CIndependentWatcher::NewL(
                worker_info& aInfo, const TDesC& aName,
                MAppEvents& aCb,
                const TTupleName& aWatchTuple,
                TInt aWatchInterval)
{
	auto_ptr<CIndependentWatcherImpl> ret(new (ELeave) CIndependentWatcherImpl(
		aInfo, aCb));
	ret->ConstructL(aName, aWatchTuple, aWatchInterval);
	return ret.release();
}
