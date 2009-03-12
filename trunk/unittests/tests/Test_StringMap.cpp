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

#include "Test_StringMap.h"

#include "app_context_impl.h"

#include "symbian_auto_ptr.h"
#include "stringmap.h"

static void DeleteHBufC( void* ptr )
{
	HBufC* b=static_cast<HBufC*>(ptr);
	delete b;
}

void CTest_StringMap::testCreationL()
{
	_LIT(KKey, "avain");
	
	auto_ptr<CGenericStringMap> map( CGenericStringMap::NewL() );
}


void CTest_StringMap::testAddL()
{
	_LIT(KKey, "avain");
	
	auto_ptr<CGenericStringMap> map( CGenericStringMap::NewL() );
	map->SetDeletor(&DeleteHBufC);

	auto_ptr<HBufC> buf( HBufC::NewL(100) );	
	buf->Des().Append( _L("tavara") );
	map->AddDataL( KKey, buf.release() );
}


void CTest_StringMap::testGetL()
{	
	auto_ptr<CGenericStringMap> map( CGenericStringMap::NewL() );
	map->SetDeletor(&DeleteHBufC);

	_LIT(KKey1, "avain1");
	_LIT(KValue1, "tavara");
	auto_ptr<HBufC> buf1( HBufC::NewL(100) );	
	buf1->Des().Append( KValue1 );
	map->AddDataL( KKey1, buf1.release() );
	
	_LIT(KKey2, "avain2");
	_LIT(KValue2, "tavaraa taas");
	auto_ptr<HBufC> buf2( HBufC::NewL(100) );	
	buf2->Des().Append( KValue2 );
	map->AddDataL( KKey2, buf2.release() );
	
	HBufC* b = (HBufC*)(map->GetData( KKey1 ));
	TS_ASSERT(b);
	TS_ASSERT_EQUALS_DESCRIPTOR(*b, KValue1());
	
	b = (HBufC*)(map->GetData( KKey2 ));
	TS_ASSERT(b);
	TS_ASSERT_EQUALS_DESCRIPTOR(*b, KValue2());

	_LIT(KKey3, "avain3");
	b = (HBufC*)(map->GetData( KKey3 ));
	TS_ASSERT(! (b) );

	TS_ASSERT_EQUALS( map->Count(), 2 );
}


void CTest_StringMap::testDeleteL()
{	
	auto_ptr<CGenericStringMap> map( CGenericStringMap::NewL() );
	map->SetDeletor(&DeleteHBufC);

	_LIT(KKey1, "avain1");
	_LIT(KValue1, "tavara");
	auto_ptr<HBufC> buf1( HBufC::NewL(100) );	
	buf1->Des().Append( KValue1 );
	map->AddDataL( KKey1, buf1.release() );
	
	_LIT(KKey2, "avain2");
	_LIT(KValue2, "tavaraa taas");
	auto_ptr<HBufC> buf2( HBufC::NewL(100) );	
	buf2->Des().Append( KValue2 );
	map->AddDataL( KKey2, buf2.release() );
	
	HBufC* b = (HBufC*)(map->GetData( KKey1 ));
	TS_ASSERT(b);
	TS_ASSERT_EQUALS_DESCRIPTOR(*b, KValue1());
	TS_ASSERT_EQUALS( map->Count(), 2 );
	
	map->DeleteL( KKey1 );
	TS_ASSERT_EQUALS( map->Count(), 1 );
	b = (HBufC*)(map->GetData( KKey1 ));
	TS_ASSERT(!b);
	
	b = (HBufC*)(map->GetData( KKey2 ));
	TS_ASSERT(b);
	TS_ASSERT_EQUALS_DESCRIPTOR(*b, KValue2());
}


void CTest_StringMap::testResetL()
{	
	auto_ptr<CGenericStringMap> map( CGenericStringMap::NewL() );
	map->SetDeletor(&DeleteHBufC);

	_LIT(KKey1, "avain1");
	_LIT(KValue1, "tavara");
	auto_ptr<HBufC> buf1( HBufC::NewL(100) );	
	buf1->Des().Append( KValue1 );
	map->AddDataL( KKey1, buf1.release() );
	
	_LIT(KKey2, "avain2");
	_LIT(KValue2, "tavaraa taas");
	auto_ptr<HBufC> buf2( HBufC::NewL(100) );	
	buf2->Des().Append( KValue2 );
	map->AddDataL( KKey2, buf2.release() );

	TS_ASSERT_EQUALS( map->Count(), 2 );
	map->Reset();
	TS_ASSERT_EQUALS( map->Count(), 0 );
	
	auto_ptr<HBufC> buf3( HBufC::NewL(100) );	
	buf3->Des().Append( KValue2 );
	map->AddDataL( KKey2, buf3.release() );
	TS_ASSERT_EQUALS( map->Count(), 1 );

	HBufC* b = (HBufC*)(map->GetData( KKey2 ));
	TS_ASSERT(b);
	TS_ASSERT_EQUALS_DESCRIPTOR(*b, KValue2());
}


void CTest_StringMap::setUp()
{
	MContextTestBase::setUp();
}


void CTest_StringMap::tearDown()
{
	MContextTestBase::tearDown();
}


