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

#ifndef BB_CONCRETEDATA_H_INCLUDED
#define BB_CONCRETEDATA_H_INCLUDED 1

#pragma warning(disable: 4800)

#include "bbdata.h"
#include "bbutil.h"
#include "bberrors.h"
#include "concretedata.hrh"

class TBBNamedData : public MBBData {
public:
	TBBNamedData(const TDesC& aName) : iName(&aName) { }

	IMPORT_C virtual const TDesC&	Name() const;
private:
	IMPORT_C virtual void SetName(const TDesC* aName);
	const TDesC*		iName;
};

class TBBSimpleData : public TBBNamedData {
public:
	IMPORT_C virtual MNestedXmlHandler*	FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBool aCheckType=ETrue);
	TBBSimpleData(const TDesC& aName) : TBBNamedData(aName), iIsAttribute(EFalse) { }
	IMPORT_C virtual TBool IsAttribute() const;
	IMPORT_C virtual void SetIsAttribute(TBool aIs);
protected:
	TBool iIsAttribute;
};

class TBBInt : public TBBSimpleData  {
public:
	TInt			iValue;

	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		IntoStringL(TDes& aString) const;
	IMPORT_C virtual void		ExternalizeL(RWriteStream& aStream) const;

	IMPORT_C virtual void		FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);

public:
	TBBInt(const TDesC& aName) : TBBSimpleData(aName), iValue(0) { }
	TBBInt(TInt aValue, const TDesC& aName) : TBBSimpleData(aName), iValue(aValue) { }
	TBBInt& operator=(TInt aValue) { iValue=aValue; return *this; }

	TInt& operator()() { return iValue; }
	const TInt& operator()() const { return iValue; }
	bool operator==(const TBBInt& aRhs) const { return iValue==aRhs.iValue; }
};

class TBBFixedLengthStringBase  : public TBBSimpleData {
public:
	TBBFixedLengthStringBase(const TDesC& aName) : TBBSimpleData(aName) { }
	IMPORT_C void IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C void IntoStringL(TDes& aString) const;
	IMPORT_C void ExternalizeL(RWriteStream& aStream) const;
	IMPORT_C void FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);

	virtual TDes& Value() = 0;
	virtual const TDesC& Value() const = 0;
	bool operator==(const TBBFixedLengthStringBase& aRhs) const { return !(Value().Compare(aRhs.Value())); }
private:
	TBBFixedLengthStringBase(const TBBFixedLengthStringBase&);
};

class TBBFixedLengthStringBase8 : public TBBSimpleData {
public:
	TBBFixedLengthStringBase8(const TDesC& aName) : TBBSimpleData(aName) { }
	IMPORT_C void IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C void IntoStringL(TDes& aString) const;
	IMPORT_C void ExternalizeL(RWriteStream& aStream) const;
	IMPORT_C void FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);

	virtual TDes8& Value() = 0;
	virtual const TDesC8& Value() const = 0;
};


const TInt KBBShortStringMaxLen(BB_SHORTSTRING_MAXLEN);
class TBBShortString : public TBBFixedLengthStringBase {
public:
	TBuf<BB_SHORTSTRING_MAXLEN> iValue;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	TDes& operator()() { return iValue; }
	const TDesC& operator()() const { return iValue; }

	virtual TDes& Value() { return iValue; }
	virtual const TDesC& Value() const  { return iValue; }
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
public:

	TBBShortString(const TDesC& aName) : TBBFixedLengthStringBase(aName) { }
	TBBShortString(const TDesC& aValue, const TDesC& aName) : TBBFixedLengthStringBase(aName) { iValue=aValue.Left(KBBShortStringMaxLen); }
	TBBShortString& operator=(const TDesC& aValue) { iValue=aValue.Left(KBBShortStringMaxLen); return *this; }
	//bool operator==(const TBBShortString& aRhs) const { return !(iValue.Compare(aRhs.iValue)); }
private:
	TBBShortString(const TBBShortString&);
};

class TBBShortString8 : public TBBFixedLengthStringBase8 {
public:
	TBuf8<BB_SHORTSTRING_MAXLEN> iValue;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	TDes8& operator()() { return iValue; }
	const TDesC8& operator()() const { return iValue; }

	virtual TDes8& Value() { return iValue; }
	virtual const TDesC8& Value() const  { return iValue; }
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);
public:

	TBBShortString8(const TDesC& aName) : TBBFixedLengthStringBase8(aName) { }
	TBBShortString8(const TDesC8& aValue, const TDesC& aName) : TBBFixedLengthStringBase8(aName) { iValue=aValue.Left(KBBShortStringMaxLen); }
	TBBShortString8& operator=(const TDesC8& aValue) { iValue=aValue.Left(KBBShortStringMaxLen); return *this; }
	bool operator==(const TBBShortString8& aRhs) const { return !(iValue.Compare(aRhs.iValue)); }
};


const TInt KBBLongStringMaxLen( BB_LONGSTRING_MAXLEN );
class TBBLongString : public TBBFixedLengthStringBase {
public:
	TBuf<BB_LONGSTRING_MAXLEN> iValue;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	TDes& operator()() { return iValue; }
	const TDesC& operator()() const { return iValue; }

	virtual TDes& Value() { return iValue; }
	virtual const TDesC& Value() const  { return iValue; }
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
public:

	TBBLongString(const TDesC& aName) : TBBFixedLengthStringBase(aName) { }
	TBBLongString(const TDesC& aValue, const TDesC& aName) : TBBFixedLengthStringBase(aName) { iValue=aValue.Left(KBBLongStringMaxLen); }
	TBBLongString& operator=(const TDesC& aValue) { iValue=aValue.Left(KBBLongStringMaxLen); return *this; }
	//bool operator==(const TBBLongString& aRhs) const { return !(iValue.Compare(aRhs.iValue)); }
};

class TBBBool : public TBBSimpleData {
public:
	TBool			iValue;

	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		IntoStringL(TDes& aString) const;
	IMPORT_C virtual void		ExternalizeL(RWriteStream& aStream) const;

	IMPORT_C virtual void		FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);
public:
	TBBBool(const TDesC& aName) : TBBSimpleData(aName), iValue() { }
	TBBBool(TBool aValue, const TDesC& aName) : TBBSimpleData(aName), iValue(aValue) {  }
	TBBBool& operator=(TBool aValue) { iValue=aValue; return *this; }

	TBool& operator()() { return iValue; }
	const TBool& operator()() const { return iValue; }

	bool operator==(const TBBBool& aRhs) const { 
		return ( (iValue && aRhs.iValue) || (!iValue && !aRhs.iValue)); 
	}
};

class TBBTime : public TBBSimpleData {
public:
	TTime			iValue;

	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		IntoStringL(TDes& aString) const;
	IMPORT_C virtual void		ExternalizeL(RWriteStream& aStream) const;

	IMPORT_C virtual void		FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);
	IMPORT_C virtual MNestedXmlHandler* FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBool aCheckType=ETrue);

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);
public:
	TBBTime(const TDesC& aName) : TBBSimpleData(aName), iValue(0) { }
	TBBTime(TTime aValue, const TDesC& aName) : TBBSimpleData(aName), iValue(aValue) {  }
	TBBTime& operator=(TTime aValue) { iValue=aValue; return *this; }

	TTime& operator()() { return iValue; }
	const TTime& operator()() const { return iValue; }

	bool operator==(const TBBTime& aRhs) const { 
		return (iValue==aRhs.iValue); 
	}
};

class TBBUint : public TBBSimpleData  {
public:
	TUint			iValue;

	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		IntoStringL(TDes& aString) const;
	IMPORT_C virtual void		ExternalizeL(RWriteStream& aStream) const;

	IMPORT_C virtual void		FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);
public:
	TBBUint(const TDesC& aName) : TBBSimpleData(aName), iValue(0) { }
	TBBUint(TUint aValue, const TDesC& aName) : TBBSimpleData(aName), iValue(aValue) { }
	TBBUint& operator=(TUint aValue) { iValue=aValue; return *this; }

	TUint& operator()() { return iValue; }
	const TUint& operator()() const { return iValue; }

	bool operator==(const TBBUint& aRhs) const { return iValue==aRhs.iValue; }
};

class TBBUid : public TBBUint {
public:
	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		IntoStringL(TDes& aString) const;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);

	TBBUid(const TDesC& aName) : TBBUint(0, aName) { }
	TBBUid(TUint aValue, const TDesC& aName) : TBBUint(aValue, aName) { }
	bool operator==(const TBBUid& aRhs) const { return iValue==aRhs.iValue; }
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
};


class TBBCompoundData : public TBBNamedData {
	// if using the default ReadPart():
	// max 32 parts, because read parts are kept in
	// a 32 bit bitmap
public:
	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		IntoStringL(TDes& aString) const;
	IMPORT_C virtual void		ExternalizeL(RWriteStream& aStream) const;

	IMPORT_C virtual void		FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);
	IMPORT_C virtual MNestedXmlHandler*	FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBool aCheckType=ETrue);
	TBBCompoundData(const TDesC& aName) : TBBNamedData(aName), iReadParts(0) { }
	IMPORT_C virtual void		ReadPart(TUint aPart, TBool aErrors); // default implementation
	// sets corresponding bit in iReadParts
	IMPORT_C virtual TBool		DidReadPart(TUint aPart) const;
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);
	IMPORT_C virtual const class TBBCompoundData* IsCompound() const;
protected:
	IMPORT_C virtual MBBData*	GetPart(const TDesC& aName,
		const TTypeName& aType, TUint& aPartNoInto);
	virtual const MBBData* Part(TUint aPartNo) const = 0;
	virtual const TDesC& StringSep(TUint aBeforePart) const = 0;

	friend class CPartContainerXml;
	TUint	iReadParts;
};

class CBBString : public TBBFixedLengthStringBase, public CBase {
public:
	HBufC*		iBuf;
	TPtr		iPtr;

	IMPORT_C static CBBString* NewL(const TDesC& aName, const TInt aSize=256);
	IMPORT_C void Zero();
	IMPORT_C void Append(const TDesC& aStr);
	IMPORT_C void TakeHBufC(HBufC* aBuf); // releases the current iBuf, replaces with aBuf and takes ownership

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;
	IMPORT_C void FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);

	const TDesC& operator()() const { if (iBuf) return *iBuf; else return KNullDesC; }

	virtual TDes& Value() { return iPtr; }
	virtual const TDesC& Value() const  { if (iBuf) return *iBuf; else return KNullDesC; }
	bool operator==(const CBBString& aRhs) const { return Value()==aRhs.Value(); }
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);

	IMPORT_C ~CBBString();
	IMPORT_C CBBString(const TDesC& aName);
	IMPORT_C void ConstructL(TInt aSize=512);
	IMPORT_C void SyncPtr(); // call this if you manipulate Value() directly
};


class CBBString8 : public TBBFixedLengthStringBase8, public CBase {
public:
	HBufC8*		iBuf;
	TPtr8		iPtr;

	IMPORT_C static CBBString8* NewL(const TDesC& aName, const TInt aSize=256);
	IMPORT_C void Zero();
	IMPORT_C void Append(const TDesC8& aStr);

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;
	IMPORT_C void FromStringL(const TDesC8& aString);
	IMPORT_C void FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);

	const TDesC8& operator()() const { if (iBuf) return *iBuf; return KNullDesC8; }

	virtual const TDesC8& Value() const  { if (iBuf) return *iBuf; return KNullDesC8; }
	bool operator==(const CBBString8& aRhs) const { return Value()==aRhs.Value(); }
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;

	IMPORT_C ~CBBString8();
	IMPORT_C void ConstructL(TInt aSize=128);
	IMPORT_C CBBString8(const TDesC& aName);
private:
	virtual TDes8& Value() { return iPtr; }
};

class TBBInt64 : public TBBSimpleData  {
public:
	TInt64			iValue;

	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		IntoStringL(TDes& aString) const;
	IMPORT_C virtual void		ExternalizeL(RWriteStream& aStream) const;

	IMPORT_C virtual void		FromStringL(const TDesC& aString);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);
public:
	TBBInt64(const TDesC& aName) : TBBSimpleData(aName), iValue(0) { }
	TBBInt64(TInt64 aValue, const TDesC& aName) : TBBSimpleData(aName), iValue(aValue) { }
	TBBInt64& operator=(TInt64 aValue) { iValue=aValue; return *this; }

	TInt64& operator()() { return iValue; }
	const TInt64& operator()() const { return iValue; }

	bool operator==(const TBBInt64& aRhs) const { return iValue==aRhs.iValue; }
};

class TBBTupleName : public TBBCompoundData {
public:
	TBBUid			iModuleUid;
	TBBInt			iModuleId;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;

	IMPORT_C TBBTupleName(const TDesC& aName);
	IMPORT_C TBBTupleName(const TDesC& aName, TInt aModuleUid, TInt aModuleId);

	IMPORT_C bool operator==(const TBBTupleName& aRhs) const;
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;
	IMPORT_C TBBTupleName& operator=(const TBBTupleName& aNameData);
	IMPORT_C TBBTupleName& operator=(const TTupleName& aNameData);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
};

class TBBComponentName : public TBBTupleName {
public:
	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C TBBComponentName(const TDesC& aName);
	IMPORT_C TBBComponentName(const TDesC& aName, TInt aModuleUid, TInt aModuleId);

	IMPORT_C TBBComponentName& operator=(const TComponentName& aNameData);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
};

#endif
