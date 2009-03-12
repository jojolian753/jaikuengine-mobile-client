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

#ifndef CMYPICTURE_H
#define CMYPICTURE_H

// INCLUDES
#include <gdi.h>

// FORWARD DECLARATIONS
class TSize;
class CFbsBitmap;

// CLASS DECLARATION
/**
*  CMyPicture the class which draw the image.
*/
class CMyPicture :public CPicture
    {
    public:  // Constructors and NO destructor (bitmap not owned)
        
        /**
        * C++ default constructor.
		* @param aSize Size of the picture in twips.
		* @param aBitmap Bitmap
        */
        CMyPicture( TSize aSize, CFbsBitmap& aBitmap, CFbsBitmap* aMask );

	public: // From CPicture

		/**
		* Prohibit linebreaks.
		*/
		TBool LineBreakPossible( TUint aClass,
								 TBool aBeforePicture,
								 TBool aHaveSpaces ) const;
		/**
        * Draw the picture
        */
		 void Draw( CGraphicsContext& aGc,
						   const TPoint& aTopLeft,
						   const TRect& aClipRect,
						   MGraphicsDeviceMap* aMap ) const;
		/**
        * There's no need for it in this , but must be implemented.
        */
		 void ExternalizeL( RWriteStream& aStream ) const;
		
        /**
        * Sets the picture's size in twips.
		* @param aSize Size.
        */
		 void SetOriginalSizeInTwips( TSize aSize );

        /**
        * Returns the picture's size in twips.
		* @param aSize Size.
        */
		 void GetOriginalSizeInTwips( TSize& aSize ) const;

	protected:	// Data

		TSize iSizeInTwips; // Size of the bitmap data
		CFbsBitmap* iBitmap; // reference to the Bitmap data
		CFbsBitmap* iMask; // reference to the Bitmap daat
    };

#endif
            
