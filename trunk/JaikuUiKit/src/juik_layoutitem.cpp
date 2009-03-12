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

#include "juik_layoutitem.h"
#include "jaiku_layoutdata.h"
#include "juik_fonts.h"

#include <aknutils.h>
#include <aknsutils.h>
#include <eikenv.h>
#include <gdi.h>

const TInt KParentRelative( JuikLayoutData::KParentRelative );
const TInt KParentRelativeLimit( JuikLayoutData::KParentRelativeLimit );

EXPORT_C TJuikLayoutItem::TJuikLayoutItem( const TRect& r ) :
	ltype(ERect), x(r.iTl.iX), y(r.iTl.iY), w(r.Size().iWidth), h(r.Size().iHeight), font(ENoFont) {}
EXPORT_C TJuikLayoutItem::TJuikLayoutItem() : ltype(ERect), x(0), y(0), w(0), h(0), font(ENoFont) {}
EXPORT_C TJuikLayoutItem::TJuikLayoutItem( TLayoutType l, TInt aX, TInt aY, TInt aW, TInt aH, TFontType ft ): 
	ltype(l), x(aX), y(aY), w(aW), h(aH), font(ft) {}
EXPORT_C TJuikLayoutItem::TJuikLayoutItem( TLayoutType l, const TPoint& p, const TSize& s, TFontType ft) : 
	ltype(l), x(p.iX), y(p.iY), w(s.iWidth), h(s.iHeight), font(ft) {}


EXPORT_C TJuikLayoutItem TJuikLayoutItem::Combine(TJuikLayoutItem aC) const
{
	TJuikLayoutItem r = aC;
	if ( aC.x >= KParentRelativeLimit ) { r.x = x + w + (aC.x - KParentRelative); } 
	else { r.x = x + aC.x; }
	
	if ( aC.y >= KParentRelativeLimit ) { r.y = y + h + (aC.y - KParentRelative); }
	else { r.y = y + aC.y; }
	
	if ( aC.w >= KParentRelativeLimit ) { r.w = w + (aC.w - KParentRelative); }
	else { r.w = aC.w; }
	
	if ( aC.h >= KParentRelativeLimit ) { r.h = h + (aC.h - KParentRelative); }
	else { r.h = aC.h; }
	
	return r;
}


EXPORT_C TJuikLayoutItem TJuikLayoutItem::Combine(const TMargins& aM) const
{
	TJuikLayoutItem r = *this;
	r.x = x + aM.iLeft;
	r.y = y + aM.iTop;
	r.w = w - r.x - aM.iRight;
	r.h = h - r.y - aM.iBottom;
	
	return r;
}


EXPORT_C TBool TJuikLayoutItem::IsReady() const
{
	return
		x < KParentRelativeLimit &&
		y < KParentRelativeLimit &&
		w < KParentRelativeLimit &&
		h < KParentRelativeLimit;		
}


EXPORT_C TPoint TJuikLayoutItem::TopLeft() const
{
	if ( ! IsReady() ) { User::Leave( KErrNotReady ); }
	return TPoint( x, y );
}

	
EXPORT_C TPoint TJuikLayoutItem::BottomRight() const
{
	if ( ! IsReady() ) { User::Leave( KErrNotReady ); }
	return TPoint( x + w, y + h );
}

EXPORT_C TRect TJuikLayoutItem::Rect() const
{
	if ( ! IsReady() ) { User::Leave( KErrNotReady ); }
	return TRect( TPoint(x, y), TSize(w, h) );
}

EXPORT_C TSize TJuikLayoutItem::Size() const
{
	return Rect().Size();
}

EXPORT_C TInt TJuikLayoutItem::Baseline() const
{
	if ( ! IsReady() ) { User::Leave( KErrNotReady ); }
	TInt b = (3*h) / 4;	
	return y + b;
}


EXPORT_C const CFont* TJuikLayoutItem::Font() const
{
	switch ( font )
		{
 		case ENormalFont:
			return CEikonEnv::Static()->NormalFont();
		case EDenseFont:
			return CEikonEnv::Static()->DenseFont();
		case ETitleFont:
			return CEikonEnv::Static()->TitleFont();
		case ESoftkeyFont:
			return LatinBold17();
		case ESmallPrimaryFont:
			return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);

		case ELogicFontPrimary:
			return JuikFonts::GetLogicalFont( JuikFonts::EPrimary );
		case ELogicFontSecondary:
			return JuikFonts::GetLogicalFont( JuikFonts::ESecondary );
		case ELogicFontTitle:
			return JuikFonts::GetLogicalFont( JuikFonts::ETitle );
		case ELogicFontPrimarySmall:
			return JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall );
		case ELogicFontDigital:
			return JuikFonts::GetLogicalFont( JuikFonts::EDigital );
		}
	User::Leave( KErrNotSupported );
	return NULL;
}


EXPORT_C TMargins8 TJuikLayoutItem::Margins() const
{
	if ( ltype != TJuikLayoutItem::EMargins )
		User::Leave( KErrGeneral ); 
	TMargins8 m;
	m.iLeft = x;
	m.iRight = y;
	m.iTop = w;
	m.iBottom = h;
	return m;
}


void DoGetTextColor(TRgb& aRgb, TInt aIndex)
{
	AknsUtils::GetCachedColor( AknsUtils::SkinInstance(), aRgb, 
							   KAknsIIDQsnTextColors,
							   aIndex );

}
								   

EXPORT_C void TJuikLayoutItem::GetTextColor(TRgb& aRgb) const
{	
	DoGetTextColor( aRgb, EAknsCIQsnTextColorsCG6 );
}


EXPORT_C void TJuikLayoutItem::GetTextHighlightColor(TRgb& aRgb) const
{
	DoGetTextColor( aRgb, EAknsCIQsnTextColorsCG10 );
}
