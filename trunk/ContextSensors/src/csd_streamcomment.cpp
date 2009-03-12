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

#include "csd_streamcomment.h"


EXPORT_C const TTypeName& CBBStreamComment::Type() const
{
	return KStreamCommentType;
}

EXPORT_C const TTypeName& CBBStreamComment::StaticType()
{
	return KStreamCommentType;
}

EXPORT_C TBool CBBStreamComment::Equals(const MBBData* aRhs) const
{
	const CBBStreamComment* rhs=bb_cast<CBBStreamComment>(aRhs);
	return (rhs && *rhs==*this);
}

EXPORT_C MBBData* CBBStreamComment::CloneL(const TDesC&) const
{
	bb_auto_ptr<CBBStreamComment> ret(new (ELeave) CBBStreamComment());
	*ret=*this;
	return ret.release();
}
_LIT(KUuid, "uuid");
_LIT(KPostUuid, "postuuid");
_LIT(KAuthorNick, "authornick");
_LIT(KAuthorDisplayName, "authordisplayname");
_LIT(KContent, "content");
_LIT(KCreated, "created");
_LIT(KChannelName, "channelname");
_LIT(KPostAuthorNick, "postauthornick");
_LIT(KPostTitle, "posttitle");
_LIT(KStreamDataId, "streamdataid");

EXPORT_C CBBStreamComment::CBBStreamComment() : TBBCompoundData(KStreamComment) , iUuid(KUuid), iPostUuid(KPostUuid), iAuthorNick(KAuthorNick), iAuthorDisplayName(KAuthorDisplayName), iContent(KContent), iCreated(KCreated), iChannelName(KChannelName), iPostAuthorNick(KPostAuthorNick), iPostTitle(KPostTitle), iStreamDataId(KStreamDataId) { }

EXPORT_C bool CBBStreamComment::operator==(const CBBStreamComment& aRhs) const
{
	return (iUuid == aRhs.iUuid &&
	iPostUuid == aRhs.iPostUuid &&
	iAuthorNick == aRhs.iAuthorNick &&
	iAuthorDisplayName == aRhs.iAuthorDisplayName &&
	iContent == aRhs.iContent &&
	iCreated == aRhs.iCreated &&
	iChannelName == aRhs.iChannelName &&
	iPostAuthorNick == aRhs.iPostAuthorNick &&
	iPostTitle == aRhs.iPostTitle &&
	iStreamDataId == aRhs.iStreamDataId);
}

EXPORT_C const MBBData* CBBStreamComment::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iUuid;
	case 1:
		return &iPostUuid;
	case 2:
		return &iAuthorNick;
	case 3:
		return &iAuthorDisplayName;
	case 4:
		return &iContent;
	case 5:
		return &iCreated;
	case 6:
		return &iChannelName;
	case 7:
		return &iPostAuthorNick;
	case 8:
		return &iPostTitle;
	case 9:
		return &iStreamDataId;
	default:
		return 0;
	}
}

_LIT(KStreamCommentSpace, " ");
EXPORT_C const TDesC& CBBStreamComment::StringSep(TUint aBeforePart) const { return KStreamCommentSpace; }

EXPORT_C CBBStreamComment& CBBStreamComment::operator=(const CBBStreamComment& aRhs)
{
	iUuid=aRhs.iUuid;
	iPostUuid=aRhs.iPostUuid;
	iAuthorNick=aRhs.iAuthorNick;
	iAuthorDisplayName=aRhs.iAuthorDisplayName;
	iContent.Zero(); iContent.Append(aRhs.iContent());
	iCreated=aRhs.iCreated;
	iChannelName=aRhs.iChannelName;
	iPostAuthorNick=aRhs.iPostAuthorNick;
	iPostTitle=aRhs.iPostTitle;
	iStreamDataId=aRhs.iStreamDataId;
	return *this;
}
