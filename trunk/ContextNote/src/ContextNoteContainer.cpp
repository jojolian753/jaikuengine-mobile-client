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

#include "ContextNoteContainer.h"

#include <eiklabel.h>  // for example label control

// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CContextNoteContainer::ConstructL(const TRect& aRect)
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CContextNoteContainer::ConstructL(const TRect& aRect)
{
	CreateWindowL();
	
	iEdit=new (ELeave) CEikEdwin;
	iEdit->SetContainerWindowL( *this );
	iEdit->ConstructL();
	iEdit->AddEdwinObserverL(this);

	TRect r(aRect);
	r.Move(4, 4);
	r.Resize(-8, -8);
	iEdit->SetRect(r);
	iEdit->ActivateL();
	iEdit->SetFocus(ETrue);
	
	SetRect(aRect);
	ActivateL();
}

// Destructor
CContextNoteContainer::~CContextNoteContainer()
{
	delete iEdit;
	delete iBuf;
}

// ---------------------------------------------------------
// CContextNoteContainer::SizeChanged()
// Called by framework when the view size is changed
// ---------------------------------------------------------
//
void CContextNoteContainer::SizeChanged()
{
	TRect r(Rect());
	r.Move(4, 4);
	r.Resize(-8, -8);
	iEdit->SetRect(r);
}

// ---------------------------------------------------------
// CContextNoteContainer::CountComponentControls() const
// ---------------------------------------------------------
//
TInt CContextNoteContainer::CountComponentControls() const
{
	return 1; // return nbr of controls inside this container
}

// ---------------------------------------------------------
// CContextNoteContainer::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CContextNoteContainer::ComponentControl(TInt aIndex) const
{
	switch ( aIndex )
        {
        case 0:
		return iEdit;
        default:
		return NULL;
        }
}

// ---------------------------------------------------------
// CContextNoteContainer::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CContextNoteContainer::Draw(const TRect& aRect) const
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
// CContextNoteContainer::HandleControlEventL(
//     CCoeControl* aControl,TCoeEvent aEventType)
// ---------------------------------------------------------
//
void CContextNoteContainer::HandleControlEventL(
						CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	// TODO: Add your control event handler code here
}

TKeyResponse CContextNoteContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	return iEdit->OfferKeyEventL(aKeyEvent, aType);
}

void CContextNoteContainer::HandleEdwinEventL(CEikEdwin* /*aEdwin*/, TEdwinEvent /*aEventType*/)
{
}

const TDesC& CContextNoteContainer::GetText()
{
	delete iBuf; iBuf=0;
	iBuf=HBufC::NewL(iEdit->TextLength());
	TPtr p=iBuf->Des();
	iEdit->GetText(p);
	return *iBuf;
}

// End of File
