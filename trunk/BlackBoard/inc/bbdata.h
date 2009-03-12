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

#ifndef BB_DATA_H_INCLUDED
#define BB_DATA_H_INCLUDED 1

#include "xmlbuf.h"
#include "xml.h"
#include <s32strm.h>
#include <bamdesca.h>

#include "cc_componentname.h"

_LIT(KAttModule, "module");
_LIT(KAttId, "id");
_LIT(KAttMajorVersion, "major_version");
_LIT(KAttMinorVersion, "minor_version");

struct TTypeName {
	TUid	iModule;
	TInt	iId;
	TInt	iMajorVersion;
	TInt	iMinorVersion;

	IMPORT_C static TTypeName IdFromAttributes(const XML_Char ** atts);
	IMPORT_C void CompareMajorL(const TTypeName& aOther) const;
	IMPORT_C MDesCArray* MakeAttributesLC() const;
	IMPORT_C void ExternalizeL(RWriteStream& aStream) const;
	IMPORT_C static TTypeName IdFromStreamL(RReadStream& aStream);
	IMPORT_C void IntoStringL(TDes& aString) const;

	IMPORT_C bool operator==(const TTypeName& rhs) const;
	IMPORT_C operator TStringArg() const;
};

const int KMaxTupleSubNameLength = 128;

struct TTupleName {
	TUid	iModule;
	TInt	iId;
	IMPORT_C bool operator==(const TTupleName& rhs) const;
	IMPORT_C operator TStringArg() const;
};

class MNestedXmlHandler : public MXmlHandler {
public:
	virtual void SetError(TInt aError) = 0;
	IMPORT_C virtual ~MNestedXmlHandler();
	
	TTimeIntervalSeconds iOffset; // added to times read
	IMPORT_C TTimeIntervalSeconds GetTimeOffset();
	TBool	iIgnoreOffset;
protected:
	IMPORT_C MNestedXmlHandler(MNestedXmlHandler* aParent=0);
	MNestedXmlHandler* iParent;
};

class MBBExternalizer {
public:
	IMPORT_C MBBExternalizer();
	virtual void BeginList(const TDesC& aName,
		TBool aIncludeBBType, const TTypeName& aBBType) = 0;
	virtual void BeginCompound(const TDesC& aName,
		TBool aIncludeBBType, const TTypeName& aBBType) = 0;

	enum TBasicType {
		EInteger=0,	// decimal or hex with leading 0x
		EFloat,		// '.' point, optional sign, optional 0
		EString,
		EBoolean,	// 'true'/'false'
		EDateTime,	// ISO datetime YYYYMMDDTHHMISS
		EBinary		// as hex
	};

	virtual void Field(const TDesC& aName,
		TBasicType aBasicType, const TDesC& aValue,
		TBool aIncludeBBType, const TTypeName& aBBType, 
		TBool aAsAttribute=EFalse) = 0;

	virtual void EndCompound(const TDesC& aName) = 0;
	virtual void EndList(const TDesC& aName) = 0;
	TTimeIntervalSeconds iOffset; // how much to subtract from times
};

class MBBData {
public:
	virtual const TDesC&    Name() const = 0;

	virtual void		IntoStringL(TDes& aString) const = 0;
	virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const = 0;
	virtual void		ExternalizeL(RWriteStream& aStream) const = 0;

	virtual void		FromStringL(const TDesC& aString) = 0;
	virtual MNestedXmlHandler*	FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBool aCheckType=ETrue) = 0;
	virtual void InternalizeL(RReadStream& aStream) = 0;
	IMPORT_C virtual TBool IsAttribute() const;

	virtual const TTypeName& Type() const = 0;
	virtual MBBData*	CloneL(const TDesC& Name) const = 0;

	IMPORT_C virtual ~MBBData();

	/*
	* a MBBData derived class should also have the method:
	static const TTypeName& StaticType();
	* so that bb_cast<>() works
	*/
	virtual TBool Equals(const MBBData* aRhs) const = 0;
	/*
	 * polymorphic operator=
	 * on a compound only changes the fields that have been set
	 * returns this
	 */
	virtual MBBData* Assign(const MBBData* aRhs) = 0;
	
	/*
	 * HACKHACK. Should have QueryInterface instead.
	 */
	IMPORT_C virtual const class TBBCompoundData* IsCompound() const;
protected:
	virtual void SetName(const TDesC* aName) = 0;
	friend class CBBGeneralHolder;
};

IMPORT_C void CleanupPushBBDataL(MBBData* aData);

class MBBDataFactory {
public:
	virtual MBBData* CreateBBDataL(const TTypeName& aType, const TDesC& aName, MBBDataFactory* aTopLevelFactory) = 0;
	// name's lifetime has to be as long or longer than the created object's
	virtual void ConstructL() = 0;
	virtual ~MBBDataFactory() { }
};

class CBBGeneralHolder : public CBase, public MBBData {
public:
	virtual const TDesC&    Name() const;

	IMPORT_C void SetOwnsValue(TBool aOwns);
	IMPORT_C TBool OwnsValue() const;

	IMPORT_C CBBGeneralHolder(const TDesC& aName,
		MBBDataFactory* aFactory, MBBData* aValue=0);
	IMPORT_C ~CBBGeneralHolder();

	IMPORT_C virtual void		IntoStringL(TDes& aString) const;
	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, 
		TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		ExternalizeL(RWriteStream& aStream) const;

	IMPORT_C virtual void		FromStringL(const TDesC& aString);
	IMPORT_C virtual MNestedXmlHandler*	FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBool aCheckType=ETrue);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);

	IMPORT_C virtual const TTypeName& Type() const;

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;
	IMPORT_C virtual MBBData*	CloneL(const TDesC& Name) const;

	IMPORT_C void SetValue(MBBData* aValue);
	const MBBData* operator()() const { return iValue; }
	MBBData* operator()() { return iValue; }
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);
private:
	MBBData*		iValue;
	MBBDataFactory*	iFactory;
	TBool iOwnsValue;
	const TDesC* iName;
	IMPORT_C virtual void SetName(const TDesC* aName);
};

class CBBDataFactory : public CBase, public MBBDataFactory {
public:
	EXPORT_C static CBBDataFactory* NewL();

	virtual MBBData* CreateBBDataL(const TTypeName& aType, const TDesC& aName, MBBDataFactory* aTopLevelFactory) = 0;
	// name's lifetime has to be as long or longer than the created object's
	IMPORT_C void ConstructL();
	IMPORT_C virtual ~CBBDataFactory();
	virtual void Reset() = 0;
};

IMPORT_C void GetBackwardsCompatibleUid(TUint& uid3);

template<typename T>
T* bb_cast(MBBData* o) { 
	if (!o) return 0; 
	if (! (o->Type() == T::StaticType())) return 0; 
	return static_cast<T*>(o);
}

template<typename T>
const T* bb_cast(const MBBData* o) { 
	if (!o) return 0; 
	if (! (o->Type() == T::StaticType())) return 0; 
	return static_cast<const T*>(o);
}

#define IMPLEMENT_ASSIGN(TypeName) \
EXPORT_C MBBData* TypeName::Assign(const MBBData* aRhs) \
{ \
	const TypeName * rhs = bb_cast< TypeName > (aRhs); \
	if (!rhs) return this; \
	*this=*rhs; \
	return this; \
}

#endif
