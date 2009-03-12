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

#include "juik_multilabel.h"

#include "juik_fonts.h"
#include "juik_label.h"
#include "juik_sizercontainer.h"

#include "juik_debug.h"

#include "raii_array.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"

#include <aknbiditextutils.h>

class CMultiLabelImpl : public CMultiLabel, public MContextBase
{
public:
	static CMultiLabelImpl* NewL(MJuikControl* aTopLeftControl,
								 TRgb aColor, TRgb aFocusColor)
	{
		CALLSTACKITEMSTATIC_N(_CL("CMultiLabelImpl"), _CL("NewL"));
		auto_ptr<CMultiLabelImpl> self( new (ELeave) CMultiLabelImpl() );
		self->ConstructL(aTopLeftControl, aColor, aFocusColor);

		return self.release();
	}
	
	CMultiLabelImpl() 
	{
#ifdef JUIK_DEBUGGING_ENABLED	
	iDebugId = KErrNotFound;
#endif
	}
	
	CJuikSizerContainer& Content() const
	{
		return *iContent;
	}

	enum 
		{
			EFirstRowSizer = 0,
			ETopLeftControl,
			EFirstLabel,
			ESecondLabel,
		};


	void ConstructL(MJuikControl* aTopLeftControl, TRgb aColor, TRgb aFocusColor)
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("ConstructL"));				
		auto_ptr<MJuikControl> tlControl(aTopLeftControl);

		iContent = CJuikSizerContainer::NewL();
		iContent->SetContainerWindowL( *this );
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			Content().SetRootSizerL( sizer );
		}
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
			Content().AddSizerL( sizer, EFirstRowSizer );
			Content().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
		}

		if ( tlControl.get() )
			{
				tlControl->CoeControl()->SetContainerWindowL( Content() );
				Content().AddControlL( tlControl.get(), ETopLeftControl );
				iTopLeftControl = tlControl.release();
				Content().GetSizerL( EFirstRowSizer )->AddL( *iTopLeftControl, 0, Juik::EExpandNot | Juik::EAlignCenterVertical ); 
			}
		
		const CFont* font = JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall );
		
		{
			auto_ptr<CJuikLabel> label( CJuikLabel::NewL(  aColor, aFocusColor, font, Content() ));
			iLabels.AppendL( label.get() );
			Content().AddControlL( label.get(), EFirstLabel );
			Content().GetSizerL( EFirstRowSizer )->AddL( *(label.release()), 0, Juik::EExpandNot ); 
		}
		
		{
			auto_ptr<CJuikLabel> label( CJuikLabel::NewL(  aColor, aFocusColor, font, Content() ));
			iLabels.AppendL( label.get() );
			Content().AddControlL( label.get(), ESecondLabel );
			Content().GetRootSizerL()->AddL( *(label.release()), 0, Juik::EExpandNot ); 
		}
	}

	void EnableFocusBackgroundL( const TRgb& aColor )
	{
		for (TInt i=0; i < iLabels.Count(); i++)
			{
				iLabels[i]->EnableFocusBackgroundL( aColor );
			}
	}

	
	void UpdateTextL(const TDesC& aText)
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("UpdateTextL"));		
		
		if ( iUnwrappedText && (aText.Compare(*iUnwrappedText) == 0) )
			return;


		for (TInt i=0; i < iLabels.Count(); i++)
			{
				iLabels[i]->ZeroL();
			}

		delete iUnwrappedText;
		iUnwrappedText = NULL;
		iUnwrappedText = aText.AllocL();
		
		TInt workLength = aText.Length() + 100;

		if ( ! iWorkBuf )
			{
				iWorkBuf = HBufC::NewL( workLength );
			}

		if ( iWorkBuf->Des().MaxLength() < workLength )
			{
				delete iWorkBuf;
				iWorkBuf = NULL;
				iWorkBuf = HBufC::NewL( workLength );
			}
		else
			{
				iWorkBuf->Des().Zero();
			}
		WrapTextL();
	}


	void SetColorsL(TRgb aNormal, TRgb aHighlight)
	{
		for (TInt i=0; i < iLabels.Count(); i++)
			{
				iLabels[i]->SetColorsL( aNormal, aHighlight );
			}
	}

	struct TLabelDef
	{
		TInt iWidth;
		TInt iMaxLines;
		class CJuikLabel* iLabel;
	};

	void WrapTextL()
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("WrapTextL"));		
		// 		if ( ! aText.Length() )
		// 			return;
		if ( ! iUnwrappedText ) 
			return;

		TInt W = Rect().Width();
		if ( W == 0)
			{
				iLabels[0]->SetTextL( *iUnwrappedText );
				iLabels[1]->ZeroL();
				return;
			}
		
		
		TInt imageW = iTopLeftControl ? iTopLeftControl->CoeControl()->MinimumSize().iWidth : 0;
		TInt w1 = Max( W - imageW, 0 );
		TInt w2 = Max( W, 0 );
		
		RAArray<TLabelDef> labelDefs; // FIXME: change to RAArray
		TLabelDef l1 = { w1, 2, iLabels[0] };
		TLabelDef l2 = { w2, 20, iLabels[1] };
		labelDefs.AppendL( l1 );
		labelDefs.AppendL( l2 );
	
		auto_ptr< CArrayFix<TInt> > widths( new (ELeave) CArrayFixFlat<TInt>(20) );
		for (TInt i = 0; i < labelDefs.Count(); i++)
			{
				widths->AppendL( labelDefs[i].iWidth, labelDefs[i].iMaxLines );
			}
		
		auto_ptr< CArrayFix<TPtrC> > wrappedArray( new (ELeave) CArrayFixFlat<TPtrC>(10) );
		
		const CFont* font = labelDefs[0].iLabel->Font();
		
		auto_ptr<HBufC> wrappedText( AknBidiTextUtils::ConvertToVisualAndWrapToArrayWholeTextL(*iUnwrappedText,
																							   *widths,
																							   *font,
																							   *wrappedArray) );
				
		TInt line = 0;
		for (TInt i=0; i < labelDefs.Count(); i++)
			{
				iWorkBuf->Des().Zero();
				
				if ( line >= wrappedArray->Count() )
					break;
				
				TPtrC currentLine( wrappedArray->At(line) );
				TBool hastextleft = currentLine.Length() != 0;
				TInt internalLine = 0;
				
				while ( internalLine < labelDefs[i].iMaxLines && hastextleft ) 
					{
						iWorkBuf->Des().Append( currentLine );
						line++;
						internalLine++;
						
						if ( line >= wrappedArray->Count() )
							break;
						
						currentLine.Set( wrappedArray->At(line) );
						hastextleft = currentLine.Length() != 0;
						if ( internalLine < labelDefs[i].iMaxLines && hastextleft)
							iWorkBuf->Des().Append( _L("\n") );
					}
				labelDefs[i].iLabel->SetWrappedTextL( *iWorkBuf );
			}
	}

	void FocusChanged(TDrawNow aDrawNow)
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("FocusChanged"));
		TBool focused = IsFocused();
		iContent->SetFocus( focused, aDrawNow );
// 		for (TInt i = 0; i < iLabels.Count(); i++)
// 			{
// 				iLabels[i]->SetUnderlining( focused );
// 			}
	}
	
	TSize MinimumSize()
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("MinimumSize"));
		TSize sz = iContent->MinimumSize();
		return sz;
	}
	
	TInt CountComponentControls() const
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("CountComponentControl"));
		return 1;
	}


	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("ComponentControl"));
		return iContent;
	}
	
		
	void PositionChanged()
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("PositionChanged"));
//  		TPoint p = Position(); 
//  		iContent->SetPosition( p );
	}
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("SizeChanged"));
		TRect r = Rect();
#ifdef JUIK_DEBUGGING_ENABLED
		DebugPrint( _L("CMultiLabelImpl::SizeChanged") );
#endif
 		iContent->SetRect( r );
		WrapTextL();
	}

	~CMultiLabelImpl() 
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("~CMultiLabelImpl"));
		delete iContent;
		delete iUnwrappedText;
		delete iWorkBuf;
		iLabels.Close();
	}
	
	void ZeroL()
	{
		CALLSTACKITEM_N(_CL("CMultiLabelImpl"), _CL("Zero"));
		for(TInt i=0; i < iLabels.Count(); i++)
			{
				iLabels[i]->ZeroL();
			}
		//if ( iTopLeftControl ) iTopLeftControl->ClearL();
		iContent->SetSize( TSize(0,0) );
	}



	void SetDebugId(TInt aDebugId)
	{
#ifdef JUIK_DEBUGGING_ENABLED
		iDebugId = aDebugId;
		for (TInt i=0; i < iLabels.Count(); i++)
			{
				iLabels[i]->SetDebugId( ((i+1)*1000000) + iDebugId );
			}
#endif
	}

#ifdef JUIK_DEBUGGING_ENABLED
	void DebugPrint(const TDesC& aMsg) const
	{
		if ( iDebugId != KErrNotFound )
			{
				TRect r = Rect();
				TBuf<100> buf;
				buf.Format( _L("MultiLabel::SizeChanged: %d (%d,%d),(%d,%d) [%d,%d]"), iDebugId, r.iTl.iX, r.iTl.iY, r.iBr.iX, r.iBr.iY, r.Width(), r.Height() );
				const_cast<CMultiLabelImpl*>(this)->Reporting().DebugLog( buf );
			}
	}

	TInt iDebugId;
#endif
	
private: // from MJuikControl
	const CCoeControl* CoeControl() const { return this; } 
	CCoeControl* CoeControl() { return this; }

	CJuikSizerContainer* iContent; // own 
	HBufC* iUnwrappedText; // own 
	HBufC* iWorkBuf; // own 
	RPointerArray<CJuikLabel> iLabels; // own, but labels are not owned
	MJuikControl* iTopLeftControl; 
};


EXPORT_C CMultiLabel* CMultiLabel::NewL( MJuikControl* aTopLeftControl, TRgb aColor, TRgb aFocusColor)
{
	return CMultiLabelImpl::NewL( aTopLeftControl, aColor, aFocusColor );
}
