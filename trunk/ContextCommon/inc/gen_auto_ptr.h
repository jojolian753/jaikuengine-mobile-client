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

#ifndef __GEN_AUTO_PTR_H__
#define __GEN_AUTO_PTR_H__  


#ifndef __E32BASE_H__
#include <e32base.h>
#endif


template<class Y> struct gen_auto_ptr_ref
{
  Y* iPtr;
  gen_auto_ptr_ref(Y* aPtr) : iPtr(aPtr) {}
};


template<class X> class gen_auto_ptr
{
public:  
  typedef X element_type;

  gen_auto_ptr(X* aPtr = 0): iPtr(aPtr)
  {
    CleanupStack::PushL(TCleanupItem(Close, (void*)this));
  }  
  
  gen_auto_ptr(gen_auto_ptr& aPtr): iPtr(aPtr.release())
  {
    CleanupStack::PushL(TCleanupItem(Close, (void*)this));
  }

  // MS VC++ 6.0 thinks this is identical to the one above
  // GCC 2.7.2 (for ER5) hates this declaration
  // GCC 2.9-psion-98r2 (V6 Nokia 9200 Series) think this is ok
  //template <class Y> gen_auto_ptr(gen_auto_ptr<Y>& aPtr): iPtr(aPtr.release())
  //{
  //  CleanupStack::PushL(TCleanupItem(Close, (void*)this));
  //}
  
  gen_auto_ptr<X>& operator=(gen_auto_ptr<X>& aRhs)
  {
    if (&aRhs != this)
    {
      delete iPtr;
      iPtr = aRhs.release();
    }
    return (*this); 
  }

  // MS VC++ 6.0 thinks this is identical to the one above
  // GCC 2.7.2 (for ER5) hates this declaration
  // GCC 2.9-psion-98r2 (V6 Nokia 9200 Series) think this is ok
  //template <class Y> gen_auto_ptr& operator=(gen_auto_ptr<Y>& aRhs)
  //{
  //  if (aRhs.get() != this->get()) {
  //    delete iPtr;
  //    iPtr = aRhs.release();
  //  }
  //  return *this;
  //}

  ~gen_auto_ptr() 
  { 
    CleanupStack::Pop();
    delete iPtr;
  }

  X& operator *() const { return *iPtr; }
  X* operator ->() const { return iPtr; }
   
  X* get() const { return iPtr; }

  X* release()
 	{ 
    X* result = iPtr;
    iPtr = 0;
		return result; 
  }

  void reset(X* aPtr = 0) {
    if (aPtr != iPtr) {
      delete iPtr;
      iPtr = aPtr;
    }
  }


  gen_auto_ptr(gen_auto_ptr_ref<X> aRef): iPtr(aRef.iPtr)
  {
    CleanupStack::PushL(TCleanupItem(Close, (void*) this));
  }

  //gen_auto_ptr& operator=(gen_auto_ptr_ref<X> aRef)
  //{
  //  if (aRef.iPtr != this->get()) {
  //    delete iPtr;
  //    iPtr = aRef.iPtr;
  //  }
  //  return *this;
  //}

  // MS VC++ 6.0 thinks this is ok
  // GCC 2.7.2 (for ER5) hates this declaration
  // GCC 2.9-psion-98r2 (V6 Nokia 9200 Series) thinks this is ok
  template <class Y> operator gen_auto_ptr_ref<Y>() 
    { return gen_auto_ptr_ref<Y>(this->release()); }
  
  // MS VC++ 6.0 thinks this is ok
  // GCC 2.7.2 (for ER5) hates this declaration
  // GCC 2.9-psion-98r2 (V6 Nokia 9200 Series) think this is ok
  //template <class Y> operator gen_auto_ptr<Y>()
  //  { return gen_auto_ptr<Y>(this->release()); }

private:
  static void Close(void* aPtr)
  { 
    gen_auto_ptr<X>* self = (gen_auto_ptr<X>*)aPtr;
    delete self->iPtr;
  }

private:
  X* iPtr;  
};

#endif // __auto_set_to_zero_H__
