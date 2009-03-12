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

#include "Test_ContactMatcher.h"

#include "utcontactengine.h"

#include "symbian_auto_ptr.h"
#include "ccu_contactmatcher.h"

#include <badesca.h>
#include <cntdb.h>
#include <cpbkcontactitem.h>
#include <pbkfields.hrh>
#include <tpbkcontactitemfield.h>

enum { EFirst = CUTContactEngine::EFirst,
	   ELast = CUTContactEngine::ELast, 
	   EPhoneNumber = CUTContactEngine::EPhoneNumber };

void CTest_ContactMatcherBasic::testSimpleMatchL()
{
  _LIT( KNumberToBeFound, "+358406669191");
  _LIT( KNumberNotToBeFound, "+358403331232");

  // Store contact
  TContactItemId id = iEngine->StoreContactL( _L("Iggy"), _L("Pop"), KNumberToBeFound);

  TS_ASSERT( id > 0 );
  
  // Assert that contact details are same
  auto_ptr<CPbkContactItem> contact( iEngine->PbkEngine().ReadContactL( id ) );
  TS_ASSERT( contact.get() );
    
  // Test matcher
  auto_ptr<CContactMatcher> matcher( CContactMatcher::NewL() );
  TS_ASSERT( matcher.get() );
  
  auto_ptr<CContactIdArray> ids( matcher->FindMatchesForNumberL( KNumberToBeFound ) );
  TS_ASSERT_EQUALS( ids->Count(), 1 );
  TS_ASSERT_EQUALS( (*ids)[0], id );

  auto_ptr<CContactIdArray> ids2( matcher->FindMatchesForNumberL( KNumberNotToBeFound ) );
  TS_ASSERT_EQUALS( ids2->Count(), 0 );
}

void CTest_ContactMatcherBasic::setUp()
{
	iEngine=0;
	MContextTestBase::setUp();
	_LIT(KUnittestDb, "c:\\unittests\\phonebook.db");
	iEngine = CUTContactEngine::NewL(KUnittestDb, ETrue);	
}


void CTest_ContactMatcherBasic::tearDown()
{
	delete iEngine;
	MContextTestBase::tearDown();
}


// 
//
//

_LIT(KIggyInternational, "+358441339347");
_LIT(KIggyNoninternational, "0441339347");

_LIT(KJohnnyInternational, "+14155133868");
_LIT(KJohnnyNoninternational, "04155133868");


void CTest_ContactMatcherAdvanced::testNoninternationalL()
{
	// Find internationally stored
	TContactItemId id = iIds->At(0);
	auto_ptr<CContactIdArray> ids( iMatcher->FindMatchesForNumberL( KIggyInternational ) );
	TS_ASSERT_EQUALS( ids->Count(), 1 );
	TS_ASSERT_EQUALS( (*ids)[0], id );

	auto_ptr<CContactIdArray> ids2( iMatcher->FindMatchesForNumberL( KIggyNoninternational ) );
	TS_ASSERT_EQUALS( ids2->Count(), 1 );
	TS_ASSERT_EQUALS( (*ids2)[0], id );
	
	// Find noninternationally stored number
	id = iIds->At(1);
	auto_ptr<CContactIdArray> ids3( iMatcher->FindMatchesForNumberL( KJohnnyNoninternational ) );
	TS_ASSERT_EQUALS( ids3->Count(), 1 );
	TS_ASSERT_EQUALS( (*ids3)[0], id);

	auto_ptr<CContactIdArray> ids4( iMatcher->FindMatchesForNumberL( KJohnnyInternational ) );
	TS_ASSERT_EQUALS( ids4->Count(), 1 );
	TS_ASSERT_EQUALS( (*ids4)[0], id);
}

void CTest_ContactMatcherAdvanced::setUp()
{
	iEngine=0;
	iIds=0;
	iMatcher=0;
	MContextTestBase::setUp();

	_LIT(KUnittestDb, "c:\\unittests\\phonebook.db");
	iEngine = CUTContactEngine::NewL(KUnittestDb, ETrue);	
	
	iIds = new (ELeave) CArrayFixFlat<TContactItemId>(5);
	iIds->AppendL( iEngine->StoreContactL( _L("Iggy"), _L("Pop"), KIggyInternational ) );
	iIds->AppendL( iEngine->StoreContactL( _L("Johnny"), _L("Rotten"), KJohnnyNoninternational ) );
	
	// Test matcher
	iMatcher = CContactMatcher::NewL();
}


void CTest_ContactMatcherAdvanced::tearDown()
{
	delete iMatcher; iMatcher=0;
	delete iEngine; iEngine=0;
	delete iIds; iIds=0;

	MContextTestBase::tearDown();
}


