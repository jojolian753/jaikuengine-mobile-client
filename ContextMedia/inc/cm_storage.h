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

#ifndef CONTEXT_STORAGE_H_INCLUDED
#define CONTEXT_STORAGE_H_INCLUDED 1

#include "cm_post.h"
#include "db.h"

// these are related to definitions in ::constructL
static const TInt KNoIconIndex=-1;
static const TInt KUnknownIconIndex=0;
static const TInt KPlaceholderIconIndex=0;
static const TInt KReplyIconIndex=1;
static const TInt KAudioIconIndex=2;
static const TInt KVideoIconIndex=3;
static const TInt KNewThreadIconIndex=4;
static const TInt KUseCodeIconIndex=5;
static const TInt KErrorIconIndex=6;

class MPostNotify {
public:
	enum TEvent {
		EMediaLoaded,
		EThumbnailLoaded,
		EUnreadChanged,
		EChildAdded,
		EErrorUpdated,
		EPlaceholderFilled,
		EPostUpdated,
		EPostHidden,
		EPostVisible,
		ELastPostChanged,
		EPostDeleted,
		EParentChanged
	};
	virtual void PostEvent(CCMPost* aParent, CCMPost* aChild, TEvent aEvent) = 0;
	// aParent is NULL for subscriptions on the individual post
};

IMPORT_C class CCMNetwork* NewDummyNetworkL();

class CPostStorage : public CCheckedActive {
public:
	enum TSortBy {
		EByDate,
		EByAuthor,
		EByUnread,
		EByTitle,
		EByLastPost,
		EAll
	};
	enum TOrder {
		EAscending,
		EDescending
	};
	enum TSpecialThreads {
		ERoot = 0,
		EDrafts = 1,
		EMediaPool = 2,
		EQueue = 3,
		EFailed = 4,
		EPublished = 5,
		ENotPublished = 7,
		ETrash = 9
	};

	IMPORT_C static CPostStorage* NewL(MApp_context& Context, CDb& Db, MBBDataFactory* aBBFactory);

	virtual CArrayPtr<CGulIcon>* IconArray() = 0;

	// adding/modifying
	virtual void AddLocalL(CCMPost* aPost, auto_ptr<CFbsBitmap>& aIcon) = 0; 
		// takes ownership of icon 
		// you have to call SubscribeL manually after this, if keeping
		// the object, you can do that when getting the child addition event
	virtual void MarkAsRead(CCMPost* aPost) = 0;
	virtual void RemovePostL(const CCMPost * aPost, TBool aMediaRemoveFile=EFalse)=0;

	enum TFromType { EByDateTime, EByUpdated };
	virtual void RemovePostsL(TInt64 aThreadId, TTime aFromTime,
		TFromType aFromType) = 0;
	virtual void MovePostsL(TInt64 aFromThreadId, TInt64 aToThreadId,
		TTime aFromTime,
		TFromType aFromType) = 0;

	virtual void RereadUnreadL() = 0;

	virtual void AddPlaceHolderL(TInt64 aThreadId) = 0;
	virtual void AddNewThreadPlaceHolderL(TInt64 aThreadId) = 0;
	virtual void AddErrorPlaceHolderL(TInt64 aThreadId) = 0;
	virtual TBool IsPlaceHolderL(TInt64 aThreadId) = 0;

	virtual void UpdatePostL(CCMPost * aPost) = 0;
	virtual void UpdateFileName(TInt64 aPostId, const TDesC& aFileName, const TDesC& aContentType) = 0;
	virtual void UpdateIcon(TInt64 aPostId, auto_ptr<CFbsBitmap>& aIcon) = 0;
	virtual void UpdateError(TInt64 aPostId, TInt aErrorCode, const TDesC& aError) = 0;
	virtual void UpdateErrorInfo(TInt64 aPostId, const CBBErrorInfo* aErrorInfo) = 0;

	virtual void CommitL() = 0;

	virtual void SetThreadVisibleL(TInt64 aPostId, TBool visible) = 0;
	virtual void SetAllThreadsVisibleL() = 0;
	virtual TInt HasHiddenThreads()=0;

	// iteration
	virtual TBool FirstL(const TInt64& aParentId, TSortBy aSortBy, TOrder aSortOrder, TBool aOnlyUnread, TBool aOnlyVisible=ETrue) = 0;

	// iterates all non-top-level posts
	virtual TBool FirstAllL() = 0;
	virtual TBool NextL() = 0;
	virtual TUint GetCurrentIndexL() = 0;
	virtual TInt64 GetCurrentIdL() = 0;
	
	virtual TInt64 GetLastPostId(TInt64 aParentId) = 0;
	virtual TInt64 GetParentId(TInt64 aPostId) = 0;
	
	// accessing
	virtual CCMPost* GetCurrentL(MPostNotify* aNotify) = 0; // addrefs, sets up for notification
	virtual CCMPost* GetByIndexL(MPostNotify* aNotify, TUint aIdx) = 0; // addrefs, sets up for notification
	virtual CCMPost* GetByPostIdL(MPostNotify* aNotify, TInt64 aPostId) = 0; // addrefs, sets up for notification
	virtual void SubscribeL(CCMPost* aPost, MPostNotify* aNotify) = 0; // addrefs, sets up for notification
	virtual void Release(CCMPost* aPost, MPostNotify* aNotify) = 0; // releases
	virtual CCMPost* GetRootL(MPostNotify* aNotify) = 0; // sets up for notification 

	// Special PostId
	IMPORT_C static TInt64 RootId(); 
	IMPORT_C static TInt64 MediaPoolId(); // a thread that contains posts representing all media on the phone
	IMPORT_C static TInt64 AddMeToMediaPoolId();

protected:
	CPostStorage();
};

#endif
