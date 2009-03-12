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

#include "ccu_contact.h"


EXPORT_C contact* contact::NewL(TContactItemId i_id, const TDesC& i_first_name, const TDesC& i_last_name, const TDesC& i_extra_name) 
{
	CALLSTACKITEMSTATIC_N(_CL("contact"), _CL("NewL"));
	auto_ptr<contact> c( new (ELeave) contact );
	c->id=i_id;	
	c->first_name=i_first_name.AllocL();
	c->last_name=i_last_name.AllocL();
	c->extra_name=i_extra_name.AllocL();
	
	return c.release();
}


EXPORT_C contact::contact() {
	id=0;
	first_name=0;
	last_name=0;
	extra_name=0;
	presence=0;
	has_nick=false;
	is_myself=false;
	show_details_in_list=ETrue;
	iUnreadCount = KErrNotFound;
	iStreamCount = KErrNotFound;
}

void SafeAppend( const TDesC& aSrc, TDes& aTgt )
{
	TInt free = aTgt.MaxLength() - aTgt.Length();
	if ( free < aSrc.Length() )
		aTgt.Append( aSrc.Left(free) );
	else 
		aTgt.Append( aSrc );		
}

EXPORT_C void contact::AppendName(TDes& aTgt, bool aLastNameFirst)
{
 	CALLSTACKITEM_N(_CL("contact"), _CL("AppendName"));
	TPtrC n1  = aLastNameFirst ? LastName() : FirstName();
	TPtrC n2 =  aLastNameFirst ? FirstName() : LastName();
	TBool addSeparator = n1.Length() > 0 && n2.Length() > 0;
	
	_LIT(KSeparator, " ");
	_LIT(KUnnamed, "(Unnamed)");

	SafeAppend( n1, aTgt );
	if (addSeparator) SafeAppend(KSeparator, aTgt);
	SafeAppend( n2, aTgt );

	if (aTgt.Length() == 0)
		SafeAppend(KUnnamed, aTgt);
}


EXPORT_C HBufC* contact::NameL(TBool aLastNameFirst)
{
	CALLSTACKITEM_N(_CL("contact"), _CL("NameL"));
	TInt length = 2 * (FirstName().Length() + LastName().Length() );
	auto_ptr<HBufC> result( HBufC::NewL(length) );
	TPtr ptr = result->Des();
	AppendName(ptr, aLastNameFirst);
	return result.release();
}


EXPORT_C void contact::Name(HBufC*& Into, bool last_name_first)
{
	CALLSTACKITEM_N(_CL("contact"), _CL("Name"));
	contact *c=this;
	TInt len=0;
	if (c->first_name) len+=c->first_name->Length();
	if (c->last_name) len+=c->last_name->Length()+50;
	len+=c->time.Length();
	if (len < 10) len=10;

	if (len > Into->Des().MaxLength()) {
		delete Into; Into=0;
		Into=HBufC::NewL(len);
	} else {
		Into->Des().Zero();
	}

	if (c->time.Length()>0) {
		Into->Des().Append(time);
		Into->Des().Append(_L(" "));
	}
	if (last_name_first) 
	{
		if (c->last_name) Into->Des().Append(*(c->last_name));
		if (c->last_name && c->last_name->Length()>0 && c->first_name && c->first_name->Length()>0) 
		{
			Into->Des().Append(_L(" "));
		}
		if (c->first_name) Into->Des().Append(*(c->first_name));
	} else {
		if (c->first_name) Into->Des().Append(*(c->first_name));
		if (c->first_name && c->first_name->Length()>0 && c->last_name && c->last_name->Length()>0) 
		{
			Into->Des().Append(_L(" "));
		}
		if (c->last_name) Into->Des().Append(*(c->last_name));
	}
	if (Into->Des().Length() == 0)
	{
		Into->Des().Append(_L("(Unnamed)"));
	}
}

EXPORT_C void contact::set_presence(CBBPresence* data)
{
	CALLSTACKITEM_N(_CL("contact"), _CL("set_presence"));

	if (presence) presence->Release();
	presence=data;
	if (presence && !has_nick) has_nick=ETrue;
	if (data) data->AddRef();

}


EXPORT_C contact::~contact() {
	if (presence) presence->Release();
	delete first_name;
	delete last_name;
	delete extra_name;
}


EXPORT_C TPtrC contact::FirstName() const
{
	return first_name ? TPtrC(*first_name) : TPtrC(KNullDesC);
}

EXPORT_C TPtrC contact::LastName() const
{
	return last_name ? TPtrC(*last_name) : TPtrC(KNullDesC);
}


EXPORT_C TPtrC contact::ExtraName() const
{
	return extra_name ? TPtrC(*extra_name) : TPtrC(KNullDesC);
}

EXPORT_C void contact::SetFirstNameL(const TDesC& aName)
{
	if ( first_name ) 
		{ delete first_name; first_name = NULL; }
	first_name = aName.AllocL();
}

EXPORT_C void contact::SetLastNameL(const TDesC& aName)
{
	if ( last_name )		
		{ delete last_name; last_name = NULL; }
	last_name = aName.AllocL();
}


EXPORT_C contact* contact::CloneL()
{
	auto_ptr<contact> c( contact::NewL(id, FirstName(), LastName(), ExtraName()) );
	c->current_idx = current_idx;
	c->has_nick = has_nick;
	c->is_myself = is_myself;
	c->show_details_in_list = show_details_in_list;
	c->time = time;
	c->set_presence( presence );
	return c.release();
}

EXPORT_C bool contact::operator==(const contact& aRhs) const
{
	return (
		FirstName() == aRhs.FirstName() &&
		LastName() == aRhs.LastName() &&
		ExtraName() == aRhs.ExtraName() &&
		has_nick == aRhs.has_nick &&
		is_myself == aRhs.is_myself
		);
}
