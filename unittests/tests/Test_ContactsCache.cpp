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

#include "Test_ContactsCache.h"

#include "utcontactengine.h"
#include "testutils.h"

#include "phonebook.h"
#include "symbian_auto_ptr.h"

#include <badesca.h>
#include <cntdb.h>
#include <cpbkcontactitem.h>
#include <pbkfields.hrh>
#include <tpbkcontactitemfield.h>
#include "jaikucacherclientsession.h"
#include <s32strm.h>

enum { EFirst = CUTContactEngine::EFirst,
	   ELast = CUTContactEngine::ELast, 
	   EPhoneNumber = CUTContactEngine::EPhoneNumber };

void CTest_ContactsCache::testManyContactsL()
{
	TBuf<100> first, last, phone;
	for (int i=0; i<500; i++) {
		first.Fill('f', 80);
		first.AppendNum(i);
		last.Fill('l', 80);
		last.AppendNum(i);
		phone.Zero();
		phone.AppendNum(i);
		auto_ptr<CDesCArray> fieldContents1( iEngine->DataArrayL( first, last, phone ) );
		TContactItemId id1 = iEngine->StoreContactL( fieldContents1->MdcaPoint( EFirst ),
									 fieldContents1->MdcaPoint( ELast ),
									 fieldContents1->MdcaPoint( EPhoneNumber ) );
	}
	RJaikuCacher c; c.Connect();
	{
		TRequestStatus s;
		c.SetContactsDb(_L("ContactsCache"), s);
		User::WaitForRequest(s);
		User::LeaveIfError(s.Int());
	}
	User::After(TTimeIntervalMicroSeconds32(5000*1000));
	TName hashname, list;
	{
		TRequestStatus s;
		c.GetContactsData(hashname, list, s);
		User::WaitForRequest(s);
		User::LeaveIfError(s.Int());
	}
	{
		RChunk hashc;
		User::LeaveIfError(hashc.OpenGlobal(hashname, ETrue));
		
		TPtrC8 p(hashc.Base(), hashc.Size());
		RDesReadStream s(p);
		TBuf8<16> hash;
		TInt contact;
		TInt count=0;
		while (contact=s.ReadInt32L()) {
			TBuf<20> b; b.AppendNum(contact);
			RDebug::Print(b);
			count++;
			s.ReadL(hash, 16);
		}
		hashc.Close();
		TS_ASSERT_EQUALS(count, 500);
	}
	{
		RChunk listc;
		User::LeaveIfError(listc.OpenGlobal(list, ETrue));
		
		TPtrC8 p(listc.Base(), listc.Size());
		RDesReadStream s(p);
		TBuf<100> str;
		TInt contact;
		TInt count=0, length;
		while (contact=s.ReadInt32L()) {
			for (int i=0; i<3; i++) {
				length=s.ReadInt32L();
				if (length>0) {
					s.ReadL(str, length);
					RDebug::Print(str);
				}
			}
			count++;
		}
		listc.Close();
		TS_ASSERT_EQUALS(count, 500);
	}
	
	c.Close();
}

void CTest_ContactsCache::testAddContactsL()
{
	/*{
		TBuf8<10> b;
		RDesWriteStream s(b);
		TBuf8<11> b2; b2.Fill('x', 8);
		s.WriteL(b2);
		TRAPD(err, s.WriteInt32L(11));
		s.CommitL();
		TInt len=b.Length();
		TS_ASSERT_EQUALS(len, 0);
	}*/
	RJaikuCacher c; c.Connect();
	{
		TRequestStatus s;
		c.SetContactsDb(_L("ContactsCache"), s);
		User::WaitForRequest(s);
		User::LeaveIfError(s.Int());
	}
	
	// Setup test data for contact 
	auto_ptr<CDesCArray> fieldContents1( iEngine->DataArrayL( _L("Neil"), _L("Gaiman"), _L("+358406669191") ) );
	TContactItemId id1 = iEngine->StoreContactL( fieldContents1->MdcaPoint( EFirst ),
												 fieldContents1->MdcaPoint( ELast ),
												 fieldContents1->MdcaPoint( EPhoneNumber ) );
	
	auto_ptr<CDesCArray> fieldContents2( iEngine->DataArrayL( _L("Paavo"), _L("Nurmi"), _L("+358403232322") ) );
	TContactItemId id2 = iEngine->StoreContactL( fieldContents2->MdcaPoint( EFirst ),
												 fieldContents2->MdcaPoint( ELast ),
												 fieldContents2->MdcaPoint( EPhoneNumber ) );
	
	auto_ptr<CDesCArray> fieldContents3( iEngine->DataArrayL( _L("John"), _L("Neil"), _L("+358403232322") ) );
	TContactItemId id3 = iEngine->StoreContactL( fieldContents3->MdcaPoint( EFirst ),
												 fieldContents3->MdcaPoint( ELast ),
												 fieldContents3->MdcaPoint( EPhoneNumber ) );
	
	auto_ptr<CDesCArray> fieldContents4( iEngine->DataArrayL( _L("John"), _L("Doe"), _L("+132232322") ) );
	TContactItemId id4 = iEngine->StoreContactL( fieldContents4->MdcaPoint( EFirst ),
												 fieldContents4->MdcaPoint( ELast ),
												 fieldContents4->MdcaPoint( EPhoneNumber ) );
	
	User::After(TTimeIntervalMicroSeconds32(1000*1000));
	
	{
		TRequestStatus s;
		c.SetContactsDb(_L("ContactsCache"), s);
		User::WaitForRequest(s);
		User::LeaveIfError(s.Int());
	}
	
	TName hashname, list;
	{
		TRequestStatus s;
		c.GetContactsData(hashname, list, s);
		User::WaitForRequest(s);
		User::LeaveIfError(s.Int());
	}
	
	{
		RChunk hashc;
		User::LeaveIfError(hashc.OpenGlobal(hashname, ETrue));
		
		TPtrC8 p(hashc.Base(), hashc.Size());
		RDesReadStream s(p);
		TBuf8<16> hash;
		TInt contact;
		TInt count=0;
		while (contact=s.ReadInt32L()) {
			TBuf<20> b; b.AppendNum(contact);
			RDebug::Print(b);
			count++;
			s.ReadL(hash, 16);
		}
		hashc.Close();
		TS_ASSERT_EQUALS(count, 4);
	}
	{
		RChunk listc;
		User::LeaveIfError(listc.OpenGlobal(list, ETrue));
		
		TPtrC8 p(listc.Base(), listc.Size());
		RDesReadStream s(p);
		TBuf<100> str;
		TInt contact;
		TInt count=0, length;
		while (contact=s.ReadInt32L()) {
			for (int i=0; i<3; i++) {
				length=s.ReadInt32L();
				if (length>0) {
					s.ReadL(str, length);
					RDebug::Print(str);
				}
			}
			count++;
		}
		listc.Close();
		TS_ASSERT_EQUALS(count, 4);
	}
	c.Close();
}


void CTest_ContactsCache::setUp()
{
	iEngine = 0;
	MContextTestBase::setUp();
	
	iEngine = CUTContactEngine::NewL(_L("ContactsCache"), ETrue);
}


void CTest_ContactsCache::tearDown()
{
 	delete iEngine;

	MContextTestBase::tearDown();
}
