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

#include "ContextMenuContainer.h"
#include "cl_settings.h"
#include "symbian_auto_ptr.h"

TInt CContextMenuArray::GetCommand(TInt aIndex)
{
	return iCommandIdArray->At(aIndex);
}

TInt CContextMenuArray::GetSelectCommand(TInt aIndex)
{
	return iSelectCommandIdArray->At(aIndex);
}

TInt CContextMenuArray::GetUnselectCommand(TInt aIndex)
{
	return iUnselectCommandIdArray->At(aIndex);
}

void CContextMenuArray::SetSelectionState(TInt aIndex, TSelectionState s)
{
	iCommandSelectionStateArray->At(aIndex)=s;
}

CContextMenuArray::TSelectionState CContextMenuArray::GetSelectionState(TInt aIndex)
{
	return iCommandSelectionStateArray->At(aIndex);
}

TBool CContextMenuArray::HasSubMenu(TInt aIndex)
{
	return iCommandHasSubMenuArray->At(aIndex);
}

TInt CContextMenuArray::MdcaCount() const
{
	return iCommandTextArray->Count();
}

TPtrC16 CContextMenuArray::MdcaPoint(TInt aIndex) const
{
	if (aIndex>MdcaCount()) {
		User::Leave(KErrArgument);
	}
	iBuf->Des().Zero();
	switch(iCommandSelectionStateArray->At(aIndex)) {
		case ESelected:
			iBuf->Des().AppendNum(1);
			break;
		case ENotSelected:
			iBuf->Des().AppendNum(2);
			break;
		case ENone:
		default:
			iBuf->Des().AppendNum(0);
			break;
	}
	iBuf->Des().Append(_L("\t"));
	iBuf->Des().Append(iCommandTextArray->At(aIndex));
	iBuf->Des().Append(_L("\t"));

	if (iCommandHasSubMenuArray->At(aIndex)) {
		iBuf->Des().AppendNum(3);
	} else {
		iBuf->Des().AppendNum(0);
	}
	return iBuf->Des();
}

void CContextMenuArray::AppendL(TInt aCommandId, const TDesC& aCommandText, TBool aHasSubMenu, 
				TSelectionState selection, TInt aSelectCommandId, TInt aUnselectCommandId)
{
	iCommandIdArray->AppendL(aCommandId);
	iSelectCommandIdArray->AppendL(aSelectCommandId);
	iUnselectCommandIdArray->AppendL(aUnselectCommandId);
	iCommandTextArray->AppendL(aCommandText);
	iCommandSelectionStateArray->AppendL(selection);
	iCommandHasSubMenuArray->AppendL(aHasSubMenu);
}	

CContextMenuArray::~CContextMenuArray()
{
	delete iCommandIdArray;
	delete iSelectCommandIdArray;
	delete iUnselectCommandIdArray;
	delete iCommandTextArray;
	delete iCommandSelectionStateArray;
	delete iCommandHasSubMenuArray;
	delete iBuf;
}


void CContextMenuArray::ConstructL()
{
	iCommandTextArray = new (ELeave) CPtrC16Array(5);
	iCommandIdArray = new (ELeave) CArrayFixFlat<TInt>(5);
	iSelectCommandIdArray = new (ELeave) CArrayFixFlat<TInt>(5);
	iUnselectCommandIdArray = new (ELeave) CArrayFixFlat<TInt>(5);
	iCommandSelectionStateArray = new (ELeave) CArrayFixFlat<TSelectionState>(5);
	iCommandHasSubMenuArray = new (ELeave) CArrayFixFlat<TBool>(5);
	iBuf = HBufC::NewL(256);
}

//----------------------------------------------------------------------

CContextMenuPopupMenuStyleListBox::~CContextMenuPopupMenuStyleListBox()
{
	ItemDrawer()->FormattedCellData()->SetIconArray(0); 
}

TKeyResponse CContextMenuPopupMenuStyleListBox::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	if (aKeyEvent.iCode == EKeyRightArrow) {
		if ( iContextMenuArray->HasSubMenu(CurrentItemIndex()) ) {
			// execute command
			CEikonEnv::Static()->EikAppUi()->HandleCommandL( iContextMenuArray->GetCommand(CurrentItemIndex()) );
			return EKeyWasConsumed;
		} 
	}
	if (aKeyEvent.iCode == EKeyLeftArrow) {
		if (iIsSubMenu) {
			// close Popup by simulating a right softkey
			TKeyEvent e = aKeyEvent; e.iCode = EKeyOK;
			return CEikFormattedCellListBox::OfferKeyEventL(e, aType);
		} 
	}

	if (aKeyEvent.iCode == EKeyOK) {
                CContextMenuArray::TSelectionState s = iContextMenuArray->GetSelectionState(CurrentItemIndex());
		TInt command_id=0;
		TBool close_menu=EFalse;
		switch(s) {
			case CContextMenuArray::ESelected:
				iContextMenuArray->SetSelectionState(CurrentItemIndex(), CContextMenuArray::ENotSelected);
				command_id = iContextMenuArray->GetUnselectCommand(CurrentItemIndex());
				DrawDeferred();
				break;
			case CContextMenuArray::ENotSelected:
				iContextMenuArray->SetSelectionState(CurrentItemIndex(), CContextMenuArray::ESelected);
				command_id = iContextMenuArray->GetSelectCommand(CurrentItemIndex());
				DrawDeferred();
				break;
			case CContextMenuArray::ENone:
				command_id = iContextMenuArray->GetCommand(CurrentItemIndex());
				close_menu = ETrue;
			default:
				break;
		}
		CEikonEnv::Static()->EikAppUi()->HandleCommandL( command_id );
		if (close_menu) return CEikFormattedCellListBox::OfferKeyEventL(aKeyEvent, aType);
		return EKeyWasConsumed;
	}
	return CEikFormattedCellListBox::OfferKeyEventL(aKeyEvent, aType);
}
