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

#ifndef CONTEXTMENUCONTAINER_H
#define CONTEXTMENUCONTAINER_H

#include <badesca.h>
#include <aknpopup.h>
#include <aknlists.h>
#include "contextmenu.hrh"
#include "app_context.h"

class CContextMenuArray : public CBase, public MDesCArray, public MContextBase {
public:
	enum TSelectionState {ENone, ESelected, ENotSelected};
public:
	CContextMenuArray(MApp_context& Context) : MContextBase(Context){ }
	void ConstructL();
	void AppendL(TInt aCommandId, const TDesC& aItemText, TBool aHasSubMenu=EFalse, 
		TSelectionState selection=ENone, TInt aSelectCommandId=0, TInt aUnselectCommandId=0);
	~CContextMenuArray();
public:
	virtual TInt MdcaCount() const;
	virtual TPtrC16 MdcaPoint(TInt aIndex) const;
public:
	TInt GetCommand(TInt aIndex);
	TInt GetSelectCommand(TInt aIndex);
	TInt GetUnselectCommand(TInt aIndex);
	TBool HasSubMenu(TInt aIndex);
	TSelectionState GetSelectionState(TInt aIndex);
	void SetSelectionState(TInt aIndex, TSelectionState s);
private:
	CPtrC16Array * iCommandTextArray;
	CArrayFixFlat<TInt>* iCommandIdArray;
	CArrayFixFlat<TInt>* iSelectCommandIdArray;
	CArrayFixFlat<TInt>* iUnselectCommandIdArray;
	CArrayFixFlat<TBool>* iCommandHasSubMenuArray;
	CArrayFixFlat<TSelectionState>* iCommandSelectionStateArray;
	HBufC * iBuf;
};


class CContextMenuPopupMenuStyleListBox: public CAknSingleGraphicPopupMenuStyleListBox {
public:
	CContextMenuPopupMenuStyleListBox(CContextMenuArray* aContextMenuArray, TBool aIsSubMenu) : 
			CAknSingleGraphicPopupMenuStyleListBox(), 
			iContextMenuArray(aContextMenuArray), iIsSubMenu(aIsSubMenu)
			{ }

	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	~CContextMenuPopupMenuStyleListBox();
private:
	CContextMenuArray * iContextMenuArray;
	TBool iIsSubMenu;
};
#endif

