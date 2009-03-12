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

#ifndef JUIK_LABEL_H
#define JUIK_LABEL_H

#include "app_context.h"
#include "juik_debug.h"
#include "juik_control.h"

#include <eiklabel.h>

class CJuikLabel : public CEikLabel, public MJuikControl, public MContextBase
{
public:
	IMPORT_C static CJuikLabel* NewL(TRgb aColor, TRgb aFocusColor, const CFont* aFont, CCoeControl& aParent);

	IMPORT_C CJuikLabel();
	IMPORT_C void SetDebugId(TInt aDebugId);

	IMPORT_C virtual ~CJuikLabel();

	IMPORT_C void SetColorsL(const TRgb& aNormal, const TRgb& aHighlight);
	IMPORT_C void EnableFocusBackgroundL( const TRgb& aColor );
	/** 
	 * Use this method instead of SetFont of CEikLabel, 
	 * otherwise size of CEikLabel is calculated incorrectly
	 */ 
	IMPORT_C void SetFontProperlyL(const CFont* aFont);
	
	IMPORT_C void ZeroL();
	IMPORT_C void UpdateTextL( const TDesC& aText );
	IMPORT_C void SetWrappedTextL(const TDesC& aText);
	
public: // from CEikLabel
	virtual void FocusChanged(TDrawNow aDrawNow);
	
	virtual TSize MinimumSize();
 protected:
	virtual	void SizeChanged();
	virtual void Draw(const TRect& aRect) const;

 protected: // own
	virtual void WrapTextL(TBool aForced = EFalse );

	void SetHighlightColorL( const TRgb& aColor );
	void SetNormalColorL( const TRgb& aColor );

 public: // from MJuikControl
	const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }

 private:

	HBufC* iUnwrappedText;
	TInt iWrapWidth;
	
	TRgb iNormalColor;
	TRgb iHighlightColor;

	TInt iRealFontDescent;

	TBool iAlreadyWrapped;

	void DebugPrintRect(const TDesC& aMsg, const TRect& aRect) const;
	void DebugPrintSize(const TDesC& aMsg, TSize& aSize) const;

	TInt iDebugId;

	TBool iUseBgFocus;
	TRgb  iBgFocusColor;
};


#endif // JUIK_LABEL_H
