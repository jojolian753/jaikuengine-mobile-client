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

#ifndef JUIK_CONTROL_H
#define JUIK_CONTROL_H

#include <coecntrl.h>

/**
 * MJuikControl extends CCoeControl interface with methods
 * that are needed in JaikuUiKit.
 *
 * Note that MJuikControl doesn't inherit from CCoeControl, but
 * it's assumed that control implementing MJuikControl is actually
 * deriving from CCoeControl. 
 * This requirement is defined via CoeControl method, which is used to 
 * get CCoeControl, which implementation should usually return "this". 
 *
 * This arrangement allows us to use existing CCoeControl derived
 * classes like CEikLabel and CEikImage as base classes, but extend
 * them with JaikuUiKit specific CCoeControl-related functionality.
 *
 * (In principle, you can return different object than this and everything works
 * fine, given that your CCoeControl-derived and MJuikControl-derived objects
 * coordinate correctly.) 
 * 
 * There is special auto_ptr implementation for MJuikControl, as it is not 
 * derived from CBase. 
 */
class MJuikControl
{
 public:
        virtual ~MJuikControl() {}

        virtual const CCoeControl* CoeControl() const = 0;
        virtual CCoeControl* CoeControl() = 0; // const_cast<CCoeControl*>(this->CoeControl()); }
		//        virtual void SetFixedWidthL(TInt /*aWidth*/) {};

		enum TFocusFrom
		{
			EAbove = 0,
			EBelow 			
		};

		virtual TBool PreFocusGained(TFocusFrom /*aGainingFocusFrom*/) { return ETrue; }
		
};


/**
 * auto_ptr for MJuikControl.
 * Copied from HBufC16 auto_ptr implementation in ContextCommon/inc/symbian_auto_ptr.h
 */

#include "symbian_auto_ptr.h"

IMPORT_C void CloseMJuikControlIndirect(TAny* aPtr);

template<> class auto_ptr<MJuikControl>
{
public:

  auto_ptr(MJuikControl* aPtr = 0): iPtr(aPtr)
  {
#ifndef __LEAVE_EQUALS_THROW__
    CleanupStack::PushL(TCleanupItem(CloseMJuikControlIndirect, (void*)&iPtr));
#endif
  }

  auto_ptr(auto_ptr& aPtr): iPtr(aPtr.release())
  {
#ifndef __LEAVE_EQUALS_THROW__
    CleanupStack::PushL(TCleanupItem(CloseMJuikControlIndirect, (void*)&iPtr));
#endif
  }

  auto_ptr<MJuikControl>& operator=(auto_ptr<MJuikControl>& aRhs)
  {
    if (&aRhs != this)
    {
      delete iPtr;
      iPtr = iPtr = aRhs.release();
    }
    return (*this);
  }

  ~auto_ptr()
  {
#ifndef __LEAVE_EQUALS_THROW__
    CleanupStack::Pop();
#endif
    delete iPtr;
  }

  const MJuikControl& operator *() const { return *iPtr; }
  MJuikControl* operator ->() const { return iPtr; }

  MJuikControl* get() const { return iPtr; }

  MJuikControl* release()
        {
    MJuikControl* result = iPtr;
    iPtr = 0;
        return result;
  }

  void reset(MJuikControl* aPtr = 0) {
    if (aPtr != iPtr) {
      delete iPtr;
      iPtr = aPtr;
    }
  }


 auto_ptr(auto_ptr_ref<MJuikControl> aRef): iPtr(aRef.iPtr)
  {
#ifndef __LEAVE_EQUALS_THROW__
    CleanupStack::PushL(TCleanupItem(CloseMJuikControlIndirect, (void*)&iPtr));
#endif
  }


  template <class Y> operator auto_ptr_ref<Y>()
    { return auto_ptr_ref<Y>(this->release()); }

private:
  MJuikControl* iPtr;
};


#endif
