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

#include  "ContextContacts.hrh"
#include  <ContextContacts.rsg>
#include  "ContextContactsView.h"
#include  "ContextContactsContainer.h" 
#include  "ContextContactsTabGroups.h"
#include  "ContextContactsAppUi.h"
#include  "ccn_connectioninfo.h"

#include "ccu_activestate.h"
#include "ccu_mainbgcontainer.h"
#include "ccu_phonebookui.h"
#include "ccu_utils.h"
#include "cc_stringtools.h"
#include "reporting.h"
#include "cu_queries.h"

#include  <aknviewappui.h>
#include  <avkon.hrh>
#include <aknquerydialog.h> 
#include <RPbkViewResourceFile.h>
#include <CPbkContactEditorDlg.h>
#include <CPbkContactEngine.h>
#include <CPbkSelectFieldDlg.h>
#include <CPbkContactItem.h>
#include <cpbkfieldinfo.h> 
#include "nickform.h"
#include <eikenv.h>
#include <akniconarray.h>
#include <eikmenup.h>
#include <aknutils.h>

#ifndef __S60V3__
//FIXME3RD
#include <sendnorm.rsg>
#else
#include <cmessagedata.h>
#endif
#include <senduimtmuids.h>

#include <aknnotewrappers.h> 
#include "cl_settings.h"
#include "md5.h"

#include <cpbksmsaddressselect.h> 
#include <cpbkmmsaddressselect.h> 
#include <cpbkemailaddressselect.h> 
#include <cpbkphonenumberselect.h> 
#include <aknmessagequerydialog.h> 

#include "cu_cellnaming.h"
#include "cu_common.h"
#include "contextcommon.h"
#include "cbbsession.h"
#include "ccu_contactmatcher.h"
#include "break.h"
#include "reporting.h"
#include "app_context_impl.h"
#include "cc_processmanagement.h"
#include "ccn_message.h"
#include "ccn_expandingnote.h"
#include "ccu_mviewnavigation.h"


void CContextContactsView::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("ConstructL"));

	BaseConstructL( R_CONTEXTCONTACTS_VIEW_CONTACTS );
	iCaptionUnmark = CEikonEnv::Static()->AllocReadResourceL(R_UNMARK);
	iBtAndIr=new (ELeave) CArrayFixFlat< TUid >(2);
	iBtAndIr->AppendL(KSenduiMtmIrUid);
	iBtAndIr->AppendL(KSenduiMtmBtUid);
#ifndef __S60V3__
	iSendAppUi = CSendAppUi::NewL(ESendCmdId);
#else
	iSendAppUi = CSendUi::NewL();
#endif
	
}

CContextContactsView::~CContextContactsView()
{
	CC_TRAPD(err, ReleaseViewImpl());
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
}

void CContextContactsView::ReleaseViewImpl()
{
	CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("ReleaseImplView"));
	RemoveContainer();
	delete iCaptionUnmark;
	delete iSendAppUi;
	delete iBtAndIr;

	delete iNetworkError;
}

TUid CContextContactsView::Id() const {
	return KViewId;
}

void CContextContactsView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("HandleCommandL"));

	if ( CJaikuViewBase::PreHandleCommandL( aCommand ) )
		return;

	Reporting().DebugLog( _L("CContextContactsView::HandleCommandL"), aCommand);
	
	// dummy contacts
	TInt selectionCount = iContactContainer->SelectedItemsCountL();
	// Do not perform dummy contact behavior when multiple contacts marked )
	if ( selectionCount <= 1 )
		{
			switch ( aCommand )
				{
				case EContextContactsCmdEdit:
				case EContextContactsCmdDuplicate:
				case EContextContactsCmdInvite:
#ifndef __S60V3__
				case EContextContactsCmdCreateSms:
				case EContextContactsCmdCreateMms:
				case EContextContactsCmdCreateEmail:
#else
				case EContextContactsMenuMsg:
#endif
				case EContextContactsCmdCall:
				case EContextContactsCmdAddJabber:
				case EContextContactsCmdEditJabber:
					if ( DummyContactBehaviorL( aCommand ) )
						return;
				};
		}

	switch ( aCommand )
        {
		case EAknSoftkeyBack:
			{
				ResetListFocusL();
				AppUi()->HandleCommandL(EAknSoftkeyBack);
				break;
			}
		case EContextContactsCmdMyRichPresence:
			{
				iContactContainer->ShowMyRichPresenceL();
				break;
			}
		case EContextContactsCmdPresenceDetails:
			{
				iContactContainer->show_presence_details_current();
				break;
			}
		case EContextContactsCmdPresenceDescription:
			{
				iContactContainer->show_presence_description_current();
				break;
			}
		case EContextContactsCmdNew:
			{
				aPhoneHelper.show_editor(-1 /* = new*/, false /*no duplicate*/);
				break;
			}
		case EContextContactsCmdEdit:
			{
				TContactItemId id = Phonebook().GetContactId(iContactContainer->get_current_idx());
				aPhoneHelper.show_editor(id, false);
				break;
			}
		case EContextContactsCmdDuplicate:
			{
				aPhoneHelper.show_editor(Phonebook().GetContactId(iContactContainer->get_current_idx()),  true);			
				break;
			}
		case EContextContactsCmdDelete:
		{
			TryDeleteContactL();
			break;
		}
		case EcontextContactsCmdAppNameCell:
		case EcontextContactsCmdAppNameCity:
		case EcontextContactsCmdAppSuspendPresence:
		case EcontextContactsCmdAppResumePresence:
		case EContextContactsCmdInvite:
			{
				AppUi()->HandleCommandL( aCommand );
			}
			break;
		case EContextContactsCmdCreateSms:
			{
				CreateSmsL();
				break;
			}
		case EContextContactsCmdCreateMms:
			{
				CreateMmsL();
				break;
			}
		case EContextContactsCmdCreateEmail:
			{
				CreateEmailL();
				break;
			}
		case EContextContactsCmdCall:
			{
// 				iContactContainer->CompareSelectedIndexesL();
				AppUi()->HandleCommandL(aCommand);
				break;
			}
		case EContextContactsCmdAddJabber:
		case EContextContactsCmdEditJabber:
		case EContextContactsCmdShowJabber:
		case EContextContactsCmdSetUserDesc:
		case EContextContactsCmdSettings:
			AppUi()->HandleCommandL(aCommand);
			break;

		case EAknCmdUnmark:
		case EAknCmdMark:
		case EContextContactsCmdMark:
		{
			iContactContainer->MarkCurrentItemL();
			return;
			break;
		}

		case EAknUnmarkAll:
		case EContextContactsCmdUnmarkAll:
		{
			iContactContainer->UnmarkAll();
			return;
			break;
		}

		case EAknMarkAll:
		case EContextContactsCmdMarkAll:
		{
			iContactContainer->MarkAllL();
			return;
			break;
		}

#ifdef __S60V3__
    	case ESendCmdId: {
    		SendVCardL(aCommand);
    	}
    	break;
		case EContextContactsMenuMsg: {
			CreateMessageL(aCommand);
		}
	    break;
#endif
		default:
			{
						
#ifndef __S60V3__
						if (!iSendAppUi->CommandIsValidL(aCommand, TSendingCapabilities(0, 0, TSendingCapabilities::EAllMTMs)))
#endif
							{
								AppUi()->HandleCommandL( aCommand );
							}
#ifndef __S60V3__
						else
							{
								if (iWhichSendUi==EVCard) SendVCardL(aCommand);
								else CreateMessageL(aCommand);
							}
#endif
						break;
			}
		}
	iContactContainer->UnmarkAll();
}


void CContextContactsView::HandleClientRectChange()
{
	CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("HandleClientRectChange"));

	
	if ( iContactContainer )
        {
		iContactContainer->SetRect( ClientRect() );
        }
}

// TODO: localization
_LIT(KOffline, "Offline: ");
_LIT(KJaiku, "Jaiku: ");

void CContextContactsView::SettingChanged(TInt  )
{
	TBuf<50> msg;
	TBool prev=iAuthenticationError;
	Settings().GetSettingL(SETTING_IDENTIFICATION_ERROR, msg);
	auto_ptr<HBufC> t(0);
	if (msg.Length()==0) {
		iAuthenticationError=EFalse;
		t.reset( CEikonEnv::Static()->AllocReadResourceL(R_CONTACTS_TITLE) );
	} else {
		iAuthenticationError=ETrue;
		t.reset( HBufC::NewL(KOffline().Length()+msg.Length()));
		t->Des().Append(KOffline);
		t->Des().Append(msg);
	}
	// 	StatusPaneUtils::SetTitlePaneTextL( *t );
	if (iAuthenticationError && !prev) {
	    t->Des().Zero();
		t->Des().Zero();
		t->Des().Append(KJaiku);
		t->Des().Append(msg);
		auto_ptr<CAknGlobalNote> note(CAknGlobalNote::NewL());
		note->ShowNoteL(EAknGlobalErrorNote, *t);
	}
}

void CContextContactsView::NetworkError(TErrorDisplay aMode, const TDesC& aMessage)
{
	if (iAuthenticationError) return;
	
	auto_ptr<HBufC> t(0);
	t.reset( HBufC::NewL(KOffline().Length()+aMessage.Length()));
	t->Des().Append(KOffline);
	t->Des().Append(aMessage);
	//StatusPaneUtils::SetTitlePaneTextL( *t );
	
	if (aMode==EIntrusive) {
		t->Des().Zero();
		t->Des().Append(KJaiku);
		t->Des().Append(aMessage);
		auto_ptr<CAknGlobalNote> note(CAknGlobalNote::NewL());
		note->ShowNoteL(EAknGlobalErrorNote, *t);
	}
}

void CContextContactsView::NetworkSuccess()
{
	SettingChanged(SETTING_IDENTIFICATION_ERROR);
}

void CContextContactsView::RealDoActivateL(const TVwsViewId&, TUid, const TDesC8& )
{
	CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("ReakDoActivateL"));
	// Active state: there is no active feed item in contact list
	ActiveState().ActiveItem().ClearL();
	
	Settings().NotifyOnChange(SETTING_IDENTIFICATION_ERROR, this);
	SettingChanged(SETTING_IDENTIFICATION_ERROR);
	StatusPaneUtils::SetContextPaneIconToDefaultL();
	if (!iNetworkError) {
		iNetworkError=CNetworkError::NewL(*this);
	}
	
	mailbox_defined = aPhoneHelper.mailbox_defined();
	exiting=true;
	if (aLog) 
	{
		aLog->write_time();
		aLog->write_to_output(_L("Showing contact list"));
		aLog->write_nl();
	}

	CreateContainerL();
	StatusPaneUtils::SetTitlePaneTextL(_L("Jaiku"));

	exiting=false;
}

void CContextContactsView::CreateContainerL()
{
	// Container 
	if (!iBgContainer)
        {
			iBgContainer = CMainBgContainer::NewL( this, ClientRect(), ThemeColors(), ProgressBarModel() );
		}
	if (!iContactContainer)
		{
			TContactUiDelegates d;
			d.iJabberData = &(JabberData());
			d.iUserPics = &(UserPics());
			d.iPhonebook = &(Phonebook());
			d.iFeedStorage = &(FeedStorage());
			d.iStreamStats = &(StreamStats());
			d.iThemeColors = &(ThemeColors());
			d.iPeriodFormatter = &(PeriodFormatter());
			iContactContainer = new (ELeave) CContextContactsContainer( d );
			iContactContainer->ConstructL( iBgContainer, aLog, this);
		}
	iBgContainer->SetContentL( iContactContainer );
	iContactContainer->set_current_idx(current_contact_idx);
	iContactContainer->set_current_top_idx(current_contact_top_idx);
	
	iBgContainer->MakeVisible( ETrue );
	iBgContainer->ActivateL();
	AppUi()->AddToStackL( *this, iBgContainer );
}

void CContextContactsView::RemoveContainer()
{
	if ( iContactContainer )
        {
			if (!exiting)
				{
					current_contact_idx = iContactContainer->get_current_idx();
					current_contact_top_idx = iContactContainer->get_current_top_idx();
				}
			delete iContactContainer;
			iContactContainer=0;
		}
	
	if ( iBgContainer )
		{
			AppUi()->RemoveFromViewStack( *this, iBgContainer );
			iBgContainer->MakeVisible(EFalse);
			delete iBgContainer;
			iBgContainer=0;
        }
}

void CContextContactsView::RealDoDeactivateL()
{
	//CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("DoDeactivate"));
	Settings().CancelNotifyOnChange(SETTING_IDENTIFICATION_ERROR, this);
	Reporting().DebugLog( _L("CContextContactsView::DoDeactivate"));
	 
	RemoveContainer();
	delete iNetworkError; iNetworkError=0;
}

void CContextContactsView::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("DynInitMenuPaneL"));

	if ( iContactContainer )
		iContactContainer->StoreCurrentContactIdL();

 	TBool failedToConstruct = ! iContactContainer;
	CommonMenus().DynInitMenuPaneL(aResourceId, aMenuPane, failedToConstruct);
	if ( failedToConstruct )
		return;

	TBool hasContacts = iContactContainer->CountVisibleItems() > 0;
	TBool hasSelections = EFalse;
	TBool hasPresence = EFalse; 
	TBool isJaikuContact = EFalse;
	TBool isMyself = EFalse;
	TBool isDummy = EFalse;
	contact* c = NULL;

	// Fill predicate values 
	TInt selectionCount = iContactContainer->SelectedItemsCountL();
	hasSelections = selectionCount > 0;

	if ( hasContacts )
		{
			
			if ( ! hasSelections )
				{
					c = Phonebook().GetContact(iContactContainer->get_current_idx());
					
					// Note: myself is also jaiku contact. i.e isMyself => isJaikuContact
					hasPresence = c && c->presence;
					isJaikuContact = EFalse;
					isMyself = EFalse;
					isDummy = JabberData().IsDummyContactId( c->id );
					TBuf<100> focusedNick; 
					JabberData().GetJabberNickL(c->id, focusedNick);
					isJaikuContact = focusedNick.Length() > 0;
					isMyself = JabberData().IsUserNickL( focusedNick );
				}
		}
	

	// Show and hide items 

	if (aResourceId == R_JAIKU_CONTACTS_LIST_MENU) 
		{		
		// No contacts
		if ( ! hasContacts ) 
		    {
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdSetUserDesc, ETrue);
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdOpen, ETrue);
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdCall, ETrue);
			SetItemDimmedIfExists(aMenuPane, EContextContactsMenuMsg, ETrue);
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdEdit, ETrue);
			SetItemDimmedIfExists(aMenuPane, EContextContactsCmdDelete, ETrue);
			SetItemDimmedIfExists(aMenuPane, EContextContactsMenuMark, ETrue);
			}
		else
		    {
		    // No selections
				if ( ! hasSelections )
					{												
						{
							TInt position = aMenuPane->NumberOfItemsInPane() - 0;
#ifndef __S60V3__
							iSendAppUi->DisplaySendMenuItemL(*aMenuPane, position, 
															 TSendingCapabilities(0, 0, TSendingCapabilities::ESupportsAttachmentsOrBodyText ));
#else
							iSendAppUi->AddSendMenuItemL(*aMenuPane, position, ESendCmdId, 
														 TSendingCapabilities(0, 0, TSendingCapabilities::EAllServices));
							
#endif
						}
					}
				else
					{
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdSetUserDesc, ETrue);
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdOpen, ETrue);
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdCall, ETrue);
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdNew, ETrue);
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdDelete, EFalse);
						SetItemDimmedIfExists(aMenuPane, EContextContactsMenuMsg, EFalse);
						SetItemDimmedIfExists(aMenuPane, EContextContactsMenuMark, EFalse);
				
#ifndef __S60V3__
						TInt flags = TSendingCapabilities::ESupportsAttachmentsOrBodyText;
#else
						TInt flags = TSendingCapabilities::EAllServices;
#endif
						if ( selectionCount > 1 )
							{
								// can't send more than one v-card with sms
								flags = TSendingCapabilities::ESupportsAttachments;
							}
						TInt position = aMenuPane->NumberOfItemsInPane() - 0;
						position = position < 0 ? 0 : position;
#ifndef __S60V3__
						iSendAppUi->DisplaySendMenuItemL(*aMenuPane, position, 
														 TSendingCapabilities(0, 0, flags));
#else						
						iSendAppUi->AddSendMenuItemL(*aMenuPane, position, ESendCmdId,
														 TSendingCapabilities(0, 0, flags));
#endif
					}
			}
		}
	
#ifndef __S60V3__
	if (aResourceId == R_MESSAGE_MENU)
		{
			iSendAppUi->DisplaySendCascadeMenuL(*aMenuPane, iBtAndIr);
			iWhichSendUi=ECreateMessage;
		}
#endif
	
	if (aResourceId == R_AVKON_MENUPANE_MARKABLE_LIST_IMPLEMENTATION)
		{		   
			TBool currentMarked = iContactContainer->IsCurrentMarked();
			SetItemDimmedIfExists( aMenuPane, EAknCmdMark, currentMarked );
			SetItemDimmedIfExists( aMenuPane, EAknCmdUnmark, ! currentMarked );

			TBool allVisibleMarked = iContactContainer->CountVisibleItems() == selectionCount;
			SetItemDimmedIfExists( aMenuPane, EAknMarkAll, allVisibleMarked );

			SetItemDimmedIfExists( aMenuPane, EAknUnmarkAll, ! hasSelections );
		}
	
#ifndef __S60V3__
	if (aResourceId == R_SENDUI_MENU)
		{
			iSendAppUi->DisplaySendCascadeMenuL(*aMenuPane);
			iWhichSendUi=EVCard;
		}
#endif
	}

TInt CContextContactsView::GetCurrentContactId()
{
	CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("GetCurrentContactId"));

	return Phonebook().GetContactId(iContactContainer->get_current_idx());
}

void CContextContactsView::before_exit()
{
	Reporting().DebugLog( _L("CContextContactsView::before_exit"));
	exiting = true;
	if (iContactContainer) iContactContainer->BeforeAppExitL();
}



void CContextContactsView::ResetListFocusL() 
{
  CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("ResetListFocusL"));

  // Reset focus
  if ( iContactContainer )
	  {
		  iContactContainer->set_current_idx( 0 );
		  iContactContainer->set_current_top_idx( 0 );
		  current_contact_idx = iContactContainer->get_current_idx();
		  current_contact_top_idx = iContactContainer->get_current_top_idx();
		  
		  // Hide Search field
		  iContactContainer->ResetAndHideSearchField();
		  //   // Set focused and top item to first item
		  //   iContactContainer->DrawNow();
	  }
  else
	  {
		  current_contact_idx = KErrNotFound;
		  current_contact_top_idx = KErrNotFound;
	  }
}



void CContextContactsView::ShowNotSupportedForDummyMsgL()
{
	
}

TBool CContextContactsView::DummyContactBehaviorL(TInt aCommand)
{
	return static_cast<CContextContactsAppUi*>(AppUi())->DummyContactBehaviorL(aCommand);
}


void CContextContactsView::DeleteSingleContactUiL(TInt contact_id)
{
	// single delete:
	contact* c = Phonebook().GetContactById( contact_id );
	if ( c )
		{
			HBufC* message = HBufC::NewL(400);
			CleanupStack::PushL( message );

			TPtr ptr = message->Des();
			c->AppendName( ptr, Phonebook().ShowLastNameFirstL() );
			SafeAppend( ptr, _L("\nNote: Contact will be deleted from phone's Contacts also" ) );
			
			
			CAknMessageQueryDialog* dlg( CAknMessageQueryDialog::NewL(*message) );
			CleanupStack::PushL(dlg);
			dlg->PrepareLC(R_CONFIRMATION_QUERY);
			
			HBufC * header = CEikonEnv::Static()->AllocReadResourceLC(R_DELETE);
			dlg->QueryHeading()->SetTextL(*header);
			CleanupStack::PopAndDestroy(header); 
			CleanupStack::Pop(dlg);			
			if ( dlg->RunLD() )
				{
					if ( Queries::ShowDisposableConfirmationL(*this, SETTING_DONT_DOUBLE_CONFIRM_DELETES) )
						{
							DeleteContactL( contact_id );
							Phonebook().ReRead();
						}
				}
			CleanupStack::PopAndDestroy(message); 
		}
}

void CContextContactsView::DeleteMultipleContactsUiL(CArrayFix<TInt>& aSelection)
{
	//mass delete
	TBuf<100> message;
	message.AppendFormat(_L("%d contacts"), aSelection.Count() );
	SafeAppend( message, _L("\nNote: Contacts will be deleted from phone's Contacts also" ) );

	
	CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(message);
	CleanupStack::PushL(dlg);
	dlg->PrepareLC(R_CONFIRMATION_QUERY);
	HBufC * header = CEikonEnv::Static()->AllocReadResourceLC(R_DELETE);
	dlg->QueryHeading()->SetTextL(*header);
	CleanupStack::PopAndDestroy(header); //header
	CleanupStack::Pop(dlg);
	
	if ( dlg->RunLD() )
		{
			if ( Queries::ShowDisposableConfirmationL(*this, SETTING_DONT_DOUBLE_CONFIRM_DELETES) )
				{
					for (int i=0; i< aSelection.Count(); i++)
						{
							TInt contact_id = Phonebook().GetContactId( aSelection.At(i) );
							DeleteContactL(contact_id);
						}
					Phonebook().ReRead();
				}
		}
}


void CContextContactsView::TryDeleteContactL() 
{
	auto_ptr< CArrayFix<TInt> > selection( iContactContainer->GetCopyOfSelectionIndexesL() );
	if (selection->Count() == 0)
		{
			TInt contact_id = Phonebook().GetContactId( iContactContainer->get_current_idx() );
			DeleteSingleContactUiL( contact_id );
		}
	else
		{
			DeleteMultipleContactsUiL( *selection );
		}
}


void CContextContactsView::CreateSmsL()
{
	TAddressSelectorF selector = SmsSelectorL;
	TMessageSenderF sender = &CMessaging::SmsSenderL;
	TPbkFieldFilterF filter = SmsFieldFilterL;
	TInt warningResource = R_CONTACTS_NO_SMS;
	HandleCreateMessageL( selector, sender, filter, warningResource, KSenduiMtmSmsUid );
}


void CContextContactsView::CreateMmsL()
{
	TAddressSelectorF selector = MmsSelectorL;
	TMessageSenderF sender = &CMessaging::MmsSenderL;
	TPbkFieldFilterF filter = MmsFieldFilterL;
	TInt warningResource = R_CONTACTS_NO_MMS;
	HandleCreateMessageL( selector, sender, filter, warningResource, KSenduiMtmMmsUid );
}


void CContextContactsView::CreateEmailL()
{
	TAddressSelectorF selector = EmailSelectorL;
	TMessageSenderF sender = &CMessaging::EmailSenderL;
	TPbkFieldFilterF filter = EmailFieldFilterL;
	TInt warningResource = R_CONTACTS_NO_EMAIL;
	HandleCreateMessageL( selector, sender, filter, warningResource, KSenduiMtmSmtpUid );
}


void CContextContactsView::HandleCreateMessageL( TAddressSelectorF aSelector, 
												 TMessageSenderF aSender, 
												 TPbkFieldFilterF aFilter,
												 TInt aWarningResource,
												 TUid aMtm)
{
	auto_ptr< CArrayFix<TInt> > selection( iContactContainer->GetCopyOfSelectionIndexesL() );
	if (selection->Count() == 0)
		{
			TInt contact_id = Phonebook().GetContactId(iContactContainer->get_current_idx());
			iMessaging->CreateSingleMessageL( contact_id, aSelector, aSender, aMtm );
		}
	else
		{
			iMessaging->CreateMassMessageL( *selection,
								aWarningResource,
								aSelector, aSender, aFilter, aMtm );
		}
}

void CContextContactsView::CreateMessageL(TInt 
#ifndef __S60V3__
										  aCommand
#endif
										  )
{
	TUid mtm=KNullUid;
#ifndef __S60V3__
	mtm = iSendAppUi->MtmForCommand(aCommand);
#else
	TInt flags = TSendingCapabilities::EAllServices;
	mtm = iSendAppUi->ShowSendQueryL(0, TSendingCapabilities(0, 0, flags), iBtAndIr, _L("Create Message"));
#endif
	if (mtm==KNullUid) {
		return;
	}
	TAddressSelectorF selector = SmsSelectorL;
	TMessageSenderF sender = &CMessaging::MessageSenderL;
	TPbkFieldFilterF filter = SmsFieldFilterL;
	TInt warningResource = R_CONTACTS_NO_SMS;

	if (mtm==KSenduiMtmSmtpUid) {
		selector=EmailSelectorL;
		filter=EmailFieldFilterL;
		warningResource=R_CONTACTS_NO_EMAIL;
	} else if (mtm!=KSenduiMtmSmsUid) {
		selector=MmsSelectorL;
		filter=MmsFieldFilterL;
		warningResource=R_CONTACTS_NO_MMS;
	}

	HandleCreateMessageL( selector, sender, filter, warningResource, mtm );

}
void CContextContactsView::SendVCardL(TInt
#ifndef __S60V3__
									  aCommand
#endif
									  )
{
	CALLSTACKITEM_N(_CL("CContextContactsView"), _CL("SendVCardL"));
	TUid mtm=KNullUid;
#ifndef __S60V3__
	mtm = iSendAppUi->MtmForCommand(aCommand);
#endif
	auto_ptr< CArrayFixFlat<TInt> > contactIds( new CArrayFixFlat<TInt>(1) );
	auto_ptr< CArrayFix<TInt> > selection( iContactContainer->GetCopyOfSelectionIndexesL() );

	auto_ptr<CExpandingNote> note( CExpandingNote::NewL(R_TEXT_NO_CONTACT_CARD) );

	if ( selection->Count() <= 0 )
		{
			// use current item
			TInt contact_id = Phonebook().GetContactId(iContactContainer->get_current_idx());
			
			if ( JabberData().IsDummyContactId( contact_id ) )
				{
					contact *c = Phonebook().GetContactById( contact_id );
					auto_ptr<HBufC> name( c->NameL( Phonebook().ShowLastNameFirstL() ) );
					note->AppendLineL( *name );
				}
			else
				{
					contactIds->AppendL(contact_id);
				}
		}
	else
		{
			// use marked selection
			for (int i =0; i< selection->Count();i++)
				{
					TInt contact_id = Phonebook().GetContactId( selection->At(i) );
					if ( JabberData().IsDummyContactId( contact_id ) )
						{
							contact *c = Phonebook().GetContactById( contact_id );
							auto_ptr<HBufC> name( c->NameL( Phonebook().ShowLastNameFirstL() ) );
							note->AppendLineL( *name );
						}
					else
						{
							contactIds->AppendL(contact_id);
						}
				}
		}

	if ( note->HasData() )
		{
			note->ShowWarningNoteL();
		}

	if ( contactIds->Count() > 0 )
		{
#ifdef __S60V3__
			TInt flags = TSendingCapabilities::EAllServices;
			if ( contactIds->Count()>1 ) {
				flags = TSendingCapabilities::ESupportsAttachments;
			}
			mtm = iSendAppUi->ShowSendQueryL(0, TSendingCapabilities(0, 0, flags), 0, _L("Send VCard"));
			
#endif
			if (mtm!=KNullUid) {
				aPhoneHelper.send_as( mtm, contactIds.get() );
			}
			iContactContainer->ResetSearchField();
		}
}



void CContextContactsView::DeleteContactL( TInt aId )
{	
	if ( aId == KNullContactId || aId == KErrNotFound )
		return;
	
	if ( JabberData().IsDummyContactId( aId ) )
		{
			CJabberData::TNick nick;
			JabberData().GetJabberNickL( aId, nick );
			JabberData().MarkNickAsRemovedL( nick );
		}
	else
		{			
			CJabberData::TNick nick;
			if ( JabberData().GetJabberNickL( aId, nick ) )
				{	
					JabberData().MarkNickAsRemovedL( nick );
				}
			Phonebook().get_engine()->DeleteContactL( aId );
 		}
}


void CContextContactsView::HandleResourceChange( TInt aType )
{
	if ( aType == KEikDynamicLayoutVariantSwitch )
		{
			if ( iBgContainer )
				{
					TRect r = ClientRect();
					iBgContainer->SetRect( r );
				}
		}
}

