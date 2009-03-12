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

#ifndef CC_ERRORINFO_H_INCLUDED
#define CC_ERRORINFO_H_INCLUDED 1

#include <e32std.h>
#include "errorinfo.h"
#include "vararg_if_macros.h"
#include "stringarg.h"

/*
 * the error reporting/raising is meannt to work like:
 *
 *	Bug(_L("record disappeared!")).Raise();
 * or
 *
 *	InputErr(_L("invalid number")).TechMsg(_L("validate returned %1", -2)).Raise();
 *
 * or
 *	EnvErr(_L("Cannot make a GPRS connection while roaming")).ErrorCode(ENoGprsWhileRoaming).Raise();
 *
 * localization: use gettext() for the user messages, no actual implementation yet tho
 *
 */

class MErrorInfoManager {
public:
	virtual void StartErrorWithInfo(TErrorCode aErrorCode,
		const TStringArg& aUserMessage, const TStringArg& aTechMessage,
		TErrorType aErrorType, TSeverity aSeverity) = 0;

	virtual MErrorInfo* GetLastErrorInfo() = 0;
	virtual MErrorInfo* GetLastErrorInfo(TErrorCode aCode) = 0;
	virtual void ResetLastErrorInfo() = 0;

	INTERFACE_VARARG(UserMsg, MErrorInfoManager&, const TStringArg&)
	// implies virtual MErrorInfoManager& UserMsg(const TStringArg& aArg) = 0;
	INTERFACE_VARARG(TechMsg, MErrorInfoManager&, const TStringArg&)
	// implies virtual MErrorInfoManager& TechMsg(const TStringArg& aArg) = 0;
	virtual MErrorInfoManager& ErrorCode(TErrorCode aErrorCode) = 0;
	virtual MErrorInfoManager& Severity(TSeverity aSeverity) = 0;
	virtual MErrorInfoManager& ErrorType(TErrorType aErrorType) = 0;
	virtual void Raise() = 0;
	virtual void Log() = 0;
	virtual MErrorInfo* Get() = 0;
};

IMPORT_C MErrorInfoManager& Bug(const TDesC& aTechMsg);
IMPORT_C MErrorInfoManager& Corrupt(const TDesC& aTechMsg);
IMPORT_C MErrorInfoManager& InputErr(const TDesC& aUserMsg);
IMPORT_C MErrorInfoManager& RemoteErr(const TDesC& aTechMsg);
IMPORT_C MErrorInfoManager& EnvErr(const TDesC& aUserMsg);
IMPORT_C MErrorInfoManager& PlainErr(TInt aError);

#define gettext(a) (TPtrC((const TText *)L ## a, sizeof(L##a)/2-1))

#endif
