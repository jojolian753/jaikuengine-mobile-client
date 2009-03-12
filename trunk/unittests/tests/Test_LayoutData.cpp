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

#include "Test_LayoutData.h"

#include "juik_layout.h"
#include "juik_layout_impl.h"
#include "jaiku_layoutids.hrh"

#include "symbian_auto_ptr.h"

TSize DoublePortrait()  { return TSize(352, 416); };
TSize DoubleLandscape() { return TSize(416, 352); };

TSize LegacyPortrait()  { return TSize(176, 208); };
TSize LegacyLandscape() { return TSize(208, 176); };

TSize QvgaPortrait()    { return TSize(240, 320); };
TSize QvgaLandscape()   { return TSize(320, 240); };



void CTest_LayoutData::testBasicReadingL()
{
	auto_ptr<CJuikLayout> lay( CJuikLayout::NewL() );
	ASSERT( lay.get() );
	TJuikLayoutItem l = lay->GetLayoutItemL( LG_contacts_list, LI_contacts_list__listbox );
	ASSERT( l.x >= 0 );
	ASSERT( l.y >= 0 );
	ASSERT( l.w >= 0 );
	ASSERT( l.h >= 0 );
}

void CTest_LayoutData::testUpdateLayoutValuesL()
{
	auto_ptr<CJuikLayout> lay( CJuikLayout::NewL() );
	lay->UpdateLayoutDataL( DoublePortrait() );
	lay->GetLayoutItemL( LG_contacts_list, LI_contacts_list__listbox );
}

void CTest_LayoutData::testAllResosReadingL()
{
	auto_ptr<CJuikLayout> lay( CJuikLayout::NewL() );
	lay->UpdateLayoutDataL( DoublePortrait() ); 	
	lay->GetLayoutItemL( LG_contacts_list, LI_contacts_list__listbox );

	lay->UpdateLayoutDataL( DoubleLandscape() ); 	
	lay->GetLayoutItemL( LG_contacts_list, LI_contacts_list__listbox );

	lay->UpdateLayoutDataL( LegacyPortrait() ); 	
	lay->GetLayoutItemL( LG_contacts_list, LI_contacts_list__listbox );

	lay->UpdateLayoutDataL( LegacyLandscape() ); 	
	lay->GetLayoutItemL( LG_contacts_list, LI_contacts_list__listbox );

	lay->UpdateLayoutDataL( QvgaPortrait() ); 	
	lay->GetLayoutItemL( LG_contacts_list, LI_contacts_list__listbox );

	lay->UpdateLayoutDataL( QvgaLandscape() ); 	
	lay->GetLayoutItemL( LG_contacts_list, LI_contacts_list__listbox );
}


void CTest_LayoutData::setUp()
{
	MContextTestBase::setUp();
}


void CTest_LayoutData::tearDown()
{
	MContextTestBase::tearDown();
}
