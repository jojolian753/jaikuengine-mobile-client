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

#include "TaskListContainer.h"

#include <eiktxlbm.h>
#include <e32svr.h>
#include <avkon.hrh>
#include "tasklist.hrh"
#include <eikenv.h>
#include <coeaui.h>
#include "tasklistappui.h"


// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CTaskListContainer::ConstructL(const TRect& aRect)
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CTaskListContainer::ConstructL(const TRect& aRect, MDesCArray* Contents, MEikListBoxObserver* ListBoxObserver)
{
	CreateWindowL();
	
	iContents=Contents;
	
	iListBox = new (ELeave) CEikTextListBox;
#ifndef __S80__
	iListBox->SetMopParent(this);
#endif
	iListBox->ConstructL(this, CEikListBox::EPaintedSelection|
		CEikListBox::EIncrementalMatching);
	
	iListBox->Model()->SetItemTextArray(Contents);
	iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);

	// S80 needs the next line to show a highlight on the current
	// item
	iListBox->View()->SetEmphasized(ETrue); 

	iListBox->MakeVisible(ETrue);
	iListBox->SetRect(aRect);
	iListBox->ActivateL();
	iListBox->DrawNow();
	
	iListBox->SetListBoxObserver(ListBoxObserver);
	
	iListBox->SetCurrentItemIndexAndDraw(Contents->MdcaCount()-1);	

	SetRect(aRect);
	ActivateL();
}

// Destructor
CTaskListContainer::~CTaskListContainer()
{
	delete iListBox;
}

// ---------------------------------------------------------
// CTaskListContainer::SizeChanged()
// Called by framework when the view size is changed
// ---------------------------------------------------------
//
void CTaskListContainer::SizeChanged()
{
	iListBox->SetRect(Rect());
}

// ---------------------------------------------------------
// CTaskListContainer::CountComponentControls() const
// ---------------------------------------------------------
//
TInt CTaskListContainer::CountComponentControls() const
{
	return 1; // return nbr of controls inside this container
}

// ---------------------------------------------------------
// CTaskListContainer::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CTaskListContainer::ComponentControl(TInt aIndex) const
{
	switch ( aIndex )
        {
        case 0:
		return iListBox;
        default:
		return NULL;
        }
}

// ---------------------------------------------------------
// CTaskListContainer::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CTaskListContainer::Draw(const TRect& aRect) const
{
	CWindowGc& gc = SystemGc();
	// TODO: Add your drawing code here
	// example code...
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
}

// ---------------------------------------------------------
// CTaskListContainer::HandleControlEventL(
//     CCoeControl* aControl,TCoeEvent aEventType)
// ---------------------------------------------------------
//
void CTaskListContainer::HandleControlEventL(
					     CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	// TODO: Add your control event handler code here
}

void CTaskListContainer::ContentsChanged()
{
	TRAPD(err,
		if (iListBox) iListBox->HandleItemRemovalL();
		iListBox->DrawNow());
}

TInt CTaskListContainer::GetCurrentIdx()
{
	return iListBox->CurrentItemIndex();
}

TKeyResponse CTaskListContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	if (aType==EEventKey && 
		(aKeyEvent.iCode==EKeyBackspace || aKeyEvent.iCode==EKeyDelete)) {
			((CEikAppUi*)iEikonEnv->AppUi())->HandleCommandL(ETaskListCmdAppKill);
	}
	return iListBox->OfferKeyEventL(aKeyEvent, aType);
}

// End of File  
