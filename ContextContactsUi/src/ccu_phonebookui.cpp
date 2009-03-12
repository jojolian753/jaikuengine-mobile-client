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
#include "ccu_phonebookui.h"
#include <contextcontactsui.rsg>

#include "app_context.h"
#include <txtrich.h>
#include <SenduiMtmUids.h>
#include <eikenv.h>

#include <aknlistquerydialog.h>
#include <cpbkcontacteditordlg.h> 
#include <MsgBioUids.h>
#include <msvapi.h>
#include <msvids.h>
#include <aknviewappui.h> 
#include <cpbkcontactitem.h> 
#include <bcardeng.h>
#include <CPbkContactEngine.h>
#include <CPbkSingleEntryFetchDlg.h> 

#include "symbian_auto_ptr.h"

EXPORT_C CPhonebookUi* CPhonebookUi::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CPhonebookUi"), _CL("NewL"));
	auto_ptr<CPhonebookUi> self( new (ELeave) CPhonebookUi() );
	self->ConstructL();
	return self.release();
}

#include "app_context_impl.h"

CPbkContactEngine* CPhonebookUi::Engine() {
	if (!iMyEngine) {
		iMyEngine=CPbkContactEngine::Static();
		if (!iMyEngine) {
			iMyEngine=CPbkContactEngine::NewL();
			AppContext().TakeOwnershipL(iMyEngine);		
		}
	}
	return iMyEngine;
}

void CPhonebookUi::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPhonebookUi"), _CL("ConstructL"));
	iPbkRes.OpenL();
}

CPhonebookUi::CPhonebookUi() : iPbkRes( * (CEikonEnv::Static() ) ) {}

EXPORT_C CPhonebookUi::~CPhonebookUi()
{
	CALLSTACKITEM_N(_CL("CPhonebookUi"), _CL("~CPhonebookUi"));
	iPbkRes.Close();
}


EXPORT_C TContactItemId CPhonebookUi::AddNewContactL(const TDesC& aFirstName,
													 const TDesC& aLastName,
													 const TDesC& aNumber,
													 const TDesC& aEmail)
{	
	CALLSTACKITEM_N(_CL("CPhonebookUi"), _CL("AddNewContactL"));
	auto_ptr<CPbkContactItem> contact( Engine()->CreateEmptyContactL() );	
	if (aFirstName != KNullDesC) contact->FindField( EPbkFieldIdFirstName )->TextStorage()->SetTextL( aFirstName );
	if (aLastName != KNullDesC)  contact->FindField( EPbkFieldIdLastName )->TextStorage()->SetTextL( aLastName );
	if (aNumber != KNullDesC)     contact->FindField( EPbkFieldIdPhoneNumberGeneral )->TextStorage()->SetTextL( aNumber );
	if (aEmail != KNullDesC)     contact->FindField( EPbkFieldIdEmailAddress )->TextStorage()->SetTextL( aEmail );
	return Engine()->AddNewContactL( *contact );   
}


EXPORT_C TContactItemId CPhonebookUi::EditNewContactL(const TDesC& aFirstName,
													  const TDesC& aLastName,
													  const TDesC& aNumber,
													  const TDesC& aEmail)
{
	CALLSTACKITEM_N(_CL("CPhonebookUi"), _CL("EditNewContactL"));
	auto_ptr<CPbkContactItem> contact( Engine()->CreateEmptyContactL() );	
	contact->FindField( EPbkFieldIdFirstName )->TextStorage()->SetTextL( aFirstName );
	contact->FindField( EPbkFieldIdLastName )->TextStorage()->SetTextL( aLastName );
	contact->FindField( EPbkFieldIdPhoneNumberGeneral )->TextStorage()->SetTextL( aNumber );
	contact->FindField( EPbkFieldIdEmailAddress )->TextStorage()->SetTextL( aEmail );
	
	CPbkContactEditorDlg *editor= CPbkContactEditorDlg::NewL(*Engine(), *contact, ETrue, -1, ETrue);
	TContactItemId res = KNullContactId;
	CC_TRAPD( err, res = editor->ExecuteLD() );
	return res;
}


EXPORT_C TContactItemId CPhonebookUi::EditContactL(TContactItemId aContactId)
{	
	CALLSTACKITEM_N(_CL("CPhonebookUi"), _CL("EditContactL"));
	auto_ptr<CPbkContactItem> contact( Engine()->ReadContactL( aContactId ));
	CPbkContactEditorDlg *editor= CPbkContactEditorDlg::NewL(*Engine(), *contact, ETrue, -1, ETrue);
	TContactItemId res = KNullContactId;
	CC_TRAPD( err, res = editor->ExecuteLD() );
	return res;
}


EXPORT_C TContactItemId CPhonebookUi::FetchContactL( )
{
	CALLSTACKITEM_N(_CL("CPhonebookUi"), _CL("FetchContactL"));
	CPbkSingleEntryFetchDlg::TParams params;
	params.iPbkEngine = Engine();
	
	
	CPbkSingleEntryFetchDlg* dlg = CPbkSingleEntryFetchDlg::NewL(params);
	TInt okPressed = dlg->ExecuteLD();
	
	return okPressed ? params.iSelectedEntry : KNullContactId;
}


EXPORT_C TContactItemId CPhonebookUi::QueryContactForNickL(const TDesC& aNick, const TDesC& aFirstName, const TDesC& aLastName)
{
	TInt selection = KErrNotFound;
    CAknListQueryDialog* dlg = new (ELeave) CAknListQueryDialog( &selection );
    if ( dlg->ExecuteLD( R_DUMMY_CONTACT_ACTION_QUERY ) )
		{
			if ( selection == 0 )
				{
					TContactItemId id = FetchContactL();
					return id;
				}
			else
				{
					TPtrC firstName = aFirstName;
					if (firstName.Length() == 0 && aLastName.Length() == 0) firstName.Set(aNick);
					TContactItemId id = EditNewContactL( firstName, aLastName, KNullDesC, KNullDesC);
				    return id; 
				}
		}
	return KNullContactId;
}
