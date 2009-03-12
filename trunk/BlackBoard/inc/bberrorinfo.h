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

#ifndef CONTEXT_BBERRORINFO_H_INCLUDED
#define CONTEXT_BBERRORINFO_H_INCLUDED 1

#include "concretedata.h"
#include "errorhandling.h"
#include "symbian_refcounted_ptr.h"

_LIT(KErrorCode, "error_code");
_LIT(KEUid, "uid");
_LIT(KCode, "code");

class TBBErrorCode : public TBBCompoundData  {
public:
	TBBUid		iUid;
	TBBInt		iCode;

	IMPORT_C operator TErrorCode() const;
        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;

	IMPORT_C TBBErrorCode& operator=(const TBBErrorCode& aErrorCode);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
public:
	IMPORT_C TBBErrorCode();
	IMPORT_C TBBErrorCode(const TBBErrorCode& aErrorCode);
	IMPORT_C TBBErrorCode(TUid aUid, TInt aCode);
	IMPORT_C TBBErrorCode(TErrorCode aErrorCode);
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C bool operator==(const TBBErrorCode& aRhs) const;
};

_LIT(KSeverity, "severity");
_LIT(KInfo, "INFO");
_LIT(KWarning, "WARNING");
_LIT(KError, "ERROR");
_LIT(KCorrupt, "CORRUPT");

class TBBSeverity : public TBBUint {
public:
        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C TBBSeverity();
	IMPORT_C TBBSeverity(TUint aSeverity);
	IMPORT_C TBBSeverity(TSeverity aSeverity);

	IMPORT_C operator TSeverity() const;
        virtual void IntoStringL(TDes& aString) const;
        virtual void FromStringL(const TDesC& aString);
};

_LIT(KErrorType, "error_type");

_LIT(KBug, "BUG");
_LIT(KInput, "INPUTDATA");
_LIT(KTemporary, "TEMPORARY");
_LIT(KLocalEnvironment, "LOCALENVIRONMENT");
_LIT(KRemote, "REMOTE");

class TBBErrorType : public TBBUint {
public:
        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C TBBErrorType();
	IMPORT_C TBBErrorType(TUint aErrorType);
	IMPORT_C TBBErrorType(TErrorType aErrorType);
	IMPORT_C operator TErrorType() const;

        virtual void IntoStringL(TDes& aString) const;
        virtual void FromStringL(const TDesC& aString);
};

_LIT(KErrorInfo, "error_info");
_LIT(KUserMsg, "user_msg");
_LIT(KTechnicalMsg, "tech_msg");
_LIT(KStackTrace, "stack_trace");

class CBBErrorInfo : public CBase, public TBBCompoundData, public MErrorInfo {
public:
	TBBSeverity	iSeverity;
	TBBErrorType	iErrorType;
	TBBErrorCode	iErrorCode;

	CBBString*	iUserMsg;
	CBBString*	iTechnicalMsg;
	CBBString*	iStackTrace;

        IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C	MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C MErrorInfo* CreateCopyL() const;
	IMPORT_C static CBBErrorInfo* NewL(MBBDataFactory* aFactory, TSeverity aSeverity, TErrorType aErrorType,
		TErrorCode aErrorCode);
	IMPORT_C static CBBErrorInfo* NewL(MBBDataFactory* aFactory, 
		const MErrorInfo* aInfo);
	IMPORT_C static CBBErrorInfo* NewL(MBBDataFactory* aFactory);
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;
	IMPORT_C const MBBData* Part(TUint aPartNo) const;

	IMPORT_C virtual void AddRef() const;
	IMPORT_C virtual void Release() const;
	IMPORT_C void SetInnerError(CBBErrorInfo* aInfo);
	IMPORT_C void SetInnerError(const MErrorInfo* aInfo);
	IMPORT_C void SetNextError(CBBErrorInfo* aInfo);
	IMPORT_C void SetNextError(const MErrorInfo* aInfo);
	IMPORT_C const MErrorInfo* InnerErrorInfo();
	IMPORT_C const MErrorInfo* NextErrorInfo();

	IMPORT_C void Zero();
private:
	CBBGeneralHolder iInnerErrorInfo;
	CBBGeneralHolder iNextErrorInfo;

	~CBBErrorInfo();
	CBBErrorInfo(MBBDataFactory* aFactory,
		TSeverity aSeverity, TErrorType aErrorType,
		TErrorCode aErrorCode);
	void ConstructL();

public:
	virtual TErrorCode ErrorCode() const;
	virtual const TDesC& StackTrace() const;
	virtual const TDesC& UserMessage() const;
	virtual const TDesC& TechMessage() const;
	virtual TErrorType ErrorType() const;
	virtual TSeverity Severity() const;
	virtual const MErrorInfo* InnerError() const;
	virtual const MErrorInfo* NextError() const;

private:
	virtual const TDesC& StringSep(TUint aBeforePart) const;

	MBBDataFactory* iFactory;

	mutable TUint	iRefCount;
	friend class auto_ptr<CBBErrorInfo>;
};

#endif
