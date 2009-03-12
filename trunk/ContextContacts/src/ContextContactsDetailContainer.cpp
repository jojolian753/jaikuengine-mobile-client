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

#include "ContextContactsDetailContainer.h"

#include "app_context.h"
#include "ccu_staticicons.h"
#include "cc_stringtools.h"
#include "juik_iconmanager.h"
#include "juik_keycodes.h"
#include "icons.h"
#include "symbian_auto_ptr.h"

#include <ContextContacts.rsg>
#include "contextcontacts.hrh"


#include <eikspane.h>
#include <akntitle.h> 
#include <aknnavi.h> 
#include <eikmenup.h>
#include <aknlists.h> 
#include <avkon.hrh>
#include <cpbkfieldinfo.h> 
#include <pbkiconinfo.h> 
#include <gulicon.h>


void CContextContactsDetailContainer::ConstructL(const TRect& /*aRect*/, CPbkContactItem * item, CAknView * view, CArrayPtr<CGulIcon>* aIconArray)
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("ConstructL"));
	
	aItem=item;
	aView = view;
	

		
	iItemArray = new (ELeave) CDesC16ArrayFlat(50);
	iIdxArray   = new (ELeave) CArrayFixFlat<TPbkFieldId>(50);
	
	populate_listbox();
	
	iListbox = new (ELeave)  CAknDoubleGraphicStyleListBox ;
	iListbox->SetContainerWindowL(*this);
	iListbox->ConstructL(this, EAknListBoxSelectionList);
	iListbox->View()->SetMatcherCursor(EFalse);
	iListbox->ItemDrawer()->FormattedCellData()->SetIconArrayL( aIconArray );
	iListbox->SetListBoxObserver(this);
	iListbox->Model()->SetItemTextArray(iItemArray);
	iListbox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	//FIX ME:
	iListbox->View()->SetListEmptyTextL(_L("(no contact details)"));
	iListbox->CreateScrollBarFrameL(ETrue);
	iListbox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	iListbox->MakeVisible(ETrue);
	ActivateL();
}


CContextContactsDetailContainer::~CContextContactsDetailContainer()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("~CContextContactsDetailContainer"));
	iListbox->ItemDrawer()->FormattedCellData()->SetIconArray( NULL );
	
	CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
        CAknNavigationControlContainer* np = (CAknNavigationControlContainer *)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
        np->Pop(NULL); 
	
	delete iIdxArray;
	delete iListbox;
	delete iItemArray;

}

void CContextContactsDetailContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("SizeChanged"));
	
	iListbox->SetRect(Rect());
}


TInt CContextContactsDetailContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("CountComponentControls"));
	
	return 1; // return nbr of controls inside this container
}

CCoeControl* CContextContactsDetailContainer::ComponentControl(TInt aIndex) const
{
	switch ( aIndex )
        {
	case 0:
		return iListbox;
        default:
		return NULL;
        }
}

void CContextContactsDetailContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("Draw"));
	
	CWindowGc& gc = SystemGc();
}

void CContextContactsDetailContainer::HandleControlEventL(
							  CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("HandleControlEventL"));
	
}

TKeyResponse CContextContactsDetailContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("OfferKeyEventL"));
	
	if ( aType != EEventKey ) // Is not key event?
        {
		return EKeyWasNotConsumed;
        }
   
	if ( iListbox )
        {
		if (aKeyEvent.iCode==JOY_DOWN || aKeyEvent.iCode == JOY_UP || aKeyEvent.iCode == JOY_CLICK)
		{
			return iListbox->OfferKeyEventL(aKeyEvent, aType);
		}
		else if (aKeyEvent.iCode == KEY_CALL)
		{
			if (ItemCount() >0 )
			{
				aView->HandleCommandL(EContextContactsCmdCall);
			}
			return EKeyWasConsumed;
		}
		else if (aKeyEvent.iCode == KEY_C)
		{
			if (ItemCount() >0 )
			{
				aView->HandleCommandL(EContextContactsCmdDelete);
			}
			return EKeyWasConsumed;
		}
	}
	return EKeyWasNotConsumed;
}

TInt CContextContactsDetailContainer::get_current_item_idx()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("get_current_item_idx"));
	
	if (ItemCount() == 0) return -1;
	return iIdxArray->At( get_current_idx() );
}

TInt CContextContactsDetailContainer::get_current_idx()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("get_current_idx"));
	
	if (ItemCount() == 0) return -1;
	return iListbox->View()->CurrentItemIndex();
}

void CContextContactsDetailContainer::HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType)
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("HandleListBoxEventL"));
	
	if(aListBox == iListbox)
	{
		switch(aEventType)
		{
		case EEventEnterKeyPressed:
			{
				if (ItemCount() >0 )
				{
					aView->MenuBar()->SetMenuTitleResourceId(R_CONTEXTCONTACTS_MENUBAR_DETAIL_SPECIFIC_VIEW);
					aView->MenuBar()->TryDisplayMenuBarL();
					aView->MenuBar()->SetMenuTitleResourceId(R_CONTEXTCONTACTS_MENUBAR_DETAIL_VIEW);
				}
				break;
			}
		default:
			break;
		}
	}	
}

TBool CContextContactsDetailContainer::IsCurrentPhoneNumber()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("IsCurrentPhoneNumber"));
	
	TInt idx = iIdxArray->At( get_current_idx() );
	TPbkFieldId id = aItem->PbkFieldAt(idx).PbkFieldId();
	
	TPbkContactItemField * field = aItem->FindField(id, idx);
	return (field->FieldInfo().IsPhoneNumberField());
}

TBool CContextContactsDetailContainer::HasPhoneNumber()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("HasPhoneNumber"));
	
	for (int i=0; i<iIdxArray->Count(); i++)
	{
		TInt idx = iIdxArray->At(i);
		TPbkFieldId id = aItem->PbkFieldAt(idx).PbkFieldId();
		TPbkContactItemField * field = aItem->FindField(id, idx);
		if (field->FieldInfo().IsPhoneNumberField()) return ETrue;
	}
	return EFalse;
}

TBool CContextContactsDetailContainer::IsCurrentMMS()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("IsCurrentMMS"));
	
	TInt idx = iIdxArray->At( get_current_idx() );
	TPbkFieldId id = aItem->PbkFieldAt(idx).PbkFieldId();
	
	TPbkContactItemField * field = aItem->FindField(id, idx);
	return (field->FieldInfo().IsMmsField());
}

TBool CContextContactsDetailContainer::HasMmsAddress()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("HasMmsAddress"));
	
	for (int i=0; i<iIdxArray->Count(); i++)
	{
		TInt idx = iIdxArray->At( i );
		TPbkFieldId id = aItem->PbkFieldAt(idx).PbkFieldId();
		TPbkContactItemField * field = aItem->FindField(id, idx);
		if (field->FieldInfo().IsMmsField()) return ETrue;
	}
	return EFalse;
}

TBool CContextContactsDetailContainer::HasEmailAddress()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("HasEmailAddress"));
	
	for (int i=0; i<iIdxArray->Count(); i++)
	{
		TInt idx = iIdxArray->At( i );
		TPbkFieldId id = aItem->PbkFieldAt(idx).PbkFieldId();
		TPbkContactItemField * field = aItem->FindField(id, idx);
		if (field->FieldInfo().IsMmsField() && !(field->FieldInfo().IsPhoneNumberField())) return ETrue;
	}
	return EFalse;
}

TBool CContextContactsDetailContainer::IsCurrentWebAddress()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("IsCurrentWebAddress"));
	
	TInt idx = iIdxArray->At( get_current_idx() );
	TPbkFieldId id = aItem->PbkFieldAt(idx).PbkFieldId();
	
	TPbkContactItemField * field = aItem->FindField(id, idx);
	if ( field->PbkFieldType() == KStorageTypeText )
		{
			TInt offset = field->PbkFieldText().Find(_L("http://"));
			return (offset!=KErrNotFound);
		}
	else
		{
			return EFalse;
		}
}

TBuf<255> CContextContactsDetailContainer::GetWebAddress()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("GetWebAddress"));
	
	TBuf<255> url;
	
	// let's try current item
	TInt idx = iIdxArray->At( get_current_idx() );
	TPbkFieldId id = aItem->PbkFieldAt(idx).PbkFieldId();
	
	TPbkContactItemField * field = aItem->FindField(id, idx);
	TInt offset = KErrNotFound;
	if ( field->PbkFieldType() == KStorageTypeText )
		{
			offset = field->PbkFieldText().Find(_L("http://"));
		}
	if (offset!=KErrNotFound)
	{
		url.Copy(field->PbkFieldText());
		return url;
	}
	else
	{
		// looks like it's somewhere else
		for (int i=0; i<iIdxArray->Count(); i++)
		{
			TInt idx = iIdxArray->At( i );
			TPbkFieldId id = aItem->PbkFieldAt(idx).PbkFieldId();
			TPbkContactItemField * field = aItem->FindField(id, idx);
			if ( field->PbkFieldType() == KStorageTypeText )
			{
				TInt offset = field->PbkFieldText().Find(_L("http://"));
				if (offset!=KErrNotFound) 
				{
					url.Copy(field->PbkFieldText());
					return url;
				}
			}
		}
		return url;
	}
}

void CContextContactsDetailContainer::populate_listbox()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("populate_listbox"));
		

	iItemArray->Reset();
	iIdxArray->Reset();
	iItemArray->Compress();
	iIdxArray->Compress();
	
	CPbkIconInfoContainer * iconInfoContainer = CPbkIconInfoContainer::NewL();  
	CleanupStack::PushL(iconInfoContainer);
	auto_ptr<HBufC> tempp(HBufC::NewL(255));
	TPtr temp=tempp->Des();
        
	TInt previous_idx = -1;
	for (int i=0; i<aItem->PbkFieldCount(); i++) {
		previous_idx++;
		TPbkFieldId id = aItem->PbkFieldAt(i).PbkFieldId();
		TPbkContactItemField * field = aItem->FindField(id, (previous_idx));
		previous_idx = aItem->FindFieldIndex(*field);
		
		if ( (field->FieldInfo().NameField() == EFalse)  ) {
			temp.Zero();
			if ( field->FieldInfo().IsImageField() ) {
				//FIXME: do something with the panel
			} else if ( field->FieldInfo().FieldStorageType() == KStorageTypeDateTime ) {
				const TInt KDateMaxLen(30);
				//do something with birthdate
				TInt icon = iconInfoContainer->Find(field->IconId())->IconId();
				TInt iconIndex = StaticIcons::GetAvkonOldSkoolIconIndex( icon );
				temp.AppendFormat(_L("%d\t"), iconIndex );  // icon id
				SafeAppend( temp, field->Label(), KDateMaxLen + 1 );
				SafeAppend( temp, _L("\t") );
				
				TBuf<KDateMaxLen> date;
				TTime time = field->PbkFieldTime();
				time.FormatL(date, _L("%D%M%Y%/0%1%/1%2%/2%3%/3") );
				
				SafeAppend( temp, date );
				iItemArray->AppendL(temp);
				iIdxArray->AppendL(i); 
			}
			else
			{
				if (  (field->Label().Length() != 0 ) &&  (field->Text().Length()!=0) ) {
					TPbkIconId iconid=field->IconId();
					TInt icon=0;
					if (iconid!=EPbkNullIconId) {
						icon = iconInfoContainer->Find(iconid)->IconId();
						icon= StaticIcons::GetAvkonOldSkoolIconIndex( icon );
					}
					temp.AppendFormat(_L("%d\t"), icon );  // icon id
					SafeAppend( temp, field->Label(), 1);
					SafeAppend( temp, _L("\t") );
					SafeAppend( temp, field->Text() );
					iItemArray->AppendL(temp);
					iIdxArray->AppendL(i);
				}
			}
		}
	}
	CleanupStack::PopAndDestroy(1); // iconcontainer
}

void CContextContactsDetailContainer::refresh(CPbkContactItem * item)
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("refresh"));
	
	aItem = item;
	
	// title
	CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
	CAknTitlePane* tp=(CAknTitlePane*)sp->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
	tp->SetText(aItem->GetContactTitleL());
	
	populate_listbox();
	iListbox->HandleItemRemovalL();
	iListbox->SetCurrentItemIndexAndDraw(0);
}

TInt CContextContactsDetailContainer::ItemCount()
{
	CALLSTACKITEM_N(_CL("CContextContactsDetailContainer"), _CL("ItemCount"));
	
	return iListbox->Model()->ItemTextArray()->MdcaCount();
}
