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

#ifndef CC_ERRORHANDLING_H_INCLUDED
#define CC_ERRORHANDLING_H_INCLUDED 1

#include <e32std.h>
#include <e32base.h>
#include "symbian_refcounted_ptr.h"
#include "context_uids.h"

/*
 * errorinfo objects are supposed to be used
 * as immutables, since they cost a lot. They are
 * constructed by framework code and should not be
 * modified by others.
 */

enum TSeverity {
	EInfo,
	EWarning,
	EError,
	ECorrupt
};

enum TErrorType {
	EBug,
	EInputData,
	ETemporary,
	ELocalEnvironment,
	ERemote
};

struct TErrorCode {
	TUint	iUid;
	TInt	iCode;
	bool operator==(const TErrorCode& aRhs) const {
		return (iUid==aRhs.iUid && iCode==aRhs.iCode);
	}
};

IMPORT_C TErrorCode MakeErrorCode(TUint iUid, TInt iCode);

const TErrorCode KErrorBug = { CONTEXT_UID_CONTEXTCOMMON, -1 };
const TErrorCode KErrorUnknown = { CONTEXT_UID_CONTEXTCOMMON, -2 };

class MErrorInfo : public MRefCounted {
public:
	virtual TErrorCode ErrorCode() const = 0;
	virtual const TDesC& StackTrace() const = 0;
	virtual const TDesC& UserMessage() const = 0;
	virtual const TDesC& TechMessage() const = 0;
	virtual TErrorType ErrorType() const = 0;
	virtual TSeverity Severity() const = 0;
	virtual const MErrorInfo* InnerError() const = 0;
	virtual const MErrorInfo* NextError() const = 0;

	// not CloneL so that doesn't clash with BB types
	virtual MErrorInfo* CreateCopyL() const = 0;
	virtual ~MErrorInfo() { }
};

class CErrorInfo : public CBase, public MErrorInfo {
public:
	IMPORT_C static CErrorInfo* NewL(TErrorCode aErrorCode,
		HBufC* aUserMsg, HBufC* aTechMessage, HBufC* aCallStack,
		TErrorType aErrorType, TSeverity aSeverity,
		MErrorInfo* aInnerError); // on success, takes ownership of HBufC's
};

#define TRAP_ERRORINFO(ERR, INFO, BLOCK) TRAP(ERR.iCode, BLOCK); INFO=GetContext()->ErrorInfoMgr().GetLastErrorInfo(ERR);
#define TRAPD_ERRORINFO(ERR, INFO, BLOCK) TErrorCode ERR; MErrorInfo* INFO; TRAP(ERR.iCode, BLOCK); INFO=GetContext()->ErrorInfoMgr().GetLastErrorInfo(ERR);
#endif
