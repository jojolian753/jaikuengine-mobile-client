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

#include "ccn_feedengine.h"

#include "app_context_impl.h"
#include "break.h"
#include "db.h"
#include "ccu_storage.h"
#include "ccu_feedstorager.h"
#include "reporting.h"
#include "symbian_auto_ptr.h"
#include "settings.h"
#include "cl_settings.h"
#include <bautils.h>

#include <aknglobalnote.h>


_LIT( KFeedName, "FEED2");
_LIT(KCorruptErrorMessage, "Feed Data is corrupted, resetting. Please report this to bugs@jaiku.com (with logs from Jaiku Tool)");

class CFeedEngineImpl : public CFeedEngine, public MContextBase
{
public:
	virtual CFeedItemStorage& Storage() 
	{
		return *iFeedStore;
	}

public:
	CFeedEngineImpl(CJabberData& aJabberData) : iJabberData(aJabberData) {}
	

	~CFeedEngineImpl()
	{
		CALLSTACKITEM_N(_CL("CFeedEngineImpl"), _CL("~CFeedEngineImpl"));
		delete iFeedStorager;
		delete iFeedStore;
		
		if (iFeedDb && iFeedDb->IsCorrupt()) {
			delete iFeedDb; iFeedDb=0;
			TRAPD(err, 
				CDb::ResetCorruptedDatabaseL(AppContext(), KFeedName);
				Reporting().ShowGlobalNote(EAknGlobalErrorNote, KCorruptErrorMessage);
			);
			
		}
		delete iFeedDb;
	}
	
	void OpenDbL()
	{
		CALLSTACKITEM_N(_CL("CFeedEngineImpl"), _CL("OpenDbL"));
		_LIT( KOldFeedName, "FEED.db");
		{
			auto_ptr<HBufC> fn(HBufC::NewL(256));
			fn->Des().Append(DataDir());
// 			fn->Des().Append(_L("\\"));
			fn->Des().Append(KOldFeedName);
			if (BaflUtils::FileExists(Fs(), *fn)) {
				_LIT(KOldFeedErrorMessage, "Removing feed data from previous version.");
				Reporting().ShowGlobalNote(EAknGlobalErrorNote, KOldFeedErrorMessage);
				BaflUtils::DeleteFile(Fs(), *fn);
			}
		}
		
		CC_TRAPD(err, iFeedDb=CDb::NewL(AppContext(), KFeedName, EFileWrite, false) );
		
		if ( err != KErrNone )
			{
				Reporting().UserErrorLog( _L("Feed db construction failed"), err);
				
				if ( err == KErrCorrupt )
					{
						auto_ptr<CAknGlobalNote> note(CAknGlobalNote::NewL());
						note->ShowNoteL(EAknGlobalErrorNote, KCorruptErrorMessage);
						
						CDb::ResetCorruptedDatabaseL(AppContext(), KFeedName);
						
						err = KErrNone;
						CC_TRAP(err, iFeedDb=CDb::NewL(AppContext(), KFeedName, EFileWrite, false) );
						Reporting().UserErrorLog( _L("Creating new feed db after corrupted failed"), err);
					}
				User::LeaveIfError(err);
			}
	}
	

	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CFeedEngineImpl"), _CL("ConstructL"));
		OpenDbL();

		TInt err = KErrNone;		
		CC_TRAP( err, iFeedStore = CFeedItemStorage::NewL(AppContext(), *iFeedDb, AppContext().BBDataFactory() ) );
		if ( err != KErrNone )
			{
				Reporting().UserErrorLog( _L("Feed item storage construction failed"), err );
				User::LeaveIfError(err);
			}
		
		CC_TRAP( err, iFeedStorager = CFeedStorager::NewL( *iFeedStore, iJabberData ) );
		if ( err != KErrNone )
			{
				Reporting().UserErrorLog( _L("Feed storager construction failed"), err );
				User::LeaveIfError(err);
			}
	}
	
	
private:
	CDb* iFeedDb;
	CFeedItemStorage* iFeedStore;
	CFeedStorager* iFeedStorager;
	
	CJabberData& iJabberData;
};

CFeedEngine* CFeedEngine::NewL(CJabberData& aJabberData)
{
	CALLSTACKITEMSTATIC_N(_CL("CFeedEngine"), _CL("NewL"));
	auto_ptr<CFeedEngineImpl> self(new (ELeave) CFeedEngineImpl(aJabberData) );
	self->ConstructL();
	return self.release();
}


