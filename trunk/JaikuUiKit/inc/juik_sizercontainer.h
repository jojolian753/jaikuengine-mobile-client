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

#ifndef JUIK_SIZERCONTAINER_H
#define JUIK_SIZERCONTAINER_H

#include "juik_control.h"
#include "juik_sizer.h"
#include "app_context.h"

#include <coecntrl.h>

/**
 * SizerContainer is a convenience control class that takes care of ownership of a bunch of sizers and controls
 * All controls are direct children of SizerContainer, but they are layouted by hierarchy of sizers. 
 * 
 * Sizers and controls can be accessed through identifiers. 
 *
 * Adding controls to sizers has to be done outside of this class, there is no point of replicating 
 * MJuikSizer interface in this class. Thus usage pattern: 
 * 
 * CJuikSizerContainer* c;
 * c->SetRootSizerL( CreateHSizerL() );
 *
 * MJuikSizer* leftsizer = CreateSizer2L();
 * c->AddSizerL(leftsizer, ELeftSideSizer)
 * c->GetRootSizerL()->AddL( leftsizer, 1, Juik::EExpand)
 *
 * c->AddControlL( CreateImageL(), EMyImage );
 * c->GetSizerL(ELeftSideSizer)->AddL( c->GetControlL( EMyImage ), 1, Juik::EExpandNot );
 * 
 * c->AddControlL( CreateImage2L(), ERightSideImage );
 * c->GetRootSizerL()->AddL( c->GetControlL( ERightSideImage ), 1, Juik::EExpandNot );
 */ 
class CJuikSizerContainer : public CCoeControl, public MJuikControl
{
 public:
	IMPORT_C static CJuikSizerContainer* NewL();
	
	virtual ~CJuikSizerContainer() {}
	virtual void SetRootSizerL(MJuikSizer* aSizer) = 0;
	virtual MJuikSizer* GetRootSizerL() = 0;
	
	virtual MJuikSizer* GetSizerL(TInt aSizerId) = 0;
	virtual MJuikControl* GetControlL(TInt aId) = 0;
	
	virtual void AddControlL(MJuikControl* aControl, TInt aControlId) = 0;
	virtual void AddSizerL(MJuikSizer* aSizer, TInt aSizerId) = 0;
	virtual void RemoveControlL( MJuikControl* aControl ) = 0;
	
	virtual void SetDebugId(TInt aDebugId) = 0;
};


/**
 * CJuikContainerBase provides convenience base class to derive from, if you 
 * use CJuikSizerContainer as your single root container. It provides necessary implementations
 * for SizeChanged and similar framework methods to make SizerContainer work. In addition it provides 
 * support for margins. Margins are defaulted to 0.
 * 
 * If you use CJuikContainerBase, you normally shouldn't override any of CCoeControl-methods
 * defined in this class, or if you do, remember to call implementation in this class also. Remember to call
 * BaseConstructL also.
 *
 * CJuikSizerContainer is still provided as a separate delegate object, and it's 
 * API is used to actually manage controls. 
 */ 

class CJuikContainerBase : public CCoeControl, public MJuikControl, public MContextBase 
{
 public:
	IMPORT_C CJuikContainerBase();
	IMPORT_C virtual ~CJuikContainerBase();
	IMPORT_C void BaseConstructL();
		
	IMPORT_C CJuikSizerContainer& RootContainer() const;


	IMPORT_C virtual void FocusChanged(TDrawNow aDrawNow);
	IMPORT_C virtual TSize MinimumSize();
	IMPORT_C virtual TInt CountComponentControls() const;
	IMPORT_C virtual CCoeControl* ComponentControl(TInt /*aIndex*/) const;
	IMPORT_C virtual void PositionChanged();
	IMPORT_C virtual void SizeChanged();
	
	IMPORT_C void Draw(const TRect& aRect) const;
	IMPORT_C void SetContainerWindowL(const CCoeControl& aControl);
	
 protected:
	CJuikSizerContainer* iContainer;
	TMargins8 iMargin;
};

#endif // JUIK_SIZERCONTAINER_H
