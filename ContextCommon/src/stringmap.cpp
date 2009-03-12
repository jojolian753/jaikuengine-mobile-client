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

#include "stringmap.h"

#include "symbian_auto_ptr.h"


EXPORT_C void CGenericStringMap::AddDataL(const TDesC& aKey, void *data, TBool overwrite)
{
	if ( aKey.Length() > KStringMapKeyMaxLength ) User::Leave(KErrTooBig);

	if (overwrite) 
		{
			DeleteL( aKey );
		}

	Entry* e = new (ELeave) Entry();
	e->key.Copy( aKey );
	e->data = data;
	iEntries.Append( e );
}



EXPORT_C void* CGenericStringMap::GetData(const TDesC& aKey)
{
	TInt ix = FindKeyL( aKey );
	if ( ix >= 0 )
		{
			return iEntries[ix]->data;
		}
	else
		{
			return NULL;
		}
}

EXPORT_C  CGenericStringMap::~CGenericStringMap()
{
	Reset();
}

EXPORT_C  int CGenericStringMap::Count() const
{
	return iEntries.Count();
}

EXPORT_C  void CGenericStringMap::DeleteL(const TDesC& aKey)
{
	TInt ix = FindKeyL(aKey);
	if ( ix >= 0 )
		{
			Entry* e = iEntries[ix];
			iEntries.Remove( ix );
			(*iDeletor)(e->data);
			delete e; 
		}
}


EXPORT_C void CGenericStringMap::SetDeletor( void(*delete_func)(void* data) )
{
	iDeletor=delete_func;
}

EXPORT_C CGenericStringMap* CGenericStringMap::NewL()
{
	auto_ptr<CGenericStringMap> ret(new (ELeave) CGenericStringMap);
	ret->ConstructL();
	return ret.release();
}


EXPORT_C void CGenericStringMap::Reset()
{
	if ( iDeletor )
		{
			for (TInt i = 0; i < iEntries.Count(); i++)
				{
					Entry* e = iEntries[i];
					(*iDeletor)(e->data);
				}
		}
	iEntries.ResetAndDestroy();
}


CGenericStringMap::CGenericStringMap()
{
}


void CGenericStringMap::ConstructL()
{
}

TInt CGenericStringMap::FindKeyL(const TDesC& aKey)
{
	for (TInt i=0; i < iEntries.Count(); i++)
		{
			if ( iEntries[i]->key.Compare( aKey ) == 0 )
				return i;
		}
	return KErrNotFound;
}


/* 	EXPORT_C virtual MGenericStringMapIterator* CreateIterator(); */


/* 	struct Entry { */
/* 		TStringKey	key; */
/* 		void*	   data; */


