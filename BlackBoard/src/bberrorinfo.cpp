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

#include "bberrorinfo.h"
#include "bbtypes.h"
#include "errorinfo.h"

EXPORT_C const TTypeName& TBBErrorCode::Type() const
{
	return KErrorCodeType;
}

EXPORT_C TBool TBBErrorCode::Equals(const MBBData* aRhs) const
{
	const TBBErrorCode* rhs=bb_cast<TBBErrorCode>(aRhs);
	return (rhs && *rhs==*this);
}

EXPORT_C TBBErrorCode::operator TErrorCode() const
{
	return MakeErrorCode(iUid(), iCode());
}

EXPORT_C const TTypeName& TBBErrorCode::StaticType()
{
	return KErrorCodeType;
}

EXPORT_C const MBBData* TBBErrorCode::Part(TUint aPartNo) const
{
	switch (aPartNo) {
	case 0:
		return &iUid;
	case 1:
		return &iCode;
	default:
		return 0;
	}
}

EXPORT_C TBBErrorCode& TBBErrorCode::operator=(const TBBErrorCode& aErrorCode)
{
	iUid()=aErrorCode.iUid();
	iCode()=aErrorCode.iCode();
	return *this;
}

EXPORT_C MBBData* TBBErrorCode::CloneL(const TDesC&) const
{
	TBBErrorCode* ret=new (ELeave) TBBErrorCode();
	*ret=*this;
	return ret;
}

EXPORT_C TBBErrorCode::TBBErrorCode() : TBBCompoundData(KErrorCode), iUid(KEUid), iCode(KCode)
{
}

EXPORT_C TBBErrorCode::TBBErrorCode(const TBBErrorCode& aErrorCode) : TBBCompoundData(KErrorCode),
	iUid(aErrorCode.iUid(), KEUid), iCode(aErrorCode.iCode(), KCode) { }

EXPORT_C TBBErrorCode::TBBErrorCode(TUid aUid, TInt aCode) : TBBCompoundData(KErrorCode),
	iUid( (TUint)aUid.iUid, KEUid), iCode(aCode, KCode) { }

EXPORT_C TBBErrorCode::TBBErrorCode(TErrorCode aErrorCode) : TBBCompoundData(KErrorCode),
	iUid(aErrorCode.iUid, KEUid), iCode(aErrorCode.iCode, KCode) { }


_LIT(KSpace, " ");

EXPORT_C const TDesC& TBBErrorCode::StringSep(TUint ) const
{
	return KSpace;
}

EXPORT_C bool TBBErrorCode::operator==(const TBBErrorCode& aRhs) const
{
	return (iUid()==aRhs.iUid() && iCode()==aRhs.iCode());
}

EXPORT_C const TTypeName& TBBSeverity::Type() const
{
	return KSeverityType;
}

EXPORT_C TBBSeverity::operator TSeverity() const
{
	return (TSeverity)iValue;
}

EXPORT_C const TTypeName& TBBSeverity::StaticType()
{
	return KSeverityType;
}

EXPORT_C MBBData* TBBSeverity::CloneL(const TDesC&) const
{
	TBBSeverity* ret=new (ELeave) TBBSeverity();
	ret->iValue=iValue;
	return ret;
}

EXPORT_C TBBSeverity::TBBSeverity() : TBBUint(KSeverity) { }

EXPORT_C TBBSeverity::TBBSeverity(TUint aSeverity) : TBBUint(aSeverity, KSeverity) { }
EXPORT_C TBBSeverity::TBBSeverity(TSeverity aSeverity) : TBBUint(aSeverity, KSeverity) { }

void TBBSeverity::IntoStringL(TDes& aString) const
{
	switch(iValue) {
	case EInfo:
		AppendCheckingSpaceL(aString, KInfo());
		break;
	case EWarning:
		AppendCheckingSpaceL(aString, KWarning());
		break;
	case EError:
		AppendCheckingSpaceL(aString, KError());
		break;
	case ECorrupt:
		AppendCheckingSpaceL(aString, KCorrupt());
		break;
	default:
		TBBUint::IntoStringL(aString);
	}
}

void TBBSeverity::FromStringL(const TDesC& aString)
{
	if (aString.CompareF(KInfo)==0) iValue=EInfo;
	else if (aString.CompareF(KWarning)==0) iValue=EWarning;
	else if (aString.CompareF(KError)==0) iValue=EError;
	else if (aString.CompareF(KCorrupt)==0) iValue=ECorrupt;
	else TBBUint::FromStringL(aString);
}

EXPORT_C const TTypeName& TBBErrorType::Type() const
{
	return KErrorKindType;
}

EXPORT_C MBBData* TBBErrorType::CloneL(const TDesC&) const
{
	TBBSeverity* ret=new (ELeave) TBBSeverity();
	ret->iValue=iValue;
	return ret;
}

EXPORT_C TBBErrorType::TBBErrorType() : TBBUint(KErrorType) { }
EXPORT_C TBBErrorType::TBBErrorType(TUint aSeverity) : TBBUint(aSeverity, KErrorType) { }
EXPORT_C TBBErrorType::TBBErrorType(TErrorType aSeverity) : TBBUint(aSeverity, KErrorType) { }

void TBBErrorType::IntoStringL(TDes& aString) const
{
	switch(iValue) {
	case EBug:
		AppendCheckingSpaceL(aString, KBug);
		break;
	case EInputData:
		AppendCheckingSpaceL(aString, KInput);
		break;
	case ETemporary:
		AppendCheckingSpaceL(aString, KTemporary);
		break;
	case ELocalEnvironment:
		AppendCheckingSpaceL(aString, KLocalEnvironment);
		break;
	case ERemote:
		AppendCheckingSpaceL(aString, KRemote);
		break;
	default:
		TBBUint::IntoStringL(aString);
	}
}

void TBBErrorType::FromStringL(const TDesC& aString)
{
	if (aString.CompareF(KBug)==0) iValue=EBug;
	else if (aString.CompareF(KInput)==0) iValue=EInputData;
	else if (aString.CompareF(KTemporary)==0) iValue=ETemporary;
	else if (aString.CompareF(KLocalEnvironment)==0) iValue=ELocalEnvironment;
	else if (aString.CompareF(KRemote)==0) iValue=ERemote;
	else TBBUint::FromStringL(aString);
}

EXPORT_C TBBErrorType::operator TErrorType() const
{
	return (TErrorType)iValue;
}

EXPORT_C MBBData* CBBErrorInfo::CloneL(const TDesC& ) const
{
	return CBBErrorInfo::NewL(iFactory, this);
}

EXPORT_C MErrorInfo* CBBErrorInfo::CreateCopyL() const 
{
	CBBErrorInfo* i=(CBBErrorInfo*)CloneL(KNullDesC);
	return i;
}

EXPORT_C const TTypeName& CBBErrorInfo::Type() const
{
	return KErrorInfoType;
}

EXPORT_C const TTypeName& CBBErrorInfo::StaticType()
{
	return KErrorInfoType;
}

EXPORT_C TBool CBBErrorInfo::Equals(const MBBData* aRhs) const
{
	const CBBErrorInfo* rhs=bb_cast<CBBErrorInfo>(aRhs);
	return (rhs &&
		rhs->iSeverity() == iSeverity() &&
		rhs->iErrorType() == iErrorType() &&
		rhs->iErrorCode==iErrorCode &&
		rhs->iUserMsg->Equals(iUserMsg) &&
		rhs->iTechnicalMsg->Equals(iTechnicalMsg) &&
		rhs->iStackTrace->Equals(iStackTrace) &&
		rhs->iInnerErrorInfo.Equals(&iInnerErrorInfo) &&
		rhs->iNextErrorInfo.Equals(&iNextErrorInfo)
		);
}

EXPORT_C CBBErrorInfo* CBBErrorInfo::NewL(MBBDataFactory* aFactory, TSeverity aSeverity, TErrorType aErrorType,
	TErrorCode aErrorCode)
{
	auto_ptr<CBBErrorInfo> ret(new (ELeave) CBBErrorInfo(aFactory, aSeverity,
		aErrorType, aErrorCode));
	ret->ConstructL();
	ret->AddRef();

	return ret.release();
}

EXPORT_C void CBBErrorInfo::SetInnerError(CBBErrorInfo* aInfo)
{
	if (iInnerErrorInfo()) bb_cast<CBBErrorInfo>(iInnerErrorInfo())->Release();
	iInnerErrorInfo.SetValue(aInfo);
	if (aInfo) aInfo->AddRef();
}

EXPORT_C void CBBErrorInfo::SetInnerError(const MErrorInfo* aInfo)
{
	if (aInfo) {
		auto_ptr<CBBErrorInfo> i(CBBErrorInfo::NewL(iFactory, aInfo));
		if (iInnerErrorInfo()) bb_cast<CBBErrorInfo>(iInnerErrorInfo())->Release();
		iInnerErrorInfo.SetValue(i.release());
	} else {
		if (iInnerErrorInfo()) bb_cast<CBBErrorInfo>(iInnerErrorInfo())->Release();
		iInnerErrorInfo.SetValue(0);
	}
}

EXPORT_C void CBBErrorInfo::SetNextError(CBBErrorInfo* aInfo)
{
	if (iNextErrorInfo()) bb_cast<CBBErrorInfo>(iNextErrorInfo())->Release();
	iNextErrorInfo.SetValue(aInfo);
	if (aInfo) aInfo->AddRef();
}

EXPORT_C void CBBErrorInfo::SetNextError(const MErrorInfo* aInfo)
{
	if (aInfo) {
		auto_ptr<CBBErrorInfo> i(CBBErrorInfo::NewL(iFactory, aInfo));
		if (iNextErrorInfo()) bb_cast<CBBErrorInfo>(iNextErrorInfo())->Release();
		iNextErrorInfo.SetValue(i.release());
	} else {
		if (iNextErrorInfo()) bb_cast<CBBErrorInfo>(iNextErrorInfo())->Release();
		iNextErrorInfo.SetValue(0);
	}
}

EXPORT_C const MErrorInfo* CBBErrorInfo::InnerErrorInfo()
{
	return bb_cast<CBBErrorInfo>(iInnerErrorInfo());
}

EXPORT_C const MErrorInfo* CBBErrorInfo::NextErrorInfo()
{
	return bb_cast<CBBErrorInfo>(iNextErrorInfo());
}

EXPORT_C CBBErrorInfo* CBBErrorInfo::NewL(MBBDataFactory* aFactory, const MErrorInfo* aInfo)
{
	auto_ptr<CBBErrorInfo> ret(new (ELeave) CBBErrorInfo(aFactory, aInfo->Severity(),
		aInfo->ErrorType(), aInfo->ErrorCode()));
	ret->ConstructL();
	ret->AddRef();

	ret->iUserMsg->Append(aInfo->UserMessage());
	ret->iTechnicalMsg->Append(aInfo->TechMessage());
	ret->iStackTrace->Append(aInfo->StackTrace());

	if ( aInfo->InnerError() ) ret->iInnerErrorInfo.SetValue(CBBErrorInfo::NewL(aFactory, aInfo->InnerError()));
	if ( aInfo->NextError() ) ret->iNextErrorInfo.SetValue(CBBErrorInfo::NewL(aFactory, aInfo->NextError()));

	return ret.release();
}

EXPORT_C CBBErrorInfo* CBBErrorInfo::NewL(MBBDataFactory* aFactory)
{
	auto_ptr<CBBErrorInfo> ret(new (ELeave) CBBErrorInfo(aFactory, EError,
		EBug, MakeErrorCode(0, KErrGeneral) ));
	ret->ConstructL();
	ret->AddRef();

	return ret.release();
}

_LIT(KInner, "inner_error");
_LIT(KNext, "next_error");

CBBErrorInfo::CBBErrorInfo(MBBDataFactory* aFactory, TSeverity aSeverity, TErrorType aErrorType,
	     TErrorCode aErrorCode) : TBBCompoundData(KErrorInfo),
	     iSeverity(aSeverity), iErrorType(aErrorType),
	     iErrorCode(aErrorCode), iInnerErrorInfo(KInner, aFactory),
		 iNextErrorInfo(KNext, aFactory),
		iFactory(aFactory)
{
	iInnerErrorInfo.SetOwnsValue(EFalse);
	iNextErrorInfo.SetOwnsValue(EFalse);
}

CBBErrorInfo::~CBBErrorInfo()
{
	delete iUserMsg;
	delete iTechnicalMsg;
	delete iStackTrace;

	if (iInnerErrorInfo()) bb_cast<CBBErrorInfo>(iInnerErrorInfo())->Release();
	if (iNextErrorInfo()) bb_cast<CBBErrorInfo>(iNextErrorInfo())->Release();
}

void CBBErrorInfo::ConstructL()
{
	iUserMsg=CBBString::NewL(KUserMsg);
	iTechnicalMsg=CBBString::NewL(KTechnicalMsg);
	iStackTrace=CBBString::NewL(KStackTrace);
	iInnerErrorInfo.SetOwnsValue(EFalse);
	iNextErrorInfo.SetOwnsValue(EFalse);
}

TErrorCode CBBErrorInfo::ErrorCode() const
{
	return iErrorCode;
}

const TDesC& CBBErrorInfo::StackTrace() const
{
	return (*iStackTrace)();
}

const TDesC& CBBErrorInfo::UserMessage() const
{
	return (*iUserMsg)();
}

const TDesC& CBBErrorInfo::TechMessage() const
{
	return (*iTechnicalMsg)();
}

TErrorType CBBErrorInfo::ErrorType() const
{
	return iErrorType;
}

TSeverity CBBErrorInfo::Severity() const
{
	return iSeverity;
}

const MErrorInfo* CBBErrorInfo::InnerError() const
{
	return bb_cast<CBBErrorInfo>(iInnerErrorInfo());
}

const MErrorInfo* CBBErrorInfo::NextError() const
{
	return bb_cast<CBBErrorInfo>(iNextErrorInfo());
}

EXPORT_C void CBBErrorInfo::AddRef() const
{
	++iRefCount;
}

EXPORT_C void CBBErrorInfo::Release() const
{
	--iRefCount;
	if (iRefCount<=0) delete this;
}

EXPORT_C const MBBData* CBBErrorInfo::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iSeverity;
	case 1:
		return &iErrorType;
	case 2:
		return &iErrorCode;
	case 3:
		return iUserMsg;
	case 4:
		return iTechnicalMsg;
	case 5:
		return iStackTrace;
	case 6:
		return &iInnerErrorInfo;
	case 7:
		return &iNextErrorInfo;
	default:
		return 0;
	}
}

_LIT(KSemiColon, "; ");

const TDesC& CBBErrorInfo::StringSep(TUint ) const
{
	return KSemiColon;
}

EXPORT_C struct TErrorCode BBErrorCode(TInt aError) {
	return MakeErrorCode(CONTEXT_UID_BLACKBOARDDATA, aError);
}

EXPORT_C void CBBErrorInfo::Zero() {
	iSeverity()=EError;
	iErrorType()=EBug;
	iErrorCode=MakeErrorCode(0, KErrGeneral);
	iUserMsg->Zero();
	iTechnicalMsg->Zero();
	iStackTrace->Zero();
	if (iInnerErrorInfo()) bb_cast<CBBErrorInfo>(iInnerErrorInfo())->Release();
	iInnerErrorInfo.SetValue(0);
	if (iNextErrorInfo()) bb_cast<CBBErrorInfo>(iNextErrorInfo())->Release();
	iNextErrorInfo.SetValue(0);
}
