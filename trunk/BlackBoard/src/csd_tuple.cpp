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

#include "csd_tuple.h"


EXPORT_C const TTypeName& CBBTuple::Type() const
{
	return KTupleType;
}

EXPORT_C const TTypeName& CBBTuple::StaticType()
{
	return KTupleType;
}

EXPORT_C TBool CBBTuple::Equals(const MBBData* aRhs) const
{
	const CBBTuple* rhs=bb_cast<CBBTuple>(aRhs);
	return (rhs && *rhs==*this);
}
EXPORT_C MBBDataFactory* CBBTuple::Factory()
{
	return iFactory;
}


EXPORT_C MBBData* CBBTuple::CloneL(const TDesC&) const
{
	bb_auto_ptr<CBBTuple> ret(new (ELeave) CBBTuple(iFactory));
	*ret=*this;
	return ret.release();
}
_LIT(KTupleId, "id");
_LIT(KExpires, "expires");
_LIT(KData, "tuplevalue");
_LIT(KTupleUuid, "uuid");

EXPORT_C CBBTuple::CBBTuple(MBBDataFactory* aFactory) : TBBCompoundData(KTuple) , iTupleId(KTupleId), iExpires(KExpires), iData(KData, aFactory), iTupleUuid(KTupleUuid), iFactory(aFactory) { }

EXPORT_C bool CBBTuple::operator==(const CBBTuple& aRhs) const
{
	return (iTupleId == aRhs.iTupleId &&
	iTupleMeta == aRhs.iTupleMeta &&
	iExpires == aRhs.iExpires &&
	iData.Equals(aRhs.iData()) &&
	iTupleUuid == aRhs.iTupleUuid);
}

EXPORT_C const MBBData* CBBTuple::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iTupleId;
	case 1:
		return &iTupleMeta;
	case 2:
		return &iExpires;
	case 3:
		return &iData;
	case 4:
		return &iTupleUuid;
	default:
		return 0;
	}
}

_LIT(KTupleSpace, " ");
EXPORT_C const TDesC& CBBTuple::StringSep(TUint aBeforePart) const { return KTupleSpace; }

EXPORT_C MBBData* CBBTuple::GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto)
{
	MBBData* p=TBBCompoundData::GetPart(aName, aType, aPartNoInto);
	if (!p || p==&iData) {
		iData.SetValue(iFactory->CreateBBDataL(aType, KData, iFactory));
		aPartNoInto=3;
		p=&iData;
	}
	return p;
}
EXPORT_C CBBTuple& CBBTuple::operator=(const CBBTuple& aRhs)
{
	iTupleId=aRhs.iTupleId;
	iTupleMeta=aRhs.iTupleMeta;
	iExpires=aRhs.iExpires;
	if (aRhs.iData()) {
		iData.SetValue(aRhs.iData()->CloneL(aRhs.iData()->Name()));
	} else {
		iData.SetValue(0);
	}
	iData.SetOwnsValue(ETrue);
	iTupleUuid=aRhs.iTupleUuid;
	return *this;
}
