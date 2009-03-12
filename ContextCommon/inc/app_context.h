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

#if !defined(APP_CONTEXT_H_INCLUDED)
#define APP_CONTEXT_H_INCLUDED 1

//#define FUNCTION_LEVEL_STATISTICS 1

#include <e32std.h>
class RFs;
class CCnvCharacterSetConverter;
class RPhone;
class RTelServer;
class RBasicGsmPhone;
class RSystemAgent;
class RWsSession;

#include "checkedactive.h"
#include "errorhandling.h"

#define DESTROY(x) { delete x; x=0; }

IMPORT_C void exceptionhandler(TExcType);

class MSettings;
class CBBSession;
class MBBDataFactory;
class MReporting;
class MCallStack;
class MErrorInfoManager;
class CTelephony;
class MDataCounter;
class MRecovery;
class MJuikLayout;
class MJuikIconManager;
class MTestSupport;

#if !defined(__SDK_VERSION_MMP__) || !defined(__CONTEXT_MMP__)
#error You must include sdk_version.mmp
#endif

class MApp_context_access {
public:
	virtual RFs&	Fs() = 0;
#ifndef __S60V3__
	virtual RSystemAgent&	SysAgent() = 0;
	virtual RTelServer&	TelServer() = 0;
	virtual RBasicGsmPhone&	Phone() = 0;
#endif
#if defined(__S60V3__) || defined(__S60V2FP3__)
	virtual CTelephony&	Telephony() = 0;
#endif
	virtual CCnvCharacterSetConverter* CC() = 0;
	virtual bool NoSpaceLeft() = 0;
	virtual const TDesC& DataDir() = 0;
	virtual const TDesC& AppDir() = 0;
	virtual MSettings& Settings() = 0;
	virtual RWsSession& Ws() = 0;
	virtual MReporting& Reporting() = 0;
	virtual void ReportActiveError(const TDesC& Source,
		const TDesC& Reason, TInt Code) = 0;
	virtual CBBSession* BBSession() = 0;
	virtual MBBDataFactory* BBDataFactory() = 0;
	virtual MErrorInfoManager& ErrorInfoMgr() = 0;
	virtual MCallStack& CallStackMgr() = 0;
	virtual MDataCounter& DataCounter() = 0;
	virtual MRecovery& Recovery() = 0;
	virtual MJuikLayout& Layout() const = 0;
	virtual MJuikIconManager& IconManager() const = 0;
	virtual MTestSupport& TestSupport()  = 0;
};

class MContextBase : public MApp_context_access {
public:
	IMPORT_C MContextBase();
	IMPORT_C MContextBase(MApp_context& Context);
	IMPORT_C virtual ~MContextBase();
	IMPORT_C MApp_context&	AppContext();
	IMPORT_C MApp_context_access&	AppContextAccess();
	IMPORT_C virtual RFs&	Fs();
#ifndef __S60V3__
	IMPORT_C virtual RSystemAgent&	SysAgent();
	IMPORT_C virtual RTelServer&	TelServer();
	IMPORT_C virtual RBasicGsmPhone&	Phone();
#endif
#if defined(__S60V3__) || defined(__S60V2FP3__)
	IMPORT_C virtual CTelephony&	Telephony();
#endif
	IMPORT_C virtual CCnvCharacterSetConverter* CC();
	IMPORT_C virtual bool NoSpaceLeft();
	IMPORT_C virtual const TDesC& DataDir();
	IMPORT_C virtual const TDesC& AppDir();
	IMPORT_C virtual MSettings& Settings(); 
	IMPORT_C virtual RWsSession& Ws();
	IMPORT_C virtual void ReportActiveError(const TDesC& Source,
		const TDesC& Reason, TInt Code);
	IMPORT_C CBBSession* BBSession();
	IMPORT_C virtual MBBDataFactory* BBDataFactory();
	IMPORT_C virtual MReporting& Reporting();
	IMPORT_C MErrorInfoManager& ErrorInfoMgr();
	IMPORT_C MCallStack& CallStackMgr();
	IMPORT_C MDataCounter& DataCounter();
	IMPORT_C MRecovery& Recovery();
	IMPORT_C MJuikLayout& Layout() const;
	IMPORT_C MJuikIconManager& IconManager() const;
	IMPORT_C MTestSupport& TestSupport();
protected:
	EXPORT_C MApp_context*	GetContext() const;
	MApp_context*	iContext;
};

class TCallStackItem {
public:
	IMPORT_C TCallStackItem(const TDesC8& Name, MApp_context* Context);
	IMPORT_C TCallStackItem(const TDesC8& Class, const TDesC8& Func, MApp_context* Context, TBool aDoStatistics);
	IMPORT_C ~TCallStackItem();
private:
	class MCallStack* iCallStack;
#ifdef __LEAVE_EQUALS_THROW__
	TBool		iUnwinding;
#endif
#ifdef FUNCTION_LEVEL_STATISTICS
	TUint		iStartTicks;
	MApp_context*	iAppContext;
	
#endif
#if defined(__WINS__)
	TBool iPopped;
	static void PopCleanupItem(void* aPtr);
#endif

#ifdef FUNCTION_LEVEL_STATISTICS
	TBool iDoStats;
#endif
};

IMPORT_C MApp_context* GetContext();


#if 1 && !defined(NO_CONTEXTCOMMON)
#define _CL(X)			(TPtrC8((const TText8 *)(X), sizeof(X)-1))
#define CALLSTACKITEM(X)	TCallStackItem __item(X, GetContext());
#define CALLSTACKITEMSTATIC(X)	TCallStackItem __item(X, ::GetContext());
#define CALLSTACKITEM2(X, Y)	TCallStackItem __item(X, Y);
#define CALLSTACKITEM_N(CLASS, FUNC)	TCallStackItem __item((CLASS), (FUNC), GetContext(), ETrue);
#define CALLSTACKITEMSTATIC_N(CLASS, FUNC)	TCallStackItem __item((CLASS), (FUNC), ::GetContext(), ETrue);
#define CALLSTACKITEM2_N(CLASS, FUNC, CTX)	TCallStackItem __item((CLASS), (FUNC), (CTX), ETrue);

#define CALLSTACKITEM_N_NOSTATS(CLASS, FUNC)	TCallStackItem __item((CLASS), (FUNC), GetContext(), EFalse);
#else
#define _CL(X)
#define CALLSTACKITEM(X)	
#define CALLSTACKITEMSTATIC(X)	
#define CALLSTACKITEM2(X, Y)	
#define CALLSTACKITEM_N(CLASS, FUNC)
#define CALLSTACKITEMSTATIC_N(CLASS, FUNC)
#define CALLSTACKITEM2_N(CLASS, FUNC, CTX)

#define CALLSTACKITEM_N_NOSTATS(CLASS, FUNC)
#endif

IMPORT_C TTime GetTime();
IMPORT_C TTime ContextToLocal(TTime aTime);

IMPORT_C TBool HasMMC(RFs& aFs);

#endif
