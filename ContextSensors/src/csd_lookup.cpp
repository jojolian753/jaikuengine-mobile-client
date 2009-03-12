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

#include "csd_lookup.h"

EXPORT_C const TTypeName& CBBLookup::Type() const
{
	return KLookupType;
}

EXPORT_C const TTypeName& CBBLookup::StaticType()
{
	return KLookupType;
}

EXPORT_C TBool CBBLookup::Equals(const MBBData* aRhs) const
{
	const CBBLookup* rhs=bb_cast<CBBLookup>(aRhs);
	return (rhs && *rhs==*this);
}

EXPORT_C CBBLookup& CBBLookup::operator=(const CBBLookup& aInfo)
{
	iLooker()=aInfo.iLooker();
	iWhen()=aInfo.iWhen();
	*iPresence=*(aInfo.iPresence);
	return *this;
}

EXPORT_C MBBData* CBBLookup::CloneL(const TDesC& ) const
{
	auto_ptr<CBBLookup> ret(new (ELeave) CBBLookup(2));
	ret->ConstructL();
	*ret=*this;
	return ret.release();
}

const MBBData* CBBLookup::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iLooker;
	case 1:
		return &iLooked;
	case 2:
		return &iWhen;
	case 3:
		return iPresence;
	case 4:
		if (iUseVersion==1) return 0;
		else return &iPresenceTimeStamp;
	default:
		return 0;
	}
}

EXPORT_C CBBLookup* CBBLookup::NewL()
{
	return NewL(2);
}

EXPORT_C CBBLookup* CBBLookup::NewL(TUint aVersion)
{
	auto_ptr<CBBLookup> ret(new (ELeave) CBBLookup(aVersion));
	ret->ConstructL();
	return ret.release();
}

EXPORT_C CBBLookup::~CBBLookup()
{
	delete iPresence;
}

EXPORT_C bool CBBLookup::operator==(const CBBLookup& aRhs) const
{
	return (
		iLooker==aRhs.iLooker &&
		iLooked==aRhs.iLooked &&
		iWhen==aRhs.iWhen &&
		*iPresence==*(aRhs.iPresence)
		);
}

_LIT(KSpace, " ");
EXPORT_C const TDesC& CBBLookup::StringSep(TUint ) const
{
	return KSpace;
}

EXPORT_C CBBLookup::CBBLookup(TUint aVersion) : TBBCompoundData(KLookup), iLooker(KLooker), 
	iLooked(KLooked), iWhen(KWhen), iPresenceTimeStamp(KTimeStamp), iCreatedVersion(aVersion),
	iUseVersion(2) { }

EXPORT_C void CBBLookup::ConstructL()
{
	iPresence=CBBPresence::NewL();
}

EXPORT_C void CBBLookup::InternalizeL(RReadStream& aStream)
{
	iUseVersion=iCreatedVersion;
	TBBCompoundData::InternalizeL(aStream);
	iUseVersion=2;
}
