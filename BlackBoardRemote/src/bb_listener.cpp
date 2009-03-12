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
#include "bb_listener.h"

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
#include "csd_gps.h"
#include "db.h"

void Log(const TDesC& msg);
void Log(const TDesC8& msg);
void Log(const TDesC& msg, TInt i);

#ifndef __WINS__
const TInt KInitialAckInterval = 3*60;
const TInt KMaxAckInterval = 15*60;
const TInt KMaxAckWait = 5*60;
#else
const TInt KInitialAckInterval = 3*60;
const TInt KMaxAckInterval = 10*60;
const TInt KMaxAckWait = 5*60;
#endif

_LIT(KEvent, "event");

const TComponentName KListener = { { CONTEXT_UID_CONTEXTNETWORK}, 1 };
const TTupleName KListenerStop = { { CONTEXT_UID_CONTEXTNETWORK}, 2 };
const TTupleName KRemoteBluetoothTuple = { { CONTEXT_UID_CONTEXTNETWORK}, 3 };
const TTupleName KLocaErrorTuple = { { CONTEXT_UID_CONTEXT_LOG}, 1002 };

class CSeenIds : public CBase, public MContextBase, public MDBStore {
public:
	CSeenIds(RDbDatabase& aDb) : MDBStore(aDb) { }
	enum TColumn {
		EId = 1,
		ETimestamp,
		ECounter
	};
	TInt iCounter, iCount;
	void ConstructL() {
		TInt cols[]= { EDbColUint32, EDbColDateTime, EDbColInt32, -1 };
		TInt idx[]= { EId, -2, ETimestamp, ECounter, -1 };

		MDBStore::ConstructL(cols, idx, false, _L("SEEN_IDS"), ETrue);
		iCount=iTable.CountL();
	}
	static CSeenIds* NewL(RDbDatabase& aDb) {
		auto_ptr<CSeenIds> ids(new (ELeave) CSeenIds(aDb));
		ids->ConstructL();
		return ids.release();
	}
	TBool SeenIdL(TUint aId) {
		SwitchIndexL(0);
		TDbSeekKey rk(aId);
		return iTable.SeekL(rk);
	}
	void SetAsSeenL(TUint aId) {
		if (iCount>100) {
			SwitchIndexL(1);
			iTable.FirstL();
			while(iCount>90) {
				iTable.DeleteL();
				iCount--;
				iTable.NextL();
			}
		}
		iTable.InsertL();
		iTable.SetColL(EId, aId);
		iTable.SetColL(ETimestamp, GetTime());
		iCounter++;
		iTable.SetColL(ECounter, iCounter);
		PutL();
	}
};

class CBBListenerImpl : public CBBListener, public MContextBase,
	public MTimeOut {
private:
	CBBListenerImpl(CBBProtocol* aProtocol, MApp_context& aContext);
	void ConstructL();
	~CBBListenerImpl();

	void CheckedRunL();
	virtual void Acked(TUint id);
	virtual void IncomingTuple(const CBBTuple* aTuple);
	virtual void Error(TInt aError, TInt aOrigError, const TDesC& aDescr);
	virtual void ReadyToWrite(TBool aReady);
	virtual void Disconnected(TBool aByStopRequest);

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

	CBBProtocol*	iProtocol;
	RBBClient	iBBClient;

	HBufC8*		iSerializedData;
	TPtr8		iP;
	CXmlBufExternalizer	*iCurrentBuf, *iFreeBuf;
	CXmlBufExternalizer	*iWaiting[2];

	TFullArgs	iFullArgs;
	TInt		iAsyncErrorCount;
	TBool		iReadyToWrite, iPendingWrite, iPendingStop;
	MBBDataFactory*	iFactory;
	CBBTuple	*iTuple;
	CTimeOut	*iTimer, *iStopTimer;
	CList<TUint>	*iToAck;
	TInt		iUnackedCount, iSent;
	TTime		iPreviousAck;
	CDb*		iSeenDb;
	CSeenIds*	iSeenIds;

	enum TGetState {
		EIdle,
		EGettingListener,
		EGettingLoca,
		EWaitForNotify
	};
	TGetState	iGetState;
	void GetListener();
	void GetLoca();
	void GetOrWaitL();

	friend class CBBListener;
	friend class auto_ptr<CBBListenerImpl>;

};

CBBListener::CBBListener() : CCheckedActive(EPriorityNormal, _L("CBBListener"))
{
	CALLSTACKITEM_N(_CL("CBBListener"), _CL("CBBListener"));

}

EXPORT_C CBBListener* CBBListener::NewL(CBBProtocol* aProtocol, MApp_context& aContext)
{
	CALLSTACKITEM_N(_CL("CBBListener"), _CL("NewL"));

	auto_ptr<CBBListenerImpl> ret(new (ELeave) CBBListenerImpl(aProtocol, aContext));
	ret->ConstructL();
	return ret.release();
}

CBBListenerImpl::CBBListenerImpl(CBBProtocol* aProtocol, MApp_context& aContext) : 
	MContextBase(aContext),
		iProtocol(aProtocol), iP(0, 0) { }

void CBBListenerImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("ConstructL"));

	iCurrentBuf=CXmlBufExternalizer::NewL(2048);
	iFreeBuf=CXmlBufExternalizer::NewL(2048);
	iSerializedData=HBufC8::NewL(2048);

	iTimer=CTimeOut::NewL(*this);
	iTimer->Wait(KInitialAckInterval);

	iFactory=BBDataFactory();
	iTuple=new (ELeave) CBBTuple(iFactory);
	iToAck=CList<TUint>::NewL();

	iSeenDb=CDb::NewL(AppContext(), _L("REMOTE_IDS"), EFileWrite);
	iSeenIds=CSeenIds::NewL(iSeenDb->Db());

	ConnectL();
	SetFilterL();
	iGetState=EWaitForNotify;
	
	CActiveScheduler::Add(this);
}

CBBListenerImpl::~CBBListenerImpl()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("~CBBListenerImpl"));

	Cancel();
	iBBClient.Close();
	delete iCurrentBuf;
	delete iFreeBuf;
	delete iSerializedData;
	delete iTuple;
	delete iToAck;
	delete iTimer;
	delete iStopTimer;
	delete iSeenIds;
	delete iSeenDb;
}

void CBBListenerImpl::ConnectL()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("ConnectL"));

	Log(_L("CBBListener::ConnectL()"));

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

void CBBListenerImpl::SetFilterL()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("SetFilterL"));

	Log(_L("CBBListener::SetFilterL()"));

	TInt errorcount=0, err=KErrNone;
	while (errorcount<5) {
		TRequestStatus s;
		iBBClient.AddNotificationL(KListener, s);
		User::WaitForRequest(s);
		err=s.Int();
		if (err==KErrNone) {
			iBBClient.AddNotificationL(KLocaMessageStatusTuple, 
				ETrue, EBBPriorityNormal,
				s);
			User::WaitForRequest(s);
			err=s.Int();
		}
		if (err==KErrNone) {
			iBBClient.AddNotificationL(KLastKnownGpsTuple, 
				ETrue, EBBPriorityNormal,
				s);
			User::WaitForRequest(s);
			err=s.Int();
		}
		if (err==KErrNone) {
			iBBClient.AddNotificationL(KLocaErrorTuple, 
				ETrue, EBBPriorityNormal,
				s);
			User::WaitForRequest(s);
			err=s.Int();
		}

		if (err==KErrNone) return;
		ConnectL();
		errorcount++;
	}
	User::Leave(err);
}

void CBBListenerImpl::WaitForNotify()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("WaitForNotify"));

	Log(_L("CBBListener::WaitForNotify()"));

	if (IsActive()) return;

	iGetState=EWaitForNotify;
	iSerializedData->Des().Zero();
	iP.Set(iSerializedData->Des());
	iBBClient.WaitForNotify(iFullArgs, iP, iStatus);
	SetActive();
}

void CBBListenerImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("DoCancel"));

	iBBClient.CancelNotify();
}

void CBBListenerImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("CheckedRunL"));

#ifdef __WINS__
	//User::Leave(KErrGeneral);
#endif

	Log(_L("CheckedRunL()"));

	{
		if (iStatus.Int()!=KErrNone && iStatus.Int()!=EDeleteNotification) {
			if (iStatus.Int()==KClientBufferTooSmall) {
				iSerializedData->Des().Zero();
				iSerializedData=iSerializedData->ReAllocL(iSerializedData->Des().MaxLength()*2);
				iAsyncErrorCount=0;
				GetOrWaitL();
				return;
			} else if (iStatus.Int()==KErrNotFound) {
				if (iGetState==EWaitForNotify) {
					User::Leave(KErrNotFound);
				} else if (iGetState==EGettingListener) {
					GetLoca();
				} else if (iGetState==EGettingLoca) {
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

	if (iFullArgs.iTupleName==KListenerStop) {
		TRequestStatus s;
		iBBClient.Delete(iFullArgs.iId, s);
		User::WaitForRequest(s);
		if (iUnackedCount>0 || iSent==0) {
			iPendingStop=ETrue;
		} else {
			if (!iStopTimer) iStopTimer=CTimeOut::NewL(*this);
			iStopTimer->Wait(10);
		}
		WaitForNotify();
		return;
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
		if ( iFullArgs.iTupleName == KLastKnownGpsTuple ) {
			iTuple->iTupleId()=0;
		} else {
			iTuple->iTupleId()=iFullArgs.iId;
		}
		iTuple->iExpires()=iFullArgs.iLeaseExpires;
		
		iFreeBuf->Zero();
		iTuple->IntoXmlL(iFreeBuf);
		iFreeBuf->Characters(_L("\n"));
	}

	iUnackedCount++;
	if (iReadyToWrite) {
		iWaiting[0]=iFreeBuf;
		WriteL();
		GetOrWaitL();
	} else {
		iWaiting[1]=iFreeBuf;
		iPendingWrite=ETrue;
	}
}

void CBBListenerImpl::GetOrWaitL()
{
	if (iGetState==EWaitForNotify) {
		WaitForNotify();
	} else if (iGetState==EGettingListener) {
		GetListener();
	} else if (iGetState==EIdle || iGetState==EGettingLoca) {
		GetLoca();
	}
}

void CBBListenerImpl::GetListener()
{
	if (IsActive()) return;

	iGetState=EGettingListener;
	iSerializedData->Des().Zero();
	iP.Set(iSerializedData->Des());
	iBBClient.Get(KListener, iFullArgs, iP, iStatus);
	SetActive();

}

void CBBListenerImpl::GetLoca()
{
	if (IsActive()) return;

	iGetState=EGettingLoca;
	iSerializedData->Des().Zero();
	iP.Set(iSerializedData->Des());
	iBBClient.Get(KLocaMessageStatusTuple, KNullDesC, iFullArgs, iP, iStatus);
	SetActive();
}

void CBBListenerImpl::WriteAckL()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("WriteAckL"));
	TUint ack=iToAck->Top();
	TBuf<100> xml=_L("<ack><id>");
	xml.AppendNum(ack);
	xml.Append(_L("</id></ack>\n"));
	iProtocol->WriteL(xml);
	iToAck->Pop();
}

void CBBListenerImpl::WriteL()
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("WriteL"));

	Log(_L("WriteL"));

	iReadyToWrite=EFalse;
	if (iFreeBuf==iWaiting[0]) {
		iFreeBuf=iCurrentBuf;
		iCurrentBuf=iWaiting[0];
	}
	CC_TRAPD(err, iProtocol->WriteL( iWaiting[0]->Buf() ));
	if (err==KErrNotReady) iPendingWrite=ETrue;
	else User::LeaveIfError(err);
	iSent++;

	iTimer->WaitMax(KMaxAckWait);
}

_LIT(KLastAck, "last_ack");

void CBBListenerImpl::Acked(TUint id)
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("Acked"));

	iTimer->Wait(KMaxAckInterval);
	TRequestStatus s;
	if (id!=0) {
		iBBClient.Delete(id, s);
		User::WaitForRequest(s);
#ifdef __WINS__
		TBuf<100> msg=_L("deleted ");
		msg.AppendNum(id); msg.Append(_L(": "));
		msg.AppendNum(s.Int());
		RDebug::Print(msg);
#endif
	}
	if (iPreviousAck + TTimeIntervalMinutes(1) < GetTime()) {
		TBBTime t(KLastAck);
		iPreviousAck=GetTime();
		t()=iPreviousAck;
		TTime expires=Time::MaxTTime();
		CC_TRAPD(err, PutTupleL(KStatusTuple, KLastAck, &t, expires));
	}
	iUnackedCount--;
	if (iPendingStop && iUnackedCount==0) {
		if (!iStopTimer) iStopTimer=CTimeOut::NewL(*this);
		iStopTimer->Wait(10);
	}
}

const TTupleName KRemoteLocaLogicTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 37 };

void CBBListenerImpl::PutTupleL(const TTupleName& aTuple,
	const TDesC& aSubName, const MBBData* aData, const TTime& aExpires)
{
	if (!aData) {
		TRequestStatus s;
		iBBClient.Delete(aTuple, aSubName, s);
		User::WaitForRequest(s);
		if (s.Int()==KErrNone || s.Int()==KErrNotFound) return;
		User::Leave(s.Int());
	}

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
			serialized.reset( HBufC8::NewL(
				serialized->Des().MaxLength()*2) );
		}
	}
	User::LeaveIfError(err);

	TUint id;
	TRequestStatus s;

	TBool replace=ETrue;
	if (aTuple==KRemoteLocaLogicTuple) replace=EFalse;
	if (aTuple==KRemoteBluetoothTuple) replace=EFalse;

	iBBClient.Put(aTuple, aSubName,
		KNoComponent, *serialized, EBBPriorityNormal,
		replace, id, s, aExpires, ETrue, EFalse);
	User::WaitForRequest(s);
	User::LeaveIfError(s.Int());
}


void CBBListenerImpl::IncomingTuple(const CBBTuple* aTuple)
{
	iTimer->Wait(KMaxAckInterval);

	TTupleName tn= { aTuple->iTupleMeta.iModuleUid(), aTuple->iTupleMeta.iModuleId() };

	auto_ptr<HBufC8> serialized(HBufC8::NewL(1024));

	if (! iSeenIds->SeenIdL(aTuple->iTupleId()) ) {
		PutTupleL(tn, aTuple->iTupleMeta.iSubName(),
			aTuple->iData(), aTuple->iExpires());
		iSeenIds->SetAsSeenL(aTuple->iTupleId());
	}

	iToAck->AppendL(aTuple->iTupleId());
}

void CBBListenerImpl::Error(TInt /*aError*/, TInt /*aOrigError*/, const TDesC& /*aDescr*/)
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("Error"));

	iReadyToWrite=EFalse;
}

void CBBListenerImpl::ReadyToWrite(TBool aReady)
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("ReadyToWrite"));

	Log(_L("ReadyToWrite"));

	if (aReady) {
		if (iToAck->iCount > 0) {
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

void CBBListenerImpl::Disconnected(TBool)
{
	CALLSTACKITEM_N(_CL("CBBListenerImpl"), _CL("Disconnected"));

	iReadyToWrite=EFalse;
	delete iStopTimer; iStopTimer=0;
}

void CBBListenerImpl::expired(CBase* aSource)
{
	if (aSource==iTimer) {
		User::Leave(KContextErrTimeoutInBBProtocol);
	} else {
		iProtocol->Disconnect(ETrue, ETrue);
	}
}
