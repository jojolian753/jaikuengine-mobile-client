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

#include "ccu_namefilter.h"

#include "phonebook.h"

class CNameArrayImpl : public CNameArray {
public:
	CNameArrayImpl(phonebook_i* aPhoneBook);
	void ConstructL();
	virtual TInt MdcaCount() const;
	virtual TPtrC16 MdcaPoint(TInt aIndex) const;
	~CNameArrayImpl();
private:
	bool	last_name_first;
	phonebook_i*	iPhoneBook;
	mutable HBufC*	iBuf;
};

EXPORT_C CNameArray* CNameArray::NewL(phonebook_i* aPhoneBook)
{
	CNameArrayImpl* ret=new (ELeave) CNameArrayImpl(aPhoneBook);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop();
	return ret;
}

CNameArrayImpl::CNameArrayImpl(phonebook_i* aPhoneBook) : iPhoneBook(aPhoneBook)
{
}

CNameArrayImpl::~CNameArrayImpl()
{
	delete iBuf;
}

void CNameArrayImpl::ConstructL()
{
	iBuf=HBufC::NewL(256);

	last_name_first=true;

#ifdef __S60V2__
	CPbkContactEngine *eng = CPbkContactEngine::Static();
	CPbkContactEngine::TPbkNameOrder order;
	if (!eng) {order = CPbkContactEngine::EPbkNameOrderLastNameFirstName;}
	else {order = eng->NameDisplayOrderL();}
	if (order != CPbkContactEngine::EPbkNameOrderLastNameFirstName) last_name_first=false;
#endif
}

TInt CNameArrayImpl::MdcaCount() const
{
	return iPhoneBook->Count();
}

TPtrC16 CNameArrayImpl::MdcaPoint(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CNameArrayImpl"), _CL("MdcaPoint"));
	contact* c=iPhoneBook->GetContact(aIndex);

	if (!c) iBuf->Des().Zero();
	else {
		c->Name( iBuf, last_name_first);
	}
	
	return iBuf->Des();
}

