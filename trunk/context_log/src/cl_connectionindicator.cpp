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

#include "cl_connectionindicator.h"

#include <JaikuConnection.mbg>

#include "bbdata.h"
#include "cbbsession.h"
#include "concretedata.h"
#include "contextcommon.h"
#include "notifystate.h"
#include "reporting.h"
#include "settings.h"
#include "cl_settings.h"
#include "csd_connectionstate_enums.h"
#include "csd_connectionstate.h"


#include <akniconsrvclient.h>

_LIT(KName, "CConnectionIndicatorImpl");
_LIT(KFunctionality, "Connection status icon"); 
const TInt KComponentId = 1;  // see ..\ids.txt 

_LIT(KIconFile, "c:\\system\\data\\JaikuConnection.mif");


/**
 * This class listens for connection status changes and shows 
 * correct icon in idle. 
 */
 
class CConnectionIndicatorImpl : public CConnectionIndicator, 
								 public MContextBase,
								 public MBBObserver,
								 public MSettingListener
{
public:

	CConnectionIndicatorImpl() : CConnectionIndicator(KName) {}

	~CConnectionIndicatorImpl() 
	{
		Cancel();
		iAnimTimer.Close();
		delete iBBSubSession;
		delete iNotifyState;
		RAknIconSrvClient::Disconnect();
	}
	
	virtual void ComponentId(TUid& aComponentUid, TInt& aComponentId, TInt& aVersion) 
	{
		aComponentUid=KUidcontext_log;
		aComponentId=KComponentId;
		aVersion=3;
	}
	
	const TDesC& Name() { return KName; }
	const TDesC& HumanReadableFunctionality() { return KFunctionality; }

	void InnerConstructL();
	void StartL();
	void StopL();
	void ComponentRunL();
	void ComponentCancel();
	void SelectIconL();
	
	void CompleteSelf();
	
	// from MBBObserver
	void NewValueL(TUint aId,
				   const TTupleName& aName, 
				   const TDesC& aSubName, 
				   const TComponentName& aComponentName, 
				   const MBBData* aData);
	void DeletedL(const TTupleName& , const TDesC& )
	{
	}

	void ScreenChanged();

protected: // from MSettingListener
	void SettingChanged(TInt aSetting);
		
private:
	/**
	 * BlackBoardSubSession is used to listen for connection states
	 */ 
	CBBSubSession	*iBBSubSession; // own
	
	/**
	 * CNotifyState is used to draw icon to screen
	 */ 
	CNotifyState* iNotifyState; // own

	/** 
	 * iConnectionState stores state after NewValueL-callback, 
	 * because all possibly leaving code needs to be done in ComponentRunL
	 */
	TInt iConnectionState;
	PresencePublisher::EState iPPState;

	/**
	 * iTimer is used for animation 
	 */
	RTimer iAnimTimer;


	TInt iPrevIcon;

	/**
	 * Jaiku Online/Offline user setting
	 */ 
	TBool iOnline, iEnabled;
};


void CConnectionIndicatorImpl::InnerConstructL()
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("InnerConstructL"));
	iBBSubSession=BBSession()->CreateSubSessionL(this);

	/**
	 * Connection to Avkon Icon Server has to be open in thread before 
	 * AknIconUtils can be used. (Normally app framework does this)
	 */ 
	User::LeaveIfError( RAknIconSrvClient::Connect() );

	User::LeaveIfError( iAnimTimer.CreateLocal() );	   
}

void CConnectionIndicatorImpl::StartL()
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("StartL"));

	// manual online/offline 
	Settings().GetSettingL(SETTING_PRESENCE_ENABLE, iEnabled);
	Settings().NotifyOnChange(SETTING_PRESENCE_ENABLE, this);

	iBBSubSession->AddNotificationL(KContextServerStatusTuple);
	iBBSubSession->AddNotificationL(KConnectionStateTuple);
	iPPState=PresencePublisher::EConnected;
	iNotifyState = CNotifyState::NewL(AppContext(), KIconFile);

	SelectIconL();
}
	
void CConnectionIndicatorImpl::StopL()
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("StopL"));
	delete iNotifyState;
	iNotifyState = NULL;
	iBBSubSession->DeleteNotifications();

	Settings().CancelNotifyOnChange(SETTING_PRESENCE_ENABLE, this);
		
	Cancel();
}


enum
	{
		EConnection = 0,
		EConnectionOff,
		EConnectionBroken,
		EConnecting1,
		EConnecting2,
		EConnectionOffline
	};

const TInt KIconIds[6] = { EMbmJaikuconnectionConnection, 
						   EMbmJaikuconnectionConnection_off,
						   EMbmJaikuconnectionConnection_broken,
						   EMbmJaikuconnectionConnecting1,
						   EMbmJaikuconnectionConnecting2,
						   EMbmJaikuconnectionConnection_offline };

const TInt KMaskIds[6] = { EMbmJaikuconnectionConnection_mask, 
						   EMbmJaikuconnectionConnection_off_mask,
						   EMbmJaikuconnectionConnection_broken_mask,
						   EMbmJaikuconnectionConnecting1_mask,
						   EMbmJaikuconnectionConnecting2_mask,
						   EMbmJaikuconnectionConnection_offline_mask };



void CConnectionIndicatorImpl::SelectIconL()
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("SelectIconL"));
	if ( ! iNotifyState ) 
		return; // not started yet

	TInt icon = EConnectionOff;
	
	if ( iEnabled )
		{
			TBool online =
				iPPState==PresencePublisher::EConnecting ||
				iPPState==PresencePublisher::EConnected ||
				iPPState==PresencePublisher::EEventListening ||
				iPPState==PresencePublisher::EResuming ||
				iPPState==PresencePublisher::EQueuingUpdate ||
				iPPState==PresencePublisher::ESendingUpdate ||
				iPPState==PresencePublisher::EReconnecting;

			if ( online ) {
				// Connection state describes mroe 
				
				switch ( iConnectionState ) {
				case ContextServer::EUnknown:
				case ContextServer::EStarting:
				case ContextServer::EStopped:
					icon = EConnectionOff;
					break;
				case ContextServer::EConnecting:
					if ( iPrevIcon == EConnecting1 )
						icon = EConnecting2;
					else
						icon = EConnecting1;
					break;
				case ContextServer::EConnected:
					icon = EConnection;
					break;
				case ContextServer::EDisconnecting:
				case ContextServer::EDisconnected:
					icon = EConnectionBroken;
					break;
				};
			}
			else {
				switch ( iPPState )
					{
					case PresencePublisher::EOffline:
					case PresencePublisher::ECallInProgress: 
					case PresencePublisher::ELowSignal:
						icon = EConnectionBroken; 
						break;
// 					case PresencePublisher::EDisabled:
// 						icon = EConnectionOffline; 
// 						break;						
					default:
						icon = EConnectionOff;
						break;
					}
			}
		}
	else
		icon = EConnectionOffline;
	
	iNotifyState->SetCurrentState( KIconIds[icon], KMaskIds[icon] );
	iPrevIcon = icon;
	if (icon == EConnecting1 || icon == EConnecting2 )
		{
			iAnimTimer.After( iStatus, TTimeIntervalMicroSeconds32(500 * 1000) );
			SetActive();
		}
}

void CConnectionIndicatorImpl::ComponentRunL()
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("ComponentRunL"));
	SelectIconL();
}


void CConnectionIndicatorImpl::ComponentCancel()
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("ComponentCancel"));
	iAnimTimer.Cancel();
}


void CConnectionIndicatorImpl::NewValueL(TUint aId,
			   const TTupleName& aName, 
			   const TDesC& aSubName, 
			   const TComponentName& aComponentName, 
			   const MBBData* aData)
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("NewValueL"));
	if (aName==KContextServerStatusTuple) {
		const TBBInt* value=bb_cast<TBBInt>(aData);
		if (!value) return;
		Cancel();
		iConnectionState = (*value)();
		SelectIconL();
	} else if (aName==KConnectionStateTuple && aSubName==PresencePublisher::KSubname) {
		const CBBConnectionState* state=bb_cast<CBBConnectionState>(aData);
		if (!state) return;
		Cancel();
		iPPState=(PresencePublisher::EState)state->iState();
		SelectIconL();
	}
}

void CConnectionIndicatorImpl::ScreenChanged()
{
	if ( iActiveState==MRecovery::ERunning) {
		Cancel();
		SelectIconL();
	}
}
	

void CConnectionIndicatorImpl::SettingChanged(TInt aSetting) 
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("SettingChanged"));
	switch ( aSetting )
		{
		case SETTING_PRESENCE_ENABLE:
			Settings().GetSettingL(SETTING_PRESENCE_ENABLE, iEnabled);
			SelectIconL();
			break;
		}
};


void CConnectionIndicatorImpl::CompleteSelf()
{
	CALLSTACKITEM_N(_CL("CConnectionIndicatorImpl"), _CL("CompleteSelf"));
	TRequestStatus* pStat = &iStatus;
	iStatus=KRequestPending;
	SetActive();
	User::RequestComplete(pStat, KErrNone);
}



CConnectionIndicator* CConnectionIndicator::NewL()
{
	CALLSTACKITEM_N(_CL("CConnectionIndicator"), _CL("NewL"));
	auto_ptr<CConnectionIndicatorImpl> self( new (ELeave) CConnectionIndicatorImpl);
	self->CComponentBase::ConstructL();
	return self.release();
}
