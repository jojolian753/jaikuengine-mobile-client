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

#include "ContextNotifySession.h"
#include "NotifyCommon.h"
#include <e32svr.h>
#include <flogger.h>
#include "symbian_auto_ptr.h"
#include "app_context.h"
#include "break.h"

bool ClientAlive(TInt ThreadId)
{
	if (ThreadId==0) return false;

	RThread c; bool ret = false;
	if (c.Open(ThreadId) != KErrNone) return false;
	if (c.ExitType() == EExitPending) ret=true;
	c.Close();
	return ret;
}


#ifndef __IPCV2__
CContextNotifySession::CContextNotifySession(RThread& aClient, CContextNotify& aServer) : CSession(aClient), iServer(aServer) { }
#else
CContextNotifySession::CContextNotifySession(CContextNotify& aServer) : iServer(aServer) { }
#endif

CContextNotifySession::~CContextNotifySession()
{
	if (iIds) {
		CList<TInt>::Node* n;
		for (n=iIds->iFirst; n; n=n->Next) {
			iServer.RemoveIcon(n->Item);
		}
	}
	for (int i=0; i<10; i++) {
		delete iNotifiedBitmaps[i];
	}
	delete iIds;
    iServer.DecrementSessions();
}

void CContextNotifySession::CompleteMessage(TInt Code, TInt aWhich)
{
	if (ClientAlive(iMessageThreadId[aWhich])) {
		iMessage[aWhich].Complete(Code);
	}
	SetMessage(MESSAGE_CLASS(), aWhich);
}

void CContextNotifySession::ServiceL(const MESSAGE_CLASS& aMessage)
{
	TInt which=0;
	if (aMessage.Function()==ENotifyOnIconChange) which=1;
	if (aMessage.Function()!=ECancel) SetMessage(aMessage, which);

	switch (aMessage.Function())
	{
		case ETerminateContextNotify:
			CompleteMessage(ERequestCompleted);
			iServer.TerminateContextNotify();
			break;

		case EAddIcon:
			AddIcon();
			break;
		case ERemoveIcon:
			RemoveIcon();
			break;
		case EChangeIcon:
			ChangeIcon();
			break;
		case ENotifyOnIconChange:
			RequestNotification();
			break;
		case ESetBackground:
			SetBackground();
			break;
		case ECancel:
			{
			CompleteMessage(KErrCancel, 1);
			aMessage.Complete(0);
			break;
			}

		default :
            PanicClient(EBadRequest);
	}

}

void CContextNotifySession::RequestNotification()
{
	for (int i=0; i<10; i++) {
		if (iNotifiedBitmaps[i] ) {
			iNotifiedBitmaps[i]->Reset();
		}
	}
	if (iChangePending) ReadIcons();
}

void CContextNotifySession::ReadIcons()
{
	if ( iMessage[1]==MESSAGE_CLASS() || !ClientAlive(iMessageThreadId[1]) ) return;
	
	TInt aIds[10];
	for (int i=0; i<10; i++) {
		aIds[i]=0;
	}
	iServer.Drawer()->GetIcons(aIds, 10);
	for (int i=0; i<10; i++) {
		if (! aIds[i] ) break;
		if (! iNotifiedBitmaps[i] ) {
			iNotifiedBitmaps[i]=new (ELeave) CFbsBitmap;
		}
		iNotifiedBitmaps[i]->Duplicate(aIds[i]);
	}
	TPtrC8 p( (TUint8*)aIds, 10*sizeof(TInt) );
	iMessage[1].WriteL(0, p, 0);
	CompleteMessage(KErrNone, 1);
	iChangePending=EFalse;
}

void CContextNotifySession::SetBackground()
{
	TInt handle=(TInt)iMessage[0].Ptr0();
	CC_TRAPD(err, iServer.SetBackgroundL(handle));
	CompleteMessage(err);
}

void CContextNotifySession::AddIcon()
{
	TInt handle, mask, id=-1;
	handle=(TInt)iMessage[0].Ptr0();
	mask=(TInt)iMessage[0].Ptr1();
	TInt err;
	CC_TRAP(err, id=iServer.AddIconL(handle, mask));
	if (err==KErrNone) {
		TPckgC<TInt> idp(id);
#ifdef __IPCV2__
		CC_TRAP(err, iMessage[0].WriteL(2, idp, 0));
#else
		CC_TRAP(err, iMessage[0].WriteL(iMessage[0].Ptr2(), idp, 0));
#endif
	}
	if (err==KErrNone) {
		CC_TRAP(err, iIds->AppendL(id));
	}
	if (err!=KErrNone) {
		iServer.RemoveIcon(id);
	}
	CompleteMessage(err);
}

void CContextNotifySession::RemoveIcon()
{
	TInt id;
	id=(TInt)iMessage[0].Ptr0();
	iServer.RemoveIcon(id);
	CompleteMessage(KErrNone);
}

void CContextNotifySession::ChangeIcon()
{
	TInt handle, mask, id;
	id=(TInt)iMessage[0].Ptr2();
	handle=(TInt)iMessage[0].Ptr0();
	mask=(TInt)iMessage[0].Ptr1();
	CC_TRAPD(err, iServer.ChangeIconL(id, handle, mask));
	CompleteMessage(err);
}

void CContextNotifySession::PanicClient(TInt aPanic) const
{
#ifndef __S60V2__
	Panic(KContextNotify, aPanic) ; // Note: this panics the client thread, not server
#else
	iMessage[0].Panic(KContextNotify, aPanic);
#endif
}

#ifndef __S60V2__
CContextNotifySession* CContextNotifySession::NewL(RThread& aClient, CContextNotify& aServer)
{
	CContextNotifySession* self = new (ELeave) CContextNotifySession(aClient, aServer) ;
#else
CContextNotifySession* CContextNotifySession::NewL(CContextNotify& aServer)
{
	CContextNotifySession* self = new (ELeave) CContextNotifySession(aServer) ;
#endif
    	CleanupStack::PushL(self) ;
    	self->ConstructL() ;
    	CleanupStack::Pop(self) ;
    	return self ;
}

void CContextNotifySession::ConstructL()
{
	iIds=CList<TInt>::NewL();
	iServer.IncrementSessions();
	iChangePending=ETrue;
}

//------------------------------------------------------------------------

void CContextNotifySession::TerminateContextNotify(const MESSAGE_CLASS& /*aMessage*/)
{
	CompleteMessage(ERequestCompleted);
	iServer.TerminateContextNotify();
}

void CContextNotifySession::SetMessage(const MESSAGE_CLASS& aMessage, TInt aWhich) 
{
	if (aMessage==MESSAGE_CLASS()) {
		iMessageThreadId[aWhich]=0;
	} else {
#ifdef __S60V2__
		RThread client;
		User::LeaveIfError(aMessage.Client(client));
		iMessageThreadId[aWhich]=client.Id();
		client.Close();
#else
		iMessageThreadId[aWhich]=aMessage.Client().Id();
#endif
	}
	iMessage[aWhich]=aMessage;
}

void CContextNotifySession::NotifyEvent(CContextNotify::TEvent aEvent)
{
	if (aEvent==CContextNotify::ETerminated) {
		CompleteMessage(EContextNotifyTerminated, 0);
		CompleteMessage(EContextNotifyTerminated, 1);
	} else if (aEvent==CContextNotify::EIconsChanged) {
		iChangePending=ETrue;
		ReadIcons();
	}
}

void CContextNotifySession::ReportError(TContextNotifyRqstComplete aErrorType, 
					TDesC & /*aErrorCode*/, TDesC & /*aErrorValue*/)
{
	CompleteMessage(aErrorType);
}
