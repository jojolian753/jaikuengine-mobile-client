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

#include "csd_feeditem.h"


EXPORT_C const TTypeName& CBBFeedItem::Type() const
{
	return KFeedItemType;
}

EXPORT_C const TTypeName& CBBFeedItem::StaticType()
{
	return KFeedItemType;
}

EXPORT_C TBool CBBFeedItem::Equals(const MBBData* aRhs) const
{
	const CBBFeedItem* rhs=bb_cast<CBBFeedItem>(aRhs);
	return (rhs && *rhs==*this);
}

EXPORT_C MBBData* CBBFeedItem::CloneL(const TDesC&) const
{
	bb_auto_ptr<CBBFeedItem> ret(new (ELeave) CBBFeedItem());
	*ret=*this;
	return ret.release();
}
_LIT(KParentUuid, "parentuuid");
_LIT(KUuid, "uuid");
_LIT(KAuthorNick, "authornick");
_LIT(KAuthorDisplayName, "authordisplayname");
_LIT(KThumbnailMbm, "thumbnailmbm");
_LIT(KThumbnailUrl, "thumbnailurl");
_LIT(KIconId, "iconid");
_LIT(KCreated, "created");
_LIT(KLinkedUrl, "linkedurl");
_LIT(KContent, "content");
_LIT(KLocation, "location");
_LIT(KFromServer, "fromserver");
_LIT(KKind, "kind");
_LIT(KIsUnread, "isunread");
_LIT(KIsGroupChild, "isgroupchild");
_LIT(KCorrelation, "correlation");
_LIT(KStreamTitle, "streamtitle");
_LIT(KStreamUrl, "streamurl");
_LIT(KChannel, "channel");
_LIT(KParentAuthorNick, "parentauthornick");
_LIT(KParentTitle, "parenttitle");
_LIT(KStreamDataId, "streamdataid");
_LIT(KMediaFileName, "mediafilename");
_LIT(KMediaDownloadState, "mediadownloadstate");

EXPORT_C CBBFeedItem::CBBFeedItem() : TBBCompoundData(KFeedItem) , iParentUuid(KParentUuid), iUuid(KUuid), iAuthorNick(KAuthorNick), iAuthorDisplayName(KAuthorDisplayName), iThumbnailMbm(KThumbnailMbm), iThumbnailUrl(KThumbnailUrl), iIconId(KIconId), iCreated(KCreated), iLinkedUrl(KLinkedUrl), iContent(KContent), iLocation(KLocation), iFromServer(KFromServer), iKind(KKind), iIsUnread(KIsUnread), iIsGroupChild(KIsGroupChild), iCorrelation(KCorrelation), iStreamTitle(KStreamTitle), iStreamUrl(KStreamUrl), iChannel(KChannel), iParentAuthorNick(KParentAuthorNick), iParentTitle(KParentTitle), iStreamDataId(KStreamDataId), iMediaFileName(KMediaFileName), iMediaDownloadState(KMediaDownloadState), iRefCount(1) { }

EXPORT_C bool CBBFeedItem::operator==(const CBBFeedItem& aRhs) const
{
	return (iParentUuid == aRhs.iParentUuid &&
	iUuid == aRhs.iUuid &&
	iAuthorNick == aRhs.iAuthorNick &&
	iAuthorDisplayName == aRhs.iAuthorDisplayName &&
	iThumbnailMbm == aRhs.iThumbnailMbm &&
	iThumbnailUrl == aRhs.iThumbnailUrl &&
	iIconId == aRhs.iIconId &&
	iCreated == aRhs.iCreated &&
	iLinkedUrl == aRhs.iLinkedUrl &&
	iContent == aRhs.iContent &&
	iLocation == aRhs.iLocation &&
	iFromServer == aRhs.iFromServer &&
	iKind == aRhs.iKind &&
	iIsUnread == aRhs.iIsUnread &&
	iIsGroupChild == aRhs.iIsGroupChild &&
	iCorrelation == aRhs.iCorrelation &&
	iStreamTitle == aRhs.iStreamTitle &&
	iStreamUrl == aRhs.iStreamUrl &&
	iChannel == aRhs.iChannel &&
	iParentAuthorNick == aRhs.iParentAuthorNick &&
	iParentTitle == aRhs.iParentTitle &&
	iStreamDataId == aRhs.iStreamDataId &&
	iMediaFileName == aRhs.iMediaFileName &&
	iMediaDownloadState == aRhs.iMediaDownloadState);
}

EXPORT_C const MBBData* CBBFeedItem::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iParentUuid;
	case 1:
		return &iUuid;
	case 2:
		return &iAuthorNick;
	case 3:
		return &iAuthorDisplayName;
	case 4:
		return &iThumbnailMbm;
	case 5:
		return &iThumbnailUrl;
	case 6:
		return &iIconId;
	case 7:
		return &iCreated;
	case 8:
		return &iLinkedUrl;
	case 9:
		return &iContent;
	case 10:
		return &iLocation;
	case 11:
		return &iFromServer;
	case 12:
		return &iKind;
	case 13:
		return &iIsUnread;
	case 14:
		return &iIsGroupChild;
	case 15:
		return &iCorrelation;
	case 16:
		return &iStreamTitle;
	case 17:
		return &iStreamUrl;
	case 18:
		return &iChannel;
	case 19:
		return &iParentAuthorNick;
	case 20:
		return &iParentTitle;
	case 21:
		return &iStreamDataId;
	case 22:
		return &iMediaFileName;
	case 23:
		return &iMediaDownloadState;
	default:
		return 0;
	}
}

_LIT(KFeedItemSpace, " ");
EXPORT_C const TDesC& CBBFeedItem::StringSep(TUint aBeforePart) const { return KFeedItemSpace; }

EXPORT_C CBBFeedItem& CBBFeedItem::operator=(const CBBFeedItem& aRhs)
{
	iParentUuid=aRhs.iParentUuid;
	iUuid=aRhs.iUuid;
	iAuthorNick=aRhs.iAuthorNick;
	iAuthorDisplayName=aRhs.iAuthorDisplayName;
	iThumbnailMbm.Zero(); iThumbnailMbm.Append(aRhs.iThumbnailMbm());
	iThumbnailUrl.Zero(); iThumbnailUrl.Append(aRhs.iThumbnailUrl());
	iIconId=aRhs.iIconId;
	iCreated=aRhs.iCreated;
	iLinkedUrl.Zero(); iLinkedUrl.Append(aRhs.iLinkedUrl());
	iContent.Zero(); iContent.Append(aRhs.iContent());
	iLocation=aRhs.iLocation;
	iFromServer=aRhs.iFromServer;
	iKind=aRhs.iKind;
	iIsUnread=aRhs.iIsUnread;
	iIsGroupChild=aRhs.iIsGroupChild;
	iCorrelation=aRhs.iCorrelation;
	iStreamTitle=aRhs.iStreamTitle;
	iStreamUrl.Zero(); iStreamUrl.Append(aRhs.iStreamUrl());
	iChannel=aRhs.iChannel;
	iParentAuthorNick=aRhs.iParentAuthorNick;
	iParentTitle=aRhs.iParentTitle;
	iStreamDataId=aRhs.iStreamDataId;
	iMediaFileName=aRhs.iMediaFileName;
	iMediaDownloadState=aRhs.iMediaDownloadState;
	return *this;
}

EXPORT_C CBBFeedItem::~CBBFeedItem()
{

	if (iDeletionNotify) iDeletionNotify->FeedItemDeleted(this);
	if (iErrorInfo) iErrorInfo->Release();

}

EXPORT_C void CBBFeedItem::AddRef() const
{
	++iRefCount;
}

EXPORT_C void CBBFeedItem::Release() const
{
	if (iRefCount==0) {
		User::Panic(_L("CSD"), 1);
	}
	--iRefCount;
	if (iRefCount==0) 
		delete this;
}
EXPORT_C void CBBFeedItem::SetDeletionNotify(MFeedItemDeletionNotify* aDeletionNotify)
{

	iDeletionNotify=aDeletionNotify;
}


