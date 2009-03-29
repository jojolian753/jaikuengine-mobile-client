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
#include <ContextWelcomeUi.rsg>

#include "ccu_constants.h"
#include "ccu_platforminspection.h"

#include "cc_processmanagement.h"
#include "cl_settings.h"
#include "symbian_auto_ptr.h"
#include "concretedata.hrh"

#include <StringLoader.h>
#include <aknviewappui.h>
#include <aknmessagequerydialog.h> 


static TInt ShowMessageDialogWithHeaderL(TInt aDialog, TInt aBodyTextResource, TInt aHeaderTextResource)
{
	auto_ptr<HBufC> text( StringLoader::LoadL( aBodyTextResource ) );
	TPtrC textP( *text ); 
	
	auto_ptr<HBufC> title( StringLoader::LoadL( aHeaderTextResource ) );
	TPtrC titleP( *title );

	auto_ptr<CAknMessageQueryDialog> note( CAknMessageQueryDialog::NewL(textP) );
	note->SetHeaderTextL(titleP);
	TInt result = note->ExecuteLD(aDialog);
	note.release();
	return result;
}



static TInt ShowConfirmationQueryL(TInt aQuery, TInt aBody)
{
	auto_ptr<HBufC> text( StringLoader::LoadL( aBody ) );
	TPtrC textP( *text ); 
	
	auto_ptr<CAknQueryDialog> note( CAknQueryDialog::NewL() );
	TInt result = note->ExecuteLD(aQuery, textP);
	note.release();
	return result;
}

static TInt ShowMessageDialogL(TInt aQuery, TInt aBody)
{
	auto_ptr<HBufC> text( StringLoader::LoadL( aBody ) );
	TPtrC textP( *text ); 
	
	auto_ptr<CAknMessageQueryDialog> note( CAknMessageQueryDialog::NewL(textP) );
	TInt result = note->ExecuteLD( aQuery );
	note.release();
	return result;
}


static TBool ShowOkCancelQueryL(TInt aBodyTextResource)
{
	return ShowConfirmationQueryL(R_OKCANCEL_QUERY_DIALOG, aBodyTextResource) != 0;
}

static TBool ShowYesNoQueryL(TInt aBodyTextResource)
{
	return ShowConfirmationQueryL(R_YESNO_QUERY_DIALOG, aBodyTextResource) != 0;
}

static TBool ShowYesNoMessageL(TInt aBodyTextResource, TInt aHeader)
{
	return ShowMessageDialogWithHeaderL(R_YESNO_MESSAGE_DIALOG_WITH_HEADER, aBodyTextResource, aHeader) != 0;
}

static TBool ShowOkCancelMessageL(TInt aBodyTextResource, TInt aHeader)
{
	return ShowMessageDialogWithHeaderL(R_OKCANCEL_MESSAGE_DIALOG_WITH_HEADER, aBodyTextResource, aHeader) != 0;
}



CWelcomeAction::~CWelcomeAction()
{
	iObservers.Close();
}


void CWelcomeAction::AddObserverL( MWelcomeActionObserver& aObserver )
{
	iObservers.Append( &aObserver );
}


void CWelcomeAction::RemoveObserverL( MWelcomeActionObserver& aObserver )
{
	for ( TInt i=0; i < iObservers.Count(); i++)
		{
			if ( iObservers[i] == &aObserver )
				{
					iObservers.Remove(i);
					return;
				}
		}
}


void CWelcomeAction::NotifyActionReadyL()
{
	for ( TInt i=0; i < iObservers.Count(); i++)
		{
			iObservers[i]->ActionReadyL( *this );
		}
}

void CWelcomeAction::NotifyQuitWelcomeL()
{
	for ( TInt i=0; i < iObservers.Count(); i++)
		{
			iObservers[i]->QuitWelcomeL( *this );
		}
}


CWelcomeAction::CWelcomeAction() {}


void CWelcomeAction::ConstructL()
{
}


//
//
//

class CWelcomeSelectionAction 
	: public CWelcomeAction, 
	  public MPageObserver 
{
public: // from CWelcomeAction
	virtual void StartBackgroundActivitiesL()
	{
		CALLSTACKITEM_N(_CL("CWelcomeSelectionAction"), _CL("StartBackgroundActivitiesL"));		
	}

	virtual void ShowUiL(MWelcomeView& aView)
	{
		CALLSTACKITEM_N(_CL("CWelcomeSelectionAction"), _CL("ShowUiL"));				
		auto_ptr<HBufC> text( StringLoader::LoadL( iBodyTextId ) );
		aView.SetPageL( CWelcomeSelectionPage::NewL( aView.ObjectProviderL(),
													 *text, 
													 *iSelections, 
													 *this, 
													 iHeaderImageId) );
	}

public:	
	virtual ~CWelcomeSelectionAction()
	{
		CALLSTACKITEM_N(_CL("CWelcomeSelectionAction"), _CL("~CWelcomeSelectionAction"));
		delete iSelections;
	}
	
	
protected:
	CWelcomeSelectionAction(CWelcomePageBase::THeaderText aHeaderImageId, TInt aBodyTextId)
		: iHeaderImageId( aHeaderImageId ),
		  iBodyTextId( aBodyTextId )
	{
	}
	
	void ConstructL(const TInt* aSelectionIds)
	{
		CALLSTACKITEM_N(_CL("CWelcomeSelectionAction"), _CL("ConstructL"));
		TInt KMaxSelections=3;

		iSelections = new (ELeave) CDesCArrayFlat( KMaxSelections );

		for (TInt i=0; aSelectionIds[i] != KErrNotFound && i < KMaxSelections; i++)
			{
				auto_ptr<HBufC> text( StringLoader::LoadL( aSelectionIds[i] ) );
				iSelections->AppendL( *text );				
			}
	}
	
protected: // from MPageObserver
	virtual void ProcessSelectionL(TInt aIndex) = 0;

	virtual void SelectedItemL(TInt aIndex) { 
		CALLSTACKITEM_N(_CL("CWelcomeSelectionAction"), _CL("SelectedItemL()"));
		ProcessSelectionL( aIndex ); }
	virtual void LeftSoftKeyL(TInt aIndex) { 
		CALLSTACKITEM_N(_CL("CWelcomeSelectionAction"), _CL("LeftSoftKey()"));
		ProcessSelectionL( aIndex ); }
	virtual void RightSoftKeyL(TInt aIndex) {
		CALLSTACKITEM_N(_CL("CWelcomeSelectionAction"), _CL("RightSoftKey()"));
	}
	
private:
	CDesCArray* iSelections;
	CWelcomePageBase::THeaderText iHeaderImageId;
	TInt iBodyTextId;
};



//                 Smaller welcome actions

class CInfoPageStep
	: public CWelcomeAction, 
	  public MPageObserver 
{
public: // from CWelcomeAction
	void StartBackgroundActivitiesL() {}
	void ShowUiL(MWelcomeView& aView)
	{		
		aView.SetPageL(  CWelcomeInfoPage::NewL(aView.ObjectProviderL(),
												*iText,
												*this, CWelcomePageBase::EHeaderText_Login) );
	}
	
public:	
	static CInfoPageStep* NewL(const TDesC& aText);
	virtual ~CInfoPageStep() { delete iText; }
	
private:
	CInfoPageStep() {}
	void ConstructL(const TDesC& aText) { iText = aText.AllocL(); }

private: // from MPageObserver
	void ProcessSelectionL( TInt aIndex ) {
		NotifyActionReadyL(); }
	void SelectedItemL(TInt aIndex) { 
		ProcessSelectionL( aIndex ); }
	void LeftSoftKeyL(TInt aIndex) { 
		ProcessSelectionL( aIndex ); }
	void RightSoftKeyL(TInt aIndex) {}
	
private:	
	HBufC* iText;
};

CInfoPageStep* CInfoPageStep::NewL(const TDesC& aText) 
{
	CALLSTACKITEMSTATIC_N(_CL("CInfoPageStep"), _CL("NewL"));
	auto_ptr<CInfoPageStep> self( new (ELeave) CInfoPageStep() );
	self->ConstructL(aText);
	return self.release();
}


///                              ///
//   CIntroPageAction            ///
//                               ///


class CIntroAction
	: public CWelcomeAction, 
	  public MPageObserver 
{
public: // from CWelcomeAction
	void StartBackgroundActivitiesL() {}
	void ShowUiL(MWelcomeView& aView)
	{		
		aView.SetPageL(  CWelcomeIntroPage::NewL(aView.ObjectProviderL(),
												 *iText,
												 *this) );
	}
	
public:	
	static CIntroAction* NewL(const TDesC& aText);
	virtual ~CIntroAction() { delete iText; }
	
private:
	CIntroAction() {}
	void ConstructL(const TDesC& aText) { iText = aText.AllocL(); }

private: // from MPageObserver
	void ProcessSelectionL( TInt aIndex ) {
		NotifyActionReadyL(); }
	void SelectedItemL(TInt aIndex) { 
		ProcessSelectionL( aIndex ); }
	void LeftSoftKeyL(TInt aIndex) { 
		ProcessSelectionL( aIndex ); }
	void RightSoftKeyL(TInt aIndex) {}
	
private:	
	HBufC* iText;
};

CIntroAction* CIntroAction::NewL(const TDesC& aText) 
{
	CALLSTACKITEMSTATIC_N(_CL("CIntroAction"), _CL("NewL"));
	auto_ptr<CIntroAction> self( new (ELeave) CIntroAction() );
	self->ConstructL(aText);
	return self.release();
}



///                              ///
//   CCalendarSettingsQuery      ///
//                               ///


class CCalendarSettingsQuery 
	: public CWelcomeAction, 
	  public MPageObserver 
{
public: // from CWelcomeAction
	void StartBackgroundActivitiesL();
	void ShowUiL(MWelcomeView& aView);
	
public:	
	static CCalendarSettingsQuery*  NewL();
	~CCalendarSettingsQuery();	
	
private:
	CCalendarSettingsQuery( );
	void ConstructL();

private: // from MPageObserver
	void ProcessSelectionL( TInt aIndex );
	void SelectedItemL(TInt aIndex);
	void LeftSoftKeyL(TInt aIndex) ;
	void RightSoftKeyL(TInt aIndex);
	
private:
	CDesCArray* iSelections;
};
	


CCalendarSettingsQuery* CCalendarSettingsQuery::NewL() 
{
	CALLSTACKITEMSTATIC_N(_CL("CCalendarSettingsQuery"), _CL("NewL"));
	auto_ptr<CCalendarSettingsQuery> self( new (ELeave) CCalendarSettingsQuery() );
	self->ConstructL();
	return self.release();
}
	
CCalendarSettingsQuery::CCalendarSettingsQuery() {}

void CCalendarSettingsQuery::ConstructL()
{				
	CALLSTACKITEM_N(_CL("CCalendarSettingsQuery"), _CL("ConstructL"));
	iSelections = new (ELeave) CDesCArrayFlat(10);
	

	auto_ptr<HBufC> text1( StringLoader::LoadL( R_TEXT_WELCOME_SHOW_TITLE_OF_EVENT ) );
	auto_ptr<HBufC> text2( StringLoader::LoadL( R_TEXT_WELCOME_SHOW_BUSY_INSTEAD ) );
	auto_ptr<HBufC> text3( StringLoader::LoadL( R_TEXT_WELCOME_DONT_SHARE_EVENTS ) );

	
	iSelections->AppendL( *text1 );
	iSelections->AppendL( *text2 );
	iSelections->AppendL( *text3 );
}

CCalendarSettingsQuery::~CCalendarSettingsQuery()
{
	CALLSTACKITEM_N(_CL("CCalendarSettingsQuery"), _CL("~CCalendarSettingsQuery"));
	delete iSelections;
}


void CCalendarSettingsQuery::StartBackgroundActivitiesL()
{
}	

void CCalendarSettingsQuery::ShowUiL(MWelcomeView& aView)
{
	CALLSTACKITEM_N(_CL("CCalendarSettingsQuery"), _CL("QueryL"));
	
	auto_ptr<HBufC> text( StringLoader::LoadL( R_TEXT_WELCOME_SHARE_CALENDAR_TEXT ) );
	aView.SetPageL( CWelcomeSelectionPage::NewL( aView.ObjectProviderL(),
												 *text, 
												 *iSelections, 
												 *this, 
												 CWelcomePageBase::EHeaderText_Calendar) );
}

void CCalendarSettingsQuery::ProcessSelectionL( TInt aIndex )
{
	CALLSTACKITEM_N(_CL("CCalendarSettingsQuery"), _CL("ProcessSelectionL"));
	TInt sharing = SHARE_CALENDAR_NONE;
	switch ( aIndex ) 
		{
		case 0: sharing = SHARE_CALENDAR_FULL; break;
		case 1: sharing = SHARE_CALENDAR_FREEBUSY; break;
		case 2: sharing = SHARE_CALENDAR_NONE; break;
		};
	
	Settings().WriteSettingL(SETTING_CALENDAR_SHARING, sharing );
	NotifyActionReadyL();
}


void CCalendarSettingsQuery::SelectedItemL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CCalendarSettingsQuery"), _CL("SelectedItemL"));
	ProcessSelectionL( aIndex );
}


void CCalendarSettingsQuery::LeftSoftKeyL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CCalendarSettingsQuery"), _CL("LeftSoftKeyL"));
	ProcessSelectionL( aIndex );
}

void CCalendarSettingsQuery::RightSoftKeyL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CCalendarSettingsQuery"), _CL("RightSoftKeyL"));
}



///                              ///
//    CNickAndPasswordQuery      ///
//                               ///


class CNickAndPasswordQuery : public CWelcomeAction, public MPageObserver {
public: // from CWelcomeAction
	void StartBackgroundActivitiesL();
	void ShowUiL(MWelcomeView& aView);

public:	
	static CNickAndPasswordQuery*  NewL();
	virtual ~CNickAndPasswordQuery();

private: // from MPageObserver
	void ProcessSelectionL();
	void SelectedItemL(TInt aIndex);
	void LeftSoftKeyL(TInt aIndex);
	void RightSoftKeyL(TInt aIndex);

private:
	CNickAndPasswordQuery();
	void ConstructL();
private:
	TBuf<50> iNick;
	TBuf<50> iPasswd;
	TBuf<BB_LONGSTRING_MAXLEN> iHashedPass;
};



void CNickAndPasswordQuery::StartBackgroundActivitiesL()
{
}
 
void CNickAndPasswordQuery::ShowUiL(MWelcomeView& aView)
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("GetNickAndPasswordL"));
	Settings().GetSettingL( SETTING_JABBER_NICK, iNick );
   	Settings().GetSettingL( SETTING_JABBER_PASS, iPasswd );
	Settings().GetSettingL( SETTING_JABBER_PASS_SHA1, iHashedPass );

	if ( iNick.Length() == 0 || (iPasswd.Length() == 0 && iHashedPass.Length() == 0) )
		{	
			auto_ptr<HBufC> text( StringLoader::LoadL( R_TEXT_WELCOME_NICK_TEXT ) );

			aView.SetPageL( CWelcomeInfoPage::NewL( aView.ObjectProviderL(),
													*text, *this,
													CWelcomePageBase::EHeaderText_Login) );
		}
	else
		{
			NotifyActionReadyL();
		}
}

void CNickAndPasswordQuery::LeftSoftKeyL(TInt aIndex)
{
	ProcessSelectionL();
}

void CNickAndPasswordQuery::RightSoftKeyL(TInt aIndex)
{
}

void CNickAndPasswordQuery::SelectedItemL(TInt aIndex)
{
	ProcessSelectionL();
}

void Exit() {
  CAknAppUi* appui = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  appui->Exit();
}

void CNickAndPasswordQuery::ProcessSelectionL()
{
	if ( iNick.Length() == 0 ) 
		{			
			while (ETrue)
				{
					CAknTextQueryDialog* dlg;
					dlg = new(ELeave)CAknTextQueryDialog(iNick, CAknQueryDialog::ENoTone );			
					if ( dlg->ExecuteLD( R_WELCOME_NICK_QUERY ) )
						{
							if ( iNick.Length() > 0 )
								{
									Settings().WriteSettingL( SETTING_JABBER_NICK, iNick );
									break;
								}
						}
				  Exit();
				}
			
		}
	
	
	if ( iPasswd.Length() == 0 && iHashedPass.Length() == 0 )
		{
			
			TBuf<50> Password;
			while ( ETrue )
				{
					
					CAknTextQueryDialog* dlg;
					dlg = new(ELeave)CAknTextQueryDialog(Password, CAknQueryDialog::ENoTone );
					dlg->PrepareLC(R_WELCOME_PASSWORD_QUERY);
					if ( dlg->RunLD( ) )
						{
							if ( Password.Length() > 0 )
								{
									Settings().WriteSettingL( SETTING_JABBER_PASS, Password );
									break;
								}
						}
 				  Exit();
				}			
		}
	NotifyActionReadyL();
}


CNickAndPasswordQuery*  CNickAndPasswordQuery::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CNickAndPasswordQuery"), _CL("NewL"));
	auto_ptr<CNickAndPasswordQuery> self( new (ELeave) CNickAndPasswordQuery( ) );
	self->ConstructL();
	return self.release();

}
	
CNickAndPasswordQuery::CNickAndPasswordQuery(  ) 
{

}
	
void CNickAndPasswordQuery::ConstructL()
{
}
	
CNickAndPasswordQuery::~CNickAndPasswordQuery()
{
}



///                              ///
//              CFinalQuery      ///
//                               ///

//#define ADD_SHORTCUT_QUERY


class CFinalQuery 
	: public CWelcomeAction, 
	  public MPageObserver 
{
public: // from CWelcomeAction
	void StartBackgroundActivitiesL();
	void ShowUiL(MWelcomeView& aView);
	
public:	
	static CFinalQuery*  NewL();
	~CFinalQuery();	
	
private:
	CFinalQuery( );
	void ConstructL();

	void AddShortcutL();
	void LaunchJaikuL();
	

private: // from MPageObserver
	void ProcessSelectionL( TInt aIndex );
	void SelectedItemL(TInt aIndex);
	void LeftSoftKeyL(TInt aIndex) ;
	void RightSoftKeyL(TInt aIndex);

	
private:
	CDesCArray* iSelections;

	enum TState
		{
			EInit = 0,
			EQueryFinalAction,
			EShowShortcutHelp,
			ELaunchingJaiku,
			ELaunchingSettings
		} iState;
	MWelcomeView* iView;
};
	


CFinalQuery* CFinalQuery::NewL() 
{
	CALLSTACKITEMSTATIC_N(_CL("CFinalQuery"), _CL("NewL"));
	auto_ptr<CFinalQuery> self( new (ELeave) CFinalQuery() );
	self->ConstructL();
	return self.release();
}
	
CFinalQuery::CFinalQuery() {}

void CFinalQuery::ConstructL()
{				
	CALLSTACKITEM_N(_CL("CFinalQuery"), _CL("ConstructL"));
	iSelections = new (ELeave) CDesCArrayFlat(10);

#ifdef ADD_SHORTCUT_QUERY
	auto_ptr<HBufC> text1( StringLoader::LoadL( R_TEXT_WELCOME_ADD_SHORTCUT_TO_IDLE) );
	auto_ptr<HBufC> text2( StringLoader::LoadL( R_TEXT_WELCOME_LAUNCH_JAIKU ) );
	
	iSelections->AppendL( *text1 );
	iSelections->AppendL( *text2 );
#else
	auto_ptr<HBufC> text1( StringLoader::LoadL( R_TEXT_WELCOME_LAUNCH_JAIKU__NOSHORTCUT ) );
	
	iSelections->AppendL( *text1 );
#endif
}

CFinalQuery::~CFinalQuery()
{
	CALLSTACKITEM_N(_CL("CFinalQuery"), _CL("~CFinalQuery"));
	delete iSelections;
}


void CFinalQuery::StartBackgroundActivitiesL()
{
}	
void CFinalQuery::ShowUiL(MWelcomeView& aView)
{
	CALLSTACKITEM_N(_CL("CFinalQuery"), _CL("QueryL"));
	iView = &aView;
	iState = EQueryFinalAction;

#ifdef ADD_SHORTCUT_QUERY	
	auto_ptr<HBufC> text( StringLoader::LoadL( R_TEXT_WELCOME_SETUP_COMPLETED_TEXT ) );
	aView.SetPageL( CWelcomeSelectionPage::NewL( aView.ObjectProviderL(),
												 *text, *iSelections, *this, CWelcomePageBase::EHeaderText_Login, ETrue) );
#else
	auto_ptr<HBufC> text( StringLoader::LoadL( R_TEXT_WELCOME_SETUP_COMPLETED_TEXT__NOSHORTCUT ) );
	_LIT( KSoftkey, "Start Jaiku");
	aView.SetPageL( CWelcomeInfoPage::NewL( aView.ObjectProviderL(),
											*text, *this, CWelcomePageBase::EHeaderText_Login, KSoftkey, ETrue) );
	
#endif
}


void CFinalQuery::AddShortcutL()
{
	iState = EShowShortcutHelp;
	TBool result = ShowOkCancelMessageL( R_TEXT_WELCOME_ADD_JAIKU_TEXT, R_TEXT_WELCOME_ADD_JAIKU_TITLE );
	if (result)
		{
			iState = ELaunchingSettings;
			
			TUint major = 0;
			TUint minor = 0;
			
			TUid applicationUid;
			TUid viewUid; 
			
			GetS60PlatformVersionL( Fs(), major, minor );
			
			if ( major >= 3 && minor >= 1 )
				{
					applicationUid = TUid::Uid( 0x100058EC );
					viewUid = TUid::Uid( 1 );
				}
			else
				{									
					applicationUid = TUid::Uid( 0x100058EC );
					viewUid = TUid::Uid( 4 );
				}
			
			CAknViewAppUi* viewAppUi = static_cast<CAknViewAppUi*>(CEikonEnv::Static()->AppUi());
			viewAppUi->ActivateViewL( TVwsViewId( applicationUid, viewUid ) );
			NotifyActionReadyL();
		}
	else
		{
			iState = EQueryFinalAction;
		}
}					

void CFinalQuery::LaunchJaikuL()
{
	iState = ELaunchingJaiku;
	ProcessManagement::StartApplicationL(KUidContextContacts);
	NotifyActionReadyL();
}


void CFinalQuery::ProcessSelectionL( TInt aIndex )
{
	CALLSTACKITEM_N(_CL("CFinalQuery"), _CL("ProcessSelectionL"));
	if ( iState == EQueryFinalAction )
		{
			TBool launchJaiku = ETrue;
#ifdef ADD_SHORTCUT_QUERY
			launchJaiku = EFalse;
			if ( aIndex == 1 )
				{
					iState = EShowShortcutHelp;
					TBool result = ShowOkCancelMessageL( R_TEXT_WELCOME_JAIKU_IS_READY, R_TEXT_WELCOME_JAIKU_IS_READY_TITLE );
					if ( result )
						{
							launchJaiku = ETrue;
						}
					else
						{
							iState = EQueryFinalAction;
							return;
						}
				}
			
#else
			launchJaiku = ETrue;
#endif
			if ( launchJaiku )
				{	
					LaunchJaikuL();
				}
			else
				{
					AddShortcutL();
				}
		}
	else if ( iState == EShowShortcutHelp ) 
		{
		}
}


void CFinalQuery::SelectedItemL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CFinalQuery"), _CL("SelectedItemL"));
	ProcessSelectionL( aIndex );
}


void CFinalQuery::LeftSoftKeyL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CFinalQuery"), _CL("LeftSoftKeyL"));
	ProcessSelectionL( aIndex );
}

void CFinalQuery::RightSoftKeyL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CFinalQuery"), _CL("RightSoftKeyL"));
}





///                              ///
//   CBluetoothQuery      ///
//                               ///


class CBluetoothQuery : public CWelcomeSelectionAction
{
public: // from CWelcomeSelectionAction
	
	void ProcessSelectionL( TInt aIndex )
	{
		CALLSTACKITEM_N(_CL("CBluetoothQuery"), _CL("ProcessSelectionL"));
		TInt scanInterval = aIndex == 0 ? 300 : 0; 
		Settings().WriteSettingL(SETTING_BT_SCAN_INTERVAL, scanInterval );
		NotifyActionReadyL();
	}
	
public:	
	static const TInt KSelectionIds[3];
	static CBluetoothQuery*  NewL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CBluetoothQuery"), _CL("NewL"));
		auto_ptr<CBluetoothQuery> self( new (ELeave) CBluetoothQuery() );
		

		self->ConstructL( KSelectionIds );
		return self.release();
	}
	CBluetoothQuery() : CWelcomeSelectionAction( CWelcomePageBase::EHeaderText_Bluetooth, R_TEXT_WELCOME_BLUETOOTH_QUERY_TEXT ) {}
	
	virtual ~CBluetoothQuery() {}	
};
	
const TInt CBluetoothQuery::KSelectionIds[] =
			{
				R_TEXT_WELCOME_BLUETOOTH_QUERY_YES,
				R_TEXT_WELCOME_BLUETOOTH_QUERY_NO,
				-1
			};	

///                              ///
//   CNetworkAccess              ///
//                               ///




class CNetworkAccess : public CWelcomeSelectionAction 
{
public: // from CWelcomeSelectionAction
	virtual void ProcessSelectionL( TInt aIndex ) 
	{
		CALLSTACKITEM_N(_CL("CNetworkAccess"), _CL("ProcessSelectionL"));
		if ( aIndex == 0 )
			{
				Settings().WriteSettingL(SETTING_ACCEPTED_NETWORK_ACCESS, ETrue);
				Settings().WriteSettingL(SETTING_ALLOW_NETWORK_ONCE, ETrue);
				Settings().WriteSettingL(SETTING_PRESENCE_ENABLE, ETrue);
				NotifyActionReadyL(); 
			}
		else
			{
				TBool yes = ShowYesNoMessageL( R_TEXT_WELCOME_CONFIRM_NO_NETWORK_ACCESS, R_TEXT_WELCOME_CONFIRM_NO_NETWORK_ACCESS_TITLE );
				if ( yes )
					{
						Settings().WriteSettingL(SETTING_ACCEPTED_NETWORK_ACCESS, EFalse);
						Settings().WriteSettingL(SETTING_ALLOW_NETWORK_ONCE, EFalse);
						Settings().WriteSettingL(SETTING_ALLOW_NETWORK_ACCESS, EFalse);
						ProcessManagement::SetAutoStartEnabledL(Fs(), DataDir()[0], EFalse);
						NotifyQuitWelcomeL();
					}
				else 
					{
						// no-op. returns to selection view
					}
			}
	}
	
public:	
	static const TInt KSelectionIds[3];
	static CNetworkAccess*  NewL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CNetworkAccess"), _CL("NewL"));
		auto_ptr<CNetworkAccess> self( new (ELeave) CNetworkAccess );

		self->ConstructL(KSelectionIds);
		return self.release();
	}

	CNetworkAccess() : CWelcomeSelectionAction(CWelcomePageBase::EHeaderText_Internet, R_TEXT_CONNECT_TO_NETWORK )
	{
	}

	~CNetworkAccess()
	{
		CALLSTACKITEM_N(_CL("CNetworkAccess"), _CL("~CNetworkAccess"));
	} 
};
	
const TInt CNetworkAccess::KSelectionIds[] =
			{
				R_TEXT_YES,
				R_TEXT_NO,
				-1
			};	




class CAutoStartup : public CWelcomeSelectionAction 
{
public: // from CWelcomeSelectionAction
	virtual void ProcessSelectionL( TInt aIndex ) 
	{
		CALLSTACKITEM_N(_CL("CAutoStartup"), _CL("ProcessSelectionL"));
		TBool enable = aIndex == 0;
		ProcessManagement::SetAutoStartEnabledL(Fs(), DataDir()[0], enable);
		NotifyActionReadyL(); 
	}
	
public:	
	static const TInt KSelectionIds[3];
	static CAutoStartup*  NewL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CAutoStartup"), _CL("NewL"));
		auto_ptr<CAutoStartup> self( new (ELeave) CAutoStartup );

		self->ConstructL(KSelectionIds);
		return self.release();
	}

	CAutoStartup() : CWelcomeSelectionAction(CWelcomePageBase::EHeaderText_AutoStart, R_TEXT_LAUNCH_JAIKU_ON_STARTUP)
	{
	}

	~CAutoStartup()
	{
		CALLSTACKITEM_N(_CL("CAutoStartup"), _CL("~CAutoStartup"));
	} 
};
	
const TInt CAutoStartup::KSelectionIds[] =
			{
				R_TEXT_YES,
				R_TEXT_NO,
				-1
			};	







class CPrivacyStatement
	: public CWelcomeAction
{
public: // from CWelcomeAction
	void StartBackgroundActivitiesL()
	{
		CALLSTACKITEM_N(_CL("CPrivacyStatement"), _CL("StartBackgroundActivitiesL"));
	}

	void ShowUiL(MWelcomeView& aView) {
		CALLSTACKITEM_N(_CL("CPrivacyStatement"), _CL("ShowUiL"));
		iAsync->TriggerAsync();
	}
	
	void RunAsync() {
		CALLSTACKITEM_N(_CL("CPrivacyStatement"), _CL("RunL"));

		TInt acceptedPrivacyVersion = 0;		
		Settings().GetSettingL(SETTING_ACCEPTED_PRIVACY_STMT_VERSION, acceptedPrivacyVersion);
		
		if ( acceptedPrivacyVersion < KJaikuPrivacyStatementVersion )
			{
				TPtrC text(*iText);
				TPtrC header(*iHeader);
				auto_ptr<CAknMessageQueryDialog> dlg( CAknMessageQueryDialog::NewL(text) );
				dlg->SetHeaderTextL( header );
				iDlg = dlg.get();
				TBool result = dlg.release()->ExecuteLD( R_PRIVACYSTATEMENT_DIALOG );
			  if (!iDlg) {
			    iAsync->TriggerAsync();
			    return;
			  }
				if (result)
					{
					  iDlg = NULL;
						Settings().WriteSettingL(SETTING_ACCEPTED_PRIVACY_STMT_VERSION, KJaikuPrivacyStatementVersion);
						NotifyActionReadyL();
					}
				else
					{
					  iDlg = NULL;
						// do not write, keep old accepted version 
						ProcessManagement::SetAutoStartEnabledL(Fs(), DataDir()[0], EFalse);
						NotifyQuitWelcomeL();
					}
			}
		else
			{
				NotifyActionReadyL();
			}
	}
  virtual void HandleOrientationChangeL() {
    if (iDlg) {
      CEikDialog* to_delete = iDlg;
      iDlg = NULL;
      delete to_delete;
    }
  }


public:	
	static CPrivacyStatement*  NewL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CPrivacyStatement"), _CL("NewL"));
		auto_ptr<CPrivacyStatement> self( new (ELeave) CPrivacyStatement);
		self->ConstructL();
		return self.release();
	}

	~CPrivacyStatement()
	{
		CALLSTACKITEMSTATIC_N(_CL("CPrivacyStatement"), _CL("~CPrivacyStatement"));
		delete iText;
		delete iHeader;
		delete iAsync;
	}
	
private:
  class CTrigger : public CActive {
   public:
    CTrigger(CPrivacyStatement* statement) : CActive(CActive::EPriorityLow), iStatement(statement) {
      CActiveScheduler::Add(this);
    }
    void RunL() {
      iStatement->RunAsync();
    }
  	void DoCancel() {}
  	void TriggerAsync() {
  	  TRequestStatus* s = &iStatus;
  	  *s = KRequestPending;
  	  User::RequestComplete(s, KErrNone);
  	  SetActive();
  	}
  	
   private:
    CPrivacyStatement* iStatement;
  };
	CPrivacyStatement()
	{
	}
	void ConstructL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CPrivacyStatement"), _CL("ConstructL"));
		iHeader = StringLoader::LoadL( R_TEXT_PRIVACY_STATEMENT_HEADER );
		iText = StringLoader::LoadL( R_TEXT_PRIVACY_STATEMENT );
		iAsync = new (ELeave) CTrigger(this);
	}	
	
		
private:
	HBufC* iHeader;
	HBufC* iText;
	CEikDialog* iDlg;
	CTrigger* iAsync;
};




///                              ///
//   CBatteryQuery      ///
//                               ///


class CBatteryQuery : public CWelcomeSelectionAction
{
public: // from CWelcomeSelectionAction
	
	void ProcessSelectionL( TInt aIndex )
	{
		CALLSTACKITEM_N(_CL("CBatteryQuery"), _CL("ProcessSelectionL"));
		
		Settings().WriteSettingL(SETTING_CONNECTIVITY_MODEL, aIndex);

		NotifyActionReadyL();
	}
	
public:	
	static const TInt KSelectionIds[3];
	static CBatteryQuery*  NewL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CBatteryQuery"), _CL("NewL"));
		auto_ptr<CBatteryQuery> self( new (ELeave) CBatteryQuery() );
		self->ConstructL( KSelectionIds );
		return self.release();
	}
	CBatteryQuery() : CWelcomeSelectionAction( CWelcomePageBase::EHeaderText_BatteryUsage, 
		R_TEXT_WELCOME_BATTERY_QUERY_TEXT ) {}
	
	virtual ~CBatteryQuery() {}	
};
	
const TInt CBatteryQuery::KSelectionIds[] =
			{
				R_TEXT_WELCOME_BATTERY_QUERY_GOOD_BATTERY,
				R_TEXT_WELCOME_BATTERY_QUERY_GOOD_PERFORMANCE,
				-1
			};	





// FACTORY METHODS
CWelcomeAction* CreatePrivacyStatementActionL()
{
	return CPrivacyStatement::NewL();
}

CWelcomeAction* CreateCalendarSharingActionL()
{
	return CCalendarSettingsQuery::NewL();
}


CWelcomeAction* CreateBluetoothQueryActionL()
{
	return CBluetoothQuery::NewL();
}


CWelcomeAction* CreateNickAndPasswordActionL()
{
	return CNickAndPasswordQuery::NewL();
}


CWelcomeAction* CreateIntroPageL()
{	
	auto_ptr<HBufC> text( StringLoader::LoadL( R_TEXT_WELCOME_INTRO ) );
	return CIntroAction::NewL(*text);
}

CWelcomeAction* CreateFinalPageL()
{	

	return CFinalQuery::NewL();
}

CWelcomeAction* CreateNetworkAccessActionL()
{	
	return CNetworkAccess::NewL();
}

CWelcomeAction* CreateAutoStartupActionL()
{	
	return CAutoStartup::NewL();
}

CWelcomeAction* CreateBatteryQueryActionL()
{	
	return CBatteryQuery::NewL();
}







