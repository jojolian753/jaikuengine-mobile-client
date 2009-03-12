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

#include "ccu_themes.h"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"

#include "contextvariant.hrh"

#include <aknsbasicbackgroundcontrolcontext.h>
#include <aknsdrawutils.h>
#include <aknsitemid.h> 
#include <aknsutils.h>
#include <bitstd.h> 
#include <eikenv.h>
#include <fbs.h>
#include <w32std.h>

// Little play with functors to do color caching
template <class TClass, class TResult> class TZeroArgF 
{
private:
	TResult (TClass::*fpt)();   // pointer to member function
	TClass* pt2Object;                  // pointer to object
	
public:
	TZeroArgF() : pt2Object(NULL), fpt(NULL) {}
	// constructor - takes pointer to an object and pointer to a member and stores
	// them in two private variables
	TZeroArgF(TClass* _pt2Object, TResult(TClass::*_fpt)())
	{ pt2Object = _pt2Object;  fpt=_fpt; };
	
	// override operator "()"
	virtual TResult operator()()
	{
		if ( pt2Object  && fpt )
 			return (*pt2Object.*fpt)();
		else 
			{
			User::Leave(KErrNotReady);
			return NULL;
			}
	}
};


// Color tools 

TRgb AverageColor(TRgb x, TRgb y, TReal aMultiplier)
{
	TInt r = x.Red()   + (1 - aMultiplier) * (y.Red()   - x.Red());
	TInt g = x.Green() + (1 - aMultiplier) * (y.Green() - x.Green());
	TInt b = x.Blue()  + (1 - aMultiplier) * (y.Blue()  - x.Blue());
	return TRgb(r,g,b);
}

/**
 * CThemeColors read skin colors from skins and for those skin items
 * that doesn't have color, but only skin graphic available, it samples
 * graphic to get average color
 * 
 * Implementation of this class uses lazy construction of colors with functors
 * It is overly complex for this particular problem, but I'm testing 
 * it here to use it later for icons, where lazy construction is clearly needed.
 */

class CThemeColorsImpl : public CThemeColors, public MContextBase
{
public:
	~CThemeColorsImpl() { iCachedColors.Close(); }
	
	TZeroArgF<CThemeColorsImpl, TRgb> DoFunctor(TRgb(CThemeColorsImpl::*aF)())
	{
		return TZeroArgF<CThemeColorsImpl, TRgb>(this, aF);
	}
	
	void ConstructL()
	{
		TCachedColor c;
		c.iIsCached = EFalse;
		c.iColor = TRgb(0,0,0);
		//c.iFactoryF = NULL;
		for (TInt i=0; i < ELogicalColorCount; i++)
			{
				iCachedColors.AppendL( c );
			}
		
		iCachedColors[EPrimaryText].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadPrimaryTextL);
		
		iCachedColors[ESecondaryText].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadSecondaryTextL);

		iCachedColors[ESpecialPrimaryText].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadSpecialPrimaryTextL);
		
		iCachedColors[EPrimaryHighlightText].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadPrimaryHighlightTextL );
		
		iCachedColors[ESecondaryHighlightText].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadSecondaryHighlightTextL );
		
		iCachedColors[ESkinHighlight].iFactoryF =
			DoFunctor(&CThemeColorsImpl::ReadSkinHighlightL );
		
		iCachedColors[EBubble].iFactoryF =
			DoFunctor(&CThemeColorsImpl::ReadPrimaryTextL );
		//DoFunctor(&CThemeColorsImpl::ReadSettingItemL );
		
		iCachedColors[EBubbleHighlight].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadSkinHighlightL );
			
		iCachedColors[EMainBackground].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadMainBackgroundL );

		iCachedColors[EProgressBackground].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadProgressBackgroundL );

		iCachedColors[EProgressAlphaBackground].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadProgressAlphaBackgroundL );
			
		iCachedColors[EProgressText].iFactoryF = 
			DoFunctor(&CThemeColorsImpl::ReadProgressTextL );

	}
	
	void Reset()
	{
		for (TInt i=0; i < iCachedColors.Count(); i++)
			{
				TCachedColor& c = iCachedColors[ i ];
				c.iIsCached = EFalse;
			}
	}

	void PrepareAllL()
	{
		RereadAllL();
	}
	
	TRgb GetColorL(TLogicalColor aIndex)
	{
		TCachedColor& c = iCachedColors[ aIndex ];
		if ( ! c.iIsCached )
			{
				c.iColor = c.iFactoryF();
				c.iIsCached = ETrue;
			}
		return c.iColor;
	}
	
	void RereadAllL()
	{
		for (TInt i=0; i < ELogicalColorCount; i++)
			{
				GetColorL( static_cast<TLogicalColor>(i) );
			}
	}
	


	TRgb ReadSkinColorL( const TAknsItemID& aID, const TInt aIndex )
	{
		CALLSTACKITEM_N(_CL(""), _CL("ReadSkinColorL"));
		TRgb c = KRgbBlack;
		MAknsSkinInstance* skin = AknsUtils::SkinInstance();
		AknsUtils::GetCachedColor(skin, c, aID, aIndex);
		return c;
	}

	TRgb ReadPrimaryTextL() 
	{
		CALLSTACKITEM_N(_CL(""), _CL("ReadPrimaryTextColorL"));
		//return ReadSkinColorL(KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG7);
		return ReadSkinColorL(KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG6);
	}

	TRgb ReadPrimaryHighlightTextL() 
	{
		CALLSTACKITEM_N(_CL(""), _CL("ReadPrimaryHighlightTextColorL"));
		// Normal highlight texts
		return ReadSkinColorL(KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG10);
		// Setting item texts
		//return ReadSkinColorL(KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG12);
	}

	TRgb ReadProgressBackgroundL()
	{
		return GetColorL(EPrimaryHighlightText);
	}
	
	TRgb ReadProgressAlphaBackgroundL()
	{
		TRgb bg=GetColorL(EProgressBackground);
		bg.SetAlpha(bg.Alpha()/2);
		return bg;
	}

	TRgb SampleBitmapL(CFbsBitmap& aBmp, TInt aBorderWidth, CFbsBitmap* aMask=NULL)
	{
		TPoint tl( aBorderWidth, aBorderWidth );
		TPoint br = (TPoint(0,0) + aBmp.SizeInPixels()) - TSize(aBorderWidth, aBorderWidth);
		TInt r,g,b,a;
		r = g = b = a = 0;
		TInt count = 0;
		for (TInt y = tl.iY; y < br.iY; y++)
			{
				for (TInt x = tl.iX; x < br.iX; x++)
					{
						TPoint p(x,y);
						TRgb c;
						aBmp.GetPixel(c, p);
						r += c.Red();
						g += c.Green();
						b += c.Blue();
						
						if ( aMask ) 
							{
								TRgb c;
								aMask->GetPixel(c, p);
								a += c.Red();
							}
						count++;
					}
			}

#ifdef __DEV__
		{
			TBuf<50> buf;
			Reporting().DebugLog( _L("Sampling results, total:") );
			buf.Format( _L("%d,%d,%d,%d"), r,g,b,a);
			Reporting().DebugLog( buf );
		}
#endif 

		// calculate average
		r /= count;
		g /= count;
		b /= count;
		a /= count;
		
#ifdef __DEV__
		{
			TBuf<50> buf;
			Reporting().DebugLog( _L("Sampling results, average:") );
			buf.Format( _L("%d,%d,%d,%d"), r,g,b,a);
			Reporting().DebugLog( buf );
		}
#endif
		
		if ( aMask )
			return TRgb(r,g,b,a);
		else 
			return TRgb(r,g,b);
	}

	TBool DrawBackgroundItem(CFbsBitGc* bmpContext, const TAknsItemID& aItemID, TSize sampleSize, TInt borderWidth )
	{
		TRect outerRect( TPoint(0,0), sampleSize );
		TRect innerRect = outerRect;
		innerRect.Shrink( borderWidth, borderWidth );
		
		auto_ptr<CAknsBasicBackgroundControlContext> background(CAknsBasicBackgroundControlContext::NewL( aItemID, outerRect, EFalse) );
		
		
		MAknsSkinInstance* skin = AknsUtils::SkinInstance();
		if ( ! skin ) 
			return EFalse;
		
		TBool skinDrawn  = AknsDrawUtils::DrawBackground( skin, background.get(), NULL, *bmpContext, TPoint(0,0), outerRect, KAknsDrawParamDefault );
		return skinDrawn;
	}

	void SampleSingleSkinItemL(const TAknsItemID& aItemID,
							   TRgb& aResult,
							   TRgb* aAlphaColor=0)
	{ 
		CALLSTACKITEM_N(_CL(""), _CL("SampleSkinItemL"));
		
		TSize sampleSize( 40, 40 );
		TInt borderWidth(0);
		
		// Create a temporary bitmap
		CEikonEnv* env = CEikonEnv::Static();
		CWsScreenDevice* screen = env->ScreenDevice();

		TDisplayMode dmode = screen->DisplayMode();
 
		auto_ptr<CFbsBitmap> bmp( new (ELeave) CFbsBitmap );
		bmp->Create( sampleSize, dmode );
    
		auto_ptr<CFbsBitmapDevice> bmpDevice(CFbsBitmapDevice::NewL( bmp.get() ) );
		auto_ptr<CFbsBitGc> bmpContext( NULL );
		{
			CFbsBitGc* ctx = NULL;
			User::LeaveIfError( bmpDevice->CreateContext( ctx ) );
			bmpContext.reset( ctx );
		}
		
		TBool skinDrawn = DrawBackgroundItem( bmpContext.get(), aItemID, sampleSize, borderWidth);
		if ( ! skinDrawn && !aAlphaColor)
			return;
		
		if (aAlphaColor) {
			CFbsBitGc& gc=*bmpContext;
			gc.SetPenStyle(CGraphicsContext::ESolidPen);
			gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
			gc.SetBrushColor(*aAlphaColor);	
			gc.SetPenColor(*aAlphaColor);			
			gc.DrawRect( TRect( TPoint(0, 0), sampleSize) );
		}
		aResult = SampleBitmapL( *bmp, borderWidth );
	}

	
	void SampleSkinItemL(const TAknsItemID& aFrameID,
						 const TAknsItemID& aCenterID,
						 TRgb& aResult)
	{
		CALLSTACKITEM_N(_CL(""), _CL("SampleSkinItemL"));
		SampleSkinItemWithBackgroundL(aFrameID, aCenterID, NULL, aResult);
	}

	
	void SampleSkinItemWithBackgroundL(const TAknsItemID& aFrameID,
									   const TAknsItemID& aCenterID,
									   const TAknsItemID* aBackgroundID,
									   TRgb& aResult)
	{
		CALLSTACKITEM_N(_CL(""), _CL("SampleSkinItemWithBackgroundL"));
		
		TSize sampleSize( 20, 20 );
		TInt borderWidth(4);
		
		// Create a temporary bitmap
		CEikonEnv* env = CEikonEnv::Static();
		CWsScreenDevice* screen = env->ScreenDevice();

		TDisplayMode dmode = screen->DisplayMode();
 
		auto_ptr<CFbsBitmap> bmp( new (ELeave) CFbsBitmap );
		bmp->Create( sampleSize, dmode );
    
		auto_ptr<CFbsBitmapDevice> bmpDevice(CFbsBitmapDevice::NewL( bmp.get() ) );
		auto_ptr<CFbsBitGc> bmpContext( NULL );
		{
			CFbsBitGc* ctx = NULL;
			User::LeaveIfError( bmpDevice->CreateContext( ctx ) );
			bmpContext.reset( ctx );
		}

		if ( aBackgroundID )
			{
				TBool bgSkinDrawn = DrawBackgroundItem( bmpContext.get(), *aBackgroundID, sampleSize, borderWidth);
				if ( ! bgSkinDrawn )
					return;
			}

    
		MAknsSkinInstance* skin = AknsUtils::SkinInstance();
		if ( ! skin ) 
			return;
	
		TRect outerRect( TPoint(0,0), sampleSize );
		TRect innerRect = outerRect;
		innerRect.Shrink( borderWidth, borderWidth );
    
		TBool skinDrawn = AknsDrawUtils::DrawFrame( skin, *bmpContext, 
													outerRect, innerRect, 
													aFrameID, aCenterID, 
													KAknsDrawParamLimitToFirstLevel | KAknsDrawParamNoClearUnderImage);
		if ( ! skinDrawn ) 
			return;
		
		aResult = SampleBitmapL( *bmp, borderWidth );
	}

	void SampleBitmapSkinItemL(const TAknsItemID& aSkinId,
							   TRgb& aResult)
	{
		CALLSTACKITEM_N(_CL(""), _CL("SampleBitmapSkinItemL"));
		    
		MAknsSkinInstance* skin = AknsUtils::SkinInstance();
		if ( ! skin ) 
			return;
		
		CFbsBitmap* bmp = NULL;
		CFbsBitmap* mask = NULL;
		AknsUtils::GetCachedMaskedBitmap( skin, aSkinId, bmp, mask );
		if ( bmp )
			{
				Reporting().DebugLog( _L("Was able to create bitmap for sampling") );
				AknIconUtils::SetSize( bmp, TSize(20,20), EAspectRatioPreservedAndUnusedSpaceRemoved) ;
				AknIconUtils::SetSize( mask, TSize(20,20), EAspectRatioPreservedAndUnusedSpaceRemoved) ;
				aResult = SampleBitmapL( *bmp, 0, mask );
				return;
			}
		else 
			{
				SampleSingleSkinItemL( aSkinId, aResult );				
			}
	}


	TRgb ReadSkinHighlightL()
	{
		CALLSTACKITEM_N(_CL(""), _CL("ReadSkinHighlightL"));
		TRgb result = TRgb(200,0,0);
		SampleSkinItemWithBackgroundL( KAknsIIDQsnFrList, KAknsIIDQsnFrListCenter, &KAknsIIDQsnBgAreaMain, result );	
		//SampleBitmapSkinItemL( KAknsIIDQsnFrListCenter, result );	
		return result;
	}

	TRgb ReadSettingHighlightL()
	{
		CALLSTACKITEM_N(_CL(""), _CL("ReadSettingHighlightL"));
		TRgb result = TRgb(200,0,0);
		SampleSkinItemL( KAknsIIDQsnFrSetOptFoc, KAknsIIDQsnFrSetOptFocCenter, result);
		return result;
	}

	TRgb ReadSettingItemL()
	{
		CALLSTACKITEM_N(_CL(""), _CL("ReadSettingItemL"));
		TRgb result = TRgb(230,230,230);
		SampleSkinItemL( KAknsIIDQsnFrSetOpt, KAknsIIDQsnFrSetOptCenter, result);	
		return result;
	}

	TRgb ReadMainBackgroundL()
	{
		CALLSTACKITEM_N(_CL(""), _CL("ReadMainBackgroundL"));
		TRgb result = GetColorL( EPrimaryText );
		SampleSingleSkinItemL( KAknsIIDQsnBgAreaMain, result);	
		return result;
	}
	
	TInt MedianDiff(TRgb aC1, TRgb aC2)
	{
		RArray<TInt> d;
		CleanupClosePushL(d);
		d.ReserveL(3);
		d.AppendL(abs(aC1.Red()-aC2.Red()));
		d.AppendL(abs(aC1.Green()-aC2.Green()));
		d.AppendL(abs(aC1.Blue()-aC2.Blue()));

		d.Sort();
		TInt ret=d[1];
		CleanupStack::PopAndDestroy();
		
		return ret;
	}
	
	TRgb ReadProgressTextL()
	{
		TRgb bg1=GetColorL(EProgressBackground);
		TRgb alpha=GetColorL(EProgressAlphaBackground);
		TRgb bg2;
		SampleSingleSkinItemL( KAknsIIDQsnBgAreaMain, bg2, &alpha);
		
		TRgb fg1=GetColorL(EBubbleHighlight);
		TRgb fg2=KRgbBlack;
		TRgb fg3=KRgbWhite;
		TRgb fg4=TRgb(127, 127, 127);
		
		TInt diff1=Min(MedianDiff(bg1, fg1), MedianDiff(bg2, fg1));
		TInt diff2=Min(MedianDiff(bg1, fg2), MedianDiff(bg2, fg2));
		TInt diff3=Min(MedianDiff(bg1, fg3), MedianDiff(bg2, fg3));
		TInt diff4=Min(MedianDiff(bg1, fg4), MedianDiff(bg2, fg4));
		
		TInt max=Max(Max(diff1, diff2), Max(diff3, diff4));
		if (diff1==max) return fg1;
		if (diff3==max) return fg3;
		if (diff4==max) return fg4;
		/*if (diff2==max)*/ return fg2;
	}

	TRgb ReadSecondaryTextL()
	{
		TRgb primary = GetColorL( EPrimaryText );
		TRgb background = GetColorL( EMainBackground );
		return AverageColor(primary, background, 0.90); 
	}
	
	TRgb ReadSpecialPrimaryTextL()
	{
		TRgb primary = GetColorL( EPrimaryText );
		TRgb background = GetColorL( EMainBackground );
		return AverageColor(primary, background, 0.65); 
	}


	TRgb ReadSecondaryHighlightTextL()
	{
		TRgb primary = GetColorL( EPrimaryHighlightText );
		TRgb bubble = GetColorL( EBubbleHighlight );
		return AverageColor(primary, bubble, 0.90); 
	}  


private:
	struct TCachedColor
	{
		TBool iIsCached;
		TRgb iColor;
		TZeroArgF<CThemeColorsImpl, TRgb> iFactoryF;
	};
	RArray<TCachedColor> iCachedColors;
};

EXPORT_C CThemeColors* CThemeColors::NewL()
{
	auto_ptr<CThemeColorsImpl> self( new (ELeave) CThemeColorsImpl );
	self->ConstructL();
	return self.release();
}

