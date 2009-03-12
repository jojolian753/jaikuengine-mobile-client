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

#include "csd_streamdata.h"


EXPORT_C const TTypeName& CBBStreamData::Type() const
{
	return KStreamDataType;
}

EXPORT_C const TTypeName& CBBStreamData::StaticType()
{
	return KStreamDataType;
}

EXPORT_C TBool CBBStreamData::Equals(const MBBData* aRhs) const
{
	const CBBStreamData* rhs=bb_cast<CBBStreamData>(aRhs);
	return (rhs && *rhs==*this);
}

EXPORT_C MBBData* CBBStreamData::CloneL(const TDesC&) const
{
	bb_auto_ptr<CBBStreamData> ret(new (ELeave) CBBStreamData());
	*ret=*this;
	return ret.release();
}
_LIT(KUuid, "uuid");
_LIT(KCorrelation, "correlation");
_LIT(KAuthorNick, "authornick");
_LIT(KAuthorDisplayName, "authordisplayname");
_LIT(KTitle, "title");
_LIT(KContent, "content");
_LIT(KUrl, "url");
_LIT(KIconId, "iconid");
_LIT(KCreated, "created");
_LIT(KKind, "kind");
_LIT(KStreamTitle, "streamtitle");
_LIT(KStreamUrl, "streamurl");
_LIT(KChannelName, "channelname");
_LIT(KStreamDataId, "streamdataid");

EXPORT_C CBBStreamData::CBBStreamData() : TBBCompoundData(KStreamData) , iUuid(KUuid), iCorrelation(KCorrelation), iAuthorNick(KAuthorNick), iAuthorDisplayName(KAuthorDisplayName), iTitle(KTitle), iContent(KContent), iUrl(KUrl), iIconId(KIconId), iCreated(KCreated), iKind(KKind), iStreamTitle(KStreamTitle), iStreamUrl(KStreamUrl), iChannelName(KChannelName), iStreamDataId(KStreamDataId) { }

EXPORT_C bool CBBStreamData::operator==(const CBBStreamData& aRhs) const
{
	return (iUuid == aRhs.iUuid &&
	iCorrelation == aRhs.iCorrelation &&
	iAuthorNick == aRhs.iAuthorNick &&
	iAuthorDisplayName == aRhs.iAuthorDisplayName &&
	iTitle == aRhs.iTitle &&
	iContent == aRhs.iContent &&
	iUrl == aRhs.iUrl &&
	iIconId == aRhs.iIconId &&
	iCreated == aRhs.iCreated &&
	iKind == aRhs.iKind &&
	iStreamTitle == aRhs.iStreamTitle &&
	iStreamUrl == aRhs.iStreamUrl &&
	iChannelName == aRhs.iChannelName &&
	iStreamDataId == aRhs.iStreamDataId);
}

EXPORT_C const MBBData* CBBStreamData::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iUuid;
	case 1:
		return &iCorrelation;
	case 2:
		return &iAuthorNick;
	case 3:
		return &iAuthorDisplayName;
	case 4:
		return &iTitle;
	case 5:
		return &iContent;
	case 6:
		return &iUrl;
	case 7:
		return &iIconId;
	case 8:
		return &iCreated;
	case 9:
		return &iKind;
	case 10:
		return &iStreamTitle;
	case 11:
		return &iStreamUrl;
	case 12:
		return &iChannelName;
	case 13:
		return &iStreamDataId;
	default:
		return 0;
	}
}

_LIT(KStreamDataSpace, " ");
EXPORT_C const TDesC& CBBStreamData::StringSep(TUint aBeforePart) const { return KStreamDataSpace; }

EXPORT_C CBBStreamData& CBBStreamData::operator=(const CBBStreamData& aRhs)
{
	iUuid=aRhs.iUuid;
	iCorrelation=aRhs.iCorrelation;
	iAuthorNick=aRhs.iAuthorNick;
	iAuthorDisplayName=aRhs.iAuthorDisplayName;
	iTitle=aRhs.iTitle;
	iContent.Zero(); iContent.Append(aRhs.iContent());
	iUrl.Zero(); iUrl.Append(aRhs.iUrl());
	iIconId=aRhs.iIconId;
	iCreated=aRhs.iCreated;
	iKind=aRhs.iKind;
	iStreamTitle=aRhs.iStreamTitle;
	iStreamUrl.Zero(); iStreamUrl.Append(aRhs.iStreamUrl());
	iChannelName=aRhs.iChannelName;
	iStreamDataId=aRhs.iStreamDataId;
	return *this;
}
