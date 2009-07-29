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

#include "juik_image.h"

#include "juik_debug.h"

#include "symbian_auto_ptr.h"

#include <akniconutils.h>
#include <aknsdrawutils.h>
#include <aknsutils.h>

EXPORT_C CJuikImage* CJuikImage::NewL(CGulIcon* aIcon, TSize aDefaultSize)
{
	CALLSTACKITEMSTATIC_N(_CL("CJuikImage"), _CL("NewL"));
	auto_ptr<CJuikImage> self( new (ELeave) CJuikImage(aIcon, aDefaultSize));
	self->ConstructL();
	return self.release();
}


EXPORT_C CJuikImage::~CJuikImage() {
}

	
CJuikImage::CJuikImage( CGulIcon* aIcon, TSize aDefaultSize ) : CEikImage(), iIcon(aIcon), iDefaultSize(aDefaultSize) {}


void CJuikImage::ConstructL()
{
	CALLSTACKITEM_N(_CL("CJuikImage"), _CL("ConstructL"));
	SetBrushStyle( CGraphicsContext::ENullBrush );
	SetPictureOwnedExternally(ETrue); 
	if ( iIcon )
		{
			SetPicture( iIcon->Bitmap(), iIcon->Mask() );
		}
}


EXPORT_C void CJuikImage::UpdateL( CGulIcon& aIcon)
{
	CALLSTACKITEM_N(_CL("CJuikImage"), _CL("UpdateL"));
	TBool hadIcon = iIcon != NULL;

	iIcon = &aIcon;
	TSize sz = iDefaultSize;
	if ( Rect().Size() != TSize(0,0) )
		sz = iMargin.InnerRect( Rect() ).Size();

	AknIconUtils::SetSize(iIcon->Bitmap(), sz);
	if ( hadIcon )
		SetNewBitmaps(iIcon->Bitmap(), iIcon->Mask());
	else 
		SetPicture(iIcon->Bitmap(), iIcon->Mask());
	SizeChanged();
}
	
EXPORT_C void CJuikImage::ClearL()
{
	CALLSTACKITEM_N(_CL("CJuikImage"), _CL("ClearL"));
	iIcon = NULL;
	SetNewBitmaps(NULL, NULL);
	SetSize( TSize(0,0) );
}

   

void CJuikImage::SetDefaultSize(TSize aSize)
{
	CALLSTACKITEM_N(_CL("CJuikImage"), _CL("SetDefaultSize"));
	iDefaultSize = aSize;
}


TSize CJuikImage::MinimumSize()
{
	CALLSTACKITEM_N_NOSTATS(_CL("CJuikImage"), _CL("MinimumSize"));
	TSize imageSize = TSize(0,0);
	if ( iIcon && iIcon->Bitmap() )
		{
			imageSize = iIcon->Bitmap()->SizeInPixels();
			if ( imageSize == TSize(0,0) )
				imageSize = iDefaultSize;
		}
	if ( imageSize == TSize(0,0) )
		return TSize(0,0);
	else
		return imageSize + iMargin.SizeDelta();
}


void CJuikImage::SizeChanged()
{
	CALLSTACKITEM_N_NOSTATS(_CL("CJuikImage"), _CL("SizeChanged"));
	TRect r = Rect();
	if ( iIcon )
		{
			TRect inner = iMargin.InnerRect(r);
			AknIconUtils::SetSize(iIcon->Bitmap(), inner.Size());
			SetNewBitmaps(iIcon->Bitmap(), iIcon->Mask());
		}
	CEikImage::SizeChanged();
}

void CJuikImage::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CJuikImage"), _CL("Draw"));
	CWindowGc& gc = SystemGc();

	
	CFbsBitmap* bmp = iIcon->Bitmap();
	CFbsBitmap* mask = iIcon->Mask();
	if ( ! bmp ) return;
	
	TRect outer = Rect();
	TRect inner = iMargin.InnerRect( outer );
	
	TSize bmpSize = bmp->SizeInPixels();
	TBool clip = inner.Size().iWidth < bmpSize.iWidth || inner.Size().iHeight < bmpSize.iHeight;

	TRect aligned = iAlignment.InnerRect( inner, bmpSize );
	
	TSize sourceSz = clip ? aligned.Size() : bmpSize;
	TRect sourceR( TPoint(0,0), sourceSz );
	if ( mask ) {
                gc.SetBrushStyle(CGraphicsContext::ENullBrush);
		gc.BitBltMasked( aligned.iTl, bmp, sourceR, mask, ETrue);
        } else {
		gc.BitBlt( aligned.iTl, bmp, sourceR);	
        }
	
//  	AknsDrawUtils::BackgroundBetweenRects( AknsUtils::SkinInstance(),
// 										   AknsDrawUtils::ControlContext( this ),
// 										   this,
// 										   gc, outer, aligned );
#ifdef JUIK_BOUNDINGBOXES
	JuikDebug::DrawBoundingBox(gc ,Rect());
#endif

}
