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

#include "utcontactengine.h"

#include "symbian_auto_ptr.h"

#include <cpbkcontactitem.h>
#include <cpbkcontactiter.h>
#include <cntdef.h>
#include <pbkfields.hrh>

CUTContactEngine* CUTContactEngine::NewL()
{
	auto_ptr<CUTContactEngine> self( new (ELeave) CUTContactEngine);
	self->ConstructL(KNullDesC, EFalse);
	return self.release();
}


CUTContactEngine* CUTContactEngine::NewL(const TDesC& aFileName, TBool aReplace)
{
	auto_ptr<CUTContactEngine> self( new (ELeave) CUTContactEngine);
	self->ConstructL( aFileName, aReplace );
	return self.release();
}

CUTContactEngine::~CUTContactEngine()
{
	if ( iOwnsEngine )
		{
			delete iEngine;
		}
  delete iFieldIds;
}


TContactItemId CUTContactEngine::StoreContactL( const TDesC& aFirstName,
												const TDesC& aLastName,
												const TDesC& aPhoneNumber )
{
	// Create a contact with few default fields
	// All the default fields are empty and won't be displayed
	// until some information is stored in them
	auto_ptr<CPbkContactItem> contact( iEngine->CreateEmptyContactL() );
  
	// Set some default fields
	contact->FindField( EPbkFieldIdFirstName )->TextStorage()->SetTextL( aFirstName );
	contact->FindField( EPbkFieldIdLastName )->TextStorage()->SetTextL( aLastName );
	contact->FindField( EPbkFieldIdPhoneNumberStandard )->TextStorage()->SetTextL( aPhoneNumber );
  
	return iEngine->AddNewContactL( *contact );  
}


void CUTContactEngine::RemoveAllContactsL()
{
	auto_ptr<CContactIdArray> ids( CContactIdArray::NewL() );
 
	// get all ids   
	auto_ptr<CPbkContactIter> iter( CreateContactIteratorL() );
  
	TContactItemId id = iter->FirstL();
	while ( id != KNullContactId )
		{
			ids->AddL(id);
			id = iter->NextL();
		}

	// Delete all 
	iEngine->DeleteContactsL( *ids );
}


void CUTContactEngine::FindExactMatchesL( const TDesC& aText, TPbkFieldId aField, RPointerArray<CPbkContactItem>& aResult)
{
	auto_ptr<CPbkContactIter> iter( CreateContactIteratorL() );
	TContactItemId id = iter->FirstL();
	while ( id != KNullContactId )
		{
			CPbkContactItem* item = iter->CurrentL(); // item not owned 
			if ( item )
				{
					TPbkContactItemField* field = item->FindField( aField );
					if ( field && field->Text() == aText )	    
						{
							aResult.Append( iter->GetCurrentL() );
						}
				}
			id = iter->NextL();
		} 
}


const CPbkContactEngine& CUTContactEngine::PbkEngine() const
{
	return *iEngine;
}


CPbkContactEngine& CUTContactEngine::PbkEngine() 
{
	return *iEngine;
}


CUTContactEngine::CUTContactEngine()
{
}

#include <bautils.h>

void CUTContactEngine::ConstructL(const TDesC& aFileName, TBool aReplace)
{
	iEngine = CPbkContactEngine::Static();
	if ( ! iEngine )
		{
			RFs& fs = Fs();
			if ( aFileName == KNullDesC )
				{
					iEngine = CPbkContactEngine::NewL(&fs);					
				}
			else
				{
#ifndef __S60V3__
					BaflUtils::EnsurePathExistsL(fs, aFileName);
					iEngine = CPbkContactEngine::NewL(aFileName, aReplace, &fs);
#else
					iEngine = CPbkContactEngine::NewL(_L("c:DBS_100065FF_unittests.cdb"), aReplace, &fs);
#endif
				}
			iOwnsEngine = ETrue;
		}
	else
		{
			iOwnsEngine = EFalse;
		}

  iFieldIds = new CArrayFixFlat<TPbkFieldId>(5);
  iFieldIds->AppendL( EPbkFieldIdFirstName );
  iFieldIds->AppendL( EPbkFieldIdLastName );
  iFieldIds->AppendL( EPbkFieldIdPhoneNumberStandard );

}

CPbkContactIter* CUTContactEngine::CreateContactIteratorL()
{
	CPbkContactIter* iterx = iEngine->CreateContactIteratorLC();
	CleanupStack::Pop(iterx); // clumsy way to deal with *LC methods and auto_ptr
	return iterx;
}

#include "phonebook.h"

TBool TestEqualPhoneBooks(class phonebook_i* pb1, class phonebook_i* pb2)
{
	if ( pb1->Count() != pb2->Count()) return EFalse;

	for (int i=0; i<pb1->Count(); i++) {
		if ( ! ( *(pb1->GetContact(i))  == *(pb2->GetContact(i)) ) ) {
			return EFalse;
		}
	}	
	return ETrue;
}


/**
 * Helper function to create CDesCArrays for StoreContactL 
 */ 
CArrayFixFlat<TPbkFieldId>* CUTContactEngine::FieldIds()
{
	return iFieldIds;
}

CDesCArray* CUTContactEngine::DataArrayL(const TDesC& aFirstName,
										 const TDesC& aLastName,
										 const TDesC& aPhoneNumber)
{
	auto_ptr<CDesCArray> data( new CDesCArrayFlat(5) );
	
	data->AppendL( aFirstName );
	data->AppendL( aLastName );
	data->AppendL( aPhoneNumber );
	
	return data.release();
}
