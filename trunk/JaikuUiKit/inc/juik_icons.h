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

#ifndef JUIK_ICONS_H
#define JUIK_ICONS_H

#include "icons.h" // For TIconId definition

#include <e32std.h>
#include <akniconutils.h>

class CGulIcon;

struct TIconID2 
{
/* TIconID2() : iBitmap( KErrNotFound ), iMask( KErrNotFound ) {} */
/* TIconID2(TInt aBmp, TInt aMask) : iBitmap( aBmp ), iMask( aMask  ) {} */
	TInt iBitmap;
	TInt iMask;
};

class JuikIcons {
 public:
	/**
	 * Constructs MAknIconFileProvider object and loads icons through it. Can be used to load icons
	 * in private directories. 
	 *
	 * Note: Loading icons via file provider seems to be slower than via file name. 
	 * 
	 * @returns MAknIconFileProvider object, which has to stay alive as long as icons are used.
	 */ 
	IMPORT_C static class MAknIconFileProvider* LoadIconsViaFileProviderL(CArrayPtrFlat<CGulIcon> * aIconList, 
																		  const TDesC& aIconFile, 
																		  const TIconID2* aIconDefs, 
																		  TInt aNbIcons);

	/**
	 * Load icons from single file. 
	 */ 
	IMPORT_C static void LoadIconsViaFileNameL(CArrayPtrFlat<CGulIcon> * aIconList, 
												const TDesC& aIconFile, 
												const TIconID2* aIconDefs, 
												TInt aNbIcons);
	
	/**
	 * Load icons from multiple files. TIconId for each icon holds both  file name and icon index.
	 */
	IMPORT_C static void LoadIconsL(CArrayPtrFlat<CGulIcon> * aIconList, const TIconID* aIconDefs, TInt aNbIcons);

	/**
	 * Load single icon
	 */
	IMPORT_C static CGulIcon* LoadSingleIconL(const TIconID& aIconId);

	/**
	 * Load single icon
	 */
	IMPORT_C static CGulIcon* LoadSingleIconL(const TDesC& aFileName, TIconID2 aId);

	/**
	 * Load single icon
	 */
	IMPORT_C static CFbsBitmap* LoadBitmapL(const TDesC& aFileName, TInt aId);
	
	
	/**
	 * Wrapper to icon sizing. Doesn't do anything on 2nd ed, resizes icon on 3rd ed. 
	 */
	IMPORT_C static void SetIconSizeL( CGulIcon& aIcon, const TSize& aSize, TScaleMode aMode = EAspectRatioPreservedAndUnusedSpaceRemoved );

	IMPORT_C static TInt GetIconIndex(TInt identifier, const TIconID2* aIconDefs, TInt aNbIcons);
	
	IMPORT_C static CFbsBitmap* CreateFilledRectL(TSize aSize, TRgb aRgb);

	IMPORT_C static CGulIcon* CreateColoredIconL(const TDesC& aFileName, TInt aIconId, TSize aSize, 
												 TRgb aRgb, TScaleMode aMode);
	
};

#endif
