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

#ifndef CONTEXT_CSD_FEEDITEM_H_INCLUDED
#define CONTEXT_CSD_FEEDITEM_H_INCLUDED

#include "bbdata.h"
#include "concretedata.h"
#include "context_uids.h"
#include "csd_uuid.h"
#include "csd_uuid.h"
#include "csd_uuid.h"
#include "bberrorinfo.h"

_LIT(KFeedItem, "feeditem");

const TTypeName KFeedItemType = { { CONTEXT_UID_SENSORDATAFACTORY }, 54, 1, 0 };
const TTupleName KFeedItemTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 54 };


class MFeedItemDeletionNotify {
public:
	virtual void FeedItemDeleted(const class CBBFeedItem * aFeedItem) = 0;
};

class CBBFeedItem : public CBase, public MRefCounted,  public TBBCompoundData {
public:
	enum TMediaDownloadState {
		EUnknown,
		ENoMedia,
		ENotDownloading,
		EQueued,
		EDownloading,
		EMediaDownloaded,
		EDownloadErrorRetrying,
		EDownloadErrorFailed,
		EDownloadPausedOffline,
		EDownloadPausedNotConnecting,
	};

	TBBUUID	iParentUuid;
	TBBUUID	iUuid;
	TBBShortString	iAuthorNick;
	TBBLongString	iAuthorDisplayName;
	CBBString8	iThumbnailMbm;
	CBBString	iThumbnailUrl;
	TBBInt	iIconId;
	TBBTime	iCreated;
	CBBString	iLinkedUrl;
	CBBString	iContent;
	TBBLongString	iLocation;
	TBBBool	iFromServer;
	TBBShortString	iKind;
	TBBBool	iIsUnread;
	TBBBool	iIsGroupChild;
	TBBUUID	iCorrelation;
	TBBShortString	iStreamTitle;
	CBBString	iStreamUrl;
	TBBShortString	iChannel;
	TBBShortString	iParentAuthorNick;
	TBBLongString	iParentTitle;
	TBBInt	iStreamDataId;
	TBBLongString	iMediaFileName;
	TBBInt	iMediaDownloadState;

	TInt iUnreadChildCounter;
	TUint iLocalDatabaseId;
	TInt iGroupChildCount;
	TBool iDontShowInOverView;
	CBBErrorInfo* iErrorInfo;
	TInt iChildCount;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C const MBBData* Part(TUint aPartNo) const;
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C CBBFeedItem();
	IMPORT_C CBBFeedItem& operator=(const CBBFeedItem& aValue);
	IMPORT_C bool operator==(const CBBFeedItem& aRhs) const;

	IMPORT_C void AddRef() const;
	IMPORT_C void Release() const;
	IMPORT_C void SetDeletionNotify(MFeedItemDeletionNotify* aDeletionNotify);

	virtual ~CBBFeedItem();
private:

	mutable TUint iRefCount;
	MFeedItemDeletionNotify* iDeletionNotify;
};

#endif
