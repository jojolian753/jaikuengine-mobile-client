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

#include "ccu_feedmodel.h"

#include "ccu_feeditem.h"
#include "ccu_progressbar.h"
#include "ccu_storage.h"

#include "app_context_impl.h"
#include "break.h"
#include "context_uids.h"
#include "cc_stringtools.h"
#include "jabberdata.h"
#include "raii_array.h"
#include "cbbsession.h" // for MBBObserver
#include "csd_threadrequest.h"
#include "csd_clienttuples.h"
#include "ccu_utils.h"

#include <e32base.h>


struct TIdTimePair
{
	TIdTimePair() : 
		iId(KErrNotFound), iTime( Time::NullTTime() ) {}

	TIdTimePair(TUint aId) : 
		iId(aId),          iTime( Time::NullTTime() ) {}
	
	TIdTimePair(const TTime& aTime) : 
		iId(KErrNotFound), iTime( aTime ) {}

	TIdTimePair(TUint aId, const TTime& aTime) : 
		iId(aId),          iTime(aTime) {}
	
	TUint iId;
	TTime iTime;
};


static TInt AscendingDate( const TIdTimePair& x, 
						  const TIdTimePair& y )
{
	if ( x.iTime < y.iTime ) return -1;
	else if ( x.iTime > y.iTime ) return +1;
	else 
		{
			TInt d = x.iId - y.iId; 
			return d;
		}
}

static TInt DescendingDate( const TIdTimePair& x, 
						   const TIdTimePair& y )
{
	return AscendingDate( y, x );
}



const TComponentName KThreadRequester = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 5 };
const TTupleName KThreadRequestTupleUsed = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 5 };

class CFeedModelImpl : public CBase, public MFeedModel, public MContextBase, public MFeedNotify
{
public:
// 	static CFeedModelImpl* NewL(CFeedItemStorage& aFeedStorage)
// 	{
// 		CALLSTACKITEMSTATIC_N(_CL("CFeedModelImpl"), _CL("NewL"));
// 		auto_ptr<CFeedModelImpl> self( new (ELeave) CFeedModelImpl(aFeedStorage) );
// 		self->ConstructL( );
// 		return self.release();
// 	}
	
	virtual ~CFeedModelImpl()
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("~CFeedModelImpl"));
		if ( iBookmark ) iStorage.ReleaseBookmark( iBookmark );
		iIds.Close();
		
		iObservers.Close();
		CC_TRAPD( ignore, iStorage.UnSubscribeL(this) );
	}
	
	CFeedModelImpl(CFeedItemStorage& aFeedStorage)
		: iStorage(aFeedStorage), iTopExtraItems(0), iBottomExtraItems(0), iAscendingDate(EFalse)
	{}


	TIdTimePair SpecialItem(TUint aId, TBool aTop) 
	{
		TTime t = Time::NullTTime();
		if ( iAscendingDate )
			t = aTop ? Time::MinTTime() : Time::MaxTTime();
		else
			t = aTop ? Time::MaxTTime() : Time::MinTTime() ;
		return TIdTimePair( aId, t );
	}
	
	TUint GetId(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("GetId"));
		if ( iIds.Count() <= aIndex && aIndex < Count() )
			{
				ReadNextIdsL( EFalse, aIndex - iIds.Count() +1 );
			}
		if ( aIndex < 0 )
			Bug( _L("Index out of range") ).Raise();
		else if ( iIds.Count() <= aIndex )
			return KMissingPostItem;
		return iIds[aIndex].iId;
	}

	TBool IsAllIdsRead() const
	{
		return iIds.Count() == Count();
	}
	
	TInt GetIndex(TUint aId)
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("GetIndex"));
		TInt ix = FindId( aId );
		
		while ( ix == KErrNotFound && ! IsAllIdsRead() )
			{
				ReadNextIdsL();
				ix = FindId( aId );
			}
		
		return ix;
	}
	
 	virtual void ReadIdsL() = 0;


	/**
	 * ProcessCurrentIdL should be overriden by subclasses. It's called
	 * by ReadNextIdsL
	 */ 
	virtual TBool ProcessCurrentIdL() 
	{
		if ( ! iStorage.CurrentIsGroupChild() )
			{
				TUint id = iStorage.GetCurrentLocalIdL();
				TTime tstamp = iStorage.GetCurrentTimeStampL();
				TIdTimePair pair(id, tstamp);
				iIds.AppendL( pair );
				return ETrue;
			}
		return EFalse;
	}


	virtual TBool InitIdReadingL()
	{
		const TBool isAscDate = EFalse;
		const TBool overviewOnly = ETrue;
		iStorage.ReleaseBookmark(iBookmark);
		return iStorage.FirstAllL( isAscDate, overviewOnly );
	}

	virtual void AllIdsReadL()
	{
	}

	virtual void ReadNextIdsL(TBool aReadAll = EFalse, TInt aMaxReadCount=12)
	{
		if ( aReadAll ) 
			iCount = KErrNotFound;

		
		TBool moreToRead = EFalse;
		if ( !aReadAll && iBookmark )
			{
				moreToRead = iStorage.NextL( iBookmark );
			}
		else
			{
				moreToRead = InitIdReadingL();
			}
		
		TInt readCount = 0;
		while ( moreToRead  )
			{
				if ( ProcessCurrentIdL() ) readCount++;
				
				if ( !aReadAll && readCount >= aMaxReadCount)
					{
						iBookmark = iStorage.Bookmark();
						break;
					}
				else 
					{
						moreToRead = iStorage.NextL();
					}
			}
		
		if ( ! iBookmark )
			AllIdsReadL();
	}
	



	void AddObserverL( MObserver* aObserver )
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("AddObserver"));
		iObservers.AppendL( aObserver );
	}

	void RemoveObserver( MObserver* aObserver )
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("RemoveObserver"));
		TInt ix = iObservers.Find( aObserver );
		if ( ix != KErrNotFound )
			iObservers.Remove( ix );
	}
	
	void NotifyObservers( TInt aIndex, MFeedNotify::TEvent aEvent )
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("NotifyObservers"));
		for (TInt i=0; i < iObservers.Count(); i++)
			{
				iObservers[i]->ItemChanged( aIndex, aEvent );
			}
	}
	


	virtual CBBFeedItem* GetItemL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("GetItemL"));


		if ( iIds.Count() <= aIndex && aIndex < Count() )
			{
				ReadNextIdsL( EFalse, aIndex - iIds.Count() );
			}

		refcounted_ptr<CBBFeedItem> item(NULL);
		if (0 <= aIndex && aIndex < iIds.Count() )
			{
				TBool ignoreNotFound = ETrue;
				item.reset( iStorage.GetByLocalIdL( iIds[aIndex].iId, ignoreNotFound ) );
			}
		
		return item.release();
	}
	

	TInt Count() const
	{
		if ( iCount == KErrNotFound ) // correct count is in iIds
			return iIds.Count();
		else
			return iCount + iTopExtraItems + iBottomExtraItems;
	}

	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("ConstructL"));
		ReadIdsL();
		iStorage.SubscribeL(this);
	}


	void MarkAsRead( TInt aIndex )
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("MarkAsRead"));
		refcounted_ptr<CBBFeedItem> item( GetItemL( aIndex ) );
		if ( item.get() )
			{
				TBool unread = item->iIsUnread();
				if ( unread )
					iStorage.MarkAsRead( item.get() );
			}
	}


	TInt FindId(TUint aId)
	{
		for (TInt i=0; i < iIds.Count(); i++)
			{
				if ( iIds[i].iId == aId )
					return i;
			}
		return KErrNotFound;
	}

	TInt LastIndex( RArray<TIdTimePair>& aArray )
	{
		return aArray.Count() - 1;
	}
	

	
	
public: // MFeedNotify
	virtual void FeedItemEvent(CBBFeedItem* aItem, MFeedNotify::TEvent aEvent)
	{	
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("FeedItemEvent"));
		FeedItemEventImpl( aItem, aEvent );
	}

	virtual void ModifyCount(TInt aAmount)
	{
		if ( iCount != KErrNotFound )
			iCount += aAmount;
	}
	
	virtual void FeedItemEventImpl(CBBFeedItem* aItem, MFeedNotify::TEvent aEvent)
	{
		CALLSTACKITEM_N(_CL("CFeedModelImpl"), _CL("FeedItemEventImpl"));
		if ( ! aItem ) return;
		
		// Update
		TUint id   = aItem->iLocalDatabaseId;
		
		switch ( aEvent )
			{
			case MFeedNotify::EAdded:
				if ( ! aItem->iIsGroupChild() )
					{
// 						TInt addIx = 0;
// 						if ( aInsertToTop  )
// 							addIx = iTopExtraItems;
// 						else
// 							addIx = iIds.Count() - iBottomExtraItems;
						
						TTime tstamp = aItem->iCreated();
						TIdTimePair pair( id, tstamp );
// 						iIds.InsertL(pair, addIx);
						TLinearOrder<TIdTimePair> ordering( iAscendingDate ?  &AscendingDate : &DescendingDate );
 						iIds.InsertInOrder(pair, ordering );
						ModifyCount( +1 );
						TInt addIx = iIds.Find( pair );
						NotifyObservers( addIx, aEvent );
					}
				break;				
			case MFeedNotify::EFeedItemDeleted:
				{
					TInt index = FindId(id);
					if ( index != KErrNotFound ) 
						{
							iIds.Remove(index);
							ModifyCount( -1 );
							NotifyObservers( index, aEvent );
						}
				}
				break;
			default:
				// do nothing
				break;
			}
	}

	virtual void AuthorCountEvent(const TDesC& aAuthor,
								  TInt aNewItemCount, TInt aNewUnreadCount) 
	{
	}

protected: // Component functionality for callbacks
	virtual void ComponentId(TUid& aComponentUid, 
							 TInt& aComponentId,
							 TInt& aVersion)
	{
		aComponentUid=KUidContextContactsUi;
		aComponentId=4; // see ../ids.txt
		aVersion=1;
	}

	void CallbackEnterComponent()
	{
		TUid uid; TInt id; TInt version;
		ComponentId(uid, id, version);
		GetContext()->SetCurrentComponent(uid, id);
	}
	
	void CallbackExitComponent()
	{
		GetContext()->SetCurrentComponent(TUid::Uid(0), 0);
	}
	
protected:

	RArray<TIdTimePair> iIds;
	CFeedItemStorage& iStorage;
	RPointerArray< MFeedModel::MObserver > iObservers;

	TInt iTopExtraItems; 
	TInt iBottomExtraItems;

	TInt iCount;
	class TBookmark* iBookmark;

	TBool iAscendingDate;
};

class CEverybodysFeedModelImpl : public CFeedModelImpl
{
public:
	CEverybodysFeedModelImpl(CFeedItemStorage& aFeedStorage) 
		: CFeedModelImpl(aFeedStorage) { iTopExtraItems=1; }
	
	virtual ~CEverybodysFeedModelImpl() {}
	
	virtual void ReadIdsL()
	{
		CALLSTACKITEM_N(_CL("CEverybodysFeedModelImpl"), _CL("ReadIdsL"));
		
		TGlobalId id; 
		id.Zero();
		TInt unreadChild;
		TInt groupChild;
		groupChild = unreadChild = 0;
		iStorage.GetChildCountsL( id, iCount, unreadChild, groupChild, ETrue );
		
		iIds.Reset();

		TIdTimePair pair = SpecialItem( KPostJaikuItem, ETrue );
		iIds.AppendL( pair );
		
		ReadNextIdsL();
	}


// 	virtual TBool ProcessCurrentIdL() 
// 	{
// 		return CFeedModelImpl::ProcessCurrentIdL();
// 	}


// 	virtual TBool InitIdReadingL()
// 	{
// 		return iStorage.FirstByAuthorL(iPersonNick);
// 	}


	// Use from upper class: virtual TBool ProcessCurrentIdL() 
	// Use from upper class: virtual TBool InitIdReadingL()
	
	virtual void FeedItemEvent(CBBFeedItem* aItem, MFeedNotify::TEvent aEvent)	
	{
		CALLSTACKITEM_N(_CL("CEverybodysFeedModelImpl"), _CL("FeedItemEvent"));
		if ( aItem && ! aItem->iDontShowInOverView )
			{
				CFeedModelImpl::FeedItemEventImpl( aItem, aEvent );
			}
	}
	
private:
};


class CAuthorFeedModelImpl : public CFeedModelImpl
{
public:
	CAuthorFeedModelImpl(const TDesC& aNick, CFeedItemStorage& aFeedStorage, TBool aUserNick) 
		: CFeedModelImpl(aFeedStorage), iPersonNick(aNick), iUserNick(aUserNick) 
	{ 
			iTopExtraItems= iUserNick ? 2 : 1; 
	}
	
	virtual ~CAuthorFeedModelImpl() {}

	virtual void ReadIdsL()
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedModelImpl"), _CL("ReadIdsL"));


		TGlobalId id; 
		id.Zero();
		TInt unreadChild = 0;
		iStorage.GetCountsByAuthorL( iPersonNick, iCount, unreadChild );
		
		iIds.Reset();

		
		iIds.AppendL( SpecialItem( KAuthorHeaderItem, ETrue ) );
		if ( iUserNick )
			iIds.AppendL( SpecialItem( KPostJaikuItem, ETrue ) );

		// storage doesn't contain anything in @jaiku.com form.
		CJabberData::TransformToUiNickL( iPersonNick ); 
		
		ReadNextIdsL();
	}
	
	virtual TBool ProcessCurrentIdL() 
	{
		return CFeedModelImpl::ProcessCurrentIdL();
	}


	virtual TBool InitIdReadingL()
	{
		return iStorage.FirstByAuthorL(iPersonNick);
	}



	virtual void FeedItemEvent(CBBFeedItem* aItem, MFeedNotify::TEvent aEvent)	
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedModelImpl"), _CL("FeedItemEvent"));
		if ( aItem && aItem->iAuthorNick() == iPersonNick )
			{
				CFeedModelImpl::FeedItemEventImpl( aItem, aEvent );
			}
	}
	
private:
	CJabberData::TNick iPersonNick;
	TBool iUserNick;
};



class CFeedGroupModelImpl : public CFeedModelImpl
{
public:
	CFeedGroupModelImpl(TGlobalId aRootGuid, CFeedItemStorage& aFeedStorage) 
		: CFeedModelImpl(aFeedStorage), iRootGuid(aRootGuid)  
	{
		iTopExtraItems = 1;
		iAscendingDate = EFalse;
	}
	
	virtual ~CFeedGroupModelImpl() {}

	virtual TBool ProcessCurrentIdL() 
	{
		if ( iStorage.CurrentIsGroupChild() )
			{
				TUint id = iStorage.GetCurrentLocalIdL();
				TTime tstamp = iStorage.GetCurrentTimeStampL();
				TIdTimePair pair(id, tstamp);
				iIds.AppendL( pair );
				return ETrue;
			}
		return EFalse;
	}
	
	
	virtual TBool InitIdReadingL()
	{
		return iStorage.FirstByParentL( iRootGuid );
	}

	virtual void ReadIdsL()
	{
		CALLSTACKITEM_N(_CL("CFeedGroupModelImpl"), _CL("ReadIdsL"));

		iIds.Reset();		
		refcounted_ptr<CBBFeedItem> rootItem( iStorage.GetByGlobalIdL( iRootGuid ) );
		if ( rootItem.get() )
			{
				// top items
				TIdTimePair pair = SpecialItem( rootItem->iLocalDatabaseId, ETrue );
				iIds.AppendL( pair );

				// item count, note that we need to read it from groupChild count!
				TInt unreadChild;
				TInt commentCount;
				commentCount = unreadChild = 0;
				iStorage.GetChildCountsL( iRootGuid, commentCount, unreadChild, iCount );
				
				ReadNextIdsL();
			}
		else
			{
				TIdTimePair pair = SpecialItem( KMissingPostItem, ETrue );

				iIds.AppendL( pair );
				const TBool KReadAll(ETrue);
				ReadNextIdsL( KReadAll );
			}
	}

	virtual void FeedItemEvent(CBBFeedItem* aItem, MFeedNotify::TEvent aEvent)	
	{
		CALLSTACKITEM_N(_CL("CFeedGroupModelImpl"), _CL("FeedItemEvent"));
		if ( aItem && aItem->iParentUuid() == iRootGuid && aItem->iIsGroupChild() )
			{
				CFeedModelImpl::FeedItemEventImpl( aItem, aEvent );
			}
	}
	
private:
	TGlobalId iRootGuid;
};



class CCommentListModelImpl : public CFeedModelImpl, public MBBObserver
{
public:
	CCommentListModelImpl(TGlobalId aParentGuid, CFeedItemStorage& aFeedStorage, CProgressBarModel& aProgressModel) 
		: CFeedModelImpl(aFeedStorage), iParentGuid(aParentGuid), iProgressModel( aProgressModel )
	{
		iTopExtraItems = 1;
		iBottomExtraItems = 1;
		iComing=-2;
		iAscendingDate = ETrue;
	}
	
	virtual ~CCommentListModelImpl() 
	{
		CALLSTACKITEM_N(_CL("CCommentListModelImpl"), _CL("~CCommentListModelImpl"));
		FetchingStatusL( KNullDesC );
		if ( iParentItem ) iParentItem->Release();
		iParentItem = NULL;
		delete iBBSubSession;
	}

	virtual TBool ProcessCurrentIdL() 
	{
		return CFeedModelImpl::ProcessCurrentIdL();
	}


	virtual TBool InitIdReadingL()
	{
		CALLSTACKITEM_N(_CL("CCommentListModelImpl"), _CL("InitIdReadingL"));
		return iStorage.FirstByParentL( iParentGuid, ETrue );
	}

	virtual void AllIdsReadL()
	{
		CALLSTACKITEM_N(_CL("CCommentListModelImpl"), _CL("AllIdsReadL"));
 		iIds.AppendL( SpecialItem( KPostCommentItem, EFalse ) );
	}

	
	virtual void ReadIdsL()
	{
		CALLSTACKITEM_N(_CL("CCommentListModelImpl"), _CL("ReadIdsL"));

		// top items 
		refcounted_ptr<CBBFeedItem> rootItem( NULL );
		if ( iParentGuid.Length() > 0 )
			rootItem.reset( iStorage.GetByGlobalIdL( iParentGuid, ETrue ) );

		
		if ( rootItem.get() )
			{		
				// Read child counts
				TInt unreadChild;
				TInt groupChild;
				groupChild = unreadChild = 0;
				iStorage.GetChildCountsL( iParentGuid, iCount, unreadChild, groupChild );
				iIds.Reset();
				
				TIdTimePair pair = SpecialItem( rootItem->iLocalDatabaseId, ETrue );
				iIds.AppendL( pair );
				ReadNextIdsL();
			}
		else
			{
				TIdTimePair pair = SpecialItem( KMissingPostItem, ETrue );
				iIds.AppendL( pair );
				const TBool KReadAll = ETrue;
				ReadNextIdsL( KReadAll );
			}
		FetchThreadL();
	}
	
	virtual void CreateParentFromChildL( CBBFeedItem& aChild )
	{
		CALLSTACKITEM_N(_CL("CCommentListModelImpl"), _CL("CreateParentFromChildL"));
		if ( iParentItem )
			{
				User::Panic(_L("CreateParentFromChildL"), 1);
				iParentItem->Release();
				iParentItem = NULL;
			}
		
		iParentItem = new (ELeave) CBBFeedItem;
		iParentItem->iUuid() = aChild.iParentUuid();
		iParentItem->iContent.FromStringL( aChild.iParentTitle() );
		iParentItem->iAuthorNick() = aChild.iParentAuthorNick();
		iParentItem->iFromServer = ETrue;
		iParentItem->iLocalDatabaseId = KMissingPostItem;
		iParentItem->iKind = FeedItem::KKindPresence;
		iParentItem->iCreated = Time::NullTTime();
	}

	virtual CBBFeedItem* GetItemL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CCommentListModelImpl"), _CL("GetItemL"));

		refcounted_ptr<CBBFeedItem> ret(CFeedModelImpl::GetItemL( aIndex ));
		if (aIndex==0 && ret.get()==0 && iIds[aIndex].iId == KMissingPostItem) {
			if (iParentItem) {
				iParentItem->AddRef();
				return iParentItem;
			}
			refcounted_ptr<CBBFeedItem> item( CFeedModelImpl::GetItemL(1) );
			if (item.get()) {
				CreateParentFromChildL(*item);
			}
			iParentItem->AddRef();
			return iParentItem;
		}
		return ret.release();
	}


	virtual void FeedItemEvent(CBBFeedItem* aItem, MFeedNotify::TEvent aEvent)	
	{
		CALLSTACKITEM_N(_CL("CCommentListModelImpl"), _CL("FeedItemEvent"));
		if (!aItem) return;
		
		// Parent itself
		if ( aItem->iUuid() == iParentGuid ) 
			{		
				if ( aEvent ==  MFeedNotify::EAdded )
					{
						iNew++;
						UpdateProgressL();
						MFeedNotify::TEvent event = aEvent;
						if ( iIds.Count() >= 1 && iIds[0].iId == KMissingPostItem )
							{
								iIds.Remove(0);
								event = MFeedNotify::EFeedItemUpdated;
							}
						
						TUint id = aItem->iLocalDatabaseId;
						TIdTimePair pair = SpecialItem( id, ETrue );
						iIds.InsertL(pair, 0);
						NotifyObservers( 0, event );
					}
				else
					{
						CFeedModelImpl::FeedItemEventImpl( aItem, aEvent );
					}
			}
		// Comments 
		else if ( aItem->iParentUuid() == iParentGuid &&  ! aItem->iIsGroupChild() )
			{
				if ( aEvent ==  MFeedNotify::EAdded ) {
					iNew++;
					UpdateProgressL();
				}
				CFeedModelImpl::FeedItemEventImpl( aItem, aEvent );
			}
	}
	
	virtual TBool IsThisThreadL(const TDesC& aSubName) {
		CALLSTACKITEM_N(_CL("CCommentListModelImpl"), _CL("IsThisThreadL"));
		refcounted_ptr<CBBFeedItem> item( CFeedModelImpl::GetItemL( 0 ) );
		if (! item.get() ) item.reset( CFeedModelImpl::GetItemL( 1 ) );
		if ( ! item.get() ) return EFalse;
		
		{
			TBuf<50> this_subname; 
			if (item->iParentUuid().Length()) item->iParentUuid.IntoStringL(this_subname);
			else item->iUuid.IntoStringL(this_subname);
			if (aSubName != this_subname) return EFalse;
		}
		return ETrue;
	}
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData) {
		
		if (! (aName == KThreadRequestReplyTuple)) return;
		if (! IsThisThreadL(aSubName) ) return;
		
		const TBBInt* coming=bb_cast<TBBInt>(aData);
		if (!coming) return;
		
		if ((*coming)()==-1) {
			FetchingStatusL(_L("Looking up new comments"));
			return;
		}
		iComing=(*coming)();
		UpdateProgressL();
	}
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName) {
		if (! (aName == KThreadRequestTuple)) return;
		
		if (! IsThisThreadL(aSubName) ) return;
		
		FetchingStatusL(_L("Waiting for new comments"));
	}
	
	virtual void FetchingStatusL(const TDesC& aMsg) {
		//StatusPaneUtils::SetTitlePaneTextL(aMsg);
		iProgressModel.SetFetchMessageL( aMsg );
	}
	virtual void UpdateProgressL() {
		TBuf<30> msg;
		if (iComing==-2) return;
		if (iComing==0) {
			FetchingStatusL(_L("No new comments"));
			return;
		}
		msg=_L("Got ");
		msg.AppendNum(iNew);
		if (iComing>0) {
			msg.Append(_L("/"));
			msg.AppendNum(iComing);
		}
		msg.Append(_L(" item(s)"));
		FetchingStatusL(msg);
	}
	virtual void FetchThreadL() {
		if (iFetched) return;
		
		if (! iBBSubSession) {
			iBBSubSession=BBSession()->CreateSubSessionL(this);
			iBBSubSession->AddNotificationL(KThreadRequestReplyTuple, ETrue);
			iBBSubSession->AddNotificationL(KThreadRequestTuple, ETrue);
		}
		TUint id=0;
		refcounted_ptr<CBBFeedItem> item( GetItemL(1) );
		if (! item.get()) item.reset(GetItemL(0));
		if ( ! item.get() ) return;
		
		TBBThreadRequest tr;
		
		if (item->iParentUuid().Length()>0) {
			tr.iThreadOwner=item->iParentAuthorNick;
			tr.iPostUuid=item->iParentUuid;
			tr.iStreamDataId()=item->iStreamDataId();
		} else {
			tr.iThreadOwner=item->iAuthorNick;
			tr.iPostUuid=item->iUuid;
			tr.iStreamDataId()=item->iStreamDataId();
		}
		
		TBuf<50> subname; tr.iPostUuid.IntoStringL(subname);
		TTime expires=GetTime(); expires+=TTimeIntervalMinutes(10);
		id=iBBSubSession->PutRequestL(KThreadRequestTuple, subname, &tr, expires,
			KOutgoingTuples, ETrue);
			
		if (id==0) {
			FetchingStatusL(_L("Already fetching comments"));
		} else {
			FetchingStatusL(_L("Going to fetch comments"));
			iComing = -1;
			iNew = Count() - 1;
		}
		iFetched=ETrue;
	}
private:
	TGlobalId iParentGuid;
	CBBSubSession* iBBSubSession;
	TBool iFetched; TInt iNew; TInt iComing;
	CBBFeedItem* iParentItem;
	CProgressBarModel& iProgressModel;
};


EXPORT_C MFeedModel* CreateEverybodysFeedModelL(CFeedItemStorage& aFeedStorage)
{
	CALLSTACKITEMSTATIC_N(_CL(""), _CL("CreateEverybodysFeedModelL"));
	auto_ptr<CEverybodysFeedModelImpl> self( new (ELeave) CEverybodysFeedModelImpl(aFeedStorage ) );
	self->ConstructL();
	return self.release();
}


EXPORT_C MFeedModel* CreateAuthorFeedModelL(const TDesC& aNick,
											CFeedItemStorage& aFeedStorage,
											TBool aUserNick)
{
	CALLSTACKITEMSTATIC_N(_CL(""), _CL("CreateAuthorFeedModelL"));
	auto_ptr<CAuthorFeedModelImpl> self( new (ELeave) CAuthorFeedModelImpl( aNick, aFeedStorage, aUserNick ) );
	self->ConstructL();
	return self.release();
}


EXPORT_C MFeedModel* CreateFeedGroupModelL(TGlobalId aRootGuid,
										   CFeedItemStorage& aFeedStorage)
{
	CALLSTACKITEMSTATIC_N(_CL(""), _CL("CreateFeedGroupModelL"));
	auto_ptr<CFeedGroupModelImpl> self( new (ELeave)  CFeedGroupModelImpl( aRootGuid, aFeedStorage ) );
	self->ConstructL();
	return self.release();
}

EXPORT_C MFeedModel* CreateCommentListModelL(TGlobalId aParentGuid,
											 CFeedItemStorage& aFeedStorage, 
											 CProgressBarModel& aProgressModel)
{
	CALLSTACKITEMSTATIC_N(_CL(""), _CL("CreateCommentListModelL"));
	auto_ptr<CCommentListModelImpl> self( new (ELeave)  CCommentListModelImpl( aParentGuid, aFeedStorage, aProgressModel ) );
	self->ConstructL();
	return self.release();
}
