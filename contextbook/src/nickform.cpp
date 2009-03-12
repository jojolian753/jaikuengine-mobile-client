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

#include "nickform.h"
#include "symbian_auto_ptr.h"
#include <contextbook.rsg>
#include <avkon.hrh>
#include <eikedwin.h>
#include <eiklabel.h>

CNickForm* CNickForm::NewL(CJabberData& Data, TInt ContactId, const TDesC& Title, phonebook_i* Book)
{
	CALLSTACKITEM(_L("CNickForm::NewL"));

	auto_ptr<CNickForm> self (
		new (ELeave) CNickForm(Data, ContactId, Book));
	self->ConstructL(Title);
	return self.release();
}

CNickForm::~CNickForm()
{
	CALLSTACKITEM(_L("CNickForm::~CNickForm"));

	delete iTitle;
}
CNickForm::CNickForm(CJabberData& Data, TInt ContactId, phonebook_i* Book) : 
	iData(Data), iContactId(ContactId), iBook(Book)
{
	CALLSTACKITEM(_L("CNickForm::CNickForm"));

}

void CNickForm::ConstructL(const TDesC& Title)
{
	CALLSTACKITEM(_L("CNickForm::ConstructL"));

	iTitle=Title.AllocL();
	iData.GetJabberNickL(iContactId, iNick);
	CAknForm::ConstructL();
}

TInt CNickForm::ExecuteLD()
{
	CALLSTACKITEM(_L("CNickForm::ExecuteLD"));

	return CAknForm::ExecuteLD(R_CB_NICK_DIALOG);
}
void CNickForm::PrepareLC()
{
	CALLSTACKITEM(_L("CNickForm::PrepareLC"));

	CAknForm::PrepareLC(R_CB_NICK_DIALOG);
}

TBool CNickForm::SaveFormDataL()
{
	CALLSTACKITEM(_L("CNickForm::SaveFormDataL"));

	GetEdwinText(iNick, 2);

	iData.SetJabberNickL(iContactId, iNick);

	if (iBook) iBook->ReRead();

	return true;
}

TBool CNickForm::OkToExitL( TInt aButtonId )
{
	CALLSTACKITEM(_L("CNickForm::OkToExitL"));

	if (aButtonId==EAknSoftkeyOk) {
		TRAPD(err, SaveFormDataL());
		if (err!=KErrNone) {
			//TODO: show error
		}
	}
	return true;
}

void CNickForm::DoNotSaveFormDataL()
{
	CALLSTACKITEM(_L("CNickForm::DoNotSaveFormDataL"));

}
void CNickForm::SetInitialCurrentLine()
{
	CALLSTACKITEM(_L("CNickForm::SetInitialCurrentLine"));

	ActivateFirstPageL();
	TryChangeFocusToL(2);
}

void CNickForm::PostLayoutDynInitL()
{
	CALLSTACKITEM(_L("CNickForm::PostLayoutDynInitL"));

	CEikEdwin* f=(CEikEdwin*)Control(2);
	f->SetTextL(&iNick);

	CEikEdwin* l=(CEikEdwin*)Control(1);
	l->SetTextL(&(*iTitle));

	CAknForm::PostLayoutDynInitL();
	// To Following line change the edit mode
	SetEditableL(ETrue);

	// The following line set to change status
	// This avoid back set form View status in the beginning 
	// Usually without change back do that, like when we start an empty Form.
	SetChangesPending(ETrue);

}

TBool CNickForm::QuerySaveChangesL()
{
	CALLSTACKITEM(_L("CNickForm::QuerySaveChangesL"));

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
