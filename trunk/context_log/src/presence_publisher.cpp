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

#include "presence_publisher.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"
#include "i_logger.h"
#include "eikenv.h"
#include "status_notif.h"
#include <context_log.mbg>

#include "app_context_impl.h"
#include "callstack.h"
#include "break.h"
#include "contextcommon.h"
#include "sha1.h"

#include "contextvariant.hrh"
#include "reporting.h"

#include "csd_connectionstate.h"

#ifndef __WINS__
_LIT(KIconFile, "c:\\system\\data\\context_log.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\context_log.mbm");
#endif

#pragma warning(disable: 4706) // assignment withing conditional expression

#ifdef __WINS__
#define TRACE(x) { RDebug::Print(x); }
#else
#define TRACE(x) 
#endif

#include "cn_networkerror.h"
void DebugLog(const TDesC& aMsg);


namespace PresencePublisher {

class TStatePoster {
public:
	TStatePoster(CPresencePublisher* aPublisher) : iPublisher(0) {
		if (!aPublisher || aPublisher->iLive) return;
		iPublisher=aPublisher;
		iPublisher->iLive=&iLive;
		iLive=1;
	}
	~TStatePoster() {
		if (!iPublisher || !iLive) return;
		iPublisher->PostConnectionState();
		iPublisher->iLive=0;
	}
private:
	CPresencePublisher* iPublisher;
	TInt iLive;
};

_LIT(KPresencePublisher, "PresencePublisher");

CPresencePublisher* CPresencePublisher::NewL(MApp_context& Context, i_status_notif* CallBack, 
					     MPresencePublisherListener& aListener, CBTDeviceList* aBuddyList,
					     CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, 
					     CBTDeviceList *aPDABTs)
{
	CALLSTACKITEMSTATIC_N(_CL("CPresencePublisher"), _CL("NewL"));

	auto_ptr<CPresencePublisher> ret( new (ELeave) CPresencePublisher(Context, 
		CallBack, aListener, aBuddyList, aLaptopBTs, aDesktopBTs, aPDABTs) );
	ret->ConstructL();
	return ret.release();
}

CPresencePublisher::~CPresencePublisher()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("~CPresencePublisher"));

	iCurrentState=EUnknown;
	TRAPD(ignored, PostConnectionState());
	
	delete iLookupIndicator;
	delete ctx;
	Settings().CancelNotifyOnChange(SETTING_PRESENCE_ENABLE, this);
	Settings().CancelNotifyOnChange(SETTING_IP_AP, this);
	Settings().CancelNotifyOnChange(SETTING_JABBER_NICK, this);
	Settings().CancelNotifyOnChange(SETTING_JABBER_PASS, this);
	Settings().CancelNotifyOnChange(SETTING_CONNECTIVITY_MODEL, this);
	Cancel();
	delete iWait;
	delete iSuspendTimer;
	if (iSessionIsOpen) iSession.Close();
	delete iSendPresence;
	delete iPresence;
	delete iPresenceRunning;
	delete iFrozen;
	delete iConnectionState;
	delete iConnectivity;
	if (iLive) *iLive=0;
}

void CPresencePublisher::PostConnectionState(const TDesC& aMessage, TInt aErrorCode, MErrorInfo* aError)
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("PostConnectionState"));
	
	if (!iConnectionState) return;
	
	iConnectionState->iState()=iCurrentState;
	if (iCurrentState==EDisabled && iConnectivity->OfflineMode()) iConnectionState->iState=EOffline;
	
	iConnectionState->iMessage()=aMessage.Left(iConnectionState->iMessage().MaxLength());
	iConnectionState->iRetry()=iWait->WaitingUntil();
	
	TTime expires=GetTime(); expires+=TTimeIntervalDays(2);
	
	BBSession()->PutL(KConnectionStateTuple, KSubname, iConnectionState, expires, KNoComponent);
}

TBool CPresencePublisher::EvaluateSuspendL(TBool aEvaluateOnly)
{
	if (iFirst) return EFalse;
	
	if (!iOnlyWhenActive) return EFalse;
	if (!iEnabled) return ETrue;
	
	if (iInJaiku /*&& !iIdle*/) {
		if (aEvaluateOnly) return EFalse;
		if (iSuspendTimer->IsActive()) {
			iSuspendTimer->Reset();
		}
		if (ConnectionSuspended()) {
			if (iConnectivity->AllowReconnect()) {
				iWaitTime=1;
				ResumeConnection();
			} else if (iCurrentState!=ECallInProgress && iCurrentState!=ELowSignal) {
				TStatePoster p(this);
				if (iConnectivity->LowSignal()) iCurrentState=ELowSignal;
				else if (iConnectivity->CallInProgress()) iCurrentState=ECallInProgress;
			}
		}
		return EFalse;
	}
	if (aEvaluateOnly) return ETrue;
	
	if (!ConnectionSuspended()) iSuspendTimer->WaitMax(60);
	return ETrue;
}

void CPresencePublisher::SuspendConnection()
{
	TStatePoster p(this);
	if(iCallBack) iCallBack->status_change(_L("Suspend Presence"));
	if (iCurrentState==EEventListening) {
		Cancel();
		iCurrentState=EConnected;
	}

	if (iNextOp==EDisable || iNextOp==ESuspend) return;
	iWait->Cancel();

	switch (iCurrentState) {
	case EDisabled:
	case EDisabling:
		break;
	case ESuspended:
	case ESuspending:
		break;
	case EReconnecting:
		iCurrentState=ESuspended;
		break;
	default:
		if (IsActive()) {
#ifdef __WINS__
			TBuf<100> msg=_L("CPP: suspendig while active ");
			msg.AppendNum(iCurrentState);
			TRACE(msg);
#endif
			iNextOp=ESuspend;
		} else {
			iStatus=KRequestPending;
			SetActive();
			iSession.MsgSuspendConnection(iStatus);
			iCurrentState=ESuspending;
		}
		break;
	}
	if (iCurrentState==ESuspended && iConnectivity->OfflineMode()) iCurrentState=EOffline;
}

void CPresencePublisher::QueueUpdate()
{
	TStatePoster p(this);
	iNewValue=true;
	if (iCurrentState==EEventListening) {
		Cancel();
		iCurrentState=EConnected;
	}
	
	if (IsActive()) return;
	iCurrentState=EQueuingUpdate;
	TRequestStatus *s=&iStatus;
	iStatus=KRequestPending;
	SetActive();
	User::RequestComplete(s, KErrNone);
}
	
bool CPresencePublisher::Disabled()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("Disabled"));

	if (iCurrentState==EDisabling || iCurrentState==EDisabled || iNextOp==EDisable) {
		return true;
	}
	return false;
}

void CPresencePublisher::ResumeConnection()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("ResumeConnection"));
	TStatePoster p(this);
	if(iCallBack) iCallBack->status_change(_L("Resume Presence"));
	if (iCurrentState==EEventListening) {
		Cancel();
		iCurrentState=EConnected;
	}

	if (IsActive() && (iNextOp==EResume || iNextOp==EEnable)) return;
	iWait->Cancel();

	switch (iCurrentState) {
	case EReconnecting:
	default:
		if (IsActive()) {
			iNextOp=EResume;
		} else {
			Restart();
		}
		break;
	}
}

void CPresencePublisher::ConnectivityStateChanged()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("ConnectivityStateChanged"));
	
	iData->iInCall()=iConnectivity->CallInProgress();
	
	TBuf<40> msg=_L("PP: StateChanged ");
	Reporting().DebugLog(msg);
	if (iConnectivity->LowSignal()) {
		Reporting().DebugLog(_L("PP: Low signal"));
	} else {
		Reporting().DebugLog(_L("PP: non-Low signal"));
	}
	if (iConnectivity->CallInProgress()) {
		Reporting().DebugLog(_L("PP: Call"));
	} else {
		Reporting().DebugLog(_L("PP: no call"));
	}
	if (iConnectivity->OfflineMode()) {
		CNetworkError::ResetRequestedL();
		CNetworkError::ResetTryingL();
		if (!ConnectionSuspended()) SuspendConnection();
		return;
	}
	if (iFirst && iEnabled && iConnectivity->AllowReconnect()) {
		iWaitTime=0;
		Restart();
		return;
	}
	if (ConnectionSuspended() && !iFirst) {
		Reporting().DebugLog(_L("PP: connection suspended"));
		if (iConnectivity->AllowReconnect()) {
			Reporting().DebugLog(_L("PP: allowing reconnect"));
			if (iCurrentState==ELowSignal) {
				iWaitTime=10;
			} else {
				iWaitTime=2;
			}
			Restart();
		} else {
			iWait->Reset();
			Reporting().DebugLog(_L("PP: not allowing reconnect"));
		}
	} else {
		if (!iFirst && iCurrentState==EReconnecting && ! iConnectivity->AllowReconnect()) {
			TStatePoster p(this);
			iCurrentState=ESuspended;
			if (iConnectivity->CallInProgress())
				iCurrentState = ECallInProgress;
			else if (iConnectivity->LowSignal())
				iCurrentState = ELowSignal;
				
			iWait->Reset();
		}
		Reporting().DebugLog(_L("PP: connection not suspended"));
	}
}

bool CPresencePublisher::ConnectionSuspended()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("ConnectionSuspended"));

	if (iCurrentState==ESuspending || iCurrentState==ESuspended || iNextOp==ESuspend ||
		iCurrentState==EDisabling || iCurrentState==EDisabled || iNextOp==EDisable ||
		iCurrentState==ECallInProgress || iCurrentState==ELowSignal ||
		iCurrentState==EOffline) return true;
	return false;
}

CPresencePublisher::CPresencePublisher(MApp_context& Context, i_status_notif* CallBack , 
				       MPresencePublisherListener& aListener, CBTDeviceList* aBuddyList,
				       CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, CBTDeviceList *aPDABTs): 
	CCheckedActive(EPriorityIdle, _L("CPresencePublisher")), 
		MPresenceMaintainer(Context, aBuddyList, aLaptopBTs, aDesktopBTs, aPDABTs), 
		iCallBack(CallBack), iListener(aListener)
		{ }

void CPresencePublisher::ShowFrozen(TBool aIsFrozen)
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("ShowFrozen"));
	if (aIsFrozen) {
		if (!iFrozenIndicator) {
			CC_TRAPD(err, iFrozenIndicator=CNotifyState::NewL(AppContext(), KIconFile));
			if (iFrozenIndicator) iFrozenIndicator->SetCurrentState(
				EMbmContext_logFrozen, EMbmContext_logFrozen);
		}
	} else {
		delete iFrozenIndicator; iFrozenIndicator=0;
	}
}

void CPresencePublisher::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("ConstructL"));

	Settings().GetSettingL(SETTING_JABBER_NICK, iFullNick);
	if (iFullNick.Length()>50) iFullNick.SetLength(50);
	TInt sep=iFullNick.Locate('@');
	if (iFullNick.Length()>0 && sep==KErrNotFound) iFullNick.Append(_L("@jaiku.com"));

	MPresenceMaintainer::ConstructL();

	{
		CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("froze"));

		MBBData *frozen=0;
		iBBSubSessionNotif->GetL(KFrozenPresenceTuple, KNullDesC, frozen, ETrue);
		iFrozen=bb_cast<CBBPresence>(frozen); 
		if (!iFrozen) delete frozen; 
		frozen=0;
		iBBSubSessionNotif->GetL(KUnfreezeTimeTuple, KNullDesC, frozen, ETrue);
		TBBTime *unft=bb_cast<TBBTime>(frozen); 
		if (unft) iUnfreezeTime=(*unft)();
		delete unft;

		if (iFrozen) {
			iFrozen->iSentTimeStamp()=GetTime();
			ShowFrozen(ETrue);
		}
		iBBSubSessionNotif->AddNotificationL(KFrozenPresenceTuple);
	}

#ifndef __JAIKU__
	{
		CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("lookupind"));
		CC_TRAPD(err, iLookupIndicator=CIncomingLookupIndicator::NewL(AppContext()));
	}
#endif

	{
#ifndef __JAIKU__
		CC_TRAPD(err, iPresenceRunning=CNotifyState::NewL(AppContext(), KIconFile));
#endif
		if (iPresenceRunning) iPresenceRunning->SetCurrentState(EMbmContext_logP_not, EMbmContext_logP_not);
	}

	{
		CC_TRAPD(err, ctx=::CApp_context::NewL(true, _L("ContextServer")));
		if (err!=KErrNone) {
			TBuf<50> msg=_L("creation of cs appcontext failed: ");
			msg.AppendNum(err);
			if(iCallBack) iCallBack->status_change(msg);
		}
	}

	iEnabled=1;
	{
		CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("settings"));
		iWait=CTimeOut::NewL(*this);
		iPresence=CXmlBufExternalizer::NewL(1024);

		iCurrentState=EDisabled;
		Settings().NotifyOnChange(SETTING_PRESENCE_ENABLE, this);
		Settings().NotifyOnChange(SETTING_IP_AP, this);
		Settings().NotifyOnChange(SETTING_JABBER_NICK, this);
		Settings().NotifyOnChange(SETTING_JABBER_PASS, this);
		Settings().NotifyOnChange(SETTING_CONNECTIVITY_MODEL, this);

		if (! Settings().GetSettingL(SETTING_PRESENCE_ENABLE, iEnabled) ) {
			iEnabled=1;
		}
	}
	{
		TInt connectivity=0;
		Settings().GetSettingL(SETTING_CONNECTIVITY_MODEL, connectivity);
		iOnlyWhenActive=(connectivity==CONNECTIVITY_WHEN_ACTIVITY_ONLY);
		iData->iConnectivityModel()=connectivity;
	}
	
	iFirst=ETrue;
	iConnectivity=CConnectivityListener::NewL(*this);
	
	iSuspendTimer=CTimeOut::NewL(*this);	
	if (iConnectivity->OfflineMode()) {
		iEnabled=0;
		CNetworkError::ResetRequestedL();
		CNetworkError::ResetTryingL();
	}
	{
		CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("enabled"));
		CActiveScheduler::Add(this);
		Settings().GetSettingL(SETTING_IP_AP, iAP);
		if (iAP<=0) iEnabled=EFalse;
		if (iEnabled) {
			iWaitTime=15;
			Restart();
		} else {
			iCurrentState=EDisabled;
			iConnectivity->SuspendExpensive();
		}
	}
#ifdef __WINS__
	TGpsLine& line=iData->iGps;
	line()=_L("$GPRMC,071546.557,V,6010.6681,N,02457.6963,E,,,070205,,*12");
#endif
	iConnectionState=new (ELeave) CBBConnectionState;
	PostConnectionState();
}

void CPresencePublisher::SettingChanged(TInt Setting)
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("SettingChanged"));
	
	TStatePoster p(this);
#ifdef __WINS__
	RDebug::Print(_L("CPresencePublisher::SettingChanged"));
#endif

	if (Setting==SETTING_IP_AP) {
		TInt ap=-1;
		Settings().GetSettingL(SETTING_IP_AP, ap);
		if (ap==iAP) return;
	}

	if (Setting==SETTING_JABBER_NICK || Setting==SETTING_JABBER_PASS) {
		Settings().WriteSettingL(SETTING_IDENTIFICATION_ERROR, KNullDesC);
	}
	iTryPlain=EFalse;
	iEnabled=1;
	if (! Settings().GetSettingL(SETTING_PRESENCE_ENABLE, iEnabled) ) {
		iEnabled=1;
	}
	if (iConnectivity->OfflineMode()) {
		iEnabled=0;
	}
	if (Setting==SETTING_JABBER_NICK) {
		iBBSubSessionNotif->DeleteL( KIncomingPresence, iFullNick, ETrue );
	}
	Settings().GetSettingL(SETTING_JABBER_NICK, iFullNick);
	if (iFullNick.Length()>50) iFullNick.SetLength(50);
	TInt sep=iFullNick.Locate('@');
	if (iFullNick.Length()>0 && sep==KErrNotFound) iFullNick.Append(_L("@jaiku.com"));
	if (Setting==SETTING_JABBER_NICK) {
		NotifyNewPresence();
	}
	if (Setting==SETTING_IP_AP) {
		Settings().WriteSettingL(SETTING_LAST_CONNECTION_REQUEST, GetTime());
		Settings().WriteSettingL(SETTING_LATEST_CONNECTION_REQUEST, GetTime());
		CNetworkError::ResetSuccessL();
	}
	if (Setting==SETTING_CONNECTIVITY_MODEL) {
		TInt connectivity=0;
		Settings().GetSettingL(SETTING_CONNECTIVITY_MODEL, connectivity);
		iOnlyWhenActive=(connectivity==CONNECTIVITY_WHEN_ACTIVITY_ONLY);
		iData->iConnectivityModel()=connectivity;
	}
	Settings().GetSettingL(SETTING_IP_AP, iAP);
	if (iEnabled && iAP>0 && !EvaluateSuspendL()) {
		iWaitTime=1;
		Restart();
	} else {
		iSuspendTimer->Reset();
		if (iCurrentState==EDisabled || iCurrentState==EDisabling) return;
		iWait->Cancel();
		if (iCurrentState==EReconnecting || iCurrentState==ESuspended || iCurrentState==EUnknown ||
				!iSessionIsOpen || iCurrentState==EOffline || iCurrentState==ELowSignal ||
				iCurrentState==ECallInProgress) {
			Cancel();
			iCurrentState=EDisabled;
			return;
		}
		if (iCurrentState!=ESuspending) {
			Cancel();
			iStatus=KRequestPending;
			SetActive();
			iSession.MsgSuspendConnection(iStatus);
			iCurrentState=ESuspending;
		}
		iNextOp=EDisable;
	}
}

void CPresencePublisher::GotSignificantChange(TBool frozenChanged, TBool aQuickRate)
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("GotSignificantChange"));
	TStatePoster p(this);
	DebugLog(_L("PP: GotSignificantChange"));
	if (iCurrentState==EEventListening) {
		Cancel();
		iCurrentState=EConnected;
	}
	iQuickRate=aQuickRate;
	if (iCurrentState==EConnected && iFrozen) {
		iFrozen->iSentTimeStamp()=GetTime();
	}
	if (!iFrozen || frozenChanged) {
		MakePresence();
		if (iCurrentState==EConnected) {
			// wait one turn of the active scheduler
			QueueUpdate();
		} else {
			iNewValue=true;
		}
	}
	NotifyNewPresence();
}

void CPresencePublisher::MakePresence()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("MakePresence"));

	iPresence->Zero();
	if (iFrozen) {
		iFrozen->MinimalXmlL(iPresence);
	} else {
		iData->MinimalXmlL(iPresence);
	}
}

void CPresencePublisher::SendUpdate()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("SendUpdate"));
	TStatePoster p(this);

	delete iSendPresence; iSendPresence=0;
	iSendPresence=iPresence->Buf().AllocL();

	iStatus=KRequestPending;
	SetActive();
	iSession.MsgUpdateUserPresence(*iSendPresence, iStatus, iQuickRate);
	iQuickRate=EFalse;

	iCurrentState=ESendingUpdate;
	iNewValue=false;

	iSentTimeStamp=GetTime();

#ifdef __WINS__
	RDebug::Print(_L("CPresencePublisher::SendUpdate()"));
#endif

	//User::Leave(1003);

}

void CPresencePublisher::Restart()
{
	TRACE(_L("PP: Restart"));
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("Restart"));
	TStatePoster p(this);
	
	if (!iFirst && EvaluateSuspendL(ETrue)) {
		iCurrentState=ESuspended;
		return;
	}
	
	iConnectivity->ResumeExpensiveL();

	//if(iCallBack) iCallBack->status_change(_L("restart"));

	iData->iSentTimeStamp=TTime(0);
	iData->iSent=EFalse;
	NotifyNewPresence();

	Cancel();
	if (iSessionIsOpen) iSession.Close();
	iSessionIsOpen=EFalse;
	
	iCurrentState=EReconnecting;
	iNextOp=ENone;
	iWait->Wait(iWaitTime);
#ifdef __WINS__
	RDebug::Print(_L("CPresencePublisher::Restart()"));
#endif
}

void CPresencePublisher::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("CheckedRunL"));
	TStatePoster p(this);

#if defined(__WINS__) || 1
	TBuf<150> msg;
	msg.Format(_L("CPresencePublisher::CheckedRunL %d, state %d, nextop %d"), iStatus.Int(), iCurrentState, iNextOp);
	Reporting().DebugLog(msg);
	//RDebug::Print(msg);
	msg.Zero();
#else
	TBuf<50> msg;
#endif

	if (iStatus<0) {
		iWaitTime=(int)(iWaitTime*1.5);
		if (iWaitTime<5) iWaitTime=5;
		TRACE(_L("PP: CCL error"));

		if(iPresenceRunning) iPresenceRunning->SetCurrentState(EMbmContext_logP_not, EMbmContext_logP_not);
		// depending on the error, we should not always restart
		//Restart();
		if (iStatus==KErrAccessDenied) {
			if(iCallBack) iCallBack->status_change(_L("User disallowed jabber connection"));
			iCurrentState = ESuspended;
		} else if (iStatus == -1 /*EIdentificationError*/ && iTryPlain) {
			TRACE(_L("PP: CCL error -id"));
			TBuf<100> error;
			error.Append(_L("Presence ID Error: "));
			error.Append(iUser);
			
			if(iCallBack) iCallBack->status_change(_L(""));
			if(iCallBack) iCallBack->error(error);
			iCurrentState = ESuspended;
			
			Settings().WriteSettingL(SETTING_PRESENCE_ENABLE, EFalse);

		} else {
			TRACE(_L("PP: CCL error other"));
			if (iStatus==-1) iTryPlain=ETrue;

			if (iStatus.Int()!=-1 && !iConnectivity->AllowReconnect()) {
				CNetworkError::ResetRequestedL();
				CNetworkError::ResetTryingL();
				iCurrentState=ESuspended;
				if (iConnectivity->CallInProgress())
					iCurrentState = ECallInProgress;
				else if (iConnectivity->LowSignal())
					iCurrentState = ELowSignal;
				iConnectivity->ResumeExpensiveL();
				return;
			}
			msg.Format(_L("Presence error %d, restarting"), iStatus.Int());
			TRACE(msg);
			Reporting().UserErrorLog(msg);
			if (iCallBack) iCallBack->status_change(msg);
			if (iStatus.Int() == KErrServerTerminated && ctx) {
				// ContextServer crashed
				auto_ptr<HBufC> stack(0);
				
				CC_TRAPD(err, stack.reset(ctx->CallStackMgr().GetFormattedCallStack(_L("Presence"))));
				if (err==KErrNone && stack.get()!=0) {
					Reporting().UserErrorLog(*stack);
					if (iCallBack) iCallBack->status_change(*stack);
				} else {
					msg.Format(_L("Couldn't read stack: %d"), err);
					Reporting().UserErrorLog(msg);
					if (iCallBack) iCallBack->status_change(msg);
				}
			}
			
			Restart();
		}
		return;
	}

	iWait->Cancel();

	iWaitTime=10;

	switch (iCurrentState) {
	case EConnecting:
	case EResuming:
		msg=_L("Presence Connected");
		iConnectivity->SuspendExpensive();
		if(iPresenceRunning) iPresenceRunning->SetCurrentState(EMbmContext_logP, EMbmContext_logP);
	case ESendingUpdate:
		iCurrentState=EConnected;
		iData->iSentTimeStamp=GetTime();
		iData->iSent=ETrue;
		NotifyNewPresence();
		break;
	case ESuspending:
		msg=_L("Presence Suspended");
		if(iPresenceRunning) iPresenceRunning->SetCurrentState(EMbmContext_logP_not, EMbmContext_logP_not);
		iCurrentState=ESuspended;
		iData->iSent=EFalse;
		if (iConnectivity->OfflineMode()) iCurrentState=EOffline;
		iConnectivity->SuspendExpensive();
		NotifyNewPresence();
		break;
	case EQueuingUpdate:
		iCurrentState=EConnected;
		break;
	default:
		msg=_L("Presence Restarting");
		Restart();
		break;
	}

	if (msg.Length()>0) if (iCallBack) iCallBack->status_change(msg);

	switch (iNextOp) {
	case ESuspend:
		iStatus=KRequestPending;
		SetActive();
		iSession.MsgSuspendConnection(iStatus);
		iCurrentState=ESuspending;
		break;
	case EResume:
		iStatus=KRequestPending;
		SetActive();
		iSession.MsgResumeConnection(iStatus);
		iCurrentState=EResuming;
		break;
	case EDisable:
		if (iSessionIsOpen) iSession.Close();
		iSessionIsOpen=EFalse;
		iCurrentState=EDisabled;
		break;
	default:
		break;
	};
	iNextOp=ENone;

	if (iNewValue && iCurrentState==EConnected) {
		DebugLog(_L("PP: CheckedRunL-SendUpdate"));
		SendUpdate();
	}
	
	if (!IsActive() && iCurrentState==EConnected) {
		DebugLog(_L("PP: CheckedRunL-RequestEventNotification"));
		iSession.MsgRequestEventNotification(iStatus);
		iCurrentState=EEventListening;
		SetActive();
	}
}
		
void CPresencePublisher::NotifyNewPresence()
{
	if (iFullNick.Length()>0) {
		TTime expires; expires=GetTime(); expires+=TTimeIntervalDays(7);
		iData->iSentTimeStamp=GetTime();
		iBBSubSessionNotif->PutL( KIncomingPresence, iFullNick, iData, expires );
	}
	iListener.NotifyNewPresence(iData);
}

void CPresencePublisher::DoCancel()
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("DoCancel"));
	TBuf<40> msg=_L("PP: DoCancel state"); msg.AppendNum(iCurrentState);
	DebugLog(msg);
	iSession.Cancel();
}

TInt CPresencePublisher::CheckedRunError(TInt /*aError*/)
{
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("CheckedRunError"));

	Restart();
	
	return KErrNone;
}

void CPresencePublisher::expired(CBase* source)
{
	TRACE(_L("PP: expired"));
	CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("expired"));
	TStatePoster p(this);

	if (source==iSuspendTimer) {
		SuspendConnection();
		return;
	}
	if (source!=iWait) {
		MPresenceMaintainer::expired(source);
		return;
	}
	
	if (iCurrentState==ELowSignal) {
		SettingChanged(SETTING_PRESENCE_ENABLE);
		return;
	}
	if (iFirst && !iConnectivity->AllowReconnect()) {
		iCurrentState=ESuspended;
		if (iConnectivity->CallInProgress())
			iCurrentState = ECallInProgress;
		else if (iConnectivity->LowSignal())
			iCurrentState = ELowSignal;
		iFirst=EFalse;
		return;
	}
	iFirst=EFalse;
	if (iCurrentState==EReconnecting) {
		//if (iCallBack)  iCallBack->status_change(_L("expired"));
		TBuf<50> tmp;

		TInt enabled=1;
		if (! Settings().GetSettingL(SETTING_PRESENCE_ENABLE, enabled) ) {
			enabled=1;
		}
		if (!enabled) {
			iCurrentState=EDisabled;
			iConnectivity->SuspendExpensive();
			return;
		}

		// read both settings, no short-circuit
		TBool pass=Settings().GetSettingL(SETTING_JABBER_PASS, iPass);
		pass= Settings().GetSettingL(SETTING_JABBER_PASS_SHA1, iPassSHA1) || pass;
		if (iFullNick.Length() && pass && iFullNick.Length()<=50) {
			tmp=iFullNick;
			TInt sep=tmp.Locate('@');
			if (iPass.Length()>0) {
				if (!iTryPlain) {
					DoSHA1(iPass, iPassSHA1);
				} else {
					iPassSHA1=iPass;
				}
			}
#ifdef __WINS__
			RDebug::Print(iPassSHA1);
#endif
			if (sep!=KErrNotFound && tmp.Length()>0) {
				{
					CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("ConnectToContextServer"));
					TInt ret=iSession.ConnectToContextServer();
					if (ret!=KErrNone) {
						iSessionIsOpen=EFalse;
						TBuf<30> msg;
						msg.Format(_L("ConnectToContextServer %d"), ret);
						if (iCallBack) iCallBack->error(msg);
						Restart();
						return;
					}
					iSessionIsOpen=ETrue;
				}
				{
					CALLSTACKITEM_N(_CL("CPresencePublisher"), _CL("ConnectToPresenceServer"));
					iUser=tmp.Mid(0, sep);
					iServer=tmp.Mid(sep+1);

					if (iCallBack) iCallBack->status_change(_L("Presence Connecting"));
					iStatus=KRequestPending;
					SetActive();
					iSession.MsgConnectToPresenceServer(iUser, iPassSHA1,
						iServer, iAP, iStatus);
					iCurrentState=EConnecting;
					iWait->Wait(5*60);
				}
			}
		}
	} else {
		Restart();
	}
}

const CBBPresence* CPresencePublisher::FrozenData() const
{
	return iFrozen;
}

_LIT(KFrozen, "frozen");

void CPresencePublisher::FreezeL()
{
	delete iFrozen; iFrozen=0;
	iFrozen=bb_cast<CBBPresence>(iData->CloneL(KNullDesC));
	if (iFrozen->iUserActive.iActive()) {
		iFrozen->iUserActive.iActive()=EFalse;
		iFrozen->iUserActive.iSince()=GetTime();
	}

	TTime expires=Time::MaxTTime();
	iBBSubSessionNotif->PutL(KFrozenPresenceTuple, KNullDesC, iFrozen, expires);

	TBBBool fr(ETrue, KFrozen);
	iBBSubSessionNotif->PutL(KPresenceFrozen, KNullDesC, &fr, expires);

	ShowFrozen(ETrue);

	MakePresence();
	if (iCurrentState==EConnected) {
		// wait one turn of the active scheduler
		QueueUpdate();
	} else {
		iNewValue=true;
	}
}

_LIT(KUnfreezeTime, "unft");

void CPresencePublisher::UnFreezeL()
{
	delete iFrozen; iFrozen=0;
	iBBSubSessionNotif->DeleteL(KFrozenPresenceTuple, KNullDesC);
	iBBSubSessionNotif->DeleteL(KPresenceFrozen, KNullDesC);

	iUnfreezeTime=GetTime();
	TBBTime unft(iUnfreezeTime, KUnfreezeTime);
	TTime expires=Time::MaxTTime();
	iBBSubSessionNotif->PutL(KUnfreezeTimeTuple, KNullDesC, &unft, expires);

	ShowFrozen(EFalse);
	MakePresence();
	if (iCurrentState==EConnected) {
		// wait one turn of the active scheduler
		QueueUpdate();
	} else {
		iNewValue=true;
	}
}

TBool CPresencePublisher::IsFrozen()
{
	return (iFrozen!=0);
}

}
