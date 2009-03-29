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

#include "cwu_welcomeaction.h"

#include "cwu_welcomeviews.h"
#include <contextwelcomeui.rsg>

#include "ccu_contactmatcher.h"
#include "ccu_phonebookui.h"

#include "app_context_impl.h"
#include "break.h"
#include "cl_settings.h"
#include "jabberdata.h"
#include "reporting.h"
#include "settings.h"
#include "symbian_auto_ptr.h"

#include <StringLoader.h>
#include <cpbkcontactengine.h>
#include <cpbkcontactitem.h>

class CUserSelection 
	: public CWelcomeAction, 
	  public MPageObserver {
public: // From CWelcomeAction
	void StartBackgroundActivitiesL();
	void ShowUiL(MWelcomeView& aView);

public:
	static CUserSelection* NewL();	
	virtual ~CUserSelection();
private: // from MPageObserver
	void ProcessSelectionL( TInt aIndex );
	void SelectedItemL(TInt aIndex) ;
	void LeftSoftKeyL(TInt aIndex) ;
	void RightSoftKeyL(TInt aIndex) ;

private: // own
	CUserSelection();
	void ConstructL();
	void StoreAsUserAndFinishL(TInt aContactId, TBool aWasMatched );
	void StoreAsDummyUserAndFinishL();

	void FetchUserUiL();
	void CreateUserUiL();
	TBool UserContactExistsL(const TDesC& aNick );
	
	void InitContactEngineL();
	TInt TryToMatchUserL();
	void QueryContactCardL();

private:
	CJabberData::TNick iNick;
	TBuf<50>  iPhoneNumber;
	TBuf<100> iEmail;
	TBuf<100> iUserFirstName;
	TBuf<100> iUserLastName;
	TBuf<100> iPassword;

private:
	CDesCArray* iUserSelection;

	CPhonebookUi* iPhonebookUi;

	MWelcomeView* iView;

	CPbkContactEngine* iMyEngine; // owned by app context!
};



CUserSelection* CUserSelection::NewL() 
{
	CALLSTACKITEMSTATIC_N(_CL("CUserSelection"), _CL("NewL"));
	auto_ptr<CUserSelection> self( new (ELeave) CUserSelection( ) );
	self->ConstructL();
	return self.release();
}

CUserSelection::CUserSelection()  {}

void CUserSelection::ConstructL()
{				
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("ConstructL"));
	iUserSelection = new CDesCArrayFlat(10);
	

	auto_ptr<HBufC> text1( StringLoader::LoadL( R_TEXT_WELCOME_SELECT_FROM_CONTACTS ));
	auto_ptr<HBufC> text2( StringLoader::LoadL( R_TEXT_WELCOME_CREATE_NEW_CONTACT_CARD ));
					
	iUserSelection->AppendL( *text1 );
	iUserSelection->AppendL( *text2 );
}
	
CUserSelection::~CUserSelection()
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("~CUserSelection"));
	delete iPhonebookUi;
	delete iUserSelection;		
}

				
void CUserSelection::StoreAsUserAndFinishL(TInt aContactId, TBool aMatched)
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("StoreAsUserL"));
	auto_ptr<CDb> db( CDb::NewL(AppContext(), _L("JABBER"), EFileWrite|EFileShareExclusive) );
	auto_ptr<CJabberData> jabber( CJabberData::NewL(AppContext(), *db, SETTING_JABBER_NICK) );

	CJabberData::TRelationReason reason = aMatched ?
		CJabberData::EMatchedWithUnverifiedNumber : CJabberData::ESetByUser;
	jabber->SetJabberNickL(aContactId, jabber->UserNickL(), reason );

	NotifyActionReadyL();
}

void CUserSelection::StoreAsDummyUserAndFinishL()
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("StoreAsDummyUserAndFinishL"));
	auto_ptr<CDb> db( CDb::NewL(AppContext(), _L("JABBER"), EFileWrite|EFileShareExclusive) );
	auto_ptr<CJabberData> jabber( CJabberData::NewL(AppContext(), *db, SETTING_JABBER_NICK) );

	TInt id = jabber->GetNewDummyContactIdL();
	CJabberData::TRelationReason reason = CJabberData::EAutomaticDummy;
	jabber->SetJabberNickL(id, jabber->UserNickL(), reason );

	NotifyActionReadyL();
}
	
void CUserSelection::StartBackgroundActivitiesL()
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("StartBackgroundActivitiesL"));	

	Reporting().DebugLog(_L("User selection, read from blackboard:"));	
	Settings().GetSettingL(SETTING_JABBER_NICK, iNick );
	Reporting().DebugLog(_L("Nick:"));	
	Reporting().DebugLog(iNick);	

	Settings().GetSettingL(SETTING_PHONENO, iPhoneNumber );
	Reporting().DebugLog(_L("Phone number:"));	
	Reporting().DebugLog(iPhoneNumber);	

	Settings().GetSettingL(SETTING_USER_EMAIL, iEmail );
	Reporting().DebugLog(_L("Email:"));	
	Reporting().DebugLog(iEmail);	

	Settings().GetSettingL(SETTING_USER_FIRSTNAME, iUserFirstName );
	Reporting().DebugLog(_L("First name:"));	
	Reporting().DebugLog(iUserFirstName);	

	Settings().GetSettingL(SETTING_USER_LASTNAME, iUserLastName );
	Reporting().DebugLog(_L("Last name:"));	
	Reporting().DebugLog(iUserLastName);	
	
}

//#define DO_CONTACT_QUERY_STEP

void CUserSelection::ShowUiL(MWelcomeView& aView)
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("ShowUiL"));	
	iView = &aView;

	InitContactEngineL();
	
	if ( iNick.Length() > 0 && UserContactExistsL( iNick ) )
		{
			NotifyActionReadyL();
		}
	else
		{
#ifdef DO_CONTACT_QUERY_STEP			
			TInt matchId = TryToMatchUserL();
			if ( matchId != KErrNotFound)
				{
					StoreAsUserAndFinishL( matchId, ETrue );
					return ;
				}
			
			QueryContactCardL();
#else
			// Just create dummy user 
			StoreAsDummyUserAndFinishL();
#endif
		}
}

void CUserSelection::InitContactEngineL()
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("InitContactEngineL"));	
	if ( ! iMyEngine )
		{
			iMyEngine = CPbkContactEngine::Static();
			if ( ! iMyEngine )
				{
					iMyEngine = CPbkContactEngine::NewL();
					GetContext()->TakeOwnershipL( iMyEngine );
				}
		}
}


TInt CUserSelection::TryToMatchUserL()
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("TryToMatchUserL"));	

	auto_ptr<CContactMatcher> matcher( CContactMatcher::NewL() );
	auto_ptr<CContactIdArray> ids(NULL);
	
	// try automatic matching 
	if ( iPhoneNumber.Length() != 0 )
		{
			ids.reset( matcher->FindMatchesForNumberL( iPhoneNumber ) );
		}
	
	if ( ids.get() && ids->Count() == 0 && iEmail.Length() != 0 )
		{
			ids.reset( matcher->FindMatchesForEmailL( iEmail ) );
		}
	
	if (ids.get() && ids->Count() == 1 )
		{
			Reporting().DebugLog(_L("Found one user"));
			return (*ids)[0];
		}
	return KErrNotFound;
}


void CUserSelection::QueryContactCardL()
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("QueryContactCardL"));	

	if ( ! iPhonebookUi )
		iPhonebookUi = CPhonebookUi::NewL();
	
	auto_ptr<HBufC> text( StringLoader::LoadL( R_TEXT_WELCOME_CONTACT_CARD_TEXT ));
	
	iView->SetPageL( CWelcomeSelectionPage::NewL( iView->ObjectProviderL(), *text, *iUserSelection, *this, CWelcomePageBase::EHeaderText_YourNumber) );
}

void CUserSelection::FetchUserUiL()
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("FetchUserUiL"));
	iView->HidePageL(ETrue);
	TContactItemId id = iPhonebookUi->FetchContactL();
	if ( id != KNullContactId )
		{
			StoreAsUserAndFinishL( id, EFalse );
			return;
		}
	else
		{
			iView->HidePageL(EFalse);
		}
}
	
void CUserSelection::CreateUserUiL()
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("CreateUserUiL"));
	TBuf<100> firstName;
	firstName = iUserFirstName;

	// Set nick as a first name, if neither first nor last name is set
	if ( firstName.Length() == 0 && iUserLastName.Length() == 0)
		{
			firstName = iNick;
			TInt ix = firstName.Locate('@');
			if (ix >= 0) firstName.Left(ix);
		}
	
	
	iView->HidePageL(ETrue);
	TContactItemId id = iPhonebookUi->EditNewContactL( firstName, iUserLastName, iPhoneNumber, iEmail );
	if ( id != KNullContactId )
		{
			StoreAsUserAndFinishL( id, EFalse );
			return;
		}
	else 
		{
			iView->HidePageL(EFalse);
		}
}
	
void CUserSelection::ProcessSelectionL( TInt aIndex )
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("ProcessSelectionL"));
	if (aIndex == 0) 
		{
			FetchUserUiL();
		}
	else
		{
			CreateUserUiL();
		}

}
	
void CUserSelection::SelectedItemL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("SelectedItemL"));
	ProcessSelectionL( aIndex );		
}


void CUserSelection::LeftSoftKeyL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("LeftSoftKeyL"));

	ProcessSelectionL( aIndex );
}

void CUserSelection::RightSoftKeyL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CUserSelection"), _CL("RightSoftKeyL"));
}


TBool CUserSelection::UserContactExistsL(const TDesC& aNick )
{
	auto_ptr<CDb> db( CDb::NewL(AppContext(), _L("JABBER"), EFileWrite|EFileShareExclusive) );
	auto_ptr<CJabberData> jabber( CJabberData::NewL(AppContext(), *db, SETTING_JABBER_NICK) );
	TInt id = jabber->GetContactIdL( aNick );

	if ( id == KNullContactId || id == KErrNotFound || jabber->IsDummyContactId( id ) )
		return EFalse;
	
	CJabberData::TRelationReason reason = CJabberData::EUnknown;
	jabber->GetRelationReasonL( id, reason );
	switch ( reason ) 
		{
		case CJabberData::EUnknown:
		case CJabberData::ERemovedByUser:
		case CJabberData::EAutomaticDummy:
			return EFalse;
		default:
			break; // continue
		};

	CPbkContactEngine* eng = iMyEngine;
	if ( ! eng ) User::Leave( KErrNotReady );
	auto_ptr<CPbkContactItem> item( NULL );
	CC_TRAPD(err, item.reset( eng->ReadContactL(id) ) );
	if (err != KErrNotFound && err != KErrNone ) 
		{
			User::Leave( err );
			return EFalse;
		}
	
	if ( item.get() )
		return ETrue;
	else
		return EFalse;
}

CWelcomeAction* CreateUserSelectionActionL()
{
	return CUserSelection::NewL();
}


