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

#include "csd_base.h"
#include "csd_cell.h"

EXPORT_C const TTypeName& TBBBaseVisit::Type() const
{
	return KBaseVisitType;
}

EXPORT_C TBool TBBBaseVisit::Equals(const MBBData* aRhs) const
{
	const TBBBaseVisit* rhs=bb_cast<TBBBaseVisit>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBBaseVisit::StaticType()
{
	return KBaseVisitType;
}

EXPORT_C void TBBBaseVisit::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	if (iEntered()==TTime(0)) return;
	TBBCompoundData::IntoXmlL(aBuf, aIncludeType);
}

EXPORT_C const MBBData* TBBBaseVisit::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iBaseId;
		break;
	case 1:
		return &iBaseName;
		break;
	case 2:
		return &iEntered;
		break;
	case 3:
		return &iLeft;
		break;
	default:
		return 0;
	}
}

EXPORT_C TBBBaseVisit& TBBBaseVisit::operator=(const TBBBaseVisit& aBaseVisit)
{
	iBaseId()=aBaseVisit.iBaseId();
	iBaseName()=aBaseVisit.iBaseName();
	iEntered()=aBaseVisit.iEntered();
	iLeft()=aBaseVisit.iLeft();
	return *this;
}

EXPORT_C MBBData* TBBBaseVisit::CloneL(const TDesC& Name) const
{
	TBBBaseVisit* ret=new (ELeave) TBBBaseVisit(Name);
	*ret=*this;
	return ret;
}

_LIT(KBaseId, "base.id");
_LIT(KBaseName, "base.name");
_LIT(KBaseLeft, "base.left");
_LIT(KBaseEntered, "base.arrived");

EXPORT_C TBBBaseVisit::TBBBaseVisit(const TDesC& aName) : TBBCompoundData(aName),
	iBaseId(KBaseId), iBaseName(KBaseName), iEntered(KBaseEntered), iLeft(KBaseLeft)
{
}

_LIT(KSpace, " ");

EXPORT_C const TDesC& TBBBaseVisit::StringSep(TUint aBeforePart) const
{
	return KSpace;
}

EXPORT_C bool TBBBaseVisit::operator==(const TBBBaseVisit& aRhs) const
{
	return (iBaseId==aRhs.iBaseId &&
		iBaseName==aRhs.iBaseName &&
		iEntered==aRhs.iEntered &&
		iLeft==aRhs.iLeft);
}

EXPORT_C const TTypeName& TBBBaseInfo::Type() const
{
	return KBaseInfoType;
}

EXPORT_C TBool TBBBaseInfo::Equals(const MBBData* aRhs) const
{
	const TBBBaseInfo* rhs=bb_cast<TBBBaseInfo>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBBaseInfo::StaticType()
{
	return KBaseInfoType;
}

EXPORT_C const MBBData* TBBBaseInfo::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iPreviousStay;
		break;
	case 1:
		return &iPreviousVisit;
		break;
	case 2:
		return &iCurrent;
		break;
	default:
		return 0;
	}
}

EXPORT_C TBBBaseInfo& TBBBaseInfo::operator=(const TBBBaseInfo& aBaseInfo)
{
	iPreviousStay=aBaseInfo.iPreviousStay;
	iPreviousVisit=aBaseInfo.iPreviousVisit;
	iCurrent=aBaseInfo.iCurrent;
	return *this;
}

EXPORT_C MBBData* TBBBaseInfo::CloneL(const TDesC& ) const
{
	TBBBaseInfo* ret=new (ELeave) TBBBaseInfo;
	*ret=*this;
	return ret;
}

_LIT(KPreviousStay, "base.previous");
_LIT(KPreviousVisit, "base.lastseen");
_LIT(KCurrent, "base.current");
_LIT(KBaseInfo, "base");

EXPORT_C TBBBaseInfo::TBBBaseInfo() : TBBCompoundData(KBaseInfo), 
	iPreviousStay(KPreviousStay), iPreviousVisit(KPreviousVisit),
	iCurrent(KCurrent) { }

EXPORT_C const TDesC& TBBBaseInfo::StringSep(TUint aBeforePart) const
{
	return KSpace;
}

EXPORT_C bool TBBBaseInfo::operator==(const TBBBaseInfo& aRhs) const
{
	return (iPreviousStay==aRhs.iPreviousStay &&
		iPreviousVisit==aRhs.iPreviousVisit &&
		iCurrent==aRhs.iCurrent);
}

EXPORT_C void TBBBaseInfo::IntoStringL(TDes& aString) const
{
	if (iCurrent.iEntered()!=TTime(0)) {
		iCurrent.iBaseName.IntoStringL(aString);
		return;
	}
	_LIT(KPrev, "last: ");
	CheckStringSpaceL(aString, 6);
	aString.Append(KPrev);
	iPreviousStay.iBaseName.IntoStringL(aString);
}

EXPORT_C TBool	TBBBaseVisit::IsSet() const
{
	return (iEntered()!=TTime(0) && iBaseName().Length()>0);
}


EXPORT_C const TTypeName& TBBLocation::Type() const
{
	return KLocationType;
}

EXPORT_C TBool TBBLocation::Equals(const MBBData* aRhs) const
{
	const TBBLocation* rhs=bb_cast<TBBLocation>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBLocation::StaticType()
{
	return KLocationType;
}

EXPORT_C const MBBData* TBBLocation::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iCellId;
	case 1:
		return &iLocationId;
	case 2:
		return &iIsBase;
	case 3:
		return &iLocationChanged;
	case 4:
		return &iEnteredLocation;
	default:
		return 0;
	}
}

EXPORT_C TBBLocation& TBBLocation::operator=(const TBBLocation& aRhs)
{
	iCellId=aRhs.iCellId;
	iLocationId()=aRhs.iLocationId();
	iIsBase()=aRhs.iIsBase();
	iLocationChanged()=aRhs.iLocationChanged();
	iEnteredLocation()=aRhs.iEnteredLocation();

	return *this;
}

EXPORT_C MBBData* TBBLocation::CloneL(const TDesC& Name) const
{
	TBBLocation* ret=new (ELeave) TBBLocation;
	*ret=*this;
	return ret;
}

_LIT(KLocation, "now_at_location");
_LIT(KLocationId, "location.id");
_LIT(KIsBase, "is_base");
_LIT(KLocationChanged, "location_changed");
_LIT(KEntered, "location.entered");


EXPORT_C TBBLocation::TBBLocation() : TBBCompoundData(KLocation),
	iCellId(KCell), iLocationId(KLocationId), iIsBase(KIsBase), iLocationChanged(KLocationChanged),
	iEnteredLocation(KEntered) { }

EXPORT_C const TDesC& TBBLocation::StringSep(TUint aBeforePart) const
{
	return KSpace;
}

EXPORT_C bool TBBLocation::operator==(const TBBLocation& aRhs) const
{
	return (
		iCellId==aRhs.iCellId &&
		iLocationId()==aRhs.iLocationId() &&
		iIsBase()==aRhs.iIsBase() &&
		iLocationChanged()==aRhs.iLocationChanged() &&
		iEnteredLocation()==aRhs.iEnteredLocation()
		);
}
