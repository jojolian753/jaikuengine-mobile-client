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

#include "ContextCallLogContainer.h"
#include <contextcalllog.rsg>
#include <akntitle.h> 
#include <s32strm.h>
#include <gulicon.h>
#include "contextcalllog.hrh"
#include <aknmessagequerydialog.h>
#include <logwrap.h>
#include <eikmenub.h>

#include "contextcalllogappui.h"
#include "presence_ui_helper.h"

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

#ifdef __S60V2__
#define USE_SKIN 1
#endif

#ifdef USE_SKIN
#include <AknsControlContext.h>
#include <AknsBasicBackgroundControlContext.h>
#include <AknsDrawUtils.h>
#endif

void CContextCallLogContainer::ConstructL(const TRect& aRect, call_log_i* callLog, Mfile_output_base * log, 
					  CAknIconArray * aIconlist, CAknView * view, HBufC * title, 
					  HBufC * emptyListboxLabel, HBufC* aLoadingListboxLabel)
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("ConstructL"));

	aLog = log;
	aView = view;
	iCallLog = callLog;
	iCallLog->AddObserverL(this);

	CreateWindowL();

	iEmptyLabel=emptyListboxLabel;

#ifdef USE_SKIN
	TRect rect=aRect;
	rect.Move( 0, -rect.iTl.iY );
	iBackground=CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain,
		rect, EFalse );
#endif

        CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
	CAknTitlePane* tp=(CAknTitlePane*)sp->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
	HBufC * t = HBufC::NewL(title->Length());
	*t = *title;
        tp->SetText(t);

	iEmptyListboxLabel = new (ELeave) CEikLabel;
	iEmptyListboxLabel->SetTextL(*aLoadingListboxLabel);

	TSize size = iEmptyListboxLabel->MinimumSize();
	
	iEmptyListboxLabel->SetPosition( TPoint( (176-size.iWidth)/2, 60) );
	iEmptyListboxLabel->SetSize(TSize(0,0));
	
	iIconlist = new (ELeave) CAknIconArray(30);
	for (TInt i = 0; i< aIconlist->Count();i++) {
		CGulIcon * ic, *icon_from; 
		icon_from= (*aIconlist)[i];
		ic=CGulIcon::NewL( icon_from->Bitmap(), icon_from->Mask());
		ic->SetBitmapsOwnedExternally(ETrue);
		iIconlist->AppendL( ic );
	}

	iListBoxArray=CPresenceArray::NewL(iCallLog);
	iNameArray=CNameArray::NewL(iCallLog);

	iListbox=new (ELeave) doublelinebox(iCallLog, aLog);
	iListbox->SetContainerWindowL(*this);
	iListbox->ConstructL(iNameArray, iListBoxArray, this, EAknListBoxMarkableList);
	iListbox->View()->SetMatcherCursor(EFalse);
	iListbox->ItemDrawer()->FormattedCellData()->SetIconArray(iIconlist);
	iListbox->SetListBoxObserver(this);
	iListbox->Model()->SetItemTextArray(iNameArray);
	iListbox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	iListbox->CreateScrollBarFrameL(ETrue);
	iListbox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);


	handle_visibility();

	SetRect(aRect);
	ActivateL();
}


CContextCallLogContainer::~CContextCallLogContainer()
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("~CContextCallLogContainer"));
	if (iCallLog) {
		iCallLog->RemoveObserverL(this);
	}
	
	delete iListbox;
	delete iNameArray;
	delete iListBoxArray;
	delete iEmptyListboxLabel;
#ifdef USE_SKIN
	delete iBackground;
#endif
}

void CContextCallLogContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("SizeChanged"));

	TRect lb_rect=Rect();
	if (iListbox) {
		iListbox->AdjustRectHeightToWholeNumberOfItems(lb_rect);
                iListbox->SetRect(lb_rect);
	}
}


TInt CContextCallLogContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("CountComponentControls"));

	return 2; 
}

CCoeControl* CContextCallLogContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("ComponentControl"));
	
	switch ( aIndex ) {
		case 0:
			return iListbox;
		case 1:
			return iEmptyListboxLabel;
		default:
			return NULL;
        }
}

void CContextCallLogContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
#ifndef USE_SKIN
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
#else
	AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, gc, aRect );
#endif
}

void CContextCallLogContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("HandleControlEventL"));
	
	// no impl 
} 

void CContextCallLogContainer::HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType)
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("HandleListBoxEventL"));

	if(aListBox == iListbox)
	{
		switch(aEventType)
		{
			case EEventEnterKeyPressed:
				if (get_current_idx() != -1 ) {
					aView->MenuBar()->SetMenuTitleResourceId(R_CONTEXTCALLLOG_MENUBAR_CLICK_ON_ITEM);
					aView->MenuBar()->TryDisplayMenuBarL();
					aView->MenuBar()->SetMenuTitleResourceId(R_CONTEXTCALLLOG_MENUBAR_VIEW1);
				}
				break;
			default:
				break;
		}
	}
}

void CContextCallLogContainer::before_change()
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("before_change"));
	
	if (! iListbox ) {
		// still constructing
		return;
	}
}

void CContextCallLogContainer::exiting()
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("exiting"));
	
	iCallLog=0;
}

void CContextCallLogContainer::contents_changed(TInt ContactId, TBool aPresenceOnly)
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("contents_changed"));
	
	if (!iListbox ) { return; }

	if (!iGotContents) {
		iEmptyListboxLabel->SetTextL(*iEmptyLabel);
		TSize size = iEmptyListboxLabel->MinimumSize();
		
		iEmptyListboxLabel->SetPosition( TPoint( (176-size.iWidth)/2, 60) );
	}
	iGotContents=ETrue;

	if (! aPresenceOnly ) {
		iListbox->HandleItemRemovalL();
		handle_visibility();
		iListbox->DrawDeferred();
	} else {
		TInt top = iListbox->TopItemIndex();
		TInt bottom = iListbox->BottomItemIndex();
		TInt i;
		TBool redraw=EFalse;
		for (i=top; (i<=bottom) && !redraw; i++) {
			if (ContactId == this->iCallLog->GetContactId(i)) redraw=ETrue;
		}
		if (redraw) iListbox->DrawDeferred();
	}
}

TKeyResponse CContextCallLogContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("OfferKeyEventL"));

	if (aKeyEvent.iCode==JOY_LEFT || aKeyEvent.iCode == JOY_RIGHT)
	{
		return EKeyWasNotConsumed;
	} 
	        
	if ( iListbox )
        {
		if (aKeyEvent.iCode==JOY_DOWN || aKeyEvent.iCode == JOY_UP || aKeyEvent.iCode == JOY_CLICK)
		{
			return iListbox->OfferKeyEventL(aKeyEvent, aType);
		} 
		if ( (aKeyEvent.iCode==KEY_CALL) && (get_current_idx() != -1) )
		{
			aView->HandleCommandL(EContextCallLogCmdCall);
			return EKeyWasConsumed;
		}
		if ( (aKeyEvent.iCode==KEY_C) && (get_current_idx() != -1) )
		{
			aView->HandleCommandL(EContextCallLogCmdDelete);
			return EKeyWasConsumed;			
		}
		return iListbox->OfferKeyEventL(aKeyEvent, aType);
        }
	else
        {
		return EKeyWasNotConsumed;
        }
}

void CContextCallLogContainer::show_presence_details_current()
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("show_presence_details_current"));
	
	TInt current_index = get_current_idx();
	if (current_index < 0) return;

	contact * c = iCallLog->GetContact(current_index);
	if (c == NULL || c->presence == NULL) return;

	TBuf<128> name;

	if (c->first_name && c->first_name->Length()>0) name.Append(*(c->first_name));
	else if (c->last_name) name.Append(*c->last_name);
	
	name.Append(iCallLog->PresenceSuffix());

	if (aLog) 
	{
		aLog->write_time();
		aLog->write_to_output(_L("Showing pres. details of "));
		aLog->write_to_output(name);
		aLog->write_nl();
	}

	TTime atTime=iCallLog->GetAtTime(current_index);

	((CContextCallLogAppUi *)(iEikonEnv->AppUi()))->DisplayPresenceDetailsL(name, c->presence, atTime);
}

void CContextCallLogContainer::show_call_details_current()
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("show_call_details_current"));
	
	if (get_current_idx() != -1) {
		CLogEvent * ev = iCallLog->get_event(get_current_idx());

		if (!ev) return;

		TLocale locale;
		TTime tt=ev->Time();
		TTimeIntervalSeconds offset(locale.UniversalTimeOffset());
		tt+=offset;
		if (locale.QueryHomeHasDaylightSavingOn()) {
			TTimeIntervalHours ds(1);
			tt+=ds;
		}
		TDateTime t=tt.DateTime();
				
		HBufC * msg = CEikonEnv::Static()->AllocReadResourceL(R_CALL_DETAIL_TEMPLATE);
		CleanupStack::PushL(msg);
		HBufC * title = CEikonEnv::Static()->AllocReadResourceL(R_CALL_DETAIL_TITLE);
		CleanupStack::PushL(title);
		
		TBuf<500> temp;

		TBuf<20> number = _L(" ");
		number.Append(ev->Number());
		TBuf<50> remote_party = _L(" ");
		remote_party.Append(ev->RemoteParty());

		temp.Format(msg->Des(), &remote_party, 
					&number, 
					(TInt)t.Day()+1, 
					(TInt)t.Month()+1,
					(TInt)t.Hour(), 
					(TInt)t.Minute() );
		CAknMessageQueryDialog * note = CAknMessageQueryDialog::NewL(temp);
		note->SetHeaderTextL(*title);
		CleanupStack::PushL(note);
		note->ExecuteLD(R_CALL_DETAIL_DIALOG);
		CleanupStack::Pop(note);
		CleanupStack::PopAndDestroy(title);
		CleanupStack::PopAndDestroy(msg);
	}
}

TInt CContextCallLogContainer::get_current_idx()
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("get_current_idx"));
	
	if (iListbox->Model()->NumberOfItems())
	{
		return iListbox->CurrentItemIndex();
	}
	return -1;
}

void CContextCallLogContainer::handle_visibility()
{
	CALLSTACKITEM_N(_CL("CContextCallLogContainer"), _CL("handle_visibility"));
	
	if (iListbox->Model()->NumberOfItems() == 0)
	{
		iEmptyListboxLabel->SetSize(iEmptyListboxLabel->MinimumSize());
	}
	else
	{
		iListbox->View()->SetCurrentItemIndex(0);
		iEmptyListboxLabel->SetSize(TSize(0,0));
	}
}
