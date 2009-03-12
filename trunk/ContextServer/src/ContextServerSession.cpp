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

#include "ContextServerSession.h"
#include "ContextCommon.h"
#include <e32svr.h>
#include <flogger.h>
#include "symbian_auto_ptr.h"
#include "util.h"

bool ClientAlive(TInt ThreadId)
{
	CALLSTACKITEM_N(_CL("RDebug"), _CL("Print"));

	if (ThreadId==0) return false;

	RThread c; bool ret = false;
	if (c.Open(ThreadId) != KErrNone) return false;
	if (c.ExitType() == EExitPending) ret=true;
	c.Close();
	return ret;
}

#ifndef __IPCV2__
CContextServerSession::CContextServerSession(RThread& aClient, CContextServer& aServer) : CSession(aClient), iServer(aServer) { }
#else
CContextServerSession::CContextServerSession(CContextServer& aServer) : iServer(aServer) { }
#endif

CContextServerSession::~CContextServerSession()
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("~CContextServerSession"));

	if (!iExiting) iServer.GetPresenceInfo()->RemoveFlags(&iFlags);
	iFlags.Close();
    if (!iExiting) iServer.DecrementSessions();
}

void CContextServerSession::Exiting() {
	iExiting=ETrue;
}

void CContextServerSession::CompleteMessage(TInt Code)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("CompleteMessage"));

	if (ClientAlive(iMessageThreadId)) {
		iMessage.Complete(Code);
	}
	SetMessage(MESSAGE_CLASS());
}

void CContextServerSession::ServiceL(const MESSAGE_CLASS& aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("ServiceL"));

	if (aMessage.Function()!=ECancel) SetMessage(aMessage);

	if ( (aMessage.Function()==EUpdateUserPresence  || aMessage.Function()==ENotifyEvent) 
			&& iLastEvent!=0) {
		TInt temp=iLastEvent;
		iLastEvent=0;
		CompleteMessage(temp);
		return;
	}
	iLastEvent=0;
	switch (aMessage.Function())
	{
		case ETerminateContextServer:
			CompleteMessage(ECSRequestCompleted);
			iServer.TerminateContextServer();
			break;
			
		case EConnectToPresenceServer:
			ConnectToPresenceServer(aMessage);
			break;

		case ERequestPresenceInfo:
			HandleRequestPresenceInfo(aMessage);
			break;

		case ERequestPresenceNotification:
			HandleRequestPresenceNotification(aMessage);
			break;
		case ERequestMessageNotification:
			HandleRequestMessageNotification(aMessage);
			break;
		case ENotifyEvent:
			// don't do anything, if there's already
			// an event, it's handled above
			break;

		case EUpdateUserPresence:
			UpdateUserPresence(aMessage);
			break;

		case ESuspendConnection:
			SuspendConnection(aMessage);
			break;

		case EResumeConnection:
			ResumeConnection(aMessage);
			break;
		
		case ECancel:
			{
			MESSAGE_CLASS prev=iMessage;
			CompleteMessage(KErrCancel);
			iServer.CancelRequest(prev);
			aMessage.Complete(0);
			break;
			}

		default :
            	PanicClient(aMessage, ECSBadRequest);
	}

}

void CContextServerSession::PanicClient(const MESSAGE_CLASS& aMessage, TInt aPanic) const
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("PanicClient"));

#ifndef __IPCV2__
	Panic(KContextServer, aPanic);
#else
	aMessage.Panic(KContextServer, aPanic);
#endif
}

#ifndef __IPCV2__
CContextServerSession* CContextServerSession::NewL(RThread& aClient, CContextServer& aServer)
#else
CContextServerSession* CContextServerSession::NewL(CContextServer& aServer)
#endif
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("NewL"));

#ifndef __IPCV2__
	CContextServerSession* self = new (ELeave) CContextServerSession(aClient, aServer);
#else
	CContextServerSession* self = new (ELeave) CContextServerSession(aServer);
#endif
    	CleanupStack::PushL(self) ;
    	self->ConstructL();
    	CleanupStack::Pop(self) ;
    	return self ;
}

void CContextServerSession::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("ConstructL"));

	iServer.IncrementSessions();
	iServer.GetPresenceInfo()->AddFlagsL(&iFlags);
}

//------------------------------------------------------------------------

void CContextServerSession::TerminateContextServer(const MESSAGE_CLASS& aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("TerminateContextServer"));

	CompleteMessage(ECSRequestCompleted);
	iServer.TerminateContextServer();
}

void CContextServerSession::ConnectToPresenceServer(const MESSAGE_CLASS& aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("ConnectToPresenceServer"));

	if (iServer.GetJabberClient()->IsConnected()) {
		CompleteMessage(ECSRequestCompleted);
	}
	else
	{
		iServer.ConnectToPresenceServer(aMessage);
	}
}

void CContextServerSession::SendNewMessage(TMessage aJabberMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("SendNewMessage"));

	if (aJabberMessage.contact==0 || aJabberMessage.message==0 || aJabberMessage.subject==0) return;

#ifndef __IPCV2__
	TInt len0 = iMessage.Client().GetDesMaxLength(iMessage.Ptr0());
	TInt len1 = iMessage.Client().GetDesMaxLength(iMessage.Ptr1());
	TInt len2 = iMessage.Client().GetDesMaxLength(iMessage.Ptr2());
#else
	TInt len0 = iMessage.GetDesMaxLength(0);
	TInt len1 = iMessage.GetDesMaxLength(1);
	TInt len2 = iMessage.GetDesMaxLength(2);
#endif

	if (len0 < 0 || len1 < 0 || len2 < 0) {
		CompleteMessage(KErrBadHandle);
		return;
	}

	if ( len0 < aJabberMessage.contact->Des().Length() || len1 < aJabberMessage.subject->Des().Length() 
		|| len2 < aJabberMessage.message->Des().Length())
	{		
		TPckgC<TInt> len0Package(aJabberMessage.contact->Des().Length());
#ifndef __IPCV2__
		iMessage.WriteL(iMessage.Ptr0(),len0Package,0);
#else
		iMessage.WriteL(0, len0Package, 0);
#endif
		
		TPckgC<TInt> len1Package(aJabberMessage.subject->Des().Length());
#ifndef __IPCV2__
		iMessage.WriteL(iMessage.Ptr1(),len1Package,0);
#else
		iMessage.WriteL(1,len1Package,0);
#endif

		TPckgC<TInt> len2Package(aJabberMessage.message->Des().Length());
#ifndef __IPCV2__
		iMessage.WriteL(iMessage.Ptr2(),len2Package,0);
#else
		iMessage.WriteL(2,len2Package,0);
#endif

		CompleteMessage(ECSBufferTooSmall);

		iServer.GetMessageHolder()->AppendMessageL(aJabberMessage);
	}
	else
	{
#ifndef __S60V2__
		iMessage.WriteL(iMessage.Ptr0(),*aJabberMessage.contact,0);
		iMessage.WriteL(iMessage.Ptr1(),*aJabberMessage.subject,0);
		iMessage.WriteL(iMessage.Ptr2(),*aJabberMessage.message,0);
#else
		iMessage.WriteL(0,*aJabberMessage.contact,0);
		iMessage.WriteL(1,*aJabberMessage.subject,0);
		iMessage.WriteL(2,*aJabberMessage.message,0);
#endif
		
		CompleteMessage(ECSRequestCompleted);
	}	
}

void CContextServerSession::HandleRequestPresenceInfo(const MESSAGE_CLASS& aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("HandleRequestPresenceInfo"));

	if ( !(iServer.GetJabberClient()->IsConnected()) )
	{
		CompleteMessage(ECSServerUnreachable);
		return;
	}
		
	iServer.GetPresenceInfo()->SetAllAsNew(&iFlags);

	HandleRequestPresenceNotification(aMessage);
}

void CContextServerSession::HandleRequestMessageNotification(const MESSAGE_CLASS& aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("HandleRequestMessageNotification"));

	TMessage m(iServer.GetMessageHolder()->GetMessage());
	if (m.subject && m.message && m.contact) {
		SendNewMessage(m);
	}
}

void CContextServerSession::SetMessage(const MESSAGE_CLASS& aMessage) 
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("SetMessage"));

	if (aMessage==MESSAGE_CLASS()) {
		iMessageThreadId=0;
	} else {
#ifdef __IPCV2__
		RThread client;
		aMessage.Client(client);
		iMessageThreadId=client.Id();
		client.Close();
#else
		iMessageThreadId=aMessage.Client().Id();
#endif
	}
	iMessage=aMessage;
}

void CContextServerSession::HandleRequestPresenceNotification(const MESSAGE_CLASS& aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("HandleRequestPresenceNotification"));

}

void CContextServerSession::NotifyEvent(CContextServer::TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("NotifyEvent"));

	if (!ClientAlive(iMessageThreadId)) {
		Log(_L("Storing event for next request"));
		if (aEvent==CContextServer::EDisconnected) {
			iLastEvent=ECSServerUnreachable;
		} else if (aEvent==CContextServer::ETerminated) {
			iLastEvent=EContextServerTerminated;
		}
			
		return;
	}

	if (aEvent==CContextServer::EDisconnected) {
		Log(_L("ECSServerUnreachable"));
		CompleteMessage(ECSServerUnreachable);
	} else if (aEvent==CContextServer::ETerminated) {
		CompleteMessage(EContextServerTerminated);
	} else if (aEvent==CContextServer::EConnected && (
			iMessage.Function()==EConnectToPresenceServer ||
			iMessage.Function()==EResumeConnection) ) {
		CompleteMessage(ECSRequestCompleted);
	} else if (aEvent==CContextServer::ENewPresenceInfo && (
		iMessage.Function()==ERequestPresenceNotification ||
		iMessage.Function()==ERequestPresenceInfo) ) {
		
	} else if (aEvent==CContextServer::ENewMessage && iMessage.Function()==ERequestMessageNotification) {
		SendNewMessage(iServer.GetMessageHolder()->GetMessage());
	}
}

void CContextServerSession::DisconnectFromPresenceServer(const MESSAGE_CLASS& aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("DisconnectFromPresenceServer"));

	CompleteMessage(ECSRequestCompleted);
	iServer.SuspendConnection();
}

#include "csd_presence.h"
#include "bbxml.h"
#include "break.h"

void CContextServerSession::UpdateUserPresence(const MESSAGE_CLASS & aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("UpdateUserPresence"));

#ifndef __IPCV2__
	TInt len=Client().GetDesLength(aMessage.Ptr0());
#else
	TInt len=aMessage.GetDesLength(0);
#endif
	auto_ptr<HBufC> tempPresenceInfo(HBufC::NewL(len));
	TPtr16 bufd=tempPresenceInfo->Des();
	
#ifndef __IPCV2__
	ReadL(aMessage.Ptr0(), bufd,0);
#else
	aMessage.ReadL(0, bufd);
#endif
	TBool quickrate=(TBool)aMessage.Int1();
	
	refcounted_ptr<CBBPresence> data( CBBPresence::NewL() );
	auto_ptr<CSingleParser> parser( CSingleParser::NewL(data.get(), EFalse, ETrue) );
	TPtrC8 bufd8( (const TText8*)bufd.Ptr(), bufd.Length()*2);
	CC_TRAPD(err, parser->ParseL(bufd8) );
	if (err!=KErrNone) return;
	
	auto_ptr<CXmlBufExternalizer> xml(CXmlBufExternalizer::NewL(4096));
	xml->iOffset=iServer.GetJabberClient()->GetTimeOffset();
	data->MinimalXmlL(xml.get());	
	{
		Log(_L("UpdateUserPresence"));
	}
	iServer.GetJabberClient()->SendPresenceInfoL(xml->Buf(), quickrate);
	CompleteMessage(ECSRequestCompleted);
}

void CContextServerSession::SuspendConnection(const MESSAGE_CLASS & aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("SuspendConnection"));

	CompleteMessage(ECSRequestCompleted);
	iServer.SuspendConnection();
}

void CContextServerSession::ResumeConnection(const MESSAGE_CLASS & aMessage)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("ResumeConnection"));

	iServer.ResumeConnection();
}

void CContextServerSession::ReportError(TContextServRqstComplete aErrorType, 
					const TDesC & /*aErrorCode*/, const TDesC & /*aErrorValue*/)
{
	CALLSTACKITEM_N(_CL("CContextServerSession"), _CL("ReportError"));

	CompleteMessage(aErrorType);
}
