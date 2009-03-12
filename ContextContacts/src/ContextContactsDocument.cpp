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

#include "ContextContactsDocument.h"
#include "ContextContactsAppUi.h"
#include "settings.h"

#include "bb_settings.h"
#include "cl_settings.h"
#include "cbbsession.h"
#include "symbian_auto_ptr.h"

#include "ccu_activecontact.h"
#include "ccu_constants.h"
#include "cc_processmanagement.h"
#include "contextvariant.hrh"
#include <StringLoader.h>
#include <contextcontacts.rsg>
#include <aknglobalnote.h>
#include "callstack.h"

CContextContactsDocument::CContextContactsDocument(CEikApplication& aApp)
: CAknDocument(aApp) { }

#include <errorui.h>

void CContextContactsDocument::ConstructL()
{
	auto_ptr<CErrorUI> eui(CErrorUI::NewL());
	
#ifdef __WINS__
	RFs fs; User::LeaveIfError(fs.Connect());
	fs.Delete(_L("e:\\fill.txt"));
	fs.Close();
#endif
	TRAPD(err, MContextDocument::ConstructL(Application(),
		iDefaultSettings, KCLSettingsTuple,
		_L("contextcontacts.txt"), _L("contacts")));

	//iContext->SetDebugCallstack(ETrue);


	if (err!=KErrNone) {
		eui->ShowGlobalErrorNoteL(err);
		User::Leave(err);
	}
	
#ifdef __JAIKU__
	TBool run_welcome=EFalse;
	TInt accepted_privacy_stmt_version=0;
	iContext->Settings().GetSettingL(SETTING_ACCEPTED_PRIVACY_STMT_VERSION, accepted_privacy_stmt_version);
	TInt text_resource=0;
	if ( accepted_privacy_stmt_version < KJaikuPrivacyStatementVersion ) {
		text_resource=R_TXT_PRIVACYSTATEMENT_NOT_ACCEPTED;
		run_welcome=ETrue;
	} else {
		TBool allowed_network_access=EFalse;
		iContext->Settings().GetSettingL(SETTING_ACCEPTED_NETWORK_ACCESS, allowed_network_access);
		if (!allowed_network_access) {
			text_resource=R_TXT_WELCOME_NOT_COMPLETED;
			run_welcome=ETrue;
		}
	}
	
	if (run_welcome) {
		auto_ptr<HBufC> message( StringLoader::LoadL( text_resource ) );

		auto_ptr<CAknGlobalNote> note( CAknGlobalNote::NewL() );
		note->ShowNoteL(EAknGlobalInformationNote, *message);
		
		iContext->CallStackMgr().SetIsExiting(ETrue);
		ProcessManagement::StartApplicationL( KUidContextWelcome );
		
		User::After(3*1000*1000); // give starter a chance to run
		
		User::Leave(KLeaveExit); 
	}
#endif

#ifdef __WINS__
	iContext->SetAppDir(_L("c:\\system\\apps\\contextcontacts\\"));
#endif
}

#ifdef __WINS__
IMPORT_C void AllocateContextCommonExceptionData();
#endif

CContextContactsDocument* CContextContactsDocument::NewL(
							 CEikApplication& aApp)     // CContextContactsApp reference
{	
	CContextContactsDocument* self = new (ELeave) CContextContactsDocument( aApp );
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();
	
	return self;
}

CContextContactsDocument::~CContextContactsDocument() 
{ 
}

CEikAppUi* CContextContactsDocument::CreateAppUiL()
{
	CALLSTACKITEM_N(_CL("CContextContactsDocument"), _CL("CreateAppUiL"));
	
#ifdef __WINS__
	User::Free(User::AllocL( 32*1024 ));
#endif
	return iAppUi=new (ELeave) CContextContactsAppUi(*iContext);
}

void CContextContactsDocument::AppUiConstructed()
{
	iAppUi=0;
}

