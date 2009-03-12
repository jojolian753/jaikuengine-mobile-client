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

#include "ccu_progressbar.h"

#include "ccu_timeperiod.h"

#include "app_context.h"
#include "bbdata.h"
#include "cbbsession.h"
#include "concretedata.h"
#include "contextcommon.h"
#include "cc_stringtools.h"
#include "cl_settings.h"
#include "cn_networkerror.h"
#include "csd_connectionstate.h"
#include "csd_connectionstate_enums.h"
#include "settings.h"
#include "timeout.h"

_LIT(KUnknown, "Unknown");
_LIT(KReconnecting, "Reconnecting");
_LIT(KConnecting, "Connecting");
_LIT(KConnected, "Connected");
_LIT(KQueuingUpdate, "QueuingUpdate");
_LIT(KSendingUpdate, "SendingUpdate");
_LIT(KSuspending, "Suspending");
_LIT(KSuspended, "Suspended");
_LIT(KResuming, "Resuming");
_LIT(KDisabling, "Stopping");
_LIT(KDisabled, "Stopped");
_LIT(KOfflineMode, "Offline mode");
_LIT(KCallInProgress, "CallInProgress");
_LIT(KLowSignal, "LowSignal");
_LIT(KEventListening, "EventListening");

_LIT(KNotImplemented, "[Not implemented]");


/**
 * Listens changes in connection state, connection error, authentication error, network error, incoming item progress 
 * and combines them to one representable state (usually text and progress bar state)
 *
 * Connection state: 
 * enum TContextServerStatus {
 *   EUnknown,
 *   EStarting,
 *   EConnecting,
 *   EConnected,
 *   EDisconnecting,
 *   EDisconnected,
 *   EStopped
 * };
 *
 * 
 * 
 *
 */ 
class CProgressBarModelImpl : public CProgressBarModel, 
							  public MContextBase,
							  public MNetworkErrorObserver,
							  public MSettingListener,
							  public MBBObserver,
							  public MTimeOut
{	
public:

	void AddListenerL( MListener& aListener )
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("AddListenerL") );
		iListeners.AppendL( &aListener );
	}

	void RemoveListener( MListener& aListener )
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("RemoveListener") );
		TInt ix = iListeners.Find( &aListener );
		if ( ix != KErrNotFound ) iListeners.Remove( ix );
	}

	virtual const TDesC& Message(TMessageId aId) 
	{
		switch ( aId )
			{
			case EStatusMessage:  return iStatusMessage;
			case EFetchMessage:       return iFetchMessage;
			case EDevMessage: return iDevMessage;
			}
		return KNullDesC; 
	}
	
	
	virtual TInt Progress() 
	{
		return iProgress;
	}
	

	void SetFetchMessageL(const TDesC& aMsg)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("SetFetchMessageL") );
		iFetchMessage.Zero();
		SafeAppend(iFetchMessage, aMsg);
		NotifyMessageL( EFetchMessage, iFetchMessage );
	}

	CProgressBarModelImpl(CTimePeriodFormatter& aPeriodFormatter) : iPeriodFormatter( aPeriodFormatter ) {}

	~CProgressBarModelImpl()
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("~CProgressBarModelImpl") );
		iListeners.Close();
		StopL();
		delete iBBSubSession;
		delete iPPState;
	}
	
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("ConstructL") );
		StartL();
	}
	
	
	void StartL()
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("StartL"));

		// connection state
		iConnectionState = ContextServer::EUnknown;
		if ( ! iBBSubSession ) iBBSubSession = BBSession()->CreateSubSessionL(this);
		const TBool getExisting = ETrue;
		iBBSubSession->AddNotificationL(KContextServerStatusTuple, getExisting);
		iBBSubSession->AddNotificationL(KConnectionStateTuple, getExisting);
		
		// download progress
		Settings().GetSettingL(SETTING_PROGRESS_INCOMING, iProgress);
		Settings().NotifyOnChange(SETTING_PROGRESS_INCOMING, this);
		iProgress = 0;
		// authentication errors
		Settings().NotifyOnChange(SETTING_IDENTIFICATION_ERROR, this);
		SettingChanged(SETTING_IDENTIFICATION_ERROR);

		// network error 
		if ( !iNetworkError ) {
			iNetworkError=CNetworkError::NewL(*this);
		}
		
		// manual online/offline 
		Settings().GetSettingL(SETTING_PRESENCE_ENABLE, iOnline);
		Settings().NotifyOnChange(SETTING_PRESENCE_ENABLE, this);

		if ( ! iReconnectingTimer )
			iReconnectingTimer = CTimeOut::NewL(*this); 
		if ( ! iLowSignalTimer )
			iLowSignalTimer = CTimeOut::NewL(*this); 

		iDevMessage = _L("[Starting]");
		DecideStateL(ETrue);
	}


	void StopL()
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("StopL"));
		delete iReconnectingTimer;
		iReconnectingTimer = NULL;
		delete iLowSignalTimer;
		iLowSignalTimer = NULL;
		delete iNetworkError;
		iNetworkError = NULL;

		Settings().CancelNotifyOnChange(SETTING_PROGRESS_INCOMING, this);
		Settings().CancelNotifyOnChange(SETTING_IDENTIFICATION_ERROR, this);
		Settings().CancelNotifyOnChange(SETTING_PRESENCE_ENABLE, this);

		if ( iBBSubSession ) iBBSubSession->DeleteNotifications();
	}


	
	const TDesC& PPStateToDevString(TInt aState) const
	{
		// State to message 
		switch ( iPPState->iState() )
			{
			case PresencePublisher::EUnknown:       return KUnknown;
			case PresencePublisher::EReconnecting:  return KReconnecting;
			case PresencePublisher::EConnecting:    return KConnecting;
			case PresencePublisher::EConnected:     return KConnected;
			case PresencePublisher::EQueuingUpdate: return KQueuingUpdate;
			case PresencePublisher::ESendingUpdate: return KSendingUpdate;
			case PresencePublisher::ESuspending:    return KSuspending;
			case PresencePublisher::ESuspended:     return KSuspended;
			case PresencePublisher::EResuming:      return KResuming;
			case PresencePublisher::EDisabling:     return KDisabling;
			case PresencePublisher::EDisabled:      return KDisabled;
			case PresencePublisher::EOffline:       return KOfflineMode;
			case PresencePublisher::ECallInProgress: return KCallInProgress;
			case PresencePublisher::ELowSignal:     return KLowSignal;
			case PresencePublisher::EEventListening: return KNullDesC; //KEventListening;
							
			default:
				return KNotImplemented;
			}		
		return KNullDesC;
	}
	


	void AppendPPStateL(TDes& aBuf, TInt aState) const
	{
		// State to message 
		switch ( aState )
			{
			case PresencePublisher::EUnknown:       SafeAppend(aBuf, _L("Starting")); break;
			case PresencePublisher::EReconnecting:  SafeAppend(aBuf, _L("Connecting")); break;
			case PresencePublisher::EConnecting:    SafeAppend(aBuf, _L("Connecting")); break;
			case PresencePublisher::EConnected:     
			case PresencePublisher::EQueuingUpdate:
				if ( iProgress < 100 )
					SafeAppend(aBuf, _L("Fetching")); 
				break;
			case PresencePublisher::ESendingUpdate: SafeAppend(aBuf, _L("Sending update")); break;
			case PresencePublisher::ESuspending:    SafeAppend(aBuf, _L("Suspending")); break;
			case PresencePublisher::ESuspended:     SafeAppend(aBuf, _L("Suspended")); break;
			case PresencePublisher::EResuming:      SafeAppend(aBuf, _L("Resuming")); break;
			case PresencePublisher::EDisabling:     SafeAppend(aBuf, _L("Stopping")); break;
			case PresencePublisher::EDisabled:      SafeAppend(aBuf, _L("Stopped")); break;
			case PresencePublisher::EOffline:       SafeAppend(aBuf, _L("Disconnected: Offline mode")); break;
			case PresencePublisher::ECallInProgress: SafeAppend(aBuf, _L("Disconnected: Call in progress")); break;
			case PresencePublisher::ELowSignal:      SafeAppend(aBuf, _L("Disconnected: Low network signal")); break;
			case PresencePublisher::EEventListening: 
				if ( iProgress < 100 )
					SafeAppend(aBuf, _L("Fetching")); 
				break;
			default:
				SafeAppend(aBuf, _L("Unknown state, not implemented")); break;	
			}		
	}
	

	TInt ReconnectingUpdateIntervalL(const TTime& aTime, const TTime& now, const TTimePeriod& period)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("ReconnectingUpdateIntervalL"));
		TInt waitSecs = 0;
		switch ( period.iUnit )
			{
			case TTimePeriod::EYears: 
			case TTimePeriod::EMonths:
			case TTimePeriod::EWeeks:
			case TTimePeriod::EDays:
				// something wrong, just stop timer
				break;

			case TTimePeriod::EHours:
				{
					TTimeIntervalMinutes minutesIv; 
					if ( aTime.MinutesFrom( now, minutesIv ) ) { Bug(_L("minutes too far apart to fit")).Raise(); }
					TInt minutes = minutesIv.Int();
					if ( minutes < 0 ) break;
					
					TInt remainder = minutes % 60; 
					if ( remainder == 0 )
						waitSecs = period.iValue <= 1 ? 1*60 : 60*60;
					else
						waitSecs = remainder * 60;
					break;
 				}
			case TTimePeriod::EMinutes:
				{
					TTimeIntervalSeconds secondsIv; 
					if ( aTime.SecondsFrom( now, secondsIv ) ) { Bug(_L("seconds too far apart to fit")).Raise(); }
					TInt seconds = secondsIv.Int();
					
					if ( seconds < 0 ) break;
					
					TInt remainder = seconds % 60; 
					if ( remainder == 0 )
						waitSecs = period.iValue <= 1 ? 1 : 60;
					else 
						waitSecs = remainder;
					break;
 				}
			case TTimePeriod::ESeconds:
				{
					TTimeIntervalSeconds secondsIv; 
					if ( aTime.SecondsFrom( now, secondsIv ) ) { Bug(_L("seconds too far apart to fit")).Raise(); }
					TInt seconds = secondsIv.Int();
					if ( seconds < 0 ) break;
					waitSecs = 1;
					break;
 				} 
			}
		return waitSecs;
	}

	void AppendIntervalL(const TTime& aTime, TDes& aBuf)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("AppendIntervalL"));
		TBuf<40> buf;
		TTime now;
		now.HomeTime();

		TTimePeriod period = TTimePeriod::BetweenL( now, aTime, 1, 1 );
		
		iPeriodFormatter.FormatPeriodL( period, buf );
		if ( buf.Length() > 0 )
			{
				SafeAppend( aBuf, _L(" in ") );
				SafeAppend( aBuf, buf );
			}
	}		

	void SetUpdateTimerL(const TTime& aTime)
	{
		TTime now;
		now.HomeTime();
		
		TTimePeriod period = TTimePeriod::BetweenL( now, aTime, 1, 1 );
		
		TInt waitSecs = ReconnectingUpdateIntervalL( aTime, now, period );
		if ( waitSecs > 0 )
			iReconnectingTimer->Wait(waitSecs);
		else
			iReconnectingTimer->Reset();
	}

	virtual void expired(CBase* aSource)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("expired"));
		if ( iReconnectingTimer == aSource )
			{
				DecideStateL();
			}
		else if (iLowSignalTimer==aSource) {
			DecideStateL();
		}
	}

	TBool CombinePPMessageL()
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("CombinePPMessageL"));
		iOldMessage = iDevMessage;
		
		iDevMessage.Zero();
		if ( ! iPPState )
			{
				iDevMessage = _L("ERROR: Missing");
				return;
			}
		
		SafeAppend( iDevMessage, PPStateToDevString( iPPState->iState() ) );
		if ( iPPState->iState() ==  PresencePublisher::EReconnecting)
			{				
				AppendIntervalL( iPPState->iRetry(), iDevMessage );
			}

		if ( iPPState->iMessage().Length() > 0 )
			{
				SafeAppend( iDevMessage, _L("\n") );
				SafeAppend( iDevMessage, iPPState->iMessage() );
			}
		
		if ( iPPState->iError )
			{
				SafeAppend( iDevMessage, _L("\n") );
				
				CBBErrorInfo* einfo = iPPState->iError;
				TPtrC p( einfo->UserMessage().Length() ?
						 einfo->UserMessage() : 
						 einfo->TechMessage() );
				SafeAppend( iDevMessage, p );
			}
		return iDevMessage.Compare( iOldMessage ) != 0;
	}


	void CombineOldConnectionStateMsgL(TBool aFirst=EFalse)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("CombineOldConnectionStateMsgL"));
		iOldMessage = iStatusMessage;
		
		if ( ! iOnline ) 
			{
				if ( iHasAuthError )
					{
						iStatusMessage = _L("Offline: ");
						SafeAppend( iStatusMessage, iAuthErrorMsg );
					}
				else
					{
						iStatusMessage = _L("Offline");
					}
			}
		else
			{
				switch ( iConnectionState )
					{
					case ContextServer::EUnknown:
						if ( aFirst )
							iStatusMessage = _L("Starting");
						else
							iStatusMessage = _L("Unknown");
						break;
						
					case ContextServer::EStarting:
						iStatusMessage = _L("Starting");
						break;
						
					case ContextServer::EConnecting:
						iStatusMessage = _L("Connecting");
						break;
						
					case ContextServer::EConnected:
						if ( iProgress == 100 )				
							iStatusMessage.Zero();
						else 
							iStatusMessage = _L("Fetching");
						break;
						
					case ContextServer::EDisconnecting:
						iStatusMessage = _L("Disconnecting");
						break;
						
					case ContextServer::EDisconnected:
						if ( iHasAuthError )
							{
								iStatusMessage = _L("Offline: ");
								SafeAppend( iStatusMessage, iAuthErrorMsg );
							}
						else if ( iHasNetworkError )
							{
								iStatusMessage = _L("Offline: ");
								SafeAppend( iStatusMessage, iNetworkErrorMsg );
							}
						else
							iStatusMessage = _L("Disconnected");
						break;
						
					case ContextServer::EStopped:
						iStatusMessage = _L("Stopped");
						break;
					}
			}
		if ( iStatusMessage.Compare( iOldMessage ) != 0 ) 
			NotifyMessageL( EStatusMessage, iStatusMessage );
	}

	
	void DecideStateL(TBool aFirst=EFalse)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("DecideStateL"));
		iOldMessage = iStatusMessage;
		
		TDes& b = iStatusMessage;
		
		b.Zero();
		if ( ! iOnline ) 
			{
				if ( iHasAuthError )
					{
						b = _L("Offline: ");
						SafeAppend( b, iAuthErrorMsg );
					}
				else
					{
						b = _L("Offline");
					}
			}
		else if ( ! iPPState )
			{
				SafeAppend( b, _L("Starting") );
			}
		else
			{
				TInt state = iPPState->iState();
				AppendPPStateL( b, state );
				

				if ( state ==  PresencePublisher::EReconnecting || state==PresencePublisher::EUnknown)
					{				
						TTime retryTime = iPPState->iRetry();
						if ( retryTime != Time::NullTTime() )
							{
								AppendIntervalL( retryTime, b );
								SetUpdateTimerL( retryTime );
							}
						if ( iHasNetworkError )
							{
								SafeAppend( b, _L("\n") );
								SafeAppend( b, iNetworkErrorMsg );
							}
					}						
				
				if ( iPPState->iMessage().Length() > 0 )
					{
						SafeAppend( b, _L("\n") );
						SafeAppend( b, iPPState->iMessage() );
					}
				
				if ( iPPState->iError )
					{
						SafeAppend( b, _L("\n") );
						
						CBBErrorInfo* einfo = iPPState->iError;
						TPtrC p( einfo->UserMessage().Length() ?
								 einfo->UserMessage() : 
								 einfo->TechMessage() );
						SafeAppend( b, p );
					}
			}
		TBool changed =  b.Compare( iOldMessage ) != 0;
		TBool devMessageChanged = CombinePPMessageL();
		
		if ( changed )
			NotifyMessageL( EStatusMessage, b );
		if ( devMessageChanged )
			NotifyMessageL( EDevMessage, iDevMessage );
	}
	
	void NotifyMessageL(TMessageId aId, const TDesC& aMsg)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("NotifyMessageL"));
		for (TInt i=0; i < iListeners.Count(); i++)
			{
				iListeners[i]->MessageChanged( aId, aMsg );
			}
	}

	void NotifyProgressL()
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("NotifyProgressL"));
		for (TInt i=0; i < iListeners.Count(); i++)
			{
				iListeners[i]->ProgressChanged( iProgress );
			}
	}
	
	//
	// event handlers 
	// 

	void NewValueL(TUint aId,
				   const TTupleName& aName, 
				   const TDesC& aSubName, 
				   const TComponentName& aComponentName, 
				   const MBBData* aData)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("NewValueL"));
		if (aName==KContextServerStatusTuple) {
			const TBBInt* value=bb_cast<TBBInt>(aData);
			if (!value) return;
			iConnectionState = (*value)();
			DecideStateL();
		}
		else if (aName==KConnectionStateTuple)
			{
				const CBBConnectionState* value=bb_cast<CBBConnectionState>(aData);
				if (!value) return;
				
				delete iPPState;
				iPPState = NULL;
				iPPState = bb_cast<CBBConnectionState>(value->CloneL(_L("stored pp")));
				
				DecideStateL();
			}
	}

	virtual void DeletedL(const TTupleName& /*aName*/, const TDesC& /*aSubName*/) 
	{
		// do nothing, KContextServerStatusTuple should be never deleted
	}

	

	void SettingChanged(TInt aSetting) 
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("SettingChanged"));
		switch ( aSetting )
			{
			case SETTING_PROGRESS_INCOMING:
				Settings().GetSettingL(SETTING_PROGRESS_INCOMING, iProgress);
				NotifyProgressL();
				DecideStateL();
				break;
			case SETTING_IDENTIFICATION_ERROR:
				iAuthErrorMsg.Zero();
				Settings().GetSettingL(SETTING_IDENTIFICATION_ERROR, iAuthErrorMsg);
				if ( iAuthErrorMsg.Length() )
					iHasAuthError = ETrue;
				else
					iHasAuthError = EFalse;
				DecideStateL();
				break;
			case SETTING_PRESENCE_ENABLE:
				Settings().GetSettingL(SETTING_PRESENCE_ENABLE, iOnline);
				DecideStateL();
				break;
			}
	};

	void NetworkError(TErrorDisplay aMode, const TDesC& aMessage)
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("NetworkError") );
		if (iHasAuthError) return;
		 
		iHasNetworkError = ETrue;
		iNetworkErrorMsg.Zero();
		SafeAppend( iNetworkErrorMsg, aMessage );
		
		if (aMode==EIntrusive) {}

		DecideStateL();
	}
	
	void NetworkSuccess()
	{
		CALLSTACKITEM_N(_CL("CProgressBarModelImpl"), _CL("NetworkSuccess") );
		iHasNetworkError = EFalse;
		SettingChanged(SETTING_IDENTIFICATION_ERROR);
	}
	

	
private:
	TInt iConnectionState;
	TBool iOnline;
	TBool iHasAuthError;
	TBool iHasNetworkError;

	TInt iProgress;

	TBuf<200> iNetworkErrorMsg;
	TBuf<200> iAuthErrorMsg;

	/**
	 * iStatusMessage contains status message combined from different error and state sources.
	 */ 
	TBuf<400> iStatusMessage;
	TBuf<1000> iOldMessage;

	/**
	 * NetworkError is used to listen for network errors
	 */ 
	CNetworkError* iNetworkError; // owned

	/**
	 * BlackBoardSubSession is used to listen for connection states
	 */ 
	CBBSubSession	*iBBSubSession; // own
	

	RPointerArray<MListener> iListeners; // content not owned

	/**
	 * PresencePublisher's connection state 
	 */
	CBBConnectionState* iPPState;

	TBuf<1000> iDevMessage; 

	TBuf<1000> iFetchMessage;

	CTimePeriodFormatter& iPeriodFormatter;


	CTimeOut* iReconnectingTimer, *iLowSignalTimer;
};




EXPORT_C CProgressBarModel* CProgressBarModel::NewL(CTimePeriodFormatter& aPeriodFormatter)
{
	CALLSTACKITEMSTATIC_N(_CL("CProgressBarModel"), _CL("NewL") );
	auto_ptr<CProgressBarModelImpl> self( new (ELeave) CProgressBarModelImpl(aPeriodFormatter) );
	self->ConstructL();
	return self.release();
}
