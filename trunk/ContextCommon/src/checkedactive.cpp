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

#include "break.h"
#include "checkedactive.h"
#include "app_context.h"
#include "app_context_impl.h"
#include "reporting.h"
#include "callstack.h"

EXPORT_C CCheckedActive::~CCheckedActive()
{
	CALLSTACKITEM_N(_CL("CCheckedActive"), _CL("~CCheckedActive"));
#ifdef __WINS__
	User::Check();
#endif
	if (iContext) {
		TBuf<120> stats=_L("STATS for ");
		stats.Append(iName);
		if (iRunLCount>0) {
			stats.Append(_L(" n: ")); stats.AppendNum(iRunLCount);
			stats.Append(_L(" min: ")); stats.AppendNum(iMinTicks);
			stats.Append(_L(" max: ")); stats.AppendNum(iMaxTicks);
			stats.Append(_L(" avg: ")); 
			stats.AppendNum(iTotalTicks/iRunLCount);
			stats.Append(_L(" var: ")); stats.AppendNum( 
				(iTotalSquaredTicks-(iTotalTicks*iTotalTicks)/iRunLCount)/iRunLCount
			);
		} else {
			stats.Append(_L(": NEVER RUN"));
		}
		iContext->Reporting().DebugLog(stats);
	}
	delete iLatestErrorStack;
	if (iLive) *iLive=0;
	iLive=0;
}

EXPORT_C CCheckedActive::CCheckedActive(TInt aPriority, const TDesC& Name) : CActive(aPriority)
{
	iContext=MApp_context::Static();
	/*
	if (iContext) {
		TBuf<65> creation;
		creation=_L("created ");
		creation.Append(Name.Left(40));
		creation.Append(_L(" at "));
		creation.AppendNum( (TUint)( (CActive*)this ), EHex);
		iContext->DebugLog(creation);
	}
	*/
	iName=Name.Left(40);
}

EXPORT_C TInt CCheckedActive::RunError(TInt aError)
{
	TInt ret=CheckedRunError(aError);
	
	if (ret!=KErrNone) {
#ifdef __WINS__
		BreakInClassModule();
#endif
		MApp_context* c=iContext;
		if (c) {
			HBufC* stack=0;
			stack=c->CallStackMgr().GetFormattedCallStack(iName);
			if (stack) {
				CleanupStack::PushL(stack);
				c->ReportActiveError(*stack,
					_L("Active Object Run Error"), ret);
				// don't reset stack here so that the stack
				// can be seen from where the error gets
				// propagated to
				// c->ResetCallStack();
				CleanupStack::PopAndDestroy(); // stack
			} else {
				c->ReportActiveError(iName,
					_L("Active Object Run Error"), ret);
			}
		}
	}
	return ret;
}

EXPORT_C void CCheckedActive::SetActive()
{
#ifdef __WINS__
	// this code is there so that we can set a
	// breakpoint if suspecting a misuse
	// of iStatus and SetActive()
	if (iStatus!=KRequestPending) {
		TInt x;
		x=1;
	}
#endif
	CActive::SetActive();
}

EXPORT_C TInt CCheckedActive::CheckedRunError(TInt aError)
{
	return aError;
}

EXPORT_C void CCheckedActive::RunL()
{
#if 0
	TRAPD(err, CheckedRunL());
	User::LeaveIfError(err);
#else
#ifdef __WINS__1
	RDebug::Print(_L("CCheckedActive::RunL"));
	RDebug::Print(iName);
#endif
#ifdef __WINS__
	User::Check();
#endif
	MApp_context* c=iContext;
	if (c) {
		c->CallStackMgr().ResetCallStack();
		c->ErrorInfoMgr().ResetLastErrorInfo();
	}

	TInt live=1;
	iLive=&live;
	TUint start, stop;
	start=User::TickCount();

#ifdef __WINS__
	if (iStatus.Int() < 0) BreakInActiveObject();
#endif

	CC_TRAPD(err, CheckedRunL());
	
	if (live && c) {
		stop=User::TickCount();
		TUint elapsed=stop-start;
		if (elapsed > iMaxTicks || iRunLCount==0) iMaxTicks=elapsed;
		if (elapsed < iMinTicks || iRunLCount==0) iMinTicks=elapsed;
		++iRunLCount;
		iTotalTicks+=elapsed;
		iTotalSquaredTicks+=elapsed*elapsed;
		iLive=0;
	}

	
	if (err!=KErrNone) {
		delete iLatestErrorStack; iLatestErrorStack=0;
		iLatestErrorStack=c->CallStackMgr().GetFormattedCallStack(iName);
		User::Leave(err);
	}
#ifdef __WINS__
	User::Check();
#endif
#endif
}

EXPORT_C const TDesC& CCheckedActive::Name()
{
	return iName;
}
