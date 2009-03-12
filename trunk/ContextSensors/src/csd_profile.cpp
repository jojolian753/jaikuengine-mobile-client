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

#include "csd_profile.h"

EXPORT_C const TTypeName& TBBProfile::Type() const
{
	return KProfileType;
}

EXPORT_C TBool TBBProfile::Equals(const MBBData* aRhs) const
{
	const TBBProfile* rhs=bb_cast<TBBProfile>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBProfile::StaticType()
{
	return KProfileType;
}

EXPORT_C const MBBData* TBBProfile::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iProfileId;
	case 1:
		return &iProfileName;
	case 2:
		return &iRingingType;
	case 3:
		return &iRingingVolume;
	case 4:
		return &iVibra;
	default:
		return 0;
	}
}

EXPORT_C TBBProfile::TBBProfile() : TBBCompoundData(KProfile),
	iProfileId(KProfileId), iProfileName(KProfileName),
	iRingingType(KProfileRingType), iRingingVolume(KProfileRingVolume),
	iVibra(KProfileVibra)
{
}

_LIT(KSpace, " ");
_LIT(KOpenParen, " (");
_LIT(KCloseParen, ")");

EXPORT_C const TDesC& TBBProfile::StringSep(TUint aBeforePart) const
{
	if (aBeforePart==0) return KNullDesC;
	if (aBeforePart==1 || aBeforePart==3 || aBeforePart==4) return KSpace;
	if (aBeforePart==2) return KOpenParen;
	if (aBeforePart==5) return KCloseParen;
	return KNullDesC;
}

EXPORT_C bool TBBProfile::operator==(const TBBProfile& aRhs) const
{
	return (
		iProfileId == aRhs.iProfileId &&
		iProfileName == aRhs.iProfileName &&
		iRingingType == aRhs.iRingingType &&
		iRingingVolume == aRhs.iRingingVolume &&
		iVibra == aRhs.iVibra
		);
}

EXPORT_C TBBProfile& TBBProfile::operator=(const TBBProfile& aProfile)
{
	iProfileId()=aProfile.iProfileId();
	iProfileName()=aProfile.iProfileName();
	iRingingType()=aProfile.iRingingType();
	iRingingVolume()=aProfile.iRingingVolume();
	iVibra()=aProfile.iVibra();
	return *this;
}

EXPORT_C MBBData* TBBProfile::CloneL(const TDesC& ) const
{
	TBBProfile* ret=new (ELeave) TBBProfile();
	*ret=*this;
	return ret;
}
