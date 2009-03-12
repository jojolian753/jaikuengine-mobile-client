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

#include "break.h"
#include "csd_bluetooth.h"

EXPORT_C const TTypeName& TBBBluetoothName::Type() const
{
	return KBluetoothNameType;
}

EXPORT_C const TTypeName& TBBBluetoothName::StaticType()
{
	return KBluetoothNameType;
}

EXPORT_C TBool TBBBluetoothName::Equals(const MBBData* aRhs) const
{
	const TBBBluetoothName *rhs=bb_cast<TBBBluetoothName>(aRhs);
	return (rhs && *this==*rhs);
}


EXPORT_C const TTypeName& TBBBluetoothAddress::Type() const
{
	return KBluetoothAddrType;
}

EXPORT_C const TTypeName& TBBBluetoothAddress::StaticType()
{
	return KBluetoothAddrType;
}

EXPORT_C TBool TBBBluetoothAddress::Equals(const MBBData* aRhs) const
{
	const TBBBluetoothAddress *rhs=bb_cast<TBBBluetoothAddress>(aRhs);
	return (rhs && *this==*rhs);
}


EXPORT_C const TTypeName& TBBBtDeviceInfo::Type() const
{
	return KBluetoothInfoType;
}

EXPORT_C TBool TBBBtDeviceInfo::Equals(const MBBData* aRhs) const
{
	const TBBBtDeviceInfo *rhs=bb_cast<TBBBtDeviceInfo>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBBtDeviceInfo::StaticType()
{
	return KBluetoothInfoType;
}

EXPORT_C const MBBData* TBBBtDeviceInfo::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iMAC;
	case 1:
		return &iNick;
	case 2:
		return &iMajorClass;
	case 3:
		return &iMinorClass;
	case 4:
		return &iServiceClass;
	default:
		return 0;
	}
}

_LIT(KMAC, "bt.mac");
_LIT(KMajorClass, "bt.majorclass");
_LIT(KMinorClass, "bt.minorclass");
_LIT(KServiceClass, "bt.serviceclass");
_LIT(KNick, "bt.name");
_LIT(KComma, ",");
_LIT(KOpenBrace, " [");
_LIT(KCloseBrace, "]");
_LIT(KColon, ":");

EXPORT_C TBBBtDeviceInfo::TBBBtDeviceInfo() : TBBCompoundData(KCSDBt),
	iMAC(KMAC), iNick(KNick), iMajorClass(KMajorClass), iMinorClass(KMinorClass), iServiceClass(KServiceClass)
{
}

EXPORT_C TBBBtDeviceInfo::TBBBtDeviceInfo(const TDesC8& aMAC, const TDesC& aNick, TInt aMajorClass,
	TInt aMinorClass, TInt aServiceClass) : TBBCompoundData(KCSDBt),
	iMAC(aMAC, KMAC), iNick(aNick, KNick), iMajorClass(aMajorClass, KMajorClass), 
	iMinorClass(aMinorClass, KMinorClass), iServiceClass(aServiceClass, KServiceClass)
{
}

EXPORT_C TBBBluetoothAddress::TBBBluetoothAddress(const TDesC8& aValue, const TDesC& aName) : 
	TBBFixedLengthStringBase8(aName) { iValue=aValue.Left(6); }

EXPORT_C const TDesC& TBBBtDeviceInfo::StringSep(TUint aBeforePart) const
{
	switch (aBeforePart) {
	case 0:
		return KNullDesC;
	case 1:
		return KOpenBrace;
	case 2:
		return KComma;
	case 3:
	case 4:
		return KColon;
	case 5:
		return KCloseBrace;
	default:
		return KNullDesC;
	}
}

EXPORT_C bool TBBBtDeviceInfo::operator==(const TBBBtDeviceInfo& aRhs) const
{
	return (
		iMAC==aRhs.iMAC &&
		iNick==aRhs.iNick &&
		iMajorClass==aRhs.iMajorClass &&
		iMinorClass==aRhs.iMinorClass &&
		iServiceClass==aRhs.iServiceClass );
}

EXPORT_C TBBBtDeviceInfo& TBBBtDeviceInfo::operator=(const TBBBtDeviceInfo& aRhs)
{
	iMAC()=aRhs.iMAC();
	iNick()=aRhs.iNick();
	iMajorClass()=aRhs.iMajorClass();
	iMinorClass()=aRhs.iMinorClass();
	iServiceClass()=aRhs.iServiceClass();
	return *this;
}

EXPORT_C CBBBtDeviceList* CBBBtDeviceList::NewL()
{
	auto_ptr<CBBBtDeviceList> ret(new (ELeave) CBBBtDeviceList());
	ret->ConstructL();
	return ret.release();
}

EXPORT_C void CBBBtDeviceList::ConstructL()
{
	CBBGenericList::ConstructL();
}

EXPORT_C TBBBtDeviceInfo* CBBBtDeviceList::First()
{
	return static_cast<TBBBtDeviceInfo*>(CBBGenericList::First());
}

EXPORT_C TBBBtDeviceInfo* CBBBtDeviceList::Next()
{
	return static_cast<TBBBtDeviceInfo*>(CBBGenericList::Next());
}

EXPORT_C const TBBBtDeviceInfo*	CBBBtDeviceList::First() const
{
	return static_cast<const TBBBtDeviceInfo*>(CBBGenericList::First());
}

EXPORT_C const TBBBtDeviceInfo*	CBBBtDeviceList::Next() const
{
	return static_cast<const TBBBtDeviceInfo*>(CBBGenericList::Next());
}

EXPORT_C void	CBBBtDeviceList::AddItemL(TBBBtDeviceInfo* aData)
{
	CBBGenericList::AddItemL(0, aData);
}

EXPORT_C const TTypeName& CBBBtDeviceList::Type() const
{
	return KBluetoothListType;
}

EXPORT_C const TTypeName& CBBBtDeviceList::StaticType()
{
	return KBluetoothListType;
}

_LIT(KSpace, " ");

CBBBtDeviceList::CBBBtDeviceList() : CBBGenericList(KCSDBtList, KCSDBt, KSpace, this)
{
}

MBBData* CBBBtDeviceList::CreateBBDataL(const TTypeName& , const TDesC& , MBBDataFactory* )
{
	return new (ELeave) TBBBtDeviceInfo();
}

EXPORT_C void CBBBtDeviceList::AddItemL(HBufC*	aName, MBBData* aData)
{
	if (!aData || !(aData->Type()==KBluetoothInfoType)) User::Leave(KErrNotSupported);
	CBBGenericList::AddItemL(aName, aData);
}

EXPORT_C TBool CBBBtDeviceList::FixedType() const
{
	return ETrue;
}


EXPORT_C TBBBtDeviceInfo::TBBBtDeviceInfo(TInquirySockAddr btaddr, const TDesC& aNick) : TBBCompoundData(KCSDBt),
	iMAC(btaddr.BTAddr().Des(), KMAC), iNick(aNick, KNick), iMajorClass(btaddr.MajorClassOfDevice(), KMajorClass),
		iMinorClass(btaddr.MinorClassOfDevice(), KMinorClass), 
		iServiceClass(btaddr.MajorServiceClass(), KServiceClass) { }


EXPORT_C TBool CBBBtDeviceList::Equals(const MBBData* aRhs) const
{
	if (!aRhs) return EFalse;
	if (! (aRhs->Type() == Type()) ) return EFalse;
	const CBBBtDeviceList* rhs=static_cast<const CBBBtDeviceList*>(aRhs);
	const TBBBtDeviceInfo *l, *r;

	TInt lcount=Count(), rcount=rhs->Count();
	if (lcount!=rcount) return EFalse;

	for (l=First(), r=rhs->First(); l && r; l=Next(), r=rhs->Next()) {
		if ( ! l->Equals(r) ) return EFalse;
	}
	if (l || r) return EFalse;
	return ETrue;
}

EXPORT_C MBBData* TBBBluetoothName::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBBluetoothName(iValue, Name);
}

EXPORT_C MBBData* TBBBluetoothAddress::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBBluetoothAddress(iValue, Name);
}

EXPORT_C MBBData* TBBBtDeviceInfo::CloneL(const TDesC& ) const
{
	TBBBtDeviceInfo* ret=new (ELeave) TBBBtDeviceInfo;
	*ret=*this;
	return ret;
}

EXPORT_C CBBBtDeviceList& CBBBtDeviceList::operator=(const CBBBtDeviceList& aList)
{
	CBBGenericList::operator=(aList);
	return *this;
}

EXPORT_C MBBData* CBBBtDeviceList::CloneL(const TDesC& ) const
{
	auto_ptr<CBBBtDeviceList> ret(CBBBtDeviceList::NewL());
	*ret=*this;
	return ret.release();
}

EXPORT_C const TTypeName& TBBNeighbourhoodInfo::Type() const
{
	return KBluetoothNeighboursType;
}

EXPORT_C const TTypeName& TBBNeighbourhoodInfo::StaticType()
{
	return KBluetoothNeighboursType;
}

EXPORT_C TBool TBBNeighbourhoodInfo::Equals(const MBBData* aRhs) const
{
	const TBBNeighbourhoodInfo*rhs=bb_cast<TBBNeighbourhoodInfo>(aRhs);
	return (rhs && rhs->iBuddies==iBuddies && rhs->iOtherPhones==iOtherPhones);
}

EXPORT_C TBBNeighbourhoodInfo& TBBNeighbourhoodInfo::operator=(const TBBNeighbourhoodInfo& aInfo)
{
	iBuddies()=aInfo.iBuddies();
	iOtherPhones()=aInfo.iOtherPhones();
	iLaptops()=aInfo.iLaptops();
	iDesktops()=aInfo.iDesktops();
	iPDAs()=aInfo.iPDAs();
	return *this;
}

EXPORT_C MBBData* TBBNeighbourhoodInfo::CloneL(const TDesC&) const
{
	TBBNeighbourhoodInfo* ret=new (ELeave) TBBNeighbourhoodInfo;
	*ret=*this;
	return ret;
}

_LIT(KNeighbours, "bt.presence");
_LIT(KBuddies, "buddies");
_LIT(KOthers, "other_phones");
_LIT(KDesktops, "own_desktops");
_LIT(KLaptops, "own_laptops");
_LIT(KPDAs, "own_pdas");

EXPORT_C TBBNeighbourhoodInfo::TBBNeighbourhoodInfo() : TBBCompoundData(KNeighbours),
iBuddies(KBuddies), iOtherPhones(KOthers), iLaptops(KLaptops), iDesktops(KDesktops), iPDAs(KPDAs),
iCreatedVersion(2), iUseVersion(2) { }

EXPORT_C TBBNeighbourhoodInfo::TBBNeighbourhoodInfo(TInt aVersion) : TBBCompoundData(KNeighbours),
iBuddies(KBuddies), iOtherPhones(KOthers), iLaptops(KLaptops), iDesktops(KDesktops), iPDAs(KPDAs),
iCreatedVersion(aVersion), iUseVersion(2) { }

EXPORT_C bool TBBNeighbourhoodInfo::operator==(const TBBNeighbourhoodInfo& aRhs) const
{
	return (iBuddies==aRhs.iBuddies && iOtherPhones==aRhs.iOtherPhones);
}

EXPORT_C const TDesC& TBBNeighbourhoodInfo::StringSep(TUint aBeforePart) const
{
	return KSpace;
}

EXPORT_C const MBBData* TBBNeighbourhoodInfo::Part(TUint aPartNo) const
{
	if (aPartNo==0) return &iBuddies;
	if (aPartNo==1) return &iOtherPhones;

	if (iUseVersion==2 && aPartNo==2) return &iLaptops;
	if (iUseVersion==2 && aPartNo==3) return &iDesktops;
	if (iUseVersion==2 && aPartNo==4) return &iPDAs;

	return 0;
}

EXPORT_C void	TBBNeighbourhoodInfo::InternalizeL(RReadStream& aStream)
{
	iUseVersion=iCreatedVersion;
	CC_TRAPD(err, TBBCompoundData::InternalizeL(aStream));
	iUseVersion=2;
	User::LeaveIfError(err);
}

IMPLEMENT_ASSIGN(TBBBluetoothAddress)
