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

#include "lookup_notif.h"
#include "symbian_auto_ptr.h"
#include "blackboardclientsession.h"
#include "contextcommon.h"
#include "blackboard_cs.h"
#include <s32mem.h>
#include "bbutil.h"
#include "break.h"
#include "bbtypes.h"
#include "csd_presence.h"
#include "bbtuple.h"
#include "app_context_impl.h"
#include "csd_clienttuples.h"
#include "csd_threadrequest.h"
#include "cbbsession.h"

class CLookupHandlerImpl : public CLookupHandler, public MContextBase {
	CLookupHandlerImpl(MOutgoingLookup& aOutgoing);
	void ConstructL();
	~CLookupHandlerImpl();

	virtual void IncomingLookupL(const MBBData* aData);
	virtual void GetNextLookupL(); // will do an async callback with the next one
	virtual void AckLookupL(TUint aId);
	virtual void Ack(TUint aId);
	virtual void CancelGetNext();

	void CheckedRunL();
	void ConnectL();
	void SetFilterL();
	void WaitForNotify();
	void WriteL(const TDesC& aBuf);
	void DoCancel();

	RBBClient	iBBClient;

	HBufC8*		iSerializedData;
	TPtr8		iP;

	TFullArgs	iFullArgs;
	TInt		iAsyncErrorCount;
	MBBDataFactory*	iFactory;
	MOutgoingLookup& iOutgoing;
	MBBData		*iWaitingData; TInt iWaitingId;

	friend class CLookupHandler;
	friend class auto_ptr<CLookupHandlerImpl>;
};

_LIT(KLookupHandler, "CLookupHandler");

CLookupHandler::CLookupHandler() : CCheckedActive(EPriorityStandard, KLookupHandler) { }

CLookupHandler* CLookupHandler::NewL(MOutgoingLookup& aOutgoing)
{
	auto_ptr<CLookupHandlerImpl> ret(new (ELeave) CLookupHandlerImpl(aOutgoing));
	ret->ConstructL();
	return ret.release();
}


CLookupHandlerImpl::CLookupHandlerImpl(MOutgoingLookup& aOutgoing) : 
	iP(0, 0), iOutgoing(aOutgoing) { }

void CLookupHandlerImpl::ConstructL()
{
	iSerializedData=HBufC8::NewL(2048);

	iFactory=CBBDataFactory::NewL();

	CActiveScheduler::Add(this);
	ConnectL();
	SetFilterL();
}

CLookupHandlerImpl::~CLookupHandlerImpl()
{
	CALLSTACKITEM_N(_CL("CLookupHandlerImpl"), _CL("~CLookupHandlerImpl"));

	Cancel();
	iBBClient.Close();
	delete iSerializedData;
	delete iFactory;
}

void CLookupHandlerImpl::IncomingLookupL(const MBBData* aData)
{
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

	TBool replace=EFalse;
	TTime expires; expires.HomeTime(); expires+=TTimeIntervalDays(7);

	iBBClient.Put(KIncomingLookupTuple, KNullDesC,
		KNoComponent, *serialized, EBBPriorityNormal,
		replace, id, s, expires, ETrue, EFalse);
	User::WaitForRequest(s);
	User::LeaveIfError(s.Int());
}

void CLookupHandlerImpl::GetNextLookupL()
{
	if (iWaitingData) {
		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrNone);
		SetActive();
	} else {
		WaitForNotify();
	}
}

void CLookupHandlerImpl::Ack(TUint aId)
{
	AckLookupL(aId);
}

void CLookupHandlerImpl::CancelGetNext()
{
	Cancel();
}

void CLookupHandlerImpl::AckLookupL(TUint aId)
{
	if (aId==0) return;

	if (aId==iWaitingId) {
		delete iWaitingData; iWaitingData=0;
		iWaitingId=0;
	}

	TRequestStatus s;
	iBBClient.Delete(aId, s);
	User::WaitForRequest(s);
	if (s==KErrNotFound) return;
	User::LeaveIfError(s.Int());
}

void CLookupHandlerImpl::ConnectL()
{
	CALLSTACKITEM_N(_CL("CLookupHandlerImpl"), _CL("ConnectL"));

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

void CLookupHandlerImpl::SetFilterL()
{
	CALLSTACKITEM_N(_CL("CLookupHandlerImpl"), _CL("SetFilterL"));

	TInt errorcount=0, err=KErrNone;
	while (errorcount<5) {
		TRequestStatus s;
		iBBClient.AddNotificationL(KOutgoingLookupTuple, ETrue, EBBPriorityNormal, s);
		User::WaitForRequest(s);
		err=s.Int();
		if (err==KErrNone) {
			iBBClient.AddNotificationL(KOutgoingTuples, s);
			User::WaitForRequest(s);
			err=s.Int();
		}
		if (err==KErrNone) return;

		ConnectL();
		errorcount++;
	}
	User::Leave(err);
}

void CLookupHandlerImpl::WaitForNotify()
{
	CALLSTACKITEM_N(_CL("CLookupHandlerImpl"), _CL("WaitForNotify"));

	if (IsActive()) return;

	iSerializedData->Des().Zero();
	iP.Set(iSerializedData->Des());
	iBBClient.WaitForNotify(iFullArgs, iP, iStatus);
	SetActive();
}

void CLookupHandlerImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CLookupHandlerImpl"), _CL("DoCancel"));

	iBBClient.CancelNotify();
}


_LIT(KValue, "value");

void CLookupHandlerImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CLookupHandlerImpl"), _CL("CheckedRunL"));

	if (iWaitingData) {
		TInt err;
		if ( iFullArgs.iTupleName == KOutgoingLookupTuple ) {
			CC_TRAP(err, iOutgoing.OutgoingLookupL(iWaitingId, iWaitingData));
		} else {
			CC_TRAP(err, iOutgoing.OutgoingTuple( bb_cast<CBBTuple>(iWaitingData) ));
		}
		return;
	}
	{
		if (iStatus.Int()!=KErrNone) {
			if (iStatus.Int()==EBufferTooSmall) {
				iSerializedData->Des().Zero();
				iSerializedData=iSerializedData->ReAllocL(iSerializedData->Des().MaxLength()*2);
				iAsyncErrorCount=0;
				WaitForNotify();
				return;
			}
			if (iAsyncErrorCount>5) User::Leave(iStatus.Int());
			ConnectL();
			SetFilterL();
			WaitForNotify();
			return;
		}
	}

	MBBData* d=0;
	{
		RDesReadStream rs(*iSerializedData);
		CleanupClosePushL(rs);
		TTypeName read_type=TTypeName::IdFromStreamL(rs);
		if ( iFullArgs.iTupleName == KOutgoingLookupTuple ) {
			if (! (read_type==KPresenceType) ) {
				{
					d=iFactory->CreateBBDataL(read_type, KValue, iFactory);
					CleanupPushBBDataL(d);
				}
				{
					d->InternalizeL(rs);
				}
				CleanupStack::Pop();
			} else {
				AckLookupL(iFullArgs.iId);
				WaitForNotify();
				CleanupStack::PopAndDestroy();
				return;
			}
		} else {
			bb_auto_ptr<MBBData> data(iFactory->CreateBBDataL(read_type, KValue, iFactory));
			data->InternalizeL(rs);
			bb_auto_ptr<CBBTuple> tuple(new (ELeave) CBBTuple(GetContext()->BBDataFactory()));
			tuple->iTupleId=iFullArgs.iId;
			tuple->iTupleMeta.iModuleUid()=iFullArgs.iTupleName.iModule.iUid;
			tuple->iTupleMeta.iModuleId()=iFullArgs.iTupleName.iId;
			tuple->iTupleMeta.iSubName()=iFullArgs.iSubName;
			tuple->iData.SetOwnsValue(ETrue);
			tuple->iData.SetValue(data.release());
			tuple->iExpires()=iFullArgs.iLeaseExpires;
			d=tuple.release();
		}
		CleanupStack::PopAndDestroy();
	}

	{
		TInt err;
		bb_auto_ptr<MBBData> p(d);
		if ( iFullArgs.iTupleName == KOutgoingLookupTuple ) {
			CC_TRAP(err, iOutgoing.OutgoingLookupL(iFullArgs.iId, d));
		} else {
			const CBBTuple *t=bb_cast<CBBTuple>(d);
			CC_TRAP(err, iOutgoing.OutgoingTuple( t));
			if (err==KErrNone) {
				TTupleName tn={ t->iTupleMeta.iModuleUid(), t->iTupleMeta.iModuleId() };
				if ( tn==KThreadRequestTuple ) {
					TTime expires=GetTime(); expires+=TTimeIntervalMinutes(2);
					TBBInt reply(-1, KValue);
					BBSession()->PutL(KThreadRequestReplyTuple, t->iTupleMeta.iSubName(),
						&reply, expires, KNoComponent);
				}
			}
		}
		if (err!=KErrNone) {
			iWaitingData=p.release();
			iWaitingId=iFullArgs.iId;
		}
	}

}
