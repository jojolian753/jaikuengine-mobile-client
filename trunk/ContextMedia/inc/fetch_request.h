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

#ifndef CONTEXT_CM_FETCH_REQUEST_H_INCLUDED
#define CONTEXT_CM_FETCH_REQUEST_H_INCLUDED 1

#include "bbdata.h"
#include "bblist.h"
#include "concretedata.h"
#include "context_uids.h"

_LIT(KCMFetchItem, "thread");
_LIT(KCMFetchItemList, "fetch");
_LIT(KCMFetchPostRequest, "request");
_LIT(KCMFetchMediaRequest, "request");

const TComponentName KContextMediaComponent = { { CONTEXT_UID_CONTEXTMEDIA }, 1 };

const TTypeName KFetchItemType =	  { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 3, 1, 0 };
const TTypeName KFetchItemListType =	  { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 4, 1, 0 };
const TTypeName KFetchPostRequestType =	  { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 5, 1, 0 };
const TTypeName KFetchMediaRequestType =  { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 6, 1, 0 };
const TTypeName KMediaThumbType =         { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 7, 1, 0 };

const TTupleName KFetchPostRequestTuple = { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 5 };
const TTupleName KFetchMediaRequestTuple = { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 6 };

class TBBFetchItem : public TBBCompoundData {
public:
	TBBInt64 iThreadId;
	TBBInt64 iLastPostId;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;

	IMPORT_C TBBFetchItem& operator=(const TBBFetchItem& aRhs);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
public:
	IMPORT_C TBBFetchItem();
	IMPORT_C TBBFetchItem(TInt64 aThreadId, TInt64 aLastPostId);
	virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C bool operator==(const TBBFetchItem& aRhs) const;
};

class CBBFetchItemList : public CBBGenericList, public MBBDataFactory
{
public:
	IMPORT_C static CBBFetchItemList* NewL();
	IMPORT_C CBBFetchItemList();
	IMPORT_C void ConstructL();
	IMPORT_C TBBFetchItem* First();
	IMPORT_C TBBFetchItem* Next();
	IMPORT_C const TBBFetchItem*	First() const;
	IMPORT_C const TBBFetchItem*	Next() const;
	IMPORT_C void	AddItemL(TBBFetchItem* aData); // takes ownership
        IMPORT_C virtual const TTypeName& Type() const;
        IMPORT_C static const TTypeName& StaticType();
	IMPORT_C virtual void	AddItemL(HBufC*	aName, MBBData* aData); // takes ownership
	IMPORT_C TBool Equals(const MBBData* aRhs) const;

	IMPORT_C CBBFetchItemList& operator=(const CBBFetchItemList& aList);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
private:
	virtual MBBData* CreateBBDataL(const TTypeName& aType, const TDesC& aName, MBBDataFactory* aTopLevelFactory);
	IMPORT_C virtual TBool FixedType() const;

};

class CBBFetchPostRequest : public CBase, public TBBCompoundData  {
public:
	TBBInt	iIAPSetting;
	TBBLongString iTargetUrl;
	CBBFetchItemList * iFetchItemList;

	TBBInt		iErrorCode;
	TBBLongString	iErrorDescr;

	IMPORT_C void SetFetchItemList(const CBBFetchItemList* aList); // clones the list

	IMPORT_C void ConstructL();

        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;
	IMPORT_C CBBFetchPostRequest(TInt aIAPSetting, const TDesC& aTargetUrl);
	IMPORT_C CBBFetchPostRequest();
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C bool operator==(const CBBFetchPostRequest& aRhs) const;
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C CBBFetchPostRequest& operator=(const CBBFetchPostRequest& aRhs);

	IMPORT_C static CBBFetchPostRequest* NewL(TInt aIAPSetting, const TDesC& aTargetUrl);
	IMPORT_C ~CBBFetchPostRequest();
};

class CBBFetchMediaRequest : public CBase, public TBBCompoundData  {
public:
	TBBInt		iIAPSetting;
	TBBInt64	iPostId;
	TBBLongString	iTargetUrl;
	TBBBool		iForce;
	
	TBBLongString   iFileName;
	TBBShortString iContentType;
	CBBString8 * iMediaThumbnail;

	TBBInt		iErrorCode;
	TBBLongString	iErrorDescr;

        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C const MBBData* Part(TUint aPartNo) const;
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C bool operator==(const CBBFetchMediaRequest& aRhs) const;
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C CBBFetchMediaRequest& operator=(const CBBFetchMediaRequest& aRhs);

	IMPORT_C static CBBFetchMediaRequest* NewL(TInt aIAPSetting, TInt64 aPostId, 
		const TDesC& aTargetUrl, TBool aForce=EFalse, const class CFbsBitmap* aBitmap=0);
	IMPORT_C ~CBBFetchMediaRequest();

        IMPORT_C CFbsBitmap * MediaThumbnail() const; //caller responsible for destruction of CFbsBitmap
private:
	void ConstructL(const CFbsBitmap* aBitmap);
	CBBFetchMediaRequest(TInt aIAPSetting, TInt64 aPostId, const TDesC& aTargetUrl, TBool aForce);
	CBBFetchMediaRequest();

};

#endif
