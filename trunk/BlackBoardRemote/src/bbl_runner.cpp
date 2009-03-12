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

#include "bbl_runner.h"
#include "break.h"

#include "independent.h"
#include <basched.h>
#include "bbl_protocol.h"
#include "bbl_listener.h"
#include "timeout.h"
#include "app_context_impl.h"
#include <s32mem.h>
#include "callstack.h"
#include "socketslistener.h"
#include "settings.h"
#include <in_sock.h>
#include <f32file.h>

_LIT(KBBLocalRunner, "CBBLocalRunner");

class MActiveNotifier {
public:
	virtual void NotifyRunL(TInt aError) = 0;
};

class CActiveCallback : public CActive {
public:
	void ConstructL() {
		CActiveScheduler::Add(this);
		iStatus=KRequestPending;
		SetActive();
	}
	CActiveCallback(MActiveNotifier& aNotifier) : CActive(EPriorityIdle), iNotifier(aNotifier) { }
	TRequestStatus* GetStatus() { return &iStatus; }
	static CActiveCallback* NewL(MActiveNotifier& aNotifier)  {
		auto_ptr<CActiveCallback> ret(new (ELeave) CActiveCallback(aNotifier));
		ret->ConstructL();
		return ret.release();
	}
	~CActiveCallback() { Cancel(); }
private:
	void RunL(void) {
		TInt err=iStatus.Int();
		iStatus=KRequestPending;
		SetActive();
		iNotifier.NotifyRunL(err);
	}
	void DoCancel() {
		iStatus=KErrCancel;
		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrCancel);
	}
	MActiveNotifier& iNotifier;
};

class CBBLocalRunnerImpl : public CBBLocalRunner, public MContextBase, 
	public MActiveNotifier, public MSettingListener {
private:
	CBBLocalRunnerImpl(MApp_context& aContext, i_status_notif* aCallBack);
	void ConstructL();
	~CBBLocalRunnerImpl();
	void StartIfEnabledL();
	void Stop();
	void SettingChanged(TInt Setting);

	independent_worker iContextNetwork;
	void NotifyRunL(TInt aError);
	TInt	iRestarts;
	CActiveCallback*	iNwCallback;
	CApp_context*		iNwContext;
	TBool			iStopping;
	i_status_notif*		iCallBack;

	static TInt start_bblocal(TAny* aPtr);

	friend class CBBLocalRunner;
	friend class auto_ptr<CBBLocalRunnerImpl>;
};

EXPORT_C CBBLocalRunner* CBBLocalRunner::NewL(MApp_context& aContext, i_status_notif* aCallBack)
{
	CALLSTACKITEM_N(_CL("CBBLocalRunner"), _CL("NewL"));
	auto_ptr<CBBLocalRunnerImpl> ret(new (ELeave) CBBLocalRunnerImpl(aContext, aCallBack));
	ret->ConstructL();
	return ret.release();
}

CBBLocalRunnerImpl::CBBLocalRunnerImpl(MApp_context& aContext, i_status_notif* aCallBack) : 
	MContextBase(aContext), iCallBack(aCallBack)
{
	CALLSTACKITEM_N(_CL("CBBLocalRunnerImpl"), _CL("CBBLocalRunnerImpl"));
}

class COwnScheduler : public CBaActiveScheduler {
public:
	void Error(TInt aError) const {
		User::Leave(aError);
		//CBaActiveScheduler::Error(aError);
	}
};

class CProtocolCreator : public CBase, public MSocketAcceptorFactory, public MBBLocalProtocolOwner {
public:
	MApp_context* c;
	void ConstructL() {
		iProtocols=CList<CBBLocalProtocol*>::NewL();
	}
private:
	CList<CBBLocalProtocol*> *iProtocols;
	TBool iInDelete;
	virtual void ProtocolDeleted(CBBLocalProtocol* aProtocol) {

		if (iInDelete) return;
		CList<CBBLocalProtocol*>::Node *i=0;
		for (i=iProtocols->iFirst; i; i=i->Next) {
			if (i->Item == aProtocol) {
				iProtocols->DeleteNode(i, ETrue);
				break;
			}
		}
	}
	~CProtocolCreator() {
		if (iProtocols) {
			iInDelete=ETrue;
			CList<CBBLocalProtocol*>::Node *i=0;
			for (i=iProtocols->iFirst; i; i=i->Next) {
				delete i->Item;
			}
			delete iProtocols;
		}
	}
	virtual MSocketAcceptor* CreateAcceptorL() {
		auto_ptr<CBBLocalProtocol> proto(CBBLocalProtocol::NewL(*c, *this));
		auto_ptr<CBBLocalListener> listen(CBBLocalListener::NewL(proto.get(), *c));
		proto->AddObserverL(listen.get());
		listen.release();
		iProtocols->AppendL(proto.get());
		return proto.release();
	}
};

class TStopActive : public CActive {
public:
	TStopActive() : CActive(EPriorityNormal) { }
	~TStopActive() { Cancel(); }
	void ConstructL() { 
		CActiveScheduler::Add(this); 
		iStatus=KRequestPending; 
		SetActive(); 
	}
	TRequestStatus* GetStatus() { return &iStatus; }

	TBool		iStoppedOnRequest;
private:
	void DoCancel() { 
		iStatus=KErrCancel; 
		TRequestStatus* s=&iStatus; 
		User::RequestComplete(s, KErrCancel); 
	}
	void RunL() {
		iStoppedOnRequest=ETrue;
		CActiveScheduler::Stop();
	}
};


void start_bblocalL(worker_info* info)
{
	CALLSTACKITEM_N(_CL("CActiveScheduler"), _CL("Stop"));
	auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("bblocal")));
	//auto_ptr<CActiveScheduler> activeScheduler(new (ELeave) CBaActiveScheduler);
	c->SetDebugLog(_L("Context"), _L("BBLocal"));
	auto_ptr<CActiveScheduler> activeScheduler(new (ELeave) COwnScheduler);
	CActiveScheduler::Install(activeScheduler.get());

	auto_ptr<CBBDataFactory> bbf(CBBDataFactory::NewL());
	c->SetBBDataFactory(bbf.get());

	auto_ptr<TStopActive> stop(new (ELeave) TStopActive); stop->ConstructL();
	info->set_do_stop(stop->GetStatus());

	auto_ptr<CProtocolCreator> creator(new (ELeave) CProtocolCreator);
	creator->ConstructL();
	creator->c=c.get();
	auto_ptr<CSocketListener> listener(CSocketListener::NewL(*creator, 2000));
	CActiveScheduler::Start();

	if ( ! stop->iStoppedOnRequest ) {
		User::Leave(KErrUnknown);
	}
}

TInt CBBLocalRunnerImpl::start_bblocal(TAny* aPtr)
{
	CALLSTACKITEMSTATIC_N(_CL("CBBLocalRunnerImpl"), _CL("start_bblocal"));
        CTrapCleanup *cl;
        cl=CTrapCleanup::New();

	User::__DbgMarkStart(RHeap::EUser);

	worker_info *wi=(worker_info*)aPtr;

        TInt err=0;
        CC_TRAP2(err,
                start_bblocalL(wi), 0);

	TTimeIntervalMicroSeconds32 w(5*1000);
	// yield
	User::After(w);
	wi->stopped(err);

	User::After(w);

	User::__DbgMarkEnd(RHeap::EUser,0);

	return err;
}

void CBBLocalRunnerImpl::StartIfEnabledL()
{
	CALLSTACKITEM_N(_CL("CBBLocalRunnerImpl"), _CL("StartIfEnabledL"));
	delete iNwCallback; iNwCallback=0;
	iNwCallback=CActiveCallback::NewL(*this);

	iContextNetwork.info.set_has_stopped(iNwCallback->GetStatus());
	iContextNetwork.start(_L("bblocal"), start_bblocal, 0, EPriorityAbsoluteBackground);
}

void CBBLocalRunnerImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBLocalRunnerImpl"), _CL("ConstructL"));
	iNwContext=CApp_context::NewL(true, _L("contextnetwork"));
	StartIfEnabledL();
}

void CBBLocalRunnerImpl::Stop()
{
	CALLSTACKITEM_N(_CL("CBBLocalRunnerImpl"), _CL("Stop"));
	iStopping=ETrue;
	iContextNetwork.stop();
	delete iNwCallback; iNwCallback=0;
}

CBBLocalRunnerImpl::~CBBLocalRunnerImpl()
{
	CALLSTACKITEM_N(_CL("CBBLocalRunnerImpl"), _CL("~CBBLocalRunnerImpl"));
	Stop();
	delete iNwContext;
}

void CBBLocalRunnerImpl::NotifyRunL(TInt aError)
{
	CALLSTACKITEM_N(_CL("CBBLocalRunnerImpl"), _CL("NotifyRunL"));
	if (iStopping) return;
	TBuf<100> msg=_L("BBLocal stopped, error: ");
	msg.AppendNum(aError);
	msg.Append(_L("callstack:"));
	iCallBack->error(msg);
	auto_ptr<HBufC> stack(0);
	
	++iRestarts;

	CC_TRAPD(err, stack.reset(iNwContext->CallStackMgr().GetFormattedCallStack(_L("BBLocal"))));
	if (err==KErrNone && stack.get()!=0) {
		iCallBack->status_change(*stack);
	}

	if (iRestarts>5 ||
		(aError==KContextErrTimeoutInBBProtocol && iRestarts>2) ) 
		User::Leave(aError);
	
	iCallBack->status_change(_L("restarting BBLocal"));
	StartIfEnabledL();
}

void CBBLocalRunnerImpl::SettingChanged(TInt /*Setting*/)
{
	CALLSTACKITEM_N(_CL("CBBLocalRunnerImpl"), _CL("SettingChanged"));
	Stop();
	StartIfEnabledL();
}


class CSocketTestReaderImpl : public CSocketTestReader, public MContextBase {
	CSocketTestReaderImpl(MApp_context& aContext) : MContextBase(aContext) { }
	
	RSocketServ iServer;
	RSocket	    iSocket;
	RFile	    iFile;

	enum TState { EInitializing, EConnecting, EReading };
	TState iState;
	TBuf8<256>	iBuffer;
	TInetAddr	iAddress;
	void ConstructL(TUint aPort, const TDesC& aFileName) {
		User::LeaveIfError(iFile.Replace(Fs(), aFileName, EFileWrite|EFileShareAny));
		User::LeaveIfError(iServer.Connect());
		iAddress.SetAddress(INET_ADDR(127,0,0,1));
		iAddress.SetPort(aPort);
		User::LeaveIfError(iSocket.Open(iServer, KAfInet, KSockStream, KProtocolInetTcp));
		CActiveScheduler::Add(this);
		iSocket.Connect(iAddress, iStatus);
		iState=EConnecting;
		SetActive();
	}
	void CheckedRunL() {
		if (iStatus==KErrNone) {
			User::LeaveIfError(iFile.Write(iBuffer));
			iSocket.Read(iBuffer, iStatus);
			iState=EReading;
			SetActive();
		} else {
			iSocket.Close();
		}
	}
	void DoCancel() {
		if (iState == EConnecting) {
			iSocket.CancelConnect();
		} else {
			iSocket.CancelRead();
		}
	}
	~CSocketTestReaderImpl() {
		Cancel();
		iSocket.Close();
		iServer.Close();
		iFile.Close();
	}

	friend auto_ptr<CSocketTestReaderImpl>;
	friend CSocketTestReader;
};

EXPORT_C CSocketTestReader* CSocketTestReader::NewL(MApp_context& aContext, TUint aPort,
	const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CSocketTestReader"), _CL("NewL"));
	auto_ptr<CSocketTestReaderImpl> ret(new (ELeave) CSocketTestReaderImpl(aContext));
	ret->ConstructL(aPort, aFileName);
	return ret.release();
}

_LIT(KSocketTestReader, "CSocketTestReader");

CSocketTestReader::CSocketTestReader() : CCheckedActive(CActive::EPriorityLow, KSocketTestReader) { }
