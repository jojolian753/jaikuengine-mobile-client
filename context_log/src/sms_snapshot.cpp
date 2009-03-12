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
#include "sms_snapshot.h"
#include "camera.h"
#include "mms.h"
#include "list.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"
#include "transfer.h"
#include "uploadview.h"
#include "timeout.h"
#include "cm_post.h"
#include <bautils.h>
#include "presencemaintainer.h"

//#define TESTING 1

class TNoUploadPrompt : public MUploadPrompt {
	void Prompt(const TDesC& /*FileName*/, MUploadCallBack* CallBack);
};

void TNoUploadPrompt::Prompt(const TDesC& /*FileName*/, MUploadCallBack* CallBack)
{
	CALLSTACKITEM_N(_CL("TNoUploadPrompt"), _CL("Prompt"));

	CallBack->Back(false, true, 0);
}

class CSmsSnapshotImpl : public CSmsSnapshot, public MContextBase, public MSnapShotNofifier, public MSettingListener,
	public MTimeOut {
private:
	CSmsSnapshotImpl(MApp_context& Context, i_status_notif* cb, MPresenceMaintainer* Presence, CHttpTransfer2* Transferer);
	void ConstructL();

	virtual void Test();

	//from i_handle_received_sms
	virtual bool handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, const TDesC& sender, const TDesC& body); // return true if message is to be deleted
	virtual void handle_change(const TMsvId& , const TDesC& ) { }
	virtual void handle_delete(const TMsvId& , const TMsvId& , const TDesC& ) { }
	virtual void handle_move(const TMsvId& , const TMsvId& , const TMsvId& , const TDesC& ) { }
	virtual void handle_error(const TDesC& ) { }
	virtual void handle_sending(const TMsvId& , const TDesC& , const TDesC& ) { }
	virtual void handle_read(const TMsvId& msg_id, 
		const TDesC& sender, TUid aMtmUid, CBaseMtm* aMtm) { }

	// from MSnapShotNofifier
	virtual void Error(TInt aCode, const TDesC& aDescription);
	virtual void Taken(const TDesC& aFileName);
	virtual void TakenL(const TDesC& aFileName);
	virtual void Info(const TDesC& aMsg);

	// from MSettingListener
	virtual void SettingChanged(TInt Setting);

	// from MTimeOut
	virtual void expired(CBase* Source);

	struct TSendTo {
		TBuf<20>	iTo;
		TBuf<150>	iBody;
		TSendTo(const TDesC& To, const TDesC& Body) {
			iTo=To.Left(20);
			iBody=Body.Left(150);
		}
		TSendTo() : iTo(), iBody() { }
	};

	CList<TSendTo>*	iToBeSend;
	CSnapShot*	iSnapShot;
	CMMS*		iMMS;
	TBool		iEnabled;
	i_status_notif*	cb;
	MPresenceMaintainer* iPresence;
	CHttpTransfer2* iTransferer;
	TNoUploadPrompt	iDummyPrompt;
	TInt		iRetryCount;
	bool		iDeleting;
	TBuf<100>	iState;
	CTimeOut	*iTimer; int iInterval;

	friend class CSmsSnapshot;

public:
	~CSmsSnapshotImpl();
};

CSmsSnapshotImpl::~CSmsSnapshotImpl()
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("~CSmsSnapshotImpl"));

	iDeleting=true;

	Settings().CancelNotifyOnChange(SETTING_SNAPSHOT_ON_SMS, this);
	Settings().CancelNotifyOnChange(SETTING_SNAPSHOT_INTERVAL, this);
	delete iMMS;
	delete iToBeSend;
	delete iSnapShot;
	delete iTimer;
}

CSmsSnapshot* CSmsSnapshot::NewL(MApp_context& Context, i_status_notif* cb, class MPresenceMaintainer* Presence, CHttpTransfer2* Transferer)
{
	CALLSTACKITEM_N(_CL("CSmsSnapshot"), _CL("NewL"));


	auto_ptr<CSmsSnapshotImpl> ret(new (ELeave) CSmsSnapshotImpl(Context, cb, Presence, Transferer));
	ret->ConstructL();
	return ret.release();
}

CSmsSnapshotImpl::CSmsSnapshotImpl(MApp_context& Context, i_status_notif* i_cb, class MPresenceMaintainer* Presence, CHttpTransfer2* Transferer) : 
MContextBase(Context), cb(i_cb), iPresence(Presence), iTransferer(Transferer) { }

CSmsSnapshot::~CSmsSnapshot() { }

void CSmsSnapshotImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("ConstructL"));

	iToBeSend=CList<TSendTo>::NewL();
	iEnabled=EFalse;
	Settings().GetSettingL(SETTING_SNAPSHOT_ON_SMS, iEnabled);
	if (iEnabled) {
		iSnapShot=CSnapShot::NewL(AppContext());
	}
	Settings().NotifyOnChange(SETTING_SNAPSHOT_ON_SMS, this);
	iMMS=CMMS::NewL();
	iTimer=CTimeOut::NewL(*this);
	if (Settings().GetSettingL(SETTING_SNAPSHOT_INTERVAL, iInterval) &&
		iInterval>0 && iEnabled) {
			iTimer->Wait(iInterval);
	}
	Settings().NotifyOnChange(SETTING_SNAPSHOT_INTERVAL, this);
}

void CSmsSnapshotImpl::Test()
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("Test"));

	handle_reception(TMsvId(1), TMsvId(1), _L("0505536758"), _L("testing"));
}

bool CSmsSnapshotImpl::handle_reception(const TMsvId& , 
										const TMsvId& , const TDesC& sender, 
										const TDesC& body)
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("handle_reception"));


	 // return true if message is to be deleted
	if (!iEnabled) return false;
	//if (sender.Length()<4) return false;
	iToBeSend->AppendL(TSendTo(sender, body));

	cb->status_change(_L("taking picture"));
	iSnapShot->TakeSnapShot(DataDir(), this);

	return true;
}

void CSmsSnapshotImpl::Error(TInt aCode, const TDesC& aDescription)
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("Error"));

	if (iDeleting) return;

	iRetryCount++;
	if (iRetryCount<=5) {
		iSnapShot->TakeSnapShot(DataDir(), this);
		cb->status_change(aDescription);
		cb->status_change(_L("retrying snapshot"));
		return;
	}

	TSendTo t=iToBeSend->Pop();
	auto_ptr<HBufC> b(HBufC::NewL(aDescription.Length()+30+t.iBody.Length()));
	b->Des().Append(_L("Failed to take picture: "));
	b->Des().Append(aDescription);
	b->Des().Append(_L(" for "));
	b->Des().Append(t.iBody);
#ifndef TESTING
	if (t.iTo.Length()>0)
		iMMS->SendMessage(t.iTo, *b, _L(""), t.iBody, false);
#endif

	b->Des().Zero();
	b->Des().Append(aDescription);
	b->Des().Append(_L(": "));
	b->Des().AppendNum(aCode);
	cb->error(*b);
}

void CSmsSnapshotImpl::TakenL(const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("TakenL"));

	TFileName recfilename=aFileName;
	recfilename.Replace(recfilename.Length()-3, 3, _L("amr"));

	iState=_L("create array");
	auto_ptr<CDesCArrayFlat> a(new (ELeave) CDesCArrayFlat(2));
	a->AppendL(aFileName);

	if (BaflUtils::FileExists(Fs(), recfilename))
		a->AppendL(recfilename);

	TSendTo t=iToBeSend->Pop();
#ifndef TESTING

	iState=_L("send MMS");
	auto_ptr<HBufC> b(HBufC::NewL(t.iBody.Length()+t.iTo.Length()+10));
	b->Des().Append(t.iBody);
	if (t.iTo.Length()>0) {
		cb->status_change(_L("sending MMS"));
		iMMS->SendMessage(t.iTo, *b, *a, t.iBody, false);

		b->Des().Append(_L(" from "));
		b->Des().Append(t.iTo);
	}

	iState=_L("makepacket");
	bb_auto_ptr<CCMPost> buf(CCMPost::NewL(BBDataFactory()));
	buf->iPresence.SetValue(bb_cast<CBBPresence>(iPresence->Data()->CloneL(KNullDesC)));
	buf->iBodyText->Append(*b);

	iState=_L("send pic");
	iTransferer->AddFileToQueueL(aFileName, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, 
		true, _L("Unknown"), buf.get(), 120);
	iState=_L("send rec");
	if (BaflUtils::FileExists(Fs(), recfilename))
		iTransferer->AddFileToQueueL(recfilename, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, 
			true, _L("Unknown"), buf.get(), 120);
	iState=_L("destroy");
#else
	iTransferer->AddFileToQueueL(aFileName, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, 
		true, _L("SMS"), buf.get(), 120);
	if (BaflUtils::FileExists(Fs(), recfilename))
		iTransferer->AddFileToQueueL(recfilename, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, 
			true, _L("SMS"), buf.get(), 120);
#endif

}

void CSmsSnapshotImpl::Taken(const TDesC& aFileName)
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("Taken"));


	iRetryCount=0;

	CC_TRAPD(err, TakenL(aFileName));

	if (err!=KErrNone) {
		iState.Append(_L(": "));
		iState.AppendNum(err);
		cb->status_change(iState);
	}

}

void CSmsSnapshotImpl::Info(const TDesC& aMsg)
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("Info"));


	cb->status_change(aMsg);
}

// from MSettingListener
void CSmsSnapshotImpl::SettingChanged(TInt Setting)
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("SettingChanged"));

	if (Setting==SETTING_SNAPSHOT_ON_SMS) {
		Settings().GetSettingL(SETTING_SNAPSHOT_ON_SMS, iEnabled);
		if (iEnabled) {
			if (!iSnapShot) iSnapShot=CSnapShot::NewL(AppContext());
			cb->status_change(_L("enabling sms snapshot"));
			if (iInterval>0) iTimer->Wait(iInterval);
		} else {
			delete iSnapShot; iSnapShot=0;
			iTimer->Reset();
			cb->status_change(_L("disabling sms snapshot"));
		}
	} else if (Setting==SETTING_SNAPSHOT_INTERVAL) {
		Settings().GetSettingL(SETTING_SNAPSHOT_INTERVAL, iInterval);
		iTimer->Reset();
		if (iEnabled && iInterval>0) iTimer->Wait(iInterval);
	}
}

void CSmsSnapshotImpl::expired(CBase* /*Source*/)
{
	CALLSTACKITEM_N(_CL("CSmsSnapshotImpl"), _CL("expired"));

	iToBeSend->AppendL(TSendTo(KNullDesC, _L("automatic snapshot")));

	cb->status_change(_L("taking picture"));
	iSnapShot->TakeSnapShot(DataDir(), this);

	if (iInterval>0) iTimer->Wait(iInterval);
}
