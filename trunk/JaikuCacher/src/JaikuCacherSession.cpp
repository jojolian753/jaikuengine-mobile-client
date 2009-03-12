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

#include "JaikuCacherSession.h"
#include "CacherCommon.h"
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
CJaikuCacherSession::CJaikuCacherSession(RThread& aClient, CJaikuCacher& aServer) : CSession(aClient), iServer(aServer) { }
#else
CJaikuCacherSession::CJaikuCacherSession(CJaikuCacher& aServer) : iServer(aServer) { }
#endif

CJaikuCacherSession::~CJaikuCacherSession()
{
	CloseChunks();
	iServer.DecrementSessions();
}

void CJaikuCacherSession::CompleteMessage(TInt Code, TInt aWhich)
{
	if (ClientAlive(iMessageThreadId[aWhich])) {
		iMessage[aWhich].Complete(Code);
	}
	SetMessage(MESSAGE_CLASS(), aWhich);
}

void CJaikuCacherSession::ServiceL(const MESSAGE_CLASS& aMessage)
{
#ifdef __WINS__
	TInt dummy;
	TBreakItem b(GetContext(), dummy);
#endif
	
	CloseChunks();
	TInt which=0;
	if (aMessage.Function()==ENotifyOnContactChange) which=1;
	if (aMessage.Function()!=ECancel) 
		SetMessage(aMessage, which);

	switch (aMessage.Function())
	{
		case ETerminateJaikuCacher:
			CompleteMessage(KErrNone);
			iServer.TerminateJaikuCacher();
			break;

		case EGetCurrentContacts:
			GetCurrentContacts(0);
			break;
		case ENotifyOnContactChange:
			RequestNotifyOnContactsChange();
			break;
		case ESetContactsDatabase:
			SetContactsDatabase();
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

void CJaikuCacherSession::RequestNotifyOnContactsChange()
{
	if (!iChangePending) {
		TInt generation;
		iServer.GetCurrentContactsNameAndGeneration(iHashName, iListName, generation);
		if (generation>iLastSentGeneration) iChangePending=ETrue;
	}
	if (iChangePending) GetCurrentContacts(1);
	iChangePending=EFalse;
}

void CJaikuCacherSession::SetContactsDatabase()
{
	TFileName db;
	iMessage[0].ReadL(0, db);
	iServer.SetContactsDbName(db);
	CompleteMessage(KErrNone, 0);
}

void CJaikuCacherSession::GetCurrentContacts(TInt aWhich)
{
	TInt generation;
	if ( iMessage[aWhich]==MESSAGE_CLASS() || !ClientAlive(iMessageThreadId[aWhich]) ) return;
	iServer.GetCurrentContactsNameAndGeneration(iHashName, iListName, generation);
	
	iMessage[aWhich].WriteL(0, iHashName, 0);
	iMessage[aWhich].WriteL(1, iListName, 0);
	iLastSentGeneration=generation;
	
	CompleteMessage(KErrNone, aWhich);
}

void CJaikuCacherSession::OpenChunksL()
{
	CloseChunks();
	User::LeaveIfError(iHashChunk.OpenGlobal(iHashName, ETrue));
	User::LeaveIfError(iListChunk.OpenGlobal(iListName, ETrue));
}

void CJaikuCacherSession::CloseChunks()
{
	if (iHashChunk.Handle()) { iHashChunk.Close(); iHashChunk.SetHandle(0); }
	if (iListChunk.Handle()) { iListChunk.Close(); iListChunk.SetHandle(0); }
}

#ifndef __S60V2__
CJaikuCacherSession* CJaikuCacherSession::NewL(RThread& aClient, CJaikuCacher& aServer)
{
	CJaikuCacherSession* self = new (ELeave) CJaikuCacherSession(aClient, aServer) ;
#else
CJaikuCacherSession* CJaikuCacherSession::NewL(CJaikuCacher& aServer)
{
	CJaikuCacherSession* self = new (ELeave) CJaikuCacherSession(aServer) ;
#endif
    	CleanupStack::PushL(self) ;
    	self->ConstructL() ;
    	CleanupStack::Pop(self) ;
    	return self ;
}

void CJaikuCacherSession::ConstructL()
{
	iServer.IncrementSessions();
}

//------------------------------------------------------------------------

void CJaikuCacherSession::TerminateJaikuCacher(const MESSAGE_CLASS& /*aMessage*/)
{
	CompleteMessage(ERequestCompleted);
	iServer.TerminateJaikuCacher();
}

void CJaikuCacherSession::SetMessage(const MESSAGE_CLASS& aMessage, TInt aWhich) 
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

void CJaikuCacherSession::NotifyEvent(CJaikuCacher::TEvent aEvent)
{
	if (aEvent==CJaikuCacher::ETerminated) {
		CompleteMessage(EJaikuCacherTerminated, 0);
		CompleteMessage(EJaikuCacherTerminated, 1);
	} else if (aEvent==CJaikuCacher::EContactsChanged) {
		iChangePending=ETrue;
		GetCurrentContacts(1);
	}
}

void CJaikuCacherSession::ReportError(TJaikuCacherRqstComplete aErrorType, 
					TDesC & /*aErrorCode*/, TDesC & /*aErrorValue*/)
{
	CompleteMessage(aErrorType);
}

void CJaikuCacherSession::PanicClient(TInt aPanic) const
{
#ifndef __S60V2__
	Panic(KJaikuCacher, aPanic) ; // Note: this panics the client thread, not server
#else
	iMessage[0].Panic(KJaikuCacher, aPanic);
#endif
}


