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

#ifndef CONTEXT_POST_H_INCLUDED
#define CONTEXT_POST_H_INCLUDED 1

#include "bbdata.h"
#include "concretedata.h"
#include "context_uids.h"
#include <gulicon.h>
#include "csd_presence.h"
#include "symbian_refcounted_ptr.h"

const TTupleName KPostTuple = { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 1 };

const TTypeName KPostType = { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 1, 2, 0 };
const TTypeName KPostType1 = { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 1, 1, 0 };
const TTypeName KSenderType = { { CONTEXT_UID_CONTEXTMEDIAFACTORY }, 2, 1, 0 };

class MPostDeletionNotify {
public:
	virtual void PostDeleted(const class CCMPost* aPost) = 0;
};

class TCMSender : public TBBCompoundData {
public:
	TBBShortString		iName;
	TBBShortString		iPhoneNo;
	TBBShortString		iJabberNick;
	TBBBtDeviceInfo		iBt;
	TBBShortString		iImei;

        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C TCMSender& operator=(const TCMSender& aInfo);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	const MBBData* Part(TUint aPartNo) const;

	IMPORT_C TCMSender();
	IMPORT_C bool operator==(const TCMSender& aRhs) const;
	virtual const TDesC& StringSep(TUint aBeforePart) const;

};

class CCMPost : public CBase, public TBBCompoundData, public MRefCounted {
public:
	TBBInt64		iParentId;
	TBBInt64		iPostId;
	TCMSender		iSender;
	TBBLongString		iMediaUrl;
	CBBString*		iBodyText;
	TBBTime			iTimeStamp;
	TBBShortString		iProject; // aware project name

	TBBInt			iUnreadCounter;
	
	TBBLongString		iMediaFileName;
	TBBUint			iLocalDatabaseId;
	CBBString*		iTag;
	CBBGeneralHolder	iPresence;
	TBBLongString		iErrorDescr;
	TBBInt			iErrorCode;
	TBBLongString		iContentType;

	enum TTags {
		ECountry	= 0x0001,
		ECity		= 0x0002,
		EBase		= 0x0004,
		ECell		= 0x0008,
		EBt		= 0x0010,
		ECalendar	= 0x0020,
		EGps		= 0x0040,
	};
	TBBInt			iIncludedTagsBitField;
	enum TSharing { EPublic, EFriendsAndFamily, EFriends, EFamily, EPrivate };
	TBBInt			iSharing;

	class CBBErrorInfo	*iErrorInfo; // stored separately in the post storage, not serialized

	IMPORT_C TInt GetThumbnailIndex() const;
	IMPORT_C const TDesC& LastPostAuthor() const;
	IMPORT_C const TTime& LastPostDate() const;
	IMPORT_C void SetLastPostInfo(const TDesC& aAuthor, const TTime& aDateTime);

        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C CCMPost& operator=(const CCMPost& aInfo);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	const MBBData* Part(TUint aPartNo) const;
	IMPORT_C static CCMPost* NewL(MBBDataFactory* aBBFactory, TInt bodysize=-1, TInt tagsize=-1); // addrefs
	IMPORT_C static CCMPost* NewL(MBBDataFactory* aBBFactory, TUint aVersion, TInt bodysize=-1, TInt tagsize=-1); // addrefs

	IMPORT_C ~CCMPost();
	IMPORT_C bool operator==(const CCMPost& aRhs) const;
	virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C void AddRef() const;
	IMPORT_C void Release() const;

	IMPORT_C TBool HasCrucialFields() const;
	IMPORT_C TBool HasMainFields() const;
	IMPORT_C virtual void	InternalizeL(RReadStream& aStream);

private:
	
	MBBData* GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto);
	IMPORT_C CCMPost(MBBDataFactory* aBBFactory, TUint aVersion);
	IMPORT_C void ConstructL(TInt bodysize, TInt tagsize);
	mutable TUint iRefCount;

	friend class CPostStorageImpl;
	IMPORT_C void SetThumbNailIndex(TInt aIconIdx);
	IMPORT_C void SetDeletionNotify(MPostDeletionNotify* aDeletionNotify);
	TInt iIconIdx;
	TBuf<50>	iLastPostAuthor;
	TTime		iLastPostDate;
	MPostDeletionNotify* iDeletionNotify;
	MBBDataFactory* iBBFactory;

	TUint		iUseVersion, iCreatedVersion;
};

#endif
