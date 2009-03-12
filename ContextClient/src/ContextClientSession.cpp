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

#include "ContextCommon.h"
#include "ContextClientSession.h"
#include <e32math.h>
#include "mutexrequest.h"
#include "server_startup.h"
#include "break.h"
#include "cbbsession.h"
#include "bbtypes.h"
#include "csd_lookup.h"
#include <contextcommon.mbg>
#include "notifystate.h"
#include "alerter.h"
#include "compat_server.h"

static const TUint KDefaultMessageSlots = 2;
static const TUid KServerUid3 = { CONTEXT_UID_CONTEXTSERVER }; // matches UID in server/group/contextserver.mbm file

_LIT(KContextServerFilename, "ContextServer");

_LIT(KIconFile, "c:\\system\\data\\contextcommon.mbm");

EXPORT_C RContextClientSession::RContextClientSession() : RSessionBase(), iTimeBuffer(NULL, 0, 0)
{
	
}

#if defined(__WINS__)
IMPORT_C TInt ContextServerThreadFunction(TAny* aParam);
#endif

EXPORT_C TInt RContextClientSession::ConnectToContextServer()
{
	TInt result;
	
#ifdef __WINS__
	CC_TRAP(result, StartServerL(KContextServerName, ContextServerThreadFunction));
#else
	CC_TRAP(result, StartServerL(KContextServerName, KServerUid3, KContextServerFilename));
#endif
	if (result == KErrNone)
	{
		result = CreateSession(KContextServerName,
			Version(),
			KDefaultMessageSlots);	
	}
	return result;
}

EXPORT_C TVersion RContextClientSession::Version() const
{
	return(TVersion(KContextServMajorVersionNumber,
		KContextServMinorVersionNumber,
		KContextServBuildVersionNumber));
}

//----------------------------------------------------------------------------------------

EXPORT_C void RContextClientSession::Cancel() const
{
	SendReceive(ECancel);
}

EXPORT_C void RContextClientSession::MsgTerminateContextServer(TRequestStatus& aStatus)
{
	SendReceive(ETerminateContextServer, aStatus);
}

EXPORT_C void RContextClientSession::MsgRequestEventNotification(TRequestStatus& aStatus)
{
	SendReceive(ENotifyEvent, aStatus);
}

EXPORT_C void RContextClientSession::MsgConnectToPresenceServer(const TDesC & username, 
								const TDesC & password, 
								const TDesC & server, TUint32 accessPointID, TRequestStatus& aStatus)
{
	apPackage()=accessPointID;
#ifndef __IPCV2__
	TAny* messageParameters[KMaxMessageArguments];
	messageParameters[0] = (void*)&username;
	messageParameters[1] = (void*)&password;
	messageParameters[2] = (void*)&server;
	messageParameters[3] = &apPackage;
#else
	TIpcArgs messageParameters(&username, &password, &server, &apPackage);
#endif

	SendReceive(EConnectToPresenceServer, messageParameters, aStatus);
}

EXPORT_C void RContextClientSession::MsgRequestPresenceInfo(TDes & contact, TDes & presenceInfo, TTime & send_timestamp, TRequestStatus& aStatus)
{
	iTimeBuffer.Set(reinterpret_cast<TUint8*>(&send_timestamp), sizeof(send_timestamp), sizeof(send_timestamp));
	
#ifndef __IPCV2__
	TAny* messageParameters[KMaxMessageArguments];
	messageParameters[0] = & contact;
	messageParameters[1] = & presenceInfo;
	messageParameters[2] = static_cast<TAny*>(&iTimeBuffer);
#else
	TIpcArgs messageParameters(&contact, &presenceInfo, &iTimeBuffer);
#endif
	
	SendReceive(ERequestPresenceInfo, messageParameters, aStatus);	
}

EXPORT_C void RContextClientSession::MsgRequestMessageNotification(TDes & contact, 
								   TDes& subject, 
								   TDes & message, 
								   TRequestStatus& aStatus)
{
#ifndef __IPCV2__
	TAny* messageParameters[KMaxMessageArguments];
	messageParameters[0] = & contact;
	messageParameters[1] = & subject;
	messageParameters[2] = & message;
#else
	TIpcArgs messageParameters(&contact, &subject, &message);
#endif
	
	SendReceive(ERequestMessageNotification, messageParameters, aStatus);
}

EXPORT_C void RContextClientSession::MsgRequestPresenceNotification(TDes & contact, TDes & presenceInfo, TTime & send_timestamp, TRequestStatus& aStatus)
{
	iTimeBuffer.Set(reinterpret_cast<TUint8*>(&send_timestamp), sizeof(send_timestamp), sizeof(send_timestamp));
	
#ifndef __IPCV2__	
	TAny* messageParameters[KMaxMessageArguments];
	messageParameters[0] = & contact;
	messageParameters[1] = & presenceInfo;
	messageParameters[2] = static_cast<TAny*>(&iTimeBuffer);
#else
	TIpcArgs messageParameters(&contact, &presenceInfo, &iTimeBuffer);
#endif

	SendReceive(ERequestPresenceNotification, messageParameters, aStatus);
}

EXPORT_C void RContextClientSession::MsgUpdateUserPresence(TDesC & info, TRequestStatus& aStatus, TBool aQuickRate)
{
#ifndef __IPCV2__
	TAny* messageParameters[KMaxMessageArguments];
	messageParameters[0] = & info;
	messageParameters[1] = aQuickRate;
#else
	TIpcArgs messageParameters(&info, aQuickRate);
#endif
	SendReceive(EUpdateUserPresence, messageParameters, aStatus);
}

EXPORT_C void RContextClientSession::MsgSendMessage(const TDesC& contact, const TDesC& subject, 
	const TDesC& message, TRequestStatus& aStatus)
{
	TRequestStatus* s=&aStatus;
	User::RequestComplete(s, KErrNotSupported);
}

EXPORT_C void RContextClientSession::MsgSuspendConnection(TRequestStatus& aStatus)
{
	SendReceive(ESuspendConnection, aStatus);
}

EXPORT_C void RContextClientSession::MsgResumeConnection(TRequestStatus& aStatus)
{
	SendReceive(EResumeConnection, aStatus);
}


EXPORT_C void SendLookupNotificationL(class CBBSubSession* aBBSession, 
			const TDesC& aJabberNick, const class MBBData* PresenceData)
{
	auto_ptr<CBBLookup> l(CBBLookup::NewL());
	l->iLooked=aJabberNick;
	l->iWhen=GetTime();

	const CBBPresence* pres=bb_cast<CBBPresence>(PresenceData);
	if (!pres) User::Leave(KErrNotSupported);
	*(l->iPresence)=*pres;
	l->iPresenceTimeStamp()=pres->iSentTimeStamp();

	TTime exp; exp.HomeTime(); exp+=TTimeIntervalDays(14);
	aBBSession->PutRequestL(KOutgoingLookupTuple, KNullDesC, l.get(),
		exp, KNoComponent);
}

_LIT(KCount, "count");

EXPORT_C void ResetUnreadLookupsL(class CBBSubSession* aBBSession)
{
	TBBInt c(0, KCount);
	TInt err=0;
	TTime expires; expires.HomeTime(); expires+=TTimeIntervalDays(7);
	aBBSession->PutL(KIncomingLookupUnread, KNullDesC, &c, expires);
}

class CIncomingLookupIndicatorImpl : public CIncomingLookupIndicator, public MContextBase,
	public MBBObserver {
private:
	CIncomingLookupIndicatorImpl(MApp_context& aContext) : MContextBase(aContext) { }

	CNotifyState*	iIndicator;
	CBBSubSession*	iBBSession;
	CAlerter*	iAlerter;

	void ConstructL() {
		iBBSession=BBSession()->CreateSubSessionL(this);
		iBBSession->AddNotificationL(KIncomingLookupUnread);

		iAlerter=CAlerter::NewL(AppContext());
		MBBData* existing=0;
		iBBSession->GetL(KIncomingLookupUnread, KNullDesC, existing, ETrue);
		bb_auto_ptr<MBBData> p(existing);
		if (existing) {
			const TBBInt* d=bb_cast<TBBInt>(existing);
			if (d && (*d)() > 0) { ShowIndicator(); }
		}
	}

	~CIncomingLookupIndicatorImpl() {
		delete iBBSession;
		delete iIndicator;
		delete iAlerter;
	}
	void ShowIndicator() {
		if (iIndicator) return;
		CC_TRAPD(err, iIndicator=CNotifyState::NewL(AppContext(), KIconFile));
		if (iIndicator) iIndicator->SetCurrentState(EMbmContextcommonLookup, EMbmContextcommonLookup);
	}
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData) 
	{
		const TBBInt* d=bb_cast<TBBInt>(aData);
		if (! d || (*d)()==0) {
			delete iIndicator; iIndicator=0;
		} else {
			iAlerter->ShortAlert();
			ShowIndicator();
		}
	}

	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName) {
		delete iIndicator; iIndicator=0;
	}

	friend class CIncomingLookupIndicator;
	friend class auto_ptr<CIncomingLookupIndicatorImpl>;
};

EXPORT_C CIncomingLookupIndicator* CIncomingLookupIndicator::NewL(MApp_context& aContext)
{
	auto_ptr<CIncomingLookupIndicatorImpl> ret(new (ELeave) CIncomingLookupIndicatorImpl(aContext));
	ret->ConstructL();
	return ret.release();
}

#ifndef __S60V2__
void RBBClient::SendReceive(TInt aFunction, TRequestStatus& aStatus)
{
	RSessionBase::SendReceive(aFunction, 0, aStatus);
}
void RBBClient::SendReceive(TInt aFunction)
{
	RSessionBase::SendReceive(aFunction, 0);
}
#endif


