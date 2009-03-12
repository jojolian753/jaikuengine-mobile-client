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
#include  <contextbook.rsg>
#include  "ContextbookView.h"
#include  "ContextbookContainer.h" 
#include  "Contextbook.hrh"
#include <aknquerydialog.h> 
#include <RPbkViewResourceFile.h>
#include <CPbkContactEditorDlg.h>
#include <CPbkContactEngine.h>
#include <CPbkSelectFieldDlg.h>
#include <CPbkContactItem.h>
#include "nickform.h"
#include <aknnavi.h>
#include <akntitle.h> 
#include <eikenv.h>
#include <akniconarray.h>

// ================= MEMBER FUNCTIONS =======================

/*
	if (id==KViewId) {
		aMenuPane->SetItemDimmed(EcontextbookCmdBook, ETrue);	
		aMenuPane->SetItemDimmed(EcontextbookCmdLog, EFalse);	
	} else {
		aMenuPane->SetItemDimmed(EcontextbookCmdBook, EFalse);	
		aMenuPane->SetItemDimmed(EcontextbookCmdLog, ETrue);
	}
*/

// ---------------------------------------------------------
// CContextbookView::ConstructL(const TRect& aRect)
// EPOC two-phased constructor
// ---------------------------------------------------------
//




//-------------
void CContextbookView::ConstructL(TUid i_id, phonebook_i* i_book, bool i_searchable, const TDesC& i_title, CAknIconArray * aIconlist)
{
	CALLSTACKITEM(_L("CContextbookView::ConstructL"));

	if (i_id==KViewId) 
	{
		BaseConstructL( R_CONTEXTBOOK_VIEW1 );
	}
	else
	{
		BaseConstructL( R_CONTEXTBOOK_VIEW2 );
	}
	id=i_id;
	book=i_book;
	searchable=i_searchable;
	title=i_title;
	iconlist = aIconlist;	
	
}

// ---------------------------------------------------------
// CContextbookView::~CContextbookView()
// ?implementation_description
// ---------------------------------------------------------
//
CContextbookView::~CContextbookView()
{
	CALLSTACKITEM(_L("CContextbookView::~CContextbookView"));

	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
	
}

// ---------------------------------------------------------
// TUid CContextbookView::Id()
// ?implementation_description
// ---------------------------------------------------------
//
TUid CContextbookView::Id() const
{
	CALLSTACKITEM(_L("CContextbookView::Id"));

	return id;
}

// ---------------------------------------------------------
// CContextbookView::HandleCommandL(TInt aCommand)
// ?implementation_description
// ---------------------------------------------------------
//
void CContextbookView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM(_L("CContextbookView::HandleCommandL"));

	switch ( aCommand )
        {
        case EAknSoftkeyBack:
		{
			AppUi()->HandleCommandL(EAknSoftkeyBack);
			break;
		}
	case EcontextbookCmdSMS:
		{
			iContainer->sms_current();
			//iLog->write_time();
			//iLog->write_to_output(_L("Cmd SendSMS"));
			//iLog->write_nl();
			break;
		}
	case EcontextbookCmdNick:
		{
			if (iLog) {
				iLog->write_time();
				iLog->write_to_output(_L("Cmd SetNick"));
				iLog->write_nl();
			}
			set_nick();
			break;
		}
	case EcontextbookCmdCall:
		{
			//iLog->write_time();
			//iLog->write_to_output(_L("Cmd Call"));
			//iLog->write_nl();
			iContainer->call_current();
			break;
		}
	case EcontextbookCmdPresenceDetails:
		{
			iContainer->show_presence_details_current();
		}
		break;

        default:
		{
			AppUi()->HandleCommandL( aCommand );
			break;
		}
    }
}

void CContextbookView::set_nick()
{
	CALLSTACKITEM(_L("CContextbookView::set_nick"));


	TInt idx=iContainer->get_current_idx();
	TInt contact=book->GetContactId(idx);

	CNickForm* f=CNickForm::NewL(iJabberData, contact, book->get_array()->MdcaPoint(idx), book);
	f->ExecuteLD();
}

// ---------------------------------------------------------
// CContextbookView::HandleClientRectChange()
// ---------------------------------------------------------
//
void CContextbookView::HandleClientRectChange()
{
	CALLSTACKITEM(_L("CContextbookView::HandleClientRectChange"));

	if ( iContainer )
        {
		iContainer->SetRect( ClientRect() );
        }
}

TInt CContextbookView::GetCurrentIdx()
{
	CALLSTACKITEM(_L("CContextbookView::GetCurrentIdx"));

	return iContainer->get_current_idx();
}

// ---------------------------------------------------------
// CContextbookView::DoActivateL(...)
// ?implementation_description
// ---------------------------------------------------------
//
void CContextbookView::DoActivateL(
				   const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/,
				   const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM(_L("CContextbookView::DoActivateL"));

	if (!iContainer)
        {
		iContainer = create_container();
		iContainer->SetMopParent(this);
		iContainer->ConstructL( ClientRect(), book, searchable, title, iLog, iconlist, current_item_index, top_item_index, current_edit_filter);
	}
	iContainer->MakeVisible(ETrue);
	
	AppUi()->AddToStackL( *this, iContainer );

}

// ---------------------------------------------------------
// CContextbookView::HandleCommandL(TInt aCommand)
// ?implementation_description
// ---------------------------------------------------------
//
void CContextbookView::DoDeactivate()
{
	CALLSTACKITEM(_L("CContextbookView::DoDeactivate"));

	if ( iContainer )
        {
			if (!exiting) 
			{
				current_item_index = iContainer->get_current_idx();
				top_item_index = iContainer->get_top_idx();
				iContainer->GetCurrentFilter(current_edit_filter);
			}
		AppUi()->RemoveFromViewStack( *this, iContainer );
		iContainer->MakeVisible(EFalse);
		delete iContainer;
		iContainer=0;
        }
	
}

CContextbookContainer* CContextbookView::create_container()
{
	CALLSTACKITEM(_L("CContextbookView::create_container"));

	return new (ELeave) CContextbookContainer;
}

void CContextbookView::before_exit()
{
	CALLSTACKITEM(_L("CContextbookView::before_exit"));

	exiting = true;
}

void CContextbookView::ResetSearchField()
{
	CALLSTACKITEM(_L("CContextbookView::ResetSearchField"));

	current_edit_filter.Zero();
	if (iContainer) iContainer->ResetSearchField();
	top_item_index=0;
	current_item_index=0;
}

// End of File

