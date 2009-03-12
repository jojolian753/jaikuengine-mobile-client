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

#ifndef __CONTEXTSERVER__
#define __CONTEXTSERVER__

#include "ver.h"

#include <e32base.h>
#include "app_context.h"

#include "ContextCommon.h"
#include "PresenceInfo.h"
#include "jabber.h"
#include "messageholder.h"

#include "contextservernotifier.h"
#include "cbbsession.h"
#include "lookup_notif.h"
#include "compat_server.h"
#include "concretedata.h"
#include "timeout.h"

_LIT(KThisUser, "USER");

class CContextServer : public SERVER_CLASS, public MJabberObserver, public MContextBase, public MBBObserver,
	public MOutgoingLookup, public MTimeOut {
public:

	static CContextServer* NewL(MApp_context& Context);
	~CContextServer();

	static TInt ThreadFunction(TAny* aNone);
	
	void IncrementSessions();
	void DecrementSessions();

	void RunL();
	TInt CheckedRunError(TInt aError);

	CContextServer(TInt aPriority, MApp_context& Context) ;

	void ConstructL() ;

	static void PanicClient(const MESSAGE_CLASS& aMessage, TContextServPanic aReason);
	static void PanicServer(TContextServPanic aReason);
	static void ThreadFunctionL();

	//-------------------------------------------------------------
	// communications ...
	//-------------------------------------------------------------
	
	void TerminateContextServer();
	void ConnectToPresenceServer();

//	void HandleRequestPresenceInfo(const MESSAGE_CLASS &aMessage);
	bool HandleRequestPresenceNotification();
	bool HandleRequestMessageNotification();

	void ConnectToPresenceServer(TDesC & username, TDesC & password, TDesC & server, TUint32 accessPoint);
	void ConnectToPresenceServer(const MESSAGE_CLASS &aMessage);
	void SuspendConnection();
	void ResumeConnection();
	
	void CancelRequest(const MESSAGE_CLASS &aMessage);

	CPresenceInfo  * GetPresenceInfo();
	CMessageHolder  * GetMessageHolder();
	CJabber * GetJabberClient();



	enum TEvent { EConnected, EDisconnected, ENewPresenceInfo, ENewMessage, ETerminated };
	void NotifyNewPresenceInfo(const TDesC & from, const TDesC & info, const TTime& stamp);
private: 
	void NotifyJabberStatus(TInt st);
	void NotifyNewMessage(const TDesC & from, const TDesC& subject, const TDesC & message);
	void NotifyCanWrite();
	virtual void NotifyMessageSent();
	virtual void NotifyContextObject(MBBData* aObject, TInt aError, const TDesC& aName);
	virtual void NotifyError(const class MErrorInfo* aError);

	virtual void OutgoingLookupL(TUint aId, const MBBData* aData);
	virtual void OutgoingTuple(const class CBBTuple* aTuple);

private:
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData);
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName) ;

	void NotifySessions(TEvent aEvent);
	void SetStatus(ContextServer::TContextServerStatus);
	TBool TrySuspendL(TBool aForcePend=EFalse);
	TBool TryResumeL();
	void expired(CBase*);

	TInt iSessionCount;
#ifndef __IPCV2__
	SESSION_CLASS *	NewSessionL(const TVersion &aVersion) const;
#else
	SESSION_CLASS *	NewSessionL(const TVersion &aVersion, const MESSAGE_CLASS &aMessage) const;
#endif
	void SetIncomingProgressL();

	CPresenceInfo *		iPresenceInfo;
	CMessageHolder *	iMessageHolder;
	CJabber *	iJabberClient;
	class CBBSubSession*	iBBSubSession;
	class CLookupHandler*	iLookupHandler;

	TUint32		iAccessPoint;
	TBuf<200>	iUsername;
	TBuf<200>	iPassword;
	TBuf<200>	iServer;
	
	TInt		iIncomingCount, iIncomingCount2, iExpectedIncoming, iExpectedIncoming2, iCurrentProgress;
	TInt		iOutgoingCount;

	TUint		iCurrentLookupId;
	TBBInt		iStatusVar;
	TBool		iPendingSuspend, iSuspended, iInJaiku, iActive;
	CTimeOut*	iSuspendTimer;
};

#endif
