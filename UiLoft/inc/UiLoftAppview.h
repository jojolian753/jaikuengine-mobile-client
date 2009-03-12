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

#ifndef __UILOFT_APPVIEW_H__
#define __UILOFT_APPVIEW_H__


#include <coecntrl.h>

/*! 
  @class CUiLoftAppView
  
  @discussion An instance of the Application View object for the UiLoft 
  example application
  */
class CUiLoftAppView : public CCoeControl
    {
public:

/*!
  @function NewL
   
  @discussion Create a CUiLoftAppView object, which will draw itself to aRect
  @param aRect the rectangle this view will be drawn to
  @result a pointer to the created instance of CUiLoftAppView
  */
    static CUiLoftAppView* NewL(const TRect& aRect);

/*!
  @function NewLC
   
  @discussion Create a CUiLoftAppView object, which will draw itself to aRect
  @param aRect the rectangle this view will be drawn to
  @result a pointer to the created instance of CUiLoftAppView
  */
    static CUiLoftAppView* NewLC(const TRect& aRect);


/*!
  @function ~CUiLoftAppView
  
  @discussion Destroy the object and release all memory objects
  */
     ~CUiLoftAppView();


public:  // from CCoeControl
/*!
  @function Draw
  
  @discussion Draw this CUiLoftAppView to the screen
  @param aRect the rectangle of this view that needs updating
  */
    void Draw(const TRect& aRect) const;
    virtual CCoeControl* ComponentControl(TInt aIndex) const;
    virtual TInt CountComponentControls() const;
	private:
	void SetSelectionL(TInt aCursorPos,TInt aAnchorPos);
	void SetColorL(TRgb aColor);
	void SetTypefaceL(const TDesC& aTypeface);
	
	void InsertMyPictureL(TInt aPos);


private:

	class CEikRichTextEditor *iEditor;
	class CAknsBasicBackgroundControlContext* iBackground;
	CArrayPtrFlat<class CGulIcon>* iIcons;

/*!
  @function ConstructL
  
  @discussion  Perform the second phase construction of a CUiLoftAppView object
  @param aRect the rectangle this view will be drawn to
  */
    void ConstructL(const TRect& aRect);
    virtual void SizeChanged();
    void HandleResourceChange( TInt aType );

/*!
  @function CUiLoftAppView
  
  @discussion Perform the first phase of two phase construction 
  */
    CUiLoftAppView();
    };


#endif // __UILOFT_APPVIEW_H__
