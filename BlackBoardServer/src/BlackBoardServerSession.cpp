/* 
    Copyright (C) 2004  Mika Raento - Renaud Petit

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


    email: mraento@cs.helsinki.fi - petit@cs.helsinki.fi 
*/


//CBlackBoardServerSession.cpp

#include "break.h"
#include "BlackBoardServerSession.h"
#include "BlackBoard_cs.h"
#include <e32svr.h>
#include <flogger.h>
#include "symbian_auto_ptr.h"
#include "bberrors.h"
#include "app_context_impl.h"
#include "callstack.h"
#include "bbtypes.h"

bool ClientAlive(TInt ThreadId)
{
	CALLSTACKITEM_N(_CL("CleanupStack"), _CL("PopAndDestroy"));


	if (ThreadId==0) return false;

	RThread c; bool ret = false;
	if (c.Open(ThreadId) != KErrNone) return false;
	if (c.ExitType() == EExitPending) ret=true;
	c.Close();
	return ret;
}

class CIdleCallBack : public CActive {
private:
	TBool iEnabled;
	MBack& iBack;
public:

	static CIdleCallBack* NewL(MBack& aBack) {
		auto_ptr<CIdleCallBack> ret(new (ELeave) CIdleCallBack(aBack));
		ret->ConstructL();
		return ret.release();
	}

	/*
	 * callbacks have EPriorityLow, so that they fire before 
	 * the timer that deletes expired tuples, in case
	 * the tuple expires immediately
	 */
	CIdleCallBack(MBack& aBack) : CActive(CActive::EPriorityLow), iEnabled(EFalse), iBack(aBack) { }
	void ConstructL() {
		CActiveScheduler::Add(this);
	}
	void RunL() { if(iEnabled) iBack.Back(); }
	void DoCancel() { iEnabled=EFalse; }
	void Reset() { iEnabled=EFalse; }
	void Trigger() { 
		iEnabled=ETrue;
		if (!IsActive()) { 
			TRequestStatus* s=&iStatus;
			User::RequestComplete(s, KErrNone);
			SetActive();
		}
	}
	~CIdleCallBack() { Cancel(); }
};

#ifndef __IPCV2__
CBlackBoardServerSession::CBlackBoardServerSession(RThread& aClient, CBlackBoardServer& aServer) : CSession(aClient), iServer(aServer) { }
#else
CBlackBoardServerSession::CBlackBoardServerSession(CBlackBoardServer& aServer) : iServer(aServer) { }
#endif


void CBlackBoardServerSession::Exiting() {
	iExiting=ETrue;
}

CBlackBoardServerSession::~CBlackBoardServerSession()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("~CBlackBoardServerSession"));

	delete iIdle;

	if (! iExiting) {
		iServer.DeleteAllNotificationsL(this);
	}

	/*
	TInt i;
	for (i=0; i<KPriorityCount; i++) {
		delete iNotifications[i];
	}
	*/
	delete iNotifications;
	delete iDeleteTupleNames;
	delete iDeleteSubNames;

	if (! iExiting) {
    	iServer.DecrementSessions();
    }
}

void CBlackBoardServerSession::CompleteMessage(TMessageIdxs aWhich, TInt Code)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("CompleteMessage"));


	if (ClientAlive(iMessageThreadId[aWhich]) && iMessage[aWhich]!=MESSAGE_CLASS()) {
		iMessage[aWhich].Complete(Code);
	}
	if (aWhich==ENotifyMsgIndex) iWaitingForNotify=EFalse;

	SetMessage(aWhich, MESSAGE_CLASS());
}

void CBlackBoardServerSession::ServiceL(const MESSAGE_CLASS& aMessage)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ServiceL"));

	TMessageIdxs msgidx;

	// support for graceful shutdown on out-of-disk-space
	if ( aMessage.Function()!=ECancelOther && aMessage.Function()!=ECancelNotify &&
		aMessage.Function()!=ETerminateBlackBoardServer) {

		TInt err=iServer.GetPermanentError();
		if (err!=KErrNone) {
			aMessage.Complete(err);
			return;
		}
	}

	msgidx=EOtherMsgIndex;
	if (aMessage.Function()!=ECancelOther && aMessage.Function()!=ECancelNotify) {
		CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ServiceL_notcancel"));
		// allow other calls while waiting for notify
		if (aMessage.Function()==ENotifyOnChange) {
			msgidx=ENotifyMsgIndex;	
		}
		SetMessage(msgidx, aMessage);
		ReadFlags(msgidx);
	}

	TInt err;
	
	switch (aMessage.Function())
	{
		case EGetByTuple:
			CC_TRAPIGNORE(err, KErrNotFound, GetByTupleL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EGetByComponent:
			CC_TRAPIGNORE(err, KErrNotFound, GetByComponentL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EPut:
			CC_TRAP(err, PutL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EDeleteByTuple:
			CC_TRAPIGNORE(err, KErrNotFound, DeleteByTupleL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EDeleteByComponent:
			CC_TRAPIGNORE(err, KErrNotFound, DeleteByComponentL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EDeleteById:
			CC_TRAP(err, DeleteByIdL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EAddNotifyByTupleFilter:
			CC_TRAP(err, AddTupleFilterL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EDeleteNotifyByTupleFilter:
			CC_TRAP(err, DeleteTupleFilterL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EAddNotifyByComponentFilter:
			CC_TRAP(err, AddComponentFilterL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case EDeleteNotifyByComponentFilter:
			CC_TRAP(err, DeleteComponentFilterL());
			if (err!=KErrNone) CompleteMessage(msgidx, err);
			break;
		case ENotifyOnChange:
		  TBool sent = EFalse;
		  CC_TRAPIGNORE(err, KClientBufferTooSmall, sent = SendWaitingL());
		  if (err != KErrNone) {
		    CompleteMessage(msgidx, err);
		  } else {
			  if (!sent) iWaitingForNotify=ETrue;
			}
			break;
		case ETerminateBlackBoardServer:
			TerminateServer();
			break;
		case ECancelOther:
			// hmm. Since all of these messages get
			// Complete()d when they come in, there isn't
			// a pending message, which makes this is a NOP
			// it'll do something, tho, if we change
			// some message to be really async
			CompleteMessage(EOtherMsgIndex, KErrCancel);
			// but this should be correct even if the
			// above does nothing
			aMessage.Complete(KErrNone);
			break;
		case ECancelNotify:
			iWaitingForNotify=EFalse;
			CompleteMessage(ENotifyMsgIndex, KErrCancel);
			aMessage.Complete(KErrNone);
			break;
		default :
            PanicClient(aMessage, EBBBadRequest);
	}
	if (err==KErrCorrupt) User::Leave(err);

}

void CBlackBoardServerSession::PanicClient(const MESSAGE_CLASS& aMessage, TInt aPanic) const
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("PanicClient"));

#ifndef __IPCV2__
	Panic(KBlackBoardServer, aPanic) ; // Note: this panics the client thread, not server
#else
	aMessage.Panic(KBlackBoardServer, aPanic) ; // Note: this panics the client thread, not server
#endif
}

#ifndef __IPCV2__
CBlackBoardServerSession* CBlackBoardServerSession::NewL(RThread& aClient, CBlackBoardServer& aServer)
#else
CBlackBoardServerSession* CBlackBoardServerSession::NewL(CBlackBoardServer& aServer)
#endif
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("NewL"));

#ifndef __IPCV2__
	CBlackBoardServerSession* self = new (ELeave) CBlackBoardServerSession(aClient, aServer);
#else
	CBlackBoardServerSession* self = new (ELeave) CBlackBoardServerSession(aServer);
#endif
    CleanupStack::PushL(self) ;
    self->ConstructL();
    CleanupStack::Pop(self) ;
    return self ;
}


void CBlackBoardServerSession::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ConstructL"));

	iServer.IncrementSessions();

	iIdle=CIdleCallBack::NewL(*this);

	/*
	TInt i;
	for (i=0; i<KPriorityCount; i++) {
		iNotifications[i]=CList<TUint>::NewL();
	}
	*/
	iNotifications=new (ELeave) CCirBuf<TUint>;
#ifndef __WINS__
	iNotificationsMaxLength=128;
#else
	// to see that there are no bugs in the realloc
	iNotificationsMaxLength=1;
#endif
	iNotifications->SetLengthL(iNotificationsMaxLength);

	iDeleteTupleNames=new (ELeave) CArrayFixSeg<TTupleName>(4);
	iDeleteSubNames=new (ELeave) CDesCArraySeg(4);
}

void CBlackBoardServerSession::AppendNotificationL(TUint aId)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("AppendNotification"));

	if (! iNotifications->Add(&aId) ) {
		auto_ptr< CCirBuf<TUint> > larger(new (ELeave) CCirBuf<TUint>);
		iNotificationsMaxLength*=2;
		larger->SetLengthL(iNotificationsMaxLength);
		TUint move;
		while (iNotifications->Remove(&move)) {
			larger->Add(&move);
		}
		larger->Add(&aId);
		delete iNotifications; iNotifications=larger.release();
	}
}

TUint CBlackBoardServerSession::PopNotification()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("PopNotification"));
	TUint id;
	if (! iNotifications->Remove(&id) ) return 0;
	return id;
}

//------------------------------------------------------------------------

void CBlackBoardServerSession::SetMessage(TMessageIdxs aWhich, const MESSAGE_CLASS& aMessage) 
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("SetMessage"));


	if (aMessage==MESSAGE_CLASS()) {
		iMessageThreadId[aWhich]=0;
	} else {
#ifdef __IPCV2__
		RThread client;
		aMessage.Client(client);
		iMessageThreadId[aWhich]=client.Id();
		client.Close();
#else
		iMessageThreadId[aWhich]=aMessage.Client().Id();
#endif
	}
	iMessage[aWhich]=aMessage;
}

void CBlackBoardServerSession::ReadTupleAndSubL(TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ReadTupleAndSubL"));

	if ( iFlags & KArgFlagNoSubName ) {
		iFullArgs.iSubName.Zero();
		TPckg<TTupleName> p(iFullArgs.iTupleName);
#ifdef __IPCV2__
		iMessage[aWhich].ReadL( 1, p );
#else
		iMessage[aWhich].ReadL( iMessage[aWhich].Ptr1(), p );
#endif
	} else {
		TTupleArgs t;
		TPckg<TTupleArgs> p(t);
#ifdef __IPCV2__
		iMessage[aWhich].ReadL( 1, p );
#else
		iMessage[aWhich].ReadL( iMessage[aWhich].Ptr1(), p );
#endif
		iFullArgs.iTupleName=t.iTupleName;
		iFullArgs.iSubName=t.iSubName;
	}
}

void CBlackBoardServerSession::ReadComponentL(TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ReadComponentL"));

	iFullArgs.iComponentName.iModule.iUid=iMessage[aWhich].Int1();
	iFullArgs.iComponentName.iId=iMessage[aWhich].Int2();
}


void CBlackBoardServerSession::ReadId(TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ReadId"));

	iFullArgs.iId=(TUint)iMessage[aWhich].Ptr1();
}

void CBlackBoardServerSession::GetByTupleL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("GetByTupleL"));

	ReadTupleAndSubL(EOtherMsgIndex);
	RADbColReadStream rs;
	iServer.GetL(iFullArgs.iTupleName, iFullArgs.iSubName, iFullArgs.iId, iFullArgs.iTupleType, 
		iFullArgs.iComponentName, rs, iSize, iFullArgs.iLeaseExpires);
	WriteDataL(rs, EOtherMsgIndex);
}

void CBlackBoardServerSession::ReadFlags(TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ReadFlags"));

	iFlags=(TUint)iMessage[aWhich].Ptr0();
}

void CBlackBoardServerSession::GetByComponentL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("GetByComponentL"));

	ReadComponentL(EOtherMsgIndex);
	RADbColReadStream rs;
	iServer.GetL(iFullArgs.iComponentName, iFullArgs.iId, iFullArgs.iTupleType, 
		iFullArgs.iTupleName, iFullArgs.iSubName, rs, iSize, iFullArgs.iLeaseExpires);
	WriteDataL(rs, EOtherMsgIndex);
}

void CBlackBoardServerSession::ReadFullArgsL(TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ReadFullArgsL"));

	TPckg<TFullArgs> p(iFullArgs);
	p.SetLength(0);
#ifdef __IPCV2__
	TRAPD(err, iMessage[aWhich].ReadL( 1, p ));
#else
	TRAPD(err, iMessage[aWhich].ReadL( iMessage[aWhich].Ptr1(), p ));
#endif
	User::LeaveIfError(err);
}

HBufC8* CBlackBoardServerSession::ReadDataL(TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ReadDataL"));

	TInt len;
#ifdef __IPCV2__
	len=iMessage[aWhich].GetDesLength(3);
#else
	len=iMessage[aWhich].Client().GetDesLength(iMessage[aWhich].Ptr3());
#endif
	if (len<0) User::Leave(len);
	auto_ptr<HBufC8> ret(HBufC8::NewL(len));
	TPtr8 p=ret->Des();
#ifdef __IPCV2__
	iMessage[aWhich].ReadL( 3, p );
#else
	iMessage[aWhich].ReadL( iMessage[aWhich].Ptr3(), p );
#endif
	return ret.release();
}

void CBlackBoardServerSession::PutL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("PutL"));

	ReadFullArgsL(EOtherMsgIndex);
	TBool Replace=EFalse;
	TBool KeepExisting=EFalse;
	if (iFlags & KArgFlagReplace) Replace=ETrue;
	if (iFlags & KArgFlagKeepExisting) KeepExisting=ETrue;

	auto_ptr<HBufC8> b(ReadDataL(EOtherMsgIndex));

	// FIXME: return warning if PutL returns an error code
	// TInt ret=
	TBool persist=ETrue;
	if (iFlags & KArgFlagDontPersist) persist=EFalse;

	if (iFlags & KArgFlagDoNotNotifySender)	iInPut=ETrue;

	TRAPD(err, iServer.PutL( iFullArgs.iTupleName, iFullArgs.iSubName, iFullArgs.iComponentName,
		b, iFullArgs.iPriority, Replace, iFullArgs.iId, iFullArgs.iLeaseExpires,
		persist, iFullArgs.iTupleType, KeepExisting ));
	iInPut=EFalse;
	User::LeaveIfError(err);

	if (iMessage[EOtherMsgIndex].Ptr2()) {
		TPckg<TUint> p(iFullArgs.iId);
#ifdef __IPCV2__
		iMessage[EOtherMsgIndex].WriteL( 2, p );
#else
		iMessage[EOtherMsgIndex].WriteL( iMessage[EOtherMsgIndex].Ptr2(), p );
#endif
	}
	CompleteMessage(EOtherMsgIndex, KErrNone);
}

void CBlackBoardServerSession::DeleteByTupleL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("DeleteByTupleL"));

	iInDelete=ETrue;
	ReadTupleAndSubL(EOtherMsgIndex);
	iServer.DeleteL(iFullArgs.iTupleName, iFullArgs.iSubName);
	CompleteMessage(EOtherMsgIndex, KErrNone);
}

void CBlackBoardServerSession::DeleteByComponentL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("DeleteByTupleL"));

	iInDelete=ETrue;
	ReadComponentL(EOtherMsgIndex);
	iServer.DeleteL(iFullArgs.iComponentName);
	CompleteMessage(EOtherMsgIndex, KErrNone);
}

void CBlackBoardServerSession::DeleteByIdL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("DeleteByIdL"));

	iInDelete=ETrue;
	ReadId(EOtherMsgIndex);
	iServer.DeleteL(iFullArgs.iId);
	CompleteMessage(EOtherMsgIndex, KErrNone);
}

void CBlackBoardServerSession::ReadPriorityL(TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ReadPriorityL"));

	iFullArgs.iPriority=(TBBPriority)iMessage[aWhich].Int3();
}

void CBlackBoardServerSession::ReadTupleL(TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("ReadTupleL"));

	iFullArgs.iTupleName.iModule.iUid=iMessage[aWhich].Int1();
	iFullArgs.iTupleName.iId=iMessage[aWhich].Int2();
}

void CBlackBoardServerSession::AddTupleFilterL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("AddTupleFilterL"));

	ReadTupleL(EOtherMsgIndex);
	ReadPriorityL(EOtherMsgIndex);
	TBool GetExisting=EFalse;
	if (iFlags & KArgFlagGetExisting) GetExisting=ETrue;

	TRAPD(err, iServer.AddNotificationL(this, iFullArgs.iTupleName,
		GetExisting, iFullArgs.iPriority));

	if (err==KErrNone || err==KErrAlreadyExists)
		CompleteMessage(EOtherMsgIndex, KErrNone);
	else
		CompleteMessage(EOtherMsgIndex, err);
}

void CBlackBoardServerSession::DeleteTupleFilterL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("DeleteTupleFilterL"));

	ReadTupleAndSubL(EOtherMsgIndex);
	iServer.DeleteNotificationL(this, iFullArgs.iTupleName);
	CompleteMessage(EOtherMsgIndex, KErrNone);
}

void CBlackBoardServerSession::AddComponentFilterL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("AddComponentFilterL"));

	ReadComponentL(EOtherMsgIndex);

	TBool GetExisting=ETrue;
	TRAPD(err, iServer.AddNotificationL(this, iFullArgs.iComponentName,
		GetExisting, EBBPriorityNormal));
	if (err==KErrNone || err==KErrAlreadyExists)
		CompleteMessage(EOtherMsgIndex, KErrNone);
	else
		CompleteMessage(EOtherMsgIndex, err);
}

void CBlackBoardServerSession::DeleteComponentFilterL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("DeleteComponentFilterL"));

	ReadComponentL(EOtherMsgIndex);
	iServer.DeleteNotificationL(this, iFullArgs.iComponentName);
	CompleteMessage(EOtherMsgIndex, KErrNone);
}

#pragma warning(disable: 4706)

void CBlackBoardServerSession::GetOneWaitingL(TUint id)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("GetOneWaitingL"));

	RADbColReadStream rs;
	iServer.GetL(id, iFullArgs.iTupleType, 
		iFullArgs.iTupleName, iFullArgs.iSubName, 
		iFullArgs.iComponentName, rs, iSize,
		iFullArgs.iLeaseExpires);
	iFullArgs.iId=id;
	WriteDataL(rs, ENotifyMsgIndex);
}
TBool CBlackBoardServerSession::SendWaitingL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("SendWaitingL"));

	for(;;) {
		TUint id;
		if ( (id=PopNotification()) ) {
			TInt err;
			CC_TRAPIGNORE(err, KErrNotFound, GetOneWaitingL(id));
			if (err==KErrNone) {
				return ETrue;
			} else if (err!=KErrNotFound) {
				// we may get a not-found if the tuple expires
				// before we get here
				AppendNotificationL(id);

				User::Leave(err);
			}
		} else {
			TTupleName name;
			TBuf<KMaxTupleSubNameLength> subname;
			if ( TopDeleteNotification(name, subname) ) {
				iFullArgs.iTupleName=name;
				iFullArgs.iSubName=subname;
				iFullArgs.iLeaseExpires=TTime(0);
				iFullArgs.iId=0;

				if ( (iMessage[ENotifyMsgIndex]) ==MESSAGE_CLASS()) 
					User::Leave(KErrGeneral);

				TPckg<TFullArgs> meta(iFullArgs);
#ifdef __IPCV2__
				iMessage[ENotifyMsgIndex].WriteL( 2, meta ); 
#else
				iMessage[ENotifyMsgIndex].WriteL( iMessage[ENotifyMsgIndex].Ptr2(), meta ); 
#endif

				CompleteMessage(ENotifyMsgIndex, EDeleteNotification);
				DeleteTopDeleteNotification();

				return ETrue;
			}
			return EFalse;
		}
	}
}

void CBlackBoardServerSession::TerminateServer()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("TerminateServer"));

	CompleteMessage(EOtherMsgIndex, KErrNone);
	iServer.TerminateServer();
}

void CBlackBoardServerSession::NotifyDirectL(TUint aId, TBBPriority aPriority,
		TTupleType aTupleType,
		const TTupleName& aTupleName, const TDesC& aSubName, 
		const TComponentName& aComponent,
		const TDesC8& aSerializedData, const TTime& aExpires)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("NotifyDirectL"));

	iFullArgs.iId=aId;
	iFullArgs.iTupleName=aTupleName;
	iFullArgs.iSubName=aSubName;
	iFullArgs.iComponentName=aComponent;
	iFullArgs.iPriority=aPriority;
	iFullArgs.iTupleType=aTupleType;
	iFullArgs.iLeaseExpires=aExpires;

	WriteDataL(aSerializedData, ENotifyMsgIndex);
}

void CBlackBoardServerSession::NotifyL(TUint aId, TBBPriority aPriority,
			TTupleType aTupleType,
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponent,
			const TDesC8& aSerializedData,
			const TTime& aExpires)

{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("NotifyL"));

	DeleteDeleteNotification(aTupleName, aSubName);
	if (iInPut) return;

	TInt err=-1;
	if ( (!aId || aPriority >= KNotifyDirectLimit) && iWaitingForNotify) {
		TRAPD(err, NotifyDirectL(aId, aPriority, aTupleType,
			aTupleName, aSubName,
			aComponent, aSerializedData, aExpires));
	} 
	if (err!=KErrNone) {
		AppendNotificationL(aId);
		if (err==KClientBufferTooSmall) {
			CompleteMessage(ENotifyMsgIndex, KClientBufferTooSmall);
			iWaitingForNotify=EFalse;
		} else if (aId==0) {
			CompleteMessage(ENotifyMsgIndex, err);
			iWaitingForNotify=EFalse;
		}
		if (aId && iWaitingForNotify) iIdle->Trigger();
	}
}

void CBlackBoardServerSession::NotifyDeletedL(const TTupleName& aTupleName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("NotifyDeletedL"));
	//if (iInDelete) return;

	AppendDeleteNotification(aTupleName, aSubName);
	if ( iWaitingForNotify ) iIdle->Trigger();
}

TBool CBlackBoardServerSession::TopDeleteNotification(TTupleName& aTupleName, TDes& aSubName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("TopDeleteNotification"));
	TInt count=iDeleteTupleNames->Count();
	if (count==0) return EFalse;
	while (iDeleteTupleNames->At(iDeleteTupleNames->Count()-1)==KNoTuple) {
		DeleteTopDeleteNotification();
		if (--count == 0) return EFalse;
	}
	aTupleName=iDeleteTupleNames->At(iDeleteTupleNames->Count()-1);
	aSubName=iDeleteSubNames->MdcaPoint(iDeleteSubNames->Count()-1);
	return ETrue;
}

void CBlackBoardServerSession::DeleteTopDeleteNotification()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("DeleteTopDeleteNotification"));

	iDeleteTupleNames->Delete(iDeleteTupleNames->Count()-1);
	iDeleteSubNames->Delete(iDeleteSubNames->Count()-1);
}

void CBlackBoardServerSession::DeleteDeleteNotification(const TTupleName& aTupleName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("DeleteDeleteNotification"));
	TInt i;
	for (i=0; i<iDeleteTupleNames->Count()-1; i++) {
		if (iDeleteTupleNames->At(i)==aTupleName &&
			iDeleteSubNames->MdcaPoint(i).Compare(aSubName)==0) {
				iDeleteTupleNames->At(i)=KNoTuple;
				break;
			}
	}
}

void CBlackBoardServerSession::AppendDeleteNotification(const TTupleName& aTupleName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("AppendDeleteNotification"));
	TInt i;
	for (i=0; i<iDeleteTupleNames->Count()-1; i++) {
		if (iDeleteTupleNames->At(i)==aTupleName &&
			iDeleteSubNames->MdcaPoint(i).Compare(aSubName)==0) {
				return;
			}
	}
	TInt err;
	TRAP(err, iDeleteTupleNames->AppendL(aTupleName) );
	if (err==KErrNone) {
		TRAP(err, iDeleteSubNames->AppendL(aSubName));
		if (err!=KErrNone) iDeleteTupleNames->Delete( iDeleteTupleNames->Count()-1 );
	}
	User::LeaveIfError(err);
}

void CBlackBoardServerSession::NotifyL(TUint aId, TBBPriority aPriority)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("NotifyL"));

#ifdef __WINS__
	TBuf<100> msg=_L("NOTIFYID: ");
	msg.AppendNum(aId);
#endif
	if (iInPut) return;

	AppendNotificationL(aId);

	if (iWaitingForNotify) iIdle->Trigger();
}

void CBlackBoardServerSession::Back()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("Back"));

	if (!iWaitingForNotify) return;
	TInt err=iServer.GetPermanentError();
	if (err==KErrNone) CC_TRAPIGNORE(err, KClientBufferTooSmall, SendWaitingL());
	if (err!=KErrNone) CompleteMessage(ENotifyMsgIndex, err);
	iWaitingForNotify=EFalse;
}

void CBlackBoardServerSession::WriteDataL(const TDesC8& data, TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("WriteDataL"));

	if ( (iMessage[aWhich]) ==MESSAGE_CLASS()) 
		User::Leave(KErrGeneral);

	TPckg<TFullArgs> meta(iFullArgs);
#ifdef __IPCV2__
	iMessage[aWhich].WriteL( 2, meta ); 
#else
	iMessage[aWhich].WriteL( iMessage[aWhich].Ptr2(), meta ); 
#endif

	if (data.Length()==0) {
		TInt x;
		x=0;
	}
#ifdef __IPCV2__
	iMessage[aWhich].WriteL( 3, data );
#else
	iMessage[aWhich].WriteL( iMessage[aWhich].Ptr3(), data );
#endif

	CompleteMessage(aWhich, KErrNone);
}

void CBlackBoardServerSession::WriteDataL(RADbColReadStream& rs, TMessageIdxs aWhich)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServerSession"), _CL("WriteDataL"));

	if ( (iMessage[aWhich])==MESSAGE_CLASS()) 
		User::Leave(KErrGeneral);

	TUint len;
#ifdef __IPCV2__
	len=iMessage[aWhich].GetDesMaxLength(3);
#else
	len=iMessage[aWhich].Client().GetDesMaxLength(iMessage[aWhich].Ptr3());
#endif
	if (len < iSize ) User::Leave(KClientBufferTooSmall);

	auto_ptr<HBufC8> b(HBufC8::NewL(iSize));
	TPtr8 p=b->Des();
	if (iSize>0) {
		rs.ReadL(p, iSize);
	}

	WriteDataL(p, aWhich);
}
