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

#include "bbtuple.h"

EXPORT_C const TTypeName& TBBTupleSubName::Type() const
{
	return KSubNameType;
}

EXPORT_C const TTypeName& TBBTupleSubName::StaticType()
{
	return KSubNameType;
}

EXPORT_C TBool TBBTupleSubName::Equals(const MBBData* aRhs) const
{
	const TBBTupleSubName* rhs=bb_cast<TBBTupleSubName>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C MBBData* TBBTupleSubName::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBTupleSubName(iValue, Name);
}

#include "csd_tuplemeta.cpp"

#if 1
#include "csd_tuple.cpp"
#else
EXPORT_C const TTypeName& CBBTuple::Type() const
{
	return KTupleType;
}

EXPORT_C TBool CBBTuple::Equals(const MBBData* aRhs) const
{
	const CBBTuple* rhs=bb_cast<CBBTuple>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& CBBTuple::StaticType()
{
	return KTupleType;
}

EXPORT_C const MBBData* CBBTuple::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iTupleId;
	case 1:
		return &iTupleMeta;
	case 2:
		return &iData;
	case 3:
		return &iExpires;
	default:
		return 0;
	}
}

_LIT(KTuple, "tuple");
_LIT(KValue, "tuplevalue");

EXPORT_C CBBTuple::CBBTuple(MBBDataFactory* aFactory/*, TUint aTupleId, TInt aModuleUid, TInt aModuleId,
			    const TDesC& aSubName, MBBData* aData*/) : TBBCompoundData(KTuple),
			    iTupleId(/*aTupleId, */KId)/*,
			    iTupleMeta(aModuleUid, aModuleId, aSubName)*/, iExpires(KExpires),
			    iData(KValue, aFactory/*, aData*/), iFactory(aFactory)
{
}

EXPORT_C bool CBBTuple::operator==(const CBBTuple& aRhs) const
{
	return (
		iTupleId==aRhs.iTupleId &&
		iTupleMeta==aRhs.iTupleMeta &&
		iExpires == aRhs.iExpires &&
		iData.Equals(aRhs.iData())
		);
}

EXPORT_C const TDesC& CBBTuple::StringSep(TUint aBeforePart) const
{
	if (aBeforePart==0 || aBeforePart > 3) return KNullDesC;
	return KSpace;
}

MBBData* CBBTuple::GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto)
{
	MBBData* p=TBBCompoundData::GetPart(aName, aType, aPartNoInto);
	if (!p || aName.Compare(KValue)==0 || p==&iData) {
		if (iExpires.Name().Compare(aName)==0) {
			aPartNoInto=3;
			return &iExpires;
		}
		iValueName=aName;
		iData.SetValue(iFactory->CreateBBDataL(aType, iValueName, iFactory));
		aPartNoInto=2;
		p=&iData;
	}
	return p;
}


EXPORT_C CBBTuple& CBBTuple::operator=(const CBBTuple& aTuple)
{
	MBBData* val=0;
	if (aTuple.iData()) {
		val=aTuple.iData()->CloneL(aTuple.iData()->Name());
	}
	iData.SetValue(val);
	iData.SetOwnsValue(ETrue);
	iTupleId()=aTuple.iTupleId();
	iTupleMeta=aTuple.iTupleMeta;
	iExpires()=aTuple.iExpires();

	return *this;
}
	
EXPORT_C MBBData* CBBTuple::CloneL(const TDesC& ) const
{
	auto_ptr<CBBTuple> ret(new (ELeave) CBBTuple(iFactory));
	*ret=*this;
	return ret.release();
}

#endif
