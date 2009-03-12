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

#include "juik_gfxstore.h"

#include "juik_icons.h"

#include "app_context.h"
#include "symbian_auto_ptr.h"

#include <gulicon.h>
#include <fbs.h> 

class CJuikGfxStoreImpl : public CJuikGfxStore, public MContextBase
{
public:
	class CGraphicItem : public CBase, public MContextBase
	{
	public:
		CGraphicItem(TComponentName aId, TSize aSize, CGulIcon* aIcon) 
			: iId(aId), iSize(aSize), iIcon(aIcon), 
			  iRefCount(1) // we start from 1, because items are cached. 
			  // Caching can be nulled, e.g. when screen is changed etc. 
		{}
		
		~CGraphicItem()
		{
			delete iIcon;
		}
		
		virtual void AddRef() 
		{
			iRefCount++;
		}

		virtual void Release() 
		{
			iRefCount--;
			// we will cache already created items, 
			// and release only if an extra release is called
			if (iRefCount <= 0) 
				{
					delete iIcon;
					iIcon = NULL;
				}
		}
 

		TComponentName iId;
		TSize iSize;
		CGulIcon* iIcon;
		mutable TInt iRefCount;
	};
	
	CJuikGfxStoreImpl() {}
	
	void ConstructL(const TDesC& aFileName)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("ConstructL"));		
		iFileName.Copy(aFileName);
	}
	
	~CJuikGfxStoreImpl()
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("~CJuikGfxStoreImpl"));		
		iGraphics.ResetAndDestroy();
	}
	

	TInt FindIcon(TComponentName aId, TSize aSize)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("FindIcon(TComponentName..."));		
		for (TInt i = 0; i < iGraphics.Count(); i++)
			{
				CGraphicItem* item = iGraphics[i];
				if ( item->iId == aId && item->iSize == aSize )
					return i;
			}
		return KErrNotFound;
	}

	
	TInt FindIcon(CGulIcon* aIcon)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("FindIcon(CGulIcon)"));		
		for (TInt i = 0; i < iGraphics.Count(); i++)
			{
				CGraphicItem* item = iGraphics[i];
				if ( item->iIcon == aIcon )
					return i;
			}
		return KErrNotFound;
	}

	TBool ReleaseIconImpl(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("ReleaseIconImpl"));		
		TInt ix = aIndex;
		if ( ix != KErrNotFound )
			{
				CGraphicItem* item = iGraphics[ix];
				item->Release();
				if ( item->iIcon == NULL )
					{
						delete item;
						iGraphics.Remove(ix);
						return ETrue;
					}
			}
		return EFalse;
	}

	void ReleaseIcon(CGulIcon* aIcon)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("ReleaseIcon(CGulIcon)"));		
		if ( aIcon )
			{
				TInt ix = FindIcon( aIcon );
				ReleaseIconImpl(ix);
			}
	}

	void ReleaseIcon(TComponentName aId, TSize aSize)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("ReleaseIcon(TComponentName...)"));		
		TInt ix = FindIcon( aId, aSize );
		ReleaseIconImpl(ix);
	}
	

	void ReleaseCachedOrphansL()
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("ReleaseCachedOrphansL"));
		TInt i = 0;
		while ( i < iGraphics.Count() ) 
			{
				if ( ! ReleaseIconImpl(i) )
					i += 1;
			}
	}
		
//  	void RemoveUnusedSizes()
// 	{
// 		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("RemoveUnusedSizes"));		
// 		TInt i = 0;
// 		while ( i < iGraphics.Count() )
// 			{
// 				CGraphicItem* item = iGraphics[i];
// 				if ( item->iId == aId && item->iIcon == NULL  )
// 					{
// 						delete item;
// 						iGraphics.Remove(i);
// 					}
// 				else
// 					{ i += 1; }
// 			}
// 	}


	CGulIcon* SetIconL(TComponentName aId, CGulIcon* aIcon)
	{		
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("SetIconL"));
		auto_ptr<CGulIcon> icon( aIcon );
		TSize iconSize = aIcon->Bitmap()->SizeInPixels();
		if ( iconSize == TSize(0,0) )
			{
				Bug( _L("No zero-sized icons allowed in cache") ).Raise();
				return NULL;
			}

		TInt ix = FindIcon( aId, iconSize  );
		if ( ix != KErrNotFound )
			{
				Bug( _L("Icon already in cache") ).Raise();
				return NULL;
			}
		
		auto_ptr<CGraphicItem> item( new (ELeave) CGraphicItem( aId, iconSize, icon.release()  ) );
		item->AddRef();
		iGraphics.AppendL( item.get() );
		CGulIcon* result = item->iIcon;
		item.release();
		return result;
	}
	
	
	CGulIcon* GetIcon(TComponentName aId, TSize aSize)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("GetIcon"));
		TInt ix = FindIcon( aId, aSize );
		if ( ix != KErrNotFound )
			{
				CGraphicItem* item = iGraphics[ix];
				item->AddRef();
				return item->iIcon;
			}
		return NULL;
	}
	


	CGulIcon* LoadColoredIconL(TComponentName aName, TInt aId, TSize aSize, TRgb aRgb)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("LoadColoredIconL"));
		auto_ptr<CGulIcon> icon(JuikIcons::CreateColoredIconL(iFileName,
															  aId, 
															  aSize, 
															  aRgb, 
															  EAspectRatioNotPreserved));
		CGulIcon* result = SetIconL( aName, icon.release());
		return result;
	}
	
	CGulIcon* GetColoredIconL(TComponentName aName, TInt aId, TSize aSize, TRgb aRgb)
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("GetColoredIconL(4)"));
		CGulIcon* icon = GetIcon( aName, aSize );
		if ( ! icon )
			icon = LoadColoredIconL( aName, aId, aSize, aRgb);
		return icon;
	}


	virtual CGulIcon* GetColoredIconL(TGfxDef aDef) 
	{
		CALLSTACKITEM_N(_CL("CJuikGfxStoreImpl"), _CL("GetColoredIconL(TGfxDef)"));
		return GetColoredIconL( aDef.iIconName, aDef.iIconId, aDef.iSize, aDef.iColor );
	}

	RPointerArray<CGraphicItem> iGraphics;
	TFileName iFileName;
}; 

EXPORT_C CJuikGfxStore* CJuikGfxStore::NewL(const TDesC& aFileName)
{
	CALLSTACKITEMSTATIC_N(_CL("CJuikGfxStoreImpl"), _CL("NewL"));
	auto_ptr<CJuikGfxStoreImpl> self( new (ELeave) CJuikGfxStoreImpl );
	self->ConstructL(aFileName);
	return self.release();
}

