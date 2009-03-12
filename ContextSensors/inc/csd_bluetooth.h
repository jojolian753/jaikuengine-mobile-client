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

#ifndef CONTEXT_CSD_BLUETOOTH_H_INCLUDED
#define CONTEXT_CSD_BLUETOOTH_H_INCLUDED 1

#include "bbdata.h"
#include "concretedata.h"
#include "context_uids.h"
#include "bblist.h"
#include <bt_sock.h>

_LIT(KCSDBt, "device");
_LIT(KCSDBtList, "devices");
_LIT(KCSDBtOwnAddress, "own_addres");

const TTypeName KBluetoothInfoType = { { CONTEXT_UID_SENSORDATAFACTORY }, 3, 1, 0 };
const TTypeName KBluetoothListType = { { CONTEXT_UID_SENSORDATAFACTORY }, 4, 1, 0 };
const TTypeName KBluetoothNameType = { { CONTEXT_UID_SENSORDATAFACTORY }, 5, 1, 0 };
const TTypeName KBluetoothAddrType = { { CONTEXT_UID_SENSORDATAFACTORY }, 6, 1, 0 };

const TTypeName KBluetoothNeighboursType1 = { { CONTEXT_UID_SENSORDATAFACTORY }, 32, 1, 0 };
const TTypeName KBluetoothNeighboursType = { { CONTEXT_UID_SENSORDATAFACTORY }, 32, 2, 0 };

const TTupleName KBluetoothTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 4 };
const TTupleName KOwnBluetoothTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 25 };


// KBluetoothNameNameSize=255 in etelbgsm.h, but we don't
// want to include that here
class TBBBluetoothName : public TBBFixedLengthStringBase {
public:
	TBuf<256> iValue;
    IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	TDes& operator()() { return iValue; }
	const TDesC& operator()() const { return iValue; }

	virtual TDes& Value() { return iValue; }
	virtual const TDesC& Value() const  { return iValue; }
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
public:
	TBBBluetoothName(const TDesC& aName) : TBBFixedLengthStringBase(aName) { }
	TBBBluetoothName(const TDesC& aValue, const TDesC& aName) : TBBFixedLengthStringBase(aName) { iValue=aValue.Left(256); }
	TBBBluetoothName& operator=(const TDesC& aValue) { iValue=aValue.Left(20); return *this; }
	bool operator==(const TBBBluetoothName& aRhs) const { return !(iValue.Compare(aRhs.iValue)); }
};

class TBBBluetoothAddress : public TBBFixedLengthStringBase8 {
public:
	TBuf8<6> iValue;
    IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	TDes8& operator()() { return iValue; }
	const TDesC8& operator()() const { return iValue; }

	virtual TDes8& Value() { return iValue; }
	virtual const TDesC8& Value() const  { return iValue; }

	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C MBBData* Assign(const MBBData* aRhs);
public:
	TBBBluetoothAddress(const TDesC& aName) : TBBFixedLengthStringBase8(aName) { }
	IMPORT_C TBBBluetoothAddress(const TDesC8& aValue, const TDesC& aName);
	bool operator==(const TBBBluetoothAddress& aRhs) const { return !(iValue.Compare(aRhs.iValue)); }
};


class TBBBtDeviceInfo : public TBBCompoundData  {
public:

	TBBBluetoothAddress	iMAC;
	TBBBluetoothName	iNick;
	TBBInt		iMajorClass;
	TBBInt		iMinorClass;
	TBBInt		iServiceClass;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;

	IMPORT_C TBBBtDeviceInfo& operator=(const TBBBtDeviceInfo& aDeviceInfo);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
public:
	IMPORT_C TBBBtDeviceInfo();
	IMPORT_C TBBBtDeviceInfo(TInquirySockAddr btaddr, const TDesC& aNick);
	IMPORT_C TBBBtDeviceInfo(const TDesC8& aMAC, const TDesC& aNick, TInt aMajorClass,
		TInt aMinorClass, TInt aServiceClass);
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;


	IMPORT_C bool operator==(const TBBBtDeviceInfo& aRhs) const;
};

class CBBBtDeviceList : public CBBGenericList, public MBBDataFactory
{
public:
	IMPORT_C static CBBBtDeviceList* NewL();
	IMPORT_C CBBBtDeviceList();
	IMPORT_C void ConstructL();
	IMPORT_C TBBBtDeviceInfo* First();
	IMPORT_C TBBBtDeviceInfo* Next();
	IMPORT_C const TBBBtDeviceInfo*	First() const;
	IMPORT_C const TBBBtDeviceInfo*	Next() const;
	IMPORT_C void	AddItemL(TBBBtDeviceInfo* aData); // takes ownership
        IMPORT_C virtual const TTypeName& Type() const;
        IMPORT_C static const TTypeName& StaticType();
	IMPORT_C virtual void	AddItemL(HBufC*	aName, MBBData* aData); // takes ownership
	IMPORT_C TBool Equals(const MBBData* aRhs) const;

	IMPORT_C CBBBtDeviceList& operator=(const CBBBtDeviceList& aList);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
private:
	virtual MBBData* CreateBBDataL(const TTypeName& aType, const TDesC& aName, MBBDataFactory* aTopLevelFactory);
	IMPORT_C virtual TBool FixedType() const;

};

class TBBNeighbourhoodInfo : public TBBCompoundData {
public:
	TBBUint	iBuddies;
	TBBUint	iOtherPhones;
	TBBUint	iDesktops; // of the owner
	TBBUint	iLaptops; // of the owner
	TBBUint	iPDAs; // of the owner

        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C TBBNeighbourhoodInfo& operator=(const TBBNeighbourhoodInfo& aInfo);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C const MBBData* Part(TUint aPartNo) const;
	IMPORT_C virtual void	InternalizeL(RReadStream& aStream);
public:
	IMPORT_C TBBNeighbourhoodInfo(TInt aVersion);
	IMPORT_C TBBNeighbourhoodInfo();
	IMPORT_C bool operator==(const TBBNeighbourhoodInfo& aRhs) const;
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	TInt iCreatedVersion, iUseVersion;
};

#endif
