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

#include "loca_errorlog.h"
#include "concretedata.h"
#include "cbbsession.h"
#include "break.h"

const TComponentName KListener = { { CONTEXT_UID_CONTEXTNETWORK}, 1 };
const TTupleName KLocaErrorTuple = { { CONTEXT_UID_CONTEXT_LOG}, 1002 };

_LIT(KErr, "err");
class CErrorLoggerImpl : public CErrorLogger {
public:
	CBBString* iString;
	void ConstructL() {
		iString=CBBString::NewL(KErr);
	}
	~CErrorLoggerImpl() {
		delete iString;
	}
	TBuf<12> iNum;
	virtual void ReportError(const TDesC& Source,
		const TDesC& Reason, TInt Code) {
			CC_TRAPD(err, ReportErrorL(Source, Reason, Code));
	}
	void ReportErrorL(const TDesC& Source,
		const TDesC& Reason, TInt Code) {
		iString->Zero();
		iString->Append(Reason);
		iString->Append(_L(" Source: "));
		iString->Append(Source);
		iString->Append(_L(" Code: "));
		iNum.Zero(); iNum.AppendNum(Code);
		iString->Append(iNum);
		TTime exp; exp=GetTime(); exp+=TTimeIntervalHours(12);
		BBSession()->PutRequestL(KLocaErrorTuple, KNullDesC,
			iString, exp, KNoComponent);
	}
	virtual void LogFormatted(const TDesC& aMsg) {
		CC_TRAPD(err, LogFormattedL(aMsg));
	}

	void LogFormattedL(const TDesC& aMsg) {
		if (aMsg.Length()==0) return;
		iString->Zero();
		iString->Append(aMsg);
		TTime exp; exp=GetTime(); exp+=TTimeIntervalHours(12);
		BBSession()->PutRequestL(KLocaErrorTuple, KNullDesC,
			iString, exp, KNoComponent);
	}
	TBool iHandlable;
	virtual void SetInHandlableEvent(TBool aHandlable) {
		iHandlable=aHandlable;
	}
};


CErrorLogger* CErrorLogger::NewL()
{
	auto_ptr<CErrorLoggerImpl> ret(new (ELeave) CErrorLoggerImpl);
	ret->ConstructL();
	return ret.release();
}
