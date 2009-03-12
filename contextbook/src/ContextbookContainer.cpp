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

#include "ContextbookContainer.h"

#include <flogger.h>

#include <eiklabel.h>  // for example label control
#include <bautils.h>
#include <sendui.h>
#include <smut.h>
#include <SenduiMtmUids.h>
#include <txtrich.h>
#include <akntitle.h> 
#include <aknnavi.h> 
#include <gulicon.h>
 
#include "doublelinebox.h"
#include "file_output_base.h"

#include "contextbookappui.h"

enum KEYCODES {
	JOY_LEFT = 0xF807,
	JOY_RIGHT = 0xF808,
	JOY_UP = 0xF809,
	JOY_DOWN = 0xF80A,
	JOY_CLICK = 0xF845,
	KEY_CALL = 0xF862,
	KEY_CANCEL = 0xF863
};

enum SCANCODES {
	CALL_SCAN = 0xC4,
	CANCEL_SCAN = 0xC5
};



// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CContextbookContainer::ConstructL(const TRect& aRect)
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CContextbookContainer::ConstructL(const TRect& aRect, phonebook_i* i_book, bool i_searchable, const TDesC& title, Cfile_output_base * aLog, CAknIconArray * aIconlist, TInt current_item_index, TInt top_item_index, TBuf<20> current_filter)
{
	CALLSTACKITEM(_L("CContextbookContainer::ConstructL"));

	iCurrentContactId=KErrNotFound;

	iconlist = new (ELeave) CAknIconArray(30);
	//copy
	for (TInt i = 0; i< aIconlist->Count();i++)
	{
		CGulIcon * ic, *icon_from; 
		icon_from= (*aIconlist)[i];
		ic=CGulIcon::NewL( icon_from->Bitmap(), icon_from->Mask());
		ic->SetBitmapsOwnedExternally(ETrue);
		iconlist->AppendL( ic );
	}
	
	iLog = aLog;
	CreateWindowL();

	CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
	CAknTitlePane* tp=(CAknTitlePane*)sp->ControlL(TUid::Uid(EEikStatusPaneUidTitle));

	HBufC* t=HBufC::NewL(title.Length());
	*t=title;
	tp->SetText(t);

	if (iLog) {
		iLog->write_time();
		iLog->write_to_output(_L("Showing "));
		iLog->write_to_output(title);
		iLog->write_nl();
	}

	sendui=CSendAppUi::NewL(0);

	searchable=i_searchable;

	resource_files=new (ELeave) CArrayFixFlat<TInt>(5);

	// for phonebook dialogs
	TFileName resfile=_L("z:\\System\\data\\PBKVIEW.rSC");
	BaflUtils::NearestLanguageFile(iEikonEnv->FsSession(), resfile); //for localization
	resource_files->AppendL(iEikonEnv->AddResourceFileL(resfile)); 
	
	book=i_book;
	book->set_observer(this);

	listbox=new (ELeave) doublelinebox(book,iLog);
	listbox->SetContainerWindowL(*this);
	listbox->ConstructL(this, EAknListBoxSelectionList);
	listbox->SetItemHeightL(40);
	listbox->View()->SetMatcherCursor(EFalse);

	listbox->ItemDrawer()->FormattedCellData()->SetIconArray(iconlist);

	listbox->SetListBoxObserver(this);
	listbox->Model()->SetItemTextArray(book->get_array());
	
	listbox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	listbox->CreateScrollBarFrameL(ETrue);
	listbox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	listbox->SetCurrentItemIndex(current_item_index);
	listbox->SetTopItemIndex(top_item_index);

	if (is_searchable()) 
	{
		edit=new (ELeave) CEikEdwin;
		edit->SetBorder(TGulBorder::ESingleBlack);
		edit->SetContainerWindowL(*this);
		edit->ConstructL();
		edit->AddEdwinObserverL(this);
		edit->SetFocus(ETrue);
		edit->SetTextL(&current_filter);
		filter();
		
	}

	phone=new (ELeave) phonehelper;
	phone->ConstructL();

	globalNote=CAknGlobalNote::NewL();

	pbkengine=CPbkContactEngine::Static();
	if (pbkengine) {
		owns_engine=false;
	} else {
		pbkengine=CPbkContactEngine::NewL();
		owns_engine=true;
	}
	SetRect(aRect);
   
	ActivateL();
}

// Destructor
CContextbookContainer::~CContextbookContainer()
{
	CALLSTACKITEM(_L("CContextbookContainer::~CContextbookContainer"));

	if (book) book->set_observer(0);

	for (int i=0; i<resource_files->Count(); i++) {
		iEikonEnv->DeleteResourceFile((*resource_files)[i]);
	}
	delete resource_files;
	delete listbox;
	delete edit;
	delete phone;
	delete sendui;
	delete globalNote;
		
	if (owns_engine) delete pbkengine;
}

// ---------------------------------------------------------
// CContextbookContainer::SizeChanged()
// Called by framework when the view size is changed
// ---------------------------------------------------------
//
void CContextbookContainer::SizeChanged()
{
	CALLSTACKITEM(_L("CContextbookContainer::SizeChanged"));

	TRect lb_rect=Rect();
	if (listbox) {

		if (edit) {
			lb_rect.Resize(-8, -20);
		} else {
			lb_rect.Resize(-8, 0);
		}

		lb_rect.Move(4, 0);
                listbox->SetRect(lb_rect);
	}

	if (edit) {
		TRect edit_rect=Rect();
		edit_rect.SetHeight(19);
		edit_rect.Resize(-8, 0);
		edit_rect.Move(4, lb_rect.Height());
		edit->SetRect(edit_rect);
		edit->SetFocus(ETrue);
	}
}

// ---------------------------------------------------------
// CContextbookContainer::CountComponentControls() const
// ---------------------------------------------------------
//
TInt CContextbookContainer::CountComponentControls() const
{
	CALLSTACKITEM(_L("CContextbookContainer::CountComponentControls"));

	if (edit) {
		return 2;
	} else {
		return 1;
	}
}

// ---------------------------------------------------------
// CContextbookContainer::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CContextbookContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM(_L("CContextbookContainer::ComponentControl"));

	switch ( aIndex )
        {
        case 0:
		return listbox;
	case 1:
		return edit;
        default:
		return NULL;
        }
}

// ---------------------------------------------------------
// CContextbookContainer::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CContextbookContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM(_L("CContextbookContainer::Draw"));

	CWindowGc& gc = SystemGc();
	// TODO: Add your drawing code here
	// example code...
	gc.SetPenStyle(CGraphicsContext::ENullPen);

	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);

	if (edit) {
		TGulBorder border(TGulBorder::ESingleBlack);
		gc.SetPenStyle(CGraphicsContext::ESolidPen);
		gc.SetBrushStyle(CGraphicsContext::ENullBrush);
		gc.SetPenColor(KRgbBlack);
		TRect edit_rect=Rect();
		edit_rect.SetHeight(19);
		edit_rect.Resize(-8, 0);
		edit_rect.Move(4, Rect().Height()-20);
		border.Draw(gc, edit_rect);
	}
}

// ---------------------------------------------------------
// CContextbookContainer::HandleControlEventL(
//     CCoeControl* aControl,TCoeEvent aEventType)
// ---------------------------------------------------------
//
void CContextbookContainer::HandleControlEventL(
						CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	CALLSTACKITEM(_L("CContextbookContainer::HandleControlEventL"));

	// TODO: Add your control event handler code here
}


TKeyResponse CContextbookContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM(_L("CContextbookContainer::OfferKeyEventL"));

		
	if(listbox && (aKeyEvent.iCode==JOY_UP || aKeyEvent.iCode==JOY_DOWN || aKeyEvent.iCode==JOY_CLICK))
	{
		return listbox->OfferKeyEventL(aKeyEvent,aType);
	}	
	else if(aKeyEvent.iCode==KEY_CALL) 
	{
		call_current();
		return EKeyWasConsumed;
	} 
	else if(edit) 
	{
		TKeyResponse ret;
		ret=edit->OfferKeyEventL(aKeyEvent, aType);
		filter();
		return ret;
	} 
	else return EKeyWasNotConsumed;
}

void CContextbookContainer::before_change()
{
	CALLSTACKITEM(_L("CContextbookContainer::before_change"));

	
	if (! listbox ) {
		// still constructing
		return;
	}
	TInt idx=get_current_idx();
	TInt id=book->GetContactId(idx);
	if (id!=KErrNotFound) iCurrentContactId=id;
}

void CContextbookContainer::contents_changed()
{
	CALLSTACKITEM(_L("CContextbookContainer::contents_changed"));

	if (!listbox ) {
		// still constructing
		return;
	}
	listbox->HandleItemRemovalL();
	TInt idx=book->GetIndex(iCurrentContactId);
	if (idx==KErrNotFound) 
	{
		listbox->SetCurrentItemIndexAndDraw(0);
	} 
	else 
	{
		listbox->SetCurrentItemIndexAndDraw(idx);
	}
	DrawNow();	
}

void CContextbookContainer::exiting()
{
	CALLSTACKITEM(_L("CContextbookContainer::exiting"));

	book=0;
}

TInt CContextbookContainer::get_current_idx()
{
	CALLSTACKITEM(_L("CContextbookContainer::get_current_idx"));

	TInt idx = listbox->View()->CurrentItemIndex();
	TInt id=book->GetContactId(idx);
	if (id!=KErrNotFound) iCurrentContactId=id;
	return idx;
}

TInt CContextbookContainer::get_top_idx()
{
	CALLSTACKITEM(_L("CContextbookContainer::get_top_idx"));

	return listbox->View()->TopItemIndex();
}

void CContextbookContainer::GetCurrentFilter(TDes& aBuffer)
{
	CALLSTACKITEM(_L("CContextbookContainer::GetCurrentFilter"));

	if (edit) {edit->GetText(aBuffer);}
}


void CContextbookContainer::sms_current()
{
	CALLSTACKITEM(_L("CContextbookContainer::sms_current"));

	TPtrC no( book->get_phone_no(get_current_idx()) );

	if (no!=KNullDesC) {
		if (iLog) {
			iLog->write_time();
			iLog->write_to_output(_L("Smsing: "));
			iLog->write_to_output(listbox->Model()->ItemTextArray()->MdcaPoint(get_current_idx()));
			iLog->write_nl();
		}
		CParaFormatLayer* paraf=CParaFormatLayer::NewL();
		CleanupStack::PushL(paraf);

		CCharFormatLayer* charf=CCharFormatLayer::NewL();
		CleanupStack::PushL(charf);

		CRichText* body;
		body=CRichText::NewL(paraf, charf);
		CleanupStack::PushL(body);
		
		CDesCArrayFlat* a=new CDesCArrayFlat(1);
		CleanupStack::PushL(a);
		a->AppendL(no);
		//s->CreateAndSendMessageL(KSenduiMtmSmsUid, 0, 0, KNullUid, a, 0, EFalse);
		sendui->CreateAndSendMessageL(KUidMsgTypeSMS, body, 0, KNullUid, a);
		CleanupStack::PopAndDestroy(4); //a, body, paraf, charf
	}
}

void CContextbookContainer::call_current()
{
	CALLSTACKITEM(_L("CContextbookContainer::call_current"));

	TInt current_index = get_current_idx();
	if (current_index < 0) return;
	TPtrC no( book->get_phone_no(current_index) );

	if (no!=KNullDesC) {
		if (iLog) {
			iLog->write_time();
			iLog->write_to_output(_L("Calling: "));
			iLog->write_to_output(listbox->Model()->ItemTextArray()->MdcaPoint(get_current_idx()));
			iLog->write_nl();
		}
		phone->make_call(no);
	}
}

void CContextbookContainer::HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType)
{
	CALLSTACKITEM(_L("CContextbookContainer::HandleListBoxEventL"));

	if(aListBox == listbox)
	{
		switch(aEventType)
		{
		case EEventEnterKeyPressed:
			show_presence_details_current();
			break;
		default:
			break;
		}
	}
}

void CContextbookContainer::HandleEdwinEventL(CEikEdwin* /*aEdwin*/,TEdwinEvent /*aEventType*/)
{
	CALLSTACKITEM(_L("CContextbookContainer::HandleEdwinEventL"));
	
}

void CContextbookContainer::filter()
{
	CALLSTACKITEM(_L("CContextbookContainer::filter"));

	if (edit) {
		TBuf<20> buf;
		edit->GetText(buf);
		book->filter(buf);
	}
}

bool CContextbookContainer::is_searchable() const
{
	CALLSTACKITEM(_L("CContextbookContainer::is_searchable"));

	return searchable;
}

void CContextbookContainer::show_presence_details_current()
{
	CALLSTACKITEM(_L("CContextbookContainer::show_presence_details_current"));

	TInt current_index = get_current_idx();
	if (current_index < 0) return;

	contact * c = book->GetContact(current_index);
	if (c == NULL || c->presence == NULL) return;

    TBuf<128> name;

	if (c->last_name) name.Append(*(c->last_name));
	if (c->last_name && c->last_name->Length()>0 && c->first_name && c->first_name->Length()>0) {
		name.Append(_L(" "));
	}
	if (c->first_name) name.Append(*(c->first_name));
	
	if (iLog) {
		iLog->write_time();
		iLog->write_to_output(_L("Checking presence details of "));
		iLog->write_to_output(name);
		iLog->write_nl();
	}
	
	((CContextbookAppUi *)(iEikonEnv->AppUi()))->DisplayPresenceDetailsL(name, (book->GetContact(current_index))->presence);
		
}

void CContextbookContainer::ResetSearchField()
{
	CALLSTACKITEM(_L("CContextbookContainer::ResetSearchField"));

	if (edit)
	{
		_LIT(KEmpty, "");
		edit->SetTextL(&(KEmpty()));
		filter();
	}
}
