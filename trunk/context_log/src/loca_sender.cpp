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

#include "loca_sender.h"

#include "i_logger.h"
#include "bluejack.h"
#include "symbian_auto_ptr.h"
#include "loca_logic.h"
#include "csd_bluetooth.h"
#include "db.h"
#include "timeout.h"
#include <e32math.h>
#include "cl_settings.h"
#include "reporting.h"
#include "app_context_impl.h"
#include "raii_f32file.h"

#ifdef  __WINS__
void LoadScriptL(CLocaLogic* cl, const TDesC& aScriptName)
{
	TFileName fn=_L("c:\\system\\data\\context\\scripts\\");
	fn.Append(aScriptName);
	RAFile f;
	f.OpenLA(GetContext()->Fs(), fn, EFileRead|EFileShareAny);
	TBuf8<256> buf8;
	TBuf<256> buf;
	_LIT(KScript, "script");
	auto_ptr<CBBString> ss(CBBString::NewL(KScript));
	while (f.Read(buf8)==KErrNone && buf8.Length()>0) {
		buf.Copy(buf8);
		ss->Append(buf);
	}
	if (aScriptName[0]=='_') {
		TPtrC script=aScriptName.Left(aScriptName.Length()-3);
		cl->NewScriptL(script, ss.get());
	} else {
		TPtrC tp=aScriptName.Mid(4);
		TPtrC script=tp.Left(tp.Length()-3);
		cl->NewScriptL(script, ss.get());
	}
}
#endif

class CLocaSenderImpl : public CLocaSender, public MContextBase,
	public Mlogger, public MObexNotifier, public MTimeOut,
	public MSettingListener {
private:
	CLocaSenderImpl(MApp_context& Context);
	~CLocaSenderImpl();
	void ConstructL();

	virtual CLocaLogic* GetLogic() {
		return iLocaLogic;
	}

	// Mlogger
	virtual void NewSensorEventL(const TTupleName& aName, 
		const TDesC& aSubName, const CBBSensorEvent& aEvent);

	// MObexNotifier
	virtual void	Error(TInt aError, MObexNotifier::TState  aAtState);
	virtual void	Success();
	virtual void	Cancelled(MObexNotifier::TState aStateBeforeCancel);

	// MTimeOut
	virtual void expired(CBase* Source);

	// MSettingListener
	virtual void SettingChanged(TInt Setting);

	// own
	void CancelSend();

	enum TState {
		EIdle,
		ESending,
		ECanceling
	};
	TState		iCurrentState;
	TBBBtDeviceInfo iSendingTo; TBTDevAddr iSendingToAddr;
	TInt		iSendingMessageId;
	TBuf<240>	iSendingWithName, iSendingWithTitle;

	CDb*		iDb;
	CBlueJack*	iBlueJack;
	CLocaLogic*	iLocaLogic;
	CTimeOut*	iTimeOut;

	TInt		iMessageTimeOut;
	TInt64		iRandomSeed;
	TBool		iEnabled;
	TBuf<50>	iNodeName;

	friend CLocaSender;
	friend auto_ptr<CLocaSenderImpl>;
};

CLocaSender* CLocaSender::NewL(MApp_context& Context)
{
	CALLSTACKITEM_N(_CL("CLocaSender"), _CL("NewL"));

	auto_ptr<CLocaSenderImpl> ret(new (ELeave) CLocaSenderImpl(Context));
	ret->ConstructL();
	return ret.release();
}


CLocaSenderImpl::CLocaSenderImpl(MApp_context& Context) : MContextBase(Context) { }

CLocaSenderImpl::~CLocaSenderImpl()
{
	CALLSTACKITEM_N(_CL("CLocaSenderImpl"), _CL("~CLocaSenderImpl"));

	Settings().CancelNotifyOnChange(SETTING_ENABLE_LOCA_BLUEJACK, this);
	Settings().CancelNotifyOnChange(SETTING_PUBLISH_AUTHOR, this);
	Settings().CancelNotifyOnChange(SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT, this);

	delete iBlueJack;
	delete iLocaLogic;
	delete iTimeOut;
	delete iDb;
}

void CLocaSenderImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLocaSenderImpl"), _CL("ConstructL"));

	Mlogger::ConstructL(AppContextAccess());

	iDb=CDb::NewL(AppContext(), _L("LOCALOGIC"), EFileWrite);

	iLocaLogic=CLocaLogic::NewL(AppContext(), iDb->Db());
	iBlueJack=CBlueJack::NewL(AppContext(), *this);
	iTimeOut=CTimeOut::NewL(*this);

	SubscribeL(KBluetoothTuple);
	SubscribeL(KRemoteBluetoothTuple);

	Settings().GetSettingL(SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT, iMessageTimeOut);
	Settings().NotifyOnChange(SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT, this);
	Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, iNodeName);
	Settings().NotifyOnChange(SETTING_PUBLISH_AUTHOR, this);

	iRandomSeed=GetTime().Int64();
	iEnabled=EFalse;
	Settings().GetSettingL(SETTING_ENABLE_LOCA_BLUEJACK, iEnabled);
	Settings().NotifyOnChange(SETTING_ENABLE_LOCA_BLUEJACK, this);

#if defined(__WINS__) && defined(CONTEXTLOCA)
	//LoadScriptL(iLocaLogic, _L("_general.py"));
	//LoadScriptL(iLocaLogic, _L("024_hangout.py"));
#endif
}

void CLocaSenderImpl::SettingChanged(TInt Setting)
{
	if (Setting==SETTING_ENABLE_LOCA_BLUEJACK)
		Settings().GetSettingL(SETTING_ENABLE_LOCA_BLUEJACK, iEnabled);
	else if (Setting==SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT)
		Settings().GetSettingL(SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT, iMessageTimeOut);
	else
		Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, iNodeName);

}

void CLocaSenderImpl::Cancelled(MObexNotifier::TState aStateBeforeCancel)
{
	//Reporting().DebugLog(_L("CLocaSenderImpl::Cancelled"));
	iTimeOut->Reset();
	iLocaLogic->Failed(iSendingTo, iSendingMessageId,
		GetTime(), CLocaLogic::ETimeOut, ETrue);
	iCurrentState=EIdle;
}

void CLocaSenderImpl::CancelSend()
{
	CALLSTACKITEM_N(_CL("CLocaSenderImpl"), _CL("CancelSend"));
	//Reporting().DebugLog(_L("CLocaSenderImpl::CancelSend"));

	iCurrentState=ECanceling;
	iBlueJack->CancelSend();
	iTimeOut->Wait(15);
}

void CLocaSenderImpl::NewSensorEventL(const TTupleName& , 
	const TDesC& aSubName, const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("CLocaSenderImpl"), _CL("NewSensorEventL"));

	//Reporting().DebugLog(_L("CLocaSenderImpl::NewSensorEventL"));
	if (!iEnabled) return;

	const CBBBtDeviceList* devices=bb_cast<CBBBtDeviceList>(aEvent.iData());
	if (!devices) return;
	if (aSubName.Length()>0) {
		iLocaLogic->UpdateStats(aSubName, devices, aEvent.iStamp());
		return;
	} else {
		if (devices->Count()<=1) return;
		iLocaLogic->UpdateStats(iNodeName, devices, aEvent.iStamp());
	}

	if (iCurrentState==ESending || iCurrentState==ECanceling) {
		// shouldn't happen
		CancelSend();
		return;
	}
	if (iCurrentState==ECanceling) {
		return;
	}

	const TBBBtDeviceInfo* i=0;
	TInt send_to=-1;
	auto_ptr<HBufC8> iSendingBody(0);
	iLocaLogic->GetMessage(devices, 
		aEvent.iStamp(), 
		send_to, iSendingMessageId,
		iSendingWithName, iSendingWithTitle,
		iSendingBody);
	if(send_to==-1) return;

	TInt ii;
	for(i=devices->First(), ii=0; i && ii<send_to; i=devices->Next(), ii++) {
	}
	if (!i) return;
	iSendingTo=*i;

	iCurrentState=ESending;
	TBTDevAddr a(iSendingTo.iMAC());
	iSendingToAddr=a;
	TInt connectcount=10;
	Settings().GetSettingL(SETTING_LOCA_BLUEJACK_CONNECT_COUNT, connectcount);
#ifdef __WINS__
	{
		TBuf<100> msg=_L("CLocaSender::Sending to ");
		TRAPD(err, iSendingTo.IntoStringL(msg));
		//Reporting().DebugLog(msg);
	}
#endif
	iBlueJack->SendMessageL(iSendingToAddr,
		iSendingWithName, iSendingWithTitle,
		*iSendingBody, connectcount);
	iTimeOut->Wait(iMessageTimeOut);
}

void CLocaSenderImpl::Error(TInt aError, MObexNotifier::TState aAtState)
{
	CALLSTACKITEM_N(_CL("CLocaSenderImpl"), _CL("Error"));
	//Reporting().DebugLog(_L("CLocaSenderImpl::Error"));

#ifdef __WINS__
	iTimeOut->Reset();
	TBuf<100> msg;
	msg=_L("locasender: error in send ");
	msg.AppendNum(aError);
	msg.Append(_L(" at state "));
	msg.AppendNum(aAtState);
	Reporting().DebugLog(msg);
#endif

	// FIXME: how to get error type?
	CLocaLogic::TSendFailure fail=CLocaLogic::EUnknown;
	if (aError==KErrCouldNotConnect && aAtState==MObexNotifier::ESending) {
		fail=CLocaLogic::ERefused;
	}
	iLocaLogic->Failed(iSendingTo, iSendingMessageId,
		GetTime(), fail, ETrue);
	iCurrentState=EIdle;
}

void CLocaSenderImpl::Success()
{
	CALLSTACKITEM_N(_CL("CLocaSenderImpl"), _CL("Success"));
	//Reporting().DebugLog(_L("CLocaSenderImpl::Success"));

	iTimeOut->Reset();
	iLocaLogic->Success(iSendingTo, iSendingMessageId, GetTime(), ETrue);
	iCurrentState=EIdle;
}

void CLocaSenderImpl::expired(CBase* Source)
{
	CALLSTACKITEM_N(_CL("CLocaSenderImpl"), _CL("expired"));

	CancelSend();
}
