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

#ifndef __PYTHON_PTR_H__
#define __PYTHON_PTR_H__  

#include <e32std.h>
#include <e32base.h>
#include <Python.h>
#include <symbian_python_ext_util.h>

void ClosePythonIndirect(TAny* aPtr);

class TInterpreterAutoLock {
public:
	TInterpreterAutoLock();
	~TInterpreterAutoLock();
};

template<class Y> struct python_ptr_ref
{
  Y* iPtr;
  python_ptr_ref(Y* aPtr) : iPtr(aPtr) {}
};


template<class X> class python_ptr
{
public:  
  typedef X element_type;

  python_ptr(X* aPtr = 0): iPtr(aPtr), iBasePtr(aPtr)
  {
    CleanupStack::PushL(TCleanupItem(ClosePythonIndirect, (void*)&iBasePtr));
  }  
  
  python_ptr(python_ptr& aPtr): iPtr(aPtr.release()), iBasePtr(iPtr)
  {
    CleanupStack::PushL(TCleanupItem(ClosePythonIndirect, (void*)&iBasePtr));
  }

  python_ptr<X>& operator=(python_ptr<X>& aRhs)
  {
    if (&aRhs != this)
    {
      if (iBasePtr) Py_XDECREF(iBasePtr);
      iBasePtr = iPtr = aRhs.get();
      Py_XINCREF(iBasePtr);
    }
    return (*this); 
  }

  ~python_ptr() 
  { 
    CleanupStack::Pop();
    Py_XDECREF(iBasePtr);
  }

  X& operator *() const { return *iPtr; }
  X* operator ->() const { return iPtr; }
  bool operator !() const { return (iPtr==0); }
   
  X* get() const { return iPtr; }

  X* release()
 	{ 
    X* result = iPtr;
    iBasePtr = iPtr = 0;
		return result; 
  }

  void reset(X* aPtr = 0) {
    if (aPtr != iPtr) {
      Py_XDECREF(iBasePtr);
      iBasePtr = iPtr = aPtr;
    }
  }


  python_ptr(python_ptr_ref<X> aRef): iPtr(aRef.iPtr), iBasePtr(iPtr)
  {
    CleanupStack::PushL(TCleanupItem(ClosePythonIndirect, (void*)&iBasePtr));
  }


  template <class Y> operator python_ptr_ref<Y>() 
    { return python_ptr_ref<Y>(iPtr); }
  
private:
  X* iPtr;  
  PyObject* iBasePtr;
};

#endif
