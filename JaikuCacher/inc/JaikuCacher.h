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

#ifndef __JAIKUCACHER__
#define __JAIKUCACHER__

#include <e32base.h>
#include <w32std.h>

#include "CacherCommon.h"

#include "compat_server.h"
#include <mpbkcontactdbobserver.h> 
#include "timeout.h"
#include "app_context.h"

class CJaikuCacher : public SERVER_CLASS, public MContextBase, public MPbkContactDbObserver, public MTimeOut {
public:
	static CJaikuCacher* NewL();
	~CJaikuCacher();

	static TInt ThreadFunction(TAny* aNone);
	
	void IncrementSessions();
	void DecrementSessions();

	void RunL();
	TInt CheckedRunError(TInt aError);

	CJaikuCacher(TInt aPriority) ;

	void ConstructL() ;

	static void PanicClient(const MESSAGE_CLASS& aMessage, TJaikuCacherPanic aReason);
	static void PanicServer(TJaikuCacherPanic aReason);
	static void ThreadFunctionL();

	void TerminateJaikuCacher();

	void CancelRequest(const MESSAGE_CLASS &aMessage);

public:
	void ReportError(TJaikuCacherRqstComplete aErrorType, TDesC & aErrorCode, TDesC & aErrorValue);
	void GetCurrentContactsNameAndGeneration(TDes& aHashNameInto, TDes& aListNameInto, TInt& aGenerationInto);
	void SetContactsDbName(const TDesC& aDb);

	enum TEvent { ETerminated, EContactsChanged };
private:
	void NotifySessions(TEvent aEvent);
	// MPbkContactDbObserver::
	void HandleDatabaseEventL(TContactDbObserverEvent aEvent); 
	
	enum TContactsState { EContactsIdle, EContactsAsyncReading };
	TContactsState iContactsState;
	
	void expired(CBase*);
	void ReadFromFileL();
	void ReadFromDbL(TBool aForceSync);
	TBool ReadNextFromDbL();
	void CreateContactsChunkL();
	void SwapContactsChunks();
	void WriteToFileL();
	void ReleaseEngine();

	TInt iSessionCount;
#ifndef __IPCV2__
	SESSION_CLASS *	NewSessionL(const TVersion &aVersion) const;
#else
	SESSION_CLASS *	NewSessionL(const TVersion &aVersion, const MESSAGE_CLASS &aMessage) const;
#endif

	class CCacheChunk *iContactListChunk, *iNextContactListChunk;
	class CCacheChunk *iContactHashChunk, *iNextContactHashChunk;
	
	class CPbkContactEngine *iContactEngine;
	class CPbkContactIter* iContactIterator; TContactItemId iCurrentContact;
	TBuf<100> iFirst, iLast, iExtra;
	TBuf8<16> iPhoneHash;
	class CPbkContactChangeNotifier* iContactChangeNotifier;
	CTimeOut* iTimer;
	TInt iContactsGeneration, iContactCount;
	TFileName iNameBuffer;
	TBool iContactsChangePending;
	TFileName iContactsDbName;
	class CGenericIntMap *seen_contacts;
};

#endif
