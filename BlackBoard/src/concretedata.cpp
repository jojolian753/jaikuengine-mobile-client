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

#include "concretedata.h"
#include "bberrors.h"
#include "bbtypes.h"
#include "bbutil.h"
#include "bbxml.h"
#include "compat_int64.h"

void LeaveCannotParseValue()
{
	User::Leave(KCannotParseValue);
}

EXPORT_C const TDesC& TBBNamedData::Name() const
{
	return *iName;
}
EXPORT_C MNestedXmlHandler* TBBSimpleData::FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser, 
						    HBufC*& aBuf, TBool aCheckType)
{
	return new(ELeave) TSingleXml(aParent, aParser, aBuf, *this, aCheckType);
}

EXPORT_C void TBBNamedData::SetName(const TDesC* aName)
{
	iName=aName;
}

EXPORT_C TBool TBBSimpleData::IsAttribute() const
{
	return iIsAttribute;
}

EXPORT_C void TBBSimpleData::SetIsAttribute(TBool aIs)
{
	iIsAttribute=aIs;
}

EXPORT_C void TBBInt::IntoStringL(TDes& aString) const
{
	TBuf<20> b;
	b.AppendNum(iValue);
	CheckStringSpaceL(aString, b.Length());
	aString.AppendNum(iValue);
}

EXPORT_C void TBBInt::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	TBuf<15> buf;
	buf.AppendNum(iValue);
	aBuf->Field(Name(), MBBExternalizer::EInteger, 
		buf, aIncludeType, Type(), iIsAttribute);
}

EXPORT_C TBool TBBInt::Equals(const MBBData* aRhs) const
{
	const TBBInt* rhs=bb_cast<TBBInt>(aRhs);
	return (rhs && iValue==rhs->iValue);
}

EXPORT_C void TBBInt::ExternalizeL(RWriteStream& aStream) const
{
	aStream.WriteInt32L(iValue);
}

EXPORT_C void TBBInt::FromStringL(const TDesC& aString)
{
	TInt err; TLex aLex;
	if (aString.Left(2).CompareF(_L("0x"))) {
		aLex.Assign(aString);
		err=aLex.Val(iValue);
	} else {
		aLex.Assign(aString.Mid(2));
		TUint v;
		err=aLex.Val(v, EHex);
		iValue=(TInt)v;
	}
	if (err==KErrNone) {
		if (aLex.Remainder().Length()==0) return;
		else 
			LeaveCannotParseValue();
	}
	if (err==KErrGeneral) 
		LeaveCannotParseValue();
	User::Leave(err);
}

EXPORT_C void TBBInt::InternalizeL(RReadStream& aStream)
{
	iValue=aStream.ReadInt32L();
}

EXPORT_C const TTypeName& TBBInt::Type() const
{
	return KIntType;
}

EXPORT_C const TTypeName& TBBInt::StaticType()
{
	return KIntType;
}


EXPORT_C const TTypeName& TBBShortString::Type() const
{
	return KShortStringType;
}

EXPORT_C const TTypeName& TBBShortString::StaticType()
{
	return KShortStringType;
}

EXPORT_C TBool TBBShortString::Equals(const MBBData* aRhs) const
{
	const TBBShortString* rhs=bb_cast<TBBShortString>(aRhs);
	return (rhs && iValue.Compare(rhs->iValue)==0);
}


EXPORT_C void TBBBool::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	TBuf<5> b;
	if (iValue) b=_L("true");
	else b=_L("false");

	aBuf->Field(Name(), MBBExternalizer::EBoolean, b, 
		aIncludeType, Type(), iIsAttribute);
}

EXPORT_C void TBBBool::IntoStringL(TDes& aString) const
{
	TBuf<5> b;
	if (iValue) b=_L("true");
	else b=_L("false");

	CheckStringSpaceL(aString, 5);
	aString.Append(b);
}

EXPORT_C void TBBBool::ExternalizeL(RWriteStream& aStream) const
{
	if (iValue)
		aStream.WriteInt8L(1);
	else
		aStream.WriteInt8L(0);
}

EXPORT_C void TBBBool::FromStringL(const TDesC& aString)
{
	if (aString.CompareF(_L("true"))==0 || aString.CompareF(_L("1"))==0 )
		iValue=ETrue;
	else if (aString.CompareF(_L("false"))==0 || aString.CompareF(_L("0"))==0 ) 
		iValue=EFalse;
	else LeaveCannotParseValue();	
}

EXPORT_C void TBBBool::InternalizeL(RReadStream& aStream)
{
	TInt8 val=aStream.ReadInt8L();
	if (val) iValue=ETrue;
	else iValue=EFalse;
}

EXPORT_C const TTypeName& TBBBool::Type() const
{
	return KBoolType;
}

EXPORT_C const TTypeName& TBBBool::StaticType()
{
	return KBoolType;
}

EXPORT_C TBool TBBBool::Equals(const MBBData* aRhs) const
{
	const TBBBool* rhs=bb_cast<TBBBool>(aRhs);
	return (rhs && *this == *rhs);
}

EXPORT_C void TBBTime::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	TBuf<16> b;
	if (aBuf->iOffset.Int()!=0) {
		TBBTime shifted( iValue, Name() );
		shifted()-=aBuf->iOffset;
		shifted.IntoStringL(b);
	} else {
		IntoStringL(b);
	}

	aBuf->Field(Name(), MBBExternalizer::EDateTime, b, aIncludeType, 
		Type(), iIsAttribute);
}

class TTimeXml : public TSingleXml {
public:
	TTimeXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBBTime& aValue, TBool aCheckType);
	virtual void EndElementL(const XML_Char *name);
private:
	TBBTime& iTimeValue;
};

TTimeXml::TTimeXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
	HBufC*& aBuf, TBBTime& aValue, TBool aCheckType) : TSingleXml(aParent, aParser,
	aBuf, aValue, aCheckType),  iTimeValue(aValue) { }
	
void TTimeXml::EndElementL(const XML_Char *name)
{
	TSingleXml::EndElementL(name);
	iTimeValue()+=GetTimeOffset();
}

EXPORT_C MNestedXmlHandler* TBBTime::FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
	HBufC*& aBuf, TBool aCheckType)
{
	return new (ELeave) TTimeXml(aParent, aParser, aBuf, *this, aCheckType);
}

EXPORT_C void TBBTime::IntoStringL(TDes& aString) const
{
	TBuf<16> b;
	TDateTime dt;
	dt=iValue.DateTime();
	_LIT(KFormatTxt,"%04d%02d%02dT%02d%02d%02d");
	b.Format(KFormatTxt, dt.Year(), (TInt)dt.Month()+1, (TInt)dt.Day()+1,
		dt.Hour(), dt.Minute(), dt.Second());

	CheckStringSpaceL(aString, 15);
	aString.Append(b);
}

EXPORT_C void TBBTime::ExternalizeL(RWriteStream& aStream) const
{
	TInt64 val=iValue.Int64();

	aStream.WriteInt32L(I64HIGH(val));
	aStream.WriteInt32L(I64LOW(val));
}

TTime ParseTimeL(const TDesC& Str)
{
	if (Str.Length() < 15) LeaveCannotParseValue();

		TInt year, month, day, hour, min, sec, err=KErrNone;
        TLex lex;
        
		lex=Str.Mid(0, 4);
        err += lex.Val(year);

        lex=Str.Mid(4, 2);
        err += lex.Val(month);

        lex=Str.Mid(6, 2);
        err += lex.Val(day);

        lex=Str.Mid(9, 2);
        err += lex.Val(hour);

        lex=Str.Mid(11, 2);
        err += lex.Val(min);

        lex=Str.Mid(13, 2);
        err += lex.Val(sec);
	
	TTime Stamp(0);

	if (err ==KErrNone) {
		TDateTime dt; 
		if ( month > 0 ) month--; // TDateTime's internal format
		if ( day > 0 ) day--;  // TDateTime's internal format
		dt.Set(year,TMonth(month),day,hour,min,sec,0);
		Stamp=TTime(dt);
		return Stamp;
	} else {
		LeaveCannotParseValue();
	}
	return Stamp;
}

EXPORT_C void TBBTime::FromStringL(const TDesC& aString)
{
	iValue=ParseTimeL(aString);
}

EXPORT_C void TBBTime::InternalizeL(RReadStream& aStream)
{
	TInt32 low, high;
	high=aStream.ReadInt32L();
	low=aStream.ReadInt32L();
	iValue=MAKE_TINT64(high, low);
}

EXPORT_C const TTypeName& TBBTime::Type() const
{
	return KTimeType;
}

EXPORT_C const TTypeName& TBBTime::StaticType()
{
	return KTimeType;
}

EXPORT_C TBool TBBTime::Equals(const MBBData* aRhs) const
{
	const TBBTime* rhs=bb_cast<TBBTime>(aRhs);
	return (rhs && *this == *rhs);
}

EXPORT_C void TBBUint::IntoStringL(TDes& aString) const
{
	TBuf<20> b;
	b.AppendNum(iValue);
	CheckStringSpaceL(aString, b.Length());
	aString.AppendNum(iValue);
}

EXPORT_C void TBBUint::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	TBuf<15> buf;
	buf.AppendNum(iValue);

	aBuf->Field(Name(), MBBExternalizer::EInteger, buf, aIncludeType, 
		Type(), iIsAttribute);

}

EXPORT_C TBool TBBUint::Equals(const MBBData* aRhs) const
{
	const TBBUint* rhs=bb_cast<TBBUint>(aRhs);
	return (rhs && iValue==rhs->iValue);
}

EXPORT_C void TBBUint::ExternalizeL(RWriteStream& aStream) const
{
	aStream.WriteUint32L(iValue);
}

EXPORT_C void TBBUint::FromStringL(const TDesC& aString)
{
	TLex aLex; TInt err;
	if (aString.Left(2).CompareF(_L("0x"))) {
		aLex.Assign(aString);
		err=aLex.Val(iValue);
	} else {
		aLex.Assign(aString.Mid(2));
		err=aLex.Val(iValue, EHex);
	}
	if (err==KErrNone) {
		if (aLex.Remainder().Length()==0) return;
		else LeaveCannotParseValue();
	}
	if (err==KErrGeneral) LeaveCannotParseValue();
	User::Leave(err);
}

EXPORT_C void TBBUint::InternalizeL(RReadStream& aStream)
{
	iValue=aStream.ReadUint32L();
}

EXPORT_C const TTypeName& TBBUint::Type() const
{
	return KUintType;
}

EXPORT_C const TTypeName& TBBUint::StaticType()
{
	return KUintType;
}

EXPORT_C void TBBCompoundData::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	aBuf->BeginCompound(Name(), aIncludeType, Type());

	TInt i=0;
	const MBBData *p;
	for (i=0, p=Part(i); p; p=Part(++i)) {
		if (p->IsAttribute())
			p->IntoXmlL(aBuf, EFalse);
	}
	for (i=0, p=Part(i); p; p=Part(++i)) {
		if (! p->IsAttribute())
			p->IntoXmlL(aBuf, EFalse);
	}

	aBuf->EndCompound(Name());
}

EXPORT_C void TBBCompoundData::IntoStringL(TDes& aString) const
{
	TUint i=0;
	for (const MBBData *p=Part(0); p; p=Part(++i)) {
		const TDesC& sep=StringSep(i);
		CheckStringSpaceL(aString, sep.Length());
		aString.Append(sep);
		p->IntoStringL(aString);
	}
	const TDesC& sep=StringSep(i);
	CheckStringSpaceL(aString, sep.Length());
	aString.Append(sep);
}

EXPORT_C void TBBCompoundData::ExternalizeL(RWriteStream& aStream) const
{
	TUint i=0, count=0;
	const MBBData *p=0;
	for (p=Part(i); p; p=Part(++i)) {
		count=i+1;
	}
	aStream.WriteInt32L(count);
	i=0;
	for (p=Part(i); p; p=Part(++i)) {
		p->ExternalizeL(aStream);
	}
}


EXPORT_C void TBBCompoundData::FromStringL(const TDesC&)
{
	User::Leave(KErrNotSupported);
}

EXPORT_C void TBBCompoundData::InternalizeL(RReadStream& aStream)
{
	TInt i=0, count=0, part;
	count=aStream.ReadInt32L();
	TTypeName t;
	for (i=0; i<count; i++) {
		MBBData *p=(MBBData *)Part(i);
		if (p==0) User::Leave(KErrCorrupt);
		p->InternalizeL(aStream);
	}
}

EXPORT_C MNestedXmlHandler* TBBCompoundData::FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
				HBufC*& aBuf, TBool aCheckType)
{
	iReadParts=0;
	return new(ELeave) CPartContainerXml(aParent, aParser, aBuf, *this, aCheckType);
}

EXPORT_C void	TBBCompoundData::ReadPart(TUint aPart, TBool aErrors)
{
	if (!aErrors)
		iReadParts ^= 0x1U << aPart;
}

EXPORT_C TBool TBBCompoundData::DidReadPart(TUint aPart) const
{
	return (iReadParts & (0x1U << aPart) );
}

EXPORT_C MBBData* TBBCompoundData::GetPart(const TDesC& aName, 
										   const TTypeName&, TUint& aPartNoInto)
{
	TUint i=0;
	for (MBBData *p=(MBBData *)Part(i); p; p=(MBBData *)Part(++i)) {
		if (p->Name().Compare(aName)==0) {
			aPartNoInto=i;
			return p;
		}
	}
	return 0;
}


EXPORT_C void TBBFixedLengthStringBase::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const 
{
	aBuf->Field(Name(), MBBExternalizer::EString, Value(), 
		aIncludeType, Type(), iIsAttribute);
}

EXPORT_C void TBBFixedLengthStringBase::IntoStringL(TDes& aString) const 
{
	CheckStringSpaceL(aString, Value().Length());
	aString.Append(Value());
}

EXPORT_C void TBBFixedLengthStringBase::ExternalizeL(RWriteStream& aStream) const 
{
	aStream.WriteInt32L(Value().Length());
	if (Value().Length()>0) {
		aStream.WriteL(Value());
	}
}

EXPORT_C void TBBFixedLengthStringBase::FromStringL(const TDesC& aString) 
{
	if (aString.Length()>Value().MaxLength()) User::Leave(KStringTooLong);
	Value()=aString;
}

EXPORT_C void TBBFixedLengthStringBase::InternalizeL(RReadStream& aStream) 
{
	TInt aLen=aStream.ReadInt32L();
	if (aLen > Value().MaxLength()) User::Leave(KStringTooLong);
	if (aLen>0) 
		aStream.ReadL(Value(), aLen);
	else
		Value().Zero();
}

EXPORT_C void TBBFixedLengthStringBase8::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const 
{
	auto_ptr<HBufC> buf(HBufC::NewL(Value().Length()*2));

	TPtr p(buf->Des());
	IntoStringL(p);
	aBuf->Field(Name(), MBBExternalizer::EBinary, *buf, 
		aIncludeType, Type(), iIsAttribute);
}

EXPORT_C void TBBFixedLengthStringBase8::IntoStringL(TDes& aString) const 
{
	CheckStringSpaceL(aString, Value().Length()*2);
	TBuf<4> hex, b;
	const TDesC8& val=Value();
	for (TInt i=0; i<val.Length(); i++) {
		hex.Num( val[i], EHex );
		if (hex.Length()==1) b=_L("0");
		else b.Zero();
		b.Append(hex);
		aString.Append(b);
	}
}

EXPORT_C void TBBFixedLengthStringBase8::ExternalizeL(RWriteStream& aStream) const 
{
	aStream.WriteInt32L(Value().Length());
	if (Value().Length()>0)
		aStream.WriteL(Value());
}

EXPORT_C void TBBFixedLengthStringBase8::FromStringL(const TDesC& aString) 
{
	if (aString.Length()/2 > Value().MaxLength()) User::Leave(KStringTooLong);

	TDes8& val=Value();
	val.Zero();

	TBuf<2> hex;
	for (TInt i=0; i<aString.Length(); i+=2) {
		TUint8 c;
		hex=aString.Mid(i, 2);
		TLex l(hex);
		User::LeaveIfError( l.Val(c, EHex) );
		val.Append(c);
	}
}

EXPORT_C void TBBFixedLengthStringBase8::InternalizeL(RReadStream& aStream) 
{
	TInt aLen=aStream.ReadInt32L();
	if (aLen > Value().MaxLength()) User::Leave(KStringTooLong);
	if (aLen>0)
		aStream.ReadL(Value(), aLen);
	else
		Value().Zero();
}


EXPORT_C void TBBUid::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	TBuf<15> buf=_L("0x");
	buf.AppendNum(iValue, EHex);
	aBuf->Field(Name(), MBBExternalizer::EInteger, buf, 
		aIncludeType, Type(), iIsAttribute);
}

EXPORT_C void TBBUid::IntoStringL(TDes& aString) const
{
	TBuf<20> b=_L("0x");
	b.AppendNum(iValue, EHex);
	CheckStringSpaceL(aString, b.Length());
	aString.Append(b);
}

EXPORT_C const TTypeName& TBBUid::Type() const
{
	return KUidType;
}

EXPORT_C TBool TBBUid::Equals(const MBBData* aRhs) const
{
	const TBBUid* rhs=bb_cast<TBBUid>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBUid::StaticType()
{
	return KUidType;
}

EXPORT_C const TTypeName& TBBLongString::Type() const
{
	return KLongStringType;
}

EXPORT_C const TTypeName& TBBLongString::StaticType()
{
	return KLongStringType;
}

EXPORT_C TBool TBBLongString::Equals(const MBBData* aRhs) const
{
	const TBBLongString* rhs=bb_cast<TBBLongString>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C CBBString* CBBString::NewL(const TDesC& aName, const TInt aSize)
{
	auto_ptr<CBBString> ret(new (ELeave) CBBString(aName));
	ret->ConstructL(aSize);
	return ret.release();
}

EXPORT_C CBBString::CBBString(const TDesC& aName): 
	TBBFixedLengthStringBase(aName), iPtr(0, 0), iBuf(0) { }

EXPORT_C void CBBString::Zero()
{
	if (!iBuf) ConstructL();
	iBuf->Des().Zero();
	iPtr.Set(iBuf->Des());
}

EXPORT_C void CBBString::Append(const TDesC& aStr)
{
	if (!iBuf) ConstructL();
	while ( iBuf->Des().Length() + aStr.Length() > iBuf->Des().MaxLength() ) {
		iBuf=iBuf->ReAllocL(iBuf->Des().MaxLength() * 2);
	}
	iBuf->Des().Append(aStr);
	iPtr.Set(iBuf->Des());
}

EXPORT_C void CBBString::TakeHBufC(HBufC* aBuf)
{
	if (aBuf==0) {
		Zero();
		return;
	}
	// releases the current iBuf, replaces with aBuf and takes ownership
	delete iBuf; iBuf=0;
	iBuf=aBuf;
	iPtr.Set(iBuf->Des());
}

EXPORT_C const TTypeName& CBBString::Type() const
{
	return KStringType;
}

EXPORT_C const TTypeName& CBBString::StaticType()
{
	return KStringType;
}


EXPORT_C TBool CBBString::Equals(const MBBData* aRhs) const
{
	const CBBString* rhs=bb_cast<CBBString>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C void CBBString::FromStringL(const TDesC& aString)
{
	if (!iBuf) ConstructL();
	iBuf->Des().Zero();
	Append(aString);
}

EXPORT_C void CBBString::InternalizeL(RReadStream& aStream)
{
	if (!iBuf) ConstructL();
	iBuf->Des().Zero();
	TInt aLen=aStream.ReadInt32L();
	if (aLen > iBuf->Des().MaxLength() ) {
		iBuf=iBuf->ReAllocL(aLen);
	}
	iPtr.Set(iBuf->Des());
	aStream.ReadL(iPtr, aLen);
}

EXPORT_C void CBBString::ConstructL(TInt aSize)
{
	iBuf=HBufC::NewL(aSize);
	iPtr.Set(iBuf->Des());
}

EXPORT_C CBBString::~CBBString()
{
	delete iBuf; iBuf=0;
}

EXPORT_C void CBBString::SyncPtr()
{
	iPtr.Set(iBuf->Des());
}

EXPORT_C CBBString8* CBBString8::NewL(const TDesC& aName, const TInt aSize)
{
	auto_ptr<CBBString8> ret(new (ELeave) CBBString8(aName));
	ret->ConstructL(aSize);
	return ret.release();
}

EXPORT_C void CBBString8::Zero()
{
	if (!iBuf) ConstructL();
	iBuf->Des().Zero();
}

EXPORT_C void CBBString8::Append(const TDesC8& aStr)
{
	if (!iBuf) ConstructL();
	while ( iBuf->Des().Length() + aStr.Length() > iBuf->Des().MaxLength() ) {
		iBuf=iBuf->ReAllocL(iBuf->Des().MaxLength() * 2);
	}
	iBuf->Des().Append(aStr);
	iPtr.Set(iBuf->Des());
}

EXPORT_C const TTypeName& CBBString8::Type() const
{
	return KString8Type;
}

EXPORT_C const TTypeName& CBBString8::StaticType()
{
	return KString8Type;
}


EXPORT_C TBool CBBString8::Equals(const MBBData* aRhs) const
{
	const CBBString8* rhs=bb_cast<CBBString8>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C void CBBString8::FromStringL(const TDesC8& aString)
{
	if (!iBuf) ConstructL();
	iBuf->Des().Zero();
	Append(aString);
}

EXPORT_C void CBBString8::FromStringL(const TDesC& aString)
{
	ConstructL(aString.Length()/2);
	TBBFixedLengthStringBase8::FromStringL(aString);
	iPtr.Set(iBuf->Des());
}

EXPORT_C void CBBString8::InternalizeL(RReadStream& aStream)
{
	if (!iBuf) ConstructL();
	iBuf->Des().Zero();
	TInt aLen=aStream.ReadInt32L();
	if (aLen > iBuf->Des().MaxLength() ) {
		iBuf=iBuf->ReAllocL(aLen);
	}
	iPtr.Set(iBuf->Des());
	aStream.ReadL(iPtr, aLen);
}

EXPORT_C void CBBString8::ConstructL(TInt aSize)
{
	if (iBuf) {
		if (iBuf->Des().MaxLength() >= aSize) {
			iBuf->Des().Zero();
			iPtr.Set(iBuf->Des());
			return;
		}
		delete iBuf; iBuf=0;
	}
	iBuf=HBufC8::NewL(aSize);
	iPtr.Set(iBuf->Des());
}

EXPORT_C CBBString8::~CBBString8()
{
	delete iBuf;
}

EXPORT_C MBBData* CBBString8::CloneL(const TDesC& Name) const
{
	CBBString8* ret=CBBString8::NewL(Name, iBuf->Size());
	ret->Value()=Value();
	return ret;
}

//--


EXPORT_C MBBData* TBBInt::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBInt(iValue, Name);
}

EXPORT_C MBBData* TBBShortString::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBShortString(iValue, Name);
}

EXPORT_C MBBData* TBBLongString::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBLongString(iValue, Name);
}

EXPORT_C MBBData* TBBBool::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBBool(iValue, Name);
}

EXPORT_C MBBData* TBBTime::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBTime(iValue, Name);
}

EXPORT_C MBBData* TBBUint::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBUint(iValue, Name);
}

EXPORT_C MBBData* TBBUid::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBUid(iValue, Name);
}

EXPORT_C MBBData* CBBString::CloneL(const TDesC& Name) const
{
	CBBString* ret=CBBString::NewL(Name, iBuf->Length());
	ret->Value()=Value();
	return ret;
}

EXPORT_C void TBBInt64::IntoStringL(TDes& aString) const
{
	TBuf<20> b;
	b.AppendNum(iValue);
	CheckStringSpaceL(aString, b.Length());
	aString.AppendNum(iValue);
}

EXPORT_C void TBBInt64::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	TBuf<30> buf;
	buf.AppendNum(iValue);
	aBuf->Field(Name(), MBBExternalizer::EInteger, buf, aIncludeType, 
		Type(), iIsAttribute);
}

EXPORT_C TBool TBBInt64::Equals(const MBBData* aRhs) const
{
	const TBBInt64* rhs=bb_cast<TBBInt64>(aRhs);
	return (rhs && iValue==rhs->iValue);
}

EXPORT_C void TBBInt64::ExternalizeL(RWriteStream& aStream) const
{
	aStream.WriteInt32L(I64LOW(iValue));
	aStream.WriteInt32L(I64HIGH(iValue));
}

EXPORT_C void TBBInt64::FromStringL(const TDesC& aString)
{
	TLex aLex; TInt err;
	if (aString.Left(2).CompareF(_L("0x"))) {
		aLex.Assign(aString);
		err=aLex.Val(iValue);
	} else {
		aLex.Assign(aString.Mid(2));
		err=aLex.Val(iValue, EHex);
	}
	if (err==KErrNone) {
		if (aLex.Remainder().Length()==0) return;
		else 
			LeaveCannotParseValue();
	}
	if (err==KErrGeneral) 
		LeaveCannotParseValue();
	User::Leave(err);
}

EXPORT_C void TBBInt64::InternalizeL(RReadStream& aStream)
{
	TInt32 low, high;
	low=aStream.ReadInt32L();
	high=aStream.ReadInt32L();
	iValue=MAKE_TINT64(high, low);
}

EXPORT_C const TTypeName& TBBInt64::Type() const
{
	return KInt64Type;
}

EXPORT_C const TTypeName& TBBInt64::StaticType()
{
	return KInt64Type;
}

EXPORT_C MBBData* TBBInt64::CloneL(const TDesC& Name) const
{
	return new (ELeave) TBBInt64(iValue, Name);
}

EXPORT_C const TTypeName& TBBShortString8::Type() const
{
	return KShortString8Type;
}

EXPORT_C const TTypeName& TBBShortString8::StaticType()
{
	return KShortString8Type;
}

EXPORT_C TBool TBBShortString8::Equals(const MBBData* aRhs) const
{
	const TBBShortString8* rhs=bb_cast<TBBShortString8>(aRhs);
	return (rhs && *rhs==*this);
}
EXPORT_C MBBData* TBBShortString8::CloneL(const TDesC& Name) const
{
	TBBShortString8* ret=new (ELeave) TBBShortString8(Name);
	ret->iValue=iValue;
	return ret;
}


EXPORT_C const TTypeName& TBBTupleName::Type() const
{
	return KTupleNameType;
}

EXPORT_C TBool TBBTupleName::Equals(const MBBData* aRhs) const
{
	const TBBTupleName* rhs=bb_cast<TBBTupleName>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBTupleName::StaticType()
{
	return KTupleNameType;
}

EXPORT_C const MBBData* TBBTupleName::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iModuleUid;
	case 1:
		return &iModuleId;
	default:
		return 0;
	}
}

_LIT(KModuleUid, "module_uid");
_LIT(KModuleId, "module_id");
_LIT(KSpace, " ");

EXPORT_C TBBTupleName::TBBTupleName(const TDesC& aName) : TBBCompoundData(aName),
	iModuleUid(KModuleUid), iModuleId(KModuleId) { }

EXPORT_C TBBTupleName::TBBTupleName(const TDesC& aName, TInt aModuleUid, TInt aModuleId) :
	TBBCompoundData(aName), iModuleUid(aModuleUid, KModuleUid), iModuleId(aModuleId, KModuleUid) { }

EXPORT_C bool TBBTupleName::operator==(const TBBTupleName& aRhs) const
{
	return (iModuleUid==aRhs.iModuleUid && iModuleId==aRhs.iModuleId);
}

EXPORT_C const TDesC& TBBTupleName::StringSep(TUint ) const
{
	return KSpace;
}

EXPORT_C TBBTupleName& TBBTupleName::operator=(const TBBTupleName& aNameData)
{
	iModuleUid()=aNameData.iModuleUid();
	iModuleId()=aNameData.iModuleId();

	return *this;
}

EXPORT_C TBBTupleName& TBBTupleName::operator=(const TTupleName& aNameData)
{
	iModuleUid()=aNameData.iModule.iUid;
	iModuleId()=aNameData.iId;

	return *this;
}

EXPORT_C MBBData* TBBTupleName::CloneL(const TDesC& Name) const
{
	TBBTupleName* ret=new (ELeave) TBBTupleName(Name);
	*ret=*this;
	return ret;
}

EXPORT_C const TTypeName& TBBComponentName::Type() const
{
	return KComponentNameType;
}

EXPORT_C TBool TBBComponentName::Equals(const MBBData* aRhs) const
{
	const TBBComponentName* rhs=bb_cast<TBBComponentName>(aRhs);
	return *rhs==*this;
}

EXPORT_C const TTypeName& TBBComponentName::StaticType()
{
	return KComponentNameType;
}

EXPORT_C TBBComponentName::TBBComponentName(const TDesC& aName) : TBBTupleName(aName) { }

EXPORT_C TBBComponentName::TBBComponentName(const TDesC& aName, TInt aModuleUid, 
											TInt aModuleId) :
	TBBTupleName(aName, aModuleUid, aModuleId) { }

EXPORT_C TBBComponentName& TBBComponentName::operator=(const TComponentName& aNameData)
{
	iModuleUid()=aNameData.iModule.iUid;
	iModuleId()=aNameData.iId;

	return *this;	
}

EXPORT_C MBBData* TBBComponentName::CloneL(const TDesC& Name) const
{
	TBBComponentName* ret=new (ELeave) TBBComponentName(Name);
	*ret=*this;
	return ret;
}

EXPORT_C CBBString8::CBBString8(const TDesC& aName) : TBBFixedLengthStringBase8(aName), iPtr(0, 0), iBuf(0) { }

IMPLEMENT_ASSIGN(TBBInt)

EXPORT_C MBBData* TBBFixedLengthStringBase::Assign(const MBBData* aRhs)
{
	Value().Zero();
	aRhs->IntoStringL( Value() );
	return this;
}

IMPLEMENT_ASSIGN(TBBShortString8)
IMPLEMENT_ASSIGN(TBBBool)
IMPLEMENT_ASSIGN(TBBTime)
IMPLEMENT_ASSIGN(TBBUint)
IMPLEMENT_ASSIGN(TBBUid)

EXPORT_C MBBData* TBBCompoundData::Assign(const MBBData* aRhs)
{
	if (!aRhs) return this;
	const TBBCompoundData* rhs=aRhs->IsCompound();
	if (!rhs) return this;
	for (int i=0; true; i++) {
		const MBBData* rhs_part=rhs->Part(i);
		if (! rhs_part ) break;
		if ( rhs->DidReadPart(i) ) {
			MBBData* this_part=(MBBData*)Part(i);
			if (! this_part) {
				TUint dummy;
				this_part=GetPart( rhs_part->Name(), rhs_part->Type(), dummy);
			}
			if (this_part) this_part->Assign(rhs_part);
		}
	}
}

EXPORT_C const TBBCompoundData* TBBCompoundData::IsCompound() const
{
	return this;
}


EXPORT_C MBBData* CBBString::Assign(const MBBData* aRhs)
{
	const CBBString* rhs=bb_cast<CBBString>(aRhs);
	if (!rhs) return this;
	this->Zero();
	this->Append(rhs->Value());
	return this;
}

EXPORT_C MBBData* CBBString8::Assign(const MBBData* aRhs)
{
	const CBBString8* rhs=bb_cast<CBBString8>(aRhs);
	if (!rhs) return this;
	this->Zero();
	this->Append(rhs->Value());
	return this;
}

IMPLEMENT_ASSIGN(TBBInt64)
