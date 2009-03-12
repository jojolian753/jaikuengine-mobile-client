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
#include "ContextMediaAppDocument.h"
#include "ContextMediaAppAppUi.h"
#include "settings.h"
#include "bb_settings.h"
#include "cl_settings.h"
#include "cl_settings_impl.h"
#include "symbian_auto_ptr.h"
#include <basched.h>
#include <coemain.h>
#include "checkedactive.h"
#include <akntitle.h>
#include "contextnotifyclientsession.h"

#ifdef __WINS_
#define DO_DEBUG

class CActiveSchedulerAccess : public CBase
        {
public:
        IMPORT_C CActiveSchedulerAccess();
        IMPORT_C ~CActiveSchedulerAccess();
        IMPORT_C static void Install(CActiveScheduler* aScheduler);
        IMPORT_C static CActiveScheduler* Current();
        IMPORT_C static void Add(CActive* anActive);
        IMPORT_C static void Start();
        IMPORT_C static void Stop();
        IMPORT_C static TBool RunIfReady(TInt& aError, TInt aMinimumPriority);
        IMPORT_C static CActiveScheduler* Replace(CActiveScheduler* aNewActiveScheduler);
        IMPORT_C virtual void WaitForAnyRequest();
        IMPORT_C virtual void Error(TInt anError) const;
private:
        void DoStart();
        void OwnedStartLoop(TInt& aRunning);
        IMPORT_C virtual void OnStarting();
        IMPORT_C virtual void OnStopping();
        IMPORT_C virtual void Reserved_1();
        IMPORT_C virtual void Reserved_2();
private:
        // private interface used through by CActiveSchedulerWait objects
        friend class CActiveSchedulerWait;
        static void OwnedStart(CActiveSchedulerWait& aOwner);
protected:
        inline TInt Level() const;
public:
        TInt iLevel;
        TPriQue<CActive> iActiveQ;
        };


class CActiveAccess : public CBase
        {
public:
enum TPriority
        {
        EPriorityIdle=-100,
        EPriorityLow=-20,
        EPriorityStandard=0,
        EPriorityUserInput=10,
        EPriorityHigh=20,
        };
public:
        IMPORT_C ~CActiveAccess();
        IMPORT_C void Cancel();
        IMPORT_C void Deque();
        IMPORT_C void SetPriority(TInt aPriority);
        inline TBool IsActive() const;
        inline TBool IsAdded() const;
        inline TInt Priority() const;
protected:
        IMPORT_C CActiveAccess(TInt aPriority);
        IMPORT_C void SetActive();
// Pure virtual
        virtual void DoCancel() =0;
        virtual void RunL() =0;
        IMPORT_C virtual TInt RunError(TInt aError);
public:
        TRequestStatus iStatus;
private:
        TBool iActive;
        TPriQueLink iLink;
        friend class CActiveScheduler;
        friend class CServer;
	friend class CDummy;
        };

class CDummy {
public:
	static IsCheckedActive(CActive* a) {
		CActiveAccess* aa=(CActiveAccess*)a;
		return ( (aa->RunL) == (void (CActiveAccess::*)(void))(CCheckedActive::RunL) );
	}
};

class CDebugScheduler : public CCoeScheduler, public MContextBase {
public:
	CDebugScheduler(MApp_context& ctx, CCoeEnv* aCoeEnv) : CCoeScheduler(aCoeEnv), MContextBase(ctx) { }
        void Error(TInt aError) const {
                User::Leave(aError);
        }
	void WaitForAnyRequest() {
		RThread me;
		CActiveSchedulerAccess* a=(CActiveSchedulerAccess*)(CActiveScheduler*)this;
		//if (me.RequestCount()<=0) {
			TDblQueIter<CActive> iterator(a->iActiveQ);
			for (CActive* active=iterator++; active; active=iterator++) {
				if (active->IsActive() && (active->iStatus!=KRequestPending)) {
					if ( CDummy::IsCheckedActive(active) ) {
						TInt x;
						x=0;
					}
					if (me.RequestCount()<=0) {
						TBuf<100> msg=_L("WRONG active object ");
						if ( CDummy::IsCheckedActive(active) ) {
							CCheckedActive* a=(CCheckedActive*)active;
							msg.Append(a->Name());
						} else {
						// WRONG!
							msg.AppendNum( (TUint)active, EHex);
						}
						DebugLog(msg);
						User::Panic(_L("CDebugScheduler"), 1);
					}
				}
			}
		//}
		CCoeScheduler::WaitForAnyRequest();
	}
};

#endif

CContextMediaAppDocument::CContextMediaAppDocument(CEikApplication& aApp)
: CAknDocument(aApp)   {}

CContextMediaAppDocument::~CContextMediaAppDocument()
{
	{

		if (iBuiltinScheduler) {
			CActiveScheduler::Replace(iBuiltinScheduler);
		}
#ifdef DO_DEBUG
		delete iDebugScheduler;
#endif
		delete iNetwork;
		delete iThreadStorage;
		delete iBBSession;
		delete iBBDataFactory;
		delete iThreadStorageDb;
	}
	delete iContext;
}

void CContextMediaAppDocument::ConstructL()
{	
	CALLSTACKITEM_N(_CL("CContextMediaAppDocument"), _CL("ConstructL"));

	iContext=CApp_context::NewL(false, _L("context_media_app"));

	CC_TRAPD(err, InnerConstructL());
	if (err!=KErrNone) {
		iContext->ReportActiveError(_L(""),
			_L("Construct"), err);
	}
	User::LeaveIfError(err);
}

void CContextMediaAppDocument::InnerConstructL()
{	
	CALLSTACKITEM_N(_CL("CContextMediaAppDocument"), _CL("InnerConstructL"));

	RThread me;
	me.SetPriority(EPriorityMuchMore);
	me.SetProcessPriority(EPriorityForeground);
	

	iContext->SetDebugLog(_L("ContextMedia"), _L("App.txt"));

#ifdef DO_DEBUG
	iDebugScheduler=new (ELeave) CDebugScheduler(*iContext, CCoeEnv::Static());
	iBuiltinScheduler=CActiveScheduler::Replace(iDebugScheduler);
#endif

#ifndef __WINS__
	TParsePtrC parse(Application()->DllName());
	iContext->SetAppDir(parse.DriveAndPath());
#else
	iContext->SetAppDir(_L("c:\\system\\apps\\contextmediaapp\\"));
#endif
	iContext->SetDataDir(_L("c:\\system\\data\\context\\cmedia\\"), EFalse);
	
	CBlackBoardSettings* s=CBlackBoardSettings::NewL(*iContext, iDefaultSettings, KCLSettingsTuple);
	iContext->SetSettings(s);

	iBBDataFactory=CBBDataFactory::NewL();
	iBBSession=CBBSession::NewL(*iContext, iBBDataFactory);
	iContext->SetBBSession(iBBSession);
	iContext->SetBBDataFactory(iBBDataFactory);

#ifdef __WINS__
//	iContext->Fs().Delete(_L("c:\\system\\apps\\contextmediaapp\\threads.db"));
//	iContext->Fs().Delete(_L("c:\\system\\apps\\contextmediaapp\\media_transfer.db"));
#endif

	iThreadStorageDb = CDb::NewL(*iContext, _L("THREADS"), EFileRead|EFileWrite|EFileShareAny);
	iThreadStorage = CPostStorage::NewL(*iContext, *iThreadStorageDb, iBBDataFactory);

	iNetwork = CCMNetwork::NewL(iThreadStorage, *iContext, 
		_L("http://db.cs.helsinki.fi/~mraento/cgi-bin/getvc.pl"),
		60*5, _L("c:\\system\\data\\context\\cmedia\\"), 
		SETTING_IP_AP);
}

CContextMediaAppDocument* CContextMediaAppDocument::NewL(CEikApplication& aApp)     
{
	CALLSTACKITEM_N(_CL("CContextMediaAppDocument"), _CL("NewL"));

	CContextMediaAppDocument* self = new (ELeave) CContextMediaAppDocument( aApp );
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CEikAppUi* CContextMediaAppDocument::CreateAppUiL()
{
	return new (ELeave) CContextMediaAppAppUi(*iContext, *iThreadStorage, iBBDataFactory, *iNetwork);
}
