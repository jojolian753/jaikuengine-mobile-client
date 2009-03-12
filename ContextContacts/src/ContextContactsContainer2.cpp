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

#include "ContextContactsContainer2.h"

#include <eiklabel.h>  // for example label control
#include "app_context.h"

// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CContextContactsContainer2::ConstructL(const TRect& aRect)
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CContextContactsContainer2::ConstructL(const TRect& aRect)
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer2"), _CL("ConstructL"));

	CreateWindowL();
	
	iLabel = new (ELeave) CEikLabel;
	iLabel->SetContainerWindowL( *this );
	iLabel->SetTextL( _L("Group View") );
	
	iToDoLabel = new (ELeave) CEikLabel;
	iToDoLabel->SetContainerWindowL( *this );
	iToDoLabel->SetTextL( _L("Feature not available") );
	
	SetRect(aRect);
	ActivateL();
}

// Destructor
CContextContactsContainer2::~CContextContactsContainer2()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer2"), _CL("~CContextContactsContainer2"));

	delete iLabel;
	delete iToDoLabel;
}

// ---------------------------------------------------------
// CContextContactsContainer2::SizeChanged()
// Called by framework when the view size is changed
// ---------------------------------------------------------
//
void CContextContactsContainer2::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer2"), _CL("SizeChanged"));

	// TODO: Add here control resize code etc.
	iLabel->SetExtent( TPoint(10,10), iLabel->MinimumSize() );
	iToDoLabel->SetExtent( TPoint(10,100), iToDoLabel->MinimumSize() );
}

// ---------------------------------------------------------
// CContextContactsContainer2::CountComponentControls() const
// ---------------------------------------------------------
//
TInt CContextContactsContainer2::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer2"), _CL("CountComponentControls"));

	return 2; // return nbr of controls inside this container
}

// ---------------------------------------------------------
// CContextContactsContainer2::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CContextContactsContainer2::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer2"), _CL("ComponentControl"));

	switch ( aIndex )
        {
        case 0:
		return iLabel;
        case 1:
		return iToDoLabel;
        default:
		return NULL;
        }
}

// ---------------------------------------------------------
// CContextContactsContainer2::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CContextContactsContainer2::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer2"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
	// TODO: Add your drawing code here
	// example code...
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
}

// ---------------------------------------------------------
// CContextContactsContainer2::HandleControlEventL(
//     CCoeControl* aControl,TCoeEvent aEventType)
// ---------------------------------------------------------
//
void CContextContactsContainer2::HandleControlEventL(
						     CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	CALLSTACKITEM_N(_CL("CContextContactsContainer2"), _CL("HandleControlEventL"));

	// TODO: Add your control event handler code here
}

// End of File  
