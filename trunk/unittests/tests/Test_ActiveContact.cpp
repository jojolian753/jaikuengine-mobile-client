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

#include "Test_ActiveContact.h"

#include "utcontactengine.h"

#include "app_context_impl.h"
#include "symbian_auto_ptr.h"

#include "ccu_contactmatcher.h"
#include "ccu_activecontact.h"
#include "ccu_userpics.h"
#include "phonebook.h"

#include "juik_iconmanager.h"
#include "cl_settings.h"

#include <badesca.h>
#include <cntdb.h>
#include <cpbkcontactitem.h>
#include <pbkfields.hrh>
#include <tpbkcontactitemfield.h>


void CTest_ActiveContact::testConstructionL()
{
	auto_ptr<CActiveContact> a( CActiveContact::NewL( *iPhonebook, *iJabberData, *iUserPics ) );
}


void CTest_ActiveContact::testSetAndGetL()
{
	auto_ptr<CActiveContact> a( CActiveContact::NewL( *iPhonebook, *iJabberData, *iUserPics ) );
	TS_ASSERT( a->GetL() == NULL );
	TS_ASSERT_EQUALS( a->GetId(), KNullContactId );


	TInt id = iContactEngine->StoreContactL( _L("Jeppe"), _L("Pulunen"), _L("0405666778") );
	contact* c = iPhonebook->GetContactById(id);		
	TS_ASSERT( c != NULL );
	id = c->id;
	a->SetL( id );

	TS_ASSERT( a->GetL() == c );
	TS_ASSERT_EQUALS( a->GetId(), id ); 
}



void CTest_ActiveContact::testActiveNickL()
{
	auto_ptr<CActiveContact> a( CActiveContact::NewL( *iPhonebook, *iJabberData, *iUserPics ) );
	TS_ASSERT( a->GetL() == NULL );
	TS_ASSERT_EQUALS( a->GetId(), KNullContactId );

	TInt id = iContactEngine->StoreContactL( _L("Jeppe"), _L("Pulunen"), _L("0405666778") );
	contact* c = iPhonebook->GetContactById(id);		
	TS_ASSERT( c != NULL );
	id = c->id;
	a->SetL( id );

  	TS_ASSERT( a->GetL() == c );
	TS_ASSERT_EQUALS( a->GetId(), id ); 

	_LIT( KNickNotInJabber, "raphaelo666@jaiku.com");
	TS_ASSERT_EQUALS( iJabberData->GetContactIdL( KNickNotInJabber ), KErrNotFound );
	a->SetActiveNickL( KNickNotInJabber );
  	TS_ASSERT( a->GetL() == NULL );
	TS_ASSERT_EQUALS( a->GetId(), KNullContactId ); 
	CJabberData::TNick nick;
	a->GetNickL( nick );
	TS_ASSERT_EQUALS_DESCRIPTOR( nick, KNickNotInJabber );
}


void CTest_ActiveContact::setUp()
{
	MContextTestBase::setUp();
	GetContext()->SetDataDir( _L("c:\\unittests\\"), EFalse );

	iContactEngine = CUTContactEngine::NewL(); //_L("c:\\unittests\\utcontacts.db"), ETrue);
	iContactEngine->RemoveAllContactsL();

	{
		TFileName jabber;
		jabber.Append( iAppContext->DataDir() );
		jabber.Append( _L("\\") );
		jabber.Append( _L("JABBER.DB") );
		BaflUtils::DeleteFile(iAppContext->Fs(), jabber);
	}
	iJabberDb=CDb::NewL(*iAppContext, _L("JABBER"), EFileWrite|EFileShareAny); 
	iJabberPics = CJabberPics::NewL(*iJabberDb);
	iJabberData = CJabberData::NewL(*iAppContext, *iJabberDb, SETTING_JABBER_NICK);
	
	iPhonebook = new (ELeave) phonebook(AppContext(), iJabberData, NULL);
	iPhonebook->ConstructL();
	
	iIconManager = CJuikIconManager::NewL();
	iUserPics = CUserPics::NewL(*iJabberPics, *iJabberData, *iIconManager);	
}


void CTest_ActiveContact::tearDown()
{
	delete iUserPics;
	delete iIconManager;
	delete iPhonebook;
	delete iJabberData;
	delete iJabberPics;
	delete iJabberDb;
	delete iContactEngine;
	MContextTestBase::tearDown();
}


