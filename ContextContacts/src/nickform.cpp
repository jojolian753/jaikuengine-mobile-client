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

#include "break.h"
#include "nickform.h"
#include "symbian_auto_ptr.h"
#include <contextcontacts.rsg>
#include <avkon.hrh>
#include <eikedwin.h>
#include <eiklabel.h>
#include <AknPopupFieldText.h>

CNickForm* CNickForm::NewL(CJabberData& Data, TInt ContactId, 
						   const TDesC& Title, phonebook_i* Book,	MPresenceFetcher* aFetcher,
						   TBool aEditable)
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("NewL"));


	auto_ptr<CNickForm> self (
		new (ELeave) CNickForm(Data, ContactId, Book, aFetcher, aEditable));
	self->ConstructL(Title);
	return self.release();
}

CNickForm::~CNickForm()
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("~CNickForm"));


	delete iTitle;
}
CNickForm::CNickForm(CJabberData& Data, TInt ContactId, phonebook_i* Book, MPresenceFetcher* aFetcher, TBool aEditable) : 
	iData(Data), iContactId(ContactId), iBook(Book), iFetcher(aFetcher), iEditable(aEditable) { }

void CNickForm::ConstructL(const TDesC& Title)
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("ConstructL"));

	iTitle=Title.AllocL();
	iData.GetJabberNickL(iContactId, iNick);
	iOldNick=iNick;
	iShowDetails=iData.GetShowDetailsInListL(iContactId); 
	CAknForm::ConstructL();
}

TInt CNickForm::ExecuteLD()
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("ExecuteLD"));

	PrepareLC();
	return RunLD();

}
void CNickForm::PrepareLC()
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("PrepareLC"));

	if (iEditable) {
		CAknForm::PrepareLC(R_CB_NICK_DIALOG);
	} else {
		CAknForm::PrepareLC(R_CB_NICK_DIALOG_READONLY);
	}
}

TBool CNickForm::SaveFormDataL()
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("SaveFormDataL"));

	if (!iEditable) return true;

	GetEdwinText(iNick, 2);

	iShowDetails=ETrue;
			
	iData.SetJabberNickL(iContactId, iNick, CJabberData::ESetByUser);
	iData.SetShowDetailsInListL(iContactId, iShowDetails);
	
	// order matters: to get the dummy item right when
	// deleting our own nick, the book has to be re-read
	// after we change the nick
	if (iFetcher) iFetcher->NickChanged(iOldNick, iNick);
	if (iBook) iBook->ReRead();

	return true;
}

TBool CNickForm::OkToExitL( TInt aButtonId )
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("OkToExitL"));


	if (aButtonId==EAknSoftkeyOk) {
		CC_TRAPD(err, SaveFormDataL());
		if (err!=KErrNone) {
			//TODO: show error
		}
	}
	return true;
}

void CNickForm::DoNotSaveFormDataL() { }

void CNickForm::SetInitialCurrentLine()
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("SetInitialCurrentLine"));

	ActivateFirstPageL();
	TryChangeFocusToL(2);
}

void CNickForm::PreLayoutDynInitL()
{
	CEikEdwin* f=(CEikEdwin*)Control(2);
	f->SetTextL(&iNick);

	CEikEdwin* l=(CEikEdwin*)Control(1);
	l->SetTextL(&(*iTitle));

	CAknPopupFieldText* p=(CAknPopupFieldText*)ControlOrNull(3);
	if (p) {
		if (iShowDetails) p->SetCurrentValueIndex(0);
		else p->SetCurrentValueIndex(1);
	}
}

void CNickForm::PostLayoutDynInitL()
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("PostLayoutDynInitL"));

	CAknForm::PostLayoutDynInitL();
	SetEditableL(iEditable);
	SetChangesPending(iEditable);
}

TBool CNickForm::QuerySaveChangesL()
{
	CALLSTACKITEM_N(_CL("CNickForm"), _CL("QuerySaveChangesL"));


	TBool isAnsYes(CAknForm::QuerySaveChangesL());
	
	if (isAnsYes)
        {
		SaveFormDataL();
        }
	else 
        {
		// Case that answer "No" to query.
		DoNotSaveFormDataL();
        }
	
	return isAnsYes;
}
