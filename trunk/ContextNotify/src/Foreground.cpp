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

#include "break.h"
#include "foreground.h"
#include "symbian_auto_ptr.h"
#include <e32std.h>
#include <apgwgnam.h>
#include "app_context.h"
#include "timeout.h"

class CForegroundImpl : public CForeground, public MTimeOut {
	virtual void AddObserver(MForegroundObserver* aObserver);

	CForegroundImpl(RWsSession& aWsSession);

	void ConstructL();
	void RunL();
	TInt RunError(TInt aError);
	void DoCancel();
	void StartL();
	void Release();
	void Listen();
	void expired(CBase*);
	void CheckFocusL();

	RPointerArray<MForegroundObserver> iObservers;
	RWsSession&	iWsSession;
	RWindowGroup	iWg; bool iWg_is_open;
	CTimeOut	*iTimer;
	TInt		iWaitCount;
	
	friend class CForeground;
public:
	virtual ~CForegroundImpl();
};

CForeground::CForeground(CActive::TPriority aPriority) : CActive(aPriority)
{
}

CForeground::~CForeground()
{
	
}

CForeground* CForeground::NewL(RWsSession& aWsSession)
{
	auto_ptr<CForegroundImpl> ret(new (ELeave) CForegroundImpl(aWsSession));
	ret->ConstructL();
	return ret.release();
}

void CForegroundImpl::AddObserver(MForegroundObserver* aObserver)
{
	User::LeaveIfError(iObservers.Append(aObserver));
}

CForegroundImpl::CForegroundImpl(RWsSession& aWsSession) : CForeground(EPriorityHigh), iWsSession(aWsSession), iWg(aWsSession)
{
}

CForegroundImpl::~CForegroundImpl()
{
	Release();
	delete iTimer;

	iObservers.Close();
}

void CForegroundImpl::ConstructL()
{
	CActiveScheduler::Add(this);
	iTimer=CTimeOut::NewL(*this);

	StartL();
}

void CForegroundImpl::CheckFocusL()
{
	iTimer->Reset();
	TInt wgid=iWsSession.GetFocusWindowGroup();
	
	if (0) {
		CApaWindowGroupName* dummy;
		dummy=CApaWindowGroupName::NewLC(iWsSession, iWg.WindowGroupId());
		CleanupStack::PopAndDestroy();
	}
	CApaWindowGroupName* gn;
	gn=CApaWindowGroupName::NewLC(iWsSession, wgid);

	if ( (gn->AppUid().iUid==0) && (iWaitCount<10) ) {
		iWaitCount++;
		iTimer->WaitShort(100);
	} else {
		iWaitCount=0;
	}
	if (iWaitCount==0 || iWaitCount==1) {
		int i;
		for (i=0; i<iObservers.Count(); i++) {
			iObservers[i]->ForegroundChanged(gn);
		}
	}
	CleanupStack::PopAndDestroy(); // gn
}

void CForegroundImpl::RunL()
{
	TBool screench=EFalse;
	if (iStatus == KErrNone) {
		TWsEvent e;
		iWsSession.GetEvent(e);
		screench=(e.Type()==EEventScreenDeviceChanged);
	}

	if (!screench) {
		CheckFocusL();
	} else {
		int i;
		for (i=0; i<iObservers.Count(); i++) {
			iObservers[i]->ScreenChanged();
		}		
	}

	Listen();
}

void CForegroundImpl::expired(CBase*)
{
	CheckFocusL();
}

TInt CForegroundImpl::RunError(TInt /*aError*/)
{
	CC_TRAPD(err, StartL());
	return err;
}

void CForegroundImpl::DoCancel()
{
	iWsSession.EventReadyCancel();
}

void CForegroundImpl::StartL()
{
	Release();
	User::LeaveIfError(iWg.Construct((TUint32)&iWg, EFalse));
	iWg_is_open=true;
	iWg.SetOrdinalPosition(-1);
	iWg.EnableReceiptOfFocus(EFalse);

	CApaWindowGroupName* wn=CApaWindowGroupName::NewLC(iWsSession);
	wn->SetHidden(ETrue);
	wn->SetWindowGroupName(iWg);
	CleanupStack::PopAndDestroy();

	User::LeaveIfError(iWg.EnableFocusChangeEvents());
	User::LeaveIfError(iWg.EnableScreenChangeEvents());
	User::LeaveIfError(iWg.EnableGroupChangeEvents());
	Listen();
}

void CForegroundImpl::Listen()
{
	iStatus=KRequestPending;
	iWsSession.EventReady(&iStatus);
	SetActive();
}

void CForegroundImpl::Release()
{
	Cancel();
	if (iWg_is_open) {
		iWg.Close();
		iWg_is_open=false;
	}
}
