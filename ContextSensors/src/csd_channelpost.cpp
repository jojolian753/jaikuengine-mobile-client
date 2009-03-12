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

#include "csd_channelpost.h"


EXPORT_C const TTypeName& CBBChannelPost::Type() const
{
	return KChannelPostType;
}

EXPORT_C const TTypeName& CBBChannelPost::StaticType()
{
	return KChannelPostType;
}

EXPORT_C TBool CBBChannelPost::Equals(const MBBData* aRhs) const
{
	const CBBChannelPost* rhs=bb_cast<CBBChannelPost>(aRhs);
	return (rhs && *rhs==*this);
}

EXPORT_C MBBData* CBBChannelPost::CloneL(const TDesC&) const
{
	bb_auto_ptr<CBBChannelPost> ret(new (ELeave) CBBChannelPost());
	*ret=*this;
	return ret.release();
}
_LIT(KUuid, "uuid");
_LIT(KChannel, "channel");
_LIT(KNick, "nick");
_LIT(KDisplayName, "displayname");
_LIT(KContent, "content");
_LIT(KCreated, "created");

EXPORT_C CBBChannelPost::CBBChannelPost() : TBBCompoundData(KChannelPost) , iUuid(KUuid), iChannel(KChannel), iNick(KNick), iDisplayName(KDisplayName), iContent(KContent), iCreated(KCreated) { }

EXPORT_C bool CBBChannelPost::operator==(const CBBChannelPost& aRhs) const
{
	return (iUuid == aRhs.iUuid &&
	iChannel == aRhs.iChannel &&
	iNick == aRhs.iNick &&
	iDisplayName == aRhs.iDisplayName &&
	iContent == aRhs.iContent &&
	iCreated == aRhs.iCreated);
}

EXPORT_C const MBBData* CBBChannelPost::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iUuid;
	case 1:
		return &iChannel;
	case 2:
		return &iNick;
	case 3:
		return &iDisplayName;
	case 4:
		return &iContent;
	case 5:
		return &iCreated;
	default:
		return 0;
	}
}

_LIT(KChannelPostSpace, " ");
EXPORT_C const TDesC& CBBChannelPost::StringSep(TUint aBeforePart) const { return KChannelPostSpace; }

EXPORT_C CBBChannelPost& CBBChannelPost::operator=(const CBBChannelPost& aRhs)
{
	iUuid=aRhs.iUuid;
	iChannel=aRhs.iChannel;
	iNick=aRhs.iNick;
	iDisplayName=aRhs.iDisplayName;
	iContent.Zero(); iContent.Append(aRhs.iContent());
	iCreated=aRhs.iCreated;
	return *this;
}
