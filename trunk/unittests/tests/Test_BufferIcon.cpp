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

#include "Test_BufferIcon.h"

#include "cu_buffer_icon.h"

#include "symbian_auto_ptr.h"
#include "raii_s32file.h"
#include "raii_f32file.h"
#include "break.h"

#include <eikfutil.h>

#ifdef __WINS__
#ifdef __S60V3__
_LIT( KMbmFile, "z:\\resource\\unittests.mbm" );
_LIT( KMbmFile2, "z:\\resource\\unittests2.mbm" );
#else
_LIT( KMbmFile, "z:\\system\\data\\unittests.mbm" );
_LIT( KMbmFile2, "z:\\system\\data\\unittests2.mbm" );
#endif
#else
_LIT( KMbmFile, "c:\\resource\\unittests.mbm" );
_LIT( KMbmFile2, "c:\\resource\\unittests2.mbm" );
#endif

HBufC8* CTest_BufferIcon::ReadContentsL(const TDesC& aFile)
{
	TFileName real = aFile;
#ifdef __S60V3__
	TParse p; p.Set(aFile, NULL, NULL);
	real = p.Drive();
	real.Append(_L("\\resource\\"));
	real.Append(p.NameAndExt());
#endif
	TEntry e;
	User::LeaveIfError(Fs().Entry(aFile, e));
 	auto_ptr<HBufC8> contents( HBufC8::NewL(e.iSize) );
 	
	TPtr8 ptr = contents->Des();

	RAFile f;
	f.OpenLA( Fs(), aFile, EFileRead );
	User::LeaveIfError(f.Read(ptr));
	
 	return contents.release();
}

void CTest_BufferIcon::testCount()
{
	{
		auto_ptr<HBufC8> buf( ReadContentsL(KMbmFile) );
		TS_ASSERT_EQUALS( 1, NumberOfBitmapsL( *buf ) );
	}
	{
		auto_ptr<HBufC8> buf( ReadContentsL(KMbmFile2) );
		TS_ASSERT_EQUALS( 2, NumberOfBitmapsL( *buf ) );
	}
}

void CTest_BufferIcon::testLoad()
{
	{

		auto_ptr<HBufC8> buf( ReadContentsL(KMbmFile) );
		auto_ptr<CFbsBitmap> bm(LoadBitmapL( *buf, 0, Ws(), screen ));
		TInt casted=(TInt)bm.get();
		TS_ASSERT_DIFFERS( 0, casted );

		TSize s=bm->SizeInPixels();
		TS_ASSERT_EQUALS( 30, s.iWidth );
		TS_ASSERT_EQUALS( 30, s.iHeight );
	}
	{

		auto_ptr<HBufC8> buf( ReadContentsL(KMbmFile2) );
		auto_ptr<CFbsBitmap> bm(LoadBitmapL( *buf, 0, Ws(), screen ));

		TSize s=bm->SizeInPixels();
		TS_ASSERT_EQUALS( 30, s.iWidth );
		TS_ASSERT_EQUALS( 30, s.iHeight );

		bm.reset( LoadBitmapL( *buf, 1 ) );
		s=bm->SizeInPixels();
		TS_ASSERT_EQUALS( 45, s.iWidth );
		TS_ASSERT_EQUALS( 60, s.iHeight );

		TRAPD(err, LoadBitmapL( *buf, 2 ) );
		TS_ASSERT_DIFFERS(err, KErrNone);
	}
}

void CTest_BufferIcon::testBroken()
{
	{
		TBuf8<20> t;
		t.Fill( TChar('a'), t.MaxLength() );
		TRAPD(err, NumberOfBitmapsL(t));
		TS_ASSERT_DIFFERS( err, KErrNone );
	}
	{
		auto_ptr<HBufC8> buf( ReadContentsL(KMbmFile) );
		TPtr8 p( (TUint8*)buf->Ptr()+21, buf->Length() -60 );
		p.Fill( TChar('a'), p.MaxLength() );
		auto_ptr<CFbsBitmap> bm(0);
		TRAPD(err, bm.reset(LoadBitmapL( *buf, 0, Ws(), screen )));
		TS_ASSERT_DIFFERS( err, KErrNone );
	}
}

void CTest_BufferIcon::setUp()
{
	screen=0;
	MContextTestBase::setUp();
	screen=new (ELeave) CWsScreenDevice(Ws());
	User::LeaveIfError(screen->Construct());
}

void CTest_BufferIcon::tearDown()
{
	delete screen; screen=0;
	MContextTestBase::tearDown();
}

