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

#include "Test_UTContactEngine.h"

#include "utcontactengine.h"

#include "symbian_auto_ptr.h"

#include <badesca.h>
#include <cntdb.h>
#include <cpbkcontactitem.h>
#include <pbkfields.hrh>
#include <tpbkcontactitemfield.h>
#include "phonebook.h"
#include "testutils.h"

enum { EFirst = CUTContactEngine::EFirst,
	   ELast = CUTContactEngine::ELast, 
	   EPhoneNumber = CUTContactEngine::EPhoneNumber };

void CTest_UTContactEngine::testAddContact()
{
  auto_ptr<phonebook> pb1(new (ELeave) phonebook(AppContext(), 0, 0));
  pb1->ConstructL();
  
  // Setup test data for contact 
  auto_ptr<CDesCArray> fieldContents( DataArray( _L("Neil"), _L("Gaiman"), _L("+358406669191") ) );
  TS_ASSERT_EQUALS( iEngine->FieldIds()->Count(), fieldContents->Count() );

  // Store contact
  TContactItemId id = iEngine->StoreContactL( fieldContents->MdcaPoint( EFirst ),
											  fieldContents->MdcaPoint( ELast ),
											  fieldContents->MdcaPoint( EPhoneNumber ) );
  TS_ASSERT( id > 0 );
  
  // Assert that contact details are same
  auto_ptr<CPbkContactItem> contact( PbkEngine().ReadContactL( id ) );
  TS_ASSERT( contact.get() );
  
  for (TInt i=0; i < fieldContents->Count(); i++)
    {
      TPbkContactItemField* field = contact->FindField( iEngine->FieldIds()->At(i) );
      TS_ASSERT( field );
      TS_ASSERT_EQUALS_DESCRIPTOR( field->Text(), fieldContents->MdcaPoint(i) );
    }  
    
  TestUtils::WaitForActiveSchedulerStopL(4);
  auto_ptr<phonebook> pb2(new (ELeave) phonebook(AppContext(), 0, 0));
  pb2->ConstructL();
  TS_ASSERT(TestEqualPhoneBooks(pb1.get(), pb2.get()));
}


void CTest_UTContactEngine::testRemoveAllContacts()
{
  auto_ptr<phonebook> pb1(new (ELeave) phonebook(AppContext(), 0, 0));
  pb1->ConstructL();
  
  testAddContact();
  
  TS_ASSERT( CntDb().CountL() > 0 );
  TS_ASSERT_EQUALS( pb1->Count(), CntDb().CountL() );
  iEngine->RemoveAllContactsL();
  TS_ASSERT_EQUALS( CntDb().CountL(), 0 );
  
  auto_ptr<phonebook> pb2(new (ELeave) phonebook(AppContext(), 0, 0));
  pb2->ConstructL();
  TS_ASSERT(! TestEqualPhoneBooks(pb1.get(), pb2.get()));
  TestUtils::WaitForActiveSchedulerStopL(4);
  TS_ASSERT_EQUALS( pb1->Count(), 0 );
  TS_ASSERT(TestEqualPhoneBooks(pb1.get(), pb2.get()));
}


void CTest_UTContactEngine::testFindExactMatchesL()
{
  auto_ptr<phonebook> pb1(new (ELeave) phonebook(AppContext(), 0, 0));
  pb1->ConstructL();
  
  // Setup test data for contact 
  auto_ptr<CDesCArray> fieldContents1( DataArray( _L("Neil"), _L("Gaiman"), _L("+358406669191") ) );
  TContactItemId id1 = iEngine->StoreContactL( fieldContents1->MdcaPoint( EFirst ),
					      fieldContents1->MdcaPoint( ELast ),
					      fieldContents1->MdcaPoint( EPhoneNumber ) );

  auto_ptr<CDesCArray> fieldContents2( DataArray( _L("Paavo"), _L("Nurmi"), _L("+358403232322") ) );
  TContactItemId id2 = iEngine->StoreContactL( fieldContents2->MdcaPoint( EFirst ),
					      fieldContents2->MdcaPoint( ELast ),
					      fieldContents2->MdcaPoint( EPhoneNumber ) );
  
  auto_ptr<CDesCArray> fieldContents3( DataArray( _L("John"), _L("Neil"), _L("+358403232322") ) );
  TContactItemId id3 = iEngine->StoreContactL( fieldContents3->MdcaPoint( EFirst ),
					      fieldContents3->MdcaPoint( ELast ),
					      fieldContents3->MdcaPoint( EPhoneNumber ) );

  auto_ptr<CDesCArray> fieldContents4( DataArray( _L("John"), _L("Doe"), _L("+132232322") ) );
  TContactItemId id4 = iEngine->StoreContactL( fieldContents4->MdcaPoint( EFirst ),
					      fieldContents4->MdcaPoint( ELast ),
					      fieldContents4->MdcaPoint( EPhoneNumber ) );
  
  RPointerArray<CPbkContactItem> results; 
  iEngine->FindExactMatchesL( _L("Neil"), iEngine->FieldIds()->At(ELast), results);
  TS_ASSERT_EQUALS( results.Count(), 1 );
  results.ResetAndDestroy();

  iEngine->FindExactMatchesL( _L("John"), iEngine->FieldIds()->At(EFirst), results);
  TS_ASSERT_EQUALS( results.Count(), 2 );
  results.ResetAndDestroy();


  iEngine->FindExactMatchesL( _L("Billy"), iEngine->FieldIds()->At(ELast), results);
  TS_ASSERT_EQUALS( results.Count(), 0 );
  results.ResetAndDestroy();

  TestUtils::WaitForActiveSchedulerStopL(4);
  TS_ASSERT_EQUALS( pb1->Count(), CntDb().CountL() );
  
  auto_ptr<phonebook> pb2(new (ELeave) phonebook(AppContext(), 0, 0));
  pb2->ConstructL();
  TS_ASSERT(TestEqualPhoneBooks(pb1.get(), pb2.get()));
}


void CTest_UTContactEngine::setUp()
{
  iEngine = 0;
	MContextTestBase::setUp();

  iEngine = CUTContactEngine::NewL();
}


void CTest_UTContactEngine::tearDown()
{
  delete iEngine;

  MContextTestBase::tearDown();
}


CPbkContactEngine& CTest_UTContactEngine::PbkEngine()
{
  return iEngine->PbkEngine();
}


CContactDatabase& CTest_UTContactEngine::CntDb()
{
  return PbkEngine().Database();
}


CDesCArray* CTest_UTContactEngine::DataArray(const TDesC& aFirstName,
					     const TDesC& aLastName,
					     const TDesC& aPhoneNumber)
{
	return iEngine->DataArrayL( aFirstName, aLastName, aPhoneNumber );
}

