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

#include "Test_JabberData.h"

#include "app_context_impl.h"

#include "jabberdata.h"
#include "db.h"
#include "break.h"
#include "cl_settings.h"

_LIT(KNick1, "jorma");	
const TInt KId1(10);


_LIT(KNick2, "pertti");	
const TInt KId2(12);

void CTest_JabberData::testSetJabberNickL()
{
	iJabberData->SetJabberNickL(KId1, KNick1, CJabberData::ESetByUser);
}

void CTest_JabberData::testGetJabberNickL()
{
	CJabberData::TNick nick;
	TS_ASSERT( ! iJabberData->GetJabberNickL(KId1, nick) );
	iJabberData->SetJabberNickL(KId1, KNick1, CJabberData::ESetByUser);
	TS_ASSERT( iJabberData->GetJabberNickL(KId1, nick) );
	TS_ASSERT_EQUALS_DESCRIPTOR( nick, KNick1() );
}


void CTest_JabberData::testDummyNicksL()
{
	_LIT(KDummyNick1, "valhalla");
	_LIT(KDummyNick2, "spede");
	_LIT(KDummyNick3, "nurmio");
	CJabberData::TNick nick;

	iJabberData->SetJabberNickL(KId1, KNick1, CJabberData::ESetByUser);
	iJabberData->SetJabberNickL(KId2, KNick2, CJabberData::ESetByUser);

	TInt dummyId1 = iJabberData->GetNewDummyContactIdL();
	TS_ASSERT( dummyId1 < -10 );
	
	iJabberData->SetJabberNickL(dummyId1, KDummyNick1, CJabberData::EAutomaticDummy);
	
	TInt dummyId2 = iJabberData->GetNewDummyContactIdL();
	TS_ASSERT_EQUALS( dummyId1 - 1, dummyId2 );
	iJabberData->SetJabberNickL(dummyId2, KDummyNick2, CJabberData::EAutomaticDummy);

	TS_ASSERT( iJabberData->GetJabberNickL(dummyId1, nick) );
	TS_ASSERT_EQUALS_DESCRIPTOR( nick, KDummyNick1() );
	TS_ASSERT( iJabberData->GetJabberNickL(dummyId2, nick) );
	TS_ASSERT_EQUALS_DESCRIPTOR( nick, KDummyNick2() );
	
	TInt dummyId3 = iJabberData->GetNewDummyContactIdL();
	TS_ASSERT_EQUALS( dummyId2 - 1, dummyId3 );
	iJabberData->SetJabberNickL(dummyId3, KDummyNick3, CJabberData::EAutomaticDummy);

	TS_ASSERT_EQUALS( dummyId3 - 1, iJabberData->GetNewDummyContactIdL());
}


void CTest_JabberData::setUp()
{
	MContextTestBase::setUp();

	GetContext()->SetDataDir( _L("c:\\unittests\\"), EFalse );
	TFileName jabber;
	jabber.Append( DataDir() );
	jabber.Append( _L("\\") );
	jabber.Append( _L("JABBER.DB") );	
	BaflUtils::DeleteFile(Fs(), jabber);

	CC_TRAPD(err, { iJabberDb=CDb::NewL(AppContext(), _L("JABBER"), EFileWrite|EFileShareAny); } );
	if ( ! err )
		{
			iJabberData = CJabberData::NewL(*GetContext(),*iJabberDb, SETTING_JABBER_NICK);
		}
}


void CTest_JabberData::tearDown()
{
	delete iJabberData;
	delete iJabberDb;

	MContextTestBase::tearDown();
}


