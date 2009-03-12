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

#include "Test_UserPics.h"

#include "app_context_impl.h"
#include "break.h"
#include "ccu_userpics.h"
#include "cl_settings.h"
#include "csd_userpic.h"
#include "cu_buffer_icon.h"
#include "db.h"
#include "icons.h"
#include "jabberpics.h"
#include "jabberdata.h"
#include "juik_iconmanager.h"
#include "raii_s32file.h"
#include "raii_f32file.h"
#include "symbian_auto_ptr.h"


#include <akniconarray.h>
#include <gulicon.h>
#include <eikfutil.h>

#include "testutils.h"

HBufC8* CTest_UserPics::ReadContentsL(const TDesC& aFile)
{
 	auto_ptr<HBufC8> contents( HBufC8::NewL(10000) );
 	
	TPtr8 ptr = contents->Des();

	RAFile f;
	f.OpenLA( iAppContext->Fs(), aFile, EFileRead );
	TInt err = f.Read(ptr);
	
 	return contents.release();
}

void CTest_UserPics::testUserPics()
{
	auto_ptr<HBufC8> contents( ReadContentsL( _L("c:\\unittestfiles\\buddy.mbm" ) ) );	
	auto_ptr<CBBUserPic> pic( new (ELeave) CBBUserPic() );
	_LIT(KNick, "harrihoo");
 	pic->iNick() = KNick();
 	pic->iPhoneNumberHash() = _L8("abaa");
 	pic->iMbm.Zero();
	pic->iMbm.Append(*contents);
	
	
	TTime now = GetTime();
	BBSession()->PutL( KUserPicTuple, KNick, pic.get(), now + TTimeIntervalMinutes(2) );
	
	TestUtils::WaitForActiveSchedulerStopL(1);
	
	TS_ASSERT( iUserPics->GetIconIndexL(KNick) != -1);
}


void CTest_UserPics::testLazyGet()
{
	auto_ptr<HBufC8> contents( ReadContentsL( _L("c:\\unittestfiles\\buddy.mbm" ) ) );	
	auto_ptr<CBBUserPic> pic( new (ELeave) CBBUserPic() );
	_LIT(KNick, "harrihoo");
 	pic->iNick() = KNick();
 	pic->iPhoneNumberHash() = _L8("abaa");
 	pic->iMbm.Zero();
	pic->iMbm.Append(*contents);
		
	TTime now = GetTime();
	BBSession()->PutL( KUserPicTuple, KNick, pic.get(), now + TTimeIntervalMinutes(2) );
	
	TestUtils::WaitForActiveSchedulerStopL(1);
	
	TS_ASSERT( iUserPics->GetIconIndexL(KNick) != -1);

	delete iUserPics;
	iUserPics = NULL;
	iUserPics = CUserPics::NewL(*iJabberPics, *iJabberData, *iIconManager);	
	
	TS_ASSERT( iUserPics->GetIconIndexL(KNick) != -1);	
}


void CTest_UserPics::setUp()
{
	MContextTestBase::setUp();

	GetContext()->SetDataDir( _L("c:\\unittests\\"), EFalse );
	TFileName jabber;
	jabber.Append( iAppContext->DataDir() );
	jabber.Append( _L("\\") );
	jabber.Append( _L("JABBER.DB") );
	BaflUtils::DeleteFile(iAppContext->Fs(), jabber);
	
	iJabberDb=CDb::NewL(*iAppContext, _L("JABBER"), EFileWrite|EFileShareAny); 
	iJabberPics = CJabberPics::NewL(*iJabberDb);
	iJabberData = CJabberData::NewL(*iAppContext, *iJabberDb, SETTING_JABBER_NICK);
	
	iIconManager = CJuikIconManager::NewL();
	
	iUserPics = CUserPics::NewL(*iJabberPics, *iJabberData, *iIconManager);	
}


void CTest_UserPics::tearDown()
{
	delete iUserPics;
	delete iJabberData;
	delete iJabberPics;
	delete iJabberDb;
	delete iIconManager;
	MContextTestBase::tearDown();
}


