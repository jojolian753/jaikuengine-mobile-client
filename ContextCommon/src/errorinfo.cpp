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

#include "errorhandling.h"

#include "symbian_auto_ptr.h"

_LIT(KUnsetNoMemory, "<failed to allocate memory for string>");

class CErrorInfoImpl : public CErrorInfo {
private:
	CErrorInfoImpl(TErrorCode aErrorCode, TErrorType aErrorType, TSeverity aSeverity,
		HBufC* aUserMsg, HBufC* aTechMsg, HBufC* aCallStack,
		MErrorInfo* aInnerInfo);
		
	virtual ~CErrorInfoImpl();

	virtual TErrorCode ErrorCode() const;
	virtual const TDesC& StackTrace() const;
	virtual const TDesC& UserMessage() const;
	virtual const TDesC& TechMessage() const;
	virtual TErrorType ErrorType() const;
	virtual TSeverity Severity() const;
	virtual const MErrorInfo* InnerError() const;
	virtual const MErrorInfo* NextError() const;

	virtual MErrorInfo* CreateCopyL() const;

	void AddRef() const;
	void Release() const;

	friend class CErrorInfo;
	friend class auto_ptr<CErrorInfoImpl>;

	TErrorCode	iErrorCode;
	HBufC		*iUserMessage, *iTechMessage, *iCallStack;
	MErrorInfo	*iInnerError, *iNextError;
	TErrorType	iErrorType;
	TSeverity	iSeverity;
	mutable TUint	iRefCount;
};


EXPORT_C CErrorInfo* CErrorInfo::NewL(TErrorCode aErrorCode,
	HBufC* aUserMsg, HBufC* aTechMessage, HBufC* aCallStack,
	TErrorType aErrorType, TSeverity aSeverity,
	MErrorInfo* aInnerError)
{
	auto_ptr<CErrorInfoImpl> ret(new (ELeave) CErrorInfoImpl(aErrorCode,
		aErrorType, aSeverity, aUserMsg, aTechMessage, aCallStack, aInnerError) );
	ret->AddRef();
	return ret.release();
}

MErrorInfo* CErrorInfoImpl::CreateCopyL() const
{
	auto_ptr<HBufC> user(iUserMessage ? iUserMessage->Des().AllocL() : 0);
	auto_ptr<HBufC> tech(iTechMessage ? iTechMessage->Des().AllocL() : 0);
	auto_ptr<HBufC> stack(iCallStack ? iCallStack->Des().AllocL() : 0);
	refcounted_ptr<MErrorInfo> inner( iInnerError ? iInnerError->CreateCopyL() : 0);
	refcounted_ptr<MErrorInfo> next( iNextError ? iNextError->CreateCopyL() : 0);

	MErrorInfo* ret=CErrorInfo::NewL(iErrorCode, user.get(),
		tech.get(), stack.get(), 
		iErrorType, iSeverity, inner.get());

	user.release();
	tech.release();
	stack.release();
	inner.release();
	next.release();

	return ret;
}


CErrorInfoImpl::CErrorInfoImpl(TErrorCode aErrorCode, TErrorType aErrorType, TSeverity aSeverity,
		HBufC* aUserMsg, HBufC* aTechMsg, HBufC* aCallStack, MErrorInfo* aInnerInfo) :
	iErrorCode(aErrorCode), 
	iUserMessage(aUserMsg), iTechMessage(aTechMsg), iCallStack(aCallStack),
	iInnerError(aInnerInfo), iErrorType(aErrorType),iSeverity(aSeverity)
{
	if (iInnerError) iInnerError->AddRef();
}

void CErrorInfoImpl::AddRef() const
{
	++iRefCount;
}
void CErrorInfoImpl::Release() const
{
	--iRefCount;
	if (iRefCount<=0) delete this;
}

CErrorInfoImpl::~CErrorInfoImpl()
{
	delete iUserMessage;
	delete iTechMessage;
	delete iCallStack;
	if (iInnerError) iInnerError->Release();
	if (iNextError) iNextError->Release();
}

TErrorCode CErrorInfoImpl::ErrorCode() const
{
	return iErrorCode;
}

const TDesC& CErrorInfoImpl::UserMessage() const
{
	if (iUserMessage) return *iUserMessage;
	return KNullDesC;
}

const TDesC& CErrorInfoImpl::TechMessage() const
{
	if (iTechMessage) return *iTechMessage;
	return KNullDesC;
}

TErrorType CErrorInfoImpl::ErrorType() const
{
	return iErrorType;
}

TSeverity CErrorInfoImpl::Severity() const
{
	return iSeverity;
}

const MErrorInfo* CErrorInfoImpl::InnerError() const
{
	return iInnerError;
}

const MErrorInfo* CErrorInfoImpl::NextError() const
{
	return iNextError;
}

EXPORT_C TErrorCode MakeErrorCode(TUint iUid, TInt iCode)
{
	TErrorCode e={iUid, iCode};
	return e;
}

const TDesC& CErrorInfoImpl::StackTrace() const
{
	if (iCallStack) return *iCallStack;
	return KNullDesC;
}
