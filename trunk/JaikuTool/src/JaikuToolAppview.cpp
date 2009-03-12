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

#include <coemain.h>
#include <JaikuTool.rsg>

#include "JaikuToolAppView.h"

// Standard construction sequence
CJaikuToolAppView* CJaikuToolAppView::NewL(const TRect& aRect)
    {
    CJaikuToolAppView* self = CJaikuToolAppView::NewLC(aRect);
    CleanupStack::Pop(self);
    return self;
    }

CJaikuToolAppView* CJaikuToolAppView::NewLC(const TRect& aRect)
    {
    CJaikuToolAppView* self = new (ELeave) CJaikuToolAppView;
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    return self;
    }

CJaikuToolAppView::CJaikuToolAppView()
    {
	// no implementation required
    }

#include <eikedwin.h>

CJaikuToolAppView::~CJaikuToolAppView()
    {
	delete iText;
    }

CCoeControl* CJaikuToolAppView::ComponentControl(TInt aIndex) const
{
	return iText;
}

TInt CJaikuToolAppView::CountComponentControls() const
{
	return 1;
}

#include <akndef.h>
#include <eikenv.h>
#include <eikappui.h>

void CJaikuToolAppView::HandleResourceChange( TInt aType )
{
	CCoeControl::HandleResourceChange(aType);
	if ( aType == KEikDynamicLayoutVariantSwitch ) {
		CEikAppUi* appui=(CEikAppUi*)iEikonEnv->AppUi();
		TRect r = appui->ClientRect();
		SetRect( r );
	}
}

_LIT(KText, "Select Options to repair, disable or report bugs about Jaiku");
void CJaikuToolAppView::ConstructL(const TRect& aRect)
    {
    // Create a window for this application view
    CreateWindowL();

	iText=new (ELeave) CEikEdwin;
	iText->ConstructL();
	iText->SetContainerWindowL( *this );
    SetRect(aRect);
	
	iText->SetFocusing(EFalse);
	iText->SetWordWrapL(ETrue);
	iText->SetTextL(&KText());
	iText->ActivateL();
	
    // Activate the window, which makes it ready to be drawn
    ActivateL();
    }

void CJaikuToolAppView::SizeChanged()
{
	TRect t=Rect();
	t.Move(10, 10);
	t.Resize(-20, -20);
	iText->SetRect(t);
}

// Draw this application's view to the screen
void CJaikuToolAppView::Draw(const TRect& /*aRect*/) const
    {
    // Get the standard graphics context 
    CWindowGc& gc = SystemGc();
    
    // Gets the control's extent
    TRect rect = Rect();
    
    // Clears the screen
    gc.Clear(rect);
    }


