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

#include "DbtestContainer.h"

#include <eiklabel.h>  // for example label control

#include <d32dbms.h>
#include <eikenv.h>
//#include <RCtmGsmPhone.h>

// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CDbtestContainer::ConstructL(const TRect& aRect)
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CDbtestContainer::ConstructL(const TRect& aRect)
{
	CreateWindowL();
	
	for (int i=0; i<6; i++) {
		rows[i] = new (ELeave) CEikLabel;
		rows[i]->SetContainerWindowL( *this );
		rows[i]->SetTextL( _L("         ") );
	}

	
	/*
	rows[0]->SetTextL(_L("datetime"));
	rows[1]->SetTextL(_L("current cell"));
	rows[2]->SetTextL(_L("actual next base"));
	rows[3]->SetTextL(_L("predicted base"));
	rows[4]->SetTextL(_L("accuracy sensitivity"));
	*/

	SetRect(aRect);
	ActivateL();
}

void CDbtestContainer::date(const TDesC& str) const
{
	rows[0]->SetTextL(str);
	DrawNow();
}

void CDbtestContainer::cell(const TDesC& str) const
{
	rows[1]->SetTextL(str);
	DrawNow();
}

void CDbtestContainer::next_base(const TDesC& str) const
{
	rows[2]->SetTextL(str);
	DrawNow();
}

void CDbtestContainer::prediction(const TDesC& str) const
{
	rows[3]->SetTextL(str);
	DrawNow();
}

void CDbtestContainer::correct(const TDesC& str) const
{
	rows[4]->SetTextL(str);
	DrawNow();
}

void CDbtestContainer::stat(const TDesC& str) const
{
	rows[5]->SetTextL(str);
	DrawNow();
}


// Destructor
CDbtestContainer::~CDbtestContainer()
{
	for (int i=0; i<6; i++) {
		delete rows[i];
	}
}

// ---------------------------------------------------------
// CDbtestContainer::SizeChanged()
// Called by framework when the view size is changed
// ---------------------------------------------------------
//
void CDbtestContainer::SizeChanged()
{
	// TODO: Add here control resize code etc.
	for (int i=0; i<6; i++) {
		rows[i]->SetExtent( TPoint(10, 10+i*16), TSize(150, 14) );
	}
}

// ---------------------------------------------------------
// CDbtestContainer::CountComponentControls() const
// ---------------------------------------------------------
//
TInt CDbtestContainer::CountComponentControls() const
{
	return 6; // return nbr of controls inside this container
}

// ---------------------------------------------------------
// CDbtestContainer::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CDbtestContainer::ComponentControl(TInt aIndex) const
{
	if (aIndex < 6) return rows[aIndex];
	return 0;
}

// ---------------------------------------------------------
// CDbtestContainer::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CDbtestContainer::Draw(const TRect& aRect) const
{
	CWindowGc& gc = SystemGc();
	// TODO: Add your drawing code here
	// example code...
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbGray);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
}

// ---------------------------------------------------------
// CDbtestContainer::HandleControlEventL(
//     CCoeControl* aControl,TCoeEvent aEventType)
// ---------------------------------------------------------
//
void CDbtestContainer::HandleControlEventL(
					   CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	// TODO: Add your control event handler code here
}

void CDbtestContainer::set_status(const TDesC& status)
{
	rows[0]->SetTextL(status);
	DrawNow();
}

void CDbtestContainer::set_error(const TDesC& status)
{
	rows[1]->SetTextL(status);
	DrawNow();
}

// End of File  
