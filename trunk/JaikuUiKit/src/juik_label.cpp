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

#include "juik_label.h"

#include "symbian_auto_ptr.h"

#include <aknbiditextutils.h>
#include <gulcolor.h>

#ifdef __S60V3__
#include <aknlayoutfont.h> 
#endif 

#include "reporting.h"

EXPORT_C CJuikLabel* CJuikLabel::NewL(TRgb aColor, TRgb aFocusColor, const CFont* aFont, CCoeControl& aParent)
{
	auto_ptr<CJuikLabel> label( new (ELeave) CJuikLabel() );
	label->SetContainerWindowL( aParent );
	label->SetFontProperlyL( aFont );
	label->OverrideColorL( EColorLabelText, aColor );
	label->SetColorsL( aColor, aFocusColor );
	label->SetAllMarginsTo( 1 );
	return label.release();
}


EXPORT_C CJuikLabel::CJuikLabel() : CEikLabel(), iWrapWidth(0), iAlreadyWrapped(EFalse), iUseBgFocus(EFalse)
{
	CALLSTACKITEM_N(_CL("CJuikLabel"), _CL("CJuikLabel"));
#ifdef JUIK_DEBUGGING_ENABLED	
	iDebugId = KErrNotFound;
#endif
	// we do wrapping ourselves
	UseLogicalToVisualConversion( EFalse );
}


EXPORT_C CJuikLabel::~CJuikLabel()
{
	CALLSTACKITEM_N(_CL("CJuikLabel"), _CL("~CJuikLabel"));
	delete iUnwrappedText;
}

EXPORT_C void CJuikLabel::SetDebugId( TInt aDebugId )
{
#ifdef JUIK_DEBUGGING_ENABLED	
	iDebugId = aDebugId;
#endif
}

EXPORT_C void CJuikLabel::UpdateTextL( const TDesC& aText )
{
		CALLSTACKITEM_N(_CL("CJuikLabel"), _CL("UpdateTextL"));

		if ( iUnwrappedText && (aText.Compare(*iUnwrappedText) == 0))
			return;

		delete iUnwrappedText;
		iUnwrappedText = NULL;
		iUnwrappedText = aText.AllocL();
		WrapTextL( ETrue );
}


EXPORT_C void CJuikLabel::SetColorsL( const TRgb& aNormal, const TRgb& aHighlight )
{
	SetNormalColorL( aNormal );
	SetHighlightColorL( aHighlight );
}


void CJuikLabel::SetHighlightColorL( const TRgb& aColor )
{
	iHighlightColor = aColor;
 	if ( IsFocused() )
		{
			OverrideColorL( EColorLabelText, iHighlightColor );
		}
}


void CJuikLabel::SetNormalColorL( const TRgb& aColor )
{
	iNormalColor = aColor;
 	if ( ! IsFocused() )
		{
			OverrideColorL( EColorLabelText, iNormalColor );
		}
}

EXPORT_C void CJuikLabel::EnableFocusBackgroundL( const TRgb& aColor )
{
	SetBrushStyle( CWindowGc::ENullBrush );
	iUseBgFocus = ETrue;
	iBgFocusColor = aColor;
}


EXPORT_C void CJuikLabel::ZeroL()
{
	CALLSTACKITEM_N(_CL("CJuikLabel"), _CL("ZeroL"));
	delete iUnwrappedText;
	iUnwrappedText = NULL;
	this->SetTextL( KNullDesC );
	SetSize(TSize(0,0));
}


EXPORT_C void CJuikLabel::SetFontProperlyL(const CFont* aFont)
{
	CALLSTACKITEM_N(_CL("CJuikLabel"), _CL("SetFontProperlyL"));
	SetFont( aFont );

#ifdef __S60V3__
	const CAknLayoutFont* layoutFont = CAknLayoutFont::AsCAknLayoutFontOrNull(aFont);
	if ( layoutFont ) 
		{
			iRealFontDescent = layoutFont->MaxDescent();
			TInt extraGap = layoutFont->TextPaneHeight() - aFont->HeightInPixels();
			if ( extraGap > 0 )
				{
					TInt gap = PixelGapBetweenLines() + extraGap;
					SetPixelGapBetweenLines(gap);
				}
		}
#endif
}


EXPORT_C void CJuikLabel::SetWrappedTextL(const TDesC& aText)
{
	CALLSTACKITEM_N_NOSTATS(_CL("CJuikLabel"), _CL("SetWrappedTextL"));
	if ( iUnwrappedText && ( aText.Compare(*iUnwrappedText) == 0) )
		return;

	delete iUnwrappedText;
	iUnwrappedText = NULL;
	iUnwrappedText = aText.AllocL();;
	iAlreadyWrapped = ETrue;
	this->SetTextL( aText );	
}

	
void CJuikLabel::FocusChanged(TDrawNow aDrawNow)
{
	CALLSTACKITEM_N_NOSTATS(_CL("CJuikLabel"), _CL("FocusChanged"));
	if ( IsFocused() )
		{
			OverrideColorL( EColorLabelText, iHighlightColor );
		}
	else
		{
			OverrideColorL( EColorLabelText, iNormalColor);
		}
	CEikLabel::FocusChanged(aDrawNow);
}

void CJuikLabel::Draw(const TRect& aRect) const
{
	CWindowGc& gc = SystemGc();
	if ( iUseBgFocus && IsFocused() )
		{
			TRect r = Rect();
			gc.SetPenStyle( CGraphicsContext::ENullPen );
			gc.SetBrushColor( iBgFocusColor );
			gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
			gc.DrawRect( r );
		}

#ifdef JUIK_BOUNDINGBOXES
	JuikDebug::DrawBoundingBox(gc ,Rect());
#endif
    CEikLabel::Draw( Rect() );
}

void CJuikLabel::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CJuikLabel"), _CL("SizeChanged"));
#ifdef JUIK_DEBUGGING_ENABLED	
	DebugPrintRect(_L("CJuikLabel::SizeChanged 0"), Rect());
#endif

	CEikLabel::SizeChanged();
#ifdef JUIK_DEBUGGING_ENABLED	
	DebugPrintRect(_L("CJuikLabel::SizeChanged 1"), Rect());
#endif
	WrapTextL();
#ifdef JUIK_DEBUGGING_ENABLED	
	DebugPrintRect(_L("CJuikLabel::SizeChanged 2"), Rect());
#endif
}


TSize CJuikLabel::MinimumSize()
{
	CALLSTACKITEM_N_NOSTATS(_CL("CJuikLabel"), _CL("MinimumSize"));
	if ( ! iUnwrappedText )
		{
			return TSize(0,0);
		}
#ifdef JUIK_DEBUGGING_ENABLED	
	DebugPrintRect(_L("CJuikLabel::MinimumSize pre"), Rect());
#endif
	TSize sz = TSize(0,0);
	if ( iText )
		{
			TPtrC ptr = (*iText);
			sz = CEikLabel::CalcMinimumSize( ptr );
		}
	else 
		return sz;
	
	// CEikLabel can't calculate font descent for scalable fonts correctly
	// last rows descent is missing
	sz += TSize(0, iRealFontDescent);

#ifdef JUIK_DEBUGGING_ENABLED	
	DebugPrintSize(_L("CJuikLabel::MinimumSize"), sz);
#endif

#ifdef JUIK_DEBUGGING_ENABLED	
	DebugPrintRect(_L("CJuikLabel::MinimumSize post"), Rect());
#endif
	return sz;
}



void CJuikLabel::WrapTextL(TBool aForced)
{
	CALLSTACKITEM_N(_CL("CJuikLabel"), _CL("WrapTextL"));
	if ( iAlreadyWrapped )
		return;

	if ( ! iUnwrappedText )
			return;
	//{ Bug( _L("Label text not set correctly") ).Raise(); }
	
	if ( Rect().Width() == 0 )
		{
			iWrapWidth = 0;
			this->SetTextL( *iUnwrappedText );
			return;
		}

	TInt wrapWidth = Rect().Width() - iMargin.SizeDelta().iWidth;
	
	if ( aForced || ( iWrapWidth != wrapWidth ) )
		{
			iWrapWidth = wrapWidth; //currentWidth == 0 ? 200 : currentWidth;
			const TInt KMaxLines = 30;
			auto_ptr< CArrayFix<TInt> > widths( new (ELeave) CArrayFixFlat<TInt>(KMaxLines) );
			widths->InsertL(0, iWrapWidth, KMaxLines);
			TInt ignore = widths->Count();
			const CFont* font = this->Font();
				
			auto_ptr<HBufC> wrapped( HBufC::NewL( iUnwrappedText->Length() + widths->Count() * (KAknBidiExtraSpacePerLine+1) ) );
			TPtr wrappedPtr = wrapped->Des();
			AknBidiTextUtils::ConvertToVisualAndWrapToStringL(*iUnwrappedText,
															  *widths,
															  *font,
															  wrappedPtr,
															  EFalse);
			
			this->SetTextL( *wrapped );
		}
}



#ifdef JUIK_DEBUGGING_ENABLED	

void CJuikLabel::DebugPrintRect(const TDesC& aMsg, const TRect& aRect) const
{
	JuikDebug::PrintRect( const_cast<CJuikLabel*>(this) , iDebugId, aMsg, aRect );
}

void CJuikLabel::DebugPrintSize(const TDesC& aMsg, TSize& aSize) const
{
	JuikDebug::PrintRect( const_cast<CJuikLabel*>(this), iDebugId, aMsg, aSize );
}

#else

void CJuikLabel::DebugPrintRect(const TDesC& aMsg, const TRect& aRect) const {}
void CJuikLabel::DebugPrintSize(const TDesC& aMsg, TSize& aSize) const {}

#endif
