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

#include "ccn_message.h"

#include "ccn_expandingnote.h"

#include "jabberdata.h"
#include "phonehelper_ui.h"


#include <cpbkmmsaddressselect.h> 
#include <cpbksmsaddressselect.h>
#include <cpbkemailaddressselect.h>
#include <cpbkfieldinfo.h> 
#include <sendui.h>
#include "phonebook.h"
#include <cpbkcontactitem.h>
#include <aknnotewrappers.h>

#ifdef __S60V3__
#include <cmessagedata.h>
#endif

CMessaging::CMessaging(class phonebook* aBook, class phonehelper_ui* aPhoneHelper, class CJabberData* aJabberData) : 
	iBook(aBook), iPhoneHelper(aPhoneHelper), iJabberData(aJabberData) { }

void CMessaging::ConstructL()
{
	iSendAppUi=CSendUi::NewL();
}

CMessaging* CMessaging::NewL(class phonebook* aBook, class phonehelper_ui* aPhoneHelper, class CJabberData* aJabberData)
{
	auto_ptr<CMessaging> ret(new (ELeave) CMessaging(aBook, aPhoneHelper, aJabberData));
	ret->ConstructL();
	return ret.release();
}

CMessaging::~CMessaging()
{
	delete iSendAppUi;
}

const TPbkContactItemField* MmsSelectorL(CPbkContactItem& aItem) 
{
	
	CPbkMmsAddressSelect* sel=new (ELeave) CPbkMmsAddressSelect();
	CPbkMmsAddressSelect::TParams params( aItem );
	params.SetUseDefaultDirectly( ETrue );	
	if ( sel->ExecuteLD( params ) )
		return params.SelectedField();
	else
		return NULL;
} 
const TPbkContactItemField* SmsSelectorL(CPbkContactItem& aItem) 
{
	
	CPbkSmsAddressSelect* sel=new (ELeave) CPbkSmsAddressSelect();
	CPbkSmsAddressSelect::TParams params( aItem );
	params.SetUseDefaultDirectly( ETrue );	
	if ( sel->ExecuteLD( params ) )
		return params.SelectedField();
	else
		return NULL;
} 

const TPbkContactItemField* EmailSelectorL(CPbkContactItem& aItem) 
{
	
	CPbkEmailAddressSelect* sel=new (ELeave) CPbkEmailAddressSelect();
	CPbkEmailAddressSelect::TParams params( aItem );
	params.SetUseDefaultDirectly( ETrue );	
	if ( sel->ExecuteLD( params ) )
		return params.SelectedField();
	else
		return NULL;
}


TBool SmsFieldFilterL( const TPbkContactItemField& aField )
{
	return aField.FieldInfo().IsPhoneNumberField();
}


TBool MmsFieldFilterL( const TPbkContactItemField& aField )
{
	return aField.FieldInfo().IsMmsField();
}


TBool EmailFieldFilterL( const TPbkContactItemField& aField )
{
	// Email field is anything you can send MMS to and that isn't phone number
	return !aField.FieldInfo().IsPhoneNumberField() && aField.FieldInfo().IsMmsField();
}


void CMessaging::SmsSenderL(CDesCArrayFlat* recip, CDesCArrayFlat* alias, TUid)
{
	iPhoneHelper->send_sms(recip, alias);	
}

void CMessaging::MmsSenderL(CDesCArrayFlat* recip, CDesCArrayFlat* alias, TUid)
{
	iPhoneHelper->send_mms(recip, alias);	
}

void CMessaging::EmailSenderL(CDesCArrayFlat* recip, CDesCArrayFlat* alias, TUid)
{
	iPhoneHelper->send_email(recip, alias);	
}

void CMessaging::MessageSenderL(CDesCArrayFlat* recip, CDesCArrayFlat* alias, TUid aMtm)
{
	if (aMtm==KNullUid || !recip || recip->Count()==0) return;
#ifndef __S60V3__
	iSendAppUi->CreateAndSendMessageL(aMtm, 0, 0, KNullUid, recip, alias, EFalse);
#else
	auto_ptr<CMessageData> data(CMessageData::NewL());
	TInt i;
	for (i=0; i< recip->Count(); i++) {
		if (alias) {
			data->AppendToAddressL( (*recip)[i], (*alias)[i] );
		} else {
			data->AppendToAddressL( (*recip)[i] );
		}
	}
	iSendAppUi->CreateAndSendMessageL(aMtm, data.get(), KNullUid, EFalse);
#endif
}

void CMessaging::CreateSingleMessageL( TInt aContactId, 
												 TAddressSelectorF aSelector, 
												 TMessageSenderF aSender,
												 TUid aMtm) 
{
 	auto_ptr<CPbkContactItem> item(  iBook->get_engine()->ReadContactL(aContactId) );	
	
	const TPbkContactItemField* field = ((*aSelector)( *item ));
	if ( field && field->StorageType() == KStorageTypeText ) 
 		{			
 			auto_ptr<CDesCArrayFlat> recip( new (ELeave) CDesCArrayFlat(1) );
 			auto_ptr<CDesCArrayFlat> alias( new (ELeave) CDesCArrayFlat(1) );
			
 			recip->AppendL( field->Text() );
 			auto_ptr<HBufC> title(item->GetContactTitleL());
 			alias->AppendL(*title);
 			(this->*aSender)(recip.get(), alias.get(), aMtm);
 		}
}


void CMessaging::CreateMassMessageL( CArrayFix<TInt>& aContactIds,
											   TInt aWarningNoteResource,
											   TAddressSelectorF aSelector, 
											   TMessageSenderF aSender,
											   TPbkFieldFilterF aFilter,
											   TUid aMtm)
{
	
	auto_ptr<CDesCArrayFlat> recip( new (ELeave) CDesCArrayFlat(1) );
	auto_ptr<CDesCArrayFlat> alias( new (ELeave) CDesCArrayFlat(1) );
	
	bool warning = false;
	auto_ptr<CExpandingNote> note( CExpandingNote::NewL( aWarningNoteResource ) );
	
	for (TInt i=0; i < aContactIds.Count(); i++)
		{
			TInt contact_id = iBook->GetContactId( aContactIds.At(i) );
			
			if ( iJabberData->IsDummyContactId( contact_id ) )
				{
					contact *c = iBook->GetContactById( contact_id );
					auto_ptr<HBufC> name( c->NameL( iBook->ShowLastNameFirstL() ) );
					note->AppendLineL( *name );
				}
			else
				{
					auto_ptr<CPbkContactItem> item( iBook->get_engine()->ReadContactL(contact_id) );
					auto_ptr<HBufC> title( item->GetContactTitleL() );
					
					int j=0;
					bool found = false;
					for (j=0; (j<item->CardFields().Count()) && (found==EFalse); j++)
						{
							TPbkContactItemField f = (item->CardFields())[j];
							if ( (*aFilter)(f) )
								{
									found = true;
									break;
								}
						}
					
					if (found)
						{
							
							const TPbkContactItemField* field = ((*aSelector)( *item ));
							if ( field && field->StorageType() == KStorageTypeText ) 
								{
									recip->AppendL( field->Text() );
									alias->AppendL( *title );
								}
							else
								{
									note->AppendLineL(*title);
								}
						}
					else
						{
							note->AppendLineL(*title);
						}
				}
		}
	
	if ( note->HasData() )
		{
			note->ShowWarningNoteL();
		}
	
	if (recip->Count() > 0)
		{
			(this->*aSender)(recip.get(), alias.get(), aMtm);
		}
	//iContainer->ResetSearchField();
}

