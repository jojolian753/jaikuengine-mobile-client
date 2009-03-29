#include "break.h"
#include "cbbsession.h"
#include "subscriptions.h"

#include "bberrors.h"
#include "symbian_refcounted_ptr.h"
#include "bbtypes.h"
#include "concretedata.h"
#include <s32mem.h>

class CRefCountedData : public MRefCounted {
public:
	MBBData* Get() { return iData; }
	const MBBData* Get() const { return iData; }
	void AddRef() const { ++iRefCount; }
	void Release() const { --iRefCount; if (iRefCount<=0) delete this; }
	static CRefCountedData* NewL(MBBData* aData) {
		CRefCountedData* c=new CRefCountedData(aData);
		if (!c) {
			delete aData;
			User::Leave(KErrNoMemory);
		}
		c->AddRef();
		return c;
	}
	virtual ~CRefCountedData() { delete iData; }
private:
	CRefCountedData(MBBData *aData) : iData(aData), iRefCount(0) { }
	MBBData* iData;
	mutable TInt iRefCount;
};

class MBlackBoardObserver {
// needs to be named like this for use with CSubscriptions
public:
	virtual void NewValue(TUint aId, const TTupleName& aTupleName, const TDesC& aSubName, 
		const TComponentName& aComponentName, 
		const CRefCountedData* aData) = 0;
	virtual void Deleted(const TTupleName& aName, const TDesC& aSubName) = 0;
};

class CBBSessionImpl : public CBBSession {
private:
	CBBSessionImpl(MApp_context& Context, MBBDataFactory* aFactory);
	void ConstructL();
	~CBBSessionImpl();
	CBBSubSession* CreateSubSessionL(MBBObserver* aObserver);

public:
	void AddNotificationL(const TTupleName& aTupleName, 
			MBlackBoardObserver* anObserver, TBool aGetExisting);
	void AddNotificationL(const TComponentName& aComponentName, 
			MBlackBoardObserver* anObserver, TBool aGetExisting);
	void DeleteNotifications(MBlackBoardObserver* anObserver);

	void GetL(const TTupleName& aName, const TDesC& aSubName, 
			MBBData*& aDataInto, TBool aIgnoreNotFound=EFalse);
	void DeleteL(TUint id, TBool aIgnoreNotFound=EFalse);
	void DeleteL(const TTupleName& aTupleName, const TDesC& aSubName, 
		TBool aIgnoreNotFound=EFalse);
	virtual void DeleteL(const TComponentName& aComponentName, 
		TBool aIgnoreNotFound=EFalse);
	virtual void PutL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName=KNoComponent);
	virtual TUint PutRequestL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName,
			TBool aKeepExisting=EFalse);
	virtual void PutReplyL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName);
	//own
	TUint DoPutL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponentName,
			const MBBData* aData, const TTime& aLeaseExpires,
			TBool aReplace, TBool aIsReply=EFalse,
			TBool aKeepExisting=EFalse);

private:
	virtual MBBDataFactory* GetBBDataFactory() { return iFactory; }

	void CheckedRunL();
	void DoCancel();

	enum TNotifyType {
		EByTuple,
		EByComponent
	};
	void NotifyL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const CRefCountedData* aData, 
		TNotifyType aNotifyType);
	void NotifyDeletedL(const TTupleName& aName, const TDesC& aSubName);

	void ReconnectL();

	friend class auto_ptr<CBBSessionImpl>;
	friend class CBBSession;

	CSubscriptions*		iSubscriptions;
	RBBClient		iClientSession;
	HBufC8			*iGetPutBuf, *iNotifyBuf;
	TFullArgs		iFull;
	TPtr8			iNotifyBufP;
	MBBDataFactory*		iFactory;
};

EXPORT_C CBBSession* CBBSession::NewL(MApp_context& Context, MBBDataFactory* aFactory)
{
	CALLSTACKITEM2_N(_CL("CBBSession"), _CL("NewL"), &Context);

	auto_ptr<CBBSessionImpl> ret(new (ELeave) CBBSessionImpl(Context, aFactory));
	ret->ConstructL();
	return ret.release();
}

_LIT(KSession, "CBBSession");

CBBSession::CBBSession(MApp_context& Context) : CCheckedActive(CActive::EPriorityLow, KSession),
	MContextBase(Context) { }

CBBSessionImpl::CBBSessionImpl(MApp_context& Context, MBBDataFactory* aFactory) : 
	CBBSession(Context), iNotifyBufP(0, 0), iFactory(aFactory) { }

void CBBSessionImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("ConstructL"));

	iSubscriptions=CSubscriptions::NewL();
	User::LeaveIfError(iClientSession.Connect());

	iGetPutBuf=HBufC8::NewL(2048);
	iNotifyBuf=HBufC8::NewL(2048);

	CActiveScheduler::Add(this);
	iNotifyBufP.Set(iNotifyBuf->Des());
	iStatus=KRequestPending;
	SetActive();
	iClientSession.WaitForNotify(iFull, iNotifyBufP, iStatus);
}

CBBSessionImpl::~CBBSessionImpl()
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("~CBBSessionImpl"));

	Cancel();
	iClientSession.Close();
	delete iSubscriptions;

	delete iGetPutBuf;
	delete iNotifyBuf;
}

void CBBSessionImpl::AddNotificationL(const TTupleName& aTupleName, 
		MBlackBoardObserver* anObserver, TBool aGetExisting)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("AddNotificationL"));

	iSubscriptions->AddNotificationL(anObserver, aTupleName, EBBPriorityNormal);
	TRequestStatus s;

	iClientSession.AddNotificationL(aTupleName, aGetExisting, EBBPriorityNormal, s);
	User::WaitForRequest(s);
	User::LeaveIfError(s.Int());
}

void CBBSessionImpl::DeleteL(TUint id, TBool aIgnoreNotFound)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("DeleteL"));

	TRequestStatus s;
	iClientSession.Delete(id, s);

	User::WaitForRequest(s);
	if (s.Int()==KErrNotFound && aIgnoreNotFound) return;
	User::LeaveIfError(s.Int());
}

void CBBSessionImpl::DeleteL(const TTupleName& aTupleName, 
							 const TDesC& aSubName,
							 TBool aIgnoreNotFound)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("DeleteL"));

	TRequestStatus s;
	iClientSession.Delete(aTupleName, aSubName, s);

	User::WaitForRequest(s);
	if (s.Int()==KErrNotFound && aIgnoreNotFound) return;
	User::LeaveIfError(s.Int());
}

void CBBSessionImpl::DeleteL(const TComponentName& aComponentName, 
							 TBool aIgnoreNotFound)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("DeleteL"));

	TRequestStatus s;
	iClientSession.Delete(aComponentName, s);

	User::WaitForRequest(s);
	if (s.Int()==KErrNotFound && aIgnoreNotFound) return;
	User::LeaveIfError(s.Int());
}

void CBBSessionImpl::AddNotificationL(const TComponentName& aComponentName, 
		MBlackBoardObserver* anObserver, TBool aGetExisting)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("AddNotificationL"));

	//FIXME: get existing
	iSubscriptions->AddNotificationL(anObserver, aComponentName, EBBPriorityNormal);
	TRequestStatus s;

	iClientSession.AddNotificationL(aComponentName, s);
	User::WaitForRequest(s);
	User::LeaveIfError(s.Int());
}

void CBBSessionImpl::DeleteNotifications(MBlackBoardObserver* anObserver)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("DeleteNotifications"));

	CC_TRAPD(err, iSubscriptions->DeleteAllSubscriptionsL(anObserver));
	if (err!=KErrNone && err!=KErrNotFound) {
		User::Panic(_L("BBSession"), err);
	}
}

_LIT(KEvent, "event");

void CBBSessionImpl::GetL(const TTupleName& aName, const TDesC& aSubName, 
		MBBData*& aDataInto, TBool aIgnoreNotFound)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("GetL"));

	TFullArgs meta;
	delete aDataInto; aDataInto=0;
	TRequestStatus s;
	TPtr8 bufp(iGetPutBuf->Des());
	iClientSession.Get(aName, aSubName, meta, bufp, s);
	User::WaitForRequest(s);
	if (aIgnoreNotFound && s.Int()==KErrNotFound) return;
	while (s.Int()==KClientBufferTooSmall) {
		iGetPutBuf=iGetPutBuf->ReAllocL( iGetPutBuf->Des().MaxLength() );
		TPtr8 bufp(iGetPutBuf->Des());
		iClientSession.Get(aName, aSubName, meta, bufp, s);
		User::WaitForRequest(s);
	}
	User::LeaveIfError(s.Int());
	RDesReadStream rs(*iGetPutBuf);
	TTypeName tn=TTypeName::IdFromStreamL(rs);
	aDataInto=iFactory->CreateBBDataL(tn, KEvent, iFactory);
	aDataInto->InternalizeL(rs);
}


TUint CBBSessionImpl::DoPutL(const TTupleName& aTupleName, const TDesC& aSubName, 
		const TComponentName& aComponentName,
		const MBBData* aData, const TTime& aLeaseExpires,
		TBool aReplace, TBool aIsReply, TBool aKeepExisting)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("DoPutL"));

	refcounted_ptr<CRefCountedData> p(CRefCountedData::NewL(aData->CloneL(KEvent)));

	TRequestStatus s;
	TInt err=KErrNone;
	{
		TPtr8 bufp(iGetPutBuf->Des());
		RDesWriteStream ws(bufp);
		aData->Type().ExternalizeL(ws);
		CC_TRAPIGNORE(err, KErrOverflow, aData->ExternalizeL(ws));
		if (err==KErrNone) ws.CommitL();
	}
	while (err==KErrOverflow) {
		iGetPutBuf=iGetPutBuf->ReAllocL( iGetPutBuf->Des().MaxLength() *2);
		TPtr8 bufp(iGetPutBuf->Des());
		RDesWriteStream ws(bufp);
		aData->Type().ExternalizeL(ws);
		CC_TRAPIGNORE(err, KErrOverflow, aData->ExternalizeL(ws));
		if (err==KErrNone) ws.CommitL();
	}
	TUint id=0;
	iClientSession.Put(aTupleName, aSubName, aComponentName, *iGetPutBuf, EBBPriorityNormal, 
		aReplace, id, s, aLeaseExpires, ETrue, EFalse, aIsReply, aKeepExisting);

	User::WaitForRequest(s);
	if (! aIsReply ) {
		CC_TRAP(err, NotifyL(id, aTupleName, aSubName, aComponentName, p.get(), EByTuple));
	} else {
		CC_TRAP(err, NotifyL(id, aTupleName, aSubName, aComponentName, p.get(), EByComponent));
	}

	User::LeaveIfError(s.Int());
	User::LeaveIfError(err);
	
	return id;
}

void CBBSessionImpl::ReconnectL()
{
	iClientSession.Close();
	User::LeaveIfError(iClientSession.Connect());
	{
		TTupleName tn=iSubscriptions->FirstSubscriptionTupleL();
		while (! (tn==KNoTuple) ) {
			TRequestStatus s;

			iClientSession.AddNotificationL(tn, EFalse, EBBPriorityNormal, s);
			User::WaitForRequest(s);
			if (s.Int()!=KErrNone && s.Int()!=KErrAlreadyExists)
				User::Leave(s.Int());
			tn=iSubscriptions->NextSubscriptionTupleL();
		}
	}
	{
		TComponentName tn=iSubscriptions->FirstSubscriptionComponentL();
		while (! (tn==KNoComponent) ) {
			TRequestStatus s;

			iClientSession.AddNotificationL(tn, s);
			User::WaitForRequest(s);
			if (s.Int()!=KErrNone && s.Int()!=KErrAlreadyExists)
				User::Leave(s.Int());
			tn=iSubscriptions->NextSubscriptionComponentL();
		}
	}
	iStatus=KRequestPending;
	SetActive();
	iClientSession.WaitForNotify(iFull, iNotifyBufP, iStatus);
}	

void CBBSessionImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("CheckedRunL"));

	if (iStatus.Int() < KErrNone) {
		CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("err"));
		if (iStatus.Int()==KClientBufferTooSmall) {
			CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("toosmall"));
			iNotifyBuf->Des().Zero();
			iNotifyBuf=iNotifyBuf->ReAllocL(iNotifyBuf->Des().MaxLength()*2);
			iNotifyBufP.Set(iNotifyBuf->Des());
			iStatus=KRequestPending;
			SetActive();
			iClientSession.WaitForNotify(iFull, iNotifyBufP, iStatus);
			return;
		} else if(iStatus.Int()==KErrServerTerminated) {
			CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("terminated"));
			ReconnectL();
			return;
		} else {
			CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("other"));
			User::Leave(iStatus.Int());
		}
	}
	if (iStatus.Int() == EDeleteNotification) {
		CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("deleted"));
		NotifyDeletedL(iFull.iTupleName, iFull.iSubName);
	} else if (iNotifyBuf->Des().Length()>0) {
		{
			CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("read"));
			RDesReadStream rs(*iNotifyBuf);
			TTypeName tn;
			TInt err;
			MBBData* datap=0;
			CC_TRAP(err, tn=TTypeName::IdFromStreamL(rs));
			if (err!=KErrNone) {
				CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("err1"));
				TBBLongString* s=new (ELeave) TBBLongString(KEvent);
				datap=s;
				s->Value().Append(_L("error reading datatype: "));
				s->Value().AppendNum(err);
			}
			if (err==KErrNone) {
				CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("create"));
				CC_TRAP(err, datap=iFactory->CreateBBDataL(tn, KEvent, iFactory));
				if (err!=KErrNone) {
					CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("err2"));
					TBBLongString* s=new (ELeave) TBBLongString(KEvent);
					datap=s;
					s->Value().Append(_L("error creating data: "));
					s->Value().AppendNum(err);
					s->Value().Append(_L(", type: "));
					tn.IntoStringL(s->Value());
				}
			}
			refcounted_ptr<CRefCountedData> data(CRefCountedData::NewL(datap));
			if (err==KErrNone) {
				CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("internalize"));
				CC_TRAP(err, data->Get()->InternalizeL(rs));
				if (err!=KErrNone) {
					CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("err"));
					TBBLongString* s=new (ELeave) TBBLongString(KEvent);
					data.reset(CRefCountedData::NewL(s));
					s->Value().Append(_L("error internalizing data: "));
					s->Value().AppendNum(err);
					s->Value().Append(_L(", type: "));
					tn.IntoStringL(s->Value());
				}
			}
			{
			TNotifyType nt=EByTuple;
			if (iFull.iTupleType==ETuplePermanentSubscriptionEvent || 
				iFull.iTupleType==ETupleReply) {
					nt=EByComponent;
			}
			{
				CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("notify"));
				NotifyL(iFull.iId, iFull.iTupleName, iFull.iSubName, 
					iFull.iComponentName, data.get(), nt);
			}
			}
		}
	}

	{
		CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("next"));
		iNotifyBuf->Des().Zero();
		iNotifyBufP.Set(iNotifyBuf->Des());
		iStatus=KRequestPending;
		SetActive();
		iClientSession.WaitForNotify(iFull, iNotifyBufP, iStatus);
	}
}

void CBBSessionImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("DoCancel"));

	iClientSession.CancelNotify();
}

void CBBSessionImpl::NotifyL(TUint aId, 
			     const TTupleName& aName, const TDesC& aSubName, 
			     const TComponentName& aComponent,
			     const CRefCountedData* aData,
			     TNotifyType aNotifyType)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("NotifyL"));

	TBBPriority prio;
	if (aNotifyType==EByTuple) {
		for (MBlackBoardObserver* obs=iSubscriptions->FirstL(aName, prio); obs; 
				obs=iSubscriptions->NextL(prio)) {
			obs->NewValue(aId, aName, aSubName, aComponent, aData);
		}
	} else {
		for (MBlackBoardObserver* obs=iSubscriptions->FirstL(aComponent, prio); obs; 
				obs=iSubscriptions->NextL(prio)) {
			obs->NewValue(aId, aName, aSubName, aComponent, aData);
		}
	}
}

void CBBSessionImpl::NotifyDeletedL(const TTupleName& aName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("NotifyDeletedL"));

	TBBPriority prio;
	for (MBlackBoardObserver* obs=iSubscriptions->FirstL(aName, prio); obs; 
			obs=iSubscriptions->NextL(prio)) {
		obs->Deleted(aName, aSubName);
	}
}

void CBBSessionImpl::PutL(const TTupleName& aTupleName, const TDesC& aSubName, 
		const MBBData* aData, const TTime& aLeaseExpires,
		const TComponentName& aComponentName)
{
	CALLSTACKITEM_N(_CL("CBBSessionImpl"), _CL("PutL"));

	DoPutL(aTupleName, aSubName, aComponentName, aData, aLeaseExpires, ETrue);
}

TUint CBBSessionImpl::PutRequestL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName,
			TBool aKeepExisting)
{
	DoPutL(aTupleName, aSubName, aComponentName, aData, aLeaseExpires, 
		EFalse, EFalse,	aKeepExisting);
}
void CBBSessionImpl::PutReplyL(const TTupleName& aTupleName, const TDesC& aSubName, 
		const MBBData* aData, const TTime& aLeaseExpires,
		const TComponentName& aComponentName)
{
	DoPutL(aTupleName, aSubName, aComponentName, aData, aLeaseExpires, EFalse, ETrue);
}

class CBBSubSessionImpl : public CBBSubSession, MBlackBoardObserver {
public:
	static CBBSubSessionImpl* NewL(MApp_context& Context, CBBSessionImpl* aSession, MBBObserver* aObserver);
	~CBBSubSessionImpl();
private:
	virtual void AddNotificationL(const TTupleName& aTupleName);
	virtual void AddNotificationL(const TTupleName& aTupleName, TBool aGetExisting);
	virtual void AddNotificationL(const TComponentName& aComponentName);
	virtual void DeleteNotifications();

	virtual void GetL(const TTupleName& aName, const TDesC& aSubName, 
			MBBData*& aDataInto, TBool aIgnoreNotFound=EFalse);
	virtual void PutL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName=KNoComponent);
	virtual TUint PutRequestL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName, TBool aKeepExisting=EFalse);
	virtual void PutReplyL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName);
	virtual void DeleteL(TUint id, TBool aIgnoreNotFound=EFalse);
	virtual void DeleteL(const TTupleName& aTupleName, 
		const TDesC& aSubName, TBool aIgnoreNotFound=EFalse);
	virtual void DeleteL(const TComponentName& aComponentName, 
		TBool aIgnoreNotFound=EFalse);

	CBBSubSessionImpl(MApp_context& Context, CBBSessionImpl* aSession, MBBObserver* aObserver);
	virtual void NewValue(TUint aId, 
		const TTupleName& aTupleName, const TDesC& aSubName, 
		const TComponentName& aComponentName, 
		const CRefCountedData* aData);
	virtual void Deleted(const TTupleName& aName, const TDesC& aSubName);

	void Async();

	void ConstructL();

	void CheckedRunL();
	void DoCancel();

	struct TItem {
		TUint		iId;
		TTupleName	iName;
		TBuf<128>	 iSubName;
		TComponentName	iComponentName;
		const CRefCountedData* iData;
		TItem(TUint aId, const TTupleName& aName, const TDesC& aSubName, const TComponentName& aComponentName,
			const CRefCountedData* aData) :
			iId(aId), iName(aName), iSubName(aSubName), iComponentName(aComponentName), iData(aData) { }
		TItem() : iId(0), iName(KNoTuple), iComponentName(KNoComponent), iData(0) { }
	};

	CList<TItem> *iPending;
	CBBSessionImpl *iSession;
	MBBObserver*	iObserver;
};

CBBSubSession* CBBSessionImpl::CreateSubSessionL(MBBObserver* aObserver) {
	return CBBSubSessionImpl::NewL(AppContext(), this, aObserver);
}

CBBSubSessionImpl::~CBBSubSessionImpl()
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("~CBBSubSessionImpl"));

	Cancel();
	if (iPending) {
		TItem t=iPending->Pop();
		while(t.iData) {
			t.iData->Release();
			t=iPending->Pop();
		}
		delete iPending;
	}
	iSession->DeleteNotifications(this);
}

CBBSubSessionImpl* CBBSubSessionImpl::NewL(MApp_context& Context, CBBSessionImpl* aSession, MBBObserver* aObserver)
{
	CALLSTACKITEM2_N(_CL("CBBSubSessionImpl"), _CL("NewL"), &Context);

	auto_ptr<CBBSubSessionImpl> ret(new (ELeave) CBBSubSessionImpl(Context, aSession, aObserver));
	ret->ConstructL();
	return ret.release();
}

void CBBSubSessionImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("ConstructL"));

	if (iObserver) {
		CActiveScheduler::Add(this);
		iPending=CList<TItem>::NewL();
	}
}

void CBBSubSessionImpl::AddNotificationL(const TTupleName& aTupleName)
{
	AddNotificationL(aTupleName, EFalse);
}

void CBBSubSessionImpl::AddNotificationL(const TTupleName& aTupleName, TBool aGetExisting)
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("AddNotificationL"));

	if (!iObserver) User::Leave(KErrArgument);
	iSession->AddNotificationL(aTupleName, this, aGetExisting);
}

void CBBSubSessionImpl::AddNotificationL(const TComponentName& aComponentName)
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("AddNotificationL"));

	if (!iObserver) User::Leave(KErrArgument);
	iSession->AddNotificationL(aComponentName, this, EFalse);
}

void CBBSubSessionImpl::DeleteNotifications()
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("DeleteNotifications"));

	iSession->DeleteNotifications(this);
}

void CBBSubSessionImpl::GetL(const TTupleName& aName, const TDesC& aSubName, 
			MBBData*& aDataInto, TBool aIgnoreNotFound)
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("GetL"));

	iSession->GetL(aName, aSubName, aDataInto, aIgnoreNotFound);
}
void CBBSubSessionImpl::PutL(const TTupleName& aTupleName, const TDesC& aSubName, 
		const MBBData* aData, const TTime& aLeaseExpires,
		const TComponentName& aComponentName)
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("PutL"));

	iSession->DoPutL(aTupleName, aSubName, aComponentName, aData, aLeaseExpires, ETrue);
}

TUint CBBSubSessionImpl::PutRequestL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName, TBool aKeepExisting)
{
	return iSession->DoPutL(aTupleName, aSubName, aComponentName, aData, aLeaseExpires, 
		EFalse, EFalse, aKeepExisting);
}

void CBBSubSessionImpl::PutReplyL(const TTupleName& aTupleName, const TDesC& aSubName, 
		const MBBData* aData, const TTime& aLeaseExpires,
		const TComponentName& aComponentName)
{
	iSession->DoPutL(aTupleName, aSubName, aComponentName, aData, aLeaseExpires, EFalse, ETrue);
}

void CBBSubSessionImpl::NewValue(TUint aId, const TTupleName& aTupleName, const TDesC& aSubName, const TComponentName& aComponentName, 
	const CRefCountedData* aData)
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("NewValue"));

	TItem t(aId, aTupleName, aSubName, aComponentName, aData);
	iPending->AppendL(t);
	t.iData->AddRef();
	Async();
}

void CBBSubSessionImpl::Deleted(const TTupleName& aName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("NewValue"));

	TItem t(0, aName, aSubName, KNoComponent, 0);
	iPending->AppendL(t);
	Async();
}

_LIT(KSubSession, "CBBSubSession");

CBBSubSession::CBBSubSession(MApp_context& Context) : CCheckedActive(CActive::EPriorityStandard, KSubSession),
		MContextBase(Context) { }

CBBSubSessionImpl::CBBSubSessionImpl(MApp_context& Context, CBBSessionImpl* aSession, 
				     MBBObserver* aObserver) : CBBSubSession(Context), 
				     iSession(aSession), iObserver(aObserver) { }

void CBBSubSessionImpl::Async()
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("Async"));

	if (IsActive()) return;
	TRequestStatus *s=&iStatus;
	iStatus=KRequestPending;
	SetActive();
	User::RequestComplete(s, KErrNone);
}

void CBBSubSessionImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("CheckedRunL"));

	TItem t=iPending->Pop();
	TInt err;
	if (iPending->iCount > 0) Async();
	if (t.iData) {
		CC_TRAP(err, iObserver->NewValueL(t.iId, t.iName, t.iSubName, t.iComponentName, t.iData->Get()));
		t.iData->Release();
	} else {
		CC_TRAP(err, iObserver->DeletedL(t.iName, t.iSubName));
	}
	User::LeaveIfError(err);
}

void CBBSubSessionImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CBBSubSessionImpl"), _CL("DoCancel"));

}

void CBBSubSessionImpl::DeleteL(TUint id, TBool aIgnoreNotFound)
{
	iSession->DeleteL(id, aIgnoreNotFound);
}

void CBBSubSessionImpl::DeleteL(const TTupleName& aTupleName, 
								const TDesC& aSubName, TBool aIgnoreNotFound)
{
	iSession->DeleteL(aTupleName, aSubName, aIgnoreNotFound);
}

void CBBSubSessionImpl::DeleteL(const TComponentName& aComponentName, 
		TBool aIgnoreNotFound) 
{
	iSession->DeleteL(aComponentName, aIgnoreNotFound);
}
