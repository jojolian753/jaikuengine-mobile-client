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

#include  <aknviewappui.h>
#include  <avkon.hrh>
#include  <ContextCallLog.rsg>
#include  "ContextCallLogView.h"
#include  "ContextCallLogContainer.h"
#include "contextcalllog.hrh"
#include <aknmessagequerydialog.h> 
#include <cpbkmemoryentryadditemdlg.h> 
#include <cpbkfieldsinfo.h> 
#include <cpbkcontactitem.h>

#ifndef NO_CPBKSINGLEENTRYFETCHDLG_H
#include <cpbksingleentryfetchdlg.h>
#endif

#include "ContextCallLogAppUi.h"
#include "cbbsession.h"
#include "contextclientsession.h"
#include <CPbkPhoneNumberSelect.h>

/*
 * Concepts:
 * !Shared data client access!
 * !Modifying Contacts!
 */

/*
 * reduce dependacies on sendo -MR
#ifndef NO_SHAREDDATACLIENT_H
#include <sharedDataI.h>
#else
*/

// reverse-engineered from COMMONENGINE.LIB
class CSharedDataI : public CBase {
public:
        IMPORT_C virtual  ~CSharedDataI(void);
        IMPORT_C void  AddCallBackL(class TCallBack const &, class TDesC16 const &);
        IMPORT_C void  AddNotify(class TDesC16 const &);
        IMPORT_C int  AddToValue(class TDesC16 const &, int &);
        IMPORT_C void  CancelSignal(class TDesC16 const &);
        IMPORT_C int  Get(class TDesC16 const &, int &);
        IMPORT_C int  Get(class TDesC16 const &, double &);
        IMPORT_C int  Get(class TDesC16 const &, class TDes16 &);
        IMPORT_C static class CSharedDataI *  NewL(class TUid const &, int);
        IMPORT_C int  Set(class TDesC16 const &, class TDesC16 const &);
        IMPORT_C int  Set(class TDesC16 const &, int &);
        IMPORT_C int  Set(class TDesC16 const &, double &);
        IMPORT_C int  Signal(class TDesC16 const &);
};
//#endif

const TInt KUidNewMisCallDataValue=0x101f4cd5;
const TUid KUidNewMisCallData={KUidNewMisCallDataValue};

void CContextCallLogView::ConstructL(TUid viewId)
{
	iViewId = viewId;
	BaseConstructL( R_CONTEXTCALLLOG_VIEW1 );

	if (iViewId == KReceivedViewId)
	{
		iTitle=CEikonEnv::Static()->AllocReadResourceL(R_RECEIVED_TITLE);
		iEmptyListboxLabel=CEikonEnv::Static()->AllocReadResourceL(R_NO_RECEIVED_LABEL);
		iLoadingListboxLabel=CEikonEnv::Static()->AllocReadResourceL(R_LOADING_RECEIVED_LABEL);
	} 
	else if (iViewId == KDialledViewId)
	{
		iTitle=CEikonEnv::Static()->AllocReadResourceL(R_DIALLED_TITLE);
		iEmptyListboxLabel=CEikonEnv::Static()->AllocReadResourceL(R_NO_DIALLED_LABEL);
		iLoadingListboxLabel=CEikonEnv::Static()->AllocReadResourceL(R_LOADING_DIALLED_LABEL);
	}
	else if (iViewId == KMissedViewId) 
	{
		iTitle=CEikonEnv::Static()->AllocReadResourceL(R_MISSED_TITLE);
		iEmptyListboxLabel=CEikonEnv::Static()->AllocReadResourceL(R_NO_MISSED_LABEL);
		iLoadingListboxLabel=CEikonEnv::Static()->AllocReadResourceL(R_LOADING_MISSED_LABEL);
	}
	else if (iViewId == KLookupViewId) 
	{
		iTitle=CEikonEnv::Static()->AllocReadResourceL(R_LOOKUP_TITLE);
		iEmptyListboxLabel=CEikonEnv::Static()->AllocReadResourceL(R_NO_LOOKUP_LABEL);
		iLoadingListboxLabel=CEikonEnv::Static()->AllocReadResourceL(R_LOADING_LOOKUP_LABEL);
	}


}

CContextCallLogView::~CContextCallLogView()
{
	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	delete iTitle;
	delete iEmptyListboxLabel;
	delete iLoadingListboxLabel;
	delete iContainer;
	delete iSendAppUi;
}

TUid CContextCallLogView::Id() const
{
	return iViewId;
}


void CContextCallLogView::HandleCommandL(TInt aCommand)
{   
	switch ( aCommand )
	{
		case EAknSoftkeyBack:
		{
			AppUi()->HandleCommandL(EAknSoftkeyBack);
			break;
		}
		case EContextCallLogCmdCall:
		{
			if ( iViewId != KLookupViewId) {
				TPtrC no(aCallLog->get_phone_no(iContainer->get_current_idx()));
				if (no!=KNullDesC) aPhoneHelper.make_callL(no);
			} else {
				CPbkPhoneNumberSelect* sel=new (ELeave) CPbkPhoneNumberSelect();
				TInt contact_id = aCallLog->GetContactId(iContainer->get_current_idx());
				CPbkContactItem * item = aCallLog->get_engine()->ReadContactL(contact_id);
				CleanupStack::PushL(item);
				TPtrC no(sel->ExecuteLD(*item, NULL, ETrue /*if there's a default number!*/));
				CleanupStack::PopAndDestroy(item);
				aPhoneHelper.make_callL(no);
			}
			break;
		}
		case EContextCallLogCmdPresenceDetails:
		{
			iContainer->show_presence_details_current();
			break;
		}
		case EContextCallLogCmdCallDetails:
		{
			iContainer->show_call_details_current();
			break;
		}
		
		case EContextCallLogCmdCreateContact:
		{
			CArrayPtrFlat<CPbkFieldInfo> * infoArray = new (ELeave) CArrayPtrFlat<CPbkFieldInfo>(20);
			CleanupStack::PushL(infoArray);
			
			for (int i=0; i<aCallLog->get_engine()->FieldsInfo().Count(); i++)
			{
				if ( ((aCallLog->get_engine()->FieldsInfo())[i])->IsPhoneNumberField()  )
				{
					infoArray->AppendL((aCallLog->get_engine()->FieldsInfo())[i]);
				}
			}
			CPbkItemTypeSelectCreateNew * dlg = new (ELeave) CPbkItemTypeSelectCreateNew;
			CleanupStack::PushL(dlg);
			CPbkFieldInfo * fieldInfo = dlg->ExecuteLD(*infoArray) ;
			CleanupStack::Pop(dlg);

			if (fieldInfo != 0)
			{
				TPtrC no(aCallLog->get_phone_no(iContainer->get_current_idx()));
				aPhoneHelper.show_editor(-1, false, -1, fieldInfo, no);
			}
			
			CleanupStack::PopAndDestroy(infoArray);
			break;
		}

		case EContextCallLogCmdUpdateContact:
		{
#ifndef NO_CPBKSINGLEENTRYFETCHDLG_H
			CPbkSingleEntryFetchDlg::TParams p; // = CPbkSingleEntryFetchDlg::TParams();
			p.iPbkEngine = aCallLog->get_engine();
					
			CPbkSingleEntryFetchDlg * anotherDlg = CPbkSingleEntryFetchDlg::NewL(p);
			if (anotherDlg->ExecuteLD())
			{
				TInt contactId = p.iSelectedEntry;
			
				CArrayPtrFlat<CPbkFieldInfo> * infoArray = new (ELeave) CArrayPtrFlat<CPbkFieldInfo>(20);
				CleanupStack::PushL(infoArray);
			
				for (int i=0; i<aCallLog->get_engine()->FieldsInfo().Count(); i++)
				{
					if ( ((aCallLog->get_engine()->FieldsInfo())[i])->IsPhoneNumberField()  )
					{
						infoArray->AppendL((aCallLog->get_engine()->FieldsInfo())[i]);
					}
				}
			
				CPbkItemTypeSelectAddToExisting * dlg = new (ELeave) CPbkItemTypeSelectAddToExisting;
				CleanupStack::PushL(dlg);
				CPbkFieldInfo * fieldInfo = dlg->ExecuteLD(*infoArray) ;
				CleanupStack::Pop(dlg);

				if (fieldInfo != 0)
				{
					TPtrC no(aCallLog->get_phone_no(iContainer->get_current_idx()));
					aPhoneHelper.show_editor(contactId, false, -1, fieldInfo, no);
				}
			
				CleanupStack::PopAndDestroy(infoArray);
			}
#endif
			break;
		}

		case EContextCallLogCmdCreateSms:
		{
			TPtrC no(aCallLog->get_phone_no(iContainer->get_current_idx()));
			HBufC * name = aCallLog->GetContact( iContainer->get_current_idx() )->first_name;
				
			if (no!=KNullDesC)
			{
				CDesCArrayFlat* recip=new CDesCArrayFlat(1);
				CleanupStack::PushL(recip);
				CDesCArrayFlat* alias=new CDesCArrayFlat(1);
				CleanupStack::PushL(alias);
				
				recip->AppendL(no);
				alias->AppendL(*name);
				aPhoneHelper.send_sms(recip, alias);

				CleanupStack::PopAndDestroy(2); // recip, alias
			}
			break;
		}

		case EContextCallLogCmdCreateMms:
		{
			TPtrC no(aCallLog->get_phone_no(iContainer->get_current_idx()));

			HBufC * name = aCallLog->GetContact( iContainer->get_current_idx() )->first_name;
				
			if (no!=KNullDesC)
			{
				CDesCArrayFlat* recip=new CDesCArrayFlat(1);
				CleanupStack::PushL(recip);
				CDesCArrayFlat* alias=new CDesCArrayFlat(1);
				CleanupStack::PushL(alias);
				
				recip->AppendL(no);
				alias->AppendL(*name);
				aPhoneHelper.send_mms(recip, alias);

				CleanupStack::PopAndDestroy(2); // recip, alias
			}
			break;
		}
		case EContextCallLogCmdDelete:
		{
			HBufC * message = CEikonEnv::Static()->AllocReadResourceLC(R_DELETE_MESSAGE);
		        CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(*message);
			CleanupStack::PushL(dlg);
			dlg->PrepareLC(R_CONFIRMATION_QUERY);
			CleanupStack::Pop(dlg);
			if ( dlg->RunLD() )
			{
				aCallLog->DeleteEvent(iContainer->get_current_idx());
			}
			CleanupStack::PopAndDestroy(); //message
			break;
		}
		case EContextCallLogCmdUseNumber:
		{
			TPtrC no(aCallLog->get_phone_no(iContainer->get_current_idx()));

			if (no != KNullDesC)
			{
			
				HBufC * prompt = CEikonEnv::Static()->AllocReadResourceLC(R_NUMBER);
				const TInt NUMBER_LENGTH=100;
				HBufC* textData=HBufC::NewL(NUMBER_LENGTH);
				CleanupStack::PushL(textData);
				textData->Des().Append(no);
				TPtr16 p=textData->Des();
				CAknTextQueryDialog* dlg = new(ELeave) CAknTextQueryDialog(p, *prompt);
				CleanupStack::PushL(dlg);
				dlg->SetMaxLength(NUMBER_LENGTH);
				CleanupStack::Pop();
				if (dlg->ExecuteLD(R_CONTEXTCALLLOG_USENUMBER) && textData->Length())
				{
					aPhoneHelper.make_callL(no);
				}
				CleanupStack::PopAndDestroy(2); // prompt, textdata
			}
			break;
		}

		case EContextCallLogCmdClear:
		{
			HBufC * msg; 
			if (iViewId == KReceivedViewId) {
				msg = CEikonEnv::Static()->AllocReadResourceLC(R_CLEAR_RECEIVED);
			} else if (iViewId == KDialledViewId) {
				msg = CEikonEnv::Static()->AllocReadResourceLC(R_CLEAR_DIALLED);
			} else if (iViewId == KMissedViewId) {
				msg = CEikonEnv::Static()->AllocReadResourceLC(R_CLEAR_MISSED);
			} else {
				msg = CEikonEnv::Static()->AllocReadResourceLC(R_CLEAR_LOOKUP);
			}
		                               
			CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(*msg);
			CleanupStack::PushL(dlg);
			dlg->PrepareLC(R_CONFIRMATION_QUERY);
			CleanupStack::Pop(dlg);
			
			if ( dlg->RunLD() ) {
				aCallLog->ClearEventList();
			}
			CleanupStack::PopAndDestroy(); // msg
			break;
		}
		default:
		{
			AppUi()->HandleCommandL( aCommand );
			break;
		}
	}
}

void CContextCallLogView::HandleClientRectChange()
{
	if ( iContainer )
        {
		iContainer->SetRect( ClientRect() );
        }
}

void CContextCallLogView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
{
	if (aResourceId == R_CONTEXTCALLLOG_VIEW1_MENU )
	{
		if ( iContainer->get_current_idx() != -1 )
		{
			aMenuPane->SetItemDimmed(EContextCallLogCmdDelete, EFalse);
			aMenuPane->SetItemDimmed(EContextCallLogCmdClear, EFalse);

			if ( iViewId != KLookupViewId) {
				aMenuPane->SetItemDimmed(EContextCallLogCmdCallDetails, EFalse);
			} else {
				aMenuPane->SetItemDimmed(EContextCallLogCmdCallDetails, ETrue);
			}
			
			contact* c=aCallLog->GetContact(iContainer->get_current_idx());
			if ( c && c->id != -1 )
			{
				if ( iViewId != KLookupViewId) {
					aMenuPane->SetItemDimmed(EContextCallLogCmdUseNumber, EFalse);
				} else {
					aMenuPane->SetItemDimmed(EContextCallLogCmdUseNumber, ETrue);
				}
				aMenuPane->SetItemDimmed(EContextCallLogCmdCall, EFalse);
				aMenuPane->SetItemDimmed(EContextCallLogMenuMsg, EFalse);
				aMenuPane->SetItemDimmed(EContextCallLogMenuContact, EFalse);
							
				if ( aCallLog->GetContact(iContainer->get_current_idx())->presence == 0)
				{
					aMenuPane->SetItemDimmed(EContextCallLogCmdPresenceDetails, ETrue);
				}
				else
				{
					aMenuPane->SetItemDimmed(EContextCallLogCmdPresenceDetails, EFalse);
				}
			}
			else
			{
				aMenuPane->SetItemDimmed(EContextCallLogCmdCall, ETrue);
				aMenuPane->SetItemDimmed(EContextCallLogMenuMsg, ETrue);
				aMenuPane->SetItemDimmed(EContextCallLogMenuContact, ETrue);
				aMenuPane->SetItemDimmed(EContextCallLogCmdUseNumber, ETrue);
				aMenuPane->SetItemDimmed(EContextCallLogCmdPresenceDetails, ETrue);
			}
			
			
		}
		else
		{
			aMenuPane->SetItemDimmed(EContextCallLogCmdCallDetails, ETrue);
			aMenuPane->SetItemDimmed(EContextCallLogCmdCall, ETrue);
			aMenuPane->SetItemDimmed(EContextCallLogMenuMsg, ETrue);
			aMenuPane->SetItemDimmed(EContextCallLogCmdClear, ETrue);
			aMenuPane->SetItemDimmed(EContextCallLogCmdDelete, ETrue);
			aMenuPane->SetItemDimmed(EContextCallLogMenuContact, ETrue);
			aMenuPane->SetItemDimmed(EContextCallLogCmdUseNumber, ETrue);
			aMenuPane->SetItemDimmed(EContextCallLogCmdPresenceDetails, ETrue);
		}
	} else if (aResourceId == R_CONTEXTCALLLOG_MENUBAR_CLICK_ON_ITEM_MENU)
	{
		contact* c=aCallLog->GetContact(iContainer->get_current_idx());
		if ( !c || c->presence == 0)
		{
			aMenuPane->SetItemDimmed(EContextCallLogCmdPresenceDetails, ETrue);
		}
		else
		{
			aMenuPane->SetItemDimmed(EContextCallLogCmdPresenceDetails, EFalse);
		}
		if ( iViewId != KLookupViewId) {
			aMenuPane->SetItemDimmed(EContextCallLogCmdCallDetails, EFalse);
		} else {
			aMenuPane->SetItemDimmed(EContextCallLogCmdCallDetails, ETrue);
		}
	}
}


void CContextCallLogView::DoActivateL(
   const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/,
   const TDesC8& /*aCustomMessage*/)
{
	mailbox_defined = aPhoneHelper.mailbox_defined();
	
	if (iViewId == KReceivedViewId) {
		((CContextCallLogAppUi*)AppUi())->SetTab(EContextCallLogReceivedTab);
		aCallLog->SetFilter(call_log_i::EReceived);		
	} else if (iViewId == KDialledViewId) {
		((CContextCallLogAppUi*)AppUi())->SetTab(EContextCallLogDialledTab);
		aCallLog->SetFilter(call_log_i::EDialled);	
	} else if (iViewId == KMissedViewId) {
		((CContextCallLogAppUi*)AppUi())->SetTab(EContextCallLogMissedTab);
		aCallLog->SetFilter(call_log_i::EMissed);	

		_LIT(KNewMisCall, "NewMisCall");
		CSharedDataI * shData = CSharedDataI::NewL(KUidNewMisCallData, EFalse);
		CleanupStack::PushL(shData);
		TInt zero = 0;
		shData->Set(KNewMisCall, zero);

		CleanupStack::PopAndDestroy();
	} else if (iViewId == KLookupViewId) {
		((CContextCallLogAppUi*)AppUi())->SetTab(EContextCallLogLookupTab);
		auto_ptr<CBBSubSession> bbs(BBSession()->CreateSubSessionL(0));
		ResetUnreadLookupsL(bbs.get());
	}
	if (!iContainer) {
		iContainer = new (ELeave) CContextCallLogContainer;
		iContainer->SetMopParent(this);
		iContainer->ConstructL( ClientRect(), aCallLog, aLog, aIconlist, this, 
			iTitle, iEmptyListboxLabel, iLoadingListboxLabel );
		AppUi()->AddToStackL( *this, iContainer );
        } 
	aCallLog->ReRead();
}

void CContextCallLogView::DoDeactivate()
{
	RDebug::Print(_L("DeActivate"));
	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
    	delete iContainer;
	iContainer = NULL;
}

void CContextCallLogView::exiting()
{
	if (iContainer) iContainer->exiting();
}
