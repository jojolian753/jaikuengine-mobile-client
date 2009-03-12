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

#include "break.h"
#include "fetch_request.h"

_LIT(KThreadId, "threadid");
_LIT(KLastPostId, "last_packetid");

_LIT(KThreadFetch, "thread");
_LIT(KThreadFetchList, "fetch");

_LIT(KIAPSetting, "iap.setting");
_LIT(KUrl, "url");

_LIT(KSpace, " ");
_LIT(KComma, ",");

_LIT(KThumbnail, "thumbnail");

_LIT(KPostId, "postid");
_LIT(KTargetUrl, "url");
_LIT(KForce, "forcefetch");
_LIT(KErrCode, "err.code");
_LIT(KErrDes, "err.desc");
_LIT(KContentType, "type");
_LIT(KFileName, "filename");

#include <s32mem.h>
#include <fbs.h>

//TBBFetchItem
EXPORT_C const TTypeName& TBBFetchItem::Type() const
{
	return KFetchItemType;
}

EXPORT_C const TTypeName& TBBFetchItem::StaticType()
{
	return KFetchItemType;
}

EXPORT_C TBool TBBFetchItem::Equals(const MBBData* aRhs) const
{
	const TBBFetchItem *rhs=bb_cast<TBBFetchItem>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const MBBData* TBBFetchItem::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iThreadId;
	case 1:
		return &iLastPostId;
	default:
		return 0;
	}
}

EXPORT_C TBBFetchItem& TBBFetchItem::operator=(const TBBFetchItem& aRhs)
{
	iThreadId()=aRhs.iThreadId();
	iLastPostId()=aRhs.iLastPostId();
	return *this;
}

EXPORT_C TBBFetchItem::TBBFetchItem() : TBBCompoundData(KCMFetchItem),
	iThreadId(KThreadId), iLastPostId(KLastPostId)
{
}

EXPORT_C TBBFetchItem::TBBFetchItem(TInt64 aThreadId, TInt64 aLastPostId) : TBBCompoundData(KCMFetchItem),
	iThreadId(aThreadId, KThreadId), iLastPostId(aLastPostId, KLastPostId)
{
}

const TDesC& TBBFetchItem::StringSep(TUint /*aBeforePart*/) const
{
	return KSpace;
}

EXPORT_C bool TBBFetchItem::operator==(const TBBFetchItem& aRhs) const
{
	return (
		iThreadId==aRhs.iThreadId &&
		iLastPostId==aRhs.iLastPostId 
		);
}

EXPORT_C MBBData* TBBFetchItem::CloneL(const TDesC& /*Name*/) const
{
	TBBFetchItem* ret=new (ELeave) TBBFetchItem;
	*ret=*this;
	return ret;
}

//CBBFetchItemList

EXPORT_C CBBFetchItemList* CBBFetchItemList::NewL()
{
	auto_ptr<CBBFetchItemList> ret(new (ELeave) CBBFetchItemList());
	ret->ConstructL();
	return ret.release();
}

CBBFetchItemList::CBBFetchItemList() : CBBGenericList(KCMFetchItemList, KCMFetchItem, KSpace, this)
{
}

EXPORT_C void CBBFetchItemList::ConstructL()
{
	CBBGenericList::ConstructL();
}

EXPORT_C TBBFetchItem* CBBFetchItemList::First()
{
	return static_cast<TBBFetchItem*>(CBBGenericList::First());
}

EXPORT_C TBBFetchItem* CBBFetchItemList::Next()
{
	return static_cast<TBBFetchItem*>(CBBGenericList::Next());
}

EXPORT_C const TBBFetchItem*	CBBFetchItemList::First() const
{
	return static_cast<const TBBFetchItem*>(CBBGenericList::First());
}

EXPORT_C const TBBFetchItem*	CBBFetchItemList::Next() const
{
	return static_cast<const TBBFetchItem*>(CBBGenericList::Next());
}

EXPORT_C void	CBBFetchItemList::AddItemL(TBBFetchItem* aData)
{
	CBBGenericList::AddItemL(0, aData);
}

EXPORT_C const TTypeName& CBBFetchItemList::Type() const
{
	return KFetchItemListType;
}

EXPORT_C const TTypeName& CBBFetchItemList::StaticType()
{
	return KFetchItemListType;
}

EXPORT_C void CBBFetchItemList::AddItemL(HBufC*	aName, MBBData* aData)
{
	if (!aData || !(aData->Type()==KFetchItemType)) User::Leave(KErrNotSupported);
	CBBGenericList::AddItemL(aName, aData);
}

EXPORT_C TBool CBBFetchItemList::Equals(const MBBData* aRhs) const
{
	if (!aRhs) return EFalse;
	if (! (aRhs->Type() == Type()) ) return EFalse;
	const CBBFetchItemList* rhs=static_cast<const CBBFetchItemList*>(aRhs);
	const TBBFetchItem *l, *r;

	TInt lcount=Count(), rcount=rhs->Count();
	if (lcount!=rcount) return EFalse;

	for (l=First(), r=rhs->First(); l && r; l=Next(), r=rhs->Next()) {
		if ( ! l->Equals(r) ) return EFalse;
	}
	if (l || r) return EFalse;
	return ETrue;
}

EXPORT_C CBBFetchItemList& CBBFetchItemList::operator=(const CBBFetchItemList& aList)
{
	CBBGenericList::operator=(aList);
	return *this;
}

EXPORT_C MBBData* CBBFetchItemList::CloneL(const TDesC& ) const
{
	auto_ptr<CBBFetchItemList> ret(CBBFetchItemList::NewL());
	*ret=*this;
	return ret.release();
}

MBBData* CBBFetchItemList::CreateBBDataL(const TTypeName& , const TDesC& , MBBDataFactory* )
{
	return new (ELeave) TBBFetchItem();
}

EXPORT_C TBool CBBFetchItemList::FixedType() const
{
	return ETrue;
}

//CBBFetchPostRequest

EXPORT_C void CBBFetchPostRequest::SetFetchItemList(const CBBFetchItemList* aList)
{
	MBBData* d=aList->CloneL(aList->Name());
	CBBFetchItemList* l=bb_cast<CBBFetchItemList>(d);
	if (!l) {
		delete d;
		User::Leave(KErrNotSupported);
	}
	delete iFetchItemList;
	iFetchItemList=l;
}

EXPORT_C void CBBFetchPostRequest::ConstructL()
{
	iFetchItemList=CBBFetchItemList::NewL();
}

EXPORT_C const TTypeName& CBBFetchPostRequest::Type() const
{
	return KFetchPostRequestType;
}

EXPORT_C const TTypeName& CBBFetchPostRequest::StaticType()
{
	return KFetchPostRequestType;
}

EXPORT_C TBool CBBFetchPostRequest::Equals(const MBBData* aRhs) const
{
	const CBBFetchPostRequest *rhs=bb_cast<CBBFetchPostRequest>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const MBBData* CBBFetchPostRequest::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iIAPSetting;
	case 1:
		return &iTargetUrl;
	case 2:
		return iFetchItemList;
	case 3:
		return &iErrorCode;
	case 4:
		return &iErrorDescr;
	default:
		return 0;
	}
}

EXPORT_C CBBFetchPostRequest::CBBFetchPostRequest() : TBBCompoundData(KCMFetchPostRequest), 
	iIAPSetting(KIAPSetting), iTargetUrl(KUrl), iErrorDescr(KErrDes), iErrorCode(KErrCode)
{ 
}

EXPORT_C CBBFetchPostRequest::CBBFetchPostRequest(TInt aIAPSetting, const TDesC& aUrl) : TBBCompoundData(KCMFetchPostRequest),
	iIAPSetting(aIAPSetting, KIAPSetting), iTargetUrl(aUrl, KUrl), iErrorDescr(KErrDes), iErrorCode(KErrCode)
{
}

const TDesC& CBBFetchPostRequest::StringSep(TUint aBeforePart) const
{
	switch (aBeforePart) {
	case 0:
		return KNullDesC;
	case 1:
	case 2:
		return KComma;
	default:
		return KNullDesC;
	}
}

EXPORT_C bool CBBFetchPostRequest::operator==(const CBBFetchPostRequest& aRhs) const
{
	return (iIAPSetting==aRhs.iIAPSetting &&
		iTargetUrl==aRhs.iTargetUrl &&
		iFetchItemList->Equals(aRhs.iFetchItemList) &&
		iErrorCode()==aRhs.iErrorCode() &&
		iErrorDescr()==aRhs.iErrorDescr()
		);
}

EXPORT_C MBBData* CBBFetchPostRequest::CloneL(const TDesC& /*Name*/) const
{
	auto_ptr<CBBFetchPostRequest> ret(new (ELeave) CBBFetchPostRequest);
	*ret=*this;
	return ret.release();
}

EXPORT_C CBBFetchPostRequest& CBBFetchPostRequest::operator=(const CBBFetchPostRequest& aRhs)
{
	MBBData* d=aRhs.iFetchItemList->CloneL(aRhs.iFetchItemList->Name());
	CBBFetchItemList* l=bb_cast<CBBFetchItemList>(d);
	if (!l) {
		delete d;
		User::Leave(KErrNotSupported);
	}
	delete iFetchItemList;
	iFetchItemList=l;

	iIAPSetting()=aRhs.iIAPSetting();
	iTargetUrl()=aRhs.iTargetUrl();
	iErrorCode()=aRhs.iErrorCode();
	iErrorDescr()=aRhs.iErrorDescr();
	
	return *this;
}

EXPORT_C CBBFetchPostRequest* CBBFetchPostRequest::NewL(TInt aIAPSetting, const TDesC& aTargetUrl)
{
	auto_ptr<CBBFetchPostRequest> ret(new (ELeave) CBBFetchPostRequest(aIAPSetting, aTargetUrl));
	ret->ConstructL();
	return ret.release();
}

EXPORT_C CBBFetchPostRequest::~CBBFetchPostRequest()
{
	delete iFetchItemList;
}

EXPORT_C const TTypeName& CBBFetchMediaRequest::Type() const
{
	return KFetchMediaRequestType;
}

EXPORT_C const TTypeName& CBBFetchMediaRequest::StaticType()
{
	return KFetchMediaRequestType;
}

EXPORT_C TBool CBBFetchMediaRequest::Equals(const MBBData* aRhs) const
{
	const CBBFetchMediaRequest*rhs=bb_cast<CBBFetchMediaRequest>(aRhs);
	return (rhs && *rhs == *this );
}

EXPORT_C const MBBData* CBBFetchMediaRequest::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iIAPSetting;
	case 1:
		return &iPostId;
	case 2:
		return &iTargetUrl;
	case 3:
		return &iForce;
	case 4:
		return &iErrorCode;
	case 5:
		return &iErrorDescr;
	case 6:
		return &iContentType;
	case 7:
		return iMediaThumbnail;
	case 8:
		return &iFileName;
	default:
		return 0;
	}
}

EXPORT_C const TDesC& CBBFetchMediaRequest::StringSep(TUint ) const
{
	return KSpace;
}

EXPORT_C bool CBBFetchMediaRequest::operator==(const CBBFetchMediaRequest& aRhs) const
{
	return (
		iIAPSetting==aRhs.iIAPSetting &&
		iPostId==aRhs.iPostId &&
		iTargetUrl==aRhs.iTargetUrl &&
		iForce==aRhs.iForce &&
		iErrorCode==aRhs.iErrorCode &&
		iErrorDescr==aRhs.iErrorDescr &&
		iContentType==aRhs.iContentType &&
		*iMediaThumbnail == *(aRhs.iMediaThumbnail) &&
		iFileName==aRhs.iFileName
		);
}

EXPORT_C MBBData* CBBFetchMediaRequest::CloneL(const TDesC& /*Name*/) const
{
	auto_ptr<CBBFetchMediaRequest> p(CBBFetchMediaRequest::NewL(iIAPSetting(), iPostId(), iTargetUrl(), iForce()));
	*p=*this;
	return p.release();
}

EXPORT_C CBBFetchMediaRequest& CBBFetchMediaRequest::operator=(const CBBFetchMediaRequest& aRhs)
{
	if (aRhs.iMediaThumbnail) {
		auto_ptr<CBBString8> s(CBBString8::NewL( KThumbnail, (*aRhs.iMediaThumbnail)().Length() ));
		s->Append((*aRhs.iMediaThumbnail)());
		delete iMediaThumbnail; iMediaThumbnail=s.release();
	} else {
		delete iMediaThumbnail; iMediaThumbnail=0;
	}

	iIAPSetting()=aRhs.iIAPSetting();
	iPostId()=aRhs.iPostId();
	iTargetUrl()=aRhs.iTargetUrl();
	iForce()=aRhs.iForce();
	iErrorCode()=aRhs.iErrorCode();
	iErrorDescr()=aRhs.iErrorDescr();
	iContentType()=aRhs.iContentType();
	iFileName()=aRhs.iFileName();

	return *this;
}

EXPORT_C CBBFetchMediaRequest* CBBFetchMediaRequest::NewL(TInt aIAPSetting, TInt64 aPostId, const TDesC& aTargetUrl, TBool aForce, const CFbsBitmap* aBitmap)
{
	auto_ptr<CBBFetchMediaRequest> ret(new (ELeave) CBBFetchMediaRequest(aIAPSetting, aPostId, aTargetUrl, aForce));
	ret->ConstructL(aBitmap);
	return ret.release();
}

EXPORT_C CBBFetchMediaRequest::~CBBFetchMediaRequest()
{
	delete iMediaThumbnail;
}

EXPORT_C CFbsBitmap * CBBFetchMediaRequest::MediaThumbnail() const
{
	if (iMediaThumbnail->iBuf->Des().Length()==0) return 0;

	RDesReadStream rStream;
	rStream.Open(*(iMediaThumbnail->iBuf));
		
	auto_ptr<CFbsBitmap> aBitmap (new (ELeave) CFbsBitmap);
	aBitmap->InternalizeL(rStream);
	rStream.Close();

	return aBitmap.release();
	
	//TPckgBuf<CFbsBitmap> xBitmap;
	//xBitmap.Append(iMediaThumbnail->iBuf->Des());
	//return &(xBitmap());
}


void CBBFetchMediaRequest::ConstructL(const CFbsBitmap* aBitmap)
{
	iMediaThumbnail = CBBString8::NewL(KThumbnail, 1024);
	TInt err=KErrNone;
	if (aBitmap) {
		{
			TPtr8 bufp(iMediaThumbnail->iBuf->Des());
			RDesWriteStream wStream;
			wStream.Open(bufp);
			CC_TRAPD(err, aBitmap->ExternalizeL(wStream));
			if (err==KErrNone) {
				wStream.CommitL();
				wStream.Close();
			}
		}
		while (err==KErrOverflow) {
			TInt size = iMediaThumbnail->iBuf->Des().MaxLength() *2;
			delete iMediaThumbnail; iMediaThumbnail=0;
			iMediaThumbnail = CBBString8::NewL(KThumbnail, size);
			TPtr8 bufp(iMediaThumbnail->iBuf->Des());
			RDesWriteStream wStream;
			wStream.Open(bufp);
			CC_TRAP(err, aBitmap->ExternalizeL(wStream));
			if (err==KErrNone)  {
				wStream.CommitL();
				wStream.Close();
			}
		}
	}

	RDebug::Print(_L("%d"), iMediaThumbnail->iBuf->Des().Length());
	
	/*if (aBitmap) {
		TPckg<CFbsBitmap> xBitmap(*aBitmap);
		iMediaThumbnail = CBBString8::NewL(KThumbnail, xBitmap.Size());
		iMediaThumbnail->Append(xBitmap);
	} else {
		iMediaThumbnail = CBBString8::NewL(KThumbnail, 0);
	}*/
}

CBBFetchMediaRequest::CBBFetchMediaRequest(TInt aIAPSetting, TInt64 aPostId, const TDesC& aTargetUrl, TBool aForce) :
TBBCompoundData(KCMFetchMediaRequest), iIAPSetting(aIAPSetting, KIAPSetting), iPostId(aPostId, KPostId), iTargetUrl(aTargetUrl, KTargetUrl), iForce(aForce, KForce),
iErrorCode(KErrCode), iErrorDescr(KErrDes), iContentType(KContentType), iFileName(KFileName)
{
}



CBBFetchMediaRequest::CBBFetchMediaRequest() : TBBCompoundData(KCMFetchMediaRequest), iIAPSetting(KIAPSetting), iPostId(KPostId), 
iTargetUrl(KTargetUrl), iForce(KForce), iErrorCode(KErrCode), iErrorDescr(KErrDes), iContentType(KContentType), iFileName(KFileName)
{
}
