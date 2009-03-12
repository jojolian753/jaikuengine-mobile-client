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

#ifndef CL_PRESENCE_PUBLISHER_H_INCLUDED
#define CL_PRESENCE_PUBLISHER_H_INCLUDED 1

#include "app_context.h"
#include "ContextClientSession.h"
//#include "ftp.h"
#include "xmlbuf.h"
#include "status_notif.h"
#include "notifystate.h"
#include "csd_presence.h"
#include "connectioninit.h"
#include "bbxml.h"
#include "settings.h"
#include "presencemaintainer.h"
#include "csd_connectionstate.h"
#include "csd_connectionstate_enums.h"

#ifdef __S60V3__
#include <Etel3rdParty.h>
#endif

class CBTDeviceList;

const TTupleName KFrozenPresenceTuple = { { CONTEXT_UID_CONTEXT_LOG }, 1 };
const TTupleName KUnfreezeTimeTuple = { { CONTEXT_UID_CONTEXT_LOG }, 2 };

class CApp_context;
class CIncomingLookupIndicator;
class CRepository;

namespace PresencePublisher {

class CPresencePublisher : public CCheckedActive, public MPresenceMaintainer,
		public MNetworkConnection, public MSettingListener, public MConnectivityCallback {
public:
	static CPresencePublisher* NewL(MApp_context& Context, i_status_notif* CallBack, 
		MPresencePublisherListener& aListener,  CBTDeviceList *aBuddyList,
		CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, CBTDeviceList *aPDABTs);
	~CPresencePublisher();

	void SuspendConnection();
	void ResumeConnection();
	bool ConnectionSuspended();
	bool Disabled();

	const CBBPresence* FrozenData() const;

	void FreezeL();
	void UnFreezeL();
	TBool IsFrozen();
private:

	CPresencePublisher(MApp_context& Context, i_status_notif* CallBack, 
		MPresencePublisherListener& aListener,  CBTDeviceList *aBuddyList,
		CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, CBTDeviceList *aPDABTs);
	void ConstructL();
	void Restart();
	void SendUpdate();
	void MakePresence();
	void QueueUpdate();

	// CCheckedActive
	void CheckedRunL();
	void DoCancel();
	virtual TInt CheckedRunError(TInt aError);

	// MTimeOut
	void expired(CBase*);

	// MSettingListener
	void SettingChanged(TInt Setting);
	void ConnectivityStateChanged();

	void GotSignificantChange(TBool frozenChanged, TBool aQuickRate=EFalse);
	void NotifyNewPresence();

	void ShowFrozen(TBool aIsFrozen);
	
	void PostConnectionState(const TDesC& aMessage=KNullDesC, 
		TInt aErrorCode=KErrNone, MErrorInfo* aError=0);
	TBool EvaluateSuspendL(TBool aEvaluateOnly=EFalse); // true if should suspend

	// data
	EState			iCurrentState;
	enum ENextOp { ENone, ESuspend, EResume, EDisable, EEnable };
	ENextOp			iNextOp;

	bool			iNewValue;
	TBool			iQuickRate;

	RContextClientSession	iSession;
	TBuf<256>		iUser, iPass, iServer;
	TBuf<256>		iFullNick;
	TBuf<50>		iPassSHA1;
	TBool			iTryPlain;
	CTimeOut*		iWait;
	TInt			iWaitTime;
	TInt			iAP;
	CXmlBufExternalizer*	iPresence;
	HBufC*			iSendPresence;
	TTime			iSentTimeStamp;

	i_status_notif*		iCallBack;
	class CApp_context*	ctx;


	MPresencePublisherListener& iListener;
	CNotifyState		*iPresenceRunning, *iFrozenIndicator;
	class CIncomingLookupIndicator* iLookupIndicator;
	CBBConnectionState	*iConnectionState;
	CConnectivityListener	*iConnectivity;
	friend class TStatePoster; TInt* iLive;
	TBool iFirst;
	CTimeOut*		iSuspendTimer;
	TBool			iOnlyWhenActive, iEnabled, iSessionIsOpen;
};

}

#endif
