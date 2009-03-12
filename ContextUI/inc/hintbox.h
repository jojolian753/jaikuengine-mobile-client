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

#ifndef CONTEXT_HINTBOX_H_INCLUDED
#define CONTEXT_HINTBOX_H_INCLUDED 1

#include <e32base.h>
#include <coecntrl.h>

class CTextBox : public CCoeControl {
	/* This class will draw a floating window with some
	   text, nicely word-wrapped. You can anchor to window
	   to any of the for corners. It'll calculate the minimum
	   size needed based on the maximum width given, and use
	   that */
public:
	enum TPosition {
		ETopLeft,
		ETopRight,
		EBottomLeft,
		EBottomRight
	};
	IMPORT_C static CTextBox* NewL(TRgb aBackgroundColor, TRgb aForegroundColor,
			TRgb aBorderColor, TInt aBorder, TInt aMargin);
	/* doesn't take ownership of the text, you need to make
	   sure it's lifetime is long enough*/
	virtual void SetText(TInt aMaxWidth, HBufC* aText, TPosition aPosFrom,
			TPoint aPos) = 0;
	virtual void Reset() = 0;
};

class CHintBox : public CBase {
public:
	IMPORT_C static CHintBox* NewL(const struct TTupleName& aTupleName,
		TInt aScale);
	virtual void AddHintL(TInt aHintId, TInt aPriority,
		CTextBox::TPosition aPosFrom, const class TPoint& aPos,
		TInt aMaxWidth,
		TInt aHintResourceId) = 0;
	virtual void ShowHintIfNotDismissed(TInt aHintId) = 0;
	virtual void DontShow() = 0;
	virtual void DismissHint(TInt aHintId) = 0;
};

#endif
