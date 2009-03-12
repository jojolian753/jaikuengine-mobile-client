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

#ifndef __JUIK_SIZER_H__
#define __JUIK_SIZER_H__

#include <e32std.h>

class MJuikSizerItem
{
 public:
	virtual TRect Rect() = 0;
	virtual void SetFixedWidthL( TInt aWidth ) = 0;
};

class MJuikSizer
{
 public: 
	virtual ~MJuikSizer() {}

	virtual void AddL( class MJuikControl& aControl, TInt aProportion, TInt aFlags ) = 0;
	virtual void AddL( MJuikSizer& aSizer, TInt aStretch, TInt aFlags ) = 0;
	virtual void InsertL( TInt aPos, class MJuikControl& aControl, TInt aProportion, TInt aFlags ) = 0;
	virtual void InsertL( TInt aPos, MJuikSizer& aSizer, TInt aStretch, TInt aFlags ) = 0;
	virtual void RemoveL( TInt aIndex ) = 0;

	virtual void SetDimensionL(const TPoint& aPos, const TSize& aSize) = 0;
	virtual void SetPositionL(const TPoint& aPos) = 0;
	virtual TSize MinSize() = 0;
	virtual TSize PresetMinSize() const = 0;
	virtual MJuikSizerItem& GetItemL(TInt aIndex) = 0;
	virtual TInt ItemCount() const = 0; 
	virtual void SetMinSize(TSize aSize) = 0;
	virtual TRect Rect() const = 0;
};

class MJuikScrollableSizer : public MJuikSizer
{
 public:
	virtual ~MJuikScrollableSizer() {}
	virtual void SetScrollPositionL(TInt aY) = 0;
	virtual TInt ScrollPosition() = 0;

	virtual void LayoutTopChildL() = 0;
	virtual void LayoutBottomChildL() = 0;
	virtual void LayoutChildL(TInt aIx) = 0;

};

namespace Juik
{
	enum TBoxOrientation
		{
			EHorizontal,
			EVertical			
		};

	enum TAlignment
		{
			EAlignNot    = 0x0000,
			EAlignLeft   = EAlignNot,
			EAlignTop    = EAlignNot,			
			EAlignRight  = 0x0100,
			EAlignBottom = 0x0200,
			EAlignCenterHorizontal = 0x0400,
			EAlignCenterVertical   = 0x0800
		};
	
	enum TStretch
		{			
			EExpandNot = 0x0000,
			EExpand    = 0x1000
		};

	IMPORT_C MJuikSizer* CreateBoxSizerL(Juik::TBoxOrientation aOrient);
	IMPORT_C MJuikScrollableSizer* CreateFixedWidthSizerL();

}



#endif // __JUIK_SIZER_H__
