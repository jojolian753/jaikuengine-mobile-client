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

#ifndef CCU_FEEDMODEL_H
#define CCU_FEEDMODEL_H

#include <e32base.h>
#include <bamdesca.h>
#include "ccu_storage.h"

enum TSpecialFeedItems
	{
		KFirstSpecialFeedItem = -104,
		KAuthorHeaderItem = -103,
		KMissingPostItem  = -102,
		KPostJaikuItem    = -101,
		KPostCommentItem  = -100,
		KLastSpecialFeedItem = -99,
	};

/**
 * MFeedModel provides unified interface to different feed models for feed views. 
 * Feed models use CFeedItemStorage to get view specific data and do caching of 
 * items 
 */ 
class MFeedModel 
{
 public:
	class MObserver 
	{
	public:
		virtual void ItemChanged(TInt aIndex, MFeedNotify::TEvent aEvent ) = 0;
	};	
	
	virtual ~MFeedModel() {}

	virtual void AddObserverL( MObserver* aObs ) = 0;
	virtual void RemoveObserver( MObserver* aObs ) = 0;

	virtual TUint GetId( TInt aIndex ) = 0;
	virtual TInt GetIndex(TUint aId) = 0;
	virtual CBBFeedItem* GetItemL( TInt aIndex ) = 0;
	virtual TInt Count() const = 0;

	virtual void MarkAsRead( TInt aIndex ) = 0;
};


IMPORT_C MFeedModel* CreateEverybodysFeedModelL(CFeedItemStorage& aFeedStorage);

IMPORT_C MFeedModel* CreateAuthorFeedModelL(const TDesC& aNick,
											CFeedItemStorage& aFeedStorage,
											TBool aIsUserNick = EFalse);

IMPORT_C MFeedModel* CreateFeedGroupModelL(TGlobalId aRootGuid,
										   CFeedItemStorage& aFeedStorage);


IMPORT_C MFeedModel* CreateCommentListModelL(TGlobalId aParentGuid,
											 CFeedItemStorage& aFeedStorage,
											 class CProgressBarModel& aProgressModel);

#endif // CCU_FEEDMODEL_H
