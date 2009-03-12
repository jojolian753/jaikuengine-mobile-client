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

#include "cm_taglist.h"
#include <avkon.hrh>
#include <aknselectionlist.h> 
#include <aknsfld.h>
#include <txtrich.h>
#include "symbian_auto_ptr.h"
#include <contextmediaui.rsg>
#include "cm_tags.h"
#include <aknmessagequerydialog.h>

enum KEYCODES {
	KEY_C = 0x0008
};

class CAknMarkableListDialogEx : public CAknMarkableListDialog {
public:
	CTagStorage*	iTagStorage;
	CAknMarkableListDialogEx(TInt &aValue, CArrayFix< TInt > *aSelectedItems, 
		CTagStorage *aArray, TInt aMenuBarResourceId, TInt aOkMenuBarResourceId, 
		MEikCommandObserver *aObserver) :
	CAknMarkableListDialog (aValue, aSelectedItems, aArray, aMenuBarResourceId,
		aOkMenuBarResourceId, aObserver), iTagStorage(aArray) { }
	static CAknMarkableListDialogEx* NewL( TInt &aOpenedItem, CArrayFix< TInt > *aSelectedItems, 
		CTagStorage *aArray, 
		TInt aMenuBarResourceId, MEikCommandObserver *aCommand = 0 ) {
		CAknMarkableListDialogEx* ret=new (ELeave) CAknMarkableListDialogEx(aOpenedItem, aSelectedItems,
			aArray, aMenuBarResourceId, 0, aCommand );
		ret->ConstructL(aMenuBarResourceId);
		return ret;
	}

	void DeleteItemL() {
		TInt idx=ListBox()->CurrentItemIndex();

		auto_ptr<HBufC> message(HBufC::NewL(256));
		message->Des()=_L("Really delete ");
		MListBoxModel* lbmodel=ListBox()->Model();
		CAknFilteredTextListBoxModel* model = STATIC_CAST( CAknFilteredTextListBoxModel*, lbmodel);

		TPtrC item=model->ItemText( idx );
		message->Des().Append( item.Mid(2) );
		message->Des().Append(_L("?"));

		{
			TPtrC16 m=message->Des();
			CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(m);
			CleanupStack::PushL(dlg);
			dlg->PrepareLC(R_CONFIRMATION_QUERY);
			dlg->QueryHeading()->SetTextL(_L("Delete?"));
			CleanupStack::Pop(dlg);
			
			if ( dlg->RunLD() )
			{
				TInt realidx=model->Filter()->FilteredItemIndex(idx);
				iTagStorage->DeleteItemL(realidx);
				ListBox()->HandleItemRemovalL();
			}
		}	
	}

	virtual TBool OkToExitL  (  TInt  aButtonId   )  {
		if (aButtonId==-2) {
			const CArrayFix<TInt> *indexes = ListBox()->SelectionIndexes();
			if (indexes->Count()==0) {
				SelectionListProcessCommandL(EAknCmdMark);
			}
		}
		return CAknMarkableListDialog::OkToExitL(aButtonId);
	}
	virtual TKeyResponse OfferKeyEventL  (  const TKeyEvent &  aKeyEvent,  
		TEventCode  aType )
	{
		if (aType==EEventKey && aKeyEvent.iCode==0xF845) {
			TInt idx=ListBox()->CurrentItemIndex();
			if (idx!=-1) {
				TBool selected=EFalse;
				const CArrayFix<TInt> *indexes = ListBox()->SelectionIndexes();
				if (indexes) {
					for (int i=0; i<indexes->Count(); i++) {
						if (indexes->At(i)==idx) {
							selected=ETrue;
							break;
						}
					}
				}
				if (!selected) {
					SelectionListProcessCommandL(EAknCmdMark);
				} else {
					SelectionListProcessCommandL(EAknCmdUnmark);
				}
			}
			return EKeyWasConsumed;
		} else {
			if (aType==EEventKey && aKeyEvent.iCode==KEY_C && FindBox()->TextLength()==0) {
				DeleteItemL();
				return EKeyWasConsumed;
			} else {
				return CAknMarkableListDialog::OfferKeyEventL(aKeyEvent, aType);
			}
		}
	}
};

class CListSearch : public CActive {
public:
	CAknMarkableListDialogEx* dlg;
	CListSearch() : CActive(CActive::EPriorityStandard) { }
	void ConstructL() {
		CActiveScheduler::Add(this);
		TRequestStatus *s=&iStatus;
		User::RequestComplete(s, KErrNone);
		SetActive();
	}
	void DoCancel() { }
	TBuf<50> iSearchText;
	void RunL() {
		for (int i=0; i<iSearchText.Length(); i++) {
			TKeyEvent ev; 
			ev.iCode=iSearchText[i];
			TEventCode c(EEventKey);
			dlg->OfferKeyEventL(ev, c);
		}
	}
	~CListSearch() {
		Cancel();
	}
};

EXPORT_C TBool SelectTagsFromListL(class CTagStorage& aStorage, class CRichText* aText, TInt& aAddedFrom, 
				   TInt aSelectionStart, TInt aSelectionEnd)
{
	TPtrC text1=aText->Read(0);
	TPtrC text(0, 0);
	auto_ptr<HBufC> buf(0);
	if (aSelectionStart>=0 && aSelectionEnd>aSelectionStart) {
		buf.reset(HBufC::NewL(text1.Length()));
		buf->Des().Append(text1.Left(aSelectionStart));
		buf->Des().Append(text1.Mid(aSelectionEnd, text1.Length()-aSelectionEnd-1));
		text.Set(buf->Des());
	} else {
		text.Set(text1.Ptr(), text1.Length()-1); // ignore end-of-text character
	}

	TInt iSelectedItem=0;
	auto_ptr<CListSearch> s(new (ELeave) CListSearch);
	auto_ptr< CArrayFixFlat<TInt> > items(new (ELeave) CArrayFixFlat<TInt>(4));
	CAknMarkableListDialogEx  * dlg=CAknMarkableListDialogEx::NewL(iSelectedItem, items.get(),
		&aStorage, R_LIST_MENUBAR);
	dlg->PrepareLC(R_LIST_DIALOG);

	s->ConstructL(); s->dlg=dlg;
	TBool add_colon=ETrue;
	if (text.Length()==0 || text[text.Length()-1]==':' || text[text.Length()-1]==':') add_colon=EFalse;
	TInt colon_pos=KErrNotFound;
	/*
	if (colon_pos!=KErrNotFound && colon_pos!=text.Length()-1) {
		TInt len=text.Length()-colon_pos-1;
		if (len>50) len=50;
		s->iSearchText=text.Mid(colon_pos+1, len);
		s->iSearchText.Trim();
		if (aStorage.TagExists(s->iSearchText)) {
			add_colon=ETrue;
			s->iSearchText.Zero();
			colon_pos=KErrNotFound;
		}
	} else if (colon_pos==KErrNotFound) {
		TInt len=text.Length();
		if (len>50) len=50;
		s->iSearchText=text.Mid(0, len);
		s->iSearchText.Trim();
		if (aStorage.TagExists(s->iSearchText)) {
			add_colon=ETrue;
			s->iSearchText.Zero();
		}
	} */
	
	if (dlg->RunLD()) {
		if (items->Count()>0) {
			if (aSelectionStart>=0 && aSelectionEnd>aSelectionStart) {
				aText->DeleteL(aSelectionStart, aSelectionEnd-aSelectionStart);
			}
			if (colon_pos!=KErrNotFound && colon_pos!=text.Length()-1) {
				aText->DeleteL(colon_pos+1, text.Length()-colon_pos-1);
			}
			if (colon_pos==KErrNotFound && s->iSearchText.Length()>0) {
				aText->DeleteL(0, aText->DocumentLength());
			}
			aAddedFrom=aText->DocumentLength();
			for (int i=0; i<items->Count(); i++) {
				if (add_colon) {
					aText->InsertL(aText->DocumentLength(), _L(":"));
				}
				// removing "0\t" from the tag
				aText->InsertL(aText->DocumentLength(), 
					aStorage.MdcaPoint(items->At(i)).Mid(2));
				add_colon=ETrue;
			}
			aText->InsertL(aText->DocumentLength(), _L(":"));
			return ETrue;
		} else {
			return EFalse;
		}
	} else {
		return EFalse;
	}
}
