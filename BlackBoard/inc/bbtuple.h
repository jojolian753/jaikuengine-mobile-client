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

#ifndef CONTEXT_BBTUPLE_H_INCLUDED
#define CONTEXT_BBTUPLE_H_INCLUDED 1

#include "bbdata.h"
#include "concretedata.h"
#include "context_uids.h"
#include "bbtypes.h"

//const TTypeName KTupleType = { { CONTEXT_UID_SENSORDATAFACTORY }, 7, 1, 0 };
const TTypeName KSubNameType = { { CONTEXT_UID_SENSORDATAFACTORY }, 8, 1, 0 };
//const TTypeName KTupleMetaType = { { CONTEXT_UID_SENSORDATAFACTORY }, 9, 1, 0 };


//_LIT(KStamp, "datetime");
//_LIT(KPriority, "priority");
_LIT(KId, "id");
_LIT(KMeta, "tuplename");
//_LIT(KModuleUid, "module_uid");
//_LIT(KModuleId, "module_id");
//_LIT(KSubName, "subname");



class TBBTupleSubName  : public TBBFixedLengthStringBase {
public:
	TBuf<128> iValue;
	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();

	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	TDes& operator()() { return iValue; }
	const TDesC& operator()() const { return iValue; }

	virtual TDes& Value() { return iValue; }
	virtual const TDesC& Value() const  { return iValue; }
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
public:
	TBBTupleSubName(const TDesC& aName) : TBBFixedLengthStringBase(aName) { }
	TBBTupleSubName(const TDesC& aValue, const TDesC& aName) : TBBFixedLengthStringBase(aName) { iValue=aValue.Left(128); }
	TBBTupleSubName& operator=(const TDesC& aValue) { iValue=aValue.Left(128); return *this; }
	bool operator==(const TBBTupleSubName& aRhs) const { return !(iValue.Compare(aRhs.iValue)); }
};

#include "csd_tuplemeta.h"

#if 1
#include "csd_tuple.h"

#else
_LIT(KExpires, "expires");

class CBBTuple : public CBase, public TBBCompoundData {
public:
	TBBUint			iTupleId;
	TBBTupleMeta		iTupleMeta;
	TBBTime			iExpires;
	CBBGeneralHolder	iData;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;

	IMPORT_C CBBTuple(MBBDataFactory* aFactory/*,
		TUint iTupleId=0, TInt aModuleUid=0, TInt aModuleId=0,
		const TDesC& aSubName=KNullDesC, MBBData* aData=0*/);

	IMPORT_C bool operator==(const CBBTuple& aRhs) const;
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C CBBTuple& operator=(const CBBTuple& aTuple);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
private:
	MBBData* GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto);
	MBBDataFactory*	iFactory;
	TBuf<50>	iValueName;
};
#endif

#endif
