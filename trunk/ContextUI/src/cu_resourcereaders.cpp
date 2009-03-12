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

#include "cu_resourcereaders.h"

#include "symbian_auto_ptr.h"

#include <eikenv.h>

namespace ResourceReaders
{

	EXPORT_C TPtrC CSettingItem::EmptyOrText(HBufC* aText)
	{
		if ( aText ) return *aText;
		else         return TPtrC(KNullDesC);
	}
	
	EXPORT_C TPtrC CSettingItem::Name()           { return EmptyOrText( iName );  }
	EXPORT_C TPtrC CSettingItem::EmptyItemText()  { return EmptyOrText( iEmptyItemText  ); }
	EXPORT_C TPtrC CSettingItem::CompulsoryText() { return EmptyOrText( iCompulsoryText ); }
	
	EXPORT_C CSettingItem::CSettingItem()
	{
	}

	EXPORT_C CSettingItem::~CSettingItem()
	{
		CALLSTACKITEM_N(_CL("CSettingItem"), _CL("~CItemResource") );
		delete iName;
		delete iEmptyItemText;
		delete iCompulsoryText;
	}
	
	
	EXPORT_C void CSettingItem::ReadResourceL(TResourceReader& aReader)
	{
		CALLSTACKITEM_N(_CL("CSettingItem"), _CL("ReadResourceL(TResourceReader)") );
		iIdentifier              = aReader.ReadInt16();
		iName                    = aReader.ReadHBufCL();
		iSettingPageResource     = aReader.ReadInt32();
		iType                    = aReader.ReadInt16();
		iSettingEditorResource   = aReader.ReadInt32();
		iAssociatedResource      = aReader.ReadInt32();
		iEmptyItemText           = aReader.ReadHBufCL();
		iCompulsoryText          = aReader.ReadHBufCL();
		// throw away reserved extension pointer:
		aReader.ReadInt32();
	}

	EXPORT_C void CSettingItem::ReadResourceL(TInt aResourceId, CEikonEnv* aEnv)
	{
		CALLSTACKITEM_N(_CL("CSettingItem"), _CL("ReadResourceL(TInt)") );
		CEikonEnv* env = aEnv;
		if ( ! env )
			env = CEikonEnv::Static();
		
		TResourceReader reader;
		env->CreateResourceReaderLC( reader,  aResourceId  );
		ReadResourceL( reader );
		CleanupStack::PopAndDestroy(); // reader 
	}
	
	EXPORT_C CSettingItemList::CSettingItemList()
	{
	}

	EXPORT_C CSettingItemList::~CSettingItemList()
	{
		CALLSTACKITEM_N(_CL("CSettingItemList"), _CL("~CListResource") );
		delete iTitle;
		iItems.ResetAndDestroy();
	}
		
	EXPORT_C void CSettingItemList::ReadResourceL(TResourceReader& aReader)
	{
		CALLSTACKITEM_N(_CL("CSettingItemList"), _CL("ReadResourceL") );
		iFlags                  = aReader.ReadInt16();
		iTitle                  = aReader.ReadHBufCL();
		iOrdinal                = aReader.ReadInt16();
		
		TInt count = aReader.ReadInt16();
		for (TInt i = 0; i < count; i++)
			{
				auto_ptr<CSettingItem> item( new (ELeave) CSettingItem );
				item->ReadResourceL( aReader );
				iItems.AppendL( item.release() );
			}
	}		
	
	EXPORT_C CSettingItem* CSettingItemList::Find(TInt aIdentifier) 
	{
		CALLSTACKITEM_N(_CL("CSettingItemList"), _CL("Find") );
		for (TInt i=0; i < iItems.Count(); i++)
			{
				if ( iItems[i]->iIdentifier == aIdentifier )
					return iItems[i];
			}
		return NULL;
	}
}
