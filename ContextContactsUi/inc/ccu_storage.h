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

#ifndef CONTEXT_CCU_STORAGE_H_INCLUDED
#define CONTEXT_CCU_STORAGE_H_INCLUDED 1

#include "csd_feeditem.h"
#include "db.h"
#include "contextvariant.hrh"
#include "csd_connectionstate_enums.h"

#ifdef __JAIKU_PHOTO_DOWNLOAD__
#include "cn_http.h"
#endif

static const TErrorCode KFeedStorageCorrupt = { CONTEXT_UID_CONTEXTCONTACTSUI, KErrCorrupt };

class MFeedNotify {
public:
	enum TEvent {
		EMediaDownloadStateChanged,
		EThumbnailLoaded,
		EUnreadChanged,
		EChildCountChanged,
		EAdded,
		EChildAdded,
		EErrorUpdated,
		EPlaceholderFilled,
		EFeedItemUpdated,
		EFeedItemHidden,
		EFeedItemVisible,
		ELastFeedItemChanged,
		EFeedItemDeleted,
		EParentChanged,
		EMediaDownloadPauseChanged
	};
	virtual void FeedItemEvent(CBBFeedItem* aItem, TEvent aEvent) = 0;
	virtual void AuthorCountEvent(const TDesC& aAuthor,
		TInt aNewItemCount, TInt aNewUnreadCount) = 0;
};

typedef TBuf8<16> TGlobalId;

class CFeedItemStorage : public CCheckedActive {
public:
	enum TOrder {
		EAscending,
		EDescending
	};

	IMPORT_C static CFeedItemStorage* NewL(MApp_context& Context, CDb& Db, 
		MBBDataFactory* aBBFactory
#ifdef __JAIKU_PHOTO_DOWNLOAD__
		, HttpFactory aHttpFactory=0
		, const TDesC& aDownloadDbName=KNullDesC
#endif
		);

	// adding/modifying
	// you call addlocal if you made the item yourself
	virtual void AddLocalL(CBBFeedItem* aFeedItem) = 0; 

	virtual void MarkAsRead(CBBFeedItem* aFeedItem) = 0;
	virtual void MarkAsRead(const TDesC& aAuthor) = 0;
	virtual void MarkAsRead(const TGlobalId& aItemOrParent) = 0;
	virtual void MarkAllAsRead() = 0;
	virtual void RemoveFeedItemL(const CBBFeedItem * aFeedItem, TBool aMediaRemoveFile=EFalse)=0;

	enum TFromType { EByDateTime, EByAccess };
	virtual TBool RemoveFeedItemsL(TTime aFromTime,
		TFromType aFromType, TInt aMinimumToLeaveByAuthor=0,
		TInt aMaximumToRemove=0) = 0;
	virtual void StopBackgroundActivities() = 0;

	// you call update if you got the item from me
	virtual void UpdateFeedItemL(CBBFeedItem * aFeedItem) = 0;

	//virtual void UpdateFileName(const TGlobalId& aFeedItemId, const TDesC& aFileName, const TDesC& aContentType) = 0;
	//virtual void UpdateIcon(const TGlobalId & aFeedItemId, auto_ptr<CFbsBitmap>& aIcon) = 0;
	//virtual void UpdateError(const TGlobalId & aFeedItemId, TInt aErrorCode, const TDesC& aError) = 0;
	virtual void UpdateErrorInfo(const TGlobalId & aFeedItemId, 
		const CBBErrorInfo* aErrorInfo, TBool aWillRetry) = 0;

	virtual void CommitL() = 0;
	
	virtual TBool CompactIfHighWaterL() = 0;

	//virtual void SetThreadVisibleL(const TGlobalId& aFeedItemId, TBool visible) = 0;
	//virtual void SetAllThreadsVisibleL() = 0;
	//virtual TInt HasHiddenThreads()=0;

	// iteration
	virtual TBool FirstAllL(TBool aAscDate=EFalse, TBool aOverViewOnly=EFalse) = 0;
	virtual TBool FirstByParentL(const TGlobalId& aParent, TBool aAscDate=EFalse) = 0;
	virtual TBool FirstByAuthorL(const TDesC& aAuthor, TBool aAscDate=EFalse) = 0;

	virtual class TBookmark* Bookmark() = 0; // you get ownership, delete with NextL or ReleaseBookmark
	virtual TBool NextL(class TBookmark*& aBookmark) = 0;
	virtual void ReleaseBookmark(class TBookmark*& aBookmark) = 0;
	
	virtual TBool NextL() = 0;
	virtual TUint GetCurrentLocalIdL() = 0;
	virtual void GetCurrentGlobalIdL(TGlobalId& aInto) = 0;
	virtual TTime GetCurrentTimeStampL() = 0;
	
	// accessing
	virtual CBBFeedItem* GetCurrentL(TBool aIgnoreNotFound = EFalse) = 0; // addrefs
	virtual CBBFeedItem* GetByLocalIdL(TUint aLocalId, TBool aIgnoreNotFound = EFalse) = 0; // addrefs
	virtual CBBFeedItem* GetByGlobalIdL(const TGlobalId& aFeedItemId, TBool aIgnoreNotFound = EFalse) = 0; // addrefs
	virtual TBool CurrentIsGroupChild() = 0;
	virtual void SubscribeL(MFeedNotify* aNotify) = 0; // addrefs
	virtual void UnSubscribeL(MFeedNotify* aNotify) = 0; // addrefs
	virtual TBool GetFirstByCorrelationL(const TGlobalId& aCorrelation, TGlobalId &aPostIdInto) = 0;
	// you have to Release() the objects you get

	virtual void GetCountsByAuthorL(const TDesC& aAuthor, TInt& aItemCount, TInt& aUnreadCount) = 0;
	virtual void GetChildCountsL(const TGlobalId& aParent, TInt& aChildCount, TInt& aUnreadChildCount,
		TInt& aGroupChildCount, TBool aOverViewOnly=EFalse) = 0;
		
	virtual void DownloadMediaForFeedItemL(CBBFeedItem* aFeedItem, TBool aManual=EFalse) = 0;

	enum TDownloadMode
	{
		EUnknownDL = -1,
		EAutomaticDL = 0,
		EOnLookDL,
		EOnRequestDL
	};
	virtual void SetDownloadModeL( TDownloadMode aMode ) = 0;
	virtual TDownloadMode DownloadMode() const = 0;
	virtual void SetOfflineL(TBool aIsOffline) = 0;
	virtual TBool IsDownloading( PresencePublisher::EState& aReason) = 0;
	virtual class CDownloader* Downloader() = 0; // for unit testing
protected:
	CFeedItemStorage();
};

namespace FeedItem
{
	_LIT(KKindPresence, "presence"); 

	IMPORT_C TBool IsComment( CBBFeedItem& aItem );

	IMPORT_C TBool HasParent( CBBFeedItem& aItem );

	IMPORT_C void FindRootParentL(CBBFeedItem& aItem, TGlobalId& aRootId, CFeedItemStorage& aStorage);

	IMPORT_C TBool IsJaiku(CBBFeedItem& aItem);
}


#endif
