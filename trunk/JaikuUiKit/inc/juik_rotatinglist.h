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

#ifndef CCU_ROTATINGLIST_H
#define CCU_ROTATINGLIST_H

#include <coecntrl.h> 

#include "app_context.h"
#include "juik_scrolllist.h"


class CRotatingList : public CCoeControl
{
 public:
	class MControlModel 
	{
	public:
		//virtual CCoeControl* CreateFreeControlL(CCoeControl* aParent) = 0;
		
		virtual void InitControlModelL(CCoeControl* aParent) = 0;
		
		/**
		 * Get control that represents aModelIndex list item. Control ownership is 
		 * transferred to rotating list, but when it doesn't need it anymore, it passes
		 * ownership back with FreeControlL call. 
		 */
		virtual MJuikControl* GetControlL( TInt aModelIndex, CCoeControl* aParent) = 0;
		

		/**
		 */
		virtual void UpdateControlL( TInt aModelIndex, MJuikControl* aControl ) = 0;
		
		
		/**
		 * This control is not needed by rotating list and can be reused or deleted. 
		 */ 
		virtual void FreeControlL( MJuikControl* aControl ) = 0;
		
		/**
		 * Count of items in list model
		 */
		virtual TInt Count() const = 0;
	};

 public:
	IMPORT_C static CRotatingList* NewL( CCoeControl* aParent, MControlModel& aModel );
	virtual ~CRotatingList() {}

	virtual TInt CurrentItemIndex() const = 0;
	virtual void SetFocusAndInitL(TInt aItemIndex)  = 0;
	virtual void SetFocusL(TInt aNewFocus) = 0;

	virtual void HandleItemUpdatedL(TInt aIndex) = 0;
	virtual void HandleItemAddedL(TInt aIndex) = 0;
	virtual void HandleItemRemovedL(TInt aIndex) = 0;
	virtual void UpdateAndRedrawItemL(TInt aIndex) = 0;

	virtual MJuikControl* GetUsedControlL(TInt aIndex) = 0;

	virtual void GetFullyVisibleItemsL(TInt& aFirst, TInt& aCount) = 0;

	virtual void AddObserverL( CSoftScrollList::MObserver& aObserver ) = 0;
	virtual void RemoveObserver( CSoftScrollList::MObserver& aObserver ) = 0;


};

#endif // CCU_ROTATINGLIST_H
