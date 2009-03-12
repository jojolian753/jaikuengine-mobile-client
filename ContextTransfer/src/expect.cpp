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

#include "expect.h"

#include "app_context.h"
#include "symbian_auto_ptr.h"

CExpect::CExpect() : bol(true)
{
	CALLSTACKITEM_N(_CL("CExpect"), _CL("CExpect"));

}

void CExpect::ConstructL()
{
	CALLSTACKITEM_N(_CL("CExpect"), _CL("ConstructL"));

}

EXPORT_C CExpect* CExpect::NewL()
{
	CALLSTACKITEM_N(_CL("CExpect"), _CL("NewL"));

	auto_ptr<CExpect> ret(new (ELeave) CExpect);
	ret->ConstructL();
	return ret.release();
}

EXPORT_C void CExpect::SetPatternL(const TDesC8& pattern)
{
	CALLSTACKITEM_N(_CL("CExpect"), _CL("SetPatternL"));

	delete pat; pat=0;
	pat=pattern.AllocL();
	expect_pos=0;
	handle_next_place();
}

EXPORT_C void CExpect::Reset()
{
	CALLSTACKITEM_N(_CL("CExpect"), _CL("Reset"));

	bol=true;
}

EXPORT_C CExpect::~CExpect()
{
	CALLSTACKITEM_N(_CL("CExpect"), _CL("~CExpect"));

	delete pat;
}

EXPORT_C bool CExpect::handle_input(char c)
{
	CALLSTACKITEM_N(_CL("CExpect"), _CL("handle_input"));

	if (pat->Length()==0) {
		if (c=='\n') bol=true;
		else bol=false;

		return false;
	}

	if (expect_pos==pat->Length()) {
		if (c=='\n') bol=true;
		else bol=false;
		return true;
	}

	bool matched=false;

	if (bol && ! match_str.Compare(_L8("^")) ) {
		expect_pos++;
		handle_next_place();
	}

	TBuf8<1> cm;
	cm.Append(c);

	if (match_any) {
		matched=true;
	} else if(neg_match) {
		if (match_str.Find(cm)==KErrNotFound) matched=true;
	} else {
		if (match_str.Find(cm)!=KErrNotFound) matched=true;
	}

	if (c=='\n') bol=true;
	else bol=false;

	if (matched) {
		expect_pos++;
		if (expect_pos==pat->Length()) {
			return true;
		}
	} else {
		expect_pos=0;
	}

	handle_next_place();
	return false;
}

void CExpect::handle_next_place()
{
	CALLSTACKITEM_N(_CL("CExpect"), _CL("handle_next_place"));

	if (pat->Length()==0) return;
	neg_match=false;

	match_any=false;

	TChar e=(*pat)[expect_pos];

	match_str.Zero();

	if ( e=='?' ) {
		match_any=true;
	} else if ( e=='[') {
		expect_pos++;
		e=(*pat)[expect_pos];
		if ( e == '^' ) {
			neg_match=true;
			expect_pos++;
		}
		while ( (e=(*pat)[expect_pos]) != ']' ) {
			match_str.Append(e);
			expect_pos++;
		}
	} else {
		match_str.Append(e);
	}
}
