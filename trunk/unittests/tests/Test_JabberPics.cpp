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

#include "Test_JabberPics.h"

#include "app_context_impl.h"

#include "jabberpics.h"
#include "db.h"
#include "break.h"
#include "cu_buffer_icon.h"
#include "symbian_auto_ptr.h"
#include "icons.h"

#include <akniconarray.h>
#include <gulicon.h>


_LIT(KJorma, "jorma");	
void CTest_JabberPics::testGet()
{

	TS_ASSERT( ! iJabberPics->FindL( KJorma ) );	
	auto_ptr<CFbsBitmap> bmp( iJabberPics->GetPicL( KJorma ) );
	TS_ASSERT( ! bmp.get() );
}

void CTest_JabberPics::testSet()
{
	auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(1) );
	const TIconID id = _INIT_T_ICON_ID("z:\\system\\data\\unittests.mbm" , 0, KErrNotFound);
	LoadIcons( icons.get(), &id, 1);
	TS_ASSERT_EQUALS( icons->Count(), 1 );
	
	CFbsBitmap* bmp1 = icons->At(0)->Bitmap();
	TS_ASSERT( bmp1 );

	TInt ignored;
	iJabberPics->SetPicL( KJorma, *bmp1, ignored );
	
	TS_ASSERT( iJabberPics->FindL( KJorma ) );
	
	auto_ptr<CFbsBitmap> bmp2( iJabberPics->GetPicL( KJorma ) );
	TS_ASSERT( bmp2.get() );
	TS_ASSERT_EQUALS( bmp1->SizeInPixels().iWidth,  bmp2->SizeInPixels().iWidth );
	TS_ASSERT_EQUALS( bmp1->SizeInPixels().iHeight,  bmp2->SizeInPixels().iHeight );
}


void CTest_JabberPics::testGetAll()
{
	auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(1) );
	const TIconID id = _INIT_T_ICON_ID("z:\\system\\data\\unittests.mbm" , 0, KErrNotFound);
	LoadIcons( icons.get(), &id, 1);
	TS_ASSERT_EQUALS( icons->Count(), 1 );
	
	auto_ptr<CDesCArray> nicks( new (ELeave) CDesCArrayFlat(4) );	
	nicks->AppendL( _L("jorma") );
	nicks->AppendL( _L("petteri") );
	nicks->AppendL( _L("tauno") );

	TInt ignored;
	CFbsBitmap* bmp1 = icons->At(0)->Bitmap();		
	for ( TInt i=0; i < nicks->Count(); i++)
		{
			iJabberPics->SetPicL( nicks->MdcaPoint(i), *bmp1, ignored );
		}
	
	for ( TInt i=0; i < nicks->Count(); i++)
		{
			TS_ASSERT( iJabberPics->FindL( nicks->MdcaPoint(i) ) );
		}

	auto_ptr<CDesCArray> fetchedNicks( new (ELeave) CDesCArrayFlat(4) );
	auto_ptr<CArrayPtrFlat<CFbsBitmap> > fetchedBmps( new (ELeave) CArrayPtrFlat<CFbsBitmap>(4) );	
	
	iJabberPics->GetAllPicsL(*fetchedNicks, *fetchedBmps);

	TS_ASSERT_EQUALS( fetchedNicks->Count(), nicks->Count() );
	TS_ASSERT_EQUALS( fetchedBmps->Count(), fetchedNicks->Count() );
	
	for ( TInt i=0; i < nicks->Count(); i++ )
		{
			TBool found = EFalse;
			for ( TInt j=0; j < fetchedNicks->Count(); j++)
				{
					if ( fetchedNicks->MdcaPoint(j).Compare( nicks->MdcaPoint(i) ) )
						{
							found = ETrue;
							break;
						}
					CFbsBitmap* bmp2 = fetchedBmps->At(j);
					TS_ASSERT_EQUALS( bmp1->SizeInPixels().iWidth,  bmp2->SizeInPixels().iWidth );
					TS_ASSERT_EQUALS( bmp1->SizeInPixels().iHeight,  bmp2->SizeInPixels().iHeight );
				}		
			TS_ASSERT(found);
		}
	fetchedBmps->ResetAndDestroy();
}


void CTest_JabberPics::testEmptyGetAll()
{

	auto_ptr<CDesCArray> fetchedNicks( new (ELeave) CDesCArrayFlat(4) );
	auto_ptr<CArrayPtrFlat<CFbsBitmap> > fetchedBmps( new (ELeave) CArrayPtrFlat<CFbsBitmap>(4) );	
	
	iJabberPics->GetAllPicsL(*fetchedNicks, *fetchedBmps);

	TS_ASSERT_EQUALS( fetchedNicks->Count(), 0 );
	TS_ASSERT_EQUALS( fetchedBmps->Count(), 0 );
	
}

void CTest_JabberPics::setUp()
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
			iJabberPics = CJabberPics::NewL(*iJabberDb);
		}
		
	{
		auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(1) );
		const TIconID id = _INIT_T_ICON_ID("z:\\system\\data\\unittests.mbm" , 0, KErrNotFound);
		LoadIcons( icons.get(), &id, 1);
	}
}


void CTest_JabberPics::tearDown()
{
	delete iJabberPics;
	delete iJabberDb;

	MContextTestBase::tearDown();
}


