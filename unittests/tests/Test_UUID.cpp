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

#include "Test_UUID.h"

#include "cc_uuid.h"

#include "app_context_impl.h"
#include "db.h"
#include "jabberdata.h"
#include "cl_settings.h"
#include "context_uids.h"

#include <badesca.h>

void CTest_UUID::testConstructAndDeleteL()
{
	{
		auto_ptr<CUuidGenerator> generator( CreateGeneratorL() );
		TS_ASSERT( generator.get() );
	}
}

void CTest_UUID::testMakeUuidL()
{
	auto_ptr<CUuidGenerator> generator( CreateGeneratorL() );
	TS_ASSERT( generator.get() );
	TBuf8<16> uuid;
	generator->MakeUuidL( uuid );
	TS_ASSERT_EQUALS( uuid.Length(), 16 );
}


void CTest_UUID::testNoSimilarUuidsL()
{
	auto_ptr<CUuidGenerator> generator( CreateGeneratorL() );
	TS_ASSERT( generator.get() );

	auto_ptr<CDesC8Array> uids( new (ELeave) CDesC8ArrayFlat(1000) );
	
	for (TInt i=0; i < 1000; i++)
		{
			TBuf8<16> uid;
			generator->MakeUuidL( uid );
			TS_ASSERT_EQUALS( uid.Length(), 16 );
			uids->InsertIsqL( uid ); // this leaves if uid already exists
		}
}

CUuidGenerator* CTest_UUID::CreateGeneratorL()
{
	const TInt KUidGeneratorId(1); // see ../ids.txt
	return CUuidGenerator::NewL(iJabberData->UserNickL(), KUidSymbianOsUnit, KUidGeneratorId);
}


void CTest_UUID::setUp()
{
	MContextTestBase::setUp();
	
	iJabberDb=CDb::NewL(*iAppContext, _L("JABBER"), EFileWrite|EFileShareAny); 
	iJabberData = CJabberData::NewL(*iAppContext, *iJabberDb, SETTING_JABBER_NICK);
}


void CTest_UUID::tearDown()
{
	delete iJabberData;
	delete iJabberDb;
	MContextTestBase::tearDown();
}
