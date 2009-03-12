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
#include "bbl_listener.h"

#include <blackboardclientsession.h>
#include "bbdata.h"
#include "symbian_auto_ptr.h"
#include "context_uids.h"
#include "bbtypes.h"
#include "csd_event.h"
#include "bbxml.h"
#include "csd_loca.h"
#include "timeout.h"
#include <s32mem.h>
#include <e32svr.h>

void Log(const TDesC& msg);
void Log(const TDesC8& msg);
void Log(const TDesC& msg, TInt i);

#ifndef __WINS__
const TInt KMaxAckInterval = 15*60;
const TInt KMaxAckWait = 5*60;
#else
const TInt KMaxAckInterval = 5*60;
const TInt KMaxAckWait = 2*60;
#endif

_LIT(KEvent, "event");
const TComponentName KListener = { { CONTEXT_UID_CONTEXTNETWORK}, 1 };

class CBBLocalListenerImpl : public CBBLocalListener, public MContextBase,
	public MTimeOut {
private:
	CBBLocalListenerImpl(CBBLocalProtocol* aProtocol, MApp_context& aContext);
	void ConstructL();
	~CBBLocalListenerImpl();

	void CheckedRunL();
	virtual void Acked(TUint id);
	virtual void IncomingTuple(const CBBTuple* aTuple);
	virtual void Error(TInt aError, TInt aOrigError, const TDesC& aDescr);
	virtual void ReadyToWrite(TBool aReady);
	virtual void Disconnected();

	void ConnectL();
	void SetFilterL();
	void WaitForNotify();
	void WriteL();
	void DoCancel();
	void WriteAckL();
	void PutTupleL(const TTupleName& aTuple,
		const TDesC& aSubName, const MBBData* aData,
		const TTime& aExpires );

	void expired(CBase* aSource);

	CBBLocalProtocol*	iProtocol;
	RBBClient	iBBClient;

	HBufC8*		iSerializedData;
	TPtr8		iP;
	CXmlBufExternalizer	*iCurrentBuf, *iFreeBuf;
	CXmlBufExternalizer	*iWaiting[2];

	TFullArgs	iFullArgs;
	TInt		iAsyncErrorCount;
	TBool		iReadyToWrite, iPendingWrite;
	MBBDataFactory*	iFactory;
	CBBTuple	*iTuple;
	CTimeOut	*iTimer;
	CList<TUint>	*iToAck;
	TTime		iPreviousAck;

	enum TGetState {
		EIdle,
		EGettingEvent,
		EWaitForNotify
	};
	TGetState	iGetState;
	void GetEvent();
	void GetOrWaitL();

	friend class CBBLocalListener;
	friend class auto_ptr<CBBLocalListenerImpl>;

};

CBBLocalListener::CBBLocalListener() : CCheckedActive(EPriorityNormal, _L("CBBLocalListener"))
{
	CALLSTACKITEM_N(_CL("CBBLocalListener"), _CL("CBBLocalListener"));

}

EXPORT_C CBBLocalListener* CBBLocalListener::NewL(CBBLocalProtocol* aProtocol, MApp_context& aContext)
{
	CALLSTACKITEM_N(_CL("CBBLocalListener"), _CL("NewL"));

	auto_ptr<CBBLocalListenerImpl> ret(new (ELeave) CBBLocalListenerImpl(aProtocol, aContext));
	ret->ConstructL();
	return ret.release();
}

CBBLocalListenerImpl::CBBLocalListenerImpl(CBBLocalProtocol* aProtocol, MApp_context& aContext) : 
	MContextBase(aContext),
		iProtocol(aProtocol), iP(0, 0) { }

void CBBLocalListenerImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("ConstructL"));

	iCurrentBuf=CXmlBufExternalizer::NewL(2048);
	iFreeBuf=CXmlBufExternalizer::NewL(2048);
	iSerializedData=HBufC8::NewL(2048);

#if DUPLEX
	iTimer=CTimeOut::NewL(*this);
	iTimer->Wait(KMaxAckInterval);
#endif

	iFactory=BBDataFactory();
	iTuple=new (ELeave) CBBTuple(iFactory);
#if DUPLEX
	iToAck=CList<TUint>::NewL();
#endif

	ConnectL();
	SetFilterL();
	iGetState=EWaitForNotify;
	
	CActiveScheduler::Add(this);
}

CBBLocalListenerImpl::~CBBLocalListenerImpl()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("~CBBLocalListenerImpl"));

	Cancel();
	iBBClient.Close();
	delete iCurrentBuf;
	delete iFreeBuf;
	delete iSerializedData;
	delete iTuple;
	delete iToAck;
	delete iTimer;
}

void CBBLocalListenerImpl::ConnectL()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("ConnectL"));

	Log(_L("CBBLocalListener::ConnectL()"));

	Cancel();
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

void CBBLocalListenerImpl::SetFilterL()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("SetFilterL"));

	Log(_L("CBBLocalListener::SetFilterL()"));

	TInt errorcount=0, err=KErrNone;
	while (errorcount<5) {
		TRequestStatus s;
		iBBClient.AddNotificationL(KAnySensorEvent, 
			ETrue, EBBPriorityNormal,
			s);
		User::WaitForRequest(s);
		err=s.Int();
		if (err==KErrNone) return;

		ConnectL();
		errorcount++;
	}
	User::Leave(err);
}

void CBBLocalListenerImpl::WaitForNotify()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("WaitForNotify"));

	Log(_L("CBBLocalListener::WaitForNotify()"));

	if (IsActive()) return;

	iGetState=EWaitForNotify;
	iSerializedData->Des().Zero();
	iP.Set(iSerializedData->Des());
	iBBClient.WaitForNotify(iFullArgs, iP, iStatus);
	SetActive();
}

void CBBLocalListenerImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("DoCancel"));

	iBBClient.CancelNotify();
}

void CBBLocalListenerImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("CheckedRunL"));

	Log(_L("CheckedRunL()"));

	{
		if (iStatus!=KErrNone && iStatus!=EDeleteNotification) {
			if (iStatus.Int()==KClientBufferTooSmall) {
				iSerializedData->Des().Zero();
				iSerializedData=iSerializedData->ReAllocL(iSerializedData->Des().MaxLength()*2);
				iAsyncErrorCount=0;
				GetOrWaitL();
				return;
			} else if (iStatus.Int()==KErrNotFound) {
				if (iGetState==EWaitForNotify) {
					User::Leave(KErrNotFound);
				} else if (iGetState==EGettingEvent) {
					SetFilterL();
					WaitForNotify();
				}
				return;
			}
			if (iAsyncErrorCount>5) User::Leave(iStatus.Int());
			++iAsyncErrorCount;
			ConnectL();
			SetFilterL();
			WaitForNotify();
			return;
		}
	}

	MBBData* d=0;
	if (iStatus!=EDeleteNotification) {
		RDesReadStream rs(*iSerializedData);
		CleanupClosePushL(rs);	
		TTypeName read_type=TTypeName::IdFromStreamL(rs);
		{
			d=iFactory->CreateBBDataL(read_type, KEvent, iFactory);
			CleanupPushBBDataL(d);
		}
		{
			d->InternalizeL(rs);
		}
		CleanupStack::Pop();
		CleanupStack::PopAndDestroy();
	}

	{
		iTuple->iData.SetValue(d);
		iTuple->iTupleMeta.iModuleUid()=iFullArgs.iTupleName.iModule.iUid;
		iTuple->iTupleMeta.iModuleId()=iFullArgs.iTupleName.iId;
		iTuple->iTupleMeta.iSubName=iFullArgs.iSubName;
		iTuple->iTupleId()=iFullArgs.iId;
		iTuple->iExpires()=iFullArgs.iLeaseExpires;
		
		iFreeBuf->Zero();
		iTuple->IntoXmlL(iFreeBuf);
		iFreeBuf->Characters(_L("\n"));
	}

	
	if (iReadyToWrite) {
		iWaiting[0]=iFreeBuf;
		WriteL();
		GetOrWaitL();
	} else {
		iWaiting[1]=iFreeBuf;
		iPendingWrite=ETrue;
	}
}

void CBBLocalListenerImpl::GetOrWaitL()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("GetOrWaitL"));
	if (iGetState==EWaitForNotify) {
		WaitForNotify();
	} else if (iGetState==EIdle || iGetState==EGettingEvent) {
		GetEvent();
	}
}

void CBBLocalListenerImpl::GetEvent()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("GetEvent"));
	if (IsActive()) return;

	iGetState=EGettingEvent;
	iSerializedData->Des().Zero();
	iP.Set(iSerializedData->Des());
	iBBClient.Get(KLocaMessageStatusTuple, KNullDesC, iFullArgs, iP, iStatus);
	SetActive();
}

void CBBLocalListenerImpl::WriteAckL()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("WriteAckL"));
	TUint ack=iToAck->Top();
	TBuf<100> xml=_L("<ack><id>");
	xml.AppendNum(ack);
	xml.Append(_L("</id></ack>\n"));
	iProtocol->WriteL(xml);
	iToAck->Pop();
}

void CBBLocalListenerImpl::WriteL()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("WriteL"));

	Log(_L("WriteL"));

	iReadyToWrite=EFalse;
	if (iFreeBuf==iWaiting[0]) {
		iFreeBuf=iCurrentBuf;
		iCurrentBuf=iWaiting[0];
	}
	CC_TRAPD(err, iProtocol->WriteL( iWaiting[0]->Buf() ));
	if (err==KErrNotReady) iPendingWrite=ETrue;
	else User::LeaveIfError(err);

#if DUPLEX
	iTimer->WaitMax(KMaxAckWait);
#endif
}

_LIT(KLastAck, "last_ack");

void CBBLocalListenerImpl::Acked(TUint id)
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("Acked"));

#if DUPLEX
	iTimer->Wait(KMaxAckInterval);
#endif
	TRequestStatus s;
	iBBClient.Delete(id, s);
	User::WaitForRequest(s);
#ifdef __WINS__
	TBuf<100> msg=_L("deleted ");
	msg.AppendNum(id); msg.Append(_L(": "));
	msg.AppendNum(s.Int());
	RDebug::Print(msg);
#endif
	if (iPreviousAck + TTimeIntervalMinutes(1) < GetTime()) {
		TBBTime t(KLastAck);
		iPreviousAck=GetTime();
		t()=iPreviousAck;
		TTime expires=Time::MaxTTime();
		CC_TRAPD(err, PutTupleL(KStatusTuple, KLastAck, &t, expires));
	}
}

const TTupleName KRemoteLocaLogicTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 37 };

void CBBLocalListenerImpl::PutTupleL(const TTupleName& aTuple,
	const TDesC& aSubName, const MBBData* aData, const TTime& aExpires)
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("PutTupleL"));
	if (!aData) return;

	auto_ptr<HBufC8> serialized(HBufC8::NewL(1024));
	TInt err=KErrOverflow;
	while (err==KErrOverflow) {
		TPtr8 bufp(serialized->Des());
		RDesWriteStream ws(bufp);

		aData->Type().ExternalizeL(ws);
		CC_TRAP(err, aData->ExternalizeL(ws));
		if (err==KErrNone) {
			ws.CommitL();
		} else if (err==KErrOverflow) {
			serialized->Des().Zero();
			serialized.reset( serialized->ReAllocL(
				serialized->Des().MaxLength()*2) );
		}
	}
	User::LeaveIfError(err);

	TUint id;
	TRequestStatus s;

	TBool replace=ETrue;
	if (aTuple==KRemoteLocaLogicTuple) replace=EFalse;

	iBBClient.Put(aTuple, aSubName,
		KNoComponent, *serialized, EBBPriorityNormal,
		replace, id, s, aExpires, ETrue, EFalse);
	User::WaitForRequest(s);
	User::LeaveIfError(s.Int());
}


void CBBLocalListenerImpl::IncomingTuple(const CBBTuple* aTuple)
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("IncomingTuple"));
#if DUPLEX
	iTimer->Wait(KMaxAckInterval);
#endif

	TTupleName tn= { aTuple->iTupleMeta.iModuleUid(), aTuple->iTupleMeta.iModuleId() };

	auto_ptr<HBufC8> serialized(HBufC8::NewL(1024));
	PutTupleL(tn, aTuple->iTupleMeta.iSubName(),
		aTuple->iData(), aTuple->iExpires());

	iToAck->AppendL(aTuple->iTupleId());
}

void CBBLocalListenerImpl::Error(TInt /*aError*/, TInt /*aOrigError*/, const TDesC& /*aDescr*/)
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("Error"));

	iReadyToWrite=EFalse;
}

void CBBLocalListenerImpl::ReadyToWrite(TBool aReady)
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("ReadyToWrite"));

	Log(_L("ReadyToWrite"));

	if (aReady) {
		if (iToAck && iToAck->iCount > 0) {
			WriteAckL();
			return;
		}
		iWaiting[0]=iWaiting[1]; iWaiting[1]=0;
		if (iWaiting[0]) {
			WriteL();
		} else {
			iReadyToWrite=ETrue;
		}
		GetOrWaitL();
	} else {
		iReadyToWrite=EFalse;
	}
}

void CBBLocalListenerImpl::Disconnected()
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("Disconnected"));

	iReadyToWrite=EFalse;
	delete this;
}

void CBBLocalListenerImpl::expired(CBase* aSource)
{
	CALLSTACKITEM_N(_CL("CBBLocalListenerImpl"), _CL("expired"));
	User::Leave(KContextErrTimeoutInBBProtocol);
}
