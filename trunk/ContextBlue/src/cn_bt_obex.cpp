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
#include "cn_bt_obex.h"

_LIT(KServerTransportName,"RFCOMM");

#define KBTServiceOBEX 0x1105

#include <btmanclient.h>
#include <btextnotifiers.h>
#include <btsdp.h>
#include <obex.h>
#include "mutexrequest.h"
#include "timeout.h"

#include "symbian_auto_ptr.h"
#include "reporting.h"

#ifdef __WINS__
//#define NO_BT
#endif

//#include <bteng.h>
// reverse engineered from bteng.lib
class RBTDeviceHandler {
public:
	IMPORT_C int  AreExistingConnectionsL(int &);
	IMPORT_C int  CloseAllBasebandConnectionsL(void);
};


_LIT(KBtAccessMutex, "ContextBtMutex");

#define ACCESS_WAIT_MSECS 500

/*
 * Concepts:
 * !Sending Bluetooth messages!
 * !Bluejacking!
 * !Closing Bluetooth connection!
 */


/*
 * There's a few tricks that this class has to do:
 * 
 *  - we use a mutex (CMutexRequest) to arbitrate access
 *    to the Bluetooth stack between these objects and CDiscoverImpl
 *  - after doing the SDP search, we don't immediately close
 *    the sdp object and don't immediately try to do the Obex send, it
 *    seems that these sometimes result in a KERN-EXEC 0. Instead
 *    we wait 1 sec before sending, and don't delete the sdp object
 *    before the next request
 *  - there's no 'Cancel' on the CObexClient, so we just delete it
 *    and manually complete the iStatus if necessary. There seems to
 *    be no way of stopping the connection before the other end
 *    says Yes/No, so we pull the rug from under the obex client by
 *    actually closing all bluetooth connection via RBTDeviceHandler
 *  - if asked to (aConnectCount), we connect-wait_a_bit-disconnect-connect
 *    so that the recipient gets multiple alerts, since the single beep
 *    is so easy to miss. That may be a bit nasty.
 *  - It took about a week to get it to actually work. Don't change it
 *    unless you really know what you are doing.
 *
 *  MR
 */

class CBtObexImpl: public CBtObex, 
	public MSdpAgentNotifier, public MSdpAttributeValueVisitor,
	public MContextBase, public MTimeOut {
private:
	CBtObexImpl(MApp_context& aContext, MObexNotifier& aNotifier);
	~CBtObexImpl();
	
	void ConstructL();

	virtual void SendMessage(const TBTDevAddr& aAddr, 
		CObexBaseObject* aObject,
		TInt	aConnectCount); // doesn't take ownership
	void CancelSend();

	virtual void DoCancel();
	virtual void CheckedRunL();
	virtual void TriggerConnectWaitIfCount();

	enum TCommState { IDLE, WAITING_ON_MUTEX, WAITING_FOR_ACCESS, 
		GETTING_SERVICE, WAITING_FOR_CONNECT, CONNECTING, SENDING_FILE,
		DISCONNECTING, ABORTING_FOR_NEW, ABORTING_FOR_CANCEL, 
		DISCONNECTING_FOR_CANCEL };
	TCommState iCurrentState;

	MObexNotifier& iNotifier;

	TBTDevAddr	iAddr;
	CSdpAgent* agent;
	CSdpSearchPattern* list;
	CObexClient* obex_client;
	CMutexRequest* iMutex;
	void ReleaseMutex();

	TObexBluetoothProtocolInfo info;
	TUint port;
	bool seen_rfcomm;

	CObexBaseObject* current_object;

	RTimer	iTimer;
#ifndef NO_BT
	RBTDeviceHandler iBtHandler;
#endif

	TInt iRetryCount;
	void RetryOrReport(TInt aError, MObexNotifier::TState aState);

	void get_service();
	void release();
	void send_next_file(); 
	void Async();

	TInt	iRemainingConnects, iBetweenConnects;
	void expired(CBase* aSource);
	CTimeOut	*iTimeOut;
	enum TWaitState { EWaitToStopConnect, EWaitToStartConnect };
	TWaitState	iWaitState;
	MObexNotifier::TState	iStateBeforeCancel;
	TBool		iInCancel;
private:
	//MSdpAgentNotifier
	virtual void AttributeRequestComplete(TSdpServRecordHandle aHandle, TInt aError);
	virtual void AttributeRequestResult(TSdpServRecordHandle aHandle, 
		TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue);
	virtual void NextRecordRequestComplete(TInt aError, 
		TSdpServRecordHandle aHandle, TInt aTotalRecordsCount);
	//MSdpAttributeValueVisitor
	virtual void VisitAttributeValueL(CSdpAttrValue &aValue, TSdpElementType aType);
	virtual void StartListL(CSdpAttrValueList &aList);
	virtual void EndListL();

	friend class CBtObex;
	friend class auto_ptr<CBtObexImpl>;
};

EXPORT_C CBtObex::CBtObex() : CCheckedActive(EPriorityNormal, _L("CBtObex")) { }

CBtObexImpl::CBtObexImpl(MApp_context& Context, MObexNotifier& aNotifier) : 
	MContextBase(Context), iNotifier(aNotifier) //, targetServiceClass(0x2345)
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("CBtObexImpl"));

}

void CBtObexImpl::ConstructL() 
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("ConstructL"));

	User::LeaveIfError(iTimer.CreateLocal());
	CActiveScheduler::Add(this); // add to scheduler
	iTimeOut=CTimeOut::NewL(*this);
	iBetweenConnects=10;
}

EXPORT_C CBtObex* CBtObex::NewL(MApp_context& aContext, MObexNotifier& aNotifier)
{
	CALLSTACKITEM_N(_CL("CBtObex"), _CL("NewL"));

	auto_ptr<CBtObexImpl> ret(new (ELeave) CBtObexImpl(aContext, aNotifier));
	ret->ConstructL();
	return ret.release();
}

void CBtObexImpl::SendMessage(const TBTDevAddr& aAddr, 
		CObexBaseObject* aObject, TInt aConnectCount)
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("SendMessage"));
	//Reporting().DebugLog(_L("CBtObexImpl::SendMessage"));

	if (iCurrentState!=IDLE) iNotifier.Error(KErrNotReady, MObexNotifier::EInitializing);

	iRemainingConnects=aConnectCount;
	if (iRemainingConnects<0) iRemainingConnects=0;

	iRetryCount=0;

	release();
	current_object=aObject;
	iAddr=aAddr;
	
	TTimeIntervalMicroSeconds32 w(120*1000*1000);
	//Reporting().DebugLog(_L("obex:wait_on_mutex"));
	iMutex=CMutexRequest::NewL(AppContext(), KBtAccessMutex, w, &iStatus);
	iCurrentState=WAITING_ON_MUTEX;
	SetActive();
}

void CBtObexImpl::RetryOrReport(TInt aError, MObexNotifier::TState aState)
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("RetryOrReport"));

	++iRetryCount;
	release();
	if (iRetryCount<5) {
		iCurrentState=WAITING_FOR_ACCESS;
		TTimeIntervalMicroSeconds32 w(ACCESS_WAIT_MSECS*1000);
		iTimer.After(iStatus, w);
		SetActive();
		port=0;
	} else {
		ReleaseMutex();
		iNotifier.Error(aError, aState);
	}
}

void CBtObexImpl::release()
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("release"));

	delete agent; agent=0;
	delete list; list=0;
	
	delete obex_client; obex_client=0;
	if (iTimeOut) iTimeOut->Reset();
	iCurrentState=IDLE;
}

void CBtObexImpl::CancelSend()
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("CancelSend"));

	switch (iCurrentState) {
	case GETTING_SERVICE:
		iStateBeforeCancel=MObexNotifier::EGettingService;
		break;
	case CONNECTING:
		iStateBeforeCancel=MObexNotifier::EConnecting;
		break;
	case SENDING_FILE:
		iStateBeforeCancel=MObexNotifier::ESending;
		break;
	default:
		iStateBeforeCancel=MObexNotifier::EInitializing;
		break;
	}

	switch(iCurrentState) {
	case GETTING_SERVICE:
	case WAITING_FOR_CONNECT:
		iCurrentState=DISCONNECTING_FOR_CANCEL;
		break;
	case CONNECTING:
		{
#ifndef NO_BT
		iBtHandler.CloseAllBasebandConnectionsL();
#endif
		/*
		TRequestStatus s;
		obex_client->Disconnect(s);
		User::WaitForRequest(s);
		*/
		iCurrentState=DISCONNECTING_FOR_CANCEL;
		}
		break;
	case WAITING_ON_MUTEX:
	case WAITING_FOR_ACCESS:
	case ABORTING_FOR_NEW:
	case ABORTING_FOR_CANCEL:
	case DISCONNECTING_FOR_CANCEL:
		Cancel();
		iCurrentState=IDLE;
		ReleaseMutex();
		iNotifier.Cancelled(iStateBeforeCancel);
		break;
	case SENDING_FILE:
		obex_client->Abort();
		iCurrentState=ABORTING_FOR_CANCEL;
		break;
	case IDLE:
		iNotifier.Cancelled(iStateBeforeCancel);
		break;
	default:
		//Cancel();
 		User::Leave(KErrNotReady);
	};
}

CBtObexImpl::~CBtObexImpl()
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("~CBtObexImpl"));

	Cancel();
	
	iTimer.Close();

	release();
	delete iTimeOut;
	delete iMutex;
}

void CBtObexImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("DoCancel"));

	if (iCurrentState==SENDING_FILE) {
		obex_client->Abort();
	}
	if (iCurrentState==CONNECTING) {
#ifndef NO_BT
		CC_TRAPD(err, iBtHandler.CloseAllBasebandConnectionsL());
#endif
		/*iInCancel=ETrue;
		CActiveScheduler::Start();
		iInCancel=EFalse;*/
		obex_client->Error(KErrCancel);
		/*
		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrNone);
		*/
		/*
		TRequestStatus s;
		obex_client->Disconnect(s);
		User::WaitForRequest(s);
		*/
	}
	if (iCurrentState==WAITING_ON_MUTEX) {
		//Reporting().DebugLog(_L("ReleaseMutex"));
		ReleaseMutex();
	}
	if (iCurrentState==WAITING_FOR_ACCESS || iCurrentState==WAITING_FOR_CONNECT) {
		//Reporting().DebugLog(_L("timer cancel"));
		iTimer.Cancel();
	}

	if (iStatus==KRequestPending) {
		//Reporting().DebugLog(_L("delete obex"));
		delete obex_client; obex_client=0;
	}
	if (iStatus==KRequestPending) {
		//Reporting().DebugLog(_L("complete request"));

		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrNone);
	}
}

void CBtObexImpl::get_service()
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("get_service"));

	port=0;
	iCurrentState=GETTING_SERVICE;
	//if (agent) User::Panic(_L("BTOBEX"), 1);
	agent = CSdpAgent::NewL(*this, iAddr);

	//if (list) User::Panic(_L("BTOBEX"), 2);
	list = CSdpSearchPattern::NewL();
	list->AddL(KBTServiceOBEX);
	agent->SetRecordFilterL(*list);

	agent->NextRecordRequestL();
}

void CBtObexImpl::send_next_file()
{	
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("send_next_file"));

	release();

	info.iTransport.Copy(KServerTransportName);
	info.iAddr.SetBTAddr(iAddr);
	info.iAddr.SetPort(port);

	obex_client=CObexClient::NewL(info);

	obex_client->Connect(iStatus);
	iCurrentState=CONNECTING;
	TriggerConnectWaitIfCount();
		
	SetActive();
	
}

void CBtObexImpl::TriggerConnectWaitIfCount()
{
	if (iRemainingConnects) {
		--iRemainingConnects;
		iWaitState=EWaitToStopConnect;
		iTimeOut->Wait(iBetweenConnects);
	} else {
		iTimeOut->Reset();
	}
}

void CBtObexImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("CheckedRunL"));
#ifdef __WINS__
	{
		TBuf<100> msg=_L("CBtObexImpl::CheckedRunL state ");
		msg.AppendNum(iCurrentState);
		msg.Append(_L(" status "));
		msg.AppendNum(iStatus.Int());
		//Reporting().DebugLog(msg);
	}
#endif
	if (iInCancel) {
		CActiveScheduler::Stop();
	}

	if (iCurrentState==ABORTING_FOR_NEW) {
		obex_client->Connect(iStatus);
		iCurrentState=CONNECTING;
		TriggerConnectWaitIfCount();
		SetActive();
		return;
	}
	if (iCurrentState==ABORTING_FOR_CANCEL) {
		obex_client->Disconnect(iStatus);
		iCurrentState=DISCONNECTING_FOR_CANCEL;
		SetActive();
		return;
	}

	if (iStatus.Int()!=KErrNone) {
		MObexNotifier::TState st;
		switch(iCurrentState) {
		case SENDING_FILE:
		case CONNECTING:
			if (iRemainingConnects>0) {
				iRemainingConnects--;
				iCurrentState=CONNECTING;
				iWaitState=EWaitToStartConnect;
				expired(iTimeOut);
				return;
			}
			st=MObexNotifier::ESending;
			break;
		default:
			st=MObexNotifier::EInitializing;
			break;
		};
		iCurrentState=IDLE;
		release();
		ReleaseMutex();
		iNotifier.Error(iStatus.Int(), st);
		return;
	}
	switch(iCurrentState) {
	case WAITING_ON_MUTEX:
		{
		//Reporting().DebugLog(_L("obex:got_mutex"));
		iCurrentState=WAITING_FOR_ACCESS;
		TTimeIntervalMicroSeconds32 w(ACCESS_WAIT_MSECS*1000);
		iTimer.After(iStatus, w);
		SetActive();
		}
		break;
	case WAITING_FOR_ACCESS:
		//Reporting().DebugLog(_L("obex:get_service"));
#ifndef NO_BT
		get_service();
#else
		ReleaseMutex();
		iCurrentState=IDLE;
		iNotifier.Success();
		return;
#endif
		break;
	case CONNECTING:
		iTimeOut->Reset();
		obex_client->Put(*current_object, iStatus);
		iCurrentState=SENDING_FILE;
		SetActive();
		break;
	case SENDING_FILE:
		iCurrentState=DISCONNECTING;
		obex_client->Disconnect(iStatus);
		SetActive();
		break;
	case DISCONNECTING:
		release();
		ReleaseMutex();
		iNotifier.Success();
		break;
	case WAITING_FOR_CONNECT:
	case GETTING_SERVICE:
		CC_TRAPD(err, send_next_file());
		if (err!=KErrNone) {
			Cancel();
			release();
			ReleaseMutex();
			iNotifier.Error(err, MObexNotifier::EInitializing);
		}
		break;
	case IDLE:
		ReleaseMutex();
		iNotifier.Error(KErrGeneral, MObexNotifier::EInitializing);
		break;
	case DISCONNECTING_FOR_CANCEL:
		ReleaseMutex();
		iNotifier.Cancelled(iStateBeforeCancel);
		break;
	}
}

void CBtObexImpl::AttributeRequestComplete(TSdpServRecordHandle /*aHandle*/, TInt aError)
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("AttributeRequestComplete"));
	//Reporting().DebugLog(_L("AttributeRequestComplete"));

	if (aError!=KErrNone && aError!=KErrEof) {
		//Reporting().DebugLog(_L("AttributeRequestComplete:0:retry"));
		RetryOrReport(aError, MObexNotifier::EGettingService);
	} else if (aError==KErrNone) {
		CC_TRAPD(err, agent->NextRecordRequestL());
		if (err!=KErrNone) {
			//Reporting().DebugLog(_L("AttributeRequestComplete:1:retry"));
			RetryOrReport(aError, MObexNotifier::EGettingService);
		}
	}
}

void CBtObexImpl::AttributeRequestResult(TSdpServRecordHandle /*aHandle*/, 
				      TSdpAttributeID /*aAttrID*/, CSdpAttrValue* aAttrValue)
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("AttributeRequestResult"));
	//Reporting().DebugLog(_L("AttributeRequestResult"));

	auto_ptr<CSdpAttrValue> owner(aAttrValue);
	if (aAttrValue) 
		aAttrValue->AcceptVisitorL(*this);
}

void CBtObexImpl::VisitAttributeValueL(CSdpAttrValue &aValue, TSdpElementType aType)
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("VisitAttributeValueL"));
	//Reporting().DebugLog(_L("VisitAttributeValueL"));

	if (aType==ETypeUUID) {
		if (aValue.UUID()==KRFCOMM) {
			seen_rfcomm=true;
		} else {
			seen_rfcomm=false;
		}
	} else if (aType==ETypeUint && seen_rfcomm) {
		port=aValue.Uint();
	}
}

void CBtObexImpl::StartListL(CSdpAttrValueList &/*aList*/)
{
	//Reporting().DebugLog(_L("StartListL"));

}

void CBtObexImpl::EndListL()
{
	//Reporting().DebugLog(_L("EndListL"));

}

void CBtObexImpl::ReleaseMutex()
{
	CALLSTACKITEM_N(_CL("CBtObexImpl"), _CL("ReleaseMutex"));

	if (!iMutex) return;

	delete iMutex;iMutex=0;
	//Reporting().DebugLog(_L("obex::releasemutex"));
}

void CBtObexImpl::NextRecordRequestComplete(TInt aError, 
					 TSdpServRecordHandle aHandle, TInt aTotalRecordsCount)
{
	//Reporting().DebugLog(_L("NextRecordRequestComplete"));
	TBuf<30> msg;
	if (aError==KErrEof) {
		//Reporting().DebugLog(_L("NextRecordRequestComplete::Eof"));
		if (port==0) {
			//Reporting().DebugLog(_L("NextRecordRequestComplete::Eof::noport"));
			//_LIT(err, "didn't find service");
			// release();
			iCurrentState=IDLE;
			ReleaseMutex();
			iNotifier.Error(KErrNotFound, MObexNotifier::EGettingService);
		} else {
			//Reporting().DebugLog(_L("NextRecordRequestComplete::Eof::found"));
			// release();
			//_LIT(f, "found port %d");
			//TBuf<30> msg;
			//msg.Format(f, port);
			//cb->status_change(msg);
			//connect_to_service();
			/*iCurrentState=GETTING_SERVICE; */

			TTimeIntervalMicroSeconds32 w(500*1000);
			iTimer.After(iStatus, w);
			SetActive();

			if (iCurrentState==GETTING_SERVICE)
				iCurrentState=WAITING_FOR_CONNECT;
			return;
		} 
	} else if (aError!=KErrNone) {
		//Reporting().DebugLog(_L("NextRecordRequestComplete::Other_error"));
		//_LIT(err, "service error: %d");
		//msg.Format(err, aError);
		//Reporting().DebugLog(_L("NextRecordRequestComplete:0:retry"));
		RetryOrReport(aError, MObexNotifier::EGettingService);
	} else if(aTotalRecordsCount==0) {
		//Reporting().DebugLog(_L("NextRecordRequestComplete::0records"));
		release();
		ReleaseMutex();
		iNotifier.Error(KErrNotFound, MObexNotifier::EGettingService);
	} else {
		//Reporting().DebugLog(_L("NextRecordRequestComplete::attributerequest"));
		agent->AttributeRequestL(aHandle, KSdpAttrIdProtocolDescriptorList);
		return;
	}
}

void CBtObexImpl::expired(CBase* aSource)
{
	if (iTimeOut->IsActive()) iTimeOut->Reset();

	if (iCurrentState!=CONNECTING) return;

	if (iWaitState==EWaitToStopConnect) {
		iCurrentState=ABORTING_FOR_NEW;
		//obex_client->Abort();
#ifndef NO_BT
		CC_TRAPD(err, iBtHandler.CloseAllBasebandConnectionsL());
#endif
	} else {
		if (!obex_client) obex_client=CObexClient::NewL(info);
		obex_client->Connect(iStatus);
		iCurrentState=CONNECTING;
		TriggerConnectWaitIfCount();
		
		SetActive();
	}
}
