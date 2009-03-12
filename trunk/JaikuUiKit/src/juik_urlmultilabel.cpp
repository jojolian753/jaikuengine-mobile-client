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

#include "juik_urlmultilabel.h"

#include "juik_fonts.h"
#include "juik_image.h"
#include "juik_keycodes.h"
#include "juik_label.h"
#include "juik_sizercontainer.h"

#include "juik_debug.h"

#include "raii_array.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"

#include <aknbiditextutils.h>
#include <finditemengine.h>

class CUrlMultiLabelImpl : public CUrlMultiLabel, public MContextBase
{
public:
	static CUrlMultiLabelImpl* NewL(CJuikImage* aImage,
									TRgb aColor, TRgb aFocusColor,
									TBool aParseUrls,
									CCoeControl& aControl)
	{
		CALLSTACKITEMSTATIC_N(_CL("CUrlMultiLabelImpl"), _CL("NewL"));
		auto_ptr<CUrlMultiLabelImpl> self( new (ELeave) CUrlMultiLabelImpl(aParseUrls) );
		self->ConstructL(aImage, aColor, aFocusColor, aControl);

		return self.release();
	}
	
	CUrlMultiLabelImpl(TBool aParseUrls) : iFocusedUrl( KErrNotFound ), iParseUrls( aParseUrls ), iLastWrapWidth(0)
	{
#ifdef JUIK_DEBUGGING_ENABLED	
	iDebugId = KErrNotFound;
#endif
	}
	
	CJuikSizerContainer& Content() const
	{
		return *iContent;
	}


	void EnableFocusBackgroundL( const TRgb& aColor )
	{
		iUseBgFocus = ETrue;
		iBgFocusColor = aColor;
	}


	void ConstructL(CJuikImage* aImage, TRgb aColor, TRgb aFocusColor, CCoeControl& aParent)
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("ConstructL"));				
		iTextColor = aColor;
		iFocusTextColor = aColor;

		auto_ptr<CJuikImage> image(aImage);

		iRunningId = 200;
		
		SetContainerWindowL( aParent );

		iContent = CJuikSizerContainer::NewL();
		iContent->SetContainerWindowL( *this );
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			Content().SetRootSizerL( sizer );
		}
		TInt firstRowSizerId = iRunningId++;
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
			Content().AddSizerL( sizer, firstRowSizerId );
			Content().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
		}		
		
		if ( image.get() )
			{
				image->SetContainerWindowL( Content() );
				Content().AddControlL( image.get(), iRunningId++ );
				iImage = image.release();
				Content().GetSizerL( firstRowSizerId )->AddL( *iImage, 0, Juik::EExpandNot | Juik::EAlignCenterVertical ); 
			}
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );
			Content().AddSizerL( sizer, iRunningId++ );
			iTextSizers.AppendL( sizer );
			Content().GetSizerL( firstRowSizerId )->AddL( *sizer, 0, Juik::EExpand ); 
		}
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );
			Content().AddSizerL( sizer, iRunningId++ );
			iTextSizers.AppendL( sizer );
			Content().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
		}
	}

	void RemoveLabels()
	{
		for (TInt i=0; i < iTextSizers.Count(); i++ )
			{
				MJuikSizer* sizer = iTextSizers[i];
				while ( sizer->ItemCount() > 0 )
					{
						sizer->RemoveL( 0 );
					}
			}
		for (TInt i=0; i < iLabelDefs.Count(); i++) 
			{
				CJuikLabel* label = iLabelDefs[i].iLabel;
				Content().RemoveControlL( label );
				//delete label;
				iLabelDefs[i].iLabel = NULL;
			}
		iLabelDefs.Reset();
	}
	
	void UpdateTextL(const TDesC& aText)
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("UpdateTextL"));		

		if ( iUnwrappedText && (aText.Compare(*iUnwrappedText) == 0))
			return;

		delete iUnwrappedText;
		iUnwrappedText = NULL;
		iUnwrappedText = aText.AllocL();
		
		iFocusedUrl = KErrNotFound;
		if ( iParseUrls )
			FindUrlsL( *iUnwrappedText );

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


	void SetColorsL(TRgb aNormal, TRgb aFocus)
	{
		iTextColor = aNormal;
		iFocusTextColor = aFocus;

		for (TInt i=0; i < iLabelDefs.Count(); i++)
			{
				CJuikLabel* l = iLabelDefs[i].iLabel;
				l->SetColorsL( iTextColor, iFocusTextColor );
			}
	}

	TRgb iNormalColor;
	TRgb iFocusColor; 

	struct TLabelDef
	{
		MJuikSizer* iSizer;
		CJuikLabel* iLabel;
		TBool iIsUrl;
	};
	
	struct TTextPart
	{
		
		TPtrC iText;
		TBool iIsUrl;
	};
	
	void PartitionToUrlsAndTextsL(RAArray<TTextPart>& aParts)
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("PartitionToUrlsAndTextsL"));		
		if ( ! iUnwrappedText )
			return;

 		if ( iFindEngine )
			{
				const CArrayFixFlat<CFindItemEngine::SFoundItem>* urls = iFindEngine->ItemArray();
				if ( urls )
					{
						TInt pos = 0;
						TInt totalLength = iUnwrappedText->Length();
						TInt urlIx = 0;
						TInt s,e;
						TTextPart part;
						s = e = 0;
						while ( pos < totalLength )
							{
								if ( urlIx < urls->Count() )
									{
										CFindItemEngine::SFoundItem url = urls->At( urlIx );
										if ( pos < url.iStartPos )
											{
												s = pos; 
												e = url.iStartPos;
												part.iText.Set(  iUnwrappedText->Mid( s, e - s ) );
												part.iIsUrl = EFalse;
												aParts.AppendL( part );
												pos = e;
											}
										else
											{
												s = pos; 
												e = pos + url.iLength; 
												part.iText.Set(  iUnwrappedText->Mid( s, e - s ) );
												part.iIsUrl = ETrue;
												aParts.AppendL( part );
												pos = e;
												urlIx++;
											}
									}
								else
									{
										s = pos; 
										e = iUnwrappedText->Length();
										part.iText.Set(  iUnwrappedText->Mid( s, e - s) );
										part.iIsUrl = EFalse;
										aParts.AppendL( part );
										pos = e;
									}
							}
					}
			}
	}

	CArrayFix<TInt>* InitialLineWidthsL(TInt W)
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("InitialLineWidthsL"));		
		auto_ptr< CArrayFix<TInt> > widths( new (ELeave) CArrayFixFlat<TInt>(20) );
		{
			TInt imageW = iImage ? 
				iImage->MinimumSize().iWidth :
				0;
			
			TInt w1 = Max( W - imageW, 0 );
			TInt w2 = Max( W, 0 );
			TInt c1 = 2;
			TInt c2 = 20;
			
			
			for (TInt i=0; i < c1; i++) 
				widths->AppendL( w1 );
			for (TInt i=0; i < c2; i++) 
				widths->AppendL( w2 );
		}
		return widths.release();
	}


	TInt GroupLinesL(TInt aCurrentLine, const CArrayFix<TPtrC>& aLines, 
					 CArrayFix<TInt>& aWidths, TDes& aOutput) 
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("GroupLinesL"));		
		
		
		if ( aWidths.Count() == 0 ) Bug( _L("width array not initialized") ).Raise();		
		TInt groupW = aWidths.At(0);

		TBool first = ETrue;
		TBool groupToSameLabel = EFalse;				

		TInt line = aCurrentLine;
		do {
			// Append new line between internal lines 
			if ( first )
				first = EFalse;
			else
				aOutput.Append( _L("\n") );
			
			aOutput.Append( aLines[ line ] );
			
			if ( aWidths.Count() ) aWidths.Delete(0);
			groupToSameLabel = aWidths.Count() ? aWidths.At(0) == groupW : ETrue;
			line += 1; 							
		} while ( line < aLines.Count() && groupToSameLabel );
		return line;
	}

	
	void WrapTextL()
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("WrapTextL"));		

		// 		if ( ! aText.Length() )
		// 			return;
		if ( ! iUnwrappedText ) 
			return;

		TInt W = Rect().Width();
		if ( W == 0)
			{
				return;
			}
		if ( W == iLastWrapWidth )
			{
				return;
			}
		iLastWrapWidth = W;
	
		RemoveLabels();
		iLabelDefs.Reset(); 

		RAArray<TTextPart> textParts;
		PartitionToUrlsAndTextsL( textParts );


		// linewidths
		auto_ptr< CArrayFix<TInt> > widths( InitialLineWidthsL(W) );
		const TInt firstLineW = widths->At( 0 );
		const TInt lastLineW = widths->At( widths->Count() - 1 );

		
 		const CFont* font = JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall );
		
		for (TInt partIx=0; partIx < textParts.Count(); partIx++)
			{
				auto_ptr< CArrayFix<TPtrC> > lines( new (ELeave) CArrayFixFlat<TPtrC>(10) );
				if (widths->Count() == 0) widths->AppendL( lastLineW );
				auto_ptr<HBufC> wrappedText( AknBidiTextUtils::ConvertToVisualAndWrapToArrayWholeTextL(textParts[partIx].iText,
																									   *widths,
																									   *font,
																									   *lines) );
				
				TInt currentLine = 0;
				while ( currentLine < lines->Count() )
					{
						iWorkBuf->Des().Zero();

						if (widths->Count() == 0)
							widths->AppendL( lastLineW ); // ensures that we have at least one width value to work
						TInt groupW = widths->At(0);
						TPtr tmpPtr = iWorkBuf->Des();
						currentLine = GroupLinesL( currentLine, *lines, *widths, tmpPtr );
						
						// workbuf now contains all text for one label						
						{
							
							MJuikSizer* sizer = groupW == firstLineW ? iTextSizers[0] : iTextSizers[1];
							auto_ptr<CJuikLabel> label( CJuikLabel::NewL( iTextColor, iFocusTextColor, font, Content() ));
							if ( iUseBgFocus )
								label->EnableFocusBackgroundL( iBgFocusColor );
							
							TLabelDef labelDef = { sizer, label.get(), textParts[partIx].iIsUrl };
							iLabelDefs.AppendL( labelDef );
							label->SetWrappedTextL( *iWorkBuf );
							Content().AddControlL( label.get(), iRunningId++ );
							sizer->AddL( *(label.release()), 0, Juik::EExpandNot ); 
						}
					}
			}
	}


	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{	
// 		TBuf<100> tmpBuf;
// 		tmpBuf.Format( _L("\t\t\tPost: type %d, code %d"), aType, aKeyEvent.iCode );
// 		Reporting().DebugLog( tmpBuf );
		
		
		if ( ! iFindEngine )
			return EKeyWasNotConsumed;

		TInt oldFocus = iFocusedUrl;
 		if ( aType == EEventKey )
 			{
 				if ( aKeyEvent.iCode == JOY_DOWN )
 					{
						iFocusedUrl++;
 					}
				else if ( aKeyEvent.iCode == JOY_UP )
					{
						iFocusedUrl--;
					}
 			}
		
		TInt urlCount = iFindEngine->ItemCount();
		if ( 0 <= oldFocus && oldFocus < urlCount )
			SetUrlFocus(oldFocus, EFalse);

		if ( 0 <= iFocusedUrl && iFocusedUrl < iFindEngine->ItemCount() )
			{
				SetUrlFocus(iFocusedUrl, ETrue);
				return EKeyWasConsumed;
			}
		else 
			{
				iFocusedUrl = KErrNotFound;
				return EKeyWasNotConsumed;				
			}
	}
	
	void SetUrlFocus(TInt aUrlIx, TBool aFocus)
	{
		TInt urlIx = -1;
		TInt labelIx = KErrNotFound;
		for (TInt i = 0; i < iLabelDefs.Count(); i++ )
			{
				if ( iLabelDefs[i].iIsUrl ) 
					urlIx++;
				if ( urlIx == aUrlIx )
					{
						labelIx = i;
						break;
					}
			}
		if ( labelIx >= 0 )
			{
				iLabelDefs[labelIx].iLabel->SetUnderlining( aFocus );
				iLabelDefs[labelIx].iLabel->SetFocus( aFocus );
			}
	}


	TBool PreFocusGained(TFocusFrom aFocusFrom)
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("PreFocusGained"));
		TInt count = 0;
		if ( iFindEngine )
			count = iFindEngine->ItemCount();
		
		if ( count > 0 )
			{
				if ( aFocusFrom == EAbove )
					iFocusedUrl = 0;
				else
					iFocusedUrl = count - 1;
				return ETrue;
			}
		else
			{
				iFocusedUrl == KErrNotFound;
				return EFalse;
			}
	}
	
	void FocusChanged(TDrawNow aDrawNow)
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("FocusChanged"));
		if ( IsFocused() && iFocusedUrl >= 0 )
			{
				SetUrlFocus(iFocusedUrl, ETrue);
			}		
		//iContent->SetFocus(IsFocused(), aDrawNow );
	}
	
	TSize MinimumSize()
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("MinimumSize"));
		TSize sz = iContent->MinimumSize();
#ifdef JUIK_DEBUGGING_ENABLED
		DebugPrintSize( _L("CUrlMultiLabelImpl::MinimumSize"), sz );
#endif

		return sz;
	}
	
	TInt CountComponentControls() const
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("CountComponentControl"));
		return 1; 
	}


	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("ComponentControl"));
		return iContent;
	}
	
		
	void PositionChanged()
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("PositionChanged"));
//  		TPoint p = Position(); 
//  		iContent->SetPosition( p );
#ifdef JUIK_DEBUGGING_ENABLED
		DebugPrintRect( _L("CUrlMultiLabelImpl::PositionChanged"), Rect() );
#endif
	}
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("SizeChanged"));
		TRect r = Rect();
#ifdef JUIK_DEBUGGING_ENABLED
		DebugPrintRect( _L("CUrlMultiLabelImpl::SizeChanged"), Rect() );
#endif
		WrapTextL();
 		iContent->SetRect( r );
	}

	~CUrlMultiLabelImpl() 
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("~CUrlMultiLabelImpl"));
		delete iFindEngine;
		delete iContent;
		delete iUnwrappedText;
		delete iWorkBuf;
		iTextSizers.Close();
		iLabelDefs.Close();
	}
	
	void ZeroL()
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("Zero"));
		RemoveLabels();
		if (iImage) iImage->ClearL();
		iContent->SetSize( TSize(0,0) );
	}


	

	void FindUrlsL(const TDesC& aContent)
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("FindUrls"));
		CFindItemEngine::TFindItemSearchCase searchFor = CFindItemEngine::EFindItemSearchURLBin; //EFindItemSearchScheme;
		if ( ! iFindEngine )
			{
				iFindEngine = CFindItemEngine::NewL( aContent, searchFor );
			}
		else 
			iFindEngine->DoNewSearchL( aContent, searchFor );
	}
	
	TPtrC FocusedUrlL() 
	{
		CALLSTACKITEM_N(_CL("CUrlMultiLabelImpl"), _CL("FocusedUrlL"));
		if ( iFindEngine || iUnwrappedText || iFocusedUrl == KErrNotFound )
			{
				const CArrayFixFlat<CFindItemEngine::SFoundItem>* urls = iFindEngine->ItemArray();
				if ( urls )
					{
						CFindItemEngine::SFoundItem url = urls->At( iFocusedUrl );
						TPtrC result( iUnwrappedText->Mid( url.iStartPos, url.iLength ) );
						return result;
					}
			}
		return TPtrC(KNullDesC);
	}


	virtual TInt UrlCount() 
	{
		return iFindEngine ? iFindEngine->ItemCount() : 0;
	}
	
	virtual TPtrC UrlAtL(TInt aIndex) 
	{
		if ( !iFindEngine || aIndex < 0 || UrlCount() <= aIndex)
			Bug(_L("Accessing non existent url")).Raise();
		

		const CArrayFixFlat<CFindItemEngine::SFoundItem>* urls = iFindEngine->ItemArray();
		if ( !urls )
			Bug(_L("No urls")).Raise();
		
		CFindItemEngine::SFoundItem url = urls->At( aIndex );
		TPtrC result( iUnwrappedText->Mid( url.iStartPos, url.iLength ) );
		return result;
	}
	

	TBool HasUrl() 
	{
		return iFindEngine && iFindEngine->ItemCount() > 0;
	}

	


	void SetDebugId(TInt aDebugId)
	{
#ifdef JUIK_DEBUGGING_ENABLED
		iDebugId = aDebugId;
		for (TInt i=0; i < iLabelDefs.Count(); i++)
			{
				iLabelDefs[i].iLabel->SetDebugId( ((i+1)*1000000) + iDebugId );
			}
#endif
	}

#ifdef JUIK_DEBUGGING_ENABLED
	void DebugPrintRect(const TDesC& aMsg, const TRect& aRect) const
	{
		JuikDebug::PrintRect( const_cast<CUrlMultiLabelImpl*>(this), iDebugId, aMsg, aRect );
	}

	void DebugPrintSize(const TDesC& aMsg, TSize& aSize) const
	{
		JuikDebug::PrintSize( const_cast<CUrlMultiLabelImpl*>(this), iDebugId, aMsg, aSize );
	}

	TInt iDebugId;
#endif
public: // from MJuikControl	
	virtual const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }
	
private:
	CJuikSizerContainer* iContent; // own 

	HBufC* iUnwrappedText; // own 
	HBufC* iWorkBuf; // own 
	CJuikImage* iImage; 
	CFindItemEngine* iFindEngine;
	TInt iFocusedUrl;
	TBool iParseUrls;

	RPointerArray<MJuikSizer> iTextSizers;

	TRgb iTextColor;
	TRgb iFocusTextColor;
	TInt iRunningId;

	RArray<TLabelDef> iLabelDefs; 
	TInt iLastWrapWidth;

	TBool iUseBgFocus;
	TRgb  iBgFocusColor;
};

EXPORT_C CUrlMultiLabel* CUrlMultiLabel::NewL( CJuikImage* aImage, TRgb aColor, TRgb aFocusColor, TBool aParseUrls, CCoeControl& aParent)
{
	return CUrlMultiLabelImpl::NewL( aImage, aColor, aFocusColor, aParseUrls, aParent);
}
