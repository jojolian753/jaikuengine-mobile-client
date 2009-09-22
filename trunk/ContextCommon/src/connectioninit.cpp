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

// Concepts:
// !Opening a GPRS (data) connection!

#pragma warning(disable: 4706)

#include "ver.h"

#include "connectioninit.h"
#include <cdbcols.h>
#include <commdb.h>
#include "pointer.h"
#include "app_context.h"
#include "break.h"
#include "settings.h"

#include <aknglobalconfirmationquery.h>
#include <akngloballistquery.h>

#ifdef __S60V3__
#include <es_enum.h>
#  if DO_QOS
#include <cs_subconparams.h>
#include <ip_subconparams.h>
#  endif
#endif

const TInt CConnectionOpener::MAX_RETRIES=2;

#ifndef __S60V2__
EXPORT_C CConnectionOpener* CConnectionOpener::NewL(MSocketObserver& Observer, RSocketServ& Serv)
#else
EXPORT_C CConnectionOpener* CConnectionOpener::NewL(MSocketObserver& Observer, RSocketServ& Serv, 
	RConnection& Connection, TBool aMonitorConnectivity)
#endif
{
#ifndef __S60V2__
	auto_ptr<CConnectionOpener> ret(new (ELeave) CConnectionOpener(Observer, Serv));
#else
	auto_ptr<CConnectionOpener> ret(new (ELeave) CConnectionOpener(Observer, Serv, Connection));
#endif
	ret->ConstructL(aMonitorConnectivity);
	return ret.release();
}

#define SETTING_LAST_CONNECTION_TIME 71
#define SETTING_LAST_CONNECTION_BYTECOUNT 72
#define SETTING_ALLOW_NETWORK_ACCESS 86
#define SETTING_ALLOW_NETWORK_ONCE 87
#define SETTING_ALLOW_ROAMING 49
#define SETTING_PRESENCE_ENABLE 9

#define SETTING_IDENTIFICATION_ERROR 90
#define SETTING_LAST_CONNECTION_SUCCESS 91
#define SETTING_LAST_CONNECTION_ATTEMPT 92
#define SETTING_LAST_CONNECTION_ERROR 93
#define SETTING_LAST_CONNECTION_REQUEST 94
#define SETTING_LATEST_CONNECTION_REQUEST 95

void CConnectionOpener::ConstructL(TBool aMonitorConnectivity)
{
	iWait=CTimeOut::NewL(*this);
	CActiveScheduler::Add(this);
	if (aMonitorConnectivity) iConnectivity=CConnectivityListener::NewL(*this);
}

#ifndef __S60V2__
CConnectionOpener::CConnectionOpener(MSocketObserver& Observer, RSocketServ &Serv) : 
CCheckedActive(EPriorityIdle, _L("ContextCommon::CConnectionOpener")), iObserver(Observer), iServ(Serv)
#else
CConnectionOpener::CConnectionOpener(MSocketObserver& Observer, RSocketServ &Serv, RConnection& Connection) : 
CCheckedActive(EPriorityIdle, _L("ContextCommon::CConnectionOpener")), iObserver(Observer), iServ(Serv), iConnection(Connection), iNetworkRegistrationStatusPckg(iNetworkRegistrationStatus),
	iCellIdPckg(iCellId), iImsiPckg(iImsi)
#endif
{
}

#include <netconerror.h>

EXPORT_C void CConnectionOpener::MakeConnectionL(TUint32 IapID)
{
	if (iConnectivity) iConnectivity->ResumeExpensiveL();
	if (IapID>30) {
		TInt x;
		x=0;
	}
	if (IapID == TUint(-1) ) {
		User::Leave(KErrAccessDenied);
	}
	Cancel();
	iIapId=IapID;

	TTimeIntervalMicroSeconds32 w(45*1000*1000);
	if (iMutex) {
		delete iMutex;
		iMutex=0;
	}
	iStatus=KRequestPending;
	SetActive();
	iMutex=CMutexRequest::NewL(* GetContext(), _L("ContextCommonConnectionInit"),
		w, &iStatus);

	current_state=WAITING_ON_MUTEX;
}

void CConnectionOpener::DoMakeConnectionL()
{
	TUint32 IapID=iIapId;

	TBuf<40> msg;
	iMadeConnection=false;

	msg.Format(_L("MakeConnectionL, iap %d"), IapID);
	iObserver.info(this, msg);
		
#ifndef __S60V2__

	if (!iInitiator) iInitiator=CIntConnectionInitiator::NewL();
	TUint32 activeiap;
	if (iInitiator->GetActiveIap(activeiap)==KErrNone && activeiap==IapID) {
		iObserver.info(this, _L("connection exists"));
		CreateSubConnectionAndTellOfSuccess();
		return;
	}
	
	iConnPref.iRanking = 1; 
	iConnPref.iDirection = ECommDbConnectionDirectionOutgoing; 
	iConnPref.iDialogPref = ECommDbDialogPrefDoNotPrompt; 
	CCommsDbConnectionPrefTableView::TCommDbIapBearer bearer; 
	bearer.iBearerSet = KMaxTUint32;
	bearer.iIapId = IapID; 
	iConnPref.iBearer = bearer;
	
	// we cannot really know if the initiator is usable
	// at this point (if retrying/reusing). Let's just
	// recreate it
	delete iInitiator; iInitiator=0;
	iInitiator=CIntConnectionInitiator::NewL();
	
	iStatus=KRequestPending;
	TInt ret=iInitiator->TerminateActiveConnection(iStatus);
	if (ret==KErrNone) {
		current_state=CLOSING;
		SetActive();
	} else {
		current_state=CONNECTING;
		iInitiator->ConnectL(iConnPref, iStatus);
		SetActive();
#  if defined(__WINS__)
		// we don't seem to get the right return on the
		// emulator :-(
		iStatus=KErrNone;
		return;
#  endif
	}
#else

	if (!iConnectionIsOpen) {
		User::LeaveIfError(iConnection.Open(iServ));
		iConnectionIsOpen=ETrue;
	}
	/*
	auto_ptr<CCommsDatabase> db(CCommsDatabase::NewL(EDatabaseTypeIAP));
	db->SetGlobalSettingL(TPtrC(ASK_USER_BEFORE_DIAL),(TInt)false);
	*/

	iConnPref.SetIapId(IapID);
	iConnPref.SetDirection(ECommDbConnectionDirectionOutgoing);
	iConnPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	iConnPref.SetBearerSet(ECommDbBearerUnknown);
	
	TBool connected = EFalse;
	
	TUint connectionCount;
	//Enumerate currently active connections across all socket servers
	User::LeaveIfError(iConnection.EnumerateConnections(connectionCount));

	TBool seen_connection=EFalse;
	if (connectionCount)
	{
		TPckgBuf<TConnectionInfo> connectionInfo;
		for (TUint i = 1; i <= connectionCount; ++i)
		{
			iConnection.GetConnectionInfo(i, connectionInfo);
			msg.Format(_L("existing conn, iap %d"), connectionInfo().iIapId);
			iObserver.info(this, msg);
			
			if (connectionInfo().iIapId == IapID)
			{
				TTime conn_time=TTime(0);
				TInt byte_count=0;
				RConnection tmp;
				seen_connection=ETrue;
#ifndef __S60V3__
				Settings().GetSettingL(SETTING_LAST_CONNECTION_TIME, conn_time);
				Settings().GetSettingL(SETTING_LAST_CONNECTION_BYTECOUNT, byte_count);
#endif				
				TInt ret;
				User::LeaveIfError(tmp.Open(iServ));
				CleanupClosePushL(tmp);
				ret=tmp.Attach(connectionInfo, RConnection::EAttachTypeNormal);
#ifndef __S60V3__
				{
					TUint down, up;
					TPckg<TUint> downp(down), upp(up);
					TRequestStatus s;
					tmp.DataTransferredRequest(upp, downp, s);
					User::WaitForRequest(s);
					TTime now; now=GetTime();
					if (s.Int()!=KErrNone || (
						conn_time < now-TTimeIntervalMinutes(10) &&
						down==byte_count) ) {
							tmp.Stop();
							ret=KErrGeneral;
							seen_connection=EFalse;
					} else if (down!=byte_count) {
						Settings().WriteSettingL(SETTING_LAST_CONNECTION_TIME, now);
						Settings().WriteSettingL(SETTING_LAST_CONNECTION_BYTECOUNT, (TInt)down);
					}
				}
#endif
				if (ret==KErrNone) {
#ifdef __S60V3__
					if (iHasSubConnection) {
						iSubConnection.Close();
						iHasSubConnection=EFalse;
					}
#endif
					if (iConnectionIsOpen) {
						iConnection.Close(); 
						iConnectionIsOpen=EFalse;
					}
					User::LeaveIfError(iConnection.Open(iServ));
					iConnectionIsOpen=ETrue;
					ret=iConnection.Attach(connectionInfo, RConnection::EAttachTypeNormal);
				}
				CleanupStack::PopAndDestroy(); //tmp
				if (ret==KErrNone) {
					connected = ETrue;
				} else {
					msg.Format(_L("Error in Attach %d"), ret);
					iObserver.info(this, msg);
				}
				break;
			}
			
		}
	}

#ifndef __S60V3__
	if (!seen_connection) {
		Settings().WriteSettingL(SETTING_LAST_CONNECTION_TIME, Time::MaxTTime());
		Settings().WriteSettingL(SETTING_LAST_CONNECTION_BYTECOUNT, 0);
	}
#endif
	if (connected) {
		iMadeConnection=false;
		iWait->Reset();
		delete iMutex; iMutex=0;
		iObserver.info(this, _L("already connected"));
		CreateSubConnectionAndTellOfSuccess();
	} else {
		Cancel();

		iStatus=KRequestPending;
		SetActive();
		
		if (iConnectivity && iConnectivity->LowSignal()) {
			TRequestStatus* s=&iStatus;
			User::RequestComplete(s, KErrNetConInadequateSignalStrengh);
		} else {
			iConnection.Start(iConnPref, iStatus);
		}
		current_state=CONNECTING;
#  ifdef __WINS__
		iWait->Wait(10);
#  else
		iWait->Wait(25);
#  endif
	}	
#endif
}

void CConnectionOpener::expired(CBase*)
{
	delete iMutex; iMutex=0;
	++iRetryCount;
	if (iRetryCount>=MAX_RETRIES || (iConnectivity && !iConnectivity->AllowReconnect())) {
		if (current_state==WAITING_ON_MUTEX) {
			TellOfError(this, -1003, _L("Opener retries exceeded in wait"));
		} else {
			Cancel();
#ifdef __S60V3__
			if (iHasSubConnection) {
				iSubConnection.Close();
				iHasSubConnection=EFalse;
			}
#endif
#ifdef __S60V2__
			if (iConnectionIsOpen) {
#  ifndef __S60V3__
				iConnection.Stop();
#  endif
				iConnection.Close(); 
				iConnectionIsOpen=EFalse;
			}
#endif
			current_state=IDLE;
			TellOfError(this, -1003, _L("Opener retries exceeded"));
		}
	} else {
		if (current_state==WAITING_ON_MUTEX) {
			iObserver.info(this, _L("Conn retry from wait"));
		} else {
			// close this here, so that we don't try to attach
			// to the failed connection
			Cancel();
#ifdef __S60V3__
			if (iHasSubConnection) {
				iSubConnection.Close();
				iHasSubConnection=EFalse;
			}
#endif
#ifdef __S60V2__
			if (iConnectionIsOpen) {
#  ifndef __S60V3__
				iConnection.Stop();
#  endif
				iConnection.Close(); 
				iConnectionIsOpen=EFalse;
			}
#endif
			iObserver.info(this, _L("Conn retry"));
		}
		current_state=IDLE;
#ifdef __WINS__
		iRetrying=ETrue;
#endif
		CC_TRAPD(err, MakeConnectionL(iIapId));
		if (err!=KErrNone) {
			TellOfError(this, err, _L("Opener retry failed"));
		}
	}
}

void CConnectionOpener::AskForPermissionL()
{
	if (iPermissionQuery) {
		iPermissionQuery->CancelListQuery();
		delete iPermissionQuery; iPermissionQuery=0;
	}
	iPermissionQuery=CAknGlobalListQuery::NewL();
	//FIXMELOC
	iPermissionQuery->SetHeadingL(_L("Jaiku: Go online?"));
	auto_ptr<CDesCArray> textArray(new (ELeave) CDesCArrayFlat(3));
	textArray->AppendL(_L("This time"));
	textArray->AppendL(_L("Always"));
	textArray->AppendL(_L("No"));
	current_state=ASKING_FOR_PERMISSION;
	iStatus=KRequestPending;
	SetActive();
	iPermissionQuery->ShowListQueryL(textArray.get(), iStatus, 1);
}
void CConnectionOpener::AskForRoamingL()
{
	if (iRoamingQuery) {
		iRoamingQuery->CancelConfirmationQuery();
		delete iRoamingQuery; iRoamingQuery=0;
	}
	iRoamingQuery=CAknGlobalConfirmationQuery::NewL();
	//FIXMELOC
	current_state=ASKING_FOR_ROAMING;
	iStatus=KRequestPending;
	SetActive();
	iRoamingQuery->ShowConfirmationQueryL(iStatus, _L("You are away from your home network. Data can be expensive. Suspend Jaiku?"),
		R_AVKON_SOFTKEYS_YES_NO);
}


void CConnectionOpener::GetRoamingStatusL()
{
#ifndef __WINS__
	if (!iTelephony) iTelephony=CTelephony::NewL();
	SetActive();
	iStatus=0;
	iTelephony->GetNetworkRegistrationStatus(iStatus, iNetworkRegistrationStatusPckg);
#else
	iNetworkRegistrationStatus.iRegStatus=CTelephony::ERegisteredOnHomeNetwork;
	TRequestStatus *s=&iStatus;
	iStatus=KRequestPending;
	SetActive();
	User::RequestComplete(s, KErrNone);
#endif
	current_state=GETTING_ROAMING_STATUS;
}

void CConnectionOpener::GetImsi()
{
	if (!iTelephony) iTelephony=CTelephony::NewL();
	SetActive();
	iStatus=0;
	iTelephony->GetSubscriberId(iStatus, iImsiPckg);
	current_state=GETTING_IMSI;
}

void CConnectionOpener::GetCellId()
{
	if (!iTelephony) iTelephony=CTelephony::NewL();
	SetActive();
	iStatus=0;
	iTelephony->GetCurrentNetworkInfo(iStatus, iCellIdPckg);
	current_state=GETTING_CELLID;
}

#include <aknglobalnote.h>
#include "app_context_impl.h"

void ResetNetworkSuccessL()
{
	MSettings& sett=GetContext()->Settings();
	sett.WriteSettingL(SETTING_LAST_CONNECTION_SUCCESS, TTime(0));
}

#ifdef __S60V3__
	void CConnectionOpener::CreateSubConnectionAndSetQosL(TDes& state)
{
#if DO_QOS
	// Create the container for all sub connection parameters
	RSubConParameterBundle subconParams;
	CleanupClosePushL(subconParams);

	// Create a container for QoS sub connection parameters (Param bundle takes ownership)

	state = _L("CSubConParameterFamily::NewL");
	CSubConParameterFamily* qosFamily = CSubConParameterFamily::NewL(subconParams,
	      KSubConQoSFamily);

	// Create the requested generic parameter set for QoS (Qos family takes ownership)
	state = _L("req CSubConQosGenericParamSet::NewL");
	CSubConQosGenericParamSet* reqGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily,                               
	      CSubConParameterFamily::ERequested);

	// Set the requested Generic Parameters
	reqGenericParams->SetDownlinkBandwidth(8);
	reqGenericParams->SetUplinkBandwidth(8);

	// Create the acceptable generic parameter set for QoS (Qos family takes ownership)
	state = _L("acc CSubConQosGenericParamSet::NewL");
	CSubConQosGenericParamSet* accGenericParams = CSubConQosGenericParamSet::NewL(*qosFamily,                               
	       CSubConParameterFamily::EAcceptable);

	// Set the acceptable Generic Parameters
	accGenericParams->SetDownlinkBandwidth(8);
	accGenericParams->SetUplinkBandwidth(8);

	// Create a requested technology specific parameter set for QoS (Qos family takes ownership)
	state=_L("req CSubConQosR99ParamSet::NewL");
	CSubConQosIPLinkR99ParamSet* reqRel99Params = CSubConQosIPLinkR99ParamSet::NewL(*qosFamily,
	       CSubConParameterFamily::ERequested);

	// Set the requested Technology Specific Params
	reqRel99Params->SetTrafficClass(RPacketQoS::ETrafficClassBackground);

	// Create a acceptable technology specific parameter set for QoS (Qos family takes ownership)
	state=_L("acc CSubConQosR99ParamSet::NewL");
	CSubConQosIPLinkR99ParamSet* accRel99Params = CSubConQosIPLinkR99ParamSet::NewL(*qosFamily,
	       CSubConParameterFamily::EAcceptable);

	// Set the acceptable Technology Specific Params
	accRel99Params->SetTrafficClass(RPacketQoS::ETrafficClassBackground);

	// Now open the sub-connection as normal
	// Create a new sub-connection
	TInt err=iSubConnection.Open(iServ, RSubConnection::ECreateNew, iConnection);
	if (err==KErrNone) iHasSubConnection=ETrue;
	else User::Leave(err);

	// Set Properties of the sub-connection
	state=_L("subconn.SetParameters");
	User::LeaveIfError(iSubConnection.SetParameters(subconParams));

	// Destroy parameters
	CleanupStack::PopAndDestroy();         // subconParams

	// Fetch the granted qos
	//RSubConParameterBundle grantedParams;
	//subconn.GetParameters(grantedParams);
#endif
}
#endif

#include "reporting.h"

void CConnectionOpener::CreateSubConnectionAndTellOfSuccess()
{
#if 0
# ifdef __S60V3__
#  if !defined(DO_QOS) || ! DO_QOS
	if (iHasSubConnection) {
		iSubConnection.Close();
		iHasSubConnection=EFalse;
	}
	TInt err=iSubConnection.Open(iServ, RSubConnection::EAttachToDefault, iConnection);
	if (err==KErrNone) iHasSubConnection=ETrue;
#  else
	TBuf<60> state;
	TRAPD(err, CreateSubConnectionAndSetQosL(state));
	if (err!=KErrNone) {
		if (iHasSubConnection) iSubConnection.Close();
		state.Append(_L(" "));
		state.AppendNum(err);
		Reporting().DebugLog(state);
	}
	iHasSubConnection=EFalse;
#  endif
# endif
#endif
	if (iConnectivity) iConnectivity->SuspendExpensive();
	iObserver.success(this);
}

void CConnectionOpener::CheckedRunL()
{
	TBuf<50> msg;
	iWait->Reset();
	TBool make_connection=EFalse;
	TBool check_allowed=EFalse;
	TBool return_to_home=EFalse;
	
	if (current_state==ASKING_FOR_PERMISSION) {
		current_state=IDLE;
		if (iStatus==-1 || iStatus==2) {
			iAllowed=EFalse;
			Settings().WriteSettingL(SETTING_PRESENCE_ENABLE, EFalse);
			TellOfError(this, KErrAccessDenied, _L("user did not allow"));
			return;
		}
		iAllowed=ETrue;
		if (iStatus==1) {
			Settings().WriteSettingL(SETTING_ALLOW_NETWORK_ACCESS, ETrue);
		} else {
			Settings().WriteSettingL(SETTING_ALLOW_NETWORK_ONCE, ETrue);
		}
		make_connection=ETrue;
	}
        // Concepts:
        // !Detecting roaming!
	if (current_state==WAITING_ON_MUTEX) {
		TInt err=KErrNone;
		current_state=IDLE;
		if (iStatus==KErrNone) {
			GetRoamingStatusL();
		} else if (iStatus==KErrTimedOut) {
			expired(0);
		} else {
			err=iStatus.Int();
			TellOfError(this, err, _L("error getting mutex"));
		}
		return;
	}

	if (current_state==GETTING_ROAMING_STATUS) {
		current_state=IDLE;
		if (iStatus!=KErrNone) {
			TellOfError(this, iStatus.Int(), _L("error getting network registration"));
			return;
		}		
		switch (iNetworkRegistrationStatus.iRegStatus) {
			case CTelephony::ERegisteredOnHomeNetwork:
				{
				TInt roaming=ERoamingUnset;
				TBool allowed=EFalse;
				Settings().GetSettingL(SETTING_ALLOW_NETWORK_ACCESS, allowed);
				Settings().GetSettingL(SETTING_ALLOW_ROAMING, roaming);
				if ( (roaming==ERoamingNo || roaming==ERoamingYes) && allowed) {
					return_to_home=ETrue;
				}
				Settings().WriteSettingL(SETTING_ALLOW_ROAMING, ERoamingUnset);
				}
				break;
			case CTelephony::ERegisteredRoaming:
				GetCellId();
				return;
				break;
			default: 
				{
				TBuf<40> msg=_L("registration status: ");
				msg.AppendNum(iNetworkRegistrationStatus.iRegStatus);
				TellOfError(this, KErrNotReady, msg);
				return;
				}
		}
		check_allowed=ETrue;
	}
	if (current_state==GETTING_CELLID) {
		current_state=IDLE;
		if (iStatus!=KErrNone) {
			TellOfError(this, iStatus.Int(), _L("error getting cellid"));
			return;
		}
		GetImsi();
		return;
	}
	if (current_state==GETTING_IMSI) {
		current_state=IDLE;
		if (iStatus!=KErrNone) {
			TellOfError(this, iStatus.Int(), _L("error getting imsi"));
			return;
		}
		
		if (iImsi.iSubscriberId.Left(3).Compare(iCellId.iCountryCode)) {
			if (iRoamingAllowed==ERoamingUnset)
				Settings().GetSettingL(SETTING_ALLOW_ROAMING, iRoamingAllowed);
			if (iRoamingAllowed==ERoamingUnset) {
				ResetNetworkSuccessL();
				AskForRoamingL();
				return;
			}
			if (iRoamingAllowed==ERoamingNo) {
				TellOfError(this, KErrAccessDenied, _L("not allowed while roaming"));
				return;
			}
		} else {
			TBool allowed=EFalse;
			TInt roaming=ERoamingUnset;
			Settings().GetSettingL(SETTING_ALLOW_NETWORK_ACCESS, allowed);
			Settings().GetSettingL(SETTING_ALLOW_ROAMING, roaming);
			if ( (roaming==ERoamingNo || roaming==ERoamingYes) && allowed) {
				return_to_home=ETrue;
			}

			Settings().WriteSettingL(SETTING_ALLOW_ROAMING, ERoamingUnset);
		}
		check_allowed=ETrue;
	}
	if (return_to_home) {
		ResetNetworkSuccessL();
		auto_ptr<CAknGlobalNote> note( CAknGlobalNote::NewL() );
		note->ShowNoteL(EAknGlobalInformationNote, 
			_L("You have returned to your home network. Jaiku will now go online."));
	}
	if (current_state==ASKING_FOR_ROAMING) {
		if (iStatus==EAknSoftkeyYes) {
			iRoamingAllowed=ERoamingNo;
			Settings().WriteSettingL(SETTING_ALLOW_ROAMING, ERoamingNo);
			TellOfError(this, KErrAccessDenied, _L("user did not allow"));
			return;
		}
		Settings().WriteSettingL(SETTING_ALLOW_ROAMING, ERoamingYes);
		iRoamingAllowed=ERoamingYes;
		check_allowed=ETrue;
	}
	if (check_allowed) {
		if (!iAllowed) {
			Settings().GetSettingL(SETTING_ALLOW_NETWORK_ACCESS, iAllowed);
		}
		if (!iAllowed) {
			Settings().GetSettingL(SETTING_ALLOW_NETWORK_ONCE, iAllowed);
		}
		if (!iAllowed) {
			AskForPermissionL();
			return;
		}
		make_connection=ETrue;
	}
	if (make_connection) {
		CC_TRAPD(err, DoMakeConnectionL());
		if (err!=KErrNone) {
			delete iMutex; iMutex=0;
			current_state=IDLE;
			TellOfError(this, err, _L("error in makeconnection"));
		}
		return;
	}

#ifndef __S60V2__
	if ( (iStatus==KConnectionTerminated || iStatus==0)
		&& current_state==CLOSING) {
		iObserver.info(this, _L("prev conn closed"));
		// The initiator seems to close its handles after a TerminateActiveConnection
		// so it has to be recreated if we want to call some other methods
		delete iInitiator; iInitiator=0;
		iInitiator=CIntConnectionInitiator::NewL();
		iStatus=KRequestPending;
		SetActive();
		iInitiator->ConnectL(iConnPref, iStatus);
		current_state=CONNECTING;
		return;
	}
	
	if (iStatus==CONNECTED) return;
	
	if (iStatus!=KErrNone && iStatus!=KConnectionPref1Exists &&
		iStatus!=KConnectionPref1Created) {
		if (iRetryCount>=MAX_RETRIES || (iConnectivity && !iConnectivity->AllowReconnect())) {
			msg.Format(_L("Opener error %d"), iStatus.Int());
			TellOfError(this, iStatus.Int(), msg);
			delete iMutex; iMutex=0;
			return;
		} else {
			msg.Format(_L("Opener error %d (Retry)"), iStatus.Int());
			iObserver.info(this, msg);
			if (current_state==CONNECTING) {
				current_state=RETRYING_CONNECT;
			} else {
				current_state=RETRYING_CLOSE;
			}
			
			// it seems that we sometimes get an error even
			// though the connection gets established. We wait
			// for 15 secs, which should be enough to detect
			// the new connection
			delete iMutex; iMutex=0;
			iWait->Wait(10);
			return;
		}
	}
	
	// If the connection doesn't exist the initiator sends *2* requestcompletes:
	// one with KConnectionPref1Exists and one with KConnectionPref1Created
	// if it does only KConnectionPref1Exists is send.
	
	if (current_state==CONNECTING) {
		iMadeConnection=true;
		delete iMutex; iMutex=0;
		CreateSubConnectionAndTellOfSuccess();
		current_state=CONNECTED;
	}
	
	if (iStatus==KConnectionPref1Exists) {
		iStatus=KRequestPending;
		SetActive();
	}
#else // __S60V2__
	msg.Format(_L("ConnectionOpener CheckedRunL state %d status %d"), current_state, iStatus.Int());
	iObserver.info(this, msg);
	
	TInt status=iStatus.Int();
#ifdef __WINS__
	iStatus=KRequestPending;
	SetActive();
#endif
	if (status==CONNECTED) {
		return;
	}

	if (status==KErrNone || status==KErrAlreadyExists) {
		if (current_state==CONNECTING) {
			iMadeConnection=true;
			delete iMutex; iMutex=0;
			current_state=CONNECTED;
			CreateSubConnectionAndTellOfSuccess();
		}
	} else {
		msg.Format(_L("Opener error %d (Retry)"), status);
		iObserver.info(this, msg);
		if (current_state==CONNECTING) {
			current_state=RETRYING_CONNECT;
		} else {
			current_state=RETRYING_CLOSE;
		}
		
		// it seems that we sometimes get an error even
		// though the connection gets established. We wait
		// for 15 secs, which should be enough to detect
		// the new connection
		delete iMutex; iMutex=0;
		
		if (iConnectivity && !iConnectivity->AllowReconnect()) {
			iWait->Wait(0);
		} else {
			iWait->Wait(10);
		}
		return;
	}
#endif
}

#ifdef __S60V3__
EXPORT_C RSubConnection& CConnectionOpener::SubConnection()
{
	return iSubConnection;
}

EXPORT_C TBool CConnectionOpener::HasSubConnection()
{
	return iHasSubConnection;
}
#endif

EXPORT_C bool CConnectionOpener::MadeConnection()
{
	return iMadeConnection;
}

void CConnectionOpener::DoCancel()
{
	if (current_state==WAITING_ON_MUTEX) {
		delete iMutex; iMutex=0;
	} else if (current_state==GETTING_ROAMING_STATUS) {
		iTelephony->CancelAsync(CTelephony::EGetNetworkRegistrationStatusCancel);
	} else if (current_state==GETTING_CELLID) {
		iTelephony->CancelAsync(CTelephony::EGetCurrentNetworkInfoCancel );
	} else if (current_state==GETTING_IMSI) {
		iTelephony->CancelAsync(CTelephony::EGetSubscriberIdCancel );
	} else if (current_state==ASKING_FOR_PERMISSION) {
		iPermissionQuery->CancelListQuery();
	} else if (current_state==ASKING_FOR_ROAMING) {
		iRoamingQuery->CancelConfirmationQuery();
	} else {
#ifndef __S60V2__
		iInitiator->Cancel();
#else
#ifdef __S60V3__
		if (iHasSubConnection) {
			iSubConnection.Close(); iHasSubConnection=EFalse;
		}
#endif
		if (iConnectionIsOpen) {
			iConnection.Close(); iConnectionIsOpen=EFalse;
		}
#ifdef __WINS__
		if (1 || !iRetrying) {
#endif
			if (iStatus==KRequestPending) {
				TRequestStatus *s=&iStatus;
				User::RequestComplete(s, KErrCancel);
			}
#ifdef __WINS__
		}
		iRetrying=EFalse;
#endif
		TInt err=iConnection.Open(iServ);
		if (err==KErrNone) {
			iConnectionIsOpen=ETrue;
		}
#endif
	}
	if (iStatus==KRequestPending) {
		TInt x;
		x=0;
	}
}

TInt CConnectionOpener::CheckedRunError(TInt aError)
{
	iWait->Reset();
	if (current_state==CONNECTING) {
		TBuf<50> msg;
		msg.Format(_L("CConnectionOpener::CheckedRunError %d"), aError);
		TellOfError(this, aError, msg);
	}
	return KErrNone;
}

void CConnectionOpener::TellOfError(CBase* source, TInt code, const TDesC& reason)
{
	if (iConnectivity) iConnectivity->SuspendExpensive();
	iObserver.error(source, code, reason);
}

EXPORT_C void CConnectionOpener::CloseConnection(TBool doStop)
{
	
	iMadeConnection=false;
	iWait->Reset();
	iRetryCount=0;
	Cancel();
	delete iMutex; iMutex=0;
#ifndef __S60V2__
	CC_TRAPD(err,
		if (!iInitiator) iInitiator=CIntConnectionInitiator::NewL();
		iInitiator->TerminateActiveConnection();
		);
		delete iInitiator; iInitiator=0;
#else
		if (doStop && iConnectionIsOpen) {
			//iStatus=KRequestPending;
			iConnection.Stop();
			//SetActive();
		}
#ifdef __S60V3__
		if (iHasSubConnection) {
			iSubConnection.Close();
			iHasSubConnection=EFalse;
		}
#endif
		if (iConnectionIsOpen) {
			iConnection.Close();
			iConnectionIsOpen=EFalse;
		}
#endif
	current_state=IDLE;
}

EXPORT_C CConnectionOpener::~CConnectionOpener()
{
	Cancel();
	delete iMutex;
	delete iWait;

	delete iTelephony;
	delete iPermissionQuery;
	delete iRoamingQuery;
#ifdef __S60V3__
	if (iHasSubConnection) iSubConnection.Close();
#endif
#ifndef __S60V2__
	if (iInitiator && iMadeConnection) iInitiator->TerminateActiveConnection();
	delete iInitiator; iInitiator=0;
#else
	if (iConnectionIsOpen) iConnection.Close();
#endif
	delete iConnectivity;
}

#include <bautils.h>

void BootFileName(TDes& aInto, class RFs& aFs, TChar aDrive)
{
	aInto.Zero();
	aInto.Append(aDrive);
	aInto.Append(_L(":\\"));
#ifndef __S60V3__
	aInto.Append(_L("system\\"));
#endif
	aInto.Append(_L("data\\context\\"));
	aInto.Append(_L("booted.dat"));
}

EXPORT_C void CConnectionOpener::CreateBootFileL(class RFs& aFs, TChar aDrive)
{
	TFileName* fnp=new (ELeave) TFileName;
	CleanupStack::PushL(fnp);
	TFileName& fn=*fnp;
	BootFileName(fn, aFs, aDrive);
	RFile f; 
	User::LeaveIfError(f.Replace(aFs, fn, EFileWrite|EFileShareAny));
	f.Close();
	CleanupStack::PopAndDestroy(1);
}

void CConnectionOpener::ConnectivityStateChanged()
{
	if (iConnectivity->LowSignal() && iWait && iWait->IsActive())  iWait->Wait(0);
}

EXPORT_C void CConnectionOpener::CheckBootFileAndResetPermissionL(class RFs& aFs, TChar aDrive, MSettings& aSettings)
{
	TFileName* fnp=new (ELeave) TFileName;
	CleanupStack::PushL(fnp);
	TFileName& fn=*fnp;
	BootFileName(fn, aFs, aDrive);
	TInt err=aFs.Delete(fn);
	CleanupStack::PopAndDestroy();
	if (err==KErrNotFound || err==KErrPathNotFound) return;
	User::LeaveIfError(err);
	aSettings.WriteSettingL(SETTING_ALLOW_NETWORK_ONCE, EFalse);
	aSettings.WriteSettingL(SETTING_LAST_CONNECTION_REQUEST, TTime(0));
	aSettings.WriteSettingL(SETTING_LATEST_CONNECTION_REQUEST, TTime(0));
}

EXPORT_C void CConnectionOpener::ResetPermissionL(MSettings& aSettings)
{
	aSettings.WriteSettingL(SETTING_ALLOW_NETWORK_ONCE, EFalse);
}

EXPORT_C void AppendUrlEncoded(TDes& into, const TDesC& str)
{
	int i;
	for (i=0; i<str.Length(); i++) {
		TChar c(str[i]);
		TUint cval=(TUint)c & 0xff;
		if (c.IsAlphaDigit() && cval<0x7f) into.Append(c);
		else {
			TBuf<5> b;
			b.Format(_L("%%%02x"), cval);
			into.Append(b);
		}
	}
}

#ifdef __S60V2__
#include "contextvariant.hrh"
#include "reporting.h"

class CConnectionErrorListenerImpl : public CConnectionErrorListener, public MContextBase {
public:
	RConnection & iConnection;
	MConnectionErrorCallback& iCallback;
	CConnectionErrorListenerImpl(RConnection& aConnection, MConnectionErrorCallback& aCallback) :
		iConnection(aConnection), iCallback(aCallback) { }
	TNifProgressBuf iProgress;
	void ConstructL() {
		CActiveScheduler::Add(this);
		iStatus=KRequestPending;
		SetActive();
		iConnection.ProgressNotification(iProgress, iStatus);
	}
	void CheckedRunL() {
#ifdef __DEV__
		TBuf<100> msg=_L("ConnectionErrorListener::RunL, stage ");
		msg.AppendNum(iProgress().iStage);
		msg.Append(_L(" error "));
		msg.AppendNum(iProgress().iError);
		Reporting().DebugLog(msg);
#endif
		if (iProgress().iError) {
			iCallback.ConnectionDisconnected(iProgress().iError);
		} else {
			iStatus=KRequestPending;
			SetActive();
			iConnection.ProgressNotification(iProgress, iStatus);
		}
	}
	void DoCancel() {
		iConnection.CancelProgressNotification();
	}
	~CConnectionErrorListenerImpl() {
		Cancel();
	}
};

CConnectionErrorListener::CConnectionErrorListener() : CCheckedActive(EPriorityStandard, _L("CConnectionErrorListener")) { }

EXPORT_C CConnectionErrorListener* CConnectionErrorListener::NewL(RConnection& aConnection, MConnectionErrorCallback& aCallback)
{
	auto_ptr<CConnectionErrorListenerImpl> ret(new (ELeave) CConnectionErrorListenerImpl(aConnection, aCallback));
	ret->ConstructL();
	return ret.release();
}

#endif

class MCenRepNotifyCallback {
public:
	virtual void IntKeyChanged( TUint32 aKey, TInt aValue) = 0;
};

class MTelephonyNotifyCallback {
public:
	virtual void StateChanged(TInt aState, TInt aError) = 0;
};

class CCenRepNotifyHandler;
class CTelephonyNotifyHandler;

#ifndef __S60V3__
#include <CenRepNotifyHandler.h>
#include <CoreApplicationUIsSDKCRKeys.h>
#include <centralrepository.h>
#include <FeatMgr.h>
#else
#include <CoreApplicationUIsSDKCRKeys.h>
#include <FeatDiscovery.h>
#include <featureinfo.h>
#include <centralrepository.h>
#endif


_LIT(KNotifyHandler, "CCenRepNotifyHandler");

class CCenRepNotifyHandler : public CCheckedActive, public MContextBase {
public:
	enum TKeyType { EIntKey };
	static CCenRepNotifyHandler* NewL( MCenRepNotifyCallback& aCallback, CRepository& aRepository,
            TKeyType , TUint32 aKey )
	{
		CALLSTACKITEMSTATIC_N(_CL("CCenRepNotifyHandler"), _CL("NewL"));
		auto_ptr<CCenRepNotifyHandler> ret(new (ELeave) CCenRepNotifyHandler(aCallback, aRepository));
		ret->ConstructL(aKey);
		return ret.release();
    	}
    MCenRepNotifyCallback& iCallback;
    CRepository& iRepository;
    CCenRepNotifyHandler(MCenRepNotifyCallback& aCallback, CRepository& aRepository) :
    	CCheckedActive(CActive::EPriorityStandard, KNotifyHandler), iCallback(aCallback), iRepository(aRepository) { }
    TUint32 iKey;
    void ConstructL(TUint32 aKey) {
	CALLSTACKITEM_N(_CL("CCenRepNotifyHandler"), _CL("ConstructL"));
    	iKey=aKey;
    	CActiveScheduler::Add(this);
    }
    void StartListeningL() {
	CALLSTACKITEM_N(_CL("CCenRepNotifyHandler"), _CL("StartListeningL"));
    	User::LeaveIfError(iRepository.NotifyRequest(iKey, iStatus));
    	SetActive();
    }
    void DoCancel() {
	CALLSTACKITEM_N(_CL("CCenRepNotifyHandler"), _CL("DoCancel"));
    	iRepository.NotifyCancel(iKey);
    }
    void CheckedRunL() {
	CALLSTACKITEM_N(_CL("CCenRepNotifyHandler"), _CL("CheckedRunL"));
        TInt connAllowed( 1 );
        iRepository.Get( KCoreAppUIsNetworkConnectionAllowed, connAllowed );
        StartListeningL();
        iCallback.IntKeyChanged(iKey, connAllowed);
    }
    ~CCenRepNotifyHandler() {
	CALLSTACKITEM_N(_CL("CCenRepNotifyHandler"), _CL("~CCenRepNotifyHandler"));
    	Cancel();
    }
};

_LIT(KTelephonyNotify, "CTelephonyNotifyHandler");

#include <e32math.h>

class CTelephonyNotifyHandler : public CCheckedActive, public MContextBase {
public:
	static CTelephonyNotifyHandler* NewL(MTelephonyNotifyCallback& aCallback, 
		CTelephony::TNotificationEvent aEvent, CTelephony::TCancellationRequest aCancel,
		TDes8& aPckg) {
		
		CALLSTACKITEMSTATIC_N(_CL("CTelephonyNotifyHandler"), _CL("NewL"));
		auto_ptr<CTelephonyNotifyHandler> ret(new (ELeave) 
			CTelephonyNotifyHandler(aCallback, aEvent, aCancel, aPckg));
		ret->ConstructL();
		return ret.release();
	}
	~CTelephonyNotifyHandler() {
		CALLSTACKITEM_N(_CL("CTelephonyNotifyHandlerImpl"), _CL("~CTelephonyNotifyHandler"));
		Cancel();
#ifdef __WINS__
		iTimer.Close();
#endif
		delete iTelephony;
	}
	void StopRandom() {
#ifdef __WINS__
		Cancel();
		CTelephony::TSignalStrengthV1 s;
		CTelephony::TSignalStrengthV1Pckg p(s);
		p().iBar=7;
		iPckg=p;
		
#endif
	}
	void Suspend() {
		Cancel();
	}
	void ResumeL() {
		if (IsActive()) return;
		StartL();
	}
	void GetL() {
		if (iEvent==CTelephony::EVoiceLineStatusChange) {
			User::LeaveIfError(Telephony().GetLineStatus(CTelephony::EVoiceLine, iPckg));
		}
	}
private:
	CTelephonyNotifyHandler(MTelephonyNotifyCallback& aCallback, 
		CTelephony::TNotificationEvent aEvent, CTelephony::TCancellationRequest aCancel,
		TDes8& aPckg) : CCheckedActive(CActive::EPriorityHigh, KTelephonyNotify),
		iCallback(aCallback), iEvent(aEvent), iNotifyCancel(aCancel),
		iPckg(aPckg) { }
		
#ifdef __WINS__
	void Wait() {
		TReal r=Math::FRand(iSeed);
		//TBool low=(r>0.9);
		TBool low=EFalse;
		TReal waitf=r/2.0*1000.0*1000.0;
		User::LeaveIfError(iTimer.CreateLocal());
		TInt wait=waitf;
		iTimer.After( iStatus, wait );
		CTelephony::TSignalStrengthV1 s;
		CTelephony::TSignalStrengthV1Pckg p(s);
		p().iBar= low ? 1 : 7;
		iPckg=p;
	}
	
#endif
	void ConstructL() {
		CALLSTACKITEM_N(_CL("CTelephonyNotifyHandlerImpl"), _CL("ConstructL"));
		CActiveScheduler::Add(this);
#ifndef __WINS__
		iTelephony = CTelephony::NewL();
#endif
		StartL();
	}
	void StartL() {
#ifndef __WINS__
		if (iEvent==CTelephony::ESignalStrengthChange) {
			iTelephony->GetSignalStrength(iStatus, iPckg);
			iCancel=CTelephony::EGetSignalStrengthCancel;
		} else if (iEvent==CTelephony::EVoiceLineStatusChange) {
			TInt err=iTelephony->GetLineStatus(CTelephony::EVoiceLine, iPckg);
			TRequestStatus* s=&iStatus;
			User::RequestComplete(s, err);
		} else {
			User::Leave(KErrNotSupported);
		}		
		SetActive();
#else
		Wait();
		SetActive();
#endif
	}
	void CheckedRunL() {
		CALLSTACKITEM_N(_CL("CTelephonyNotifyHandlerImpl"), _CL("CheckedRunL"));
		User::After(TTimeIntervalMicroSeconds32(100));
#ifndef __WINS__
		{
			CALLSTACKITEM_N(_CL("CTelephonyNotifyHandlerImpl"), _CL("CheckedRunL-notifychange"));
			iTelephony->NotifyChange(iStatus, iEvent, iPckg);
		}
		iCancel=iNotifyCancel;
#else
		Wait();
#endif
		SetActive();
		iCallback.StateChanged(iEvent, iStatus.Int());
	}
	void DoCancel() {
		CALLSTACKITEM_N(_CL("CTelephonyNotifyHandlerImpl"), _CL("DoCancel"));
#ifndef __WINS__
		iTelephony->CancelAsync(iCancel);
#else
		iTimer.Cancel();
#endif
	}
	CTelephony* iTelephony;
	MTelephonyNotifyCallback& iCallback;
	CTelephony::TNotificationEvent iEvent;
	CTelephony::TCancellationRequest iCancel;
	CTelephony::TCancellationRequest iNotifyCancel;
	TDes8& iPckg;
#ifdef __WINS__
	RTimer iTimer;
	TInt64	iSeed;
#endif
};

class CConnectivityListenerImpl : public CConnectivityListener, public MContextBase,
	public MTelephonyNotifyCallback, public MCenRepNotifyCallback, public MTimeOut {
private:
	CConnectivityListenerImpl(MConnectivityCallback& aCallback);
	
	virtual void IntKeyChanged( TUint32 aKey, TInt aValue);
	virtual void StateChanged(TInt aState, TInt aError);

	virtual TBool LowSignal();
	virtual TBool CallInProgress();
	virtual TBool OfflineMode();
	virtual TBool AllowReconnect();
	virtual TBool AllowConnect();
	virtual void SuspendExpensive();
	virtual void ResumeExpensiveL();
	
	void NotifyChange();
	
	virtual void expired(CBase*);
	void ConstructL();
	~CConnectivityListenerImpl();
	void StopRandom() {
		iSignalHandler->StopRandom();
	}
	void SimulateCallStart();
	void SimulateCallEnd();
		
	MConnectivityCallback& iCallback;
#ifdef __S60V3__
	CTelephony::TSignalStrengthV1	iSignalStrength;
	CTelephony::TSignalStrengthV1Pckg iSignalStrengthPckg;
	CTelephony::TCallStatusV1	iCallStatus;
	CTelephony::TCallStatusV1Pckg	iCallStatusPckg;
	CTelephonyNotifyHandler	*iSignalHandler, *iCallHandler;
	class CRepository  *iOfflineRepository;
	class CCenRepNotifyHandler * iOfflineNotifyHandler;
#endif
	TBool iOfflineMode;
	friend class CConnectivityListener;
#ifdef __WINS__
	RChunk iSignalChunk;
#endif
	TInt iSimulatedCall;
};

CConnectivityListenerImpl::CConnectivityListenerImpl(MConnectivityCallback& aCallback) :
	iCallback(aCallback)
#ifdef __S60V3__
		, iSignalStrengthPckg(iSignalStrength), iCallStatusPckg(iCallStatus)
#endif
	{ }
	
void CConnectivityListenerImpl::NotifyChange()
{
	CALLSTACKITEM_N(_CL("CConnectivityListenerImpl"), _CL("NotifyChange"));
	iCallback.ConnectivityStateChanged();
}

void CConnectivityListenerImpl::SuspendExpensive()
{
	iSignalStrength.iBar=2;
	iCallStatus.iStatus=CTelephony::EStatusIdle;
	if (iSignalHandler) iSignalHandler->Suspend();
	if (iCallHandler) iCallHandler->Suspend();
}

void CConnectivityListenerImpl::ResumeExpensiveL()
{
	if (iSignalHandler) iSignalHandler->ResumeL();
	if (iCallHandler) iCallHandler->ResumeL();
}

void CConnectivityListenerImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CConnectivityListenerImpl"), _CL("ConstructL"));
#ifdef __S60V3__
    if( CFeatureDiscovery::IsFeatureSupportedL( KFeatureIdOfflineMode ) )
        {
        // check if connection is not allowed and
        // come up with a listerner on the offline setting
        TInt connAllowed( 1 );

        iOfflineRepository = ::CRepository::NewL( KCRUidCoreApplicationUIs );

        iOfflineRepository->Get( KCoreAppUIsNetworkConnectionAllowed, connAllowed );

        iOfflineMode = !connAllowed;

        iOfflineNotifyHandler = CCenRepNotifyHandler::NewL( *this, *iOfflineRepository,
            CCenRepNotifyHandler::EIntKey, (TUint32)KCoreAppUIsNetworkConnectionAllowed );
        iOfflineNotifyHandler->StartListeningL();
    }
#endif
#if defined(__S60V3__)
	iSignalHandler=CTelephonyNotifyHandler::NewL(*this, CTelephony::ESignalStrengthChange,
		CTelephony::ESignalStrengthChangeCancel, iSignalStrengthPckg);
#  if !defined(__WINS__)
	iCallHandler=CTelephonyNotifyHandler::NewL(*this, CTelephony::EVoiceLineStatusChange,
		CTelephony::EVoiceLineStatusChangeCancel, iCallStatusPckg);
#  endif
#endif
#ifdef __WINS__
	TInt err=iSignalChunk.CreateGlobal(_L("signalstate"), 16, 16);
	if (err!=KErrNone && err!=KErrAlreadyExists) User::Leave(err);
	if (err==KErrAlreadyExists) err=iSignalChunk.OpenGlobal(_L("signalstate"), EFalse);
	User::LeaveIfError(err);
#endif
}

CConnectivityListenerImpl::~CConnectivityListenerImpl()
{
	CALLSTACKITEM_N(_CL("CConnectivityListenerImpl"), _CL("~CConnectivityListenerImpl"));
#ifdef __S60V3__
	delete iOfflineNotifyHandler;
	delete iOfflineRepository;
#endif
	delete iSignalHandler;
	delete iCallHandler;
#ifdef __WINS__
	iSignalChunk.Close();
#endif
}

void CConnectivityListenerImpl::IntKeyChanged( TUint32 aKey, TInt aValue)
{
	CALLSTACKITEM_N(_CL("CConnectivityListenerImpl"), _CL("IntKeyChanged"));
	if (aKey==KCoreAppUIsNetworkConnectionAllowed) {
		iOfflineMode = !aValue;
		NotifyChange();
	}
}


void CConnectivityListenerImpl::expired(CBase*)
{
	// nothing here for now
}

TBool CConnectivityListenerImpl::OfflineMode()
{
	return iOfflineMode;
}

void CConnectivityListenerImpl::StateChanged(TInt aState, TInt aError)
{
#ifdef __WINS__
	TInt* ss=(TInt*)iSignalChunk.Base();
	*ss=iSignalStrength.iBar;
#endif
	NotifyChange();
}

TBool CConnectivityListenerImpl::LowSignal()
{
	TInt strength;
#ifndef __WINS__
	strength=iSignalStrength.iBar;
#else
	strength=*(TInt*)iSignalChunk.Base();
#endif
	if (strength>-1 && strength<=1) return ETrue;
	return EFalse;
}

TBool CConnectivityListenerImpl::CallInProgress()
{
	if (iSimulatedCall==1) return ETrue;
	if (iSimulatedCall==2) return EFalse;
	
#ifndef __WINS__
	if (iCallHandler && (! iCallHandler->IsActive())) {
		iCallHandler->GetL();
	}
	if (iCallStatus.iStatus!=CTelephony::EStatusIdle && 
		iCallStatus.iStatus!=CTelephony::EStatusUnknown &&
		iCallStatus.iStatus!=CTelephony::EStatusDisconnecting) return ETrue;
#endif
	return EFalse;
}

void CConnectivityListenerImpl::SimulateCallStart()
{
	iSimulatedCall=1;
	NotifyChange();
}

void CConnectivityListenerImpl::SimulateCallEnd()
{
	iSimulatedCall=2;
	NotifyChange();
}

TBool CConnectivityListenerImpl::AllowReconnect()
{
	if (LowSignal() || CallInProgress() || OfflineMode()) return EFalse;
	return ETrue;
}

TBool CConnectivityListenerImpl::AllowConnect()
{
	return ETrue;
}

EXPORT_C CConnectivityListener* CConnectivityListener::NewL(MConnectivityCallback& aCallback)
{
	auto_ptr<CConnectivityListenerImpl> ret(new (ELeave) CConnectivityListenerImpl(aCallback));
	ret->ConstructL();
	return ret.release();
}
