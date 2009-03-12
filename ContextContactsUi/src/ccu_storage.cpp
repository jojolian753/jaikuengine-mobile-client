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

#include "ccu_storage.h"

#include "break.h"
#include "symbian_refcounted_ptr.h"
#include "raii_d32dbms.h"
#include "symbian_tree.h"
#include "cl_settings.h"
#include "reporting.h"
#include "bberrorinfo.h"
#include "cbbsession.h"
#include "timeout.h"
#include "csd_idle.h"
#include "csd_current_app.h"
#include "csd_event.h"
#include "cc_stringtools.h"

#include <eikenv.h> 
#include <gulicon.h>
#include <bautils.h>
#include <charconv.h>

#include "contextvariant.hrh"

//#define DEBUG_STORAGE
#ifdef DEBUG_STORAGE
#include "callstack.h"
#endif

#ifdef __JAIKU_PHOTO_DOWNLOAD__
#include "downloader.h"
#endif


//#define EXPLICIT_TRANSACTIONS

/*
 * The NotifyFeedItemEvent(...) functions may call SwitchIndex or move around in the
 * table, either directly or by the functions they call. The manipulation functions
 * that call NotifyFeedItemEvent() must take care that they return to where they
 * were in the table after calling.
 */

// ? is not valid in real nicks, so we can use anything beginning with it to store private data
_LIT(KOverviewCountNick, "?ov");
_LIT(KAllCountNick, "?all");

class CAuthorStats : public CBase, public MContextBase, public MDBStore {
public:
	enum TColumns {
		EDbId = 1,
		EAuthor,
		EUnread,
		EChildCount,
		EUnreadUntilId,
	};
	enum TIndices {
		EIdxAuthor = 0,
		EIdxDbId,
	};
	CAuthorStats(MApp_context& Context, CDb& Db);
	void ConstructL();
	void UpdateStatsL(TUint aAuthor, TInt& aChildDelta, TInt& aUnreadDelta, TDes& aAuthorNick);
	void SetStatsL(TUint aAuthor, TInt& aChildCount, TInt& aUnreadCount, TDes& aAuthorNick); // -1 means don't set
	void SetAllAsUnread(CList<MFeedNotify*>	*aSubscribers);
	void GetStatsL(TUint aAuthor, TInt& aChildCount, TInt& aUnreadCount, TDes& aAuthorNick);
	void SetAuthorAsRead(TUint aAuthor, TUint aUntilId, TDes& aAuthorNick);
	
	void UpdateStatsL(const TDesC& aAuthor, TInt& aChildDelta, TInt& aUnreadDelta);
	void SetStatsL(const TDesC& aAuthor, TInt& aChildCount, TInt& aUnreadCount); // -1 means don't set
	void GetStatsL(const TDesC& aAuthor, TInt& aChildCount, TInt& aUnreadCount);
	void SetAuthorAsRead(const TDesC& aAuthor, TUint aUntilId);

	TUint32 IsUnreadUntilId(const TDesC& aAuthor);
	TUint32 IsUnreadUntilId(TUint32 aAuthor);
	
	const TDesC& GetNickByIdL(TUint aId);
	TUint GetIdByNickL(const TDesC& aAuthor);
private:
	void UpdateThisStatsL(TInt& aChildDelta, TInt& aUnreadDelta);
	void SetThisStatsL(TInt& aChildCount, TInt& aUnreadCount, TBool insert_if_doesnt_exist); // -1 means don't set
	//virtual CDb* SupportIncremental() { return &iDb; }
	TBool SeekToAuthorL(const TDesC& aAuthor, TBool aInsertIfDoesntExist);
	TBool SeekToAuthorL(TUint aAuthor, TBool aInsertIfDoesntExist);
	CDb&	iDb;
};

#define KGlobalIdLength 16

class TBookmark {
public:
	TDbBookmark	iBookmark;
	TInt		iOrder;
	TUint		iAuthorId;
	TGlobalId	iParent;
	TBool		iFound;
	TBool		iAscDate;
	TBool		iOverViewOnly;
};

class CFeedItemStorageImpl: public CFeedItemStorage, 
		public MContextBase, public MDBStore, 
		public MFeedItemDeletionNotify,
		public MBBObserver, public MTimeOut
#ifdef __JAIKU_PHOTO_DOWNLOAD__
						  ,  public MDownloadObserver 
						  ,  public MPurgeObserver
#endif // __JAIKU_PHOTO_DOWNLOAD__
{
	enum TColumns {
		EDbId = 1,
		EParentId,
		EFeedItemId,
		EDatetime,
		ELastAccess,
		EAuthorId,
		EUnread,
		EIcon, 
		EContents,
		EHidden,
		EErrorInfo,
		EUpdated,
		EChildCount,
		EUnreadChildren,
		EGroupChild,
		EGroupChildCount,
		ECorrelation,
		EShowInOverView,
		EMediaDownloadState,
	};
	enum TIndices {
		EIdxDbId = 0,
		EIdxDatetime,
		EIdxParentDatetime ,
		EIdxAuthorDatetime,
		EIdxFeedItemId,
		EIdxAuthorDbId,
		EIdxCorrelation,
	};

	// adding
	virtual void AddLocalL(CBBFeedItem* aFeedItem);
	virtual void DoAddLocalL(CBBFeedItem* aFeedItem, TBool aReplace);
	virtual void MarkAsRead(CBBFeedItem* aFeedItem);
	virtual void MarkAsRead(const TDesC& aAuthor);
	virtual void MarkAsRead(const TGlobalId& aItemOrParent);
	virtual void MarkAllAsRead();
	virtual void RemoveFeedItemL(const CBBFeedItem * aFeedItem, TBool aMediaRemoveFile=EFalse);
	virtual TBool RemoveFeedItemsL(TTime aFromTime,
		TFromType aFromType, TInt aMinimumToLeaveByAuthor=0,
		TInt aMaximumToRemove=0);
	virtual TBool RemoveFeedItemsL(TTime aFromTime,
		TFromType aFromType, TInt aMinimumToLeaveByAuthor=0,
		TInt aMaximumToRemove=0, TBool aContinue=EFalse);
	virtual void DoRemoveFeedItemL(const CBBFeedItem * aFeedItem, TBool aMediaRemoveFile=ETrue);
	void DoMarkAsReadL(TUint db_id, TInt& read_count);
	void UpdateReadCounters(const TGlobalId& from_id, TUint aAuthor, TInt read_counter, TBool aIsGroupChild);
	void UpdateReadCounters(const TDesC8& from_id, TUint aAuthor, TInt read_counter, TBool aIsGroupChild);

	void UpdateChildCounters(const TGlobalId& from_id, TUint aAuthor, TInt child_counter, TBool aIsGroupChild,
		TBool aUpdateUp=ETrue);
	void UpdateChildCounters(const TDesC8& from_id, TUint aAuthor, TInt child_counter, TBool aIsGroupChild,
		TBool aUpdateUp=ETrue);

	virtual void GetCountsByAuthorL(const TDesC& aAuthor, TInt& aItemCount, TInt& aUnreadCount);
	void GetChildCountsL(const TGlobalId& aParent, TInt& aChildCount, TInt& aUnreadChildCount,
		TInt& aGroupChildCount, TBool aOverViewOnly=EFalse);

	void UpdateFeedItemL(CBBFeedItem * aFeedItem, TBool aNotify);
	virtual void UpdateFeedItemL(CBBFeedItem * aFeedItem) { UpdateFeedItemL(aFeedItem, ETrue); }
	//virtual void UpdateFileName(TGlobalId aFeedItemId, const TDesC& aFileName, const TDesC& aContentType);
	//virtual void UpdateIcon(const TGlobalId& aFeedItemId, auto_ptr<CFbsBitmap>& aIcon);
	//virtual void UpdateError(const TGlobalId& aFeedItemId, TInt aErrorCode, const TDesC& aError);
	virtual void UpdateErrorInfo(const TGlobalId& aFeedItemId, const CBBErrorInfo* aErrorInfo, TBool aWillRetry);
	virtual void UpdateErrorInfo(TUint aDbId, const CBBErrorInfo* aErrorInfo, TBool aWillRetry);
	virtual void UpdateCurrentErrorInfo(const CBBErrorInfo* aErrorInfo, TBool aWillRetry);
	virtual void UpdateDownloadStateL(CBBFeedItem * aFeedItem);
	virtual void UpdateDownloadStateL(TInt64 aId, CBBFeedItem::TMediaDownloadState aState);

	virtual void CommitL();
	virtual TBool CompactIfHighWaterL();
	
	virtual void DownloadMediaForFeedItemL(CBBFeedItem* aFeedItem, TBool aManual=EFalse);
	virtual void SetDownloadModeL( TDownloadMode aMode );
	virtual TDownloadMode DownloadMode() const;
	virtual void SetOfflineL(TBool aIsOffline);
	virtual TBool IsDownloading( PresencePublisher::EState& aReason);

	//virtual void SetThreadVisibleL(const TGlobalId& aFeedItemId, TBool visible);
	//virtual void SetAllThreadsVisibleL();
	//virtual TInt HasHiddenThreads();

	//void DoUpdateFileName(const TGlobalId& aFeedItemId, const TDesC& aFileName, const TDesC& aContentType);
	//void DoUpdateIcon(const TGlobalId& aFeedItemId, auto_ptr<CFbsBitmap>& aIcon);

	// iteration
	virtual TBool FirstAllL(TBool aAscDate=EFalse, TBool aOverViewOnly=EFalse);
	virtual TBool FirstByParentL(const TGlobalId& aParent, TBool aAscDate=EFalse);
	virtual TBool FirstByAuthorL(const TDesC& aAuthor, TBool aAscDate=EFalse);

	virtual TBookmark* Bookmark(); // you get ownership, delete with NextL or ReleaseBookmark
	virtual TBool NextL(TBookmark*& aBookmark);
	virtual void ReleaseBookmark(TBookmark*& aBookmark);

	virtual TBool NextL();
	virtual TUint GetCurrentLocalIdL();
	virtual void GetCurrentGlobalIdL(TGlobalId& aInto);
	virtual TBool CurrentIsGroupChild();
	virtual TTime GetCurrentTimeStampL();
	
	// accessing
	virtual CBBFeedItem* GetCurrentL(TBool aIgnoreNotFound = EFalse); // addrefs
	virtual CBBFeedItem* GetByLocalIdL(TUint aLocalId, TBool aIgnoreNotFound = EFalse); // addrefs
	virtual CBBFeedItem* GetByGlobalIdL(const TGlobalId& aFeedItemId, TBool aIgnoreNotFound = EFalse); // addrefs
	virtual void SubscribeL(MFeedNotify* aNotify); // addrefs
	virtual void UnSubscribeL(MFeedNotify* aNotify); // addrefs
	virtual TBool GetFirstByCorrelationL(const TGlobalId& aCorrelation, TGlobalId &aPostIdInto);
	
	CFeedItemStorageImpl(MApp_context& Context, CDb& Db, MBBDataFactory* aBBFactory);
	void ConstructL(
#ifdef __JAIKU_PHOTO_DOWNLOAD__
		HttpFactory aHttpFactory=0,
		const TDesC& aDownloadDbName=KNullDesC
#endif
);
	~CFeedItemStorageImpl();
	virtual TInt TextLength(TInt aColumn);
	
	void ReadChildCountsL(const TGlobalId& aParent, TInt& aChildCount, TInt& aUnreadChildCount,
		TInt& aGroupChildCount);
	
	void DoCancel() { }
	void CheckedRunL();
	void Async();
	void SwitchIndexL(TInt aIndex);
	TBool IsUnread();
	CBBFeedItem::TMediaDownloadState MediaDownloadState(CBBFeedItem* aItem);
	
#ifdef DEBUG_STORAGE
	void PrintDebugObserver(MFeedNotify* aNotify);
	void PrintDebugRelease(MFeedNotify* aNotify);
	void PrintDebugUpdate(CBBFeedItem* aFeedItem);
#else
	void PrintDebugObserver(MFeedNotify* ) { }
	void PrintDebugRelease(MFeedNotify* ) { }
	void PrintDebugUpdate(CBBFeedItem* ) { }
#endif
	friend class CFeedItemStorage;
	friend class auto_ptr<CFeedItemStorageImpl>;

	TBool SeekToFeedItemIdL(const TGlobalId& aFeedItemId);
	TBool SeekToDbIdL(TUint aDbId);
	TBool NotInOverView();

	//TInt CountHiddenThreadsL();

	void FeedItemDeleted(const class CBBFeedItem* aFeedItem);

	CDb&	iDb;

	TGlobalId iSeekParentId; 
	TUint  iSeekAuthorId;
	TBuf<50>  iLCAuthor;
	TBool iSeekOnlyUnread, iOverViewOnly;
	enum TSeekBy { ESeekAll, ESeekParent, ESeekAuthor };
	TSeekBy iSeekBy; TBool iAscDate;
	TBool iFound;

	//TInt iNbHiddenThreads;

	// notification support
	CGenericIntMap*		iDbIdToObjectMap;
	CList<MFeedNotify*>	*iSubscribers;

	void NotifyObservers(CBBFeedItem* aFeedItem, MFeedNotify::TEvent aEvent);
	void NotifyObserversL(CBBFeedItem* aFeedItem, MFeedNotify::TEvent aEvent);
	void NotifyObservers(const TDesC& aAuthor, TInt aNewItemCount, TInt aNewUnreadCount);
	static void ReleaseFeedItem(void* data);

	enum TDlLevels {
		EDlLevelAutomatic = 0,
		EDlLevelOnLook = 10,
		EDlLevelOnRequest = 20,
		EDlLevelManualRequest = 30,
	};
	
	MBBDataFactory*		iBBFactory;
	CAuthorStats*	iAuthorCounts;
	TInt			iAsyncCount;
#ifdef __WINS__
	// debug aid
	TBool iInMarkAuthor;
#endif
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData);
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName);
	virtual void expired(CBase*);
	virtual void StopBackgroundActivities();
	CTimeOut*		iPurgeTimer;
	CBBSubSession*		iBBSubSession;
	TBool			iInJaiku, iIdle;
	TTime			iLastPurge;
	
	TBool iInPurge;
	TInt iCurrentAuthorCount, iCurrentAuthorUnreadChange, iCurrentAuthorChildChange;
	TBool iCurrentAuthorHaveSeenAJaiku;
	TUint iCurrentAuthorId;
	TDbBookmark iRow;

#ifdef __JAIKU_PHOTO_DOWNLOAD__	
	CDownloader		*iDownloader;
	CDb				*iDownloadDb;
	virtual void DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
        	const TDesC& aContentType);
	virtual void DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr,
		TBool aWillRetry);
	virtual void DownloadStarted(TInt64 aRequestId);
	virtual void Dequeued(TInt64 aRequestId);
	virtual void ConnectivityChanged(TBool aConnecting, TBool aOfflineMode,
		TBool aLowSignal, TBool aCallInProgress);
	TBool	iOfflineMode, iLowSignal, iCallInProgress, iConnecting;
	
	void FilePurged(const TDesC& aFilename) { }
	CPurger* iPurger, *iOldPurger;
#endif
	//virtual CDb* SupportIncremental() { return &iDb; }
	class CDownloader* Downloader() {
#ifdef __JAIKU_PHOTO_DOWNLOAD__	
		return iDownloader;
#else
		return 0;
#endif	
	}
};


EXPORT_C CFeedItemStorage* CFeedItemStorage::NewL(MApp_context& Context, CDb& Db, MBBDataFactory* aBBFactory
#ifdef __JAIKU_PHOTO_DOWNLOAD__
		, HttpFactory aHttpFactory
		, const TDesC& aDownloadDbName
#endif
)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorage"), _CL("NewL"));

	auto_ptr<CFeedItemStorageImpl> ret(new (ELeave) CFeedItemStorageImpl(Context, Db, aBBFactory));
	ret->ConstructL(
#ifdef __JAIKU_PHOTO_DOWNLOAD__
		aHttpFactory, aDownloadDbName
#endif
	);
	return ret.release();
}

void CFeedItemStorageImpl::ReleaseFeedItem(void* data)
{
	CALLSTACKITEMSTATIC_N(_CL("CFeedItemStorageImpl"), _CL("ReleaseFeedItem"));
	CBBFeedItem* p=(CBBFeedItem*)data;
	p->SetDeletionNotify(0);
}

CFeedItemStorageImpl::~CFeedItemStorageImpl()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("~CFeedItemStorageImpl"));

	Cancel();
	TRAPD(err, {
	CommitL();
	});

	if (iDbIdToObjectMap) {
		iDbIdToObjectMap->SetDeletor(ReleaseFeedItem);
	}
	delete iDbIdToObjectMap;
	delete iSubscribers;
	delete iAuthorCounts;
	delete iPurgeTimer;
	delete iBBSubSession;

#ifdef __JAIKU_PHOTO_DOWNLOAD__
	delete iDownloader;
	delete iDownloadDb;
	delete iPurger;
	delete iOldPurger;
#endif // __JAIKU_PHOTO_DOWNLOAD__
}

_LIT(KThread, "thread");

void CFeedItemStorageImpl::AddLocalL(CBBFeedItem* aFeedItem)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("AddLocalL"));
	PrintDebugUpdate(aFeedItem);


	{
		SwitchIndexL(EIdxFeedItemId);
		TDbSeekKey k( aFeedItem->iUuid() );
		if (iTable.SeekL(k)) {
			User::Leave(KErrAlreadyExists);
		}

	}

#ifdef EXPLICIT_TRANSACTIONS
	if (! iDb.Db().InTransaction() ) BeginL();
	TTransactionHolder th(*this);
#endif

	TRAPD(err, DoAddLocalL(aFeedItem, EFalse));
	if (err==KErrCorrupt) {
		iDb.MarkCorrupt();
		Corrupt(_L("Feed db corrupt")).ErrorCode(KFeedStorageCorrupt).Raise();
	}
	User::LeaveIfError(err);

	Async();
}

void CFeedItemStorageImpl::SwitchIndexL(TInt aIndex)
{
	TRAPD(err, MDBStore::SwitchIndexL(aIndex));
	if (err==KErrCorrupt) {
		iDb.MarkCorrupt();
		Corrupt(_L("Feed db corrupt")).ErrorCode(KFeedStorageCorrupt).Raise();
	}
	User::LeaveIfError(err);
}


TBool CFeedItemStorageImpl::IsUnread()
{
	if (iTable.ColInt32(EUnread)<=0) return EFalse;
	
	TUint dbid=iTable.ColUint32(EDbId);
	
	if (dbid < iAuthorCounts->IsUnreadUntilId(iTable.ColUint32(EAuthorId))) return EFalse;
	if (dbid < iAuthorCounts->IsUnreadUntilId(KAllCountNick)) return EFalse;
	
	return ETrue;
}

CBBFeedItem::TMediaDownloadState CFeedItemStorageImpl::MediaDownloadState(CBBFeedItem* aItem)
{
#ifdef __JAIKU_PHOTO_DOWNLOAD__
	if (aItem->iMediaDownloadState()==CBBFeedItem::EQueued) {
		TInt64 dl_req=MAKE_TINT64(0, aItem->iLocalDatabaseId);
		CDownloader::TDownloadable t=iDownloader->IsDownloadable(dl_req);
		if (t==CDownloader::ENotQueued) {
			return CBBFeedItem::ENotDownloading;
		} else if (t==CDownloader::ENotDownloadable) {
			return CBBFeedItem::EDownloadPausedOffline;
		} else {
			return CBBFeedItem::EQueued;
		}
	} else {
		return (CBBFeedItem::TMediaDownloadState)aItem->iMediaDownloadState();
	}
#else
	return CBBFeedItem::ENoMedia;
#endif
}

void CFeedItemStorageImpl::DoAddLocalL(CBBFeedItem* aFeedItem, TBool aReplace)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("DoAddLocalL"));

	if (!aFeedItem) 
		User::Leave(KErrArgument);

	if (aFeedItem->iUuid().Length() != aFeedItem->iUuid().MaxLength()) {
		User::Leave(KErrArgument);
	}
	
	TBool hasRemoteMedia = aFeedItem->iThumbnailUrl().Length() > 0;
	TBool hasLocalMedia = aFeedItem->iMediaFileName().Length() > 0;
	
	if (hasLocalMedia) {
		aFeedItem->iMediaDownloadState()=CBBFeedItem::EMediaDownloaded;
	} else if (hasRemoteMedia) {
		aFeedItem->iMediaDownloadState()=CBBFeedItem::ENotDownloading ;
	} else {
		aFeedItem->iMediaDownloadState()=CBBFeedItem::ENoMedia;
	}

	TInt cc, uc, gc;
	ReadChildCountsL(aFeedItem->iUuid(), cc, uc, gc);
	TUint authorid;
	
	InsertL();

	{
		if (aFeedItem->iParentUuid() != aFeedItem->iUuid()) {
			iTable.SetColL(EParentId, aFeedItem->iParentUuid());
		}
		if (! aFeedItem->iIsGroupChild() && aFeedItem->iParentUuid().Length()==0) {
			iTable.SetColL(ECorrelation, aFeedItem->iCorrelation());
		}
		iTable.SetColL(EFeedItemId, aFeedItem->iUuid());
		iTable.SetColL(EDatetime, aFeedItem->iCreated());
		iTable.SetColL(EUpdated, aFeedItem->iCreated());
		iTable.SetColL(EHidden, EFalse);

		TTime t; t=GetTime();
		iTable.SetColL(ELastAccess, t);

		iLCAuthor=aFeedItem->iAuthorNick();
		iLCAuthor.LowerCase();
		authorid=iAuthorCounts->GetIdByNickL(iLCAuthor);
		aFeedItem->iAuthorNick()=iLCAuthor;
		
		iTable.SetColL(EAuthorId, authorid);
		iTable.SetColL(EUnread, aFeedItem->iIsUnread() ? 1 : 0);
		iTable.SetColL(EChildCount, cc);
		iTable.SetColL(EUnreadChildren, uc);
		iTable.SetColL(EGroupChild, aFeedItem->iIsGroupChild());
		iTable.SetColL(EGroupChildCount, gc);
		iTable.SetColL(EShowInOverView, (!aFeedItem->iDontShowInOverView && ! aFeedItem->iIsGroupChild()) ? 1 : 0);
		iTable.SetColL(EMediaDownloadState, aFeedItem->iMediaDownloadState());
		{
			iTable.SetColNullL(EContents);
			RADbColWriteStream w; w.OpenLA(iTable, EContents);
			aFeedItem->ExternalizeL(w);
			w.CommitL();
		}
	}

	{
		TUint newid=aFeedItem->iLocalDatabaseId=iTable.ColUint(EDbId);
		PutL();

		if (!aReplace) {
			iDbIdToObjectMap->AddDataL(newid, aFeedItem);
			aFeedItem->SetDeletionNotify(this);
		}

	}
	aFeedItem->AddRef();
	refcounted_ptr<CBBFeedItem> p(aFeedItem);

	{
		NotifyObservers(aFeedItem, MFeedNotify::EAdded);
	}

	TBool groupchild=aFeedItem->iIsGroupChild();
	{
		UpdateReadCounters(aFeedItem->iParentUuid(), authorid,
			aFeedItem->iIsUnread() ? 1 : 0, groupchild);			
	}
	{
		UpdateChildCounters(aFeedItem->iParentUuid(), 
			authorid,
			1, groupchild, cc==0 );
	}
	if (! aFeedItem->iDontShowInOverView) {
		TInt ov_cc=1, ov_uc=0;
		if (aFeedItem->iIsUnread()) ov_uc=1;
		iAuthorCounts->UpdateStatsL(KOverviewCountNick(), ov_cc, ov_uc);
	}
}

void CFeedItemStorageImpl::RemoveFeedItemL(const CBBFeedItem * aFeedItem, TBool aMediaRemoveFile)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("RemoveFeedItemL"));

#ifdef EXPLICIT_TRANSACTIONS
	if (! iDb.Db().InTransaction()) BeginL();
	TTransactionHolder th(*this);
#endif

	DoRemoveFeedItemL(aFeedItem, aMediaRemoveFile);

	Async();
}

TBool CFeedItemStorageImpl::NotInOverView()
{
	if (iTable.ColInt32(EGroupChild)) return ETrue;
	if (iTable.IsColNull(EShowInOverView) || iTable.ColInt32(EShowInOverView)==1) return EFalse;
	return ETrue;
}

void CFeedItemStorageImpl::DoRemoveFeedItemL(const CBBFeedItem * aFeedItem, TBool aMediaRemoveFile)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("DoRemoveFeedItemL"));

	if ( aFeedItem->iUuid().Length() != KGlobalIdLength ) User::Leave(KErrArgument);
	
	SeekToFeedItemIdL(aFeedItem->iUuid());
	if (!iFound)  User::Leave(KErrNotFound);
	iTable.GetL();
	TInt unread=iTable.ColInt32(EUnread) > 0 ? 1 : 0;
	TBool groupchild=aFeedItem->iIsGroupChild();
	TUint authorid=iTable.ColUint32(EAuthorId);
	if (unread!=0) {
		UpdateReadCounters(aFeedItem->iParentUuid(), authorid, -unread, groupchild);
	}
	UpdateChildCounters(aFeedItem->iParentUuid(), 
		authorid,
		-1, groupchild);
	if (aMediaRemoveFile) 
		BaflUtils::DeleteFile(Fs(), aFeedItem->iMediaFileName(), 0);
	if (! aFeedItem->iDontShowInOverView) {
		TInt cc=-1, uc=0;
		if (unread) uc=-1;
		iAuthorCounts->UpdateStatsL(KOverviewCountNick, cc, uc);
	}

	SeekToFeedItemIdL(aFeedItem->iUuid());
	if (!iFound)  User::Leave(KErrNotFound);
	DeleteL(); // not MDBStore::DeleteL because of the transaction around this

	NotifyObservers( (CBBFeedItem*)aFeedItem, MFeedNotify::EFeedItemDeleted);
}

TBool CFeedItemStorageImpl::RemoveFeedItemsL(TTime aFromTime,
				    TFromType aFromType, TInt aMinimumToLeaveByAuthor,
				    TInt maxdeletions)
{
	return RemoveFeedItemsL(aFromTime, aFromType, aMinimumToLeaveByAuthor, maxdeletions, EFalse);
}

TBool CFeedItemStorageImpl::RemoveFeedItemsL(TTime aFromTime,
				    TFromType aFromType, TInt aMinimumToLeaveByAuthor,
				    TInt maxdeletions, TBool aContinue)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("RemoveFeedItemsL"));

	// This was commented out already before implicit transaction change
// #ifdef EXPLICIT_TRANSACTIONS
// 	//if (! iDb.Db().InTransaction()) BeginL();
// 	//TTransactionHolder th(*this);
	//CommitL();
// #endif

	if (iInPurge && CompactIfCloseL()) {
		return ETrue;
	}
	if (aMinimumToLeaveByAuthor>0) {
		SwitchIndexL(EIdxAuthorDatetime);
	} else {
		if (aFromType==EByDateTime) {
			SwitchIndexL(EIdxDatetime);
		} else {
			User::Leave(KErrNotSupported);
		}
	}
	if (!aContinue) iInPurge=EFalse;
	if (!iInPurge) {
		if (aMinimumToLeaveByAuthor>0) {
			if (! iTable.LastL()) return EFalse;
		} else {
			if (! iTable.FirstL()) return EFalse;
		}
	}

	TInt deleted=0;
	
	if (!iInPurge) {
		iCurrentAuthorId=0; iCurrentAuthorCount=0; 
		iCurrentAuthorUnreadChange=0; iCurrentAuthorChildChange=0;
		iCurrentAuthorHaveSeenAJaiku=EFalse;
	} else {
		iTable.GotoL(iRow);
	}
	iInPurge=EFalse;
	
	TInt ov_cc=0, ov_uc=0;
	
	for (;;) {
		TInt unread=0;
		iTable.GetL();

		refcounted_ptr<CBBFeedItem> p(0);
		if (aMinimumToLeaveByAuthor==0) {
			if (iTable.ColDes8(EFeedItemId).Length()==0) {
				if (! iTable.NextL() ) break;
				continue;
			}
			if (aFromType==EByDateTime && iTable.ColTime(EDatetime) > aFromTime) break;
			if (aFromType==EByAccess && (
				iTable.IsColNull(ELastAccess) ||
				iTable.ColTime(ELastAccess) > aFromTime) ) break;
		} else  {
			if (iTable.ColDes8(EFeedItemId).Length()==0) {
				if (! iTable.PreviousL() ) break;
				continue;
			}
			if (iTable.ColUint32(EAuthorId) != iCurrentAuthorId) {
				if (iCurrentAuthorUnreadChange!=0 || iCurrentAuthorChildChange!=0) {
					TBuf<50> author;
					iAuthorCounts->UpdateStatsL(iCurrentAuthorId, 
						iCurrentAuthorChildChange, iCurrentAuthorUnreadChange,
						author);
					NotifyObservers(author, iCurrentAuthorChildChange, iCurrentAuthorUnreadChange);
				}
				iCurrentAuthorChildChange=iCurrentAuthorUnreadChange=0;				
				iCurrentAuthorId=iTable.ColUint32(EAuthorId);
				iCurrentAuthorCount=0;
				iCurrentAuthorHaveSeenAJaiku=EFalse;
			}
			if (!iCurrentAuthorHaveSeenAJaiku && !iTable.IsColNull(EContents) && iTable.ColDes8(EParentId).Length()==0 && iTable.ColInt(EGroupChild)==0) {
				TUint dbid=iTable.ColUint32(EDbId);
				p.reset((CBBFeedItem*)iDbIdToObjectMap->GetData(dbid));
				if (! p.get() ) {
					p.reset(new (ELeave) CBBFeedItem);
					RADbColReadStream s; s.OpenLA(iTable, EContents);
					p->InternalizeL(s);
				} else {
					p->AddRef();
				}
			}
			if (p.get() && FeedItem::IsJaiku(*p)) {
				iCurrentAuthorCount++;
				iCurrentAuthorHaveSeenAJaiku=ETrue;
				if (! iTable.PreviousL() ) break;
				continue;
			}
			if (! iTable.ColInt(EGroupChild)) iCurrentAuthorCount++;
			if (iCurrentAuthorCount <= aMinimumToLeaveByAuthor || iTable.ColTime(EDatetime) > aFromTime) {
				if (! iTable.PreviousL() ) break;
				continue;
			}
		}
		if (! p.get() && ! iTable.IsColNull(EContents) ) {			
			TUint dbid=iTable.ColUint32(EDbId);
			p.reset((CBBFeedItem*)iDbIdToObjectMap->GetData(dbid));
			if (! p.get() ) {
				p.reset(new (ELeave) CBBFeedItem);
				RADbColReadStream s; s.OpenLA(iTable, EContents);
				p->InternalizeL(s);
			} else {
				p->AddRef();
			}
		}
		if (p.get() && FeedItem::IsJaiku(*p) && !iCurrentAuthorHaveSeenAJaiku) {
			iCurrentAuthorHaveSeenAJaiku=ETrue;
			if (! iTable.PreviousL() ) break;
			continue;
		}

		TBool groupchild;
		TUint a;
		{
			TUint dbid=iTable.ColUint32(EDbId);
			groupchild=iTable.ColInt(EGroupChild);
			iRow=iTable.Bookmark();
			unread=IsUnread();
			a=iTable.ColUint32(EAuthorId);
			if (!groupchild) {
				if (iTable.IsColNull(EShowInOverView) || iTable.ColInt32(EShowInOverView)==1) {
					ov_cc--;
					if (unread) ov_uc--;
				}
			}
		}
		{
			TGlobalId parent_id=iTable.ColDes8(EParentId);
			UpdateReadCounters(parent_id, 0, -unread, groupchild);
			UpdateChildCounters(parent_id, 0, -1, groupchild);
		}
		if (!groupchild) {
			TInt u=0;
			if (unread) u=-1;
			TInt itemc=-1;
			TBuf<50> an;
			if (aMinimumToLeaveByAuthor==0) {
				iAuthorCounts->UpdateStatsL(a, itemc, u, an);
				NotifyObservers(an, itemc, u);
			} else {
				iCurrentAuthorChildChange+=itemc;
				iCurrentAuthorUnreadChange+=u;
			}
		}
		iTable.GotoL(iRow);
		iTable.GetL();
		if (p.get()) {
			TInt err=Fs().Delete( p->iMediaFileName() );
			if (err!=KErrNone && err!=KErrBadName && err!=KErrNotFound && err!=KErrPathNotFound) {
				User::Leave(err);
			}
			{
				NotifyObservers(p.get(), MFeedNotify::EFeedItemDeleted);
			}
			p.reset();
		}
		if (aMinimumToLeaveByAuthor>0) {
			SwitchIndexL(EIdxAuthorDatetime);
		} else {
			if (aFromType==EByDateTime) {
				SwitchIndexL(EIdxDatetime);
			} else {
				User::Leave(KErrNotSupported);
			}
		}
		{
			iTable.GotoL(iRow);
			DeleteL();
			//iTable.DeleteL();
			deleted++;
		}
		if (aMinimumToLeaveByAuthor==0) {
			if (! iTable.NextL() ) break;
		} else {
			if (! iTable.PreviousL() ) break;
		}
		if (maxdeletions>0 && deleted>maxdeletions) {
			iRow=iTable.Bookmark();
			iInPurge=ETrue;
			iPurgeTimer->Wait(10);
			break;
		}
	}
	iAuthorCounts->UpdateStatsL(KOverviewCountNick, ov_cc, ov_uc);
	
	if (iCurrentAuthorUnreadChange!=0 || iCurrentAuthorChildChange!=0) {
		TBuf<50> an;
		iAuthorCounts->UpdateStatsL(iCurrentAuthorId, iCurrentAuthorChildChange, iCurrentAuthorUnreadChange, an);
		NotifyObservers(an, iCurrentAuthorChildChange, iCurrentAuthorUnreadChange);
		iCurrentAuthorUnreadChange=0; iCurrentAuthorChildChange=0;
	}
	return deleted>0;
}

TBool CFeedItemStorageImpl::FirstAllL(TBool aAscDate, TBool aOverViewOnly)
{
#ifdef __WINS__
	// debug aid
	if (iInMarkAuthor) {
		User::Panic(_L("iteraring in callback"), 1);		
	}
#endif
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("FirstAllL"));
	if (!iHasTransactionHolder) CommitL();
	iFound=EFalse;
	iSeekBy=ESeekAll;
	iAscDate=aAscDate;
	iOverViewOnly=aOverViewOnly;
	SwitchIndexL(EIdxDatetime);
	
	TBool found;
	if (!iAscDate) {
		found=iTable.LastL();
		while (found) {
			iTable.GetL();
			if (iTable.ColDes8(EFeedItemId).Length()==0) return EFalse;
			if (iOverViewOnly && NotInOverView()) {
				found=iTable.PreviousL();
			} else {
				break;
			}
		}
	} else {
		iTable.FirstL(); // first is root with TTime(0)
		found=iTable.NextL();
		while (found) {
			iTable.GetL();
			if (iOverViewOnly && NotInOverView()) {
				found=iTable.NextL();
			} else {
				break;
			}
		}
	}
	return iFound=found;
}

void CFeedItemStorageImpl::ReadChildCountsL(const TGlobalId& aParent, TInt& aChildCount, TInt& aUnreadChildCount,
	TInt& aGroupChildCount)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("ReadChildCountsL"));
	aGroupChildCount=aUnreadChildCount=aChildCount=0;
	TBool found=FirstByParentL(aParent);
	while (found) {
		iTable.GetL();
		if (iTable.ColInt(EGroupChild)) {
			aGroupChildCount++;
		} else {
			aChildCount++;
			if (IsUnread()) {
				aUnreadChildCount++;
			}
		}
		found=NextL();
	}
}

void CFeedItemStorageImpl::GetChildCountsL(const TGlobalId& aParent, TInt& aChildCount, TInt& aUnreadChildCount,
	TInt& aGroupChildCount, TBool aOverViewOnly)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetChildCountsL"));
	aGroupChildCount=aUnreadChildCount=aChildCount=0;
	
	if (aOverViewOnly) {
		if (aParent.Length()!=0) User::Leave(KErrNotSupported);
		iAuthorCounts->GetStatsL(KOverviewCountNick, aChildCount, aUnreadChildCount);
		aGroupChildCount=0;
		return;
	}
	if (SeekToFeedItemIdL(aParent)) {
		iTable.GetL();
		aChildCount=iTable.ColInt(EChildCount);
		aUnreadChildCount=iTable.ColInt(EUnreadChildren);
		aGroupChildCount=iTable.ColInt(EGroupChildCount);
	}
}

// iteration
TBool CFeedItemStorageImpl::FirstByParentL(const TGlobalId& aParentId, TBool aAscDate)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("FirstByParentL"));
#ifdef __WINS__
	// debug aid
	if (iInMarkAuthor) {
		User::Panic(_L("iteraring in callback"), 1);		
	}
#endif
	if (!iHasTransactionHolder) CommitL();

	if (aParentId.Length()!=KGlobalIdLength) User::Leave(KErrArgument);
	
	iSeekParentId=aParentId;
	//TODO
	iSeekOnlyUnread=EFalse;
	//iSeekOnlyVisible=aOnlyVisible;
	iSeekBy=ESeekParent;
	iAscDate=aAscDate;
	iOverViewOnly=EFalse;
	TBool& ret=iFound;
	ret=EFalse;

	SwitchIndexL(EIdxParentDatetime);
	TDbSeekMultiKey<2> rk;
	rk.Add(aParentId);
	
	if (!iAscDate) {
		rk.Add(Time::MaxTTime());
		ret=iTable.SeekL(rk, RDbTable::ELessThan);
	} else {
		rk.Add(TTime(0));
		ret=iTable.SeekL(rk, RDbTable::EGreaterThan);
	}
	if (!ret) return ret;

	if (iSeekOnlyUnread) {
		TInt unread=0;
		TGlobalId parent;
		for(;;) {
			iTable.GetL();
			unread=IsUnread();
			parent=iTable.ColDes8(EParentId);
			if (parent!=aParentId) return ret=EFalse;
			
			if (unread>0) return ret=ETrue;
			ret=iTable.PreviousL();
			if (!ret) return ret;
		}
	} else {
		TGlobalId parent;
		iTable.GetL();
		parent=iTable.ColDes8(EParentId);
		if (parent!=aParentId) return ret=EFalse;
	}
	return ret=ETrue;
}

TBool CFeedItemStorageImpl::FirstByAuthorL(const TDesC& aAuthorIn, TBool aAscDate)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("FirstByAuthorL"));
#ifdef __WINS__
	// debug aid
	if (iInMarkAuthor) {
		User::Panic(_L("iteraring in callback"), 1);		
	}
#endif
	if (!iHasTransactionHolder) CommitL();
	iLCAuthor=aAuthorIn; iLCAuthor.LowerCase();
	iSeekAuthorId=iAuthorCounts->GetIdByNickL(iLCAuthor);

	//TODO
	iSeekOnlyUnread=EFalse;
	//iSeekOnlyVisible=aOnlyVisible;
	iSeekBy=ESeekAuthor;
	iAscDate=aAscDate;
	iOverViewOnly=EFalse;
	TBool& ret=iFound;
	ret=EFalse;

	SwitchIndexL(EIdxAuthorDatetime);
	TDbSeekMultiKey<2> rk;
	rk.Add(iSeekAuthorId);
	
	if (!iAscDate) {
		rk.Add(Time::MaxTTime());
		ret=iTable.SeekL(rk, RDbTable::ELessThan);
	} else {
		rk.Add(TTime(0));
		ret=iTable.SeekL(rk, RDbTable::EGreaterThan);
	}
	if (!ret) return ret;

	if (iSeekOnlyUnread) {
		TInt unread=0;
		TUint author;
		for(;;) {
			iTable.GetL();
			unread=IsUnread();
			author=iTable.ColUint32(EAuthorId);
			if (author!=iSeekAuthorId) return ret=EFalse;
			
			if (unread>0) return ret=ETrue;
			ret=iTable.PreviousL();
			if (!ret) return ret;
		}
	} else {
		TUint author;
		iTable.GetL();
		author=iTable.ColUint32(EAuthorId);
		if (author!=iSeekAuthorId) return ret=EFalse;
	}
	return ret=ETrue;
}

TBookmark* CFeedItemStorageImpl::Bookmark()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("Bookmark"));
	TBookmark* bm=new (ELeave) TBookmark;
	bm->iBookmark=iTable.Bookmark();
	if (iSeekBy==ESeekParent) {
		bm->iParent=iSeekParentId;
	} else {
		bm->iAuthorId=iSeekAuthorId;
	}
	bm->iOrder=iSeekBy;
	bm->iFound=iFound;
	bm->iAscDate=iAscDate;
	bm->iOverViewOnly=iOverViewOnly;
	
	return bm;
}

TBool CFeedItemStorageImpl::NextL(TBookmark*& aBookmark)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("NextL"));
	if (!aBookmark) return NextL();
	
	iSeekBy=(TSeekBy)aBookmark->iOrder;
	iAscDate=aBookmark->iAscDate;
	iOverViewOnly=aBookmark->iOverViewOnly;
	if (iSeekBy==ESeekParent) {
#ifdef __WINS__
		// debug aid
		if (iInMarkAuthor) {
			User::Panic(_L("iteraring in callback"), 1);		
		}
#endif
		iSeekParentId=aBookmark->iParent;
		SwitchIndexL(EIdxParentDatetime);
	} else if(iSeekBy==ESeekAll) {
#ifdef __WINS__
		// debug aid
		if (iInMarkAuthor) {
			User::Panic(_L("iteraring in callback"), 1);		
		}
#endif
		SwitchIndexL(EIdxDatetime);
	} else {
		SwitchIndexL(EIdxAuthorDatetime);
		iSeekAuthorId=aBookmark->iAuthorId;
	}
	iFound=aBookmark->iFound;
	iTable.GotoL(aBookmark->iBookmark);
	TBool ret=EFalse;
	ret=NextL();
	delete aBookmark; aBookmark=0;
	return ret;
}

void CFeedItemStorageImpl::ReleaseBookmark(TBookmark*& aBookmark)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("ReleaseBookmark"));
	delete aBookmark; aBookmark=0;
}

TBool CFeedItemStorageImpl::NextL()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("NextL"));

	TBool& ret=iFound;
	if (!ret) return EFalse;

	if (iSeekBy==ESeekAll) {
		do {
			if (!iAscDate) ret=iTable.PreviousL();
			else ret=iTable.NextL();
			if (ret) {
				iTable.GetL();
				if (iTable.ColDes8(EFeedItemId).Length()==0) {
					ret=EFalse;
					break;
				}
				if (iOverViewOnly && NotInOverView()) {
					;
				} else {
					break;
				}
			}
		} while (ret);
		return ret;
	} else if (iSeekBy==ESeekParent) {
		TGlobalId parent; TInt unread; TBool hidden;
		for(;;) {
			if (!iAscDate) ret=iTable.PreviousL();
			else ret=iTable.NextL();
			if (!ret) return ret;
			iTable.GetL();
			parent=iTable.ColDes8(EParentId);
			if (parent!=iSeekParentId) return EFalse;
			if (iSeekOnlyUnread) {
				unread=IsUnread();
				if (unread) return ETrue;
			} else	{
				return ETrue;
			}
		}
	} else {
		TUint author; TInt unread; TBool hidden;
		for(;;) {
			if (!iAscDate) ret=iTable.PreviousL();
			else ret=iTable.NextL();
			if (!ret) return ret;
			iTable.GetL();
			author=iTable.ColUint32(EAuthorId);
			if (author!=iSeekAuthorId) return EFalse;
			if (iSeekOnlyUnread) {
				unread=IsUnread();
				if (unread) return ETrue;
			} else	{
				return ETrue;
			}
		}
	}
}

TUint CFeedItemStorageImpl::GetCurrentLocalIdL()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetCurrentLocalIdL"));

	if (!iFound) User::Leave(KErrNotFound);

	iTable.GetL();
	return iTable.ColUint32(EDbId);
}

TBool CFeedItemStorageImpl::CurrentIsGroupChild()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("CurrentIsGroupChild"));
	if (!iFound) User::Leave(KErrNotFound);

	iTable.GetL();
	return iTable.ColInt(EGroupChild);
}

void CFeedItemStorageImpl::DownloadMediaForFeedItemL(CBBFeedItem* aFeedItem, TBool aManual)
{
#ifdef __JAIKU_PHOTO_DOWNLOAD__
	if (aFeedItem->iLocalDatabaseId==0) {
		Bug(_L("Trying to to download media for a feeditem not in the database")).Raise();
	}
	TUint dbid=aFeedItem->iLocalDatabaseId;
	
	TInt64 dl_req=MAKE_TINT64(0, dbid);
	
	if (aFeedItem->iThumbnailUrl().Length()>0 && aFeedItem->iMediaFileName().Length()==0) {
		iDownloader->DownloadL(dl_req, aFeedItem->iThumbnailUrl(),
			aManual ? EDlLevelOnRequest : -1);
		aFeedItem->iMediaDownloadState()=CBBFeedItem::EQueued;
		UpdateDownloadStateL(aFeedItem);
		
	}
#endif // __JAIKU_PHOTO_DOWNLOAD__
}

void CFeedItemStorageImpl::SetDownloadModeL( CFeedItemStorage::TDownloadMode aMode )
{
#ifdef __JAIKU_PHOTO_DOWNLOAD__	
	if (aMode==EUnknownDL) User::Leave(KErrArgument);
	TInt8 level;
	if (aMode==EAutomaticDL) level=EDlLevelAutomatic;
	else if (aMode==EOnLookDL) level=EDlLevelOnLook;
	else level=EDlLevelOnRequest;
	
	iDownloader->SetLevelL(level);
#endif
}

CFeedItemStorage::TDownloadMode CFeedItemStorageImpl::DownloadMode() const
{
#ifdef __JAIKU_PHOTO_DOWNLOAD__	
	TInt level=iDownloader->GetLevelL();
	if (level<EDlLevelOnLook) return EAutomaticDL;
	if (level<EDlLevelOnRequest) return EOnLookDL;
	return EOnRequestDL;
#else
	return EUnknownDL;
#endif
}

void CFeedItemStorageImpl::SetOfflineL(TBool aIsOffline)
{
#ifdef __JAIKU_PHOTO_DOWNLOAD__	
	if (aIsOffline) {
		iDownloader->SetDownloadLimitLevelL(EDlLevelOnRequest);
	} else {
		iDownloader->SetNoDownloadLimitLevelL();
	}
	auto_ptr<MGenericIntMapIterator> iter(iDbIdToObjectMap->CreateIterator());
	TUint32 key; TAny* ptr;
	while (iter->NextL(key, ptr)) {
		TBool changed=EFalse;
		CBBFeedItem* i=(CBBFeedItem*)ptr;
		if (aIsOffline) {
			if (i->iMediaDownloadState()==CBBFeedItem::EQueued) {
				i->iMediaDownloadState()=CBBFeedItem::EDownloadPausedOffline;
				changed=ETrue;
			}
		} else {
			if (i->iMediaDownloadState()==CBBFeedItem::EDownloadPausedOffline) {
				i->iMediaDownloadState()=CBBFeedItem::EQueued;
				changed=ETrue;
			}
		}
		if (changed) NotifyObservers(i, MFeedNotify::EMediaDownloadStateChanged);
	}
#endif
}

TBool CFeedItemStorageImpl::IsDownloading( PresencePublisher::EState& aReason)
{
#ifdef __JAIKU_PHOTO_DOWNLOAD__	
	if (iConnecting) return ETrue;
	
	if (iOfflineMode) aReason=PresencePublisher::EOffline;
	else if (iCallInProgress) aReason=PresencePublisher::ECallInProgress;
	else if (iLowSignal) aReason=PresencePublisher::ELowSignal;
	
	return EFalse;
#else
	return EFalse;
#endif
}


// accessing
CBBFeedItem* CFeedItemStorageImpl::GetCurrentL(TBool aIgnoreNotFound) // addrefs
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetCurrentL"));
	if (!iHasTransactionHolder) CommitL();

	if (!iFound) 
		{
		if ( ! aIgnoreNotFound ) 
			User::Leave(KErrNotFound);
		return NULL;
		}

// 	TTime t; t=GetTime();
// 	if (!iHasTransactionHolder) {
// 		TAutomaticTransactionHolder th(*this);
// 		UpdateL();
// 		iTable.SetColL(ELastAccess, t);
// 		PutL();
// 	} else {
// 		UpdateL();
// 		iTable.SetColL(ELastAccess, t);
// 		PutL();
// 	}
	iTable.GetL();
	TUint dbid=iTable.ColUint32(EDbId);
	
	TInt64 dl_req=MAKE_TINT64(0, dbid);
	
	CBBFeedItem* existing=(CBBFeedItem*)iDbIdToObjectMap->GetData(dbid);
	if (existing) {
		existing->AddRef();
		return existing;
	}

	CBBFeedItem* n=new (ELeave) CBBFeedItem; // addrefs
	refcounted_ptr<CBBFeedItem> p(n);
	{
		RADbColReadStream s; s.OpenLA(iTable, EContents);
		p->InternalizeL(s);
	}
	p->iParentUuid()=iTable.ColDes8(EParentId);
	p->iLocalDatabaseId=iTable.ColUint32(EDbId);
	p->iIsUnread()=IsUnread();
	p->iUnreadChildCounter=iTable.ColInt32(EUnreadChildren);
	p->iChildCount=iTable.ColInt32(EChildCount);
	p->iGroupChildCount=iTable.ColInt32(EGroupChildCount);
	p->iMediaDownloadState()=iTable.ColInt32(EMediaDownloadState);
	p->iMediaDownloadState()=MediaDownloadState(p.get());
	
	TBool show_in_overview=ETrue;
	if (! iTable.IsColNull(EShowInOverView) && iTable.ColInt32(EShowInOverView)==0) {
		show_in_overview=EFalse;
	}
	if (p->iIsGroupChild()) show_in_overview=EFalse;
	p->iDontShowInOverView=!show_in_overview;

	if (! iTable.IsColNull(EErrorInfo) ) {
		RADbColReadStream s; s.OpenLA(iTable, EErrorInfo);
		refcounted_ptr<CBBErrorInfo> info(CBBErrorInfo::NewL(BBDataFactory()));
		CC_TRAPD(err, info->InternalizeL(s));
		if (err!=KErrNone) {
			info->iUserMsg->Append(_L("Errors exist, but cannot retrieve fully"));
			if (info->iErrorCode.iCode()==0)
				info->iErrorCode=MakeErrorCode(0, err);
		}
		p->iErrorInfo=info.release();
	}

	CC_TRAPD(err, iDbIdToObjectMap->AddDataL(dbid, p.get()));

	p->SetDeletionNotify(this);
	
	return p.release();
}

void CFeedItemStorageImpl::SubscribeL(MFeedNotify* aNotify) // sets up for notification
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("SubscribeL"));

	iSubscribers->AppendL(aNotify);
}

void CFeedItemStorageImpl::UnSubscribeL(MFeedNotify* aNotify) // sets up for notification
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UnSubscribeL"));

	CList<MFeedNotify*>::Node* i=0;
	for (i=iSubscribers->iFirst; i; i=i->Next) {
		if (i->Item==aNotify) {
			iSubscribers->DeleteNode(i, true);
			return;
		}
	}
}

CBBFeedItem* CFeedItemStorageImpl::GetByLocalIdL(TUint aIdx, TBool aIgnoreNotFound) // addrefs
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetByLocalIdL"));
	if (!iHasTransactionHolder) CommitL();

	SeekToDbIdL(aIdx);
	return GetCurrentL(aIgnoreNotFound);
}

CBBFeedItem* CFeedItemStorageImpl::GetByGlobalIdL(const TGlobalId& aFeedItemId, TBool aIgnoreNotFound) // addrefs
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetByGlobalIdL"));
	if (!iHasTransactionHolder) CommitL();

	SeekToFeedItemIdL(aFeedItemId);

	return GetCurrentL(aIgnoreNotFound);
}

TInt CFeedItemStorageImpl::TextLength(TInt aColumn) {
	if (aColumn==EParentId || aColumn==EFeedItemId) return 16;
	return 50;
}


_LIT(KClassName, "CFeedItemStorageImpl");

CFeedItemStorage::CFeedItemStorage() : CCheckedActive(EPriorityLow, KClassName) { }

CFeedItemStorageImpl::CFeedItemStorageImpl(MApp_context& Context, CDb& Db, MBBDataFactory* aBBFactory) 
  : MContextBase(Context),
	MDBStore(Db.Db()), iDb(Db), iBBFactory(aBBFactory)
{ }


const TInt KCurrentDbVersion=5;

void CFeedItemStorageImpl::ConstructL(
#ifdef __JAIKU_PHOTO_DOWNLOAD__
		HttpFactory aHttpFactory,
		const TDesC& aDownloadDbName
#endif
)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("ConstructL"));

	CActiveScheduler::Add(this);

	TInt cols[]= { 
		EDbColUint32,	// dbid
		EDbColText8,	// parentid
		EDbColText8,	// itemid
		EDbColDateTime,	// timestamp
		EDbColDateTime,	// last access

		EDbColUint32,	// author
		EDbColInt32,	// unread counter
		EDbColLongBinary, // icon
		EDbColLongBinary, // contents

		EDbColBit,      // hidden
		EDbColLongBinary, // error info
		EDbColDateTime, // updated stamp
		
		EDbColInt32,	// child count
		EDbColInt32,	// unread children
		EDbColBit,		// isGroupChild
		EDbColInt32,	// EGroupChildCount
		EDbColText8,	// ECorrelation
		EDbColBit,	// EShowInOverView
		EDbColInt32,	// EMediaDownloadState
		-1
	};
	TInt col_flags[]={ TDbCol::EAutoIncrement, 0, 0, 0, 0, 
		0, 0, 0, 0,
		0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0 };
	TInt idxs[]= { EDbId, -2, EDatetime, -2, EParentId, EDatetime, -2, EAuthorId, EDatetime, -2, EFeedItemId, -2, 
		EAuthorId, EDbId, -2, ECorrelation, -1 };

	_LIT(KFeedItems, "FEEDITEMS");
	MDBStore::ConstructL(cols, idxs, false, KFeedItems, ETrue, col_flags);

	iDbIdToObjectMap=CGenericIntMap::NewL();
	iSubscribers=CList<MFeedNotify*>::NewL();
	iAuthorCounts=new (ELeave) CAuthorStats(AppContext(), iDb);
	iAuthorCounts->ConstructL();
	
	TGlobalId root;
	TInt version=0;
	iLastPurge=TTime(0);
	if (! SeekToFeedItemIdL(root) ) {
		InsertL();
		iTable.SetColL(EFeedItemId, root);
		iTable.SetColL(EParentId, root);
		iTable.SetColL(EChildCount, 0);
		//iTable.SetColL(EGroupChildCount, gc); // not kept
		iTable.SetColL(EUnreadChildren, 0);
		iTable.SetColL(EDatetime, TTime(0));
		iTable.SetColL(ELastAccess, TTime(0));
		PutL();
		TInt cc=0, uc=0;
		iAuthorCounts->SetStatsL(KOverviewCountNick(), cc, uc);
	}
	
	SeekToFeedItemIdL(root);
	UpdateL();
	TBuf8<10> versionbuf; versionbuf.AppendNum(KCurrentDbVersion);
	iTable.SetColL(ECorrelation, versionbuf);
	iTable.SetColL(ELastAccess, iLastPurge);
	PutL();
	
	iInJaiku=ETrue; iIdle=EFalse;
	
	iBBSubSession=BBSession()->CreateSubSessionL(this);
	iBBSubSession->AddNotificationL(KIdleTuple);
	iBBSubSession->AddNotificationL(KCurrentAppTuple);
	iPurgeTimer=CTimeOut::NewL(*this, CActive::EPriorityIdle);
	
#ifdef __JAIKU_PHOTO_DOWNLOAD__

	if (aDownloadDbName.Length()>0) {
		iDownloadDb=CDb::NewL(AppContext(), aDownloadDbName, EFileWrite);
	} else {
		iDownloadDb=CDb::NewL(AppContext(), _L("FEED2_DL.db"), EFileWrite);
	}

	// Photo download directory 
 	auto_ptr<HBufC> dldir(HBufC::NewL(256));
 	// Commented out directory under data
 	
 	{
 		dldir->Des().Append(DataDir());
 		dldir->Des().Append(_L("Media"));
		iOldPurger=CPurger::NewL(*this, *dldir);
		iOldPurger->TriggerL(0);
		dldir->Des().Zero();
 	} 
// 	dldir->Des().Append(DataDir());
// 	dldir->Des().Append(_L("Media"));
	// We will use private directory for now
	TPtr des = dldir->Des();
	Fs().PrivatePath( des );
	
	iDownloader=CDownloader::NewL(AppContext(), iDownloadDb->Db(), *dldir, *this,
		_L("storage-dl"), aHttpFactory);
		
	iPurger=CPurger::NewL(*this, *dldir);
#endif
}

void CFeedItemStorageImpl::UpdateDownloadStateL(TInt64 aId, CBBFeedItem::TMediaDownloadState aState)
{
	TUint dbid=I64LOW(aId);
	TBool found=SeekToDbIdL(dbid);
	if (!found) {
		return;
	}
	refcounted_ptr<CBBFeedItem> p(0);
	p.reset((CBBFeedItem*)iDbIdToObjectMap->GetData(dbid));
	if (p.get()) {
		p->AddRef();
	} else {
		TAutomaticTransactionHolder th(*this);
		UpdateL();
		iTable.SetColL(EMediaDownloadState, aState);
		PutL();
		return;
	}
	p->iMediaDownloadState()=aState;
	UpdateDownloadStateL(p.get());
}

#ifdef __JAIKU_PHOTO_DOWNLOAD__
void CFeedItemStorageImpl::DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
    const TDesC& /*aContentType*/) 
{
	TUint dbid=I64LOW(aRequestId);
	TBool found=SeekToDbIdL(dbid);
	if (!found) {
		Fs().Delete(aFileName);
		return;
	}
	refcounted_ptr<CBBFeedItem> p(0);
	p.reset((CBBFeedItem*)iDbIdToObjectMap->GetData(dbid));
	if (p.get()) {
		p->AddRef();
		p->iMediaFileName()=aFileName;
		p->iMediaDownloadState()=CBBFeedItem::EMediaDownloaded;
		UpdateFeedItemL(p.get(), EFalse);
		NotifyObservers(p.get(), MFeedNotify::EMediaDownloadStateChanged);
	} else {
		p.reset(GetCurrentL());
		p->iMediaDownloadState()=CBBFeedItem::EMediaDownloaded;
		p->iMediaFileName()=aFileName;
		UpdateFeedItemL(p.get(), EFalse);
	}
}

void CFeedItemStorageImpl::DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr,
	TBool aWillRetry)
{
	TUint dbid=I64LOW(aRequestId);
	refcounted_ptr<CBBErrorInfo> ei(CBBErrorInfo::NewL(BBDataFactory(), EError, ERemote,
		MakeErrorCode(0, aCode)));
	
	//ei->iUserMsg->Append(_L("Download failed"));
	// 	AppendHttpErrorL(aCode, ei->iUserMsg->Value());
	ei->iUserMsg->Append(aDescr);
	ei->iTechnicalMsg->Append(aDescr);
	
	UpdateErrorInfo(dbid, ei.get(), aWillRetry);
}

void CFeedItemStorageImpl::DownloadStarted(TInt64 aRequestId)
{
	UpdateDownloadStateL(aRequestId, CBBFeedItem::EDownloading);
}

void CFeedItemStorageImpl::Dequeued(TInt64 aRequestId)
{
	UpdateDownloadStateL(aRequestId, CBBFeedItem::ENotDownloading);
}

void CFeedItemStorageImpl::ConnectivityChanged(TBool aConnecting, TBool aOfflineMode,
	TBool aLowSignal, TBool aCallInProgress)
{
	iOfflineMode=aOfflineMode;
	iLowSignal=aLowSignal;
	iCallInProgress=aCallInProgress;
	iConnecting=aConnecting;
	
	NotifyObservers(0, MFeedNotify::EMediaDownloadPauseChanged);
}

#endif 

void CFeedItemStorageImpl::NewValueL(TUint , const TTupleName& aName, const TDesC& , 
	const TComponentName& , const MBBData* aData)
{
	const CBBSensorEvent* ev=bb_cast<CBBSensorEvent>(aData);
	if ( aName == KCurrentAppTuple) {
		iInJaiku=ETrue;
		if (ev) {
			const TBBCurrentApp* app=bb_cast<TBBCurrentApp>(ev->iData());
			if (app && app->iUid()!=KUidContextContacts.iUid ) iInJaiku=EFalse;
		}
	}
	if ( aName == KIdleTuple ) {
		iIdle=EFalse;
		if (ev) {
			const TBBUserActive* act=bb_cast<TBBUserActive>(ev->iData());
			if (act && (! act->iActive())) iIdle=ETrue;
		}
	}
	if (iIdle || !iInJaiku) {
		if (! iPurgeTimer->IsActive() ) {
			TTime now=GetTime();
#ifndef __WINS__
			if (iLastPurge+TTimeIntervalHours(2) < now) {
#else
			if (iLastPurge+TTimeIntervalMinutes(2) < now) {
#endif
				Reporting().UserErrorLog(_L("Starting purge"));
				iPurgeTimer->Wait(1);
			}
		}
	} else {
		if ( iPurgeTimer->IsActive()) {
			Reporting().UserErrorLog(_L("Stopping purge"));
			iPurgeTimer->Reset();
		}
	}
}

void CFeedItemStorageImpl::DeletedL(const TTupleName& aName, const TDesC& )
{
	if ( aName == KCurrentAppTuple) iInJaiku=ETrue;
	if ( aName == KIdleTuple ) iIdle=EFalse;
	iPurgeTimer->Reset();
}

void CFeedItemStorageImpl::StopBackgroundActivities() 
{
	if (iPurgeTimer->IsActive()) {
		Reporting().UserErrorLog(_L("Pausing purge"));
		iPurgeTimer->Wait(5);
	}
}

void CFeedItemStorageImpl::expired(CBase*) {
	TTime limit=GetTime();
	limit-=TTimeIntervalDays(3);
	
	TBool cont;
	TRAPD(err, cont=RemoveFeedItemsL(limit, EByDateTime, 7, 5, ETrue));
	if (err==KErrCorrupt) {
		iDb.MarkCorrupt();
		Corrupt(_L("Feed db corrupt")).ErrorCode(KFeedStorageCorrupt).Raise();
	}
	
	User::LeaveIfError(err);
	
	if (cont) {
		Reporting().UserErrorLog(_L("Continuing purge"));
		//iPurgeTimer->WaitShort(10);
		iPurgeTimer->Wait(0);
	} else {
		iPurgeTimer->Reset();
		iLastPurge=GetTime();
		TGlobalId root;
		SeekToFeedItemIdL(root);
		UpdateL();
		iTable.SetColL(ELastAccess, iLastPurge);
		PutL();
		Reporting().UserErrorLog(_L("Done purge"));
#ifdef __JAIKU_PHOTO_DOWNLOAD__
#  ifndef __WINS__
		iPurger->TriggerL(1024);
#  else
		iPurger->TriggerL(100);
#  endif
#endif
	}
}

void CFeedItemStorageImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("CheckedRunL"));
	if (iDb.Db().InTransaction()) {
		//TAutoBusy busy(Reporting());
		iDb.CommitL();
	}
}

void CFeedItemStorageImpl::Async()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("Async"));
	iAsyncCount++;
	// force sync commit on large number of changes
	if (iAsyncCount>10) {
		if (iDb.Db().InTransaction()) {
			iDb.CommitL();
		}
		iAsyncCount=0;
	}
	if (IsActive()) return;
	TRequestStatus *s=&iStatus;
	User::RequestComplete(s, KErrNone);
	SetActive();
}

void CFeedItemStorageImpl::UpdateReadCounters(const TGlobalId& aParent, TUint aAuthor,
	TInt read_counter, TBool aIsGroupChild)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateReadCounters"));
	UpdateReadCounters( (const TDesC8&)aParent, aAuthor, read_counter, aIsGroupChild);
}

void CFeedItemStorageImpl::UpdateReadCounters(const TDesC8& aParent, TUint aAuthor,
	TInt read_counter, TBool aIsGroupChild)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateReadCounters"));

	if (read_counter==0) return;
	if (aIsGroupChild) return;
	
	if (aAuthor!=0) {
		TInt cc=0;
		TInt uc=read_counter;
		iAuthorCounts->UpdateStatsL(aAuthor, cc, uc, iLCAuthor);
		NotifyObservers(iLCAuthor, cc, uc);
	}
	if (! SeekToFeedItemIdL(aParent)) return;

	iTable.GetL();
	
	TUint dbid=iTable.ColUint32(EDbId);
	TInt count=iTable.ColInt32(EUnreadChildren);
	count+=read_counter;
	
	UpdateL();
	iTable.SetColL(EUnreadChildren, count);
	if (iTable.ColDes8(EFeedItemId).Length()>0) {
		iTable.SetColL(ELastAccess, GetTime());
	}
	PutL();
	CBBFeedItem* p=(CBBFeedItem*)iDbIdToObjectMap->GetData(dbid);
	if (p) {
		p->iUnreadChildCounter=count;
		NotifyObservers(p, MFeedNotify::EUnreadChanged);
	}
}

void CFeedItemStorageImpl::UpdateChildCounters(const TGlobalId& from_id, TUint aAuthor,
	TInt Child_counter, TBool aIsGroupChild, TBool aUpdateUp)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateChildCounters"));
	UpdateChildCounters( (const TDesC8&)from_id, aAuthor, Child_counter, aIsGroupChild, aUpdateUp);
}

void CFeedItemStorageImpl::UpdateChildCounters(const TDesC8& from_id, TUint aAuthor,
	TInt Child_counter, TBool aIsGroupChild, TBool aUpdateUp)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateChildCounters"));

	if (Child_counter==0) return;
	if (aAuthor!=0 && !aIsGroupChild) {
		TInt cc=Child_counter;
		TInt uc=0;
		iAuthorCounts->UpdateStatsL(aAuthor, cc, uc, iLCAuthor);
		NotifyObservers(iLCAuthor, cc, uc);
	}

	TColumns count_col=EChildCount;
	if (aIsGroupChild) count_col=EGroupChildCount;
	TGlobalId parent_id=from_id;
	TBool root=EFalse;
	TBool updateNextParent = ETrue;
	for (;;) {
		if (root) break;
		if (! SeekToFeedItemIdL(parent_id) ) {
			TGlobalId rootid;
			SeekToFeedItemIdL(rootid);
		}
		iTable.GetL();
		TUint dbid=iTable.ColUint32(EDbId);
		parent_id=iTable.ColDes8(EParentId);
		TInt count=iTable.ColInt32(count_col)+Child_counter;
		if (count<0) {
			Child_counter-=count; // ??? what the hell ??? 
			count=0;
		}
			
		if (iTable.ColDes8(EFeedItemId).Length()==0) root=ETrue;
		
		if ( updateNextParent || root ) // always update root
			{
				UpdateL();
				iTable.SetColL(count_col, count);
		
				if (!root) iTable.SetColL(ELastAccess, GetTime());
				PutL();
				CBBFeedItem* p=(CBBFeedItem*)iDbIdToObjectMap->GetData(dbid);
				if (p) {
					p->iChildCount=iTable.ColInt32(EChildCount);
					p->iGroupChildCount=iTable.ColInt32(EGroupChildCount);
					NotifyObservers(p, MFeedNotify::EChildCountChanged);
				}
			}
		// only update first parent for normal childs (=comments)
		updateNextParent = count_col != EChildCount;
	}
}


void CFeedItemStorageImpl::MarkAsRead(const TDesC& aAuthor)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("MarkAsRead-author"));
	
	TDbSeekMultiKey<2> k;
	TUint32 authorid=iAuthorCounts->GetIdByNickL(aAuthor);
	k.Add( (TUint)(authorid+1));
	k.Add((TUint)0);
	SwitchIndexL(EIdxAuthorDbId);
	TBool found=iTable.SeekL(k, RDbTable::ELessThan);
	if (!found) return;
	iTable.GetL();
	if (iTable.ColUint32(EAuthorId)!=authorid) return;
	TUint dbid=iTable.ColUint32(EDbId);
	iAuthorCounts->SetAuthorAsRead(authorid, dbid+1, iLCAuthor);
	
	TInt cc=-1, uc=0;
	iAuthorCounts->GetStatsL(authorid, cc, uc, iLCAuthor);
	
	auto_ptr<MGenericIntMapIterator> iter(iDbIdToObjectMap->CreateIterator());
	TUint32 key; TAny* ptr;
	while (iter->NextL(key, ptr)) {
		CBBFeedItem* i=(CBBFeedItem*)ptr;
		if (i && i->iAuthorNick()==aAuthor) {
			i->iIsUnread()=EFalse;
			NotifyObservers(i, MFeedNotify::EUnreadChanged);
		}
	}
	
	NotifyObservers(aAuthor, cc, uc);
}

void CFeedItemStorageImpl::MarkAsRead(CBBFeedItem* aFeedItem)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("MarkAsRead-feeditem"));
	MarkAsRead(aFeedItem->iUuid());
}

void CFeedItemStorageImpl::MarkAsRead(const TGlobalId& aPostOrParent)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("MarkAsRead"));

	TGlobalId parent_id;
	TInt read_counter=0;

#ifdef EXPLICIT_TRANSACTIONS
	if (! iDb.Db().InTransaction()) BeginL();
	TTransactionHolder th(*this);
#endif

	TBool gc=EFalse;

	if (SeekToFeedItemIdL(aPostOrParent)) {
	        iTable.GetL();
	        TUint dbid=iTable.ColUint32(EDbId);
		parent_id=iTable.ColDes8(EParentId);
		gc=iTable.ColInt(EGroupChild);
		DoMarkAsReadL(dbid, read_counter);
	} else {
		return;
	}

	UpdateReadCounters(parent_id, 0, -read_counter, gc);

	Async();
}

void CFeedItemStorageImpl::MarkAllAsRead()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("MarkAllAsRead"));
	
	SwitchIndexL(EIdxDbId);
	TBool found=iTable.LastL();
	if (!found) return;
	
	iTable.GetL();
	TUint dbid=iTable.ColUint32(EDbId);
	iAuthorCounts->SetAuthorAsRead(KAllCountNick(), dbid+1);
	
	auto_ptr<MGenericIntMapIterator> iter(iDbIdToObjectMap->CreateIterator());
	TUint32 key; TAny* ptr;
	while (iter->NextL(key, ptr)) {
		CBBFeedItem* i=(CBBFeedItem*)ptr;
		if (i) {
			i->iIsUnread()=EFalse;
			NotifyObservers(i, MFeedNotify::EUnreadChanged);
		}
	}
	
	iAuthorCounts->SetAllAsUnread(iSubscribers);
}

void CFeedItemStorageImpl::DoMarkAsReadL(TUint db_id, TInt& read_counter)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("DoMarkAsReadL"));

	auto_ptr<CArrayFixFlat<TUint> > aFeedItemIdArray (new (ELeave) CArrayFixFlat<TUint>(50));

	if (SeekToDbIdL(db_id)) {
		iTable.GetL();
		TGlobalId post_id   = iTable.ColDes8(EFeedItemId);
		CBBFeedItem* p=(CBBFeedItem*)iDbIdToObjectMap->GetData(db_id);
		UpdateL();
		TBool unread_changed=EFalse;
		if (iTable.ColInt32(EUnread)>0 && ! iTable.ColInt32(EGroupChild)) {
			unread_changed=ETrue;
			read_counter++;
		}
		iTable.SetColL(EUnread, 0);
		if (iTable.ColDes8(EFeedItemId).Length()>0) {
			iTable.SetColL(ELastAccess, GetTime());
		}
		PutL();
		if (unread_changed) {
			TInt cc=0, uc=-1;
			TBuf<50> an;
			iAuthorCounts->UpdateStatsL(iTable.ColUint32(EAuthorId), cc, uc, an);
			NotifyObservers(an, cc, uc);
		}

		if (p) {
			p->iIsUnread()=0;
			NotifyObservers(p, MFeedNotify::EUnreadChanged);
		}
			
		TBool ok = FirstByParentL(post_id);
		while (ok) {
			TUint dbid=GetCurrentLocalIdL();
			if (dbid == db_id) {
				User::Panic(_L("FEEDSTORAGE"), -1);
			}
			aFeedItemIdArray->AppendL(dbid);
			ok = NextL();
		}
	}
	
	if (aFeedItemIdArray->Count()!=0) {
		int i;
		for (i=0; i< aFeedItemIdArray->Count(); i++) {
			TUint idx=(*aFeedItemIdArray)[i];
			DoMarkAsReadL(idx, read_counter);
		}
	}
}

TBool CFeedItemStorageImpl::SeekToFeedItemIdL(const TGlobalId& aFeedItemId)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("SeekToFeedItemIdL"));

	SwitchIndexL(EIdxFeedItemId);
	TDbSeekKey rk(aFeedItemId);
	TBool &ret=iFound;
	ret=EFalse;

	ret=iTable.SeekL(rk);
	return ret;
}

TBool CFeedItemStorageImpl::SeekToDbIdL(TUint aDbId)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("SeekToDbIdL"));

	SwitchIndexL(EIdxDbId);
	TDbSeekKey rk(aDbId);
	TBool &ret=iFound;
	ret=EFalse;

	ret=iTable.SeekL(rk);
	return ret;
}

TBool CFeedItemStorageImpl::GetFirstByCorrelationL(const TGlobalId& aCorrelation, TGlobalId &aPostIdInto)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetFirstByCorrelationL"));
	SwitchIndexL(EIdxCorrelation);
	TDbSeekKey rk(aCorrelation);

	TBool ret=iTable.SeekL(rk);
	if (ret) {
		iTable.GetL();
		aPostIdInto=iTable.ColDes8(EFeedItemId);
	}
	return ret;
}

void CFeedItemStorageImpl::NotifyObservers(CBBFeedItem* aFeedItem, MFeedNotify::TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("NotifyObservers"));
	CC_TRAPD(err, NotifyObserversL(aFeedItem, aEvent));
	User::LeaveIfError(err);
}

void CFeedItemStorageImpl::NotifyObserversL(CBBFeedItem* aFeedItem, MFeedNotify::TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("NotifyObserversL"));

	CList<MFeedNotify*>::Node* i=0;
	for (i=iSubscribers->iFirst; i; i=i->Next) {
		i->Item->FeedItemEvent(aFeedItem, aEvent);
	}
}

void CFeedItemStorageImpl::NotifyObservers(const TDesC& aAuthor, TInt aNewItemCount, TInt aNewUnreadCount)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("NotifyObservers(aAuthor)"));

	CList<MFeedNotify*>::Node* i=0;
	for (i=iSubscribers->iFirst; i; i=i->Next) {
		i->Item->AuthorCountEvent(aAuthor, aNewItemCount, aNewUnreadCount);
	}
}

void CFeedItemStorageImpl::FeedItemDeleted(const class CBBFeedItem* aFeedItem)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("FeedItemDeleted"));

	CC_TRAPD(err, iDbIdToObjectMap->DeleteL(aFeedItem->iLocalDatabaseId));
	if (err!=KErrNone) { User::Panic(_L("ContextMedia"), KErrCorrupt); }
}

void CFeedItemStorageImpl::UpdateFeedItemL(CBBFeedItem * aFeedItem, TBool aNotify)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateFeedItemL"));
	PrintDebugUpdate(aFeedItem);

	if (aFeedItem->iUuid().Length()!=aFeedItem->iUuid().MaxLength())
		User::Leave(KErrArgument);
		
	SeekToFeedItemIdL(aFeedItem->iUuid());
	if (!iFound) User::Leave(KErrNotFound);

#ifdef EXPLICIT_TRANSACTIONS
	if (! iDb.Db().InTransaction()) BeginL();
	TTransactionHolder th(*this);
#endif 

	UpdateL();
	TUint32 dbid=iTable.ColUint(EDbId);
	
	{
		// we have to set this to null, since we might have
		// updated it in the same transaction, in which case
		// the new contents would get appended instead of replacing
		iTable.SetColNullL(EContents);
		RADbColWriteStream w; w.OpenLA(iTable, EContents);
		aFeedItem->ExternalizeL(w);
		w.CommitL();
	}

	TTime t; t=GetTime();
	iTable.SetColL(ELastAccess, t);
	iTable.SetColL(EUpdated, t);
	TUint prev_author=iTable.ColUint32(EAuthorId);
	iLCAuthor=aFeedItem->iAuthorNick();
	iLCAuthor.LowerCase();
	TUint authorid=iAuthorCounts->GetIdByNickL(iLCAuthor);
	iTable.SetColL(EAuthorId, authorid);

	TGlobalId new_parentid;
	if (aFeedItem->iUuid() != aFeedItem->iParentUuid()) {
		new_parentid=aFeedItem->iParentUuid();
	}
	TGlobalId prev_parent=iTable.ColDes8(EParentId);
	if (prev_parent != new_parentid ) {
		iTable.SetColL(EParentId, new_parentid);
	}
	iTable.SetColL(EMediaDownloadState, aFeedItem->iMediaDownloadState());

	TInt unread=0;
	unread=iTable.ColInt32(EUnread);
	PutL();
	refcounted_ptr<CBBFeedItem> existingp((CBBFeedItem*)iDbIdToObjectMap->GetData(dbid));
	CBBFeedItem* existing=existingp.get();
	if (existing) existing->AddRef();
	if (existing && existing!=aFeedItem) *existing=*aFeedItem;
	
	if (prev_parent != new_parentid) {
		aFeedItem->iParentUuid()=prev_parent;
		if (existing) NotifyObservers(aFeedItem, MFeedNotify::EParentChanged);
		aFeedItem->iParentUuid()=new_parentid;
		if (existing) NotifyObservers(aFeedItem, MFeedNotify::EChildAdded);
		if (unread!=0) {
			UpdateReadCounters(prev_parent, prev_author, -unread, aFeedItem->iIsGroupChild());
			UpdateReadCounters(aFeedItem->iParentUuid(), authorid, unread, aFeedItem->iIsGroupChild());
		}
	}
	if (existing && aNotify) NotifyObservers(existing, MFeedNotify::EFeedItemUpdated);

	Async();
}
	
#if 0
void CFeedItemStorageImpl::UpdateFileName(TGlobalId aFeedItemId, const TDesC& aFileName, const TDesC& aContentType)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateFileName"));

#ifdef EXPLICIT_TRANSACTIONS
	if (! iDb.Db().InTransaction() ) BeginL();
	TTransactionHolder th(*this);
#endif

	TGlobalId parent=DoUpdateFileName(aFeedItemId, aFileName, aContentType);
	if (IsFirstFeedItemInThread(parent, aFeedItemId)) {
		DoUpdateFileName(parent, aFileName, aContentType);
	}
}

TGlobalId CFeedItemStorageImpl::DoUpdateFileName(TGlobalId aFeedItemId, const TDesC& aFileName, const TDesC& aContentType)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("DoUpdateFileName"));

	refcounted_ptr<CBBFeedItem> post(GetByLocalIdL(0, aFeedItemId));
	post->iMediaFileName()=aFileName;
	post->iContentType()=aContentType;

	if (aContentType.Left(5).CompareF(_L("video"))==0) {
		post->SetThumbNailIndex(EMbmContextmediaVideo);
	} else if (aContentType.Left(5).CompareF(_L("audio"))==0) {
		post->SetThumbNailIndex(EMbmContextmediaAudio);
	}

	{
		UpdateL();
		{
			iTable.SetColNullL(EContents);
			RADbColWriteStream w; w.OpenLA(iTable, EContents);
			post->ExternalizeL(w);
			w.CommitL();
		}
		PutL();
	}

	refcounted_ptr<CBBFeedItem> parent(0);
	if (post->iParentUuid()==RootId()) {
		iRoot->AddRef();
		parent.reset(iRoot);
	} else if (iMediaPool && post->iParentUuid()==MediaPoolId()) {
		iMediaPool->AddRef();
		parent.reset(iMediaPool);
	} else {
		parent.reset(GetByLocalIdL(0, post->iParentUuid()));
	}
	
	NotifyObservers(parent.get(), post.get(), MFeedNotify::EMediaLoaded);

	return post->iParentUuid();
}

void CFeedItemStorageImpl::UpdateIcon(TGlobalId aFeedItemId, auto_ptr<CFbsBitmap>& aIcon)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateIcon"));

	auto_ptr<CFbsBitmap> icon2(0);
	if (aIcon.get()) {
		icon2.reset(new (ELeave) CFbsBitmap);
		icon2->Duplicate(aIcon->Handle());
	}

	TGlobalId parent=DoUpdateIcon(aFeedItemId, aIcon);
	if (IsFirstFeedItemInThread(parent, aFeedItemId)) {
		DoUpdateIcon(parent, icon2);
	}
}

TGlobalId CFeedItemStorageImpl::DoUpdateIcon(TGlobalId aFeedItemId, auto_ptr<CFbsBitmap>& aIcon)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("DoUpdateIcon"));

	refcounted_ptr<CBBFeedItem> post(GetByLocalIdL(0, aFeedItemId));

	UpdateL();
	{
		RADbColWriteStream w; w.OpenLA(iTable, EIcon);
		aIcon->ExternalizeL(w);
		w.CommitL();
	}
	PutL();

	auto_ptr<CGulIcon> icon(CGulIcon::NewL(aIcon.get()));
	aIcon.release();
	TInt idx;
	if (iNextFreeIconSlot==-1) {
		iIconArray->AppendL(icon.get());
		idx=iIconArray->Count()-1;
	} else {
		TInt temp=(TInt)iIconArray->At(iNextFreeIconSlot);
		iIconArray->At(iNextFreeIconSlot)=icon.get();
		idx=iNextFreeIconSlot;
		iNextFreeIconSlot=temp;
	}
	icon.release();

	post->SetThumbNailIndex(idx);

	refcounted_ptr<CBBFeedItem> parent(0);
	if (post->iParentUuid()==RootId()) {
		iRoot->AddRef();
		parent.reset(iRoot);
	} else if (iMediaPool && post->iParentUuid()==MediaPoolId()) {
		iMediaPool->AddRef();
		parent.reset(iMediaPool);
	} else {
		parent.reset(GetByLocalIdL(0, post->iParentUuid()));
	}
	
	NotifyObservers(parent.get(), post.get(), MFeedNotify::EThumbnailLoaded);

	return post->iParentUuid();
}

TGlobalId CFeedItemStorageImpl::GetParentId(TGlobalId aFeedItemId)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetParentId"));
	if (!iHasTransactionHolder) CommitL();
	if (!SeekToFeedItemIdL(aFeedItemId)) User::Leave(KErrNotFound);

	iTable.GetL();
	return iTable.ColDes8(EParentId);
}
#endif


void CFeedItemStorageImpl::UpdateErrorInfo(const TGlobalId & aFeedItemId, const CBBErrorInfo* aErrorInfo, TBool aWillRetry)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateErrorInfo"));

	if (!iHasTransactionHolder) CommitL();
	if (!SeekToFeedItemIdL(aFeedItemId)) return; // we can get errors after post has been deleted

	UpdateCurrentErrorInfo(aErrorInfo, aWillRetry);
}

void CFeedItemStorageImpl::UpdateDownloadStateL(CBBFeedItem * aFeedItem)
{
	if (! SeekToDbIdL(aFeedItem->iLocalDatabaseId) )  User::Leave(KErrNotFound);
	
	{
		TAutomaticTransactionHolder th(*this);
		UpdateL();
		iTable.SetColL(EMediaDownloadState, aFeedItem->iMediaDownloadState());
		PutL();
	}
	NotifyObservers(aFeedItem, MFeedNotify::EMediaDownloadStateChanged);
}

void CFeedItemStorageImpl::UpdateErrorInfo(TUint aDbId, const CBBErrorInfo* aErrorInfo, TBool aWillRetry)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateErrorInfo"));

	if (!iHasTransactionHolder) CommitL();
	if (!SeekToDbIdL(aDbId)) return; // we can get errors after post has been deleted

	UpdateCurrentErrorInfo(aErrorInfo, aWillRetry);
}

void CFeedItemStorageImpl::UpdateCurrentErrorInfo(const CBBErrorInfo* aErrorInfo, TBool aWillRetry)
{
	iTable.GetL();
	if (!aErrorInfo && iTable.IsColNull(EErrorInfo)) return;

	TUint dbid=iTable.ColUint32(EDbId);
	CBBFeedItem* existing=(CBBFeedItem*)iDbIdToObjectMap->GetData(dbid);

	{
		TAutomaticTransactionHolder th(*this);
		UpdateL();
		iTable.SetColNullL(EErrorInfo);
		CBBFeedItem::TMediaDownloadState s;
		if (aErrorInfo) {
			if (aWillRetry) s=CBBFeedItem::EDownloadErrorRetrying;
			else s=CBBFeedItem::EDownloadErrorFailed;
			iTable.SetColL(EMediaDownloadState, s);
		} 
		if (existing) existing->iMediaDownloadState()=s;
		if (aErrorInfo) {
			RADbColWriteStream w; w.OpenLA(iTable, EErrorInfo);
			aErrorInfo->ExternalizeL(w);
			w.CommitL();
		}
		PutL();
	}

	if (existing) {
		if (existing->iErrorInfo) existing->iErrorInfo->Release();
		existing->iErrorInfo=0;
		if (aErrorInfo) {
			existing->iErrorInfo=bb_cast<CBBErrorInfo>(aErrorInfo->CloneL(KNullDesC));
		} else {
			existing->iErrorInfo=0;
		}
		NotifyObservers(existing, MFeedNotify::EErrorUpdated);
	}
}

#if 0
void CFeedItemStorageImpl::UpdateError(TGlobalId aFeedItemId, TInt aErrorCode, const TDesC& aError)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("UpdateError"));

	refcounted_ptr<CBBFeedItem> post(GetByLocalIdL(0, aFeedItemId));
	post->iErrorDescr()=aError;
	post->iErrorCode()=aErrorCode;

	CommitL();
	{
		TAutomaticTransactionHolder th(*this);
		UpdateL();
		{
			iTable.SetColNullL(EContents);
			RADbColWriteStream w; w.OpenLA(iTable, EContents);
			post->ExternalizeL(w);
			w.CommitL();
		}
		PutL();
	}

	refcounted_ptr<CBBFeedItem> parent(0);
	if (post->iParentUuid()==RootId()) {
		iRoot->AddRef();
		parent.reset(iRoot);
	} else if (iMediaPool && post->iParentUuid()==MediaPoolId()) {
		iMediaPool->AddRef();
		parent.reset(iMediaPool);
	} else {
		parent.reset(GetByLocalIdL(0, post->iParentUuid()));
	}
	
	NotifyObservers(parent.get(), post.get(), MFeedNotify::EErrorUpdated);
}
#endif

void CFeedItemStorageImpl::CommitL()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("CommitL"));
	if (! iDb.Db().InTransaction() ) return;
	TAutoBusy busy(Reporting());
	iDb.CommitL();
}

TBool CFeedItemStorageImpl::CompactIfHighWaterL()
{
	return CompactIfCloseL();
}

void CFeedItemStorageImpl::GetCurrentGlobalIdL(TGlobalId& aInto)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetCurrentGlobalIdL"));

	if (!iFound) 
		User::Leave(KErrNotFound);

	iTable.GetL();
	aInto=iTable.ColDes8(EFeedItemId);
}

TTime CFeedItemStorageImpl::GetCurrentTimeStampL()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetCurrentTimeStampL"));

	if (!iFound) 
		User::Leave(KErrNotFound);

	iTable.GetL();
	TTime result = iTable.ColTime(EDatetime);
	return result;
}

void CFeedItemStorageImpl::GetCountsByAuthorL(const TDesC& aAuthor, TInt& aItemCount, TInt& aUnreadCount)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetCountsByAuthorL"));
	iLCAuthor=aAuthor;
	iLCAuthor.LowerCase();
	iAuthorCounts->GetStatsL(iLCAuthor, aItemCount, aUnreadCount);
}

#if 0

TGlobalId CFeedItemStorageImpl::GetLastFeedItemId(TGlobalId aParentId)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("GetLastFeedItemId"));

	if (aParentId==MediaPoolId()) {
		SwitchIndexL(3);
		TDbSeekKey k(TGlobalId(-1500000));

		/* typically, posts in the media pool have negative ids*/
		TBool found=EFalse;
		found=iTable.SeekL(k, RDbTable::EGreaterEqual);
		if (!found) return TGlobalId(0);

		iTable.GetL();
		if (iTable.ColDes8(EFeedItemId) >= TGlobalId(0)) return TGlobalId(0);

		TGlobalId postid=iTable.ColDes8(EFeedItemId);
		return postid;
	} else {
		SwitchIndexL(6);
		TDbSeekMultiKey<2> k;
		k.Add(aParentId+1);
		k.Add(TGlobalId(0));

		TBool found=EFalse;
		found=iTable.SeekL(k, RDbTable::ELessThan);
		if (!found) return TGlobalId(0);

		iTable.GetL();
		if (iTable.ColDes8(EParentId) != aParentId) return TGlobalId(0);

		return iTable.ColDes8(EFeedItemId);
	}
}

void CFeedItemStorageImpl::SetThreadVisibleL(TGlobalId aFeedItemId, TBool visible)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("SetThreadVisibleL"));

	CommitL();
	if (aFeedItemId == RootId()) 
		User::Leave(KErrArgument);
	if (aFeedItemId == MediaPoolId()) 
		User::Leave(KErrArgument);
	if (aFeedItemId == AddMeToMediaPoolId()) {
		User::Leave(KErrArgument);
	}

	SeekToFeedItemIdL(aFeedItemId);
	if (!iFound)  User::Leave(KErrNotFound);
	
	iTable.GetL();
	TBool hidden = EFalse;
	if (! iTable.IsColNull(EHidden) )
		hidden = iTable.ColUint32(EHidden);

	if (hidden != visible) {
		{
			TAutomaticTransactionHolder th(*this);
			UpdateL();
			iTable.SetColL(EHidden, !visible);
			TTime t; t=GetTime();
			iTable.SetColL(ELastAccess, t);

			PutL();
		}

		refcounted_ptr<CBBFeedItem> placeholder(GetCurrentL(0));
		
		if (visible) {
			NotifyObservers(placeholder.get(), MFeedNotify::EFeedItemVisible);
			iNbHiddenThreads--;
		} else {
			NotifyObservers(placeholder.get(), MFeedNotify::EFeedItemHidden);
			iNbHiddenThreads++;
		}
	}
}

void CFeedItemStorageImpl::SetAllThreadsVisibleL()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("SetAllThreadsVisibleL"));

	CommitL();
	TBool ok = FirstL(RootId(), CFeedItemStorage::EByDate, CFeedItemStorage::EAscending, EFalse, EFalse);

	while (ok) {
		iTable.GetL();
		TBool hidden=EFalse;
		if ( ! iTable.IsColNull(EHidden) )
			hidden = iTable.ColUint32(EHidden);
		if (hidden) {
			{
				TAutomaticTransactionHolder th(*this);
				UpdateL();
				iTable.SetColL(EHidden, EFalse);
				TTime t; t=GetTime();
				iTable.SetColL(ELastAccess, t);
				PutL();
			}

			refcounted_ptr<CBBFeedItem> placeholder(GetCurrentL(0));
			NotifyObservers(placeholder.get(), MFeedNotify::EFeedItemVisible);
			iNbHiddenThreads--;
		}
		ok = NextL();
	}
}

TInt CFeedItemStorageImpl::HasHiddenThreads()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("HasHiddenThreads"));

	return (iNbHiddenThreads!=0);
}

TInt CFeedItemStorageImpl::CountHiddenThreadsL()
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("CountHiddenThreadsL"));

	TBool ok = FirstL(RootId(), CFeedItemStorage::EByAuthor, CFeedItemStorage::EAscending, EFalse, EFalse);
	
	TInt ret=0;
		
	while (ok) {
		iTable.GetL();
		TBool hidden = EFalse;
		if (! iTable.IsColNull(EHidden) )
			hidden = iTable.ColUint32(EHidden);
		if (hidden) ret++;
		ok=NextL();
	}
	return ret;
}
#endif


#ifdef DEBUG_STORAGE
void CFeedItemStorageImpl::PrintDebugObserver(MFeedNotify* aNotify)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("PrintDebugObserver"));
	if (!aNotify) return;
	if (CallStackMgr().GetCurrentClass().Compare(_CL("CFeedItemStorageImpl")) ) {
		TBuf<100> msg=_L("**Observer ");
		msg.AppendNum( (TUint32)aNotify, EHex );
		msg.Append(_L(" "));
		TBuf<50> cn;
		TInt state;
		CC()->ConvertToUnicode(cn, CallStackMgr().GetCurrentClass(), state);
		msg.Append(cn);
		RDebug::Print(msg);
	}
}
void CFeedItemStorageImpl::PrintDebugRelease(MFeedNotify* aNotify)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("PrintDebugRelease"));
	if (!aNotify) return;
	TBuf<100> msg=_L("**Release ");
	msg.AppendNum( (TUint32)aNotify, EHex );
	RDebug::Print(msg);
}

void CFeedItemStorageImpl::PrintDebugUpdate(CBBFeedItem* aFeedItem)
{
	CALLSTACKITEM_N(_CL("CFeedItemStorageImpl"), _CL("PrintDebugUpdate"));
	if (!aFeedItem) return;
	TBuf<256> msg=_L("**FeedItem update: id ");
	msg.AppendNum(aFeedItem->iUuid());
	msg.Append(_L(" parent "));
	msg.AppendNum(aFeedItem->iParentUuid());
	msg.Append(_L(" filename "));
	msg.Append(aFeedItem->iMediaFileName());
	msg.Append(_L(" by"));
	TBuf<50> cn;
	TInt state;
	CC()->ConvertToUnicode(cn, CallStackMgr().GetCurrentClass(), state);
	msg.Append(cn);
	//RDebug::Print(msg);
	Reporting().DebugLog(msg);
}
#endif

CAuthorStats::CAuthorStats(MApp_context& Context, CDb& Db) : MContextBase(Context), MDBStore(Db.Db()), iDb(Db) { }

void CAuthorStats::ConstructL()
{
	CALLSTACKITEM_N(_CL("CAuthorStats"), _CL("ConstructL"));
	TInt cols[]= { 
		EDbColUint32,	// dbid
		EDbColText,	// author
		EDbColInt32,	// unread counter
		EDbColInt32,	// child counter
		EDbColUint32,	// unread until
		-1
	};
	TInt col_flags[]={ TDbCol::EAutoIncrement, 0, 0, 0, 0 };
	TInt idxs[]= { EAuthor, -2, EDbId, -1 };

	_LIT(KCounts, "AUTHORCOUNTS");
	MDBStore::ConstructL(cols, idxs, false, KCounts, ETrue, col_flags);
}

void CAuthorStats::SetStatsL(const TDesC& aAuthor, TInt& aChildCount, TInt& aUnreadCount)
{
	CALLSTACKITEM_N(_CL("CAuthorStats"), _CL("SetStatsL"));
	if (aAuthor.Length()==0) return;
	//if (aChildCount==-1 && aUnreadCount==-1) return;
	
	TBool insert_if_doesnt_exist=(aChildCount>0 || aUnreadCount>0);
	if (! SeekToAuthorL(aAuthor, insert_if_doesnt_exist) ) {
		aChildCount=aUnreadCount=0;
		return;
	}
	SetThisStatsL(aChildCount, aUnreadCount, insert_if_doesnt_exist);
	
}

void CAuthorStats::UpdateStatsL(const TDesC& aAuthor, TInt& aChildDelta, TInt& aUnreadDelta)
{
	CALLSTACKITEM_N(_CL("CAuthorStats"), _CL("UpdateStatsL"));
	if (aAuthor.Length()==0) return;
	if (aChildDelta==0 && aUnreadDelta==0) return;
	
	SeekToAuthorL(aAuthor, ETrue);
	UpdateThisStatsL(aChildDelta, aUnreadDelta);
}

void CAuthorStats::SetStatsL(TUint aAuthor, TInt& aChildCount, TInt& aUnreadCount, TDes& aAuthorNick)
{
	CALLSTACKITEM_N(_CL("CAuthorStats"), _CL("SetStatsL"));
	if (aAuthor==0) return;
	
	TBool insert_if_doesnt_exist=(aChildCount>0 || aUnreadCount>0);
	if (! SeekToAuthorL(aAuthor, insert_if_doesnt_exist) ) {
		aChildCount=aUnreadCount=0;
		return;
	}
	aAuthorNick=iTable.ColDes(EAuthor);
	SetThisStatsL(aChildCount, aUnreadCount, insert_if_doesnt_exist);
	
}

void CAuthorStats::UpdateStatsL(const TUint aAuthor, TInt& aChildDelta, TInt& aUnreadDelta, TDes& aAuthorNick)
{
	CALLSTACKITEM_N(_CL("CAuthorStats"), _CL("UpdateStatsL"));
	if (aAuthor==0) return;
	if (aChildDelta==0 && aUnreadDelta==0) return;
	
	SeekToAuthorL(aAuthor, ETrue);
	aAuthorNick=iTable.ColDes(EAuthor);
	UpdateThisStatsL(aChildDelta, aUnreadDelta);
}

void CAuthorStats::UpdateThisStatsL(TInt& aChildDelta, TInt& aUnreadDelta)
{
	TInt cc=iTable.ColInt(EChildCount)+aChildDelta;
	TInt uc=iTable.ColInt(EUnread)+aUnreadDelta;
	if (cc==0 && uc==0) {
		iTable.Cancel();
		DeleteL();
	} else {
		iTable.SetColL(EChildCount, cc);
		iTable.SetColL(EUnread, uc);
		PutL();
	}
	aChildDelta=cc;
	aUnreadDelta=uc;
}

void CAuthorStats::SetThisStatsL(TInt& aChildCount, TInt& aUnreadCount, TBool insert_if_doesnt_exist)
{
	if (aChildCount==0 && aUnreadCount==0) 
	{
		DeleteL();
		return;
	}
	
	if (!insert_if_doesnt_exist) UpdateL();
	
	if ( aChildCount>-1) iTable.SetColL(EChildCount, aChildCount);
	else aChildCount=iTable.ColInt(EChildCount);
	if ( aUnreadCount>-1) iTable.SetColL(EUnread, aUnreadCount);
	else aUnreadCount=iTable.ColInt(EUnread);
	PutL();
}

void CAuthorStats::GetStatsL(const TDesC& aAuthor, TInt& aChildCount, TInt& aUnreadCount)
{
	CALLSTACKITEM_N(_CL("CAuthorStats"), _CL("GetStatsL"));
	aChildCount=aUnreadCount=0;
	if (SeekToAuthorL(aAuthor, EFalse)) {
		iTable.GetL();
		aChildCount=iTable.ColInt(EChildCount);
		aUnreadCount=iTable.ColInt(EUnread);
	}
}

void CAuthorStats::GetStatsL(TUint aAuthor, TInt& aChildCount, TInt& aUnreadCount, TDes& aAuthorNick)
{
	CALLSTACKITEM_N(_CL("CAuthorStats"), _CL("GetStatsL"));
	aChildCount=aUnreadCount=0;
	if (SeekToAuthorL(aAuthor, EFalse)) {
		iTable.GetL();
		aChildCount=iTable.ColInt(EChildCount);
		aUnreadCount=iTable.ColInt(EUnread);
	}
}

TBool CAuthorStats::SeekToAuthorL(const TDesC& aAuthor, TBool aInsertIfDoesntExist)
{
	CALLSTACKITEM_N(_CL("CAuthorStats"), _CL("SeekToAuthorL"));
	TDbSeekKey k(aAuthor);
	SwitchIndexL(EIdxAuthor);
	TBool found=iTable.SeekL(k);
	if (aInsertIfDoesntExist) {
		if (found) {
			UpdateL();
		} else {
			InsertL();
			iTable.SetColL(EAuthor, aAuthor);
			iTable.SetColL(EUnread, 0);
			iTable.SetColL(EChildCount, 0);
			iTable.SetColL(EUnreadUntilId, 0);
		}
		found=ETrue;
	} else if(found) {
		iTable.GetL();
	}
	return found;
}

TBool CAuthorStats::SeekToAuthorL(TUint aAuthor, TBool aInsertIfDoesntExist)
{
	TDbSeekKey k(aAuthor-1);
	SwitchIndexL(EIdxDbId);
	TBool found=iTable.SeekL(k);
	if (!found && aInsertIfDoesntExist) User::Leave(KErrCorrupt);
	if (found) {
		iTable.GetL();
		if (aInsertIfDoesntExist) iTable.UpdateL();
	}
	return found;
}

TUint32 CAuthorStats::IsUnreadUntilId(const TDesC& aAuthor)
{
	if (!SeekToAuthorL(aAuthor, EFalse)) return 0;
	return iTable.ColUint32(EUnreadUntilId);
}

TUint32 CAuthorStats::IsUnreadUntilId(TUint32 aAuthor)
{
	if (!SeekToAuthorL(aAuthor, EFalse)) return 0;
	return iTable.ColUint32(EUnreadUntilId);
}

void CAuthorStats::SetAllAsUnread(CList<MFeedNotify*> *aSubscribers)
{
	TDbBookmark bm;;	
	TBool found=iTable.FirstL();
	CList<MFeedNotify*>::Node *i=0;
	while (found) {
		iTable.GetL();
		bm=iTable.Bookmark();
		if (iTable.ColInt(EUnread)>0) {
			UpdateL();
			iTable.SetColL(EUnread, 0);
			PutL();
			for (i=aSubscribers->iFirst; i; i=i->Next) {
				i->Item->AuthorCountEvent(iTable.ColDes(EAuthor), iTable.ColInt(EChildCount), 0);
			}
		}
		iTable.GotoL(bm);
		found=iTable.NextL();
	}
}

void CAuthorStats::SetAuthorAsRead(TUint aAuthor, TUint aUntilId, TDes& aAuthorNick)
{
	SeekToAuthorL(aAuthor, ETrue);
	iTable.SetColL(EUnread, 0);
	iTable.SetColL(EUnreadUntilId, aUntilId);
	aAuthorNick=iTable.ColDes(EAuthor);
	PutL();	
}
void CAuthorStats::SetAuthorAsRead(const TDesC& aAuthor, TUint aUntilId)
{
	SeekToAuthorL(aAuthor, ETrue);
	iTable.SetColL(EUnread, 0);
	iTable.SetColL(EUnreadUntilId, aUntilId);
	PutL();	
}


const TDesC& CAuthorStats::GetNickByIdL(TUint aId)
{
	SeekToAuthorL(aId, ETrue);
	PutL();
	return iTable.ColDes(EAuthor);
}

TUint CAuthorStats::GetIdByNickL(const TDesC& aAuthor)
{
	SeekToAuthorL(aAuthor, ETrue);
	PutL();
	iTable.GetL();
	return iTable.ColUint32(EDbId)+1;
}


