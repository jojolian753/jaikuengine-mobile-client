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

#ifndef __JAIKUTOOL_DOCUMENT_H__
#define __JAIKUTOOL_DOCUMENT_H__


#include <akndoc.h>

// Forward references
class CJaikuToolAppUi;
class CEikApplication;


/*! 
  @class CJaikuToolDocument
  
  @discussion An instance of class CJaikuToolDocument is the Document part of the AVKON
  application framework for the JaikuTool example application
  */
class CJaikuToolDocument : public CAknDocument
    {
public:

/*!
  @function NewL
  
  @discussion Construct a CJaikuToolDocument for the AVKON application aApp 
  using two phase construction, and return a pointer to the created object
  @param aApp application creating this document
  @result a pointer to the created instance of CJaikuToolDocument
  */
    static CJaikuToolDocument* NewL(CEikApplication& aApp);

/*!
  @function NewLC
  
  @discussion Construct a CJaikuToolDocument for the AVKON application aApp 
  using two phase construction, and return a pointer to the created object
  @param aApp application creating this document
  @result a pointer to the created instance of CJaikuToolDocument
  */
    static CJaikuToolDocument* NewLC(CEikApplication& aApp);

/*!
  @function ~CJaikuToolDocument
  
  @discussion Destroy the object and release all memory objects
  */
    ~CJaikuToolDocument();

public: // from CAknDocument
/*!
  @function CreateAppUiL 
  
  @discussion Create a CJaikuToolAppUi object and return a pointer to it
  @result a pointer to the created instance of the AppUi created
  */
    CEikAppUi* CreateAppUiL();

private:

/*!
  @function ConstructL
  
  @discussion Perform the second phase construction of a CJaikuToolDocument object
  */
    void ConstructL();

/*!
  @function CJaikuToolDocument
  
  @discussion Perform the first phase of two phase construction 
  @param aApp application creating this document
  */
    CJaikuToolDocument(CEikApplication& aApp);

    };


#endif // __JAIKUTOOL_DOCUMENT_H__
