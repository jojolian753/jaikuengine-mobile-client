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

#include "bb_logger.h"

#include <blackboardclientsession.h>
#include "symbian_auto_ptr.h"

#include "independent.h"
#include "cl_settings.h"
#include <basched.h>
#include "bb_protocol.h"
#include "bb_listener.h"
#include "timeout.h"
#include "app_context_impl.h"
#include <s32mem.h>
#include "callstack.h"
#include "break.h"
#include "bb_settings.h"
#include "cl_settings.h"
#include "cl_settings_impl.h"
#include "bb_listener.h"


_LIT(KBBLogger, "CBBLogger");

const TComponentName KContextLogComponent = { { CONTEXT_UID_CONTEXT_LOG }, 1 };

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

class CBBLoggerImpl : public CBBLogger, public MContextBase, public MActiveNotifier, public MSettingListener {
private:
	virtual void NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, const CBBSensorEvent& aEvent);

	CBBLoggerImpl(MApp_context& aContext, i_status_notif* aCallBack);
	void ConstructL();
	~CBBLoggerImpl();
	void StartIfEnabledL();
	void Stop();
	void SettingChanged(TInt Setting);

	void ConnectL();
	RBBClient iBBClient;
	HBufC8*		iBuf;

	independent_worker iContextNetwork;
	struct TNetworkArgs {
		TBuf<50>	iHost;
		TInt		iPort;
		TBuf<50>	iAuthor;
		TInt		iAP;
	};
	void NotifyRunL(TInt aError);
	TInt	iRestarts;
	TNetworkArgs	iNetworkArgs;
	CActiveCallback*	iNwCallback;
	CApp_context*		iNwContext;
	TBool			iStopping;
	i_status_notif*		iCallBack;

	static TInt start_contextnw(TAny* aPtr);

	friend class CBBLogger;
	friend class auto_ptr<CBBLoggerImpl>;
};

CBBLogger* CBBLogger::NewL(MApp_context& aContext, i_status_notif* aCallBack)
{
	CALLSTACKITEM_N(_CL("CBBLogger"), _CL("NewL"));
	auto_ptr<CBBLoggerImpl> ret(new (ELeave) CBBLoggerImpl(aContext, aCallBack));
	ret->ConstructL();
	return ret.release();
}

CBBLogger::~CBBLogger()
{
	CALLSTACKITEM_N(_CL("CBBLogger"), _CL("~CBBLogger"));
}

CBBLoggerImpl::CBBLoggerImpl(MApp_context& aContext, i_status_notif* aCallBack) : 
	MContextBase(aContext), iCallBack(aCallBack)
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("CBBLoggerImpl"));
}

void CBBLoggerImpl::NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("NewSensorEventL"));
	return;

	if (! aEvent.iData() || aEvent.iPriority() < CBBSensorEvent::VALUE) return;

	while(1) {
		iBuf->Des().Zero();
		TPtr8 p=iBuf->Des();
		RDesWriteStream ws(p);
		CleanupClosePushL(ws);
		aEvent.Type().ExternalizeL(ws);
		CC_TRAPD(err, aEvent.ExternalizeL(ws));
		ws.CommitL();
		CleanupStack::PopAndDestroy();
		if (err==KErrNone) break;
		else if (err==KErrOverflow) iBuf=iBuf->ReAllocL( iBuf->Des().MaxLength()*2 );
		else User::Leave(err);
	}

	TInt errorcount=0;

	while (errorcount<3) {
		TRequestStatus s;
		TUint id;
		TTime t(0);
		iBBClient.Put(aName, KNullDesC, KContextLogComponent, 
			*iBuf, EBBPriorityNormal, EFalse, id, s, t);
		User::WaitForRequest(s);
		if (s.Int()==KErrNone) return;
		ConnectL();
	}
}

class TStopActive : public CActive, public MBBNotifier, public MTimeOut {
public:
	TStopActive() : CActive(EPriorityNormal), iProtocol(0) { }
	~TStopActive() { Cancel(); delete iTimer; }
	void ConstructL() { 
		iTimer=CTimeOut::NewL(*this); 
		CActiveScheduler::Add(this); 
		iStatus=KRequestPending; SetActive(); }
	void SetProtocol(CBBProtocol *aProtocol) { 
		iProtocol=aProtocol; 
		iProtocol->AddObserverL(this);
	}
	TRequestStatus* GetStatus() { return &iStatus; }

	TInt		iLastError, iAcks;
	TBool		iStoppedOnRequest;
private:
	void DoCancel() { 
		iStatus=KErrCancel; 
		TRequestStatus* s=&iStatus; 
		User::RequestComplete(s, KErrCancel); 
	}
	void RunL() {
		iStoppedOnRequest=ETrue;
		if (iProtocol) {
			iTimer->Wait(5);
			iProtocol->Disconnect(ETrue, ETrue);
		} else {
			CActiveScheduler::Stop();
		}
	}

	CBBProtocol	*iProtocol;
	CTimeOut	*iTimer;

	virtual void Acked(TUint id) { ++iAcks; }
	virtual void Error(TInt aError, TInt aOrigError, const TDesC& aDescr) { 
		iLastError=aError;
		iProtocol->Disconnect(EFalse, ETrue); 
		iTimer->Wait(5);
	}
	virtual void ReadyToWrite(TBool aReady) { }
	virtual void Disconnected(TBool aByStopRequest) { 
		if (aByStopRequest)
			iStoppedOnRequest=ETrue;
		iTimer->Reset(); 
		CActiveScheduler::Stop(); 
	}
	virtual void IncomingTuple(const CBBTuple* aTuple) { }
	void expired(CBase*) { CActiveScheduler::Stop(); }

};

class COwnScheduler : public CBaActiveScheduler {
public:
	void Error(TInt aError) const {
		User::Leave(aError);
		//CBaActiveScheduler::Error(aError);
	}
};

void start_contextnwL(const TDesC& aHost, TInt aPort, const TDesC& aAuthor, TInt aAP, worker_info* info)
{
	CALLSTACKITEM_N(_CL("CBaActiveScheduler"), _CL("Error"));
	auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("contextnetwork")));
	//auto_ptr<CActiveScheduler> activeScheduler(new (ELeave) CBaActiveScheduler);
	c->SetDebugLog(_L("Context"), _L("Network"));
	auto_ptr<CActiveScheduler> activeScheduler(new (ELeave) COwnScheduler);
	CActiveScheduler::Install(activeScheduler.get());

	auto_ptr<CBBDataFactory> bbf(CBBDataFactory::NewL());
	c->SetBBDataFactory(bbf.get());
	TNoDefaults t;
	auto_ptr<CBlackBoardSettings> s(CBlackBoardSettings::NewL(*c, 
		t, KCLSettingsTuple));
	c->SetSettings(s.get());
	s.release();

	auto_ptr<TStopActive> stop(new (ELeave) TStopActive); stop->ConstructL();
	info->set_do_stop(stop->GetStatus());

	auto_ptr<CBBProtocol> proto(CBBProtocol::NewL(*c));
	stop->SetProtocol(proto.get());
	auto_ptr<CBBListener> listen(CBBListener::NewL(proto.get(), *c));
	proto->AddObserverL(listen.get());

	proto->ConnectL(aAP, aHost, aPort, aAuthor);

	//User::Leave(KErrGeneral);
	CActiveScheduler::Start();

	if ( stop->iAcks == 0 && ! stop->iStoppedOnRequest)
		User::Leave(KContextErrTimeoutInBBProtocol);
	if ( ! stop->iStoppedOnRequest ) {
		TInt err=stop->iLastError;
		if (err==KErrNone) err=KErrUnknown;
		User::Leave(err);
	}
}

TInt CBBLoggerImpl::start_contextnw(TAny* aPtr) {
        CTrapCleanup *cl;
        cl=CTrapCleanup::New();

	User::__DbgMarkStart(RHeap::EUser);

	TTimeIntervalMicroSeconds32 w(5*1000*1000);
	User::After(w);

	worker_info *wi=(worker_info*)aPtr;
	TNetworkArgs *args=(TNetworkArgs*)wi->worker_args;

        TInt err=0;
        CC_TRAP2(err,
                start_contextnwL(args->iHost, args->iPort, args->iAuthor, args->iAP, wi), 0);

	w=TTimeIntervalMicroSeconds32(5*1000);
	// yield
	User::After(w);
	wi->stopped(err);

	User::After(w);

	User::__DbgMarkEnd(RHeap::EUser,0);

	return err;
}

void CBBLoggerImpl::StartIfEnabledL()
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("StartIfEnabledL"));
	iStopping=EFalse;
	TBool nw_enabled=EFalse;
	if (Settings().GetSettingL(SETTING_CONTEXTNW_ENABLED, nw_enabled) && nw_enabled) {
		if (Settings().GetSettingL(SETTING_CONTEXTNW_HOST, iNetworkArgs.iHost) &&
				Settings().GetSettingL(SETTING_CONTEXTNW_PORT, iNetworkArgs.iPort) &&
				Settings().GetSettingL(SETTING_IP_AP, iNetworkArgs.iAP) &&
				iNetworkArgs.iAP>0
				) {

			Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, iNetworkArgs.iAuthor);
			delete iNwCallback; iNwCallback=0;
			iNwCallback=CActiveCallback::NewL(*this);

			iContextNetwork.info.set_has_stopped(iNwCallback->GetStatus());
			iContextNetwork.start(_L("contextnw"), start_contextnw, (TAny*)&iNetworkArgs, EPriorityAbsoluteBackground);
		}
	}
}

void CBBLoggerImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("ConstructL"));
	Mlogger::ConstructL(AppContext());

	Settings().NotifyOnChange(SETTING_IP_AP, this);
	Settings().NotifyOnChange(SETTING_CONTEXTNW_PORT, this);
	Settings().NotifyOnChange(SETTING_CONTEXTNW_HOST, this);
	Settings().NotifyOnChange(SETTING_CONTEXTNW_ENABLED, this);

	iNwContext=CApp_context::NewL(true, _L("contextnetwork"));
	StartIfEnabledL();

	iBuf=HBufC8::NewL(2048);
	ConnectL();
}

void CBBLoggerImpl::Stop()
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("Stop"));
	iStopping=ETrue;
	iContextNetwork.stop();
	delete iNwCallback; iNwCallback=0;
}

CBBLoggerImpl::~CBBLoggerImpl()
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("~CBBLoggerImpl"));
	Stop();
	Settings().CancelNotifyOnChange(SETTING_IP_AP, this);
	Settings().CancelNotifyOnChange(SETTING_CONTEXTNW_PORT, this);
	Settings().CancelNotifyOnChange(SETTING_CONTEXTNW_HOST, this);
	Settings().CancelNotifyOnChange(SETTING_CONTEXTNW_ENABLED, this);
	iBBClient.Close();
	delete iBuf;
	delete iNwContext;
}

void CBBLoggerImpl::ConnectL()
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("ConnectL"));
	TInt errorcount=0;
	TInt err=KErrNone;
	while (errorcount<5) {
		iBBClient.Close();
		err=iBBClient.Connect();
		if (err==KErrNone) return;
		errorcount++;
	}
	User::Leave(err);
}

void CBBLoggerImpl::NotifyRunL(TInt aError)
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("NotifyRunL"));
	if (iStopping) return;
	if (aError==0) {
#ifdef TESTBUILD
		CActiveScheduler::Stop();
#endif
		return;
	}
	TBuf<100> msg=_L("ContextNetwork stopped, error: ");
	msg.AppendNum(aError);
	msg.Append(_L("callstack:"));
	if (iCallBack) iCallBack->error(msg);
	auto_ptr<HBufC> stack(0);
	
	++iRestarts;

	CC_TRAPD(err, stack.reset(iNwContext->CallStackMgr().GetFormattedCallStack(_L("ContextNetwork"))));
	if (err==KErrNone && stack.get()!=0) {
		if (iCallBack) iCallBack->status_change(*stack);
	}

	if (iRestarts>5 ||
		(aError==KContextErrTimeoutInBBProtocol && iRestarts>1) ) 
		User::Leave(aError);
	
	if (iCallBack) iCallBack->status_change(_L("restarting contextnetwork"));
	StartIfEnabledL();
}

void CBBLoggerImpl::SettingChanged(TInt Setting)
{
	CALLSTACKITEM_N(_CL("CBBLoggerImpl"), _CL("SettingChanged"));
	Stop();
	StartIfEnabledL();
}
