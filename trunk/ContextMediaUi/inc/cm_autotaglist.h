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

#ifndef CONTEXT_CM_AUTOTAGLIST_H_INCLUDED
#define CONTEXT_CM_AUTOTAGLIST_H_INCLUDED 1

#include <e32base.h>
#include <bamdesca.h>
#include <eikfrlbd.h> 

class MAskForNames {
public:
	virtual TBool NameCell(TInt aBaseId, const class TBBCellId& aCellId, TDes& aNameInto) = 0;
	virtual TBool NameCity(const class TBBCellId& aCellId, TDes& aNameInto) = 0;
	virtual TBool GetExistingCell(TInt aBaseId, const class TBBCellId& aCellId, TDes& aNameInto) = 0;
	virtual TBool GetExistingCity(const class TBBCellId& aCellId, TDes& aNameInto) = 0;
};

class CAutoTagArray : public CBase, public MDesCArray {
public:
	IMPORT_C static CAutoTagArray*	NewL(MAskForNames* aAskForNames=0,
		class CHintBox* aHints=0);

	virtual TInt GetIncludedTagsBitField() const = 0;
	virtual void ToggleField(TInt aIndex) = 0;
	virtual void ClearField(TInt aIndex) = 0;

	virtual TInt GetSharing() const = 0;
	virtual void SetPost(class CCMPost* aPost) = 0;
	virtual TInt GetIndexBit(TInt aIndex) const = 0;
	virtual TBool IsKnown(TInt aIndex) const = 0;
};

class CAutoTagListBox : public CCoeControl {
public:
	IMPORT_C static CAutoTagListBox* NewL(CAutoTagArray* aArray, 
		const CCoeControl* aParent, class CAknIconArray *aIconList,
		class CHintBox* aHintBox);

	virtual TInt RowHeight() = 0;
	virtual TInt Width() = 0;
	virtual TInt BorderWidth() = 0;
	IMPORT_C static class CAknIconArray * CreateIconList();
	virtual void SetCurrentItemIndexAndDraw(TInt aItemIndex) const = 0;
	virtual void SetTopItemIndex(TInt aItemIndex) const = 0;
	virtual TInt CurrentItemIndex() const = 0;
	virtual TInt BottomItemIndex() const = 0;
	virtual TInt TopItemIndex() const = 0;
	virtual void SetCurrentItemIndex(TInt aItemIndex) const = 0;
};

#endif
