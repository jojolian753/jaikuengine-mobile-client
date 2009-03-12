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

#if !defined(APP_CONTEXT_IMPL_H_INCLUDED)
#define APP_CONTEXT_IMPL_H_INCLUDED 1

#include "app_context.h"
#include <e32std.h>
class MPausable;
class MActiveErrorReporter;



class MApp_context: public MApp_context_access {
public:
	virtual ~MApp_context();
	virtual void SetFileLog(MPausable* log) = 0;
	virtual void SetDataDir(const TDesC& Dir, bool UseMMC) = 0;

	virtual void SetLayout(MJuikLayout* aLayout) = 0;
	virtual void SetIconManager(MJuikIconManager* aIconManager) = 0;

	IMPORT_C static MApp_context* Static();

	virtual void SetDebugLog(const TDesC& aDir, const TDesC& aFile) = 0;
	virtual MActiveErrorReporter* GetActiveErrorReporter() = 0;
	virtual void TakeOwnershipL(CBase* aObject) = 0; 
		// will be cleaned up when the Context is deleted
		// leaves only after taking ownership (like the cleanupstack)

	virtual void RunOnShutdown( void(*function)(void) ) = 0;
	virtual void SetCurrentComponent(TUid aComponentUid, TInt aComponentId) = 0; // there can only be one
	virtual void GetCurrentComponent(TUid& aComponentUid, TInt& aComponentId) = 0;
	virtual void GetInitialComponent(TUid& aComponentUid, TInt& aComponentId) = 0;
private:
	int		refcount;
#ifdef __WINS__
public:
	class TBreakItem* iCurrentBreakItem;
#endif
};

class MDiskSpace {
public:
	virtual void DiskSpaceThreshold(TInt aDrive) = 0;
};

class CDiskSpaceNotifier;

class CApp_context : public CBase, public MApp_context {
public:
	IMPORT_C static CApp_context* NewL(bool aFsOnly=false, const TDesC& Name=KNullDesC);
	IMPORT_C static CApp_context* Static();
	virtual ~CApp_context();
	virtual void SetActiveErrorReporter(MActiveErrorReporter* Reporter) = 0;
	virtual void SetSettings(MSettings* Settings) = 0;  // takes ownership
	virtual void SetDataDir(const TDesC& Dir, bool UseMMC) = 0;
	virtual void SetAppDir(const TDesC& Dir) = 0;
	virtual void SetBBSession(CBBSession* BBSession) = 0;
	virtual void SetBBDataFactory(MBBDataFactory* aFactory) = 0;
	virtual void SetDataCounter(MDataCounter* DataCounter) = 0;
	virtual void SetDebugCallstack(TBool aDoDebug) = 0;
	virtual const TDesC& Name() = 0;
#ifdef __WINS__
	void *iHookData;
	virtual const TDesC8& FullCallStackBuffer() const = 0;
#endif
	virtual void SetRecovery(MRecovery* aRecovery) = 0;
};


#ifdef __WINS__
IMPORT_C void StartStarterL(const TDesC& StackName, TUid AppUid, bool CheckForRunning, RWsSession& Ws);
#else
IMPORT_C void StartStarterL(const TDesC& StackName, TUid AppUid, bool CheckForRunning);
#endif

#endif
