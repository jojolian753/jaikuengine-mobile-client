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

#include  <ContextContacts.rsg>
#include  "ContextContactsDetailView.h"
#include  "ContextContactsDetailContainer.h" 
#include  "ContextContacts.hrh"
#include "break.h"
#include "reporting.h"
#include "app_context_impl.h"
#include "ccu_utils.h"
#include "ccu_activestate.h"
#include "ccu_staticicons.h"
#ifndef __S60V2__
#include "browser_interface.h"
#endif

#include "ccu_mainbgcontainer.h"

#ifndef __S60V3__
//FIXME3RD
#include  <SenduiMtmUids.h>
#include <sendui.h>
#include <sendnorm.rsg>
#endif
#include  <txtrich.h>
#include  <MsgBioUids.h>
#include <cpbkphonenumberselect.h> 
#include <cpbksmsaddressselect.h> 
#include <cpbkmmsaddressselect.h> 
#include <cpbkemailaddressselect.h> 
#include <akniconarray.h>
#include <aknmessagequerydialog.h>
#include <aknlists.h> 
#include <cpbkselectfielddlg.h> 
#include <cpbkfieldinfo.h> 
#include <viewcli.h>

class CDefaultNumbersPopupList : public CAknPopupList {
public:
	static CDefaultNumbersPopupList * NewL(CPbkContactItem * item);
	~CDefaultNumbersPopupList();
protected:
	void ConstructL();
	void ProcessCommandL (TInt aCommandId); 
	void HandleListBoxEventL (CEikListBox *aListBox, TListBoxEvent aEventType); 
private:
	CDefaultNumbersPopupList(CPbkContactItem * item) : CAknPopupList(), aItem(item) {}
	void DisplaySelection();
	void Refresh();
	void PopulateListbox();
private:
	CPbkContactItem * aItem;
	CDesCArrayFlat * iArrayItems;
	CEikTextListBox * iListbox;
	HBufC * iPhoneDefault;
	HBufC * iSmsDefault;
	HBufC * iMmsDefault;
	HBufC * iEmailDefault;
	HBufC * iNoDefault;
	HBufC * iDefault;
};

CDefaultNumbersPopupList * CDefaultNumbersPopupList::NewL(CPbkContactItem * item)
{
	CALLSTACKITEM_N(_CL("CDefaultNumbersPopupList"), _CL("NewL"));


	auto_ptr<CDefaultNumbersPopupList> ret(new (ELeave) CDefaultNumbersPopupList(item));
	ret->ConstructL();
	return ret.release();
}

void CDefaultNumbersPopupList::Refresh()
{
	CALLSTACKITEM_N(_CL("CDefaultNumbersPopupList"), _CL("Refresh"));

	iArrayItems->Reset();
	iArrayItems->Compress();
	PopulateListbox();
	iListbox->HandleItemRemovalL();
}

void CDefaultNumbersPopupList::PopulateListbox()
{
	CALLSTACKITEM_N(_CL("CDefaultNumbersPopupList"), _CL("PopulateListbox"));


	TBuf<200> phone;
	phone.Append(*iPhoneDefault);
	phone.Append(_L("\t"));
	if( aItem->DefaultPhoneNumberField() != 0)
	{
		phone.Append(aItem->DefaultPhoneNumberField()->Label());
	}
	else
	{
		phone.Append(*iNoDefault);
	}
	iArrayItems->AppendL(phone);

	TBuf<200> sms;
	sms.Append(*iSmsDefault);
	sms.Append(_L("\t"));
	if( aItem->DefaultSmsField() != 0)
	{
		sms.Append(aItem->DefaultSmsField()->Label());
	}
	else
	{
		sms.Append(*iNoDefault);
	}
	iArrayItems->AppendL(sms);

	TBuf<200> mms;
	mms.Append(*iMmsDefault);
	mms.Append(_L("\t"));
	if( aItem->DefaultMmsField() != 0)
	{
		mms.Append(aItem->DefaultMmsField()->Label());
	}
	else
	{
		mms.Append(*iNoDefault);
	}
	iArrayItems->AppendL(mms);

	TBuf<200> email;
	email.Append(*iEmailDefault);
	email.Append(_L("\t"));
	if( aItem->DefaultEmailField () != 0)
	{
		email.Append(aItem->DefaultEmailField()->Label());
	}
	else
	{
		email.Append(*iNoDefault);
	}
	iArrayItems->AppendL(email);
}

void CDefaultNumbersPopupList::ConstructL()
{
	CALLSTACKITEM_N(_CL("CDefaultNumbersPopupList"), _CL("ConstructL"));
	
	iListbox = new (ELeave)  CAknDoublePopupMenuStyleListBox;

	iPhoneDefault =CEikonEnv::Static()->AllocReadResourceL(R_DEFAULT_PHONE);
	iSmsDefault =CEikonEnv::Static()->AllocReadResourceL(R_DEFAULT_SMS);
	iMmsDefault =CEikonEnv::Static()->AllocReadResourceL(R_DEFAULT_MMS);
	iEmailDefault =CEikonEnv::Static()->AllocReadResourceL(R_DEFAULT_EMAIL);
	iNoDefault =CEikonEnv::Static()->AllocReadResourceL(R_NO_DEFAULT);
	iDefault = CEikonEnv::Static()->AllocReadResourceL(R_DEFAULTS);

	iArrayItems = new CDesCArrayFlat(1);
	PopulateListbox();

	// original contsructl
	CAknPopupList::ConstructL(iListbox, R_DEFAULT_POPUP_CBA, AknPopupLayouts::EMenuDoubleWindow);
	CAknPopupList::SetTitleL(*iDefault);

	iListbox->ConstructL(this, CEikListBox::ELeftDownInViewRect);
	iListbox->CreateScrollBarFrameL(ETrue);
	iListbox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);

	CTextListBoxModel* model = iListbox->Model();
	model->SetItemTextArray(iArrayItems);
	model->SetOwnershipType(ELbmDoesNotOwnItemArray);
}

CDefaultNumbersPopupList::~CDefaultNumbersPopupList()
{
	CALLSTACKITEM_N(_CL("CDefaultNumbersPopupList"), _CL("~CDefaultNumbersPopupList"));

	
	delete iListbox;
	delete iArrayItems;

	delete iPhoneDefault;
	delete iSmsDefault;
	delete iMmsDefault;
	delete iEmailDefault;
	delete iNoDefault;
	delete iDefault;
	
}

void CDefaultNumbersPopupList::ProcessCommandL (TInt aCommandId)
{
	CALLSTACKITEM_N(_CL("CDefaultNumbersPopupList"), _CL("ProcessCommandL"));


	switch (aCommandId)
	{
		case EContextContactsCmdAssign:
		{
			DisplaySelection();
			break;
		}
		case EAknSoftkeyBack:
		default:
		{
			CAknPopupList::ProcessCommandL(aCommandId);
		}

	}
}

void CDefaultNumbersPopupList::DisplaySelection()
{
	CALLSTACKITEM_N(_CL("CDefaultNumbersPopupList"), _CL("DisplaySelection"));


	CPbkSelectFieldDlg * dlg = new CPbkSelectFieldDlg;
	CleanupStack::PushL(dlg);

	CPbkFieldArray& fields = aItem->CardFields(); //original
	CPbkFieldArray * to_display = new CPbkFieldArray(); // with selected
	CleanupStack::PushL(to_display);

	TInt idx = iListbox->CurrentItemIndex();
	TInt focus = -1;
	
	switch (idx)
	{
		case 0: //default phone
		{
			for( int i=0; i<fields.Count();i++ )
			{ 
				if ( (fields.At(i)).FieldInfo().IsPhoneNumberField() )
				{
					to_display->AppendL(fields.At(i));
					if ( (fields.At(i)).DefaultPhoneNumberField() )
					{
						focus = (to_display->Count() -1);
					}
				}
			}
			TPbkContactItemField * field = dlg->ExecuteLD(*to_display, R_SELECT_NONE_CBA,KNullDesC,focus );
			if (!field)
			{
				aItem->RemoveDefaultPhoneNumberField();
			}
			else
			{
				aItem->SetDefaultPhoneNumberFieldL(field);
			}
			break;
		}
		case 1: // default sms
		{
			for( int i=0; i<fields.Count();i++ )
			{ 
				if ( (fields.At(i)).FieldInfo().IsPhoneNumberField() )
				{
					to_display->AppendL(fields.At(i));
					if ( (fields.At(i)).DefaultSmsField() )
					{
						focus = (to_display->Count() -1);
					}
				}
			}
			TPbkContactItemField * field = dlg->ExecuteLD(*to_display, R_SELECT_NONE_CBA,KNullDesC,focus );
			if (!field)
			{
				aItem->RemoveDefaultSmsField();
			}
			else
			{
				aItem->SetDefaultSmsFieldL(field);
			}
			break;
		}
		case 2: //default mms
		{
			for( int i=0; i<fields.Count();i++ )
			{ 
				if ( (fields.At(i)).FieldInfo().IsMmsField() )
				{
					to_display->AppendL(fields.At(i));
					if ( (fields.At(i)).DefaultMmsField() )
					{
						focus = (to_display->Count() -1);
					}
				}
			}
			TPbkContactItemField * field = dlg->ExecuteLD(*to_display, R_SELECT_NONE_CBA,KNullDesC,focus );
			if (!field)
			{
				aItem->RemoveDefaultMmsField();
			}
			else
			{
				aItem->SetDefaultMmsFieldL(field);
			}
			break;
		}
		case 3: // default email
		{
			for( int i=0; i<fields.Count();i++ )
			{ 
				if ( (fields.At(i)).FieldInfo().IsMmsField() && !(fields.At(i)).FieldInfo().IsPhoneNumberField() )
				{
					to_display->AppendL(fields.At(i));
					if ( (fields.At(i)).DefaultEmailField() )
					{
						focus = (to_display->Count() -1);
					}
				}
			}
			TPbkContactItemField * field = dlg->ExecuteLD(*to_display, R_SELECT_NONE_CBA,KNullDesC, focus );
			if (!field)
			{
				aItem->RemoveDefaultEmailField();
			}
			else
			{
				aItem->SetDefaultEmailFieldL(field);
			}
			
			break;
		}
		default:
			break;
	}
	CleanupStack::PopAndDestroy(); //to_display
	CleanupStack::Pop(); // dlg
	Refresh();
}

void CDefaultNumbersPopupList::HandleListBoxEventL (CEikListBox *aListBox, TListBoxEvent aEventType)
{
	CALLSTACKITEM_N(_CL("CDefaultNumbersPopupList"), _CL("HandleListBoxEventL"));


	if (aListBox == iListbox)
	{
		if (aEventType == EEventEnterKeyPressed)
		{
			DisplaySelection();
		}
	}
}

//---------------------------------------------------------------------------------------------------


void CContextContactsDetailView::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailView"), _CL("ConstructL"));


	BaseConstructL( R_CONTEXTCONTACTS_DETAIL_VIEW );
#ifndef __S60V3__
//FIXME3RD
	iSendAppUi=CSendAppUi::NewL(ESendCmdId);
#endif

	pbkengine = CPbkContactEngine::Static();
	if (pbkengine) 	{ owns_engine=false;} 
	else 
	{
		pbkengine=CPbkContactEngine::NewL();
		owns_engine=true;
	}
	LoadIconsL();
}


CContextContactsDetailView::~CContextContactsDetailView()
{
	CC_TRAPD(err, ReleaseViewImpl());
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
}

void CContextContactsDetailView::ReleaseViewImpl()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailView"), _CL("ReleaseViewImpl"));
	RemoveContainerL();
	delete iDetailIcons;
	delete iViewState;
#ifndef __S60V3__
//FIXME3RD
	delete iSendAppUi;
#endif
	if (owns_engine) delete pbkengine;
}

TUid CContextContactsDetailView::Id() const {
    return KDetailViewId;
}

void CContextContactsDetailView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM_N(_CL("CContextContactsDetailView"), _CL("HandleCommandL"));


	switch ( aCommand )
        {
		case EAknSoftkeyBack:
		{
			ActivateParentViewL();
			break;
		}
		case EContextContactsCmdCreateSms:
		{
			if ( iContainer->IsCurrentPhoneNumber() )
			{
				TPtrC no( iItem->PbkFieldAt(iContainer->get_current_item_idx()).PbkFieldText() );
				CDesCArrayFlat* recip=new CDesCArrayFlat(1);
				CleanupStack::PushL(recip);
				CDesCArrayFlat* alias=new CDesCArrayFlat(1);
				CleanupStack::PushL(alias);
				recip->AppendL(no);
				alias->AppendL( *(iItem->GetContactTitleL()) );
				aPhoneHelper.send_sms(recip, alias);
				CleanupStack::PopAndDestroy(2); // recip, alias
			}
			else
			{
                                CPbkSmsAddressSelect* sel=new (ELeave) CPbkSmsAddressSelect();
#ifndef __S60V3__
				TPtrC no(sel->ExecuteLD(*iItem, NULL, EFalse));
#else
				TPtrC no(0, 0);
				CPbkSmsAddressSelect::TParams p(*iItem);
				TBool selected=sel->ExecuteLD(p);
				if (selected) {
					const TPbkContactItemField* f=p.SelectedField();
					if (f && f->StorageType()==KStorageTypeText) {
						no.Set(f->Text());
					}
				}
#endif

				if (no.Length()>0)
				{
					CDesCArrayFlat* recip=new CDesCArrayFlat(1);
					CleanupStack::PushL(recip);
					CDesCArrayFlat* alias=new CDesCArrayFlat(1);
					CleanupStack::PushL(alias);
					recip->AppendL(no);
					alias->AppendL( *(iItem->GetContactTitleL()) );
					aPhoneHelper.send_sms(recip, alias);
					CleanupStack::PopAndDestroy(2); // recip, alias
				}
			}			
			break;
		}
		case EContextContactsCmdCreateMms:
		{
			if ( (iContainer->IsCurrentMMS()) || (iContainer->IsCurrentPhoneNumber()) ) 
			{
				TPtrC no( iItem->PbkFieldAt(iContainer->get_current_item_idx()).PbkFieldText() );
				CDesCArrayFlat* recip=new CDesCArrayFlat(1);
				CleanupStack::PushL(recip);
				CDesCArrayFlat* alias=new CDesCArrayFlat(1);
				CleanupStack::PushL(alias);
				recip->AppendL(no);
				alias->AppendL(*(iItem->GetContactTitleL()));
				aPhoneHelper.send_mms(recip,alias);
				CleanupStack::PopAndDestroy(2); // recip, alias
			}
			else
			{
				CPbkMmsAddressSelect* sel=new (ELeave) CPbkMmsAddressSelect();
#ifndef __S60V3__
				TPtrC no(sel->ExecuteLD(*iItem, NULL, EFalse));
#else
				TPtrC no(0, 0);
				CPbkMmsAddressSelect::TParams p(*iItem);
				TBool selected=sel->ExecuteLD(p);
				if (selected) {
					const TPbkContactItemField* f=p.SelectedField();
					if (f && f->StorageType()==KStorageTypeText) {
						no.Set(f->Text());
					}
				}
#endif
				if ((no!=KNullDesC))
				{
					CDesCArrayFlat* recip=new CDesCArrayFlat(1);
					CleanupStack::PushL(recip);
					CDesCArrayFlat* alias=new CDesCArrayFlat(1);
					CleanupStack::PushL(alias);
					recip->AppendL(no);
					alias->AppendL(*(iItem->GetContactTitleL()));
					aPhoneHelper.send_mms(recip,alias);
					CleanupStack::PopAndDestroy(2); // recip,alias
				}
			}
			break;
		}
		case EContextContactsCmdCreateEmail:
		{
			if ( iContainer->IsCurrentMMS())
			{
				TPtrC no( iItem->PbkFieldAt(iContainer->get_current_item_idx()).PbkFieldText() );
				CDesCArrayFlat* recip=new CDesCArrayFlat(1);
				CleanupStack::PushL(recip);
				CDesCArrayFlat* alias=new CDesCArrayFlat(1);
				CleanupStack::PushL(alias);
				recip->AppendL(no);
				alias->AppendL(*(iItem->GetContactTitleL()));
				aPhoneHelper.send_email(recip,alias);
				CleanupStack::PopAndDestroy(2); // recip,alias
			}
			else
			{
				CPbkEmailAddressSelect* sel=new (ELeave) CPbkEmailAddressSelect();
#ifndef __S60V3__
				TPtrC no(sel->ExecuteLD(*iItem, NULL, EFalse));
#else
				TPtrC no(0, 0);
				CPbkEmailAddressSelect::TParams p(*iItem);
				TBool selected=sel->ExecuteLD(p);
				if (selected) {
					const TPbkContactItemField* f=p.SelectedField();
					if (f && f->StorageType()==KStorageTypeText) {
						no.Set(f->Text());
					}
				}
#endif
				if (no!=KNullDesC)
				{
					CDesCArrayFlat* recip=new CDesCArrayFlat(1);
					CleanupStack::PushL(recip);
					CDesCArrayFlat* alias=new CDesCArrayFlat(1);
					CleanupStack::PushL(alias);
					recip->AppendL(no);
					alias->AppendL(*(iItem->GetContactTitleL()));
					aPhoneHelper.send_email(recip, alias);
					CleanupStack::PopAndDestroy(2); // recip
				}
			}
			break;
		}
		case EContextContactsCmdCall:
		{
			if ( iContainer->IsCurrentPhoneNumber() )
			{
				TPtrC no( iItem->PbkFieldAt(iContainer->get_current_item_idx()).PbkFieldText() );
				aPhoneHelper.make_callL(no);
			}
			else
			{
				CPbkPhoneNumberSelect* sel=new (ELeave) CPbkPhoneNumberSelect();
#ifndef __S60V3__
				TPtrC no(sel->ExecuteLD(*iItem, NULL, EFalse));
#else
				TPtrC no(0, 0);
				CPbkPhoneNumberSelect::TParams p(*iItem);
				TBool selected=sel->ExecuteLD(p);
				if (selected) {
					const TPbkContactItemField* f=p.SelectedField();
					if (f && f->StorageType()==KStorageTypeText) {
						no.Set(f->Text());
					}
				}
#endif
				aPhoneHelper.make_callL(no);
			}
			break;
		}
		case EContextContactsCmdEdit:
		{
			aPhoneHelper.show_editor(iViewState->FocusedContactId(), false, iContainer->get_current_item_idx());
			Refresh();
			break;
		}
		case EContextContactsCmdDefaults:
		{
			CPbkContactItem * temp_item = pbkengine->OpenContactLCX(iViewState->FocusedContactId());
			CDefaultNumbersPopupList * popup = CDefaultNumbersPopupList::NewL(temp_item);
			CleanupStack::PushL(popup);
			popup->ExecuteLD();
			CleanupStack::Pop();//popup
			pbkengine->CommitContactL(*temp_item);
			CleanupStack::PopAndDestroy(2); // lock, temp_item

			Refresh();
			break;
		}
		case EContextContactsCmdGoToWeb:
		{
			TBuf<255> url;
			url.Copy(iContainer->GetWebAddress());
			if (url.Length() == 0) return;

			#ifndef __S60V3__
			// FIXME3RD
			#ifndef __WINS__
		
			#  ifndef __S60V2__

			auto_ptr<CDorisBrowserInterface> ido(CDorisBrowserInterface::NewL());
			ido->AppendL(CDorisBrowserInterface::EOpenURL_STRING, url);
			ido->ExecuteL();
		
			#  else
			HBufC8* addr8=HBufC8::NewLC(url.Length());
			TPtr8 addrp=addr8->Des();
			CC()->ConvertFromUnicode(addrp, url);
		
			TUid KUidOperaBrowserUid = {0x101F4DED};
			TUid KUidOperaRenderViewUid = {0};

			TVwsViewId viewId(KUidOperaBrowserUid, KUidOperaRenderViewUid);
		
			CVwsSessionWrapper* vws;
			vws=CVwsSessionWrapper::NewLC();
			vws->ActivateView(viewId, KUidOperaRenderViewUid, *addr8);
			CleanupStack::PopAndDestroy(2);
			#  endif

			#endif
			#endif
			break;
		}
		case EContextContactsCmdDelete:
		{
			//note: this isn't the behaviour of original contact app
			TInt idx = iContainer->get_current_item_idx();
			TInt id = iItem->PbkFieldAt(idx).PbkFieldId();
			TPbkContactItemField * field = iItem->FindField(id, idx);
			TInt length = field->Label().Length() + field->Text().Length() + 10;

			HBufC * header = CEikonEnv::Static()->AllocReadResourceL(R_DELETE);
			CleanupStack::PushL(header);
			HBufC * message= HBufC::NewL(length);
			CleanupStack::PushL(message);

			message->Des().Append(field->Label());
			message->Des().Append(_L(" ("));
			message->Des().Append(field->Text());
			message->Des().Append(_L(") ?"));

			CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(*message);
			CleanupStack::PushL(dlg);
			dlg->PrepareLC(R_CONFIRMATION_QUERY);
			dlg->QueryHeading()->SetTextL(*header);
			CleanupStack::Pop(dlg);
			
			if ( dlg->RunLD() )
			{
				CPbkContactItem * temp_item = pbkengine->OpenContactLCX(iViewState->FocusedContactId());
				temp_item->RemoveField(iContainer->get_current_item_idx());
				pbkengine->CommitContactL(*temp_item);
				CleanupStack::PopAndDestroy(2);
			}
			CleanupStack::PopAndDestroy(2); //header, message
						
			Refresh();
			break;
		}
#ifdef __S60V3__
		case EContextContactsMenuMsgCurrent:
		case EContextContactsMenuMsg:
		{
			HandleCreateMessageL();
		}
			break;
#endif

		default:
		{
#ifndef __S60V3__
//FIXME3RD
			if (!iSendAppUi->CommandIsValidL(aCommand, TSendingCapabilities(0, 0, TSendingCapabilities::EAllMTMs)))
#endif
			{
				AppUi()->HandleCommandL( aCommand );
			}
#ifndef __S60V3__
			else
			{
				TUid mtm = iSendAppUi->MtmForCommand(aCommand);
				CArrayFixFlat<TInt> * c = new CArrayFixFlat<TInt>(1);
				CleanupStack::PushL(c);
				c->AppendL(iViewState->FocusedContactId());
				aPhoneHelper.send_as( mtm, c);
				CleanupStack::PopAndDestroy(1); // c
			}
#endif
			break;
		}
	}
}

#ifdef __S60V3__

#include <senduiconsts.h>

void CContextContactsDetailView::HandleCreateMessageL()
{
	TInt idx = iContainer->get_current_item_idx();
	
	const MPbkFieldData& fielddata=iItem->PbkFieldAt(idx);
	TPbkFieldId id = fielddata.PbkFieldId();
	const TPbkContactItemField * field = iItem->FindField(id, idx);
	
	TAddressSelectorF selector = SmsSelectorL;
	TMessageSenderF sender = &CMessaging::MessageSenderL;
	TPbkFieldFilterF filter = SmsFieldFilterL;
	TInt warningResource = R_CONTACTS_NO_SMS;
	
	auto_ptr<CArrayFixFlat<TUid> > disabled_mtms( new (ELeave) CArrayFixFlat< TUid >(4) );

	const CPbkFieldInfo &info=field->FieldInfo();
	TBool is_address_field=EFalse;
	auto_ptr<CDesCArrayFlat> recip( new (ELeave) CDesCArrayFlat(1));
	auto_ptr<CDesCArrayFlat> alias( new (ELeave) CDesCArrayFlat(1));
	
	auto_ptr<HBufC> title(iItem->GetContactTitleL());
	alias->AppendL(*title);
	
	if (!info.IsPhoneNumberField()) {
		disabled_mtms->AppendL(KSenduiMtmSmsUid);
	} else {
		is_address_field=ETrue;
	}
	if (!info.IsPhoneNumberField() && !info.IsMmsField() ) {
		disabled_mtms->AppendL(KSenduiMtmMmsUid);
	} else {
		is_address_field=ETrue;
	}
	if (!info.IsEmailField()) {
		disabled_mtms->AppendL(KSenduiMtmSmtpUid);
	} else {
		is_address_field=ETrue;
	}
	
	if (!is_address_field) disabled_mtms->Reset();
	else recip->AppendL( fielddata.PbkFieldText() );
	disabled_mtms->AppendL(KSenduiMtmIrUid);
	disabled_mtms->AppendL(KSenduiMtmBtUid);
	
	TUid mtm=iMessaging->iSendAppUi->ShowSendQueryL(0,  TSendingCapabilities(KCapabilitiesForAllServices), 
		disabled_mtms.get(), _L("Send Message"));
		
	if (mtm==KNullUid) return;
	
	if (is_address_field) {
		iMessaging->MessageSenderL(recip.get(), alias.get(), mtm);
	} else {
		if (mtm==KSenduiMtmSmtpUid) {
			selector=EmailSelectorL;
			filter=EmailFieldFilterL;
			warningResource=R_CONTACTS_NO_EMAIL;
		} else if (mtm!=KSenduiMtmSmsUid) {
			selector=MmsSelectorL;
			filter=MmsFieldFilterL;
			warningResource=R_CONTACTS_NO_MMS;
		}
		iMessaging->CreateSingleMessageL( iItem->Id(), selector, sender, mtm );
	}
	
}
#endif

// ---------------------------------------------------------
// CContextContactsDetailView::HandleClientRectChange()
// ---------------------------------------------------------
//
void CContextContactsDetailView::HandleClientRectChange()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailView"), _CL("HandleClientRectChange"));

	if ( iBgContainer )
        {
		iBgContainer->SetRect( ClientRect() );
        }
}

void CContextContactsDetailView::RealDoActivateL(const TVwsViewId& /*aPrevViewId*/,
												 TUid /*aCustomMessageId*/,
												 const TDesC8& aCustomMessage)
{
	delete iViewState; iViewState=0;
	iViewState = CPbkViewState::NewL(aCustomMessage);

	TInt id = iViewState->FocusedContactId();
	if ( id == KErrNotFound ) 
		{
			id = ActiveState().ActiveContact().GetId();
			iViewState->SetFocusedContactId( id );
		}
	
	if (!iContainer) {
		delete iItem; iItem=0;
		iItem = pbkengine->ReadContactL( id ); 
	}
	
	ActiveState().ActiveContact().SetL( id );
	ActiveState().ActiveItem().ClearL();


	mailbox_defined = aPhoneHelper.mailbox_defined();

	if (!iBgContainer)
		{
			iBgContainer = CMainBgContainer::NewL( this, ClientRect(), ThemeColors(), ProgressBarModel() );
			SetBaseContainer( iBgContainer );
		}
	
	if (!iContainer)
        {
			iContainer = new (ELeave) CContextContactsDetailContainer;
			iContainer->SetContainerWindowL(*iBgContainer);
			iContainer->ConstructL( TRect(), iItem, this, iDetailIcons);
        }
	iBgContainer->SetContentL( iContainer );		
	iBgContainer->MakeVisible(ETrue);
	iBgContainer->ActivateL();
	AppUi()->AddToStackL( *this, iBgContainer );
    
	UpdateStatusPaneL();
	ActiveState().ActiveContact().AddListenerL( *this );
}

void CContextContactsDetailView::RemoveContainerL()
{
	if ( iBgContainer )
		{
			AppUi()->RemoveFromStack( iBgContainer );
			delete iBgContainer;
			iBgContainer = NULL;
			SetBaseContainer( NULL );
		}

	if ( iContainer )
        {
			delete iContainer;
			iContainer = NULL;
		}
	
	delete iItem; iItem=0;
	ActiveState().ActiveContact().RemoveListenerL( *this );
}

void CContextContactsDetailView::RealDoDeactivateL()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailView"), _CL("RealDoDeactivateL"));
	RemoveContainerL();
}


void CContextContactsDetailView::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailView"), _CL("DynInitMenuPaneL"));
	
	if (aResourceId == R_CONTEXTCONTACTS_DETAIL_VIEW_MENU) /*r_contextcontacts_detail_view_menu*/
	{
		
		
		if (iContainer->ItemCount() > 0)
		{
			if (!(iContainer->HasPhoneNumber())) aMenuPane->SetItemDimmed(EContextContactsCmdCall, ETrue);
			if (!(iContainer->HasMmsAddress()))   aMenuPane->SetItemDimmed(EContextContactsMenuMsg, ETrue);
			aMenuPane->SetItemDimmed(EContextContactsCmdEdit, EFalse);
			aMenuPane->SetItemDimmed(EContextContactsCmdDelete, EFalse);
			aMenuPane->SetItemDimmed(EContextContactsCmdDefaults, EFalse);
#ifndef __S60V3__
			iSendAppUi->DisplaySendMenuItemL(*aMenuPane, 6, TSendingCapabilities(0, 0, TSendingCapabilities::ESupportsAttachmentsOrBodyText  ));
#endif
			if ((iContainer->GetWebAddress().Length()==0)) aMenuPane->SetItemDimmed(EContextContactsCmdGoToWeb, ETrue);
			
		}
		else
		{
			aMenuPane->SetItemDimmed(EContextContactsCmdEdit, EFalse);
			aMenuPane->SetItemDimmed(EContextContactsCmdGoToWeb, ETrue);
			aMenuPane->SetItemDimmed(EContextContactsCmdCall, ETrue);
			aMenuPane->SetItemDimmed(EContextContactsMenuMsg, ETrue);
			aMenuPane->SetItemDimmed(EContextContactsCmdDelete, ETrue);
			aMenuPane->SetItemDimmed(EContextContactsCmdDefaults, ETrue);
		}
		
	}
	else if (aResourceId == R_CONTEXTCONTACTS_DETAIL_VIEW_SPECIFIC_MENU) /*r_contextcontacts_detail_view_specific_menu*/
	{
		aMenuPane->SetItemDimmed(EContextContactsCmdEdit, EFalse);
		aMenuPane->SetItemDimmed(EContextContactsMenuMsgCurrent, ETrue);
		aMenuPane->SetItemDimmed(EContextContactsCmdCall, ETrue);
		aMenuPane->SetItemDimmed(EContextContactsCmdGoToWeb, ETrue);
		
		if (iContainer->IsCurrentPhoneNumber())
		{
			aMenuPane->SetItemDimmed(EContextContactsCmdCall, EFalse);
			aMenuPane->SetItemDimmed(EContextContactsMenuMsgCurrent, EFalse);
		}
		if (iContainer->IsCurrentMMS())
		{
			aMenuPane->SetItemDimmed(EContextContactsMenuMsgCurrent, EFalse);
		}
		if (iContainer->IsCurrentWebAddress())
		{
			aMenuPane->SetItemDimmed(EContextContactsCmdGoToWeb, EFalse);
		}
	}
#ifndef __S60V3__
	else if (aResourceId == R_MESSAGE_CURRENT_MENU)
	{
		aMenuPane->SetItemDimmed(EContextContactsCmdCreateSms, ETrue);
		aMenuPane->SetItemDimmed(EContextContactsCmdCreateMms, ETrue);
		aMenuPane->SetItemDimmed(EContextContactsCmdCreateEmail, ETrue);
		if (iContainer->IsCurrentPhoneNumber() )
		{
			aMenuPane->SetItemDimmed(EContextContactsCmdCreateSms, EFalse);
			aMenuPane->SetItemDimmed(EContextContactsCmdCreateMms, EFalse);
			
		}
		else if (iContainer->IsCurrentMMS())
		{
			aMenuPane->SetItemDimmed(EContextContactsCmdCreateMms, EFalse);
			if (mailbox_defined)
			{
				aMenuPane->SetItemDimmed(EContextContactsCmdCreateEmail, EFalse);
			}
		}
	}
	else if (aResourceId == R_MESSAGE_MENU)
	{
		if (!mailbox_defined) aMenuPane->SetItemDimmed(EContextContactsCmdCreateEmail, ETrue);
	}
	else if (aResourceId == R_SENDUI_MENU)
	{
		iSendAppUi->DisplaySendCascadeMenuL(*aMenuPane, NULL);
	}
#endif
}

void CContextContactsDetailView::Refresh()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailView"), _CL("Refresh"));

	
	delete iItem; iItem=0;
	iItem = pbkengine->ReadContactL( iViewState->FocusedContactId() );
	iContainer->refresh(iItem);
	UpdateStatusPaneL();
}


void CContextContactsDetailView::LoadIconsL()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailView"), _CL("LoadIconsL"));
	if ( iDetailIcons )
		{
			delete iDetailIcons;
			iDetailIcons = NULL;
		}
	iDetailIcons = StaticIcons::LoadAvkonIconsOldSkoolL();
}


void CContextContactsDetailView::HandleResourceChange( TInt aType )
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
