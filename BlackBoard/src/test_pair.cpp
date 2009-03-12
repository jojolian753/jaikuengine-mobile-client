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

#include "bbtypes.h"
#include "concretedata.h"
#include "bberrors.h"
#include "bbutil.h"
#include "bbxml.h"

const TTypeName KPairType = { KBBUidValue, 1002, 1, 0 };
_LIT(KFirst, "first");
_LIT(KSecond, "second");

class TBBPair : public MBBData {
public:
	const TDesC&		iName;

	TBBInt			iIntVal;
	TBBShortString		iStringVal;

	virtual void		IntoStringL(TDes& aString) const;
	virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	virtual void		ExternalizeL(RWriteStream& aStream) const;

	virtual void		FromStringL(const TDesC& aString);
	virtual MNestedXmlHandler*	FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBool aCheckType=ETrue);
	virtual void		InternalizeL(RReadStream& aStream);
	virtual const TDesC&	Name() const;
	virtual MBBData*	CloneL(const TDesC& aName) const; 

	virtual const TTypeName& Type() const;
	TBool Equals(const MBBData* aRhs) const;

	TBBPair(const TDesC& aName) : iName(aName), iIntVal(KFirst), iStringVal(KSecond) { }
	TBBPair(TInt aIntValue, const TDesC& aStringVal, const TDesC& aName) : iName(aName), iIntVal(aIntValue, KFirst), iStringVal(aStringVal, KSecond) { }
	TBBPair& operator=(const TBBPair aValue) { iIntVal=aValue.iIntVal.iValue; iStringVal=aValue.iStringVal.iValue; return *this; }
	bool operator==(const TBBPair& aRhs) const  { return iIntVal==aRhs.iIntVal &&
		iStringVal==aRhs.iStringVal; }
};

class CPairXml : public CContainerXml {
public:
	CPairXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBBPair& aValue, TBool aCheckType) : 
	CContainerXml(aParent, aParser, aBuf, aValue, aCheckType), iPairValue(aValue) { }
private:
	TBBPair& iPairValue;
	virtual void StartInnerElementL(const XML_Char *name, const XML_Char **atts) {
		TPtrC namep(name);
		if (! namep.Compare(_L("first")) ) {
			SetCurrentHandler( iPairValue.iIntVal.FromXmlL(this, iParser,
				iBuf, iCheckType) );
			GetCurrentHandler()->StartElement(name, atts);
		} else if (! namep.Compare(_L("second")) ) {
			SetCurrentHandler( iPairValue.iStringVal.FromXmlL(this, iParser,
				iBuf, iCheckType) );
			GetCurrentHandler()->StartElement(name, atts);
		} else {
			User::Leave(KUnexpectedElement);
		}
	}

	virtual void EndInnerElementL(const XML_Char * /*name*/) {
	}

	virtual void Error(XML_Error Code, const XML_LChar * /*String*/, long /*ByteIndex*/) {
		SetError(Code);
	}
	virtual void SetError(TInt aError) {
		User::Leave(aError);
	}
};

MBBData* TBBPair::CloneL(const TDesC& aName) const
{
	TBBPair* p=new (ELeave) TBBPair(aName);
	p->iIntVal()=iIntVal();
	p->iStringVal()=iStringVal();
	return p;
}

void TBBPair::IntoStringL(TDes& aString) const
{
	iIntVal.IntoStringL(aString);
	CheckStringSpaceL(aString, 1);
	aString.Append(_L(" "));
	iStringVal.IntoStringL(aString);
}

void TBBPair::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	aBuf->BeginCompound(Name(), aIncludeType, Type());
	iIntVal.IntoXmlL(aBuf, aIncludeType);
	iStringVal.IntoXmlL(aBuf, aIncludeType);
	aBuf->EndCompound(Name());
}

void TBBPair::ExternalizeL(RWriteStream& aStream) const
{
	iIntVal.ExternalizeL(aStream);
	iStringVal.ExternalizeL(aStream);
}

void TBBPair::FromStringL(const TDesC& aString)
{
	TLex aLex(aString);
	aLex.Mark();
	aLex.SkipCharacters();
	iIntVal.FromStringL(aLex.MarkedToken());
	aLex.SkipSpace();
	aLex.Mark();
	iStringVal.FromStringL(aLex.RemainderFromMark());
}

MNestedXmlHandler* TBBPair::FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser, HBufC*& aBuf, TBool aCheckType)
{
	return new(ELeave) CPairXml(aParent, aParser, aBuf, *this, aCheckType);
}

void TBBPair::InternalizeL(RReadStream& aStream)
{
	iIntVal.InternalizeL(aStream);
	iStringVal.InternalizeL(aStream);
}

const TTypeName& TBBPair::Type() const
{
	return KPairType;
}

const TDesC& TBBPair::Name() const
{
	return iName;
}

TBool TBBPair::Equals(const MBBData* aRhs) const
{
	if (!aRhs) return EFalse;
	if (aRhs->Type() == Type() ) return EFalse;
	const TBBPair* rhs=static_cast<const TBBPair*>(aRhs);
	return (iIntVal==rhs->iIntVal && iStringVal==rhs->iStringVal);
}
