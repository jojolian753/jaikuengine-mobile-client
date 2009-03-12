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

#include "ContextContactsContainer.h"

#include "contextvariant.hrh"

#include "cl_settings.h"

#include "break.h"
#include "contextcontactsappui.h"
#include "cbbsession.h"
#include "jabberdata.h"
#include "jabberpics.h"
#include "ccu_userpics.h"
#include "contextclientsession.h"
#include "file_output_base.h"
#include <ContextContacts.rsg>
#include "contextcontacts.hrh"

#ifdef __JAIKU_ENABLED__
#include "ccu_contactslistbox.h"
#include "juik_listbox.h"
#include "juik_layout.h"
#include "jaiku_layoutids.hrh"
#else
#include "presence_ui_helper.h"
#include "doublelinebox.h"
#endif



#ifndef __S60V3__
//FIXME3RD
#include <sendui.h>
#include <SenduiMtmUids.h>
#endif

#include <smut.h>
#include <txtrich.h>
#include <bautils.h>
#include <mmsconst.h>
#include <miutset.h>
#include <s32strm.h>
#include <akntitle.h> 
#include <eikmenub.h>


enum KEYCODES {
	JOY_LEFT = 0xF807,
	JOY_RIGHT = 0xF808,
	JOY_UP = 0xF809,
	JOY_DOWN = 0xF80A,
	JOY_CLICK = 0xF845,
	KEY_CALL = 0xF862,
	KEY_CANCEL = 0xF863,
	KEY_C = 0x0008
};

enum SCANCODES {
	CALL_SCAN = 0xC4,
	CANCEL_SCAN = 0xC5
};



/*
 * Concepts:
 * !Listbox filtering!
 */

void CContextContactsContainer::ConstructL(CCoeControl* aParent, 
										   Mfile_output_base * log, 
										   CAknView * view)
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("ConstructL"));
	SetContainerWindowL( *aParent );
	iJabberData= iDelegates.iJabberData;
	iFindBoxVisible=EFalse;
	iBBSession=BBSession()->CreateSubSessionL(0);
	{
		aLog = log;
		aView = view;
	}
	
	{
		iCachedSelection = new (ELeave) CArrayFixFlat<TInt>(100);
	}

	{
		iPhonebook= iDelegates.iPhonebook;
		iPhonebook->AddObserverL(this);

	}

#ifdef __JAIKU_ENABLED__

	{
		
		iContactsList = CJaikuContactsListController::NewL(this, iDelegates);
		iListbox = iContactsList->GetListBoxL();		
	    iListbox->SetListBoxObserver(this);
		iListbox->AddItemChangeObserverL(this);
	}
#else
 
	{
		iListBoxArray=CPresenceArray::NewL(book);
		iNameArray=CNameArray::NewL(book);

		iListbox=new (ELeave) doublelinebox(iPhonebook, aLog);
		
		//iListbox->SetContainerWindowL(*this);
		iListbox->ConstructL(iNameArray, iListBoxArray, this, EAknListBoxMarkableList);
		iListbox->View()->SetMatcherCursor(EFalse);
		iListbox->SetListBoxObserver(this);
		iListbox->Model()->SetItemTextArray(iNameArray);
		iListbox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
		//FIX ME: use resource
		iListbox->View()->SetListEmptyTextL(_L("(No match)"));
		iListbox->CreateScrollBarFrameL(ETrue);
		iListbox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, 
			CEikScrollBarFrame::EAuto);
	}
#endif
	
	{
		CAknSearchField::TSearchFieldStyle style(CAknSearchField::ESearch );
		//CAknSearchField::TSearchFieldStyle style(CAknSearchField::EPlain );
		iFindBox = CreateFindBoxL( iListbox, iListbox->Model(), style );
		iFindBox->MakeVisible( ETrue );
		iFindBox->SetFocus(ETrue);

		//StoreCurrentContactIdL();
		ActivateL();
	}
}



void CContextContactsContainer::ResetSearchField()
{
	if (iFindBox) iFindBox->ResetL();
}

void CContextContactsContainer::ResetAndHideSearchField()
{
	if (iFindBox) iFindBox->ResetL();
	if (iFilter) iFilter->HandleOfferkeyEventL();
	iListbox->HandleItemRemovalL();
	DrawDeferred();
}

CContextContactsContainer::~CContextContactsContainer()
{
	CC_TRAPD(err, ReleaseCContextContactsContainer());
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
}

void CContextContactsContainer::ReleaseCContextContactsContainer()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("~CContextContactsContainer"));

	delete iBBSession;

	if (iPhonebook) iPhonebook->RemoveObserverL(this);	
	delete iFindBox;
	//delete iListbox;
	delete iContactsList;
	delete iCachedSelection;

#ifndef __JAIKU_ENABLED__
	delete iNameArray;
	delete iListBoxArray;
#endif
}


void CContextContactsContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("SizeChanged"));


	TRect rect = Rect();
	TJuikLayoutItem parent( rect );

	TJuikLayoutItem l = TJuikLayoutItem(rect).Combine( Layout().GetLayoutItemL(LG_contacts_list, LI_contacts_list__listbox));	
	TRect lb_rect=l.Rect();

	TJuikLayoutItem findPaneL = 
		parent.Combine(Layout().GetLayoutItemL( LG_contacts_list, LI_contacts_list__find_pane ) );
	
	if ( iListbox ) {
		if (iFindBoxVisible) {
			//if (iFindBox) iFindBox->SetFocus(ETrue);
			lb_rect.Resize(0, - findPaneL.h );
		} else {
			lb_rect.Resize(0, 0);
		}
#ifdef __JAIKU_ENABLED__
// 		iListbox->AdjustRectHeightToWholeNumberOfItems(lb_rect);
//                 iListbox->SetRect(lb_rect);


 	    iListbox->AdjustAndSetRect( lb_rect );
#else
		iListbox->AdjustRectHeightToWholeNumberOfItems(lb_rect);
                iListbox->SetRect(lb_rect);
#endif
        }
	SizeChangedForFindBox();
	
}


void CContextContactsContainer::SizeChangedForFindBox()
{
 	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("SizeChangedForFindBox"));
	if (iFindBox) {

		TRect rect = Rect();
		TJuikLayoutItem parent( rect );
		TJuikLayoutItem findPaneL = 
			parent.Combine(Layout().GetLayoutItemL( LG_contacts_list, LI_contacts_list__find_pane ) );
		TRect findPaneR = findPaneL.Rect();
		TInt y = rect.Height() - findPaneR.Height();
		findPaneR.Move( 0, y );
		
		if (! iFindBoxVisible) {
			findPaneR = TRect(0,0,0,0);
		} 
		iFindBox->SetRect(findPaneR);
	}
}
TInt CContextContactsContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("CountComponentControls"));
	if ( iFindBox ) return 2;
	else            return 1;
}

CCoeControl* CContextContactsContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("ComponentControl"));

	switch ( aIndex )
        {
		case 0:
			return iListbox;
			break;
		case 1:
			return iFindBox;
			break;
		default:
			return NULL;
        }
}
void CContextContactsContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();

}

void CContextContactsContainer::HandleControlEventL(
    CCoeControl* ,TCoeEvent )
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("HandleControlEventL"));
}


TKeyResponse CContextContactsContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("OfferKeyEventL"));

	// the search field / listbox combo
	TBool needRefresh( EFalse );
	TBool flagsOfPopup( EFalse );
	if ( aType != EEventKey ) /* Is not key event? */{
		// the find field catches the key presses it needs
		// but doesn't actually do the search. This
		// triggers it
		if (iFilter) iFilter->HandleOfferkeyEventL();
		if (iFindBox) {

			if (iFindBox->TextLength()!= 0) {
				if (!iFindBoxVisible) { // not already showing
					iFindBoxVisible=ETrue;
					SizeChanged();
					DrawDeferred();
				}
			} else {
				if (iFindBoxVisible) { // already showing
					iFindBoxVisible=EFalse;
					SizeChanged();
					DrawDeferred();
				}
			}
		}
		return iListbox->OfferKeyEventL(aKeyEvent, aType);
        }
// 	if (aKeyEvent.iCode==JOY_LEFT) {
// #ifdef GROUP_TAB
// 		return EKeyWasNotConsumed;
// #else
// // 		show_presence_details_current();
// 		return EKeyWasConsumed;
// #endif
// 	} 
	
// 	if (aKeyEvent.iCode == JOY_RIGHT) {
// #ifdef GROUP_TAB
// 		return EKeyWasNotConsumed;
// #else
// // 		show_presence_description_current();
// 		return EKeyWasConsumed;
// #endif
// 	} 
	        
	if ( iListbox ) {
		if (aKeyEvent.iCode==JOY_DOWN || aKeyEvent.iCode == JOY_UP || aKeyEvent.iCode == JOY_CLICK)
			{			
				TKeyResponse response = iListbox->OfferKeyEventL(aKeyEvent, aType);
				StoreCurrentContactIdL();
				return response;
			} 
		
		if (aKeyEvent.iCode==KEY_CALL) {
			if ( CountVisibleItems() >0 ) {
				StoreCurrentContactIdL();

				aView->HandleCommandL(EContextContactsCmdCall);
				return EKeyWasConsumed;
			}
			
		}
		if (aKeyEvent.iCode==KEY_C) {			
			if (iFindBox) {
				StoreCurrentContactIdL();
				if ( (iFindBox->TextLength() == 0) && (CountVisibleItems() >0)  ) {
					aView->HandleCommandL(EContextContactsCmdDelete);
					return EKeyWasConsumed;
				} 
			}
		}
		if (iFindBox) {
			TBool needs_refresh;
			TKeyResponse response = AknFind::HandleFindOfferKeyEventL( aKeyEvent, aType, this, iListbox, 
																	   iFindBox, EFalse, needs_refresh);
			StoreCurrentContactIdL();
			return response;
		}
		return iListbox->OfferKeyEventL(aKeyEvent, aType);
        } else {
		return EKeyWasNotConsumed;
        }
}


void CContextContactsContainer::before_change()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("before_change"));

	
	if (! iListbox ) {
		// still constructing
		return;
	}
	iCurrentIdx = get_current_idx();
	iTopIdx = get_current_top_idx();
}

void CContextContactsContainer::contents_changed(TInt ContactId, TBool aPresenceOnly)
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("contents_changed"));

	if (!iListbox ) { return; }

	if (! aPresenceOnly ) {
		iListbox->HandleItemRemovalL();
		iFilter->HandleItemArrayChangeL();
 		set_current_idx(iCurrentIdx);
		set_current_top_idx(iTopIdx);
	} else {
		TInt top = get_current_top_idx();
		TInt bottom = get_current_bottom_idx();
		TInt i;
		TBool redraw=EFalse;
		for (i=top; (i<=bottom) && !redraw; i++) {
			if (ContactId == KErrNotFound) redraw=ETrue;
			else if (ContactId == iPhonebook->GetContactId(i)) redraw=ETrue;
		}
		if (redraw) iListbox->DrawDeferred();
	}
}

void CContextContactsContainer::BeforeAppExitL()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("exiting"));
	if (iFindBox) 
		{
			iFindBox->SetFocus(EFalse);
			delete iFindBox;
			iFindBox = NULL;
		}
}

void CContextContactsContainer::exiting()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("exiting"));
	if ( iPhonebook )
		iPhonebook->RemoveObserverL( this );
	iPhonebook=0;
}

void CContextContactsContainer::ListBoxItemsChanged (CEikListBox *aListBox)
{
	//StoreCurrentContactIdL();
}

void CContextContactsContainer::HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType)
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("HandleListBoxEventL"));

	contact * c=0;

	if(aListBox == iListbox)
	{
		switch(aEventType)
		{
			case EEventEnterKeyPressed:
			{
				StoreCurrentContactIdL();

#ifdef GROUP_TAB
				c = iPhonebook->GetContact(get_current_idx());
				if (c == NULL || c->presence == NULL) {
					aView->HandleCommandL(EContextContactsCmdOpen);
				} else {
					aView->MenuBar()->SetMenuTitleResourceId(R_CONTEXTCONTACTS_MENUBAR_CLICK_ON_CONTACT);
					aView->MenuBar()->TryDisplayMenuBarL();
					aView->MenuBar()->SetMenuTitleResourceId(R_CONTEXTCONTACTS_MENUBAR_VIEW_CONTACTS);
				}
#else
				aView->HandleCommandL(EContextContactsCmdOpen);
#endif
				break;
			}
			default:
				break;
		}
	}
}

void CContextContactsContainer::ShowMyRichPresenceL()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("ShowMyRichPresence"));
		
	// FIXME: this relies on assumption that "myself" is first
	TBool firstIsMe = EFalse;
	if ( iJabberData->UserNickL().Length() > 0 )
		{
			const TInt first = 0;
			contact* c = iPhonebook->GetContact(first);
			
			if ( c )
				{
					TBuf<50> firstsNick;
					iJabberData->GetJabberNickL(c->id, firstsNick);
					// drop @jaiku.com
					firstIsMe = iJabberData->IsUserNickL( firstsNick );
				}
		}
	
	if ( firstIsMe )
		{
			set_current_idx(0);
			set_current_top_idx(0);
			iListbox->HandleItemRemovalL();
			show_presence_details_current();
		}
}

void CContextContactsContainer::show_presence_details_current()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("show_presence_details_current"));

	TInt current_index = get_current_idx();
	if (current_index < 0) return;
	
	contact * c = iPhonebook->GetContact(current_index);
	if (c == NULL || c->presence == NULL) return;

	//FIX ME: check the CPbkContact engine status for first name/last name
	TBuf<128> name;

#ifdef __JAIKU__
	TPtrC firstName( * (c->first_name) );
	TPtrC lastName ( * (c->last_name) );
	if ( firstName.Length() )
	    {
	    name.Append( firstName );
	    }
	else
	    {
	    name.Append( lastName );
	    }
#else
	if (c->last_name) name.Append(*(c->last_name));
	if (c->last_name && c->last_name->Length()>0 && c->first_name && c->first_name->Length()>0) {
		name.Append(_L(" "));
	}
	if (c->first_name) name.Append(*(c->first_name));
#endif	
	
	if (aLog) 
	{
		aLog->write_time();
		aLog->write_to_output(_L("Showing pres. details of "));
		aLog->write_to_output(name);
		aLog->write_nl();
	}
	TBuf<50> nick;
	iJabberData->GetJabberNickL(c->id, nick);

	if (! c->show_details_in_list ) 
		SendLookupNotificationL(iBBSession, nick, c->presence);

	//((CContextContactsAppUi *)(iEikonEnv->AppUi()))->DisplayPresenceDetailsL(name, nick, c->presence);
	((CContextContactsAppUi *)(iEikonEnv->AppUi()))->DisplayRichPresenceL();
}

void CContextContactsContainer::show_presence_description_current()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("show_presence_description_current"));

	TInt current_index = get_current_idx();
	if (current_index < 0) return;

	contact * c = iPhonebook->GetContact(current_index);
	if (c == NULL || c->presence == NULL) return;

	//FIX ME: check the CPbkCOntact engine status for first name/last name
	TBuf<128> name;

	if (c->last_name) name.Append(*(c->last_name));
	if (c->last_name && c->last_name->Length()>0 && c->first_name && c->first_name->Length()>0) {
		name.Append(_L(" "));
	}
	if (c->first_name) name.Append(*(c->first_name));
	
	if (aLog) 
	{
		aLog->write_time();
		aLog->write_to_output(_L("Showing pres. description of "));
		aLog->write_to_output(name);
		aLog->write_nl();
	}
	((CContextContactsAppUi *)(iEikonEnv->AppUi()))->DisplayPresenceDescriptionL(name, 
		(iPhonebook->GetContact(current_index))->presence);
}


CAknSearchField* CContextContactsContainer::CreateFindBoxL(CEikListBox* aListBox, CTextListBoxModel* aModel, CAknSearchField::TSearchFieldStyle aStyle )
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("CreateFindBoxL"));

	CAknSearchField* findbox = NULL;

	if ( aListBox && aModel )
	{
		CAknFilteredTextListBoxModel* model = STATIC_CAST( CAknFilteredTextListBoxModel*, aModel );
		findbox = CAknSearchField::NewL( *this, aStyle, NULL, 30 );
		CleanupStack::PushL(findbox);
		model->CreateFilterL( aListBox, findbox );
		iFilter = model->Filter();

#ifndef __JAIKU_ENABLED__
		iListbox->iPresenceModel->SetFilter(iFilter);
#endif
		CleanupStack::Pop(findbox); // findbox
        }
	return findbox;
}

// void CContextContactsContainer::SizeChangedForFindBox()
// {
// 	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("SizeChangedForFindBox"));

// 	if ( iListbox && iFindBox ) {
// 		//TInt findWindowResourceId( R_AVKON_FIND_PANE );
// 		//TInt listAreaResourceId( R_AVKON_LIST_GEN_PANE_X );
// 		//TInt findWindowParentId( R_AVKON_MAIN_PANE_PARENT_NONE );
// 		//TBool flagsOfPopup( ETrue ); 
// 		//AknFind::HandleFindSizeChanged( this, iListbox, iFindBox, flagsOfPopup, findWindowResourceId,
// 		//listAreaResourceId, R_AVKON_LIST_GEN_PANE_WITH_FIND_POPUP, findWindowParentId );
// 	}
// }

void CContextContactsContainer::MarkCurrentItemL()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("MarkCurrentItemL"));

	TInt real_idx = get_current_idx();
	iListbox->View()->ToggleItemL(iListbox->CurrentItemIndex());
}

void CContextContactsContainer::MarkAllL()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("MarkAllL"));

	// matk all the visible items !
	for (TInt j = 0; j< iFilter->FilteredNumberOfItems(); j++)
	{
		iListbox->View()->SelectItemL(j);
	}
}

void CContextContactsContainer::UnmarkAll()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("UnmarkAll"));
	iListbox->View()->ClearSelection();
}

TInt CContextContactsContainer::CountVisibleItems()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("CountVisibleItems"));

	return iFilter->FilteredNumberOfItems();
}

TInt CContextContactsContainer::SelectedItemsCountL()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("SelectedItemsCount"));
	iFilter->UpdateSelectionIndexesL();
	return iFilter->SelectionIndexes()->Count();
}

CArrayFix<TInt>* CContextContactsContainer::GetCopyOfSelectionIndexesL()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("GetCopyOfSelectionIndexesL"));
	iFilter->UpdateSelectionIndexesL();
	CArrayFix<TInt>* selections = iFilter->SelectionIndexes();
	auto_ptr< CArrayFix<TInt> > copy( new (ELeave) CArrayFixFlat<TInt>( 100 ) );
	for (TInt i=0; i < selections->Count(); i++)
		{
			copy->AppendL( selections->At(i) );
		}
	return copy.release();
}

TInt CContextContactsContainer::get_current_idx()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("get_current_idx"));
	if ( ! iFilter ) return KErrNotFound;
	if (iFilter->FilteredNumberOfItems() == 0) return KErrNotFound;
	TInt real_idx = iFilter->FilteredItemIndex(iListbox->CurrentItemIndex());
	return real_idx;
}

TInt CContextContactsContainer::get_current_top_idx()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("get_current_top_idx"));

	if (iFilter->FilteredNumberOfItems() == 0) return -1;
	TInt real_idx = iFilter->FilteredItemIndex(iListbox->TopItemIndex());
	return real_idx;
}

TInt CContextContactsContainer::get_current_bottom_idx()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("get_current_bottom_idx"));

	if (iFilter->FilteredNumberOfItems() == 0) return -1;
	TInt real_idx = iFilter->FilteredItemIndex(iListbox->BottomItemIndex());
	return real_idx;
}

void CContextContactsContainer::set_current_idx(TInt real_idx)
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("set_current_idx"));
	if (real_idx == -1) 
		{
			if (iFilter->FilteredNumberOfItems() < 1) return;
			real_idx = 0;
		}

	if (iFilter->FilteredNumberOfItems() < 1) return;
	
	TBuf8<20> sel;
	
	TInt idx;
	if (real_idx > iFilter->FilteredNumberOfItems() ) {
		sel=_L8("idx = 0;");
		idx = 0;	
	} else if (real_idx == iFilter->FilteredNumberOfItems()) {
		sel=_L8("idx = real_idx-1;");
		idx = real_idx-1;
	} else {
		sel=_L8("idx = iFilter->");
		idx = iFilter->VisibleItemIndex(real_idx);
	}
	
	if (idx >= 0)
		{
			TBuf8<15> ib; ib.AppendNum(idx);
			CALLSTACKITEM_N(sel, ib);
			iListbox->View()->SetCurrentItemIndex(idx);
			StoreCurrentContactIdL();
			iListbox->DrawDeferred();
		}
}

void CContextContactsContainer::set_current_top_idx(TInt real_idx)
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("set_current_top_idx"));

	if (real_idx == -1) return;
	if (iFilter->FilteredNumberOfItems() < 1) return;

	TInt idx;
	if (real_idx > iFilter->FilteredNumberOfItems() )
	{
		idx = 0;	
	}
	else if (real_idx == iFilter->FilteredNumberOfItems())
	{
		idx = real_idx-1;
	}
	else
	{
		idx = iFilter->VisibleItemIndex(real_idx);
	}
	if (idx < 0) return;
	iListbox->View()->SetTopItemIndex(idx);
	//iListbox->View()->SetCurrentItemIndex(idx);
	iListbox->DrawDeferred();
}

TBool CContextContactsContainer::IsCurrentMarked()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer"), _CL("IsCurrentMarked"));

	TInt idx = get_current_idx();

	const CListBoxView::CSelectionIndexArray* selectionArray = iListbox->View()->SelectionIndexes();

	TBool found = EFalse;
	for (TInt i = 0; (i < selectionArray->Count()) && (found == EFalse); i++)
	{
		if ((*selectionArray)[i] == idx) 
		{ 
			found = ETrue;
		}
	}
	return found;
}


void CContextContactsContainer::StoreCurrentContactIdL() 
{
	if ( iPhonebook )
		{
			TInt id = iPhonebook->GetContactId( get_current_idx() );
			((CContextContactsAppUi *)(iEikonEnv->AppUi()))->SetCurrentContactIdL( id ) ;	
		}
}

void CContextContactsContainer::HandleResourceChange( TInt aType, const TRect& aRect )
{
	CCoeControl::HandleResourceChange(aType);
	if ( aType == KEikDynamicLayoutVariantSwitch ) SetRect(aRect);
}


// TTypeUid::Ptr CContextContactsContainer::MopSupplyObject(TTypeUid aId)
// {
// 	if (aId.iUid == MAknsControlContext::ETypeId)
// 		{
// 			return MAknsControlContext::SupplyMopObject( aId, iBackground );
// 		}
// 	return TTypeUid::Null();
// }
