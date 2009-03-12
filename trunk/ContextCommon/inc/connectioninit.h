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

#ifndef CC_CONNECTIONINIT_H_INCLUDED
#define CC_CONNECTIONINIT_H_INCLUDED 1

#include "ver.h"

#include <in_sock.h>
#include <es_sock.h>
#include "app_context.h"

#ifndef __S60V2__
#include <capcodec.h>
#include <intconninit.h>
#endif
#ifndef __S60V3__
#include <AgentClient.h>
#endif

#include "timeout.h"
#include "mutexrequest.h"
#include "checkedactive.h"
#include <Etel3rdParty.h>

#ifdef __S60V2__
#include <CommDbConnPref.h>
#endif

class MConnectivityCallback {
public:
	virtual void ConnectivityStateChanged() = 0;
};

class CConnectivityListener : public CBase {
public:
	IMPORT_C static CConnectivityListener* NewL(MConnectivityCallback& aCallback);
	virtual TBool LowSignal() = 0;
	virtual TBool CallInProgress() = 0;
	virtual TBool OfflineMode() = 0;
	virtual TBool AllowReconnect() = 0;
	virtual TBool AllowConnect() = 0;
	virtual void SuspendExpensive() = 0;
	virtual void ResumeExpensiveL() = 0;
	virtual void SimulateCallStart() = 0;
	virtual void SimulateCallEnd() = 0;	
};

class MSocketObserver {
public:
	virtual void success(CBase* source) = 0;
	virtual void error(CBase* source, TInt code, const TDesC& reason) = 0;
	virtual void info(CBase* source, const TDesC& msg) = 0;
};

class MNetworkConnection {
public:
        virtual void ResumeConnection() = 0;
        virtual void SuspendConnection() = 0;
};

#ifdef __S60V2__
class MConnectionErrorCallback {
public:
	virtual void ConnectionDisconnected(TInt aError) = 0;
};

class CConnectionErrorListener : public CCheckedActive {
public:
	IMPORT_C static CConnectionErrorListener* NewL(class RConnection& aConnection, MConnectionErrorCallback& aCallback);
protected:
	CConnectionErrorListener();
};
#endif

class CConnectionOpener : public CCheckedActive, public MTimeOut, public MContextBase, 
	public MConnectivityCallback {
	// separate class since the CIntConnectionInitiator gives multiple
	// events from single invokation :-(
public:
#ifndef __S60V2__
	IMPORT_C static CConnectionOpener* NewL(MSocketObserver& Observer, RSocketServ& Serv);
#else
	IMPORT_C static CConnectionOpener* NewL(MSocketObserver& Observer, RSocketServ& Serv, 
		RConnection& Connection, TBool aMonitorConnectivity=ETrue);
#endif
	IMPORT_C void MakeConnectionL(TUint32 Iap); // async
	void DoMakeConnectionL();
	IMPORT_C void CloseConnection(TBool doStop=EFalse); // sync
	IMPORT_C bool MadeConnection();
	IMPORT_C ~CConnectionOpener();
	
	IMPORT_C static void CreateBootFileL(class RFs& aFs, TChar aDrive);
	IMPORT_C static void CheckBootFileAndResetPermissionL(class RFs& aFs, TChar aDrive, MSettings& aSettings);
	IMPORT_C static void ResetPermissionL(MSettings& aSettings);
	enum TRoamingAllowed { ERoamingUnset, ERoamingYes, ERoamingNo };
#ifdef __S60V3__
	IMPORT_C RSubConnection& SubConnection();
	IMPORT_C TBool HasSubConnection();
#endif 
private:
#ifndef __S60V2__
	CConnectionOpener(MSocketObserver& Observer, RSocketServ& Serv);
#else
	CConnectionOpener(MSocketObserver& Observer, RSocketServ& Serv, RConnection& Connection);
#endif
	void ConstructL(TBool aMonitorConnectivity);
	enum state { IDLE, WAITING_ON_MUTEX, GETTING_ROAMING_STATUS, GETTING_CELLID, GETTING_IMSI, ASKING_FOR_ROAMING, ASKING_FOR_PERMISSION, CLOSING, RETRYING_CLOSE, CONNECTING, RETRYING_CONNECT, CONNECTED };
	state current_state;
	
	void CheckedRunL();
	void DoCancel();
	TInt CheckedRunError(TInt aError);
	void AskForPermissionL();
	void AskForRoamingL();
	void GetRoamingStatusL();
	void GetCellId();
	void GetImsi();
	void ConnectivityStateChanged();

	MSocketObserver& iObserver;

	TUint32		iIapId;
	bool		iMadeConnection;
	RSocketServ&	iServ;

#ifdef __S60V2__
	RConnection&	iConnection; TBool iConnectionIsOpen;
	TCommDbConnPref iConnPref;
#else
	CIntConnectionInitiator *iInitiator;
	CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref iConnPref;
#endif

	void CreateSubConnectionAndTellOfSuccess();
	void TellOfError(CBase* source, TInt code, const TDesC& reason);
#ifdef __S60V3__
	RSubConnection	iSubConnection; TBool iHasSubConnection;
	void CreateSubConnectionAndSetQosL(TDes& aState);
#endif
	TInt	iRetryCount;
	static const TInt MAX_RETRIES;
	CTimeOut	*iWait;
	void expired(CBase*);

	CMutexRequest*	iMutex;
	class CAknGlobalListQuery* iPermissionQuery;
	class CAknGlobalConfirmationQuery* iRoamingQuery;
	TBool	iAllowed; TInt iRoamingAllowed;
	
	CTelephony::TNetworkRegistrationV1  iNetworkRegistrationStatus;
	CTelephony::TNetworkRegistrationV1Pckg iNetworkRegistrationStatusPckg;
	CTelephony::TNetworkInfoV1Pckg iCellIdPckg;
	CTelephony::TNetworkInfoV1 iCellId;
	CTelephony::TSubscriberIdV1Pckg iImsiPckg;
	CTelephony::TSubscriberIdV1 iImsi;
	
	CTelephony *iTelephony;
	CConnectivityListener* iConnectivity;
#ifdef __WINS__
	TBool	iRetrying;
#endif
};

IMPORT_C void AppendUrlEncoded(TDes& into, const TDesC& str);

#endif
