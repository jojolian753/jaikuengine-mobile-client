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

#include "cm_post.h"
#include "bberrorinfo.h"

EXPORT_C const TTypeName& CCMPost::Type() const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("Type"));

	return KPostType;
}

EXPORT_C const TTypeName& CCMPost::StaticType()
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("StaticType"));

	return KPostType;
}

EXPORT_C TBool CCMPost::Equals(const MBBData* aRhs) const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("Equals"));

	const CCMPost*rhs=bb_cast<CCMPost>(aRhs);
	return (rhs && *rhs == *this );
}

_LIT(KBody, "description");
_LIT(KPost, "packet");
_LIT(KParentId, "threadid");
_LIT(KPostId, "packetid");
_LIT(KAuthorName, "author.name");
_LIT(KAuthorPhoneNo, "author.phone");
_LIT(KMediaUrl, "media.url");
_LIT(KMediaFileName, "media.filename");
_LIT(KTimeStamp, "datetime");
_LIT(KUnreadCounter, "unread_counter");
_LIT(KDatabaseId, "dbid");
_LIT(KTag, "tag");
_LIT(KProject, "project");
_LIT(KPostErrorDescr, "errordescr");
_LIT(KPostErrorCode, "errorcode");
_LIT(KContentType, "contenttype");
_LIT(KIncludedTags, "included_tags");
_LIT(KSharing, "sharing");

EXPORT_C CCMPost& CCMPost::operator=(const CCMPost& aInfo)
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("operator"));

	auto_ptr<CBBString> s(CBBString::NewL( KBody, (*aInfo.iBodyText)().Length() ));
	auto_ptr<CBBString> t(CBBString::NewL( KTag, (*aInfo.iTag)().Length() ));

	s->Append((*aInfo.iBodyText)());
	t->Append((*aInfo.iTag)());

	if (aInfo.iPresence()) {
		refcounted_ptr<CBBPresence> p( bb_cast<CBBPresence>(aInfo.iPresence()->CloneL(KNullDesC)));
		iPresence.SetValue(p.release());
	} else {
		iPresence.SetValue(NULL);
	}

	delete iBodyText; iBodyText=s.release();
	delete iTag; iTag=t.release();

	iParentId()=aInfo.iParentId();
	iPostId()=aInfo.iPostId();
	iSender=aInfo.iSender;
	iMediaUrl()=aInfo.iMediaUrl();
	iTimeStamp()=aInfo.iTimeStamp();
	iUnreadCounter()=aInfo.iUnreadCounter();
	iMediaFileName()=aInfo.iMediaFileName();
	iLocalDatabaseId()=aInfo.iLocalDatabaseId();
	iProject()=aInfo.iProject();
	iErrorDescr()=aInfo.iErrorDescr();
	iErrorCode()=aInfo.iErrorCode();
	iContentType()=aInfo.iContentType();
	iIncludedTagsBitField()=aInfo.iIncludedTagsBitField();
	iSharing()=aInfo.iSharing();

	return *this;
}

EXPORT_C MBBData* CCMPost::CloneL(const TDesC& ) const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("CloneL"));

	auto_ptr<CCMPost> p(CCMPost::NewL(iBBFactory, (*iBodyText)().Length(), (*iTag)().Length()));
	*p=*this;
	return p.release();
}

const MBBData* CCMPost::Part(TUint aPartNo) const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("Part"));

	switch(aPartNo) {
	case 0:
		return &iParentId;
	case 1:
		return &iPostId;
	case 2:
		return &iSender;
	case 3:
		return &iMediaUrl;
	case 4:
		return iBodyText;
	case 5:
		return &iTimeStamp;
	case 6:
		return &iUnreadCounter;
	case 7:
		return &iMediaFileName;
	case 8:
		return &iLocalDatabaseId;
	case 9:
		return iTag;
	case 10:
		return &iPresence;
	case 11:
		return &iProject;
	case 12:
		return &iErrorDescr;
	case 13:
		return &iErrorCode;
	case 14:
		return &iContentType;
	default:
		if (iUseVersion>1) {
			if (aPartNo==15) return &iIncludedTagsBitField;
			if (aPartNo==16) return &iSharing;
		}
		return 0;
	}
}

EXPORT_C TBool CCMPost::HasCrucialFields() const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("HasCrucialFields"));

	// parent and post -> to be able to place
	// it in a thread
	TUint crucial=0x3; // 0011
	return ( (crucial & iReadParts) == crucial );
}

EXPORT_C TBool CCMPost::HasMainFields() const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("HasMainFields"));

	// parent, post, sender, url, time
	TUint mainfields=0x2f; // 0010 1111;
	return ( (mainfields & iReadParts) == mainfields );
}

EXPORT_C CCMPost::CCMPost(MBBDataFactory* aBBFactory, TUint aVersion) : TBBCompoundData(KPost),
	iParentId(KParentId), iPostId(KPostId), 
	iMediaUrl(KMediaUrl),
	iTimeStamp(KTimeStamp),
	iUnreadCounter(KUnreadCounter), iMediaFileName(KMediaFileName),
	iLocalDatabaseId(KDatabaseId), iIconIdx(-1), iPresence(KPresence, aBBFactory),
	iBBFactory(aBBFactory), iProject(KProject), iErrorDescr(KPostErrorDescr),
	iErrorCode(KPostErrorCode), iContentType(KContentType), iLastPostAuthor(KNullDesC),
	iIncludedTagsBitField(0xffffff, KIncludedTags), iSharing(EPublic, KSharing), iUseVersion(2),
	iCreatedVersion(aVersion)
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("CCMPost"));

	TTime now=GetTime();
	iLastPostDate=now;
}

EXPORT_C void CCMPost::InternalizeL(RReadStream& aStream)
{
	iUseVersion=iCreatedVersion;
	TBBCompoundData::InternalizeL(aStream);
	iUseVersion=2;
}

EXPORT_C CCMPost::~CCMPost()
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("~CCMPost"));

	if (iDeletionNotify) iDeletionNotify->PostDeleted(this);
	delete iBodyText;
	delete iTag;
	if (iErrorInfo) iErrorInfo->Release();
}

EXPORT_C void CCMPost::ConstructL(TInt bodysize, TInt tagsize)
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("ConstructL"));

	if (bodysize<=0) bodysize=128;
	if (tagsize<=0) tagsize=32;

	iBodyText=CBBString::NewL(KBody, bodysize);
	iTag=CBBString::NewL(KTag, tagsize);
}

EXPORT_C bool CCMPost::operator==(const CCMPost& aRhs) const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("operator"));

	return (
		iParentId == aRhs.iParentId &&
		iPostId == aRhs.iPostId &&
		iSender == aRhs.iSender &&
		iMediaUrl == aRhs.iMediaUrl &&
		*iBodyText == *(aRhs.iBodyText) &&
		iTimeStamp == aRhs.iTimeStamp &&
		iUnreadCounter == aRhs.iUnreadCounter &&
		iMediaFileName == aRhs.iMediaFileName &&
		iLocalDatabaseId == aRhs.iLocalDatabaseId &&
		*iTag == *(aRhs.iTag) && 
		(
			(! iPresence() && !(aRhs.iPresence())) ||
			(iPresence() && iPresence()->Equals(aRhs.iPresence()))
		) &&
		iProject == aRhs.iProject &&
		iErrorDescr == aRhs.iErrorDescr &&
		iErrorCode == aRhs.iErrorCode &&
		iContentType == aRhs.iContentType &&
		iIncludedTagsBitField() == aRhs.iIncludedTagsBitField() &&
		iSharing() == aRhs.iSharing() &&
		(
			(! iErrorInfo && !aRhs.iErrorInfo) ||
			iErrorInfo->Equals(aRhs.iErrorInfo)
		)
		);
}

_LIT(KSpace, " ");

const TDesC& CCMPost::StringSep(TUint ) const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("StringSep"));

	return KSpace;
}

EXPORT_C CCMPost* CCMPost::NewL(MBBDataFactory* aBBFactory, TInt bodysize, TInt tagsize)
{
	return NewL(aBBFactory, 2, bodysize, tagsize);
}

EXPORT_C CCMPost* CCMPost::NewL(MBBDataFactory* aBBFactory, TUint aVersion, TInt bodysize, TInt tagsize)
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("NewL"));

	auto_ptr<CCMPost> ret(new (ELeave) CCMPost(aBBFactory, aVersion));
	ret->ConstructL(bodysize, tagsize);
	ret->AddRef();
	return ret.release();
}


EXPORT_C void CCMPost::AddRef() const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("AddRef"));

#ifdef __WINS__
	if (iPostId()==TInt64(0)) {
		TInt x;
		x=0;
	}
#endif
	++iRefCount;
}

EXPORT_C void CCMPost::Release() const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("Release"));

#ifdef __WINS__
	if (iPostId()==TInt64(0)) {
		TInt x;
		x=0;
	}
#endif
	--iRefCount;
	if (iRefCount<0) {
		User::Panic(_L("CONTEXTMEDIA"), 1);
	}
	if (iRefCount==0) 
		delete this;
}

EXPORT_C TInt CCMPost::GetThumbnailIndex() const
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("GetThumbnailIndex"));

	return iIconIdx;
}

EXPORT_C void CCMPost::SetDeletionNotify(MPostDeletionNotify* aDeletionNotify)
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("SetDeletionNotify"));

	iDeletionNotify=aDeletionNotify;
}

EXPORT_C void CCMPost::SetThumbNailIndex(TInt aIconIdx)
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("SetThumbNailIndex"));

	iIconIdx=aIconIdx;
}

EXPORT_C const TTypeName& TCMSender::Type() const
{
	CALLSTACKITEM_N(_CL("TCMSender"), _CL("Type"));

	return KSenderType;
}

EXPORT_C const TTypeName& TCMSender::StaticType()
{
	CALLSTACKITEM_N(_CL("TCMSender"), _CL("StaticType"));

	return KSenderType;
}

EXPORT_C TBool TCMSender::Equals(const MBBData* aRhs) const
{
	CALLSTACKITEM_N(_CL("TCMSender"), _CL("Equals"));

	const TCMSender* rhs=bb_cast<TCMSender>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C TCMSender& TCMSender::operator=(const TCMSender& aInfo)
{
	CALLSTACKITEM_N(_CL("TCMSender"), _CL("operator"));

	iName()=aInfo.iName();
	iPhoneNo()=aInfo.iPhoneNo();
	iJabberNick()=aInfo.iJabberNick();
	iBt=aInfo.iBt;
	iImei()=aInfo.iImei();

	return *this;
}

EXPORT_C MBBData* TCMSender::CloneL(const TDesC& ) const
{
	CALLSTACKITEM_N(_CL("TCMSender"), _CL("CloneL"));

	TCMSender* ret=new (ELeave) TCMSender;
	*ret=*this;
	return ret;
}

const MBBData* TCMSender::Part(TUint aPartNo) const
{
	CALLSTACKITEM_N(_CL("TCMSender"), _CL("Part"));

	switch(aPartNo) {
	case 0:
		return &iName;
	case 1:
		return &iPhoneNo;
	case 2:
		return &iJabberNick;
	case 3:
		return &iBt;
	case 4:
		return &iImei;
	default:
		return 0;
	}
}

EXPORT_C const TDesC& CCMPost::LastPostAuthor() const
{
	return iLastPostAuthor;
}

EXPORT_C const TTime& CCMPost::LastPostDate() const
{
	return iLastPostDate;
}

EXPORT_C void CCMPost::SetLastPostInfo(const TDesC& aAuthor, const TTime& aDateTime)
{
	iLastPostAuthor=aAuthor.Left(50);
	iLastPostDate=aDateTime;
}

_LIT(KName, "authorname");
_LIT(KPhone, "phoneno");
_LIT(KImei, "imei");
_LIT(KSender, "sender");
_LIT(KJabber, "jabbernick");

EXPORT_C TCMSender::TCMSender() : TBBCompoundData(KSender), iName(KName), 
	iPhoneNo(KPhone), iJabberNick(KJabber), iImei(KImei) { }

EXPORT_C bool TCMSender::operator==(const TCMSender& aRhs) const
{
	CALLSTACKITEM_N(_CL("TCMSender"), _CL("operator"));

	return (
		iName == aRhs.iName &&
		iPhoneNo == aRhs.iPhoneNo &&
		iJabberNick == aRhs.iJabberNick &&
		iBt == aRhs.iBt &&
		iImei == aRhs.iImei
	);
}

const TDesC& TCMSender::StringSep(TUint ) const
{
	CALLSTACKITEM_N(_CL("TCMSender"), _CL("StringSep"));

	return KSpace;
}

MBBData* CCMPost::GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto)
{
	CALLSTACKITEM_N(_CL("CCMPost"), _CL("GetPart"));

	MBBData* part=TBBCompoundData::GetPart(aName, aType, aPartNoInto);
	if (!part) {
		if (aName.Left(7).CompareF(_L("presenc")) == 0) {
			iPresence.SetValue(iBBFactory->CreateBBDataL(aType, aName, iBBFactory));
			aPartNoInto=10;
			part=&iPresence;
		}
	}
	return part;
}
