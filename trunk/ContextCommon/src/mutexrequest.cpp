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
#include "mutexrequest.h"
#include "symbian_auto_ptr.h"

/*
 * OK: this isn't actually quite safe
 * if this thread gets killed while the other
 * is waiting on the mutex, it never gets Signal()ed
 *
 * we assume that clients will eventually close all
 * handles to the mutex, retry and so recover
 *
 */

EXPORT_C CMutexRequest* CMutexRequest::NewL(MApp_context& Context,
		const TDesC& aName, TTimeIntervalMicroSeconds32 aTimeOut, 
		TRequestStatus* aToSignal)
{
	CALLSTACKITEM2_N(_CL("CMutexRequest"), _CL("NewL"), &Context);

	auto_ptr<CMutexRequest> ret(new (ELeave) CMutexRequest(Context));
	ret->ConstructL();
	ret->OpenGlobalL(aName);
	ret->WaitL(aToSignal, aTimeOut);
	return ret.release();
}

CMutexRequest::CMutexRequest(MApp_context& Context) : 
	CActive(EPriorityNormal), MContextBase(Context) { }

void CMutexRequest::ConstructL()
{
	CALLSTACKITEM_N(_CL("CMutexRequest"), _CL("ConstructL"));

	CActiveScheduler::Add(this);
}

void CMutexRequest::OpenGlobalL(const TDesC& aName)
{
	CALLSTACKITEM_N(_CL("CMutexRequest"), _CL("OpenGlobalL"));

	if (iIsOpen) User::Leave(KErrAlreadyExists);
	TInt err=iMutex.OpenGlobal(aName);
	if (err!=KErrNone) {
		User::LeaveIfError(iMutex.CreateGlobal(aName, 1));
	}
	iIsOpen=ETrue;
}

void WaitLoopL(TAny* aArgs)
{
	CMutexRequest* mr=(CMutexRequest*)aArgs;
	mr->iMutex.Wait();
	mr->iParent.RequestComplete(mr->iRequestStatus, KErrNone);
}

TInt WaitLoop(TAny* aPtr)
{
        CTrapCleanup *cl;
        cl=CTrapCleanup::New();

        TInt err=0;
        CC_TRAP(err,
                WaitLoopL(aPtr));

	delete cl;

	return err;
}

void CMutexRequest::WaitL(TRequestStatus* aToSignal, TTimeIntervalMicroSeconds32 aTimeOut)
{
	CALLSTACKITEM_N(_CL("CMutexRequest"), _CL("WaitL"));

	//DebugLog(_L("CMutexRequest::WaitL"));

	if (iChildIsOpen) User::Leave(KErrAlreadyExists);
	if (!iParentIsOpen) {
		User::LeaveIfError( iParent.Open(iParent.Id()) );
		iParentIsOpen=ETrue;
	}

	if (!iIsOpen) User::Leave(KErrNotReady);
	*aToSignal=KRequestPending;
	iRequestStatus=aToSignal;

	TBuf<100> name=_L("MutextWait ");
	name.AppendNum(iParent.Id());
	name.AppendNum( (TUint)this );
	User::LeaveIfError(iChild.Create(name, WaitLoop, 8192, 8192, 64*1024, this));
	iChildIsOpen=ETrue;
	iChild.Resume();
	iWaited=ETrue;

	User::LeaveIfError( iTimer.CreateLocal() );
	iTimer.After(iStatus, aTimeOut);

	SetActive();
}

EXPORT_C CMutexRequest::~CMutexRequest()
{
	CALLSTACKITEM_N(_CL("CMutexRequest"), _CL("~CMutexRequest"));

	//DebugLog(_L("~CMutexRequest"));
	if (iChildIsOpen) {
		iChild.Kill(KErrNone);
		iChild.Close();
	}
	Cancel();
	if (iParentIsOpen) {
		iParent.Close();
	}
	if (iRequestStatus && *iRequestStatus==KRequestPending) {
		iWaited=EFalse;
		User::RequestComplete(iRequestStatus, KErrDied);
	}
	if (iWaited) iMutex.Signal();
	if (iIsOpen) iMutex.Close();
	iTimer.Close();
}

void CMutexRequest::DoCancel()
{
	CALLSTACKITEM_N(_CL("CMutexRequest"), _CL("DoCancel"));

	iTimer.Cancel();
}

void CMutexRequest::RunL()
{
	CALLSTACKITEM_N(_CL("CMutexRequest"), _CL("RunL"));

	// timed out

	iChild.Kill(KErrNone);
	iChild.Close();
	iChildIsOpen=EFalse;
	
	if (iRequestStatus && *iRequestStatus==KRequestPending) {
		iWaited=EFalse;
		User::RequestComplete(iRequestStatus, KErrTimedOut);
	}
}
