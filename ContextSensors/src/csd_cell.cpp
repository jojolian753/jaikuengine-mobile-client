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

#include "csd_cell.h"

EXPORT_C const TTypeName& TBBShortNetworkName::Type() const
{
	return KShortNetworkNameType;
}

EXPORT_C const TTypeName& TBBShortNetworkName::StaticType()
{
	return KShortNetworkNameType;
}

EXPORT_C TBool TBBShortNetworkName::Equals(const MBBData* aRhs) const
{
	const TBBShortNetworkName* rhs=bb_cast<TBBShortNetworkName>(aRhs);
	return (rhs && iValue.Compare(rhs->iValue)==0);
}

_LIT(KMCC, "location.mcc");
_LIT(KMNC, "location.mnc");
_LIT(KShortName, "location.network");
_LIT(KLAC, "location.lac");
_LIT(KCellId, "location.cellid");
_LIT(KMappedCellId, "location.id");

EXPORT_C TBBCellId::TBBCellId(const TDesC& aName) : TBBCompoundData(aName),
	iMCC(KMCC), iMNC(KMNC), iShortName(KShortName), iLocationAreaCode(KLAC),
	iCellId(KCellId), iMappedId(KMappedCellId)
{
}

EXPORT_C bool TBBCellId::operator==(const TBBCellId& aRhs) const
{
	return (iMCC == aRhs.iMCC &&
		iMNC == aRhs.iMNC &&
		iShortName == aRhs.iShortName &&
		iLocationAreaCode == aRhs.iLocationAreaCode &&
		iCellId == aRhs.iCellId &&
		iMappedId == aRhs.iMappedId);
}

const MBBData* TBBCellId::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iMCC;
	case 1:
		return &iMNC;
	case 2:
		return &iShortName;
	case 3:
		return &iLocationAreaCode;
	case 4:
		return &iCellId;
	case 5:
		return &iMappedId;
	default:
		return 0;
	}
}

EXPORT_C const TTypeName& TBBCellId::Type() const
{
	return KCellIdType;
}

EXPORT_C const TTypeName& TBBCellId::StaticType()
{
	return KCellIdType;
}

EXPORT_C TBool TBBCellId::Equals(const MBBData* aRhs) const
{
	const TBBCellId* rhs=bb_cast<TBBCellId>(aRhs);
	return (rhs && *this==*rhs);
}

_LIT(KComma, ", ");

EXPORT_C const TDesC& TBBCellId::StringSep(TUint aBeforePart) const
{
	if (aBeforePart>0 && aBeforePart<=4) return KComma;
	return KNullDesC;
}

EXPORT_C void TBBCellId::IntoStringL(TDes& aString) const
{
	TUint i=0;
	for (const MBBData *p=Part(i); i<5; p=Part(++i)) {
		const TDesC& sep=StringSep(i);
		CheckStringSpaceL(aString, sep.Length());
		aString.Append(sep);
		p->IntoStringL(aString);
	}
	const TDesC& sep=StringSep(i);
	CheckStringSpaceL(aString, sep.Length());
	aString.Append(sep);
}


EXPORT_C TBBCellId& TBBCellId::operator=(const TBBCellId& aRhs)
{
	iMCC()=aRhs.iMCC();
	iMNC()=aRhs.iMNC();
	iShortName()=aRhs.iShortName();
	iLocationAreaCode()=aRhs.iLocationAreaCode();
	iCellId=aRhs.iCellId();
	iMappedId()=aRhs.iMappedId();
	return *this;
}

EXPORT_C TBBCellId::TBBCellId(const TBBCellId& aRhs) : TBBCompoundData(aRhs.Name()),
	iMCC(KMCC), iMNC(KMNC), iShortName(KShortName), iLocationAreaCode(KLAC),
	iCellId(KCellId), iMappedId(KMappedCellId)
{
	iMCC()=aRhs.iMCC();
	iMNC()=aRhs.iMNC();
	iShortName()=aRhs.iShortName();
	iLocationAreaCode()=aRhs.iLocationAreaCode();
	iCellId=aRhs.iCellId();
	iMappedId()=aRhs.iMappedId();
}

EXPORT_C MBBData* TBBShortNetworkName::CloneL(const TDesC& Name) const
{
	TBBShortNetworkName* ret=new (ELeave) TBBShortNetworkName(Name);
	*ret=iValue;
	return ret;
}


EXPORT_C MBBData* TBBCellId::CloneL(const TDesC& Name) const
{
	TBBCellId* ret=new (ELeave) TBBCellId(Name);
	*ret=*this;
	return ret;
}

void SkipComma(TLex& l)
{
	TChar c;
	while ( (c=l.Get()) && c!=',');
	l.Get();
}

EXPORT_C void TBBCellId::FromStringL(const TDesC& aString)
{
	TLex l(aString);
	User::LeaveIfError(l.Val(iMCC()));
	SkipComma(l);
	User::LeaveIfError(l.Val(iMNC()));
	SkipComma(l);
	l.Mark(); SkipComma(l); 

	// network name may be empty
	if (l.MarkedToken().Length()>2) {
		iShortName()=l.MarkedToken().Left(l.MarkedToken().Length()-2);
	} else {
		iShortName().Zero();
	}
	User::LeaveIfError(l.Val(iLocationAreaCode()));
	SkipComma(l);
	User::LeaveIfError(l.Val(iCellId()));
}
