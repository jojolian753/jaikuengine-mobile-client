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
#include "ver.h"
#include "ContextServer.h"
#include "ContextServerSession.h"
#include "ContextCommon.h"
#include <E32SVR.H>
#include <basched.h>

#include "app_context.h"
#include "app_context_impl.h"
#include <flogger.h>
#include "callstack.h"
#include "server_startup.h"
#include "cbbsession.h"
#include "csd_idle.h"
#include "csd_event.h"
#include "csd_lookup.h"
#include "bbxml.h"
#include "reporting.h"
#include "bb_settings.h"
#include "cl_settings.h"
#include "cl_settings_impl.h"
#include "csd_userpic.h"
#include "cn_datacounter_impl.h"
#include "md5.h"
#include "bb_recovery.h"
#include "cc_schedulers.h"
#include "csd_current_app.h"

void Log(const TDesC& msg);

bool ClientAlive(TInt ThreadId);

_LIT(KLookupSubject, "CONTEXTLOOKUP");

#pragma warning(disable:4706)
_LIT(KStatus, "status");
const TUid KContextServerComponentUid= { CONTEXT_UID_CONTEXTSERVER };
const TInt KContextServerComponentId=1;

#define SETTING_PROGRESS_INCOMING 96
#define SETTING_PROGRESS_OUTGOING 97
#define SETTING_CONNECTIVITY_MODEL 101
#define CONNECTIVITY_WHEN_ACTIVITY_ONLY 0
#define CONNECTIVITY_ALWAYS_ON 1

CContextServer::CContextServer(TInt aPriority, MApp_context& Context) : SERVER_CLASS(aPriority), MContextBase(Context), iStatusVar(KStatus) { }

CContextServer::~CContextServer()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("~CContextServer"));

	CContextServerSession* session;
	iSessionIter.SetToFirst();
	while( (session = reinterpret_cast<CContextServerSession*>(iSessionIter++)) ) {
		session->Exiting();
	}
	delete iLookupHandler;
	delete iMessageHolder;
	delete iJabberClient;
	SetStatus(ContextServer::EStopped);
	delete iBBSubSession;
	delete iPresenceInfo;
	delete iSuspendTimer;
}

void CContextServer::SetStatus(ContextServer::TContextServerStatus aStatus)
{
	if (!iBBSubSession) return;
	TTime expires=GetTime(); expires+=TTimeIntervalDays(2);
	iStatusVar()=aStatus;
	CC_TRAPD(ignored, iBBSubSession->PutL(KContextServerStatusTuple, KNullDesC, &iStatusVar, expires));
}

CContextServer* CContextServer::NewL(MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CContextServer"), _CL("NewL"), &Context);

	CContextServer* contextServer = new (ELeave) CContextServer(EPriorityNormal, Context);
    	CleanupStack::PushL(contextServer);
    	contextServer->ConstructL();
    	CleanupStack::Pop(contextServer);
    	return contextServer;
}

void MakeDeviceIdL(TDes& deviceid)
{
	TBuf<50> buf;
	TTime now=GetTime();
	TInt64 seed=now.Int64();

	TUint ticks=User::TickCount();

	MD5_CTX md5;
	MD5Init(&md5);
	TBuf8<16> hash; hash.SetLength(hash.MaxLength());
	MD5Update(&md5, (TUint8*)&seed, sizeof(seed));
	MD5Update(&md5, (TUint8*)&ticks, sizeof(ticks));

	MD5Final( (TUint8*)hash.Ptr(), &md5);
	for (int i=0; i<16; i++) {
		deviceid.AppendNumFixedWidth( hash[i], EHex, 2 );
	}
}

void CContextServer::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("ConstructL"));

	iPresenceInfo = CPresenceInfo::NewL();
	iMessageHolder = CMessageHolder::NewL();

	TBuf<50> deviceid;
	Settings().GetSettingL(SETTING_DEVICE_ID, deviceid);
	if (deviceid.Length()==0) {
		MakeDeviceIdL(deviceid);
		Settings().WriteSettingL(SETTING_DEVICE_ID, deviceid);
	}
	iJabberClient = CJabber::NewL(*this, AppContext(), deviceid);
	iBBSubSession= BBSession()->CreateSubSessionL(this);
	SetStatus(ContextServer::EStarting);
	iBBSubSession->AddNotificationL(KIdleTuple, ETrue);
	iBBSubSession->AddNotificationL(KCurrentAppTuple, ETrue);
	iSuspendTimer=CTimeOut::NewL(*this);

	StartL(KContextServerName);
}

#ifndef __IPCV2__
SESSION_CLASS* CContextServer::NewSessionL(const TVersion& aVersion) const
#else
SESSION_CLASS* CContextServer::NewSessionL(const TVersion& aVersion, const MESSAGE_CLASS &aMessage) const
#endif
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("NewSessionL"));

	// check version
	if (!User::QueryVersionSupported(TVersion(KContextServMajorVersionNumber,
                                                KContextServMinorVersionNumber,
                                                KContextServBuildVersionNumber),
                                                aVersion))
	{
		User::Leave(KErrNotSupported);
	}
	
	// create new session
#ifndef __IPCV2__
	RThread client = Message().Client();
	return CContextServerSession::NewL(client, *const_cast<CContextServer*> (this));
#else
	return CContextServerSession::NewL(*const_cast<CContextServer*> (this));
#endif
}


void CContextServer::IncrementSessions()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("IncrementSessions"));

    iSessionCount++;
}

void CContextServer::DecrementSessions()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("DecrementSessions"));

    iSessionCount--;
    if (iSessionCount <= 0) {
      	iSessionCount =0;
		CC_TRAPD(err, iJabberClient->Disconnect(EFalse));
		CActiveScheduler::Stop();
	}
}

TInt CContextServer::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("CheckedRunError"));

	if (aError == KErrBadDescriptor)
	{
        	// A bad descriptor error implies a badly programmed client, so panic it;
        	// otherwise report the error to the client
        	PanicClient(Message(), ECSBadDescriptor);
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

void CContextServer::PanicClient(const MESSAGE_CLASS& aMessage, TContextServPanic aPanic)
{
	CALLSTACKITEMSTATIC_N(_CL("CContextServer"), _CL("PanicClient"));

	aMessage.Panic(KContextServer, aPanic);
}

void CContextServer::PanicServer(TContextServPanic aPanic)
{
	CALLSTACKITEMSTATIC_N(_CL("CContextServer"), _CL("PanicServer"));

    	User::Panic(KContextServer, aPanic);
}

void CContextServer::RunL()
{
	AppContext().CallStackMgr().ResetCallStack();


	CC_TRAPD(err, SERVER_CLASS::RunL());
	if (err!=KErrNone) {
		TBuf<30> msg;
		msg.Format(_L("Error in RunL: %d"), err);
		Log(msg);
		CheckedRunError(err);
	}

	AppContext().CallStackMgr().ResetCallStack();
}

_LIT(KName, "ContextServer");
void CContextServer::ThreadFunctionL()
{
	{ RThread me; me.RenameMe(KName); }
	
#ifndef __S60V3__
	#if defined (__WINS__)
	UserSvr::ServerStarted();
	#endif
#endif

	__UHEAP_MARK;

	CApp_context* c=CApp_context::NewL(false, _L("ContextServer"));
	CleanupStack::PushL(c);
	{

	// make sure the call stack item is not on the stack
	// after app context is deleted

    	// Construct active scheduler
    	CActiveScheduler* activeScheduler = new (ELeave) CDebugBaScheduler(*c);

    	CleanupStack::PushL(activeScheduler) ;

    	// Install active scheduler
    	// We don't need to check whether an active scheduler is already installed
    	// as this is a new thread, so there won't be one
    	CActiveScheduler::Install(activeScheduler);

	CBBDataFactory* bbf=CBBDataFactory::NewL();
	CleanupStack::PushL(bbf);
	c->SetBBDataFactory(bbf);

	CBBSession* bbs=CBBSession::NewL(*c, bbf);
	CleanupStack::PushL(bbs);
	c->SetBBSession(bbs);
	
	CBBRecovery* rec=CBBRecovery::NewL();
	CleanupStack::PushL(rec);
	c->SetRecovery(rec);
	rec->RegisterComponent(KContextServerComponentUid, KContextServerComponentId,
		1, gettext("Connection to Jaiku Server"));

	TClSettings defaultSettings;
	CBlackBoardSettings* s=CBlackBoardSettings::NewL(*c, 
		defaultSettings, KCLSettingsTuple);
	c->SetSettings(s);

	CDataCounter* counter=CDataCounter::NewL();
	CleanupStack::PushL(counter);
	c->SetDataCounter(counter);

    // Construct our server
	CContextServer* server=CContextServer::NewL(*c);
	CleanupStack::PushL(server);

	RSemaphore semaphore;
	TBuf<50> semaphorename; MakeServerSemaphoreName(semaphorename, KContextServerName);
	if (semaphore.CreateGlobal(semaphorename, 0)!=KErrNone) {
		User::LeaveIfError(semaphore.OpenGlobal(semaphorename));
	}
	
	// Semaphore opened ok
	semaphore.Signal();
	semaphore.Close();

	// Start handling requests
	CActiveScheduler::Start();
	CleanupStack::PopAndDestroy(6); // CServer, activeScheduler, cbbfactory, cbbsession, counter, recovery
	}
		 
	CleanupStack::PopAndDestroy(1); // appcontext

	__UHEAP_MARKEND;

}

#if defined(__WINS__)
EXPORT_C TInt ContextServerThreadFunction(TAny* aParam)
{
	return CContextServer::ThreadFunction(aParam);
}
#endif


TInt CContextServer::ThreadFunction(TAny* /*aNone*/)
{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if (cleanupStack == NULL)
	{
      	PanicServer(ECSCreateTrapCleanup);
	}
	RSemaphore process_semaphore;
	TInt err=process_semaphore.OpenGlobal(KName);
	if (err==KErrNotFound) {
		err=process_semaphore.CreateGlobal(KName, 1);
	}
	if (err!=KErrNone) goto failed;
	err=process_semaphore.Wait(30*1000*1000);
	if (err!=KErrNone) goto failed2;

	CC_TRAP2(err, ThreadFunctionL(), 0);
	if (err != KErrNone && err!=KErrDiskFull && err!=KLeaveExit && err!=KErrNoMemory && err!=KErrNotReady) {
        	PanicServer(ECSSrvCreateServer);
	}
	
failed2:
	process_semaphore.Signal(); process_semaphore.Close();
	
failed:
    	delete cleanupStack;
    	cleanupStack = NULL;

	return err;
}


//---------------------------------------------------------------------------

void CContextServer::TerminateContextServer()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("TerminateContextServer"));

	iJabberClient->Disconnect(EFalse);
	NotifySessions(ETerminated);
	Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 0);
	Settings().WriteSettingL(SETTING_PROGRESS_OUTGOING, 0);
	CActiveScheduler::Stop();
}

void CContextServer::ConnectToPresenceServer(TDesC & username, TDesC & password, TDesC & server, TUint32 accessPoint)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("ConnectToPresenceServer"));

	iAccessPoint = accessPoint;
	iUsername.Copy(username);
	iPassword.Copy(password);
	iServer.Copy(server);

	TInt model;
	Settings().GetSettingL(SETTING_CONNECTIVITY_MODEL, model);
	iJabberClient->ConnectL(username,password,server,accessPoint, model==CONNECTIVITY_WHEN_ACTIVITY_ONLY);
	SetStatus(ContextServer::EConnecting);
	
	Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 5);
	Settings().WriteSettingL(SETTING_PROGRESS_OUTGOING, 5);
}

void CContextServer::ConnectToPresenceServer(const MESSAGE_CLASS &aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("ConnectToPresenceServer"));

#ifndef __IPCV2__
	const TAny* clientsDescriptor0 = aMessage.Ptr0();
	const TAny* clientsDescriptor1 = aMessage.Ptr1();
	const TAny* clientsDescriptor2 = aMessage.Ptr2();
	const TAny* clientsDescriptor3 = aMessage.Ptr3();
#else
	const TInt clientsDescriptor0 = 0;
	const TInt clientsDescriptor1 = 1;
	const TInt clientsDescriptor2 = 2;
	const TInt clientsDescriptor3 = 3;
#endif
	TBuf<50> username;
	aMessage.ReadL(clientsDescriptor0, username);

	TBuf<50> password;
	aMessage.ReadL(clientsDescriptor1, password);

	TBuf<50> server;
	aMessage.ReadL(clientsDescriptor2, server);

	TUint32 accessPointId;
	TPckg<TUint32> apPackage(accessPointId);
	aMessage.ReadL(clientsDescriptor3, apPackage,0);

	ConnectToPresenceServer(username,password,server,accessPointId);
}

void CContextServer::NotifySessions(TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("NotifySessions"));

	CContextServerSession* session=0;

	iSessionIter.SetToFirst();
	while( (session = reinterpret_cast<CContextServerSession*>(iSessionIter++)) ) {
		session->NotifyEvent(aEvent);
	}
}

void CContextServer::SuspendConnection()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("SuspendConnection"));

	//NotifySessions(EDisconnected);
	SetStatus(ContextServer::EDisconnecting);
	iCurrentProgress=0;
	Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 0);
	Settings().WriteSettingL(SETTING_PROGRESS_OUTGOING, 0);
	iJabberClient->Disconnect(ETrue);
}

void CContextServer::ResumeConnection()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("ResumeConnection"));

	SetStatus(ContextServer::EConnecting);
	iCurrentProgress=10;
	Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 10);
	Settings().WriteSettingL(SETTING_PROGRESS_OUTGOING, 10);
	TInt model;
	Settings().GetSettingL(SETTING_CONNECTIVITY_MODEL, model);
	iJabberClient->ConnectL(iUsername,iPassword,iServer,iAccessPoint, model==CONNECTIVITY_WHEN_ACTIVITY_ONLY);
}

CPresenceInfo *  CContextServer::GetPresenceInfo()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("GetPresenceInfo"));

	return iPresenceInfo;
}

CJabber * CContextServer::GetJabberClient()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("GetJabberClient"));

	return iJabberClient;
}

void CContextServer::NewValueL(TUint , const TTupleName& aName, const TDesC& , 
	const TComponentName& , const MBBData* aData)
{
	const CBBSensorEvent* ev=bb_cast<CBBSensorEvent>(aData);
	if ( aName == KCurrentAppTuple) {
		if (!ev) return;
		const TBBCurrentApp* app=bb_cast<TBBCurrentApp>(ev->iData());
		if (!app) {
			iInJaiku=EFalse;
			return;
		}
		if ( app->iUid()==KUidContextContacts.iUid ) {
			Log(_L("EVENT: in Jaiku"));
			iInJaiku=ETrue;
			if (iActive) {
				if (! TryResumeL()) return;
				iCurrentProgress=15;
				Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 15);
				iIncomingCount=0; iExpectedIncoming=-2; iExpectedIncoming2=-2;
			}
			return;
		} else {
			Log(_L("EVENT: left Jaiku"));
			TrySuspendL();
			iInJaiku=EFalse;
		}
	}
	if (! ( aName == KIdleTuple ) ) return;

	if (!ev) {
		if (! TryResumeL() ) return;
		iCurrentProgress=15;
		Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 15);
		iIncomingCount=0; iExpectedIncoming=-2; iExpectedIncoming2=-2;
		return;
	}
	const TBBUserActive* act=bb_cast<TBBUserActive>(ev->iData());
	if (!act || act->iSince()==TTime(0) || act->iActive()) {
		Log(_L("EVENT: active"));
		iActive=ETrue;
		if (iInJaiku) {
			if (! TryResumeL() ) return;
			iCurrentProgress=15;
			Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 15);
			iIncomingCount=0; iExpectedIncoming=-2; iExpectedIncoming2=-2;
		}
	} else {
		Log(_L("EVENT: inactive"));
		iActive=EFalse;
		TrySuspendL();
	}
}

void CContextServer::expired(CBase*) {
	iJabberClient->SuspendL();
	iPendingSuspend=EFalse;
	iSuspended=ETrue;
}

TBool CContextServer::TrySuspendL(TBool aForcePend)
{
	if (iSuspended) return ETrue;
	
	if (iIncomingCount < iExpectedIncoming || aForcePend) {
		if (iPendingSuspend) return EFalse;
		
		iSuspendTimer->Wait(30);
		iPendingSuspend=ETrue;
		return EFalse;
	}
	iSuspendTimer->Reset();
	TBool ret=iJabberClient->SuspendL();
	iPendingSuspend=EFalse;
	iSuspended=ETrue;
	return ret;
}

TBool CContextServer::TryResumeL()
{
	iSuspendTimer->Reset();
	iPendingSuspend=EFalse;
	if (!iSuspended) return EFalse;
	TBool ret=iJabberClient->ResumeL();
	if (ret) iJabberClient->ReadTimeout();
	iSuspended=EFalse;
	return ret;
}

void CContextServer::DeletedL(const TTupleName& aName, const TDesC& )
{
	if (! ( aName == KIdleTuple ) ) return;

	TryResumeL();
}

void CContextServer::NotifyJabberStatus(TInt st)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("NotifyJabberStatus"));

	switch (st)
	{
	case MJabberObserver::EJabberConnected:
		Log(_L("STATUS: connected"));
		iSuspended=EFalse;
		iIncomingCount=0; iExpectedIncoming=-2; iIncomingCount2=iExpectedIncoming2=0;
		Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 15);
		Settings().WriteSettingL(SETTING_PROGRESS_OUTGOING, 15);
		SetStatus(ContextServer::EConnected);
		{
			delete iLookupHandler; iLookupHandler=0;
			iLookupHandler=CLookupHandler::NewL(*this);
			iLookupHandler->GetNextLookupL();
			NotifySessions(EConnected);
		}
		break;
	case MJabberObserver::EJabberDisconnected:
		Log(_L("STATUS: disconnected"));
		iSuspended=EFalse;
		iIncomingCount=0; iExpectedIncoming=-1; iIncomingCount2=iExpectedIncoming2=0;
		Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 0);
		Settings().WriteSettingL(SETTING_PROGRESS_OUTGOING, 0);
		SetStatus(ContextServer::EDisconnected);
		if (iLookupHandler) iLookupHandler->CancelGetNext();
		NotifySessions(EDisconnected);
		break;
	case MJabberObserver::EConnectionNotAllowed:
	case MJabberObserver::EIdentificationFailed:
		Log(_L("STATUS: failed or not allowed"));
		iIncomingCount=0; iExpectedIncoming=-1; iIncomingCount2=iExpectedIncoming2=0;
		Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 0);
		Settings().WriteSettingL(SETTING_PROGRESS_OUTGOING, 0);
		SetStatus(ContextServer::EDisconnected);
		{
			CContextServerSession* session=0;
			iSessionIter.SetToFirst();
			TInt err=ECSConnectionNotAllowed;
			if (st==MJabberObserver::EIdentificationFailed) {
				err=ECSIdentificationError;
			}
			while( (session = reinterpret_cast<CContextServerSession*>(iSessionIter++)) ) {
				session->ReportError( (TContextServRqstComplete)err, _L(""), _L(""));
			}
		}
	case MJabberObserver::EJabberPendingSuspend:
		iSuspended=EFalse;
		Log(_L("STATUS: pending suspend"));
		TrySuspendL(ETrue);
		return;
	}
}

void CContextServer::NotifyNewPresenceInfo(const TDesC & from, const TDesC & info, 
					   const TTime& send_timestamp)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("NotifyNewPresenceInfo"));

#ifdef __WINS__
	RDebug::Print(from.Left(128));
	RDebug::Print(info.Left(128));

	//DEBUG
	TBuf<10> buf;
	send_timestamp.FormatL(buf, _L("%H%T%S"));
	RDebug::Print(buf);
#endif

	if (from.Compare(KThisUser)==0) {
		TInt slashpos;
		slashpos=iJabberClient->iFullNick.Locate('/');
		if (slashpos>0) {
			iPresenceInfo->Update(iJabberClient->iFullNick.Left(slashpos), 
				info, send_timestamp);
		}
	} else {
		iPresenceInfo->Update(from,info,send_timestamp);
	}

	NotifySessions(ENewPresenceInfo);
}
/*
void CContextServer::ReportError(MJabberObserver::TErrorType aErrorType, const TDesC & aErrorCode, const TDesC & aErrorValue)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("ReportError"));

	CContextServerSession* session=0;
	
	iSessionIter.SetToFirst();
	switch (aErrorType)
	{
		case EIdentificationFailed:
			while( (session = reinterpret_cast<CContextServerSession*>(iSessionIter++)) ) {
				session->ReportError(EIdentificationError, aErrorCode, aErrorValue);
			}
			break;
		default:
			while( (session = reinterpret_cast<CContextServerSession*>(iSessionIter++)) ) {
				session->ReportError(ERequestCompleted, aErrorCode, aErrorValue);
			}
			break;
	}
		

	
	ERequestCompleted = 1,
	EIdentificationError = -1,
	EServerUnreachable = -2,
	EBufferTooSmall = -3,
	EContextServerTerminated = -4

	
}
*/


void CContextServer::CancelRequest(const MESSAGE_CLASS &aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("CancelRequest"));

	switch(aMessage.Function()) {
	case EConnectToPresenceServer:
	case EResumeConnection:
		if (iLookupHandler) iLookupHandler->CancelGetNext();
		SetStatus(ContextServer::EDisconnecting);
		iJabberClient->Disconnect(ETrue);
		break;
	}
}

CMessageHolder  * CContextServer::GetMessageHolder()
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("GetMessageHolder"));

	return iMessageHolder;
}

_LIT(KCount, "count");

void CContextServer::NotifyNewMessage(const TDesC & from, const TDesC& subject, const TDesC & message)
{
	CALLSTACKITEM_N(_CL("CContextServer"), _CL("NotifyNewMessage"));

	if (subject.CompareF( KLookupSubject )) {
		Log(_L("Incoming lookup from ")); Log(from);
		Log(message);

		TBBInt c(1, KCount);
		TInt err=0;
		TTime expires; expires.HomeTime(); expires+=TTimeIntervalDays(7);
		CC_TRAP(err, iBBSubSession->PutL(KIncomingLookupUnread, KNullDesC, &c, expires) );
		if (err!=KErrNone) {
			Reporting().UserErrorLog(_L("Failed to keep lookup notification count"), err);
		}
		TPtrC8 xml8( (TUint8*)message.Ptr(), message.Size());

		auto_ptr<CBBLookup> data( CBBLookup::NewL() );
		auto_ptr<CSingleParser> parser( CSingleParser::NewL(data.get(), EFalse, ETrue) );
		CC_TRAP(err, parser->ParseL(xml8) );
		if (err!=KErrNone) return;

		TInt slashpos;
		slashpos=from.Locate('/');
		if (slashpos==KErrNotFound) {
			data->iLooker=from;
		} else {
			data->iLooker=from.Left(slashpos);
		}
		iLookupHandler->IncomingLookupL(data.get());

	} else {
		RDebug::Print(_L("NewMessage"));
		RDebug::Print(from.Left(128));
		RDebug::Print(subject.Left(128));
		RDebug::Print(message.Left(128));
		iMessageHolder->AppendMessageL(from, subject, message);
		NotifySessions(ENewMessage);
	}
}

void CContextServer::NotifyMessageSent()
{
	iLookupHandler->AckLookupL(iCurrentLookupId);
	iCurrentLookupId=0;
	iLookupHandler->GetNextLookupL();
}

void CContextServer::NotifyCanWrite()
{
	if (iLookupHandler) iLookupHandler->GetNextLookupL();
}

void CContextServer::NotifyError(const class MErrorInfo* aError)
{
	Recovery().ReportError(KContextServerComponentUid, KContextServerComponentId,
		aError);
}

void CContextServer::SetIncomingProgressL()
{
	{
		TBuf<100> msg=_L("PROGRESS: expected ");
		msg.AppendNum(iExpectedIncoming);
		msg.Append(_L(" received "));
		msg.AppendNum(iIncomingCount);
		msg.Append(_L(" expected2 "));
		msg.AppendNum(iExpectedIncoming2);
		msg.Append(_L(" received2 "));
		msg.AppendNum(iIncomingCount2);
		Log(msg);
	}
	if (iExpectedIncoming>=0) {
		if (iIncomingCount<iExpectedIncoming) {
			TReal p=iIncomingCount;
			p/=iExpectedIncoming;
			
			TInt pi=0;
			if (iExpectedIncoming2<0) {
				p*=70.0;
				pi=30+p;
			} else {
				p*=50.0;
				pi=50+p;
			}
			if ( abs(iCurrentProgress-pi)>2) {
				Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, pi);
				iCurrentProgress=pi;
			}
			if (iCurrentProgress==100) iCurrentProgress=99;
		} else if (iIncomingCount>=iExpectedIncoming) {
			if (iCurrentProgress<100) {
				iCurrentProgress=100;
				Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 100);
			}
			if (iPendingSuspend) TrySuspendL();
		}
	} else if(iExpectedIncoming2>=0) {
		if (iIncomingCount2<iExpectedIncoming2) {
			TReal p=iIncomingCount2;
			p/=iExpectedIncoming2;
			p*=35.0;
			TInt pi=15+p;
			if ( abs(iCurrentProgress-pi)>2) {
				Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, pi);
				iCurrentProgress=pi;
			}
			if (iCurrentProgress==50) iCurrentProgress=49;
		} else if (iIncomingCount2>=iExpectedIncoming2) {
			if (iCurrentProgress<50) {
				iCurrentProgress=50;
				Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 50);
			}
		}
	} else if(iExpectedIncoming!=-2) {
		if (iCurrentProgress!=100) {
			Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 100);
			iCurrentProgress=100;
		}
		if (iPendingSuspend) TrySuspendL();
	} else {
		iCurrentProgress=15;
		Settings().WriteSettingL(SETTING_PROGRESS_INCOMING, 15);
	}
}
void CContextServer::NotifyContextObject(MBBData* aObject, TInt aError, const TDesC& aName)
{
	const CBBTuple* tuple=bb_cast<CBBTuple>(aObject);
	if (iSuspendTimer->IsActive()) iSuspendTimer->Wait(30);
	if (tuple) {
		if (iExpectedIncoming2<0 || iExpectedIncoming2<=iIncomingCount2) {
			iIncomingCount++;
		} else {
			iIncomingCount2++;
		}
		SetIncomingProgressL();
		TUint id=tuple->iTupleId();
		const CBBSensorEvent* ev=bb_cast<CBBSensorEvent>(tuple->iData());
		TTupleName tn={tuple->iTupleMeta.iModuleUid(), tuple->iTupleMeta.iModuleId()};
		TUint uid3=tn.iModule.iUid;
		GetBackwardsCompatibleUid(uid3);
		tn.iModule.iUid=uid3;
		
		{
			TBuf<100> msg=_L("TUPLE: ");
			msg.AppendNum(tn.iModule.iUid, EHex);
			msg.Append(_L(" "));
			msg.AppendNum(tn.iId);
			msg.Append(_L(" "));
			msg.Append(tuple->iTupleMeta.iSubName());
			Log(msg);
		}
		if (ev) {
			const TBBUserGiven* given=bb_cast<TBBUserGiven>(ev->iData());
			if (given) {
				Settings().WriteSettingL(SETTING_OWN_DESCRIPTION_TIME, given->iSince());
				Settings().WriteSettingL(SETTING_OWN_DESCRIPTION, given->iDescription());
				if (id>0) iJabberClient->AckObjectL(id);
				return;
			}
		} 
		
		if (tn == KIncomingPresence) {
			const CBBPresence* p=bb_cast<CBBPresence>(tuple->iData());
			if (p) {
				iPresenceInfo->Update( tuple->iTupleMeta.iSubName(), p, EFalse);
				if (id>0) iJabberClient->AckObjectL(id);
				return;
			}
		}
		// the server does send reasonable timestamps now
		// but it's still not a bad idea to check them forward
		
		TTime expires=tuple->iExpires();
		TTime defexp; defexp=GetTime(); defexp+=TTimeIntervalDays(14);
		if (expires<defexp) expires=defexp;
		
		if (! tuple->iData()) {
			BBSession()->DeleteL(tn, tuple->iTupleMeta.iSubName(), ETrue);
		} else {
			BBSession()->PutL(tn, tuple->iTupleMeta.iSubName(), tuple->iData(), expires);
		}
		if (id>0) iJabberClient->AckObjectL(id);
		return;
	}
	if (aName==_L("ack") || aName==_L("http://www.cs.helsinki.fi/group/context ack")) {
		const TBBUint* ack=bb_cast<TBBUint>(aObject);
		if (ack) {
			iLookupHandler->Ack( (*ack)() );
		}
		return;
	}
	
	TBool msgcount=EFalse, msgcount2=EFalse;
	if (aName==_L("messagecount") || aName==_L("http://www.cs.helsinki.fi/group/context messagecount")) msgcount=ETrue;
	if (aName==_L("messagecount2") || aName==_L("http://www.cs.helsinki.fi/group/context messagecount2")) msgcount2=ETrue;
	
	if (msgcount || msgcount2) {
		iJabberClient->NoReadTimeout();
		const TBBUint* count=bb_cast<TBBUint>(aObject);
		if (count) {
			TBuf<30> msg=_L("got messagecount");
			if (msgcount2) msg.Append(_L("2 "));
			else msg.Append(_L(" "));
			
			msg.AppendNum((*count)());
			Log(msg);
			
			if (msgcount)
				iExpectedIncoming=(*count)();
			else
				iExpectedIncoming2=(*count)();
			SetIncomingProgressL();
		}
		return;
	}
}

void CContextServer::OutgoingLookupL(TUint aId, const MBBData* aData)
{
	Log(_L("Outgoing lookup"));
	iCurrentLookupId=aId;
	const CBBLookup* l=bb_cast<CBBLookup>(aData);
	if (!l) User::Leave(KErrNotSupported);

	auto_ptr<CXmlBufExternalizer> xml(CXmlBufExternalizer::NewL(2048));
	xml->iOffset=iJabberClient->GetTimeOffset();
	aData->IntoXmlL(xml.get());
	Log(xml->Buf());

	iJabberClient->SendMessageL( l->iLooked(), KLookupSubject, xml->Buf());
}

void CContextServer::OutgoingTuple(const class CBBTuple* aTuple)
{
	iJabberClient->SendContextObjectL(aTuple);
}
