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

#include "ContextNotify.h"
#include "ContextNotifySession.h"
#include "NotifyCommon.h"
#include <E32SVR.H>
#include <basched.h>

#include <flogger.h>
#include "app_context.h"
#include "break.h"
#include "server_startup.h"

void Log(const TDesC& msg);

bool ClientAlive(TInt ThreadId);

#pragma warning(disable:4706)

CContextNotify::CContextNotify(TInt aPriority) : SERVER_CLASS(aPriority)
{

}

CContextNotify::~CContextNotify()
{
	delete iForeground;
	delete iDrawer;
	if (ws_is_open) iWsSession.Close();
}

CContextNotify* CContextNotify::NewL()
{
	CContextNotify* ContextNotifyer = new (ELeave) CContextNotify(EPriorityNormal);
	CleanupStack::PushL(ContextNotifyer);
	ContextNotifyer->ConstructL();
	CleanupStack::Pop(ContextNotifyer);
	return ContextNotifyer;
}

void CContextNotify::SetBackgroundL(TInt aHandle)
{
	iDrawer->SetBackgroundL(aHandle);
}

#include <eikenv.h>

void CContextNotify::ConstructL()
{
	RWindowGroup& wg=CEikonEnv::Static()->RootWin();
	CApaWindowGroupName* wn=CApaWindowGroupName::NewLC(CEikonEnv::Static()->WsSession());
	wn->SetHidden(ETrue);
	wn->SetWindowGroupName(wg);
	CleanupStack::PopAndDestroy();

	User::LeaveIfError(iWsSession.Connect());
	ws_is_open=true;
	iForeground=CForeground::NewL(iWsSession);

	iDrawer=CDrawer::NewL(iWsSession);

	iForeground->AddObserver(iDrawer);

	StartL(KContextNotifyName);
}

#ifndef __IPCV2__
SESSION_CLASS* CContextNotify::NewSessionL(const TVersion& aVersion) const
#else
SESSION_CLASS* CContextNotify::NewSessionL(const TVersion& aVersion, const MESSAGE_CLASS &aMessage) const
#endif
{
	// check version
	if (!User::QueryVersionSupported(TVersion(KContextNotifyMajorVersionNumber,
		KContextNotifyMinorVersionNumber,
		KContextNotifyBuildVersionNumber),
		aVersion))
	{
		User::Leave(KErrNotSupported);
	}

	// create new session
#ifndef __IPCV2__
	RThread client = Message().Client();
	return CContextNotifySession::NewL(client, *const_cast<CContextNotify*> (this));
#else
	return CContextNotifySession::NewL(*const_cast<CContextNotify*> (this));
#endif
}


void CContextNotify::IncrementSessions()
{
	iSessionCount++;
}

void CContextNotify::DecrementSessions()
{
	iSessionCount--;
	if (iSessionCount <= 0)
	{
		iSessionCount =0;
		CActiveScheduler::Stop();
	}    
}

TInt CContextNotify::CheckedRunError(TInt aError)
{
	if (aError == KErrBadDescriptor)
	{
		// A bad descriptor error implies a badly programmed client, so panic it;
		// otherwise report the error to the client
		PanicClient(Message(), EBadDescriptor);
	}
	else
	{
		Message().Complete(aError);
	}

	//
	// The leave will result in an early return from CServer::RunL(), skipping
	// the call to request another message. So do that now in order to keep the
	// server running.
	ReStart();

	return KErrNone;	// handled the error fully
}

void CContextNotify::PanicClient(const MESSAGE_CLASS& aMessage, TContextNotifyPanic aPanic)
{
	aMessage.Panic(KContextNotify, aPanic);
}

void CContextNotify::PanicServer(TContextNotifyPanic aPanic)
{
	User::Panic(KContextNotify, aPanic);
}

void CContextNotify::RunL()
{
	CC_TRAPD(err, SERVER_CLASS::RunL());
	if (err!=KErrNone) {
		TBuf<20> msg;
		msg.Format(_L("Error in RunL: %d"), err);
		Log(msg);
		CheckedRunError(err);
	}
}

_LIT(KName, "ContextNotify");
void CContextNotify::ThreadFunctionL()
{
	RSemaphore process_semaphore;
	TInt err=process_semaphore.OpenGlobal(KName);
	if (err==KErrNotFound) {
		err=process_semaphore.CreateGlobal(KName, 1);
	}
	User::LeaveIfError(err);
	err=process_semaphore.Wait(30*1000*1000);
	if (err!=KErrNone) {
		process_semaphore.Close();
		User::Leave(err);
	}

	{ RThread me; me.RenameMe(KName); }
	
#if defined (__WINS__) && !defined(EKA2)
	UserSvr::ServerStarted();
#endif

	{
		CActiveScheduler* activeScheduler = new (ELeave) CBaActiveScheduler;

		CleanupStack::PushL(activeScheduler) ;
		CActiveScheduler* prev=0;
		
		prev=CActiveScheduler::Replace(activeScheduler);

		// Construct our server
		CContextNotify* notif=CContextNotify::NewL();
		CleanupStack::PushL(notif);

		RSemaphore semaphore;
		TBuf<50> semaphorename; MakeServerSemaphoreName(semaphorename, KContextNotifyName);
		if (semaphore.CreateGlobal(semaphorename, 0)!=KErrNone) {
			User::LeaveIfError(semaphore.OpenGlobal(semaphorename));
		}

		// Semaphore opened ok
		semaphore.Signal();
		semaphore.Close();

		// Start handling requests
		CActiveScheduler::Start();
		CActiveScheduler::Replace(prev);
		CleanupStack::PopAndDestroy(2); // CServer, activeScheduler, 
	}

	process_semaphore.Signal(); process_semaphore.Close();
}

#if defined(__WINS__)
EXPORT_C TInt ContextNotifyThreadFunction(TAny* aParam)
{
	return CContextNotify::ThreadFunction(aParam);
}
#endif

#include <eikappui.h>
#include <eikenv.h>

// Need an AppUi if we have a CEikonEnv as it accesses one
// unconditionally on some devices, at least the E71.
class OurAppUi : public CEikAppUi {
 public:
  void ConstructL() {
    BaseConstructL(ENoAppResourceFile | ENoScreenFurniture);
  }
};

TInt CContextNotify::ThreadFunction(TAny* /*aNone*/)
{
	__UHEAP_MARK;
	
	//CTrapCleanup* cleanupStack = CTrapCleanup::New();
	CEikonEnv* eik=new (ELeave) CEikonEnv;
	if (eik == NULL)
	{
		PanicServer(ECreateTrapCleanup);
	}
	CC_TRAPD(err, eik->ConstructL(EFalse));

  CEikAppUi* app_ui = new OurAppUi;
	CC_TRAP(err, app_ui->ConstructL());

	CC_TRAP(err, ThreadFunctionL());
	if (err != KErrNone)
	{
		PanicServer(ESrvCreateServer);
	}

	eik->DestroyEnvironment();
	
	return KErrNone;
	
	__UHEAP_MARKEND;

}


//---------------------------------------------------------------------------

void CContextNotify::TerminateContextNotify()
{
	NotifySessions(ETerminated);
	//Log();
	CActiveScheduler::Stop();
}

void CContextNotify::NotifySessions(TEvent aEvent)
{
	CContextNotifySession* session=0;

	iSessionIter.SetToFirst();
	while( (session = reinterpret_cast<CContextNotifySession*>(iSessionIter++)) ) {
		session->NotifyEvent(aEvent);
	}
}

void CContextNotify::ReportError(TContextNotifyRqstComplete aErrorType, TDesC & aErrorCode, TDesC & aErrorValue)
{
	CContextNotifySession* session=0;

	iSessionIter.SetToFirst();
	while( (session = reinterpret_cast<CContextNotifySession*>(iSessionIter++)) ) {
		session->ReportError(aErrorType, aErrorCode, aErrorValue);
	}
}

void CContextNotify::CancelRequest(const MESSAGE_CLASS &aMessage)
{
	// there's nothing to cancel (at least yet)
	switch(aMessage.Function()) {
	default:
		break;
	}
}

TInt CContextNotify::AddIconL(TInt aIconHandle, TInt aMaskHandle)
{
	CFbsBitmap* bm=new (ELeave) CFbsBitmap;
	CleanupStack::PushL(bm);
	User::LeaveIfError(bm->Duplicate(aIconHandle));
	CFbsBitmap* mask=new (ELeave) CFbsBitmap;
	CleanupStack::PushL(mask);
	User::LeaveIfError(mask->Duplicate(aMaskHandle));
	TInt id=iDrawer->AddIconL(bm, mask);
	CleanupStack::Pop(2);
	NotifySessions(EIconsChanged);
	return id;
}

void CContextNotify::ChangeIconL(TInt aId, TInt aIconHandle, TInt aMaskHandle)
{
	CFbsBitmap* bm=new (ELeave) CFbsBitmap;
	CleanupStack::PushL(bm);
	User::LeaveIfError(bm->Duplicate(aIconHandle));
	CFbsBitmap* mask=new (ELeave) CFbsBitmap;
	CleanupStack::PushL(mask);
	User::LeaveIfError(mask->Duplicate(aMaskHandle));
	iDrawer->ChangeIconL(bm, mask, aId);
	CleanupStack::Pop(2);
	NotifySessions(EIconsChanged);
}

void CContextNotify::RemoveIcon(TInt aId)
{
	iDrawer->RemoveIcon(aId);
	NotifySessions(EIconsChanged);
}

CDrawer* CContextNotify::Drawer()
{
	return iDrawer;
}
