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

#ifndef __JUIK_ICONMANAGER_H__
#define __JUIK_ICONMANAGER_H__

/** 
 * CJuikIconManager is intended for situations, where icons 
 * are supplied by several entities (static icons, dummy icons, presence icons)  
 * and their amount varies dynamically (e.g. buddy icons) 
 * and same icons can be used in different situations
 *
 * CJuikIconManager takes care of representing several dynamic icon
 * sets as a one icon array to Avkon listbox and hiding common indexing from
 * different icon suppliers. 
 *
 * Usage:
 *  void ConstructL( ) 
 *  {
 *  TInt iVendorId = IconManager().GetIconProviderId( )
 *  IconManager().SetIconsL( iVendorId, icons );
 *  } 
 *  
 *  void NewIcon() 
 *  {
 *    IconManager().AddIconL(iVendorId, icon);
 *  }
 * 
 *  void UpdateIcon()
 *  {
 *	  IconManager().ReplaceIconL(iVendorId, iconId, icon);	
 *  }
 * 
 *  TPtrC MdcaPoint()
 *  { 
 *    IconManager().GetListBoxIconIndexL(iVendorId, iconId);
 *  }
 */ 

#include <e32base.h>
#include "juik_icons.h"
class CGulIcon;


/*
 * MStaticIconProvider
 */
class MStaticIconProvider
{
public: 
	virtual TInt GetListBoxIndexL(TInt aIconId) = 0;
	virtual CGulIcon* GetIconL(TInt aIconId) = 0;
};


/*
 * M interface should be offered to icon providers
 */
class MJuikIconManager 
{
 public:
	virtual TInt GetNewProviderIdL() = 0;

	virtual void LoadStaticIconsL(const TDesC& aFileName, const TIconID2* aIconDefs, TInt aNbIcons) = 0;
	
	virtual MStaticIconProvider* GetStaticIconProviderL(const TDesC& aIconFile) = 0;
	
	virtual void LoadIconsL(TInt aProviderId, const TIconID* aIconDefs, TInt aNbIcons) = 0;					
	virtual	void SetIconsL(TInt aProviderId, CArrayPtr<CGulIcon>& aIcons) = 0;
	virtual CArrayPtr<CGulIcon>* GetProviderIconsL(TInt aProviderId) = 0;
	//	virtual void CArrayPtr<CGulIcon>* GetProviderIconsL(TInt aProviderId) = 0;

	virtual TInt AddIconL(TInt aProviderId, CGulIcon* aIcon) = 0;
	virtual void ReplaceIconL(TInt aProviderId, TInt aIconId, CGulIcon* aIcon) = 0;
	virtual void DeleteIconL(TInt aProviderId, TInt aIconId) = 0;
	virtual TInt GetListBoxIndexL(TInt aProviderId, TInt aIconId) = 0;
	virtual CArrayPtr<CGulIcon>* GetIconArray() const = 0;

	virtual void RemoveProviderIconsL(TInt aProvider) = 0;
};

/**
 *
 */ 
class CJuikIconManager : public CBase, public MJuikIconManager 
{
 public:
	IMPORT_C static CJuikIconManager* NewL();
	virtual ~CJuikIconManager() {};		
};

#endif
