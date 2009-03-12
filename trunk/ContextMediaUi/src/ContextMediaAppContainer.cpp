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

#include "break.h"
#include "ContextMediaAppContainer.h"
#include <akntitle.h>
#include <aknnavi.h>
#include "ContextMediaAppView2.h"
#include "ContextMediaAppView.h"
#include <aknviewappui.h> 
#include "contextmediaui.hrh"
#include <contextmediaui.rsg>
#include <txtrich.h>
#include "cm_post.h"
#include "cl_settings.h"
#include <avkon.rsg>
#include <aknnotewrappers.h>
#include <eiksbfrm.h>
#include <akncontext.h>
#include <bautils.h>
#include "Contextnotifyclientsession.h"
#include "cm_tags.h"
#include "cm_taglist.h"
#include "cm_autotaglist.h"
#include "bberrorinfo.h"
#include "raii_f32file.h"
#include "hintbox.h"
#include "cc_imei.h"
#include "cu_common.h"
#include "util.h"
#include "ccu_utils.h"

#include "jaiku_layout.h"
#include "juik_fonts.h"

#ifdef __S60V2__
#define USE_SKIN 1
#endif

#ifdef USE_SKIN
#include <AknsControlContext.h>
#include <AknsBasicBackgroundControlContext.h>
#include <AknsDrawUtils.h>
#endif



/*
 * We want to load jpeg files as quickly as possible.
 * Using the CMdaImageFileToBitmapUtility directly is very slow,
 * so we use CPAlbImageViewerBasic which is a lot faster (it
 * uses CMdaImageFileToBitmapUtility internally, so it must
 * be giving some special parameters to it we are not privy to).
 * The problem with CPAlbImageViewerBasic is that the loading
 * isn't asynchronous to the caller: it all happens in one
 * function call. However, the LoadImageL actually starts another
 * active scheduler loop! So we may get user-interface events while
 * in the call, introducing re-entrancy. Handling of that 
 * re-entrancy is done in the DisplayPicture and DisplayPictureInnerL.
 *
 * Now we only use CPAlbImageViewerBasic on v1, and CImageDecoder on v2.
 *
 */

// Some display constants

#define KTextColor TRgb(0,0,0)
#define KFocusedBgColor TRgb(205,205,255)
#define KUnfocusedBgColor TRgb(255, 255, 255)
#define KTagsBgColor TRgb(255, 255, 255)

#define	KRgbTags TRgb(0, 100, 0)

#ifdef __WINS__
	_LIT(KMbmVideo, "z:\\system\\data\\contextmediaui_video.mbm");
	_LIT(KMbmAudio, "z:\\system\\data\\contextmediaui_audio.mbm");
	_LIT(KMbmUnknown, "z:\\system\\data\\contextmediaui_unknown.mbm");
#else
	_LIT(KMbmVideo, "c:\\system\\data\\contextmediaui_video.mbm");
	_LIT(KMbmAudio, "c:\\system\\data\\contextmediaui_audio.mbm");
	_LIT(KMbmUnknown, "c:\\system\\data\\contextmediaui_unknown.mbm");
#endif

EXPORT_C TInt LoadCmuiResourceL()
{
	CEikonEnv* env=CEikonEnv::Static();
	return LoadSystemResourceL(env, _L("contextmediaui"));
}

enum THints {
	/* don't change IDs if you adding, data is stored in the bb */
	EClickTagsHint=1,
	ETagHint,
	EMoveUpToContextHint,
};


TMargins8 Margins(TInt left, TInt right, TInt top, TInt bottom)
{
	TMargins8 m;
	m.iLeft = left;
	m.iRight = right;
	m.iTop = top;
	m.iBottom = bottom;
	return m;
}


class CArrowControl : public CCoeControl
{
public:
	enum TDirection
		{
			ELeftArrow = 0,
			ERightArrow 
		} iDirection;
	CArrowControl(TDirection aDir) : iDirection( aDir ) 
	{
	}

protected:	
	void Draw(const TRect& aRect) const
	{
		TRect r = Rect();
		CWindowGc& gc = SystemGc();
		
		TRect arrowR = r ; 
		TInt centerX = iDirection == ERightArrow ? arrowR.iTl.iX : arrowR.iBr.iX;
		TPoint top = TPoint( centerX, arrowR.iTl.iY  );
		TPoint bottom = TPoint( centerX, arrowR.iBr.iY );
		TInt tipX = iDirection == ELeftArrow ? arrowR.iTl.iX : arrowR.iBr.iX;
		TPoint tip = TPoint( tipX, arrowR.Center().iY );
		
		gc.DrawLine( tip, top );
		gc.DrawLine( tip, bottom );
	}
};


class CBorderedContainer : public CCoeControl
{
public:
	CBorderedContainer() 
	{ 
		iBorderWidth = 1;
		iContentMargin = Margins(0,0,0,0);
	}

	~CBorderedContainer() 
	{
		delete iContent;
	}

	void SetMargin(TMargins8 aMargin)
	{
		iContentMargin = aMargin;
	}
	
	void SetContentL( CCoeControl* aContent )
	{
		if ( iContent ) 
			{
				delete iContent;
				iContent = NULL;
			}
		iContent = aContent;
		iContent->SetContainerWindowL( *this );
	}
	
	CCoeControl* Content()
	{
		return iContent;
	}

protected: 
	TInt CountComponentControls() const
	{
		return iContent ? 1 : 0; 
	}
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		return iContent;
	}
	
	
protected:	
	void SizeChanged() 
	{
		TMargins8 borderMargin = Margins( iBorderWidth, iBorderWidth, iBorderWidth, iBorderWidth );
		TRect contentRect = iContentMargin.InnerRect( borderMargin.InnerRect( Rect() ) );
		iContent->SetRect( contentRect );
	}
	
	

	void Draw(const TRect& aRect) const
	{
		TRect r = Rect();
		CWindowGc& gc = SystemGc();
		
		gc.SetBrushColor( KTagsBgColor );
		gc.SetPenColor( KTextColor );
		gc.SetPenSize( TSize( iBorderWidth, iBorderWidth ) );
		gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
		gc.SetPenStyle( CGraphicsContext::ESolidPen );
		
		gc.DrawRect( r );
	}
	
	TInt iBorderWidth;
	TMargins8 iContentMargin;
	CCoeControl* iContent;
};
	


const TTupleName KMediaContainerHintTuple = { { CONTEXT_UID_CONTEXTMEDIAUI }, 1 };

class OurOwnRichTextEditor : public CEikRichTextEditor
{
public:
	OurOwnRichTextEditor(HBufC * aDefaultValue,
		CContextMediaAppGeneralContainer* aContainer): 
			iDefaultValue(aDefaultValue), iContainer(aContainer) {}
        void ConstructL(const CCoeControl* aParent, TRgb aColor, TInt aNumberOfLines,TInt aTextLimit,TInt
		aEdwinFlags,TInt aFontControlFlags=EGulFontControlAll,TInt aFontNameFlags=EGulNoSymbolFonts);
	CParaFormatLayer* l;
	CCharFormatLayer* cl;
	inline ~OurOwnRichTextEditor() {
		delete l;
		delete cl;
	}
	void PrepareForFocusLossL();
	void PrepareForFocusGainL();
	void Reset();
	void Zero();
	void AppendParagraph(const TDesC& aText, TRgb aColor, CParaFormat::TAlignment aAlignment);
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
private:
	HBufC * iDefaultValue;
	TBool iShowingDefault;
	CContextMediaAppGeneralContainer* iContainer;
	TRgb iColor;
};

// #ifdef __S60V2__
// #include <ImageConversion.h>
// class MLoaderCb {
// public:
// 	virtual void Loaded(TInt aError) = 0;
// };

// class CJpegView : public CCoeControl, public MContextBase, public MLoaderCb {
// public:
// 	CContextMediaAppPostContainer* iContainer;

// 	class CLoader : public CActive, public MContextBase {
// 	public:

// 		CLoader(const TSize& aSize) : CActive(EPriorityNormal), iMaxSize(aSize) { }
// 		void ConstructL(MLoaderCb* aCb) {
// 			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("ConstructL"));
// 			CActiveScheduler::Add(this);
// 			msg=_L("Loading...");
// 			iCb=aCb;
// 		}

// 		TSize ScaledSize() { return iScaledSize; }

// 		TSize ScaleSize(TSize aPhotoSize, TSize aMaxSize) 
// 		{
// 			TReal hQ = aPhotoSize.iWidth / (TReal) aMaxSize.iWidth; 
// 			TReal vQ = aPhotoSize.iHeight / (TReal) aMaxSize.iHeight; 

// 			TInt photoV = vQ > hQ ? aPhotoSize.iHeight : aPhotoSize.iWidth;
// 			TInt maxV = vQ > hQ ? aMaxSize.iHeight : aMaxSize.iWidth;
// 			TInt resultV = photoV; 
			
// 			if ( maxV == 0 )
// 				return TSize(0,0);

// 			TInt scale = 1; 
// 			while ( resultV > maxV )
// 				{
// 					scale *= 2; 
// 					resultV /= 2;
// 				}

// 			return TSize( aPhotoSize.iWidth / scale, aPhotoSize.iHeight / scale);
// 		}
			
		
// 		void StartToConvertFileL(const TDesC& aFilename) {
// 			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("StartToConvertFileL"));

// 			iDone=EFalse;
// 			Cancel();
// 			delete iDecoder; iDecoder=0;
// 			delete iBitmap; iBitmap=0;
// 			CC_TRAPD(err, iDecoder=CImageDecoder::FileNewL(Fs(), aFilename) );
// 			if ( err != KErrNone )
// 				User::Leave( err );

// 			TFrameInfo frameInfo=iDecoder->FrameInfo();
 			
// 			TSize photoSz = frameInfo.iOverallSizeInPixels;
			
// 			TBool doScale = photoSz.iWidth > iMaxSize.iWidth || photoSz.iHeight > iMaxSize.iHeight;
// 			TSize scaledSize = photoSz;
			
// 			if ( doScale )
// 				{
// 					scaledSize = ScaleSize( photoSz, iMaxSize );
// 				}
						
// 			iBitmap=new (ELeave) CFbsBitmap();
// 			iBitmap->Create(scaledSize, EColor64K);
// 			iDecoder->Convert(&iStatus, *iBitmap);
			
// 			SetActive();
// 		}
// 		TBuf<30> msg;
// 		void DoCancel() {
// 			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("DoCancel"));
// 			if ( iDecoder )
// 				iDecoder->Cancel();
// 		}
// 		void RunL() {
// 			TInt aError=iStatus.Int();
// 			if (aError==KErrUnderflow) {
// 				iDecoder->ContinueConvert(&iStatus);
// 				SetActive();
// 				return;
// 			}
// 			if (aError!=KErrNone) {
// 				msg=_L("Error loading ");
// 				msg.AppendNum(aError);
// 			} else {
// 				iDone=ETrue;
// 			}
// 			delete iDecoder; iDecoder=0;
// 			iCb->Loaded(aError);
// 		}
// 		TBool iDone;
// 		~CLoader() {
// 			Cancel();
// 			delete iDecoder;
// 			delete iBitmap;
// 		}

// 		CImageDecoder* iDecoder;
// 		CFbsBitmap* iBitmap;
// 		MLoaderCb* iCb;
		
// 		TSize iScaledSize;
// 		TSize iMaxSize;
// 	};

// // 	class CLoader : public CActive, public MContextBase {
// // 	public:
// // 		CImageDecoder* iDecoder;
// // 		CFbsBitmap* iBitmap;
// // 		MLoaderCb* iCb;
// // 		TRect iRect;
// // 		CLoader(const TRect& aRect) : CActive(EPriorityNormal), iRect(aRect) { }
// // 		void ConstructL(MLoaderCb* aCb) {
// // 			CActiveScheduler::Add(this);
// // 			msg=_L("Loading...");
// // 			iCb=aCb;
// // 		}
// // 		TBool iExact;
// // 		TSize iSize;
// // 		TSize Size() { return iSize; }
// // 		void SetFileL(const TDesC& aFilename) {
// // 			iDone=EFalse;
// // 			Cancel();
// // 			delete iDecoder; iDecoder=0;
// // 			delete iBitmap; iBitmap=0;
// // 			iDecoder=CImageDecoder::FileNewL(Fs(), aFilename);
// // 			TFrameInfo frameInfo=iDecoder->FrameInfo();
// // 			if (frameInfo.iOverallSizeInPixels.iWidth > frameInfo.iOverallSizeInPixels.iHeight) {
// // 				if (iRect.Width()<320) {
// // 					iSize=TSize(160, 120);
// // 				} else {
// // 					iSize=TSize(320, 240);
// // 				}
// // 			} else {
// // 				if (iRect.Width()<320) {
// // 					iSize=TSize(90, 120);
// // 				} else {
// // 					iSize=TSize(180, 240);
// // 				}
// // 			}
// // 			int scale=1, use_scale=-1; TBool exact=EFalse;
// // 			TInt height=iSize.iHeight;
// // 			for (int i=0; i<4; i++) {
// // 				if (frameInfo.iOverallSizeInPixels.iHeight < height*scale) {
// // 					if (scale==1) use_scale=1;
// // 					else use_scale=scale/2;
// // 					break;
// // 				}
// // 				if (frameInfo.iOverallSizeInPixels.iHeight == height*scale ) {
// // 					exact=ETrue;
// // 					use_scale=scale;
// // 					break;
// // 				}
// // 				scale*=2;
// // 			}
// // 			if (use_scale==-1) use_scale=8;
// // 			TSize s( frameInfo.iOverallSizeInPixels.iWidth/use_scale, 
// // 				frameInfo.iOverallSizeInPixels.iHeight/use_scale);
// // 			iBitmap=new (ELeave) CFbsBitmap();
// // 			iBitmap->Create(s, EColor64K);
// // 			iDecoder->Convert(&iStatus, *iBitmap);
// // 			SetActive();
// // 		}
// // 		TBuf<30> msg;
// // 		void DoCancel() {
// // 			iDecoder->Cancel();
// // 		}
// // 		void RunL() {
// // 			TInt aError=iStatus.Int();
// // 			if (aError==KErrUnderflow) {
// // 				iDecoder->ContinueConvert(&iStatus);
// // 				SetActive();
// // 				return;
// // 			}
// // 			if (aError!=KErrNone) {
// // 				msg=_L("Error loading ");
// // 				msg.AppendNum(aError);
// // 			} else {
// // 				iDone=ETrue;
// // 			}
// // 			delete iDecoder; iDecoder=0;
// // 			iCb->Loaded(aError);
// // 		}
// // 		TBool iDone;
// // 		~CLoader() {
// // 			Cancel();
// // 			delete iDecoder;
// // 			delete iBitmap;
// // 		}
// // 	};
// 	CLoader* iLoader;
// 	TRect iRect;
// 	void ConstructL(CContextMediaAppPostContainer* aParent, const TRect& aRect) {
// 		iContainer=aParent;
// 		SetContainerWindowL(*aParent);
// 		SetRect(aRect);
		
// 		ActivateL();
// 		iLoader=new (ELeave) CLoader(aRect.Size());
// 		iLoader->ConstructL(this);
// 	}
// 	void SetFileL(const TDesC& aFilename) {
// 		iLoader->StartToConvertFileL(aFilename);
// 	}

// 	virtual void Loaded(TInt aError) {
// 		iContainer->MediaLoaded(ETrue);
// 	}



// // 	void Draw(const TRect& aRect) const {
// // 		CALLSTACKITEM_N(_CL("CJuikPhotoImpl"), _CL("Draw"));
// // 		TRect fullR = Rect();
// // 		TRect innerR = fullR; //Margin.InnerRect( Rect() );
		
// //  		CWindowGc& gc = SystemGc();
		
// // 		TRect imgR; 
// // 		if (iLoader->iDone) {
// // 			CFbsBitmap* bmp = iLoader->iBitmap;
			
// // 			if (!bmp) return;
			
			
// // 			CWindowGc& gc = SystemGc();

// // 			TRect outer = Rect();
// // 			TRect inner = outer;// Margin.InnerRect( outer );
			
// // 			TSize bmpSize = bmp->SizeInPixels();
// // 			TBool clip = inner.Size().iWidth < bmpSize.iWidth || inner.Size().iHeight < bmpSize.iHeight;
			
// // 			TRect aligned = iAlignment.InnerRect( inner, bmpSize );
// // 			imgR = aligned;
// // 			TSize sourceSz = clip ? aligned.Size() : bmpSize;
// // 			TRect sourceR( TPoint(0,0), sourceSz );
// // 			gc.BitBlt( aligned.iTl, bmp, sourceR);	
// // 		}

// // // 		gc.SetBrushColor(KRgbWhite);
// // // 		gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
// // // 		TRect borderR = iMargin.OuterRect( imgR );
// // // 		DrawUtils::ClearBetweenRects( gc, borderR, imgR );
		
		
// // // #ifdef JUIK_BOUNDINGBOXES
// // // 	JuikDebug::DrawBoundingBox(gc ,Rect());
// // // #endif

// // 	}

// 	void Draw(const TRect& aRect) const {
// 		CWindowGc& gc = SystemGc();
// 		gc.SetPenStyle(CGraphicsContext::ENullPen);
// 		gc.SetBrushColor(KRgbWhite);
// 		gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
// 		gc.DrawRect(aRect);

// 		TRect fullR = Rect();

// 		if (iLoader->iDone) {
// 			TSize s=iLoader->iBitmap->SizeInPixels();
// 			TRect r=fullR;
// 			r.SetSize(s);
// 			if (s.iWidth<iSize.iWidth) r.Move((iSize.iWidth-s.iWidth)/2, 0);

// 			if ( aRect == fullR) {
// 				if ( iLoader->iBitmap->SizeInPixels() != s) {
// 					gc.DrawBitmap(r, iLoader->iBitmap);
// 				} else {
// 					gc.BitBlt(r.iTl, iLoader->iBitmap);
// 				}
// 			} else {
// 				if (r.Intersects(aRect)) {
// 					TRect inter=r;
// 					inter.Intersection(aRect);
// 					TSize bitmaps=iLoader->iBitmap->SizeInPixels();

// 					if ( bitmaps != s) {
// 						TRect from=inter;
// 						from.Move( -fullR.iTl.iX, -fullR.iTl.iY );
// 						if (s.iWidth<iSize.iWidth) from.Move(-(iSize.iWidth-s.iWidth)/2, 0);
// 						double scale=(double)bitmaps.iHeight / (double)s.iHeight;
// 						from.SetSize( TSize(
// 							from.Width() * scale,
// 							from.Height() * scale ));
// 						from.Move(
// 							from.iTl.iX * scale - from.iTl.iX,
// 							from.iTl.iY * scale - from.iTl.iY);
// 						gc.DrawBitmap(inter, iLoader->iBitmap, from);
// 					} else {
// 						TPoint p=aRect.iTl;
// 						inter.Move( -fullR.iTl.iX, -fullR.iTl.iY );
// 						gc.BitBlt(p, iLoader->iBitmap, inter);
// 					}
// 				}
// 			}
// 		} else {
// 			TRect textr=fullR;
// 			textr.SetSize( TSize(iSize.iWidth, 30) );
// 			if (aRect.Intersects(textr)) {
// 				const CFont* fontUsed = iEikonEnv->DenseFont();
// 				gc.UseFont(fontUsed);
// 				gc.DrawText(iLoader->msg, Rect().iTl);
// 			}
// 		}
// 	}
// 	CFbsBitmap* Bitmap() {
// 		return iLoader->iBitmap;
// 	}

// 	~CJpegView() {
// 		delete iLoader;
// 	}
// };
// #endif

void GetIMEI(TDes& aInto)
{
#ifndef __WINS__
	GetImeiL(aInto);
#else
	// Return a fake IMEI when working on emulator
	_LIT(KEmulatorImsi, "244050000000000");
	aInto=KEmulatorImsi;
#endif
}

void CopyFbsBitmapL(CFbsBitmap * orig, CFbsBitmap * dest)
{
	TSize s = orig->SizeInPixels();
	User::LeaveIfError(dest->Create(s, orig->DisplayMode()));

	TBitmapUtil orig_util(orig);
	TBitmapUtil dest_util(dest);
	orig_util.Begin(TPoint(0,0)); 
	dest_util.Begin(TPoint(0,0), orig_util); 

	TInt xPos;
	for (TInt yPos=0;yPos<s.iHeight;yPos++) {
		orig_util.SetPos(TPoint(0,yPos));
		dest_util.SetPos(TPoint(0,yPos));
		for (xPos=0;xPos<s.iWidth;xPos++) {
			dest_util.SetPixel(orig_util);
			orig_util.IncXPos();
			dest_util.IncXPos();
		}
	}
	orig_util.End();
	dest_util.End();
}

void AddBusyToBitmap(CFbsBitmap * bitmap)
{
	TRgb target_colour=KRgbRed;
	if (bitmap->DisplayMode() == EGray2) {
		target_colour=KRgbBlack;
	}

	TDisplayMode m = bitmap->DisplayMode();
	if (m>=EColor4K) m=EColor256;
	auto_ptr<CPalette> p (CPalette::NewDefaultL(m));
	TInt colour_idx = p->NearestIndex(KRgbRed);

	TBitmapUtil util(bitmap);
	util.Begin(TPoint(0,0));
	TInt xPos,yPos;
	for (yPos=5; yPos<20; yPos++) {
		for (xPos=5; xPos<20; xPos++) {
			util.SetPos(TPoint(xPos,yPos));
			util.SetPixel(colour_idx);
			util.IncXPos();
		}
	}
	util.End();
}

//-------------------------------------------------------------------------------------

CContextMediaAppGeneralContainer::CContextMediaAppGeneralContainer(CCMNetwork &aCMNetwork, CPostStorage &aStorage, TInt64 aNode, 
				CPostStorage::TSortBy aSort, CPostStorage::TOrder aOrder, CAknView * aView): 
			iStorage(aStorage), 
			iCMNetwork(aCMNetwork),
			iNode(aNode),
			iSort(aSort), 
			iOrder(aOrder),
			iView (aView){}

void CContextMediaAppGeneralContainer::BaseConstructL()
{
	iVideoMbmPath=KMbmVideo();
	iAudioMbmPath=KMbmAudio();
	iUnknownMbmPath=KMbmUnknown();
	if (! BaflUtils::FileExists(Fs(), iVideoMbmPath))	iVideoMbmPath.Replace(0, 1, _L("e"));
	if (! BaflUtils::FileExists(Fs(), iAudioMbmPath))	iAudioMbmPath.Replace(0, 1, _L("e"));
	if (! BaflUtils::FileExists(Fs(), iUnknownMbmPath)) 	iUnknownMbmPath.Replace(0, 1, _L("e"));
}

void CContextMediaAppGeneralContainer::GetVideoMbmPath(TFileName &aFilename)
{
	aFilename.Copy(iVideoMbmPath);
}

void CContextMediaAppGeneralContainer::GetAudioMbmPath(TFileName &aFilename)
{
	aFilename.Copy(iAudioMbmPath);
}

void CContextMediaAppGeneralContainer::GetUnknownMbmPath(TFileName &aFilename)
{
	aFilename.Copy(iUnknownMbmPath);
}


CContextMediaAppGeneralContainer::~CContextMediaAppGeneralContainer()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppGeneralContainer"), _CL("~CContextMediaAppGeneralContainer"));
	iEikonEnv->ScreenDevice()->ReleaseFont(iLatin12);
}

EXPORT_C CContextMediaAppListboxContainer::CContextMediaAppListboxContainer(CCMNetwork &aCMNetwork, CPostStorage &aStorage, TInt64 aNode, 
				 CPostStorage::TSortBy aSort, CPostStorage::TOrder aOrder, CAknView * aView,
				 TBool aStandAlone) : 
				CContextMediaAppGeneralContainer(aCMNetwork, aStorage, aNode, aSort, aOrder, aView),
					iStandAlone(aStandAlone) { }

TBool CContextMediaAppListboxContainer::IsCurrentPlayableMedia()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("IsCurrentPlayableMedia"));

	return EFalse;
}

TBool CContextMediaAppListboxContainer::IsLoadingMedia()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("IsLoadingMedia"));

	return EFalse;
}

void CContextMediaAppListboxContainer::expired(CBase * /*Source*/)
{
	if (iListbox) {
		iListbox->HandleItemAdditionL();
		iListbox->DrawNow();
	}
}

void CContextMediaAppListboxContainer::ConstructL(const TRect& aRect)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("ConstructL"));

	BaseConstructL();
	CreateWindowL();

	iTimer = CTimeOut::NewL(*this);

	{
#ifdef USE_SKIN
		TRect rect=aRect;
		rect.Move( 0, -rect.iTl.iY );
		iBackground=CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain,
			rect, EFalse );
#endif
	}

	// Bufs from resource
	iNoTitleBuf = CEikonEnv::Static()->AllocReadResourceL(R_NO_TITLE);
	iNoItemBuf = CEikonEnv::Static()->AllocReadResourceL(R_NO_ITEM);
	iNoVisibleItemBuf = CEikonEnv::Static()->AllocReadResourceL(R_NO_ITEM_VISIBLE);
	iNewThreadBuf = CEikonEnv::Static()->AllocReadResourceL(R_NEW_THREAD);
        
	// Create array and necessary additions
	CContextMediaArray::TAdditionalItem aItem = CContextMediaArray::ENone;
	if (iView->Id()==KThreadViewId) {
		aItem = CContextMediaArray::EAddReply;
	} else if (iView->Id()==KThreadsByDatePromptId){
		aItem = CContextMediaArray::EUseVCode;
	}
	iMediaTextArray = CContextMediaArray::NewL(iCMNetwork, iStorage, iNode, iSort, iOrder, *this, aItem,
		iStandAlone);

	if (!iStandAlone) {

		if (iView->Id()==KThreadsByDatePromptId) {
			auto_ptr<HBufC> t(iEikonEnv->AllocReadResourceL(R_SELECT_THREAD));
			StatusPaneUtils::SetTitlePaneTextL(*t);
		} else if (iNode != CPostStorage::RootId()) {
			TInt count = ((MDesCArray*)iMediaTextArray)->MdcaCount();
			CCMPost * aPost =0;
			if (count) aPost = iMediaTextArray->GetPostAt(count-1);

			if (aPost) {
				if (aPost->iBodyText->Value().Length()==0) {
					StatusPaneUtils::SetTitlePaneTextL(*iNoTitleBuf);
				} else {
					StatusPaneUtils::SetTitlePaneTextL(aPost->iBodyText->Value());
				}
				//CGulIcon * icon = iStorage.IconArray()->At(aPost->GetThumbnailIndex());
				//CFbsBitmap * aBitmap = new (ELeave) CFbsBitmap;
				//aBitmap->Duplicate(icon->Bitmap()->Handle());
				//cp->SetPicture(aBitmap);
			} else {
				// New thread!
				//CGulIcon * icon = iStorage.IconArray()->At(KNewThreadIconIndex); 
				//CFbsBitmap * aBitmap = new (ELeave) CFbsBitmap;
				//aBitmap->Duplicate(icon->Bitmap()->Handle());
				//cp->SetPicture(aBitmap);
				StatusPaneUtils::SetTitlePaneTextL(*iNewThreadBuf);
			}
		} else {
			//cp->SetPictureToDefaultL();
			auto_ptr<HBufC> t(iEikonEnv->AllocReadResourceL(R_TITLE));
			StatusPaneUtils::SetTitlePaneTextL(*t);
		}
	}

	iListbox = new (ELeave) CContextMediaBox(iMediaTextArray, iStandAlone);
	//iListbox->SetMopParent(this);
	//iListbox->SetContainerWindowL(*this);
	iListbox->ConstructL(this, EAknListBoxSelectionList|EAknListBoxLoopScrolling );

	TSize itemSize = TJuikLayoutItem(aRect).Combine( Layout().GetLayoutItemL( LG_medialistbox, LI_medialistbox__itemsize ) ).Size();
	iListbox->SetItemHeightL( itemSize.iHeight );
	iListbox->View()->SetMatcherCursor(EFalse);
	iListbox->Model()->SetItemTextArray(iMediaTextArray);



	iListbox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	iListbox->SetListBoxObserver(this);
	iListbox->CreateScrollBarFrameL(ETrue);
	iListbox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);

	iNoItemLabel = new (ELeave) CEikLabel;
	iNoItemLabel->SetPosition( TPoint(itemSize.iHeight, itemSize.iHeight) );

	SetRect(aRect);
	iListbox->ItemDrawer()->FormattedCellData()->SetIconArray(iStorage.IconArray());

	ActivateL();
}


void CContextMediaAppListboxContainer::PostEvent(CCMPost* /*aParent*/, CCMPost* aChild, TEvent aEvent)
{
	if (aEvent==EChildAdded) {
		TInt count = iMediaTextArray->MdcaCount();
		if ((iView->Id()==KThreadViewId) && (aChild==iMediaTextArray->GetPostAt(count-1))) {
			CLocalNotifyWindow* tp=CLocalNotifyWindow::Global();
			if (aChild->iBodyText->Value().Length()==0) {
				StatusPaneUtils::SetTitlePaneTextL(*iNoTitleBuf);
			} else {
				StatusPaneUtils::SetTitlePaneTextL(aChild->iBodyText->Value());
			}
			iListbox->HandleItemAdditionL();
			iListbox->DrawNow();
		} else {
			iTimer->Wait(1);
		}		
	} else if ((aEvent==EPostVisible) || (aEvent==EPostUpdated) || (aEvent==ELastPostChanged)) {
		iTimer->Wait(1);
	} else if (aEvent==EThumbnailLoaded ) {
		TInt count = iMediaTextArray->MdcaCount();
		if ((iView->Id()==KThreadViewId) && (aChild==iMediaTextArray->GetPostAt(count-1))) {
			if (!iStandAlone) {
				//change current status pane bitmap
				CGulIcon * icon = iStorage.IconArray()->At(aChild->GetThumbnailIndex());
				CFbsBitmap * aBitmap = new (ELeave) CFbsBitmap;
				aBitmap->Duplicate(icon->Bitmap()->Handle());
				CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
				CAknContextPane *cp =(CAknContextPane*)sp->ControlL(TUid::Uid(EEikStatusPaneUidContext));
				cp->SetPicture(aBitmap);
				cp->DrawNow();
			}
		} 
		iListbox->DrawNow();
	} else {
		iListbox->DrawDeferred();
	}
}


CContextMediaAppListboxContainer::~CContextMediaAppListboxContainer()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("~CContextMediaAppListboxContainer"));
	
	delete iTimer;

	if (iView->Id()==KThreadsByDatePromptId) {
		CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
		CAknNavigationControlContainer *np =0;

		if (sp) np=(CAknNavigationControlContainer *)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi)); 
		if (np) np->Pop(NULL);

	} else if (iNode != CPostStorage::RootId()) {
		CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
		CAknContextPane *cp =0;
		CAknNavigationControlContainer* np =0;
		if (sp) cp = (CAknContextPane*)sp->ControlL(TUid::Uid(EEikStatusPaneUidContext));
		if (cp) cp->SetPictureToDefaultL();
		
		if (sp) np = (CAknNavigationControlContainer *)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
		if (np) np->Pop(NULL);
		
	}
	if (iListbox) {
		iListbox->MakeVisible(EFalse);
		iListbox->ItemDrawer()->FormattedCellData()->SetIconArray(0);
	}
	
	delete iListbox;
	delete iNoItemLabel;

	delete iMediaTextArray;
	
	delete iNoTitleBuf;
	delete iNoItemBuf;
	delete iNoVisibleItemBuf;
	delete iNewThreadBuf;
#ifdef USE_SKIN
	delete iBackground;
#endif
}

void CContextMediaAppListboxContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("SizeChanged"));

	if (iListbox) {
		TRect lb_rect=Rect();
		iListbox->AdjustRectHeightToWholeNumberOfItems(lb_rect);
		iListbox->SetRect( lb_rect );
	}
}

TInt CContextMediaAppListboxContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("CountComponentControls"));

	return 2; 
}

CCoeControl* CContextMediaAppListboxContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("ComponentControl"));

	switch ( aIndex ) {
		case 0:	 return iListbox;
		case 1:  return iNoItemLabel;
		default: return NULL;
        }
}

void CContextMediaAppListboxContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("Draw"));


	// handling visibility of listbox and default empty text
	if (iListbox->Model()->NumberOfItems() == 0) {
		if (iStorage.HasHiddenThreads()) {
			iNoItemLabel->SetTextL(*iNoVisibleItemBuf);
		} else {
			iNoItemLabel->SetTextL(*iNoItemBuf);
		}
		iNoItemLabel->SetSize( iNoItemLabel->MinimumSize());
		TRect r = Rect();
		iNoItemLabel->SetPosition( TPoint( (r.Width() - iNoItemLabel->Size().iWidth)/2, 50) );
	} else {
		iNoItemLabel->SetSize( TSize(0,0) );
	}
	
	CWindowGc& gc = SystemGc();

#ifndef USE_SKIN
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
#else
	AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, gc, aRect );
#endif
}

void CContextMediaAppListboxContainer::HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("HandleListBoxEventL"));

	if(aListBox == iListbox) {
		switch(aEventType) {
			case EEventEnterKeyPressed: {
				if (get_current_post()) {
					iView->HandleCommandL(EcontextmediaappCmdOpenItem);
				}
				break;
			}
			default:
				break;
		}
	}
}

void CContextMediaAppGeneralContainer::mark_as_read()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppGeneralContainer"), _CL("mark_as_read"));

	CCMPost * aPost = get_current_post();
	if (aPost) iStorage.MarkAsRead(aPost);
}

TKeyResponse CContextMediaAppListboxContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("OfferKeyEventL"));

	if ((aKeyEvent.iCode == EKeyLeftArrow )|| (aKeyEvent.iCode == EKeyRightArrow )) {
		if (iView->Id()==KThreadsByDatePromptId) {
			return EKeyWasConsumed;
		} else {
			return EKeyWasNotConsumed;
		}
	}

	if ( iListbox ) {
                if ((aKeyEvent.iCode ==EKeyOK) && !get_current_post() ) {
			if (iView->Id()==KThreadsByDatePromptId) {
				iView->HandleCommandL(EAknSoftkeySelect);
				return EKeyWasConsumed;
			} else {
				if (iListbox->Model()->NumberOfItems()!=0) iView->HandleCommandL(EcontextmediaappCmdReply);
				return EKeyWasConsumed;
			}
		} else if (aKeyEvent.iCode == EKeyYes){
			if (iView->Id()==KThreadViewId) iView->HandleCommandL(EcontextmediaappCmdReply);
			return EKeyWasConsumed;
		} else {
			return iListbox->OfferKeyEventL(aKeyEvent, aType);
		}
	} else {
		return EKeyWasNotConsumed;
        }
}

TInt CContextMediaAppListboxContainer::get_current_idx()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("get_current_idx"));

	if (iListbox->Model()->NumberOfItems()) {
		return iListbox->CurrentItemIndex();
	}
	return -1;
}

TInt CContextMediaAppListboxContainer::get_top_idx()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("get_top_idx"));

	if (this->iListbox->Model()->NumberOfItems()) {
		return iListbox->TopItemIndex();
	}
	return -1;
}

TInt CContextMediaAppListboxContainer::get_item_count()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("get_item_count"));

	return iListbox->Model()->NumberOfItems();
}


void CContextMediaAppListboxContainer::set_current_idx(TInt idx)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("set_current_idx"));

	if (iListbox && (idx!=-1) && (iListbox->Model()->NumberOfItems()>idx)) {
		iListbox->SetCurrentItemIndex(idx);
	}
}

void CContextMediaAppListboxContainer::set_top_idx(TInt idx)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("set_top_idx"));

	if (iListbox && (idx!=-1) && (iListbox->Model()->NumberOfItems()>idx)) {
		iListbox->SetTopItemIndex(idx);
	}
}


CCMPost * CContextMediaAppListboxContainer::get_current_post()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppListboxContainer"), _CL("get_current_post"));

	TInt idx = get_current_idx();
	
	if (idx>-1) return iMediaTextArray->GetPostAt(idx);
	return 0;
}

//_---------------------------------------------------------------------------------
//_---------------------------------------------------------------------------------
//_---------------------------------------------------------------------------------
//_---------------------------------------------------------------------------------
//_---------------------------------------------------------------------------------

void CContextMediaAppGeneralContainer::CtrlSetRect(CCoeControl* aControl, const TRect& aRect) const
{
	aControl->SetRect(aRect);
}

void CContextMediaAppGeneralContainer::ScaleRect(TRect& aTo, const TRect& aFrom) const
{
	aTo=aFrom;
}

void CContextMediaAppGeneralContainer::CtrlSetSize(CCoeControl* aControl, const TSize& aSize) const
{
	aControl->SetSize(aSize);
}
void CContextMediaAppGeneralContainer::CtrlSetExtent(CCoeControl* aControl, const TPoint& aTl, const TSize& aSize) const
{
	CtrlSetPosition(aControl, aTl);
	CtrlSetSize(aControl, aSize);
}
void CContextMediaAppGeneralContainer::CtrlSetPosition(CCoeControl* aControl, const TPoint& aTl) const
{
	aControl->SetPosition(aTl);
}

void CContextMediaAppGeneralContainer::ScaleRect(TSize& aTo, const TSize& aFrom) const
{
	aTo=aFrom;
}

EXPORT_C CContextMediaAppPostContainer::CContextMediaAppPostContainer(CCMNetwork &aCMNetwork, CPostStorage &aStorage, TInt64 aNode, 
				 CPostStorage::TSortBy aSort, CPostStorage::TOrder aOrder, CAknView * aView, 
				TBool aStandAlone, CTagStorage* aTagStorage,
				 const TDesC& aFileName, MBBDataFactory * aFactory,
				 CAknIconArray *aTagIcons, MAskForNames* aAskForNames) : 
CContextMediaAppGeneralContainer(aCMNetwork, aStorage, aNode,aSort,aOrder,aView), iFileName(aFileName), 
	iFactory(aFactory), iStandAlone(aStandAlone), iTagStorage(aTagStorage), iTagIcons(aTagIcons),
	iAskForNames(aAskForNames) {}

const CFont* CContextMediaAppGeneralContainer::Latin12()
{
	return JuikFonts::Tiny();
}

void CContextMediaAppPostContainer::ConstructL(const TRect& aRect )
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("ConstructL"));

	BaseConstructL();
	
	iLoadingMedia=EFalse;
	iPlayingMedia=EFalse;
	iMediaReady=EFalse;
	iMediaBrowsingOn = (iFileName.Length()==0);

	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("ConstructL1"));
		iVideoEngine = CVideoEngine::NewL();
		iAudioEngine = CAudioEngine::NewL();
		iDisplayTimeOut = CTimeOut::NewL(*this, CActive::EPriorityIdle);
		iVolumeDisplayTimeOut = CTimeOut::NewL(*this);
		iPostArray = new (ELeave) CArrayFixFlat<TUint>(50);
		iControls=new (ELeave) CArrayPtrFlat< CCoeControl >(10);
	}

// array of post ids
	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("get_posts"));
		if (iView->Id()==KPostViewId) {
			TBool ok= EFalse;
			if (iNode != iStorage.RootId() ) {
				CC_TRAP(iLoadError, iPost = iStorage.GetByPostIdL(this, iNode)); 
				if (iLoadError==KErrNone) {
					iParentPost = iStorage.GetByPostIdL(this, iPost->iParentId());
					ok = iStorage.FirstL(iPost->iParentId(), iSort, iOrder, EFalse);
				}
			} else {
				iPost=iParentPost=0;
				ok=iStorage.FirstAllL();
				if (ok) iPost=iStorage.GetCurrentL(this);
			}
			int i=0;
			while (ok) {
				TUint idx=iStorage.GetCurrentIndexL();
				iPostArray->AppendL(idx);
				if (idx == iPost->iLocalDatabaseId()) {
					iCurrentPostIndex = i;
				}
				i++;
				ok = iStorage.NextL();
			}
		} else {
			if (iMediaBrowsingOn) {
				// we get posts from the thread mediapool
				TBool ok = iStorage.FirstL(CPostStorage::MediaPoolId(), iSort, iOrder, ETrue /*important!*/);
				while (ok) {
					TUint idx=iStorage.GetCurrentIndexL();
					iPostArray->AppendL(idx);
					ok = iStorage.NextL();
				}
			} else {

				// we create a single post from the given iFileName
				TTime t; t=GetTime();
				auto_ptr<CFbsBitmap> bm;
				refcounted_ptr<CCMPost>  aPost(CCMPost::NewL(iFactory, 0));
				aPost->iParentId = CPostStorage::MediaPoolId(); 
				aPost->iMediaUrl = iFileName; 
				aPost->iMediaFileName = iFileName;
				TBuf<40> mime; GetMimeTypeL(iFileName, mime);
				aPost->iContentType() = mime;
				aPost->iPostId = CPostStorage::AddMeToMediaPoolId();
				aPost->iUnreadCounter = 1; // IMPORTANT, cause a read post doesn't appear in the list
				aPost->iTimeStamp = t;

				aPost->iPresence.SetValue(iPresence); iPresence=0;

				TBuf<100> buf;
				if ( Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, buf ) && buf.Length() != 0) {
					aPost->iSender.iName().Append(buf);
				}
				if ( Settings().GetSettingL(SETTING_UPLOAD_TAG, buf) ) {
					aPost->iTag->Append(buf);
				}
				if ( Settings().GetSettingL(SETTING_JABBER_NICK, buf ) && buf.Length() != 0) {
					aPost->iSender.iJabberNick().Append(buf);
				}
				if ( Settings().GetSettingL(SETTING_PHONENO, buf ) && buf.Length() != 0) {
					aPost->iSender.iPhoneNo().Append(buf);
				}
				TInt sett;
				if ( Settings().GetSettingL(SETTING_PUBLISH_SHARING, sett) ) {
					aPost->iSharing()=sett;
				}
				if ( Settings().GetSettingL(SETTING_PUBLISH_AUTOTAGS, sett) ) {
					aPost->iIncludedTagsBitField()=sett;
				}
#ifndef __S60V3__
				GetIMEI(aPost->iSender.iImei()); 
#endif
				// FIX ME: BT ADDR HERE
				iStorage.AddLocalL(aPost.get(), bm);

				TBool ok = iStorage.FirstL(CPostStorage::MediaPoolId(), iSort, iOrder, ETrue);
				if (ok) {
					TUint idx=iStorage.GetCurrentIndexL();
					iPostArray->AppendL(idx);
				}
			}
			iCurrentPostIndex=0;
			iPost=iStorage.GetByIndexL( this, (*iPostArray)[iCurrentPostIndex] );
		}
	}

// BEGIN: Graphic components-----------------------------------------------
	CreateWindowL();

	TBool show_autotags=ETrue;
	Settings().GetSettingL(SETTING_SHOW_AUTOTAGS, show_autotags);
	TBool show_tags=ETrue;
	Settings().GetSettingL(SETTING_SHOW_TAGS, show_tags);
	if (iView->Id()!=KPostViewId) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("hintbox"));
		TInt scale = 1;
		iHints=CHintBox::NewL( KMediaContainerHintTuple, scale);
	}
	if (iStandAlone && show_autotags) {
		{
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("autotags1"));

			iAutoTagArray=CAutoTagArray::NewL(iAskForNames, iHints);
		}
		{
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("autotags2"));
			iAutoTags=CAutoTagListBox::NewL(iAutoTagArray, this, iTagIcons, iHints);
		}
		{
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("autotags3"));
			iAutoTags->ActivateL();
		}
		{
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("autotags4"));
			ShowAutoTags(EFalse, aRect);
		}
	}


	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("resources"));

		iLoadingBuf = iEikonEnv->AllocReadResourceL(R_LOADING);
		iDownloadingBuf = iEikonEnv->AllocReadResourceL(R_DOWNLOADING);
		iDefaultCommentBuf = iEikonEnv->AllocReadResourceL(R_DEFAULT_COMMENT);
		iDefaultSignatureBuf = iEikonEnv->AllocReadResourceL(R_DEFAULT_SIGNATURE);
		iDefaultTagsBuf = iEikonEnv->AllocReadResourceL(R_TAGS_DEFAULT);
	}
	
	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("controls"));

		iSBFrame = new (ELeave) CEikScrollBarFrame(this, 0, ETrue);
		iSBFrame->SetScrollBarVisibilityL(CEikScrollBarFrame::EOn, CEikScrollBarFrame::EOn);
#ifndef __S60V3__
		iSBFrame->SetScrollBarManagement(CEikScrollBar::EVertical, CEikScrollBarFrame::EFloating);
#endif

		{
			iLeftArrow = new (ELeave) CBorderedContainer();
			iLeftArrow->SetContainerWindowL(*this);
			iLeftArrow->SetContentL( new (ELeave) CArrowControl( CArrowControl::ELeftArrow ) );		
			iLeftArrow->SetFocusing(EFalse);
		}

		{
			iRightArrow = new (ELeave) CBorderedContainer();
			iRightArrow->SetContainerWindowL(*this);
			iRightArrow->SetContentL( new (ELeave) CArrowControl( CArrowControl::ERightArrow ) );		
			iRightArrow->SetFocusing(EFalse);
		}

		{
			iTimeBox = new (ELeave) CBorderedContainer();
			iTimeBox->SetContainerWindowL(*this); 
			iTimeLabel = new (ELeave) CEikLabel;

			iTimeBox->SetContentL( iTimeLabel );
			
			iTimeLabel->SetFont(Latin12());
			iTimeLabel->SetAlignment(TGulAlignment(EHCenterVCenter));
			iTimeLabel->OverrideColorL(EColorLabelText, KTextColor);

			iTimeLabel->SetAllMarginsTo(0);

			iTimeBox->SetFocusing(EFalse);
		}
		
		
		{
			iCountBox = new (ELeave) CBorderedContainer();
			iCountBox->SetContainerWindowL(*this); 
			iCountLabel = new (ELeave) CEikLabel;
			iCountBox->SetContentL( iCountLabel );
			
			iCountLabel->SetFont(Latin12());
			iCountLabel->SetAlignment(TGulAlignment(EHCenterVCenter));
			iCountLabel->OverrideColorL(EColorLabelText, KTextColor);
			iCountLabel->SetAllMarginsTo(0);									

			iCountBox->SetFocusing(EFalse);
		}

		{
			iLoadingLabel=new (ELeave) CEikLabel;
			iLoadingLabel->SetContainerWindowL( *this );

			if (! iStandAlone) {
				iLoadingLabel->SetFont(LatinBold17());
				iLoadingLabel->SetAlignment(TGulAlignment(EHCenterVCenter));
			} else {
				iLoadingLabel->SetFont(Latin12());
				iLoadingLabel->SetAlignment(TGulAlignment(EHLeftVTop));
			}
			iLoadingLabel->SetEmphasis(CEikLabel::EFullEmphasis);
			iLoadingLabel->OverrideColorL(EColorLabelText, KTextColor);
			iLoadingLabel->OverrideColorL(EColorLabelHighlightFullEmphasis, 
										  KUnfocusedBgColor);
			iLoadingLabel->SetFocusing(EFalse);
		}

		{
			iErrorLabel=new (ELeave) CEikLabel;
			iErrorLabel->SetContainerWindowL( *this );
			iErrorLabel->SetFont(Latin12());
			iErrorLabel->SetFocusing(EFalse);
			iErrorLabel->MakeVisible(EFalse);
		}

		{
			iVolumeIndicator=new (ELeave) CEikLabel;
			iVolumeIndicator->SetContainerWindowL( *this );
			iVolumeIndicator->SetTextL(KNullDesC);
			iVolumeIndicator->SetFont(Latin12());
			iVolumeIndicator->SetAlignment(TGulAlignment(EHCenterVCenter));
			iVolumeIndicator->OverrideColorL(EColorLabelText, KRgbBlue);
			iVolumeIndicator->SetFocusing(EFalse);
		}
	}

	if (iView->Id()==KPostViewId) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("textcontrols1"));

		
		{ 
			iComment = new (ELeave) OurOwnRichTextEditor(0, this);
			iComment->ConstructL(this, KRgbBlack, 3, 1100, 0);
			iComment->SetContainerWindowL(*this);
			
			iComment->SetBackgroundColorL(KUnfocusedBgColor);
			iComment->SetReadOnly(ETrue);
			iComment->SetFocusing(EFalse);
		}
	} else {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("textcontrols2"));

		iLoadingLabel->SetFocusing(ETrue);
		iControls->AppendL(iLoadingLabel);
		TInt scale=1;

		iHints->AddHintL(ETagHint, 10, 
			CTextBox::ETopRight, TPoint(160*scale, 20*scale), 
			140*scale, R_HINT_TAGS);
		iHints->AddHintL(EClickTagsHint, 9, 
			CTextBox::ETopRight, TPoint(160*scale, 20*scale), 
			140*scale, R_HINT_CLICKTAG);
		iHints->AddHintL(EMoveUpToContextHint, 8, 
			CTextBox::ETopRight, TPoint(160*scale, 20*scale), 
			140*scale, R_HINT_MOVEUP);

		if (iAutoTags)
			iControls->AppendL(iAutoTags);

		{  
			iComment = new (ELeave) OurOwnRichTextEditor(iDefaultCommentBuf, this);
			iComment->SetAknEditorAllowedInputModes(EAknEditorAllInputModes);
			iComment->ConstructL(this, KRgbTags, 2, 1000, EAknEditorFlagDefault);
			iComment->SetContainerWindowL(*this);	
			iComment->SetBackgroundColorL(KUnfocusedBgColor);
		}

		if (! iStandAlone ) {
			iSignature = new (ELeave) OurOwnRichTextEditor(iDefaultSignatureBuf, this);
			iSignature->SetAknEditorAllowedInputModes(EAknEditorAllInputModes);
			iSignature->ConstructL(this, KRgbTags, 1, 50, 0);
			iSignature->SetContainerWindowL(*this);
			CtrlSetExtent(iSignature, TPoint(1,171), TSize(174, 16));
			iSignature->SetBackgroundColorL(KUnfocusedBgColor);
			iControls->AppendL(iComment);
			iControls->AppendL(iSignature);
		} else {
			if (show_tags) {
				iTags = new (ELeave) OurOwnRichTextEditor(iDefaultTagsBuf, this);
				iTags->SetAknEditorAllowedInputModes(EAknEditorAllInputModes);
				iTags->ConstructL(this, KRgbTags, 1, 100, 0);
				iTags->SetContainerWindowL(*this);
				iTags->SetBackgroundColorL(KUnfocusedBgColor);
				iControls->AppendL(iTags);
			}
			iControls->AppendL(iComment);
		}
	}

	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("selecttags"));
		iSelectTagLabel=new (ELeave) CEikLabel;
		iSelectTagLabel->SetContainerWindowL( *this );

		{
			TBuf<30> select_tags;
			iEikonEnv->ReadResourceAsDes16(select_tags, R_SELECT_TAGS);

			iSelectTagLabel->SetTextL(select_tags);
		}
		iSelectTagLabel->SetFont(iEikonEnv->NormalFont());
		iSelectTagLabel->SetAlignment(TGulAlignment(EHCenterVCenter));
		iSelectTagLabel->OverrideColorL(EColorLabelText, KRgbBlue);
		iSelectTagLabel->SetFocusing(EFalse);
		iSelectTagLabel->SetEmphasis(CEikLabel::EFullEmphasis);
		iSelectTagLabel->OverrideColorL(EColorLabelHighlightFullEmphasis, KUnfocusedBgColor);

		iSelectTagLabel->MakeVisible(EFalse);
	}

	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("notify"));

		iNotifyControl=new (ELeave) CNotifyWindowControl;
		iNotifyControl->ConstructL(this);
		iNotifyControl->SetContainerWindowL(*this);
		iControls->AppendL(iNotifyControl);
	}

// END: Graphic components-----------------------------------------------
	RestoreOriginalCBAL();

	SetRect(aRect);
	ActivateL();
	display_current();
}

void CContextMediaAppPostContainer::HandleEdwinEventL(CEikEdwin* /*aEdwin*/, TEdwinEvent /*aEventType*/) { }

CContextMediaAppPostContainer::~CContextMediaAppPostContainer()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("~CContextMediaAppPostContainer"));
	
	if (iLive) *iLive=0;

	delete iDisplayTimeOut;
	delete iVolumeDisplayTimeOut;
	delete iAutoTimer;
	delete iHints;

	if (iMediaDisplay)
		{
			iMediaDisplay->RemoveListener( *this );
		}
	
	if (!iLive) {
		delete iMediaDisplay;
		iMediaDisplay=0;
	}
	
	if (iVideoEngine) iVideoEngine->CloseController();
	if (iAudioEngine) iAudioEngine->CloseController();
	delete iAudioEngine;
	delete iVideoEngine;

	iStorage.Release(iPost, this); iPost=0;
	iStorage.Release(iParentPost, this); iParentPost=0;
	delete iPostArray;
	
	delete iLoadingLabel;
	delete iErrorLabel;

	delete iLeftArrow;
	delete iRightArrow;

	delete iTimeBox;
	
	delete iCountBox;

	delete iComment;
	delete iSignature;
	delete iTags;
	delete iVolumeIndicator;
	delete iSelectTagLabel;

	delete iControls;
	
	delete iLoadingBuf;
	delete iDownloadingBuf;
	delete iDefaultSignatureBuf;
	delete iDefaultCommentBuf;
	delete iDefaultTagsBuf;
	delete iNotifyControl;

	delete iAutoTags;
	delete iAutoTagArray;

	delete iSBFrame;
	delete iPresence;
}

void CContextMediaAppPostContainer::SetPresence(CBBPresence *aPresence)
{
	delete iPresence;
	iPresence=aPresence;
}
void CContextMediaAppPostContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("SizeChanged"));
	MJuikLayout& lay = Layout();

	TRect r = Rect();
	TJuikLayoutItem full( r );

	TJuikLayoutItem upper = full.Combine( lay.GetLayoutItemL( LG_mediaview, LI_mediaview__upper) );
	TJuikLayoutItem lower = full.Combine( lay.GetLayoutItemL( LG_mediaview, LI_mediaview__lower) );

	KPictureRectangle           = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper, LI_mediaviewupper__picture) ).Rect();
	KVideoRectangle             = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper, LI_mediaviewupper__video  ) ).Rect();
	
	KPictureSize                = KPictureRectangle.Size();
	KPictureSizeStandAlone      = KPictureRectangle.Size();
	KPictureRectangleStandAlone = KPictureRectangle;
	KVideoRectangle2            = KVideoRectangle;



	TJuikLayoutItem l;

	l = full.Combine( Layout().GetLayoutItemL( LG_mediaview,  LI_mediaview__indicators ) );
	iNotifyControl->SetRect( l.Rect() );	


	l = full.Combine( Layout().GetLayoutItemL( LG_mediapost,  LI_mediapost__selecttags ) );
	iSelectTagLabel->SetRect( l.Rect() );	
	
	
	// upper
	
	l = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper, LI_mediaviewupper__leftarrow ) );
	iLeftArrow->SetRect( l.Rect() );
	
	l = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper, LI_mediaviewupper__rightarrow ) );
	iRightArrow->SetRect( l.Rect() );
	
	l = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper, LI_mediaviewupper__timebox ) );
	iTimeBox->SetRect( l.Rect() );
	
	l = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper, LI_mediaviewupper__countbox ) );
	iCountBox->SetRect( l.Rect() );
	
	l = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper,  LI_mediaviewupper__loadinglabel ) );
	iLoadingLabel->SetRect( l.Rect() );
	
	l = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper,  LI_mediaviewupper__errorlabel ) );
	iErrorLabel->SetRect( l.Rect() );

	l = upper.Combine( Layout().GetLayoutItemL( LG_mediaviewupper,  LI_mediaviewupper__volumeindicator ) );
	iVolumeIndicator->SetRect( l.Rect() );
	
	// lower
	l = lower.Combine( Layout().GetLayoutItemL( LG_mediaviewlower,  LI_mediaviewlower__comment ) );
	iComment->SetRect( l.Rect() );	

	if ( iTags )
		{
			l = lower.Combine( Layout().GetLayoutItemL( LG_mediaviewlower,  LI_mediaviewlower__tags ) );
			iTags->SetRect( l.Rect() );	
		}	
}

TInt CContextMediaAppPostContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("CountComponentControls"));

	if (!iTags && !iSignature) return 12;
	return 13; 
}

void CContextMediaAppPostContainer::ShowAutoTags(TBool aDraw, const TRect& aRect)
{
	if (!iAutoTags) return;
	TPoint p(15, 0);
	TRect r( p, 
			 TSize(iAutoTags->Width(), 
				   iAutoTags->RowHeight()*iAutoTagArray->MdcaCount()+
				   iAutoTags->BorderWidth()*2));
	
	TJuikLayoutItem full( aRect );
	TJuikLayoutItem upper = full.Combine( Layout().GetLayoutItemL( LG_mediaview, LI_mediaview__upper) );
	TInt y = upper.Rect().iBr.iY - r.Height(); 
	r.Move(0,y);

	iAutoTags->SetRect(r);
	iAutoTags->MakeVisible(ETrue);
	iAutoTags->SetTopItemIndex(0);
	if (aDraw) iAutoTags->DrawNow();
	iAutoTagsIsVisible=ETrue;
}

void CContextMediaAppPostContainer::DrawMediaDisplay()
{
	if (iMediaDisplay && iMediaReady) {
		CWindowGc& gc = SystemGc();
#ifndef __S60V2__
		ActivateGc();

		if (iStandAlone) 
			gc.DrawBitmap(KPictureRectangleStandAlone, iMediaDisplay->Bitmap());
		else
			gc.DrawBitmap(KPictureRectangle, iMediaDisplay->Bitmap());

		DeactivateGc();
#else
		if (iStandAlone) 
			iMediaDisplay->DrawNow(KPictureRectangleStandAlone);
		else
			iMediaDisplay->DrawNow(KPictureRectangle);
#endif
	} else {
		iLoadingLabel->DrawNow();
	}

	if (iErrorLabel->IsVisible()) iErrorLabel->DrawNow();

	iLeftArrow->DrawNow();
	iRightArrow->DrawNow();
	iCountBox->DrawNow();
	iTimeBox->DrawNow();
	iNotifyControl->DrawNow();

}

void CContextMediaAppPostContainer::AnimateAutoTags(TBool aIsVisible, TBool aWithBackground)
{
	if (!iAutoTags) return;
	if (iAutoTagsIsVisible==aIsVisible) return;
	iAutoTagsIsVisible=aIsVisible;

	CWindowGc& gc = SystemGc();

	TInt steps, h;
	h=iAutoTags->RowHeight();
	steps=iAutoTagArray->MdcaCount()-1;

	TRgb clearedbg=KRgbWhite;
	if (iLoadingLabel->IsFocused() && iView->Id()!=KPostViewId) clearedbg=KFocusedBgColor;

	if (aIsVisible) {
		ActivateGc();
		for (int i=0; i<steps; i++) {
			TRect r=iAutoTags->Rect();
			r.Move(0, -h);
			r.Resize(0, h);
			iAutoTags->SetRect(r);
			DeactivateGc();
			iAutoTags->SetTopItemIndex(iAutoTagArray->MdcaCount()-1-(i+1));
			iAutoTags->DrawNow();
			ActivateGc();
			if (i%2==1) iEikonEnv->Flush();
		}
		if (steps%2==0) iEikonEnv->Flush();
		DeactivateGc();
	} else {
		for (int i=steps; i>0; i--) {
			TRect r=iAutoTags->Rect();
			TRect cleared=r;
			cleared.Resize(0, -(r.Height()-h));
			r.Move(0, h);
			r.Resize(0, -h);
			iAutoTags->SetRect(r);
			iAutoTags->SetTopItemIndex(iAutoTagArray->MdcaCount()-1-(i-1));
			ActivateGc();
			if (!aWithBackground &&
					iMediaReady && iMediaDisplay && iMediaDisplay->IsVisible()) {
				TRect source=cleared;
				source.Move(-KPictureRectangleStandAlone.iTl.iX, -KPictureRectangleStandAlone.iTl.iY);
#ifndef __S60V2__
				gc.DrawBitmap(cleared, iMediaDisplay->Bitmap(), source);
				DeactivateGc();
#else
				DeactivateGc();
				iMediaDisplay->DrawNow(cleared);
				// drawn outsid
#endif
			} else {
				gc.SetPenStyle(CGraphicsContext::ENullPen);
				gc.SetBrushColor(clearedbg);
				gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
				gc.DrawRect(cleared);
				DeactivateGc();
			}
			iAutoTags->DrawNow();
			if (i%2==0) iEikonEnv->Flush();
		}
		if (steps%2==1) iEikonEnv->Flush();
	}
}

CCoeControl* CContextMediaAppPostContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("ComponentControl"));

#ifdef __WINS__
	TBuf<30> msg=_L("**CONTROL ");
	msg.AppendNum(aIndex);
	RDebug::Print(msg);
#endif
	switch ( aIndex ) {
		case 0: return iLoadingLabel;
		case 1: 
			if (!iLive && iMediaDisplay) return iMediaDisplay;
			else return iLoadingLabel;
		case 2:	return iLeftArrow;
		case 3: return iRightArrow;
		case 4: return iTimeBox;
		case 5: return iCountBox;
		case 6: return iComment;
		case 7: return iNotifyControl;
	    case 8: return iVolumeIndicator;
		case 9: return iSelectTagLabel;
		case 10: if (iAutoTags) {
				return iAutoTags;
			 }
			return iVolumeIndicator;
		case 11: return iErrorLabel;
		case 12: {
			if (iSignature) return iSignature;
			else return iTags;
			 }
	
		default: return 0;
        }
}

void CContextMediaAppPostContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("Draw"));

	if ( (iView->Id()==KPostViewId) && 
		( 
			(!iAutoTags && iCurrentScrollPos==0) || 
			(iAutoTags && iCurrentScrollPos==1)
			) ) {
		//necessary to display the comment properly. Reset the cursor position to beginning of text
		//makes it readable!
		iComment->MoveCursorL(TCursorPosition::EFPageUp, EFalse); 
	}

	CWindowGc& gc = SystemGc();
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
		
	TRect re = iLoadingLabel->Rect();	
	gc.SetBrushColor(KRgbWhite);
	gc.DrawRect(re);

}

TKeyResponse CContextMediaAppPostContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("OfferKeyEventL"));

	if (aType!=EEventKey) iInPress=EFalse;

	//if (aType != EEventKey) return EKeyWasNotConsumed;

	/*if ( (aKeyEvent.iCode == EKeyCBA1) || (aKeyEvent.iCode == EKeyCBA2) ){
		if (iLoadingMedia) {
			return EKeyWasConsumed;
		} else {
			return EKeyWasNotConsumed;
		}
	}*/

	if (aType == EEventKey && aKeyEvent.iCode == EKeyYes){
		if (iView->Id()==KPostViewId) {
			if ((!iPlayingMedia)&& (!iLoadingMedia) && !iStandAlone) iView->HandleCommandL(EcontextmediaappCmdReply);
		} else {
			if (!iLoadingMedia) iView->HandleCommandL(EcontextmediaappCmdPublish);
		}
		return EKeyWasConsumed;
	}

	if (aType == EEventKey && aKeyEvent.iCode == EKeyUpArrow) {
		if (iPlayingMedia) {
			if (iMediaFileType==EAudio) {
				TInt vol = iAudioEngine->IncreaseVolume();
				ShowVolume(vol);
			} else if (iMediaFileType==EVideo) {
				TInt vol = iVideoEngine->IncreaseVolume();
				ShowVolume(vol);
			}
		} else {
			if (iView->Id()==KPostViewId) {
				if ( (!iAutoTags && iCurrentScrollPos>0) || iCurrentScrollPos>1) {
					iSBFrame->MoveThumbsBy(0, -1);
					iComment->MoveDisplayL(TCursorPosition::EFLineUp);
					iSBFrame->DrawScrollBarsNow();
					iCurrentScrollPos--;
				} else if (iAutoTags && iCurrentScrollPos==1) {
					iSBFrame->MoveThumbsBy(0, -1);
					AnimateAutoTags(ETrue);
					iSBFrame->DrawScrollBarsNow();
					iCurrentScrollPos--;
				}
			} else {
				if (iComment->IsFocused() ) {
					if (iComment->CursorPos() == 0) {			
						FocusPrevious();
					} else {
						iComment->MoveCursorL(TCursorPosition::EFLineUp, EFalse);
					}
				} else if (iTags && iTags->IsFocused()) {
					if (iTags->CursorPos() == 0) {			
						FocusPrevious();
					} else {
						iTags->MoveCursorL(TCursorPosition::EFLineUp, EFalse);
					}
				} else if (iSignature && iSignature->IsFocused()) {
					if (iSignature->CursorPos() == 0) {			
						FocusPrevious();
					} else {
						iSignature->MoveCursorL(TCursorPosition::EFLineUp, EFalse);
					}
				} else if (iAutoTags && iAutoTags->IsFocused()) {
					if (!iPost->iPresence() || iAutoTags->CurrentItemIndex() == iAutoTags->TopItemIndex()) {
						FocusPrevious();
					} else {
						iAutoTags->SetCurrentItemIndexAndDraw(iAutoTags->CurrentItemIndex()-1);
					}
				} else {
					FocusPrevious();
				}
			}
		}
		return EKeyWasConsumed;
	}

	if (aType == EEventKey && aKeyEvent.iCode == EKeyDownArrow) {
		if (iPlayingMedia) {
			if (iMediaFileType==EAudio) {
				TInt vol = iAudioEngine->DecreaseVolume();
				ShowVolume(vol);
			} else if (iMediaFileType==EVideo) {
				TInt vol = iVideoEngine->DecreaseVolume();
				ShowVolume(vol);
			}
		} else {
			if (iView->Id()==KPostViewId) {
				if (iAutoTags && iCurrentScrollPos==0) {
					iSBFrame->MoveThumbsBy(0, 1);
					AnimateAutoTags(EFalse);
					iSBFrame->DrawScrollBarsNow();
					iCurrentScrollPos++;
				} else if (iCurrentScrollPos+1<iMaxScrollPos) {
					iSBFrame->MoveThumbsBy(0, 1);
					iComment->MoveDisplayL(TCursorPosition::EFLineDown);
					iSBFrame->DrawScrollBarsNow();
					iCurrentScrollPos++;
				}
			} else {
				if (iComment->IsFocused() ) {
					if (iComment->CursorPos() == iComment->TextLength()) {			
						FocusNext();
					} else {
						iComment->MoveCursorL(TCursorPosition::EFLineDown, EFalse);
					}
				} else if (iTags && iTags->IsFocused()) {
					if (iTags->CursorPos() == iTags->TextLength()) {			
						FocusNext();
					} else {
						iTags->MoveCursorL(TCursorPosition::EFLineDown, EFalse);
					}
				} else if (iSignature && iSignature->IsFocused()) {
					if (iSignature->CursorPos() == iSignature->TextLength()) {			
						FocusNext();
					} else {
						iSignature->MoveCursorL(TCursorPosition::EFLineDown, EFalse);
					}
				} else if (iAutoTags && iAutoTags->IsFocused()) {
					if (iAutoTags->CurrentItemIndex() == iAutoTags->BottomItemIndex()) {
						FocusNext();
					} else {
						iAutoTags->SetCurrentItemIndexAndDraw(iAutoTags->CurrentItemIndex()+1);
					}
				} else {
					FocusNext();
				}
			}
			
		}
		return EKeyWasConsumed;
	}
	
	if (aType == EEventKey && aKeyEvent.iCode==EKeyLeftArrow) {
		if (iView->Id()==KPostViewId) {
                        if (!iLoadingMedia && !iPlayingMedia && has_previous()) {
				display_previous(aKeyEvent.iRepeats);
			}
		} else if (!iPlayingMedia) {
			if (iComment->IsFocused()) {
				return iComment->OfferKeyEventL(aKeyEvent, aType);
			} else if (iTags && iTags->IsFocused()) {
				return iTags->OfferKeyEventL(aKeyEvent, aType);
			} else if (iSignature && iSignature->IsFocused()) {
				return iSignature->OfferKeyEventL(aKeyEvent, aType);
			} else if (iMediaBrowsingOn && !iLoadingMedia && !iPlayingMedia && has_previous()) {
				display_previous(aKeyEvent.iRepeats);
			}
		}
		return EKeyWasConsumed;
	}

	if (aType == EEventKey && aKeyEvent.iCode==EKeyRightArrow) {
		if (iView->Id()==KPostViewId) {
			if (!iLoadingMedia && !iPlayingMedia && has_next()) display_next(aKeyEvent.iRepeats);
		} else if (!iPlayingMedia){
			if (iComment->IsFocused()) {
				return iComment->OfferKeyEventL(aKeyEvent, aType);
			} else if (iTags && iTags->IsFocused()) {
				return iTags->OfferKeyEventL(aKeyEvent, aType);
			} else if (iSignature && iSignature->IsFocused()) {
				return iSignature->OfferKeyEventL(aKeyEvent, aType);
			} else if (iMediaBrowsingOn && !iLoadingMedia && !iPlayingMedia && has_next()) {
				display_next(aKeyEvent.iRepeats);
			}
		}
		return EKeyWasConsumed;
	}

	if (	(iComment && iComment->IsFocused()) ||
		(iTags && iTags->IsFocused()) ||
		(iSignature && iSignature->IsFocused()) ) {
	} else {
		if ( aType == EEventKey && aKeyEvent.iCode == EKeyBackspace) {
			if (iPlayingMedia) {
				return EKeyWasConsumed;
			} else if (iAutoTags && iAutoTags->IsFocused() ) {
				return iAutoTags->OfferKeyEventL(aKeyEvent, aType);
			} else 	if (iView->Id()==KPostViewId) {
				return EKeyWasNotConsumed;
			} else {
				if (iLoadingLabel->IsFocused()){
					iView->HandleCommandL(EcontextmediaappCmdDelete);
					return EKeyWasConsumed;
				} else {
					return EKeyWasNotConsumed;
				}
			}
		}
	}

	if ( aType == EEventKey && (aKeyEvent.iCode == EKeyOK) && iTags && iTags->IsFocused() && iTagStorage) {
		TInt added_from;
		TCursorSelection sel=iTags->Selection();
		// if the selection extends beyond the new text length we get
		// an ETEXT-12 panic, so reset selection
		iTags->ClearSelectionL();
		iSelectTagLabel->MakeVisible(ETrue);
		iSelectTagLabel->DrawNow();
		if (iHints) iHints->DismissHint(EClickTagsHint);
		if ( SelectTagsFromListL(*iTagStorage, iTags->RichText(), added_from,
				sel.LowerPos(), sel.HigherPos() ) ) {
			iSelectTagLabel->MakeVisible(EFalse);
			iTags->HandleTextChangedL();
			iTags->SetCursorPosL(added_from, EFalse);
			iTags->SetCursorPosL(iTags->TextLength(), ETrue);
			DrawMediaDisplay();
		} else {
			iSelectTagLabel->MakeVisible(EFalse);
			if (sel.iCursorPos<iTags->TextLength() && sel.iAnchorPos<iTags->TextLength() ) {
				// with FEPs clearing the selection may delete text,
				// so we must be careful not to try to select text that is
				// not there
				iTags->SetSelectionL(sel.iCursorPos, sel.iAnchorPos);
			}
			DrawMediaDisplay();
		}
		return EKeyWasConsumed;
	}
	if ( aType == EEventKey && (aKeyEvent.iCode == EKeyOK || aKeyEvent.iCode==EKeyDelete) && iAutoTags && iAutoTags->IsFocused() ){
		return iAutoTags->OfferKeyEventL(aKeyEvent, aType);
	}
	if ( aType == EEventKey && (aKeyEvent.iCode == EKeyOK) && IsCurrentPlayableMedia()){
		if (iView->Id()==KPostViewId) {
			if (!iPlayingMedia) {
				Play();
			} else {
				if (iMediaFileType==EVideo) {
					if ( iVideoEngine->GetEngineState() == CVideoEngine::EPPlaying ) {
						Pause();
					} else if (iVideoEngine->GetEngineState() == CVideoEngine::EPPaused) {
						Resume();
					} 
				} else if (iMediaFileType==EAudio){
					if ( iAudioEngine->GetEngineState() == CAudioEngine::EPPlaying ) {
						Pause();
					} else if (iAudioEngine->GetEngineState() == CAudioEngine::EPPaused) {
						Resume();
					} 
				}
			}
		} else {
			if ( iLoadingLabel->IsFocused() ) {
				if (!iPlayingMedia) {
					Play();
				} else {
					if (iMediaFileType==EVideo) {
						if ( iVideoEngine->GetEngineState() == CVideoEngine::EPPlaying ) {
							Pause();
						} else if (iVideoEngine->GetEngineState() == CVideoEngine::EPPaused) {
							Resume();
						} 
					} else if (iMediaFileType==EAudio){
						if ( iAudioEngine->GetEngineState() == CAudioEngine::EPPlaying ) {
							Pause();
						} else if (iAudioEngine->GetEngineState() == CAudioEngine::EPPaused) {
							Resume();
						} 
					}
				}
			}

		}
		return EKeyWasConsumed;
	}

	if (iComment && iComment->IsFocused()) return iComment->OfferKeyEventL(aKeyEvent, aType);
	if (iTags && iTags->IsFocused()) return iTags->OfferKeyEventL(aKeyEvent, aType);
	if (iSignature && iSignature->IsFocused()) return iSignature->OfferKeyEventL(aKeyEvent, aType);
	return EKeyWasNotConsumed;
}

void CContextMediaAppPostContainer::PostEvent(CCMPost* aParent, CCMPost* aChild, TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("PostEvent"));

	TBool count_changed=EFalse;
	if (iCurrentPostIndex!=-1 && aChild->iLocalDatabaseId()==(*iPostArray)[iCurrentPostIndex]) {
		if ( (aEvent == EMediaLoaded) || (aEvent == EErrorUpdated) ) {
			if (DisplayPicture())
				DrawNow();
		}
		if (aEvent == EPostDeleted || aEvent == EParentChanged ) {
			if (has_previous()) iCurrentPostIndex--;
			else if (has_next()) iCurrentPostIndex++;
			else iCurrentPostIndex=-1;

			iCurrentChanged=ETrue;
		}
	} 
	if (aParent==iParentPost) {
		if (aEvent == EPostDeleted || aEvent == EParentChanged ) {
			int delete_from=-1;
			int count=iPostArray->Count();
			for (int i=0; i<count; i++) {
				if ( (*iPostArray)[i]==aChild->iLocalDatabaseId() ) {
					delete_from=i;
					break;
				}
			}
			if (delete_from!=-1) {
				count_changed=ETrue;
				for (int i=delete_from; i<count-1; i++) {
					(*iPostArray)[i]=(*iPostArray)[i+1];
				}
				iPostArray->Delete(count-1);
				if (iCurrentPostIndex>=delete_from) {
					--iCurrentPostIndex;
					iCurrentChanged=ETrue;
				}
			}
		}
	}
	if (iCurrentChanged) {
		iDisplayTimeOut->Wait(0);
	} else if (count_changed) {
		CC_TRAPD(err, SetCountTextL());
		iCountBox->DrawDeferred();
	}
}

TBool CContextMediaAppPostContainer::has_next()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("has_next"));

	return (iPostArray->Count() > 1);
}

TBool CContextMediaAppPostContainer::has_previous()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("has_previous"));

	return (iPostArray->Count() > 1);
}

TInt CContextMediaAppPostContainer::acceleration(TInt aRepeats)
{
	if (iInPress) {
		iRepeats++;
	} else {
		iRepeats=0;
	}
	iInPress=ETrue;
	if (iRepeats<2) return 1;
	if (iRepeats<4) return 2;
	if (iRepeats<6) return 5;
	return 10;
}

void CContextMediaAppPostContainer::display_next(TInt aRepeats)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_next"));

	if (iLive) *iLive=2;

	if (iView->Id()!=KPostViewId) {
		save_contribution_to_post(EFalse);
		iStorage.UpdatePostL(iPost); // save post to storage
	}

	if (iCurrentPostIndex == iPostArray->Count()-1) {
		iCurrentPostIndex=0;
	} else {
		TInt move_by=acceleration(aRepeats);
		iCurrentPostIndex+=move_by;
		if (iCurrentPostIndex>=iPostArray->Count()) iCurrentPostIndex=iPostArray->Count()-1;
	}

	iStorage.Release(iPost, this); iPost=0;
	CC_TRAP(iLoadError, iPost=iStorage.GetByIndexL( this, (*iPostArray)[iCurrentPostIndex] ));
	display_current();
	DrawNow();
}

void CContextMediaAppPostContainer::display_previous(TInt aRepeats)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_previous"));

	if (iLive) *iLive=2;

	if (iView->Id()!=KPostViewId) {
		save_contribution_to_post(EFalse);
		TRAPD(ignore, iStorage.UpdatePostL(iPost)); // save post to storage
	}
	if (iCurrentPostIndex==0) {
		iCurrentPostIndex=iPostArray->Count()-1;
	} else {
		TInt move_by=acceleration(aRepeats);
		iCurrentPostIndex-=move_by;
		if (iCurrentPostIndex<0) iCurrentPostIndex=0;
	}

	iStorage.Release(iPost, this); iPost=0;
	CC_TRAP(iLoadError, iPost=iStorage.GetByIndexL(this, (*iPostArray)[iCurrentPostIndex]));
	display_current();
	DrawNow();
}

void OurOwnRichTextEditor::Zero()
{
	RichText()->Reset();
	iShowingDefault=EFalse;
}

void OurOwnRichTextEditor::AppendParagraph(const TDesC& aText, TRgb aColor, CParaFormat::TAlignment aAlignment)
{
	if (aText.Length()==0) return;

	CParaFormat paraFormat;
	TParaFormatMask paraFormatMask;
	TFontSpec fontspec = iContainer->Latin12()->FontSpecInTwips();
	TCharFormat charFormat( fontspec.iTypeface.iName, fontspec.iHeight );
	TCharFormatMask charFormatMask;

	TInt pos=RichText()->DocumentLength();
	if (pos>0) {
		RichText()->InsertL(pos, CEditableText::EParagraphDelimiter);
		pos=RichText()->DocumentLength();
	}
	RichText()->InsertL(pos, aText);
	RichText()->GetParaFormatL(&paraFormat, paraFormatMask, pos, aText.Length());
	paraFormatMask.SetAttrib(EAttLineSpacingControl);
	paraFormatMask.SetAttrib(EAttLineSpacing);
	paraFormatMask.SetAttrib(EAttAlignment);
	paraFormat.iLineSpacingControl = CParaFormat::ELineSpacingExactlyInTwips;
	if (fontspec.iHeight < 150) {
		paraFormat.iLineSpacingInTwips=115;
	} else {
		paraFormat.iLineSpacingInTwips = fontspec.iHeight-20;
	}
	paraFormat.iHorizontalAlignment = aAlignment;
	RichText()->ApplyParaFormatL(&paraFormat, paraFormatMask, pos, aText.Length());
	charFormat.iFontPresentation.iTextColor=aColor;
	charFormatMask.SetAttrib(EAttColor);
	charFormatMask.SetAttrib(EAttFontTypeface);
	charFormatMask.SetAttrib(EAttFontHeight);
	RichText()->ApplyCharFormatL(charFormat, charFormatMask, pos, aText.Length());

	CEikRichTextEditor::HandleTextChangedL();
}

void CContextMediaAppPostContainer::SetCountTextL()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("SetCountTextL"));
	TBuf<40> count;
	if (iStandAlone) {
		if (iNode == iStorage.RootId()) {
			refcounted_ptr<CCMPost> root(iStorage.GetRootL(0));
			count.Append(root->iBodyText->Value().Left(20));
			count.Append(_L(":"));
		}
		if (iParentPost) {
			count.Append(iParentPost->iBodyText->Value().Left(20));
		}
		count.Append(_L(" "));
	}
	count.AppendFormat(_L("%d/%d"), iCurrentPostIndex+1, iPostArray->Count());
	iCountLabel->SetTextL(count);
}

void CContextMediaAppPostContainer::display_current(TInt aError)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current"));

	bool do_draw=false;
	if (iCurrentChanged) {
		if (iCurrentPostIndex<0) {
			iView->HandleCommandL(EAknSoftkeyBack);
			return;
		} else {
			iStorage.Release(iPost, this); iPost=0;
			CC_TRAP(iLoadError, iPost=iStorage.GetByIndexL(this, (*iPostArray)[iCurrentPostIndex]));
			do_draw=true;
		}
	}
	iErrorLabel->MakeVisible(EFalse);
	iCurrentChanged=EFalse;
	if (iLoadError!=KErrNone) {
		iErrorMessage=_L("Error loading data: ");
		iErrorMessage.AppendNum(iLoadError);
		iErrorLabel->SetTextL(iErrorMessage);
		iErrorLabel->MakeVisible(ETrue);
		iLoadingLabel->SetTextL(iErrorMessage);
		do_draw=ETrue;
	}
	/* get current post */
	CCMPost * aPost = iPost;
	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_reset"));
		/* reset visual components */
		if (iMediaDisplay) iMediaDisplay->MakeVisible(EFalse);
	}
	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_2"));
		if (aError==KErrNone) {
			if (iLoadError==KErrNone)
				iLoadingLabel->SetTextL(_L("Loading..."));
		} else if (aError==KErrNotFound) {
			iLoadingLabel->SetTextL(_L("Media not found"));
		} else {
			TBuf<30> msg=_L("Error getting media ");
			msg.AppendNum(aError);
			iLoadingLabel->SetTextL(msg);
		}
	}


	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_1"));
		if ( iPost && (iNode == CPostStorage::RootId() || iParentPost==0)) {
			iStorage.Release(iParentPost, this); iParentPost=0;
			CC_TRAPD(err, iParentPost=iStorage.GetByPostIdL( this, iPost->iParentId() ));
		} 
	}

	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_3"));
		if (iAutoTagArray) iAutoTagArray->SetPost(iPost);
		iMediaReady=EFalse;
		iGoneUpFromAutoTags=EFalse;

		SetCountTextL();

		iMediaReady = EFalse;
		iMediaFileType = EUnknown;
		iRightArrow->MakeVisible(has_next());
		iLeftArrow->MakeVisible(has_previous());
	}
	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_4"));

		if (!iStandAlone && iView->Id()==KPostViewId) {
			/* set previous view indices to reflect browsing movement */
			CAknViewAppUi * appui = (CAknViewAppUi *)(iEikonEnv->AppUi());
			CContextMediaAppThreadView * view = (CContextMediaAppThreadView *)(appui->View(KThreadViewId));
			if (view) {
				view->SetCurrentIndex(get_current_idx()+1);
				view->SetTopIndex(get_top_idx()+1);
			}
		}
	}

	/* update text */
	TInt lines=0;
	if (aPost) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_5"));
		
		{ // Set time 
			TBuf<5> time;
			TTime now=GetTime();
			TTime timestamp=ContextToLocal(aPost->iTimeStamp());
			if ((now.DateTime().Year()==timestamp.DateTime().Year()) 
				&& (now.DateTime().Month()==timestamp.DateTime().Month())
				&& (now.DateTime().Day()==timestamp.DateTime().Day()) ) {
				
				timestamp.FormatL(time, _L("%F%H:%T"));
			} else {
				//timestamp.FormatL(time, _L("%F%D/%M"));
				timestamp.FormatL(time, _L("%F%M-%D"));
			}
			iTimeLabel->SetTextL(time);
		}

		if (aPost->iErrorInfo) {
			iErrorLabel->SetTextL( (aPost->iErrorInfo->UserMessage()) );
			iErrorLabel->MakeVisible(ETrue);
		}

		if (iView->Id()==KPostViewId) {
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_5_1"));
			iComment->Zero();
			iComment->AppendParagraph(aPost->iTag->Value(), KRgbTags, CParaFormat::EJustifiedAlign);
			iComment->AppendParagraph(aPost->iBodyText->Value(), KRgbBlack, CParaFormat::EJustifiedAlign);
			if (!iStandAlone) {
				iComment->AppendParagraph(aPost->iSender.iName(), 
					KRgbBlue, CParaFormat::ERightAlign);
			}
		} else {
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_5_2"));
             		if (aPost->iBodyText->Value().Length()) {
				iComment->Zero();
				iComment->AppendParagraph(aPost->iBodyText->Value(), KRgbTags, 
					CParaFormat::EJustifiedAlign);
			} else {
				iComment->Reset();
			}

			if (! iStandAlone ) {
				if (aPost->iSender.iName().Length()) {
					iSignature->Zero();
					iSignature->AppendParagraph(aPost->iSender.iName(), 
						KRgbBlack, CParaFormat::EJustifiedAlign);
				} else {
					iSignature->Reset();
				}
			} else {
				if (iTags) {
					if (aPost->iTag->Value().Length()) {
						iTags->Zero();
						iTags->AppendParagraph(aPost->iTag->Value(), 
							KRgbBlack, CParaFormat::EJustifiedAlign);
					} else {
						iTags->Reset();
					}
				}
			}
		}
		iComment->HandleTextChangedL();
		if (iSignature) iSignature->HandleTextChangedL();
		if (iTags) iTags->HandleTextChangedL();
	} else {
		iTimeLabel->SetTextL(KNullDesC);
		if (iTags) {
			iTags->Reset();
			iTags->HandleTextChangedL();
		}
		if (iSignature) {
			iSignature->Reset();
			iSignature->HandleTextChangedL();
		}
		iComment->Zero();
		iComment->AppendParagraph(_L(" "), KRgbTags, CParaFormat::ELeftAlign);
		iComment->HandleTextChangedL();
	}
	
	// adjust up&down arrow indicators to scroll text, if necessary
	if (iView->Id()==KPostViewId) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_6"));
		/* little hack to know how many scrolling positions we should have for the scroll bars */
		iComment->MoveCursorL(TCursorPosition::EFPageUp, EFalse);
		while (iComment->CursorPos()!=iComment->TextLength()) {
			iComment->MoveCursorL(TCursorPosition::EFLineDown, EFalse);
			lines++;
		}
		
		// Handling of the textbox+up/down arrow below
		iCurrentScrollPos=0;
		iMaxScrollPos=lines-3; // 3lines displayed on screen
		if (iAutoTags) {
			iCurrentScrollPos=1;
			if (iMaxScrollPos<=0) iMaxScrollPos=1;
			else iMaxScrollPos++;
		}
		if (iMaxScrollPos>0) {
			iMaxScrollPos++;
			iModel = TEikScrollBarModel(iMaxScrollPos, 0, 0);
			iSBFrame->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
			iSBFrame->Tile(&iModel);
			if (iCurrentScrollPos) iSBFrame->MoveThumbsBy(0, iCurrentScrollPos);
		} else {
			iModel = TEikScrollBarModel(0, 0, 0);
			iSBFrame->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EOff);
			iSBFrame->Tile(&iModel);
		}
		iSBFrame->DrawScrollBarsNow();
	}

	// set default focus
	if (iView->Id()==KPostViewId) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_7_1"));
		iLoadingLabel->SetFocus(ETrue);
	} else {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_7_2"));
		int i=0;
		for(i=0; i<iControls->Count();i++) {
			CCoeControl* cntrl=iControls->At(i);
			cntrl->SetFocus(EFalse);
		}
	
		if (!iMediaBrowsingOn) {
			iComment->PrepareForFocusGainL();
			iComment->SetFocus(ETrue);
			iComment->OverrideColorL(EColorControlBackground, KFocusedBgColor);
			iComment->OverrideColorL(EColorLabelHighlightFullEmphasis, KFocusedBgColor);
		} else {
			iLoadingLabel->SetFocus(ETrue);
			iLoadingLabel->OverrideColorL(EColorControlBackground, KFocusedBgColor);
			iLoadingLabel->OverrideColorL(EColorLabelHighlightFullEmphasis, KFocusedBgColor);
		}
	}

	// make the display asynchronous but with no delay
	if (iPost) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("display_current_8"));
//#ifndef __S60V2__
		/*
		 * If we don't wait for that second, the N70 doesn't manage
		 * to rotate photos taken with the side button :-(. Even
		 * with the wait, it sometimes fails, but not as often.
		 */
		if (!iMediaBrowsingOn) iDisplayTimeOut->Wait(1);
		else iDisplayTimeOut->Wait(0);
		ShowAutoTags(EFalse, Rect());


/*#else
		ShowAutoTags(EFalse);
		DisplayPicture();
#endif*/
	} else {
		if (iAutoTags) iAutoTags->MakeVisible(EFalse);
	}
	if (do_draw) DrawNow();
}

TBool CContextMediaAppPostContainer::CloseCurrentMedia()
{
	if (iLive) return EFalse;
	delete iMediaDisplay; iMediaDisplay=0;
	CC_TRAPD(err, iLoadingLabel->SetTextL(_L("Closed for sending")));
	return ETrue;
}

#ifdef __S60V2__

TBool CContextMediaAppPostContainer::DisplayPicture()
{
	TInt err;
	CC_TRAP(err, DisplayPictureInnerL());
	if (err!=KErrNone) {
		TBuf<50> msg;
		TBool retry=EFalse;
		if (err!=KErrNotFound && err!=KErrPathNotFound) {
			msg=_L("Error displaying:\n");
			msg.AppendNum(err);
		} else {
			msg=_L("Media not\nfound");
		}
		if (err==KErrInUse) retry=ETrue;
		CC_TRAP(err, iLoadingLabel->SetTextL(msg));
		if (iAutoTags && !iAutoTags->IsFocused()) AnimateAutoTags(EFalse, ETrue);
		DrawDeferred();
		if (retry) iDisplayTimeOut->Wait(1);
	}
	return ETrue;
}

void CContextMediaAppPostContainer::DisplayPictureInnerL()
{
	if (!iPost) return;
	if (! iMediaDisplay ) {
// 		iMediaDisplay=new (ELeave) CJpegView;
// 		iMediaDisplay->ConstructL(this, KPictureRectangleStandAlone);
		TSize sz = KPictureRectangleStandAlone.Size();
		iMediaDisplay = CJuikPhoto::NewL( this, sz, sz );
		iMediaDisplay->SetDrawBackground( ETrue );
		iMediaDisplay->AddListenerL( *this );
		iMediaDisplay->SetRect( KPictureRectangleStandAlone );
		iMediaDisplay->ActivateL();
	}

	TFileName fn;
	if (iPost->iContentType().FindF(_L("image/"))!=KErrNotFound) {
		fn = iPost->iMediaFileName();
		iMediaFileType = EImage;
	} else if (iPost->iContentType().FindF(_L("video/"))!=KErrNotFound){
		GetVideoMbmPath(fn);
		iMediaFileType = EVideo;
	} else if (iPost->iContentType().FindF(_L("audio/"))!=KErrNotFound){
		GetAudioMbmPath(fn);
		iMediaFileType = EAudio;
	} else {
		GetUnknownMbmPath(fn);
		iMediaFileType = EUnknown;
	}
	iLoadingLabel->SetTextL(KNullDesC);
	iMediaDisplay->SetFileL(fn);
	iMediaDisplay->MakeVisible(ETrue);
	iMediaReady=ETrue;
	return;
}

#else
TBool CContextMediaAppPostContainer::DisplayPicture()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPicture"));

	TInt err;
	CC_TRAP(err, iLoadingLabel->SetTextL(_L("Loading....")));
	if (iLive) {
		iWaitCount++;
		TBuf<40> msg=_L("Loading...");
		msg.AppendNum(iWaitCount);
		if (!iAutoTags) CC_TRAP(err, iLoadingLabel->SetTextL(msg));
		DrawNow();
		if (iWaitCount>1)
			iDisplayTimeOut->Wait(1);
		else
			iDisplayTimeOut->Wait(0);
		return EFalse;
	}
	iWaitCount=0;
	if (!iAutoTags) DrawNow();

	TInt aLive=1; iLive=&aLive;

	CPAlbImageViewerBasic* localMediaDisplay=iMediaDisplay;
	iMediaDisplay=0;
	CC_TRAP(err, DisplayPictureInnerL(aLive, localMediaDisplay));
	if (err!=KErrNone && aLive==1) {
		iMediaDisplay=localMediaDisplay;
		if (iMediaDisplay) iMediaDisplay->MakeVisible(EFalse);
		TBuf<50> msg;
		TBool retry=EFalse;
		if (err!=KErrNotFound && err!=KErrPathNotFound) {
			msg=_L("Error displaying:\n");
			msg.AppendNum(err);
		} else {
			msg=_L("Media not\nfound");
		}
		if (err==KErrInUse) retry=ETrue;
		CC_TRAP(err, iLoadingLabel->SetTextL(msg));
		if (iAutoTags && !iAutoTags->IsFocused()) AnimateAutoTags(EFalse, ETrue);
		DrawDeferred();
		if (retry) iDisplayTimeOut->Wait(1);
	}

	TBool ret=ETrue;
	if (aLive!=1) {
		ret=EFalse;
		delete localMediaDisplay; localMediaDisplay=0;
		iMediaDisplay=0;
	} else {
		iMediaDisplay=localMediaDisplay;
	}
	iLive=0;

	if (err!=KErrNone) return EFalse;;
	return ret;
}

void CContextMediaAppPostContainer::DisplayPictureInnerL(TInt& aLive,
							 CPAlbImageViewerBasic* & localMediaDisplay)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL"));


	if (!iPost) return;

	//delete localMediaDisplay; localMediaDisplay=0;

	iMediaReady=EFalse;
	if (!localMediaDisplay) {
#if 1
		if (!iStandAlone) { 
			localMediaDisplay = CPAlbImageViewerBasic::NewL(this, KPictureRectangle);
		} else {
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_1"));
			localMediaDisplay = CPAlbImageViewerBasic::NewL(this, KPictureRectangleStandAlone);
		}
#else
		localMediaDisplay = CPAlbImageViewerBasic::NewL(this, KPictureRectangle);
#endif
	} else {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2"));
		localMediaDisplay->MakeVisible(EFalse);
		DrawNow();
	}

	//iLoadingMedia=ETrue;

	const CCMPost * aPost = iPost;
	TBuf<255> msg;

	if (iStandAlone) iLoadingLabel->SetTextL(_L(""));
	if (!aPost) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_1"));
		if (iView->Id()==KPostViewId) {
			msg.Append(_L("err: no post!\n")); //FIX ME: use resource
			iLoadingLabel->SetTextL(msg);
			iLoadingMedia=EFalse;
		} else {
			msg.Append(_L("No media found!\n")); // FIX ME: use resource
			iLoadingLabel->SetTextL(msg);
			iLoadingMedia=EFalse;
			CEikButtonGroupContainer * cba = CEikButtonGroupContainer::Current();
			if( cba) {
				cba->SetCommandSetL( R_AVKON_SOFTKEYS_BACK );
				cba->DrawNow();
			}
		}
		return;
	} else if (aPost->iErrorCode()!=0) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_2"));
		msg.AppendFormat(_L("err %d:\n"), aPost->iErrorCode());
		msg.Append(aPost->iErrorDescr());
		iLoadingLabel->SetTextL(msg);
	} else if (aPost->iMediaFileName().Length()==0) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_3"));
		msg.Append(*iDownloadingBuf);
		iLoadingLabel->SetTextL(msg);	
		iCMNetwork.FetchMedia(aPost);
	} else if (aPost->iMediaFileName().Length()!=0) {
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_4"));
		if (!iAutoTags) {
			iLoadingLabel->SetTextL(*iLoadingBuf);
			DrawNow();
		}
		TFileName fn;
		if (aPost->iContentType().FindF(_L("image/"))!=KErrNotFound) {
			fn = aPost->iMediaFileName();
			iMediaFileType = EImage;
		} else if (aPost->iContentType().FindF(_L("video/"))!=KErrNotFound){
			GetVideoMbmPath(fn);
			iMediaFileType = EVideo;
		} else if (aPost->iContentType().FindF(_L("audio/"))!=KErrNotFound){
			GetAudioMbmPath(fn);
			iMediaFileType = EAudio;
		} else {
			GetUnknownMbmPath(fn);
			iMediaFileType = EUnknown;
		}

		TInt err=KErrNone;
		if (!localMediaDisplay->IsBusy()) {
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_3"));
#ifdef __S60V2__
			localMediaDisplay->ReleaseImageLoader();
#endif
			localMediaDisplay->ReleaseMemoryAndVisibleBitmap();
			RDebug::Print(_L("Release mem, about to load"));
#ifdef __S60V2__
			// !Efficient loading of large bitmaps!
			localMediaDisplay->SetImageNameAndDisplaymodeL(fn, EColor64K);
			if (aLive==0) {
				CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_3_1"));
				delete localMediaDisplay; localMediaDisplay=0;
				return;
			}
			if (aLive==2) return;
			if (iStandAlone) {
				CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_3_2"));
				TSize s=localMediaDisplay->UnscaledSizeL();
				TInt zoom=0;
				/*
				 * We have to set the zoom before loading the image
				 * to get the quickest loading and lowest memory usage
				 */
				if (s.iWidth > s.iHeight) {
					if (s.iWidth >= 2*160 && s.iWidth < 4*160) {
						zoom=-2;
					} else if (s.iWidth >= 4*160 && s.iWidth < 8*160) {
						zoom=-4;
					} else if (s.iWidth >= 8*160) {
						zoom=-8;
					}
				} else {
					if (s.iHeight >= 2*120 && s.iHeight < 4*120) {
						zoom=-2;
					} else if (s.iHeight >= 4*120 && s.iHeight < 8*120) {
						zoom=-4;
					} else if (s.iHeight >= 8*120) {
						zoom=-8;
					}
				}
				while (localMediaDisplay->ZoomRatio() > zoom) {
					CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_3_3"));
					localMediaDisplay->ZoomOutL();
					if (aLive==0) {
						delete localMediaDisplay; localMediaDisplay=0;
					}
					if (aLive==0 || aLive==2) return;
				}
			}
			CC_TRAPD(errd, localMediaDisplay->LoadImageL());
#else 
			CC_TRAPD(errd, err=localMediaDisplay->LoadImageL(fn, EColor4K));
#endif
			if (aLive==0) {
				delete localMediaDisplay; localMediaDisplay=0;
				return;
			}
			if (aLive==2) return;
			if (errd) err=errd;
			if (err==KErrNone) {
				CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_3_4"));
#if 1
				if (!iStandAlone) {
					localMediaDisplay->FreeScaleL(KPictureSize, ETrue);

				} else {
					//localMediaDisplay->FreeScaleL(KPictureSizeStandAlone, ETrue);
					localMediaDisplay->ScaleOptimumL();
				}
#else
				localMediaDisplay->FreeScaleL(KPictureSize, EFalse);
#endif
			}
			if (aLive==0) {
				delete localMediaDisplay; localMediaDisplay=0;
				return;
			}
			if (aLive==2) return;
			{
				CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_3_5"));
#ifdef __S60V2__
				localMediaDisplay->ReleaseImageLoader();
#endif
			}
		} else {
			CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_2_4"));
			iDisplayTimeOut->Wait(1);
		}

		if (err!=KErrNone) {
			msg.AppendFormat(_L("Err %d:\n can't open\n"), err);
			msg.Append(fn);
			iLoadingLabel->SetTextL(msg);
		} else {
			if (!iStandAlone && iView->Id()==KPostViewId) {
				mark_as_read();
			}
			iMediaReady=ETrue;
			iLoadingLabel->SetTextL(KNullDesC);
		}
	}
	{
		CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("DisplayPictureL_3"));
		localMediaDisplay->MakeVisible(iMediaReady);
		MediaLoaded(iMediaReady);
	}
}

#endif

void CContextMediaAppPostContainer::MediaLoaded(TBool aMediaReady)
{
	iLoadingMedia=EFalse;
	if (iAutoTags && !iAutoTags->IsFocused()) AnimateAutoTags(EFalse, ETrue);
	if (iLoadingLabel->Text()->Length()>1) iLoadingLabel->MakeVisible(ETrue);
	DrawDeferred();
}

void CContextMediaAppPostContainer::expired(CBase * source)
{
	if (source==iDisplayTimeOut) {
		if (!iCurrentChanged) {
			DisplayPicture();
		} else {
			display_current();
		}
	} else if (source==iVolumeDisplayTimeOut) {
		iVolumeIndicator->SetTextL(KNullDesC);
		iVolumeIndicator->DrawDeferred();
	} else if (source==iAutoTimer) {
		if (iAutoTags->IsFocused()) return;
		AnimateAutoTags(EFalse);
	}
}

EXPORT_C void CContextMediaAppPostContainer::Play()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("Play"));

	if (!iMediaReady) return;
	if (!IsCurrentPlayableMedia()) return;
	CCMPost * aPost = iPost;

	CC_TRAPD(ignore, PlayMediaFileL(aPost->iMediaFileName());)
}

void CContextMediaAppPostContainer::PlayMediaFileL(const TDesC& aFilename)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("PlayMediaFileL"));

	if (!IsCurrentPlayableMedia()) return;
	iPlayingMedia = ETrue;

	iRightArrow->MakeVisible(EFalse);
	iLeftArrow->MakeVisible(EFalse);

	iTimeBox->MakeVisible(EFalse);
	iCountBox->MakeVisible(EFalse);

	TInt volume=0;
	Settings().GetSettingL(SETTING_PLAYBACK_VOLUME, volume);
	if (iMediaFileType==EVideo) {
		iMediaReady=EFalse;
		iMediaDisplay->MakeVisible(EFalse);
		iLoadingLabel->SetTextL(*iLoadingBuf);

		iMediaDisplay->DrawNow();
		iLoadingLabel->DrawNow();
		
		TRect r = KVideoRectangle2;
		iVideoEngine->SetNewFileL( aFilename );
		iVideoEngine->InitControllerL(this, ControlEnv()->WsSession(), 
						*(ControlEnv()->ScreenDevice()),
						Window(), r, r, volume);
	} else if (iMediaFileType==EAudio) {
		iAudioEngine->SetNewFileL(aFilename);
		iAudioEngine->InitControllerL(this, volume);
	} 
}

void CContextMediaAppPostContainer::PauseMediaFileL()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("PauseMediaFileL"));

	if (iMediaFileType==EVideo) {
		iVideoEngine->DoPauseL();
	} else if (iMediaFileType==EAudio){
		iAudioEngine->DoPauseL();
	}

	CEikButtonGroupContainer * cba = CEikButtonGroupContainer::Current();
	if( cba) {
		cba->SetCommandSetL( R_CONTINUE_STOP );
		cba->DrawNow();
	}
}

void CContextMediaAppPostContainer::StopMediaFileL()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("StopMediaFileL"));

	if (iMediaFileType==EVideo) {
		iVideoEngine->DoStopL();
	} else if (iMediaFileType==EAudio){
		iAudioEngine->DoStopL();
	}
}	

void CContextMediaAppPostContainer::ResumeMediaFileL()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("ResumeMediaFileL"));

	if (iMediaFileType==EVideo) {
		if ( iVideoEngine->GetEngineState() != CVideoEngine::EPPaused ) return;
		iVideoEngine->DoPlayL();
	} else if (iMediaFileType==EAudio){
		if ( iAudioEngine->GetEngineState() != CAudioEngine::EPPaused ) return;
		iAudioEngine->DoPlayL();
	}
	
	CEikButtonGroupContainer * cba = CEikButtonGroupContainer::Current();
	if( cba) {
		cba->SetCommandSetL( R_PAUSE_STOP );
		cba->DrawNow();
	}		
}

void CContextMediaAppPostContainer::PlayCompletedL(TInt aError)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("PlayCompletedL"));

	iRightArrow->MakeVisible(has_next());
	iLeftArrow->MakeVisible(has_previous());

	iTimeBox->MakeVisible(ETrue);
	iCountBox->MakeVisible(ETrue);
	iLoadingLabel->MakeVisible(ETrue);

	RestoreOriginalCBAL();
	iPlayingMedia=EFalse;

	if (aError != KErrNone) {
		TBuf<255> msg;
		msg.AppendFormat(_L("err %d: can't play\n"), aError);
		iLoadingLabel->SetTextL(msg);
		iMediaReady=EFalse;
	} else {
		iMediaReady=ETrue;
	}
	DrawNow();	
}

void CContextMediaAppPostContainer::InitControllerCompletedL(TInt aError)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("InitControllerCompletedL"));

	if (aError==KErrNone) {
		CEikButtonGroupContainer * cba = CEikButtonGroupContainer::Current();
		if( cba)  {
			cba->SetCommandSetL( R_PAUSE_STOP );
			cba->DrawNow();
		}

		if (iMediaFileType==EVideo) {
			iMediaDisplay->MakeVisible(EFalse);
			iLoadingLabel->MakeVisible(EFalse);
			iVideoEngine->DoPlayL();
		} else if (iMediaFileType==EAudio){
			iAudioEngine->DoPlayL();
		}
	} else {
		// FIX ME: popup warning note 'couldn't initialise'
	}
}

void CContextMediaAppPostContainer::RestoreOriginalCBAL()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("RestoreOriginalCBAL"));

	CEikButtonGroupContainer * cba = CEikButtonGroupContainer::Current();
	if (cba) {
		if (iView->Id()!=KPostViewId) {
			if (iMediaBrowsingOn && !iStandAlone) {
				cba->SetCommandSetL( R_AVKON_SOFTKEYS_SELECT_BACK );
			} else {
				if (!iStandAlone) {
					if (iNode != CPostStorage::MediaPoolId()) {
						cba->SetCommandSetL( R_CBA_POST_CANCEL);
					} else {
						cba->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK);
					}
				} else {
					if (!iMediaBrowsingOn) {
						cba->SetCommandSetL( R_CBA_PUBLISH_OPTIONS );
					} else {
						cba->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK);
					}
				}
			}
		} else {
			cba->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
		}
		cba->DrawNow();
	}
}

EXPORT_C void CContextMediaAppPostContainer::Pause()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("Pause"));

	CC_TRAPD(ignore, PauseMediaFileL();)
}

EXPORT_C void CContextMediaAppPostContainer::Resume()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("Resume"));

	CC_TRAPD(ignore, ResumeMediaFileL();)
}

EXPORT_C void CContextMediaAppPostContainer::Stop()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("Stop"));

	CC_TRAPD(ignore, StopMediaFileL();)
}

CCMPost * CContextMediaAppPostContainer::get_current_post()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("get_current_post"));

	return iPost; // here, no need to query the storage...
}

TInt CContextMediaAppPostContainer::get_current_idx()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("get_current_idx"));

	return iCurrentPostIndex;
}

TInt CContextMediaAppPostContainer::get_top_idx()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("get_top_idx"));

	if (iCurrentPostIndex<=2) {
		return 0;
	} else {
		return (iCurrentPostIndex-2);
	}
}

void CContextMediaAppPostContainer::set_current_idx(TInt idx)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("set_current_idx"));

	iCurrentPostIndex = idx;
}

void CContextMediaAppPostContainer::set_top_idx(TInt /*idx*/)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("set_top_idx"));

	// no impl
}

TBool CContextMediaAppPostContainer::IsCurrentPlayableMedia()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("IsCurrentPlayableMedia"));

	return ( (iMediaFileType==EVideo && iVideoEngine) || (iMediaFileType==EAudio) );
}

TInt CContextMediaAppPostContainer::get_item_count()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("get_item_count"));

	return 0;
}

void CContextMediaAppPostContainer::save_contribution_to_post(TBool aConfirmed)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppContainer2"), _CL("save_contribution_to_post"));

	CCMPost * aPost = get_current_post();
	if (aPost) {
		if (iComment) {
			CALLSTACKITEM_N(_CL("CContextMediaAppContainer2"), _CL("comment"));
			const TDesC& comment = iComment->RichText()->Read(0, iComment->TextLength());
			aPost->iBodyText->Zero();
			if (comment.CompareF(*iDefaultCommentBuf)!=0) {
				aPost->iBodyText->Append(comment);
			}
		}

		if (iSignature) {
			CALLSTACKITEM_N(_CL("CContextMediaAppContainer2"), _CL("signature"));
			const TDesC& sign = iSignature->RichText()->Read(0, iSignature->TextLength());
			if (sign.CompareF(*iDefaultSignatureBuf)!=0) {
				aPost->iSender.iName().Copy(sign);
				CC_TRAPD(ignore, Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR, sign));
			}
		}

		if (iAutoTagArray) {
			CALLSTACKITEM_N(_CL("CContextMediaAppContainer2"), _CL("autotags"));
			TInt ignore;
			if (aPost->iSharing() != iAutoTagArray->GetSharing()) {
				aPost->iSharing()=iAutoTagArray->GetSharing();
				CC_TRAPD(ignore, Settings().WriteSettingL(SETTING_PUBLISH_SHARING, aPost->iSharing()));
			}
			if (aPost->iIncludedTagsBitField()!=iAutoTagArray->GetIncludedTagsBitField()) {
				aPost->iIncludedTagsBitField()=iAutoTagArray->GetIncludedTagsBitField();
				CC_TRAP(ignore, Settings().WriteSettingL(SETTING_PUBLISH_AUTOTAGS, aPost->iIncludedTagsBitField()));
			}
		}
		if (iTags) {
			CALLSTACKITEM_N(_CL("CContextMediaAppContainer2"), _CL("tags"));
			const TDesC& tags = iTags->RichText()->Read(0, iTags->TextLength());
			if (tags.Locate(':')!=KErrNotFound && iHints) iHints->DismissHint(ETagHint);
			if (tags.CompareF(*iDefaultTagsBuf)!=0) {
				if (aPost->iTag->Value().Compare(tags)) {
					aPost->iTag->Zero();
					aPost->iTag->Append(tags);
					CC_TRAPD(ignore, Settings().WriteSettingL(SETTING_UPLOAD_TAG, tags));
				}
				if (iTagStorage) iTagStorage->AddFromStringAsNecessaryL(tags);
			} else if (aPost->iTag->Value().Length()>0) {
				aPost->iTag->Zero();
				CC_TRAPD(ignore, Settings().WriteSettingL(SETTING_UPLOAD_TAG, KNullDesC));
			}
		}
		if (!iStandAlone) {
			TTime t; t=GetTime();
			aPost->iTimeStamp() = t;
		}
	}
}

void CContextMediaAppPostContainer::ShowHintByControl(CCoeControl* aCurrentControl)
{
	if (!iHints) return;
	if (aCurrentControl==iTags) {
		iHints->ShowHintIfNotDismissed(ETagHint);
		iHints->ShowHintIfNotDismissed(EClickTagsHint);
		iHints->ShowHintIfNotDismissed(EMoveUpToContextHint);
	} else if (aCurrentControl==iAutoTags) {
		iHints->DismissHint(EMoveUpToContextHint);
	}
}

void CContextMediaAppPostContainer::FocusNext()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("FocusNext"));

	if (iHints) iHints->DontShow();

        for (int i=0;i<iControls->Count();i++) {
		if (iControls->At(i)->IsFocused()) {
			if (i+1<iControls->Count()) {
				iControls->At(i)->PrepareForFocusLossL();
				if (iAutoTags && iControls->At(i)==iAutoTags) {
					AnimateAutoTags(EFalse);
				} else {
					iControls->At(i)->OverrideColorL(EColorControlBackground, KUnfocusedBgColor);
					iControls->At(i)->OverrideColorL(EColorLabelHighlightFullEmphasis, KUnfocusedBgColor);
				}
				iControls->At(i)->SetFocus(EFalse);
				iControls->At(i)->DrawNow();
				if (iControls->At(i)==iLoadingLabel && iMediaReady) DrawMediaDisplay();
				iControls->At(i+1)->PrepareForFocusGainL();
				if (iAutoTags && iControls->At(i+1)==iAutoTags) {
					AnimateAutoTags(ETrue);
					if (iGoneUpFromAutoTags && iPost->iPresence()) iAutoTags->SetCurrentItemIndex(0);
					else iAutoTags->SetCurrentItemIndex(iAutoTagArray->MdcaCount()-1);
				} else {
					iControls->At(i+1)->OverrideColorL(EColorControlBackground, KFocusedBgColor);
					iControls->At(i+1)->OverrideColorL(EColorLabelHighlightFullEmphasis, KFocusedBgColor);
					iControls->At(i+1)->SetFocus(ETrue);
				}
				iControls->At(i+1)->SetFocus(ETrue);
				iControls->At(i+1)->DrawNow();
				
				iSBFrame->MoveThumbsBy(0, 1);
				iSBFrame->DrawScrollBarsNow();
				ShowHintByControl(iControls->At(i+1));

				return;
			}
		}
	}
}

void CContextMediaAppPostContainer::FocusPrevious()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppPostContainer"), _CL("FocusPrevious"));

	if (iHints) iHints->DontShow();

	for (int i=iControls->Count()-1;i>=0;i--) {
		if (iControls->At(i)->IsFocused()) {
			if ((i-1)>=0 ) {
				if (iAutoTags && iControls->At(i)==iAutoTags) {
					AnimateAutoTags(EFalse);
					iGoneUpFromAutoTags=ETrue;
				} else {
					iControls->At(i)->OverrideColorL(EColorControlBackground, KUnfocusedBgColor);
					iControls->At(i)->OverrideColorL(EColorLabelHighlightFullEmphasis, KUnfocusedBgColor);
					iControls->At(i)->PrepareForFocusLossL();
				}
				iControls->At(i)->SetFocus(EFalse);

				iControls->At(i-1)->MakeVisible(ETrue);
				iControls->At(i-1)->PrepareForFocusGainL();

				iControls->At(i-1)->SetFocus(ETrue);
				if (iAutoTags && iControls->At(i-1)==iAutoTags) {
					iAutoTags->SetCurrentItemIndex(iAutoTagArray->MdcaCount()-1);
					AnimateAutoTags(ETrue);
				} else {
					iControls->At(i-1)->OverrideColorL(EColorControlBackground, KFocusedBgColor);
					iControls->At(i-1)->OverrideColorL(EColorLabelHighlightFullEmphasis, KFocusedBgColor);
					iControls->At(i-1)->DrawNow();
				}
				iControls->At(i)->DrawNow();
				if (iControls->At(i-1)==iLoadingLabel && iMediaReady) DrawMediaDisplay();
				iSBFrame->MoveThumbsBy(0, -1);
				iSBFrame->DrawScrollBarsNow();
				ShowHintByControl(iControls->At(i-1));
				return;
			}
		}
	}	
}

TBool CContextMediaAppPostContainer::IsLoadingMedia()
{
	return (iLive!=0);
}

void CContextMediaAppPostContainer::ShowVolume(TInt volume)
{
	CALLSTACKITEM_N(_CL("OurOwnRichTextEditor"), _CL("ShowVolume"));
	
	TRAPD(ignore, Settings().WriteSettingL(SETTING_PLAYBACK_VOLUME, volume));

	/*CWindowGc& gc = SystemGc();

	TRect aRect = TRect(TPoint(0,0), TSize(40,40));
	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	gc.SetPenColor(KRgbYellow);
	gc.SetBrushStyle(CGraphicsContext::ENullBrush);
	const CFont* fontUsed = iEikonEnv->DenseFont();
	gc.UseFont(fontUsed);
	TInt baseline = aRect.Height() - 2 * fontUsed->AscentInPixels();
	TBuf<10> buf;
	buf.AppendFormat(_L("vol.%d"), volume);
	gc.DrawText(buf, aRect, baseline, CGraphicsContext::ECenter);
*/
	iVolumeDisplayTimeOut->Reset();
	TBuf<10> buf;
	buf.AppendFormat(_L("vol.%d"), volume);
	iVolumeIndicator->SetTextL(buf);
	iVolumeIndicator->DrawDeferred();
	iVolumeDisplayTimeOut->Wait(1);

}


///--------------------------------------------------------------------------------------------

void OurOwnRichTextEditor::ConstructL(const CCoeControl* aParent, TRgb aColor, TInt aNumberOfLines,TInt aTextLimit,
				      TInt aEdwinFlags,TInt aFontControlFlags,TInt aFontNameFlags)
{
	CALLSTACKITEM_N(_CL("OurOwnRichTextEditor"), _CL("ConstructL"));
	CEikRichTextEditor::ConstructL(aParent, aNumberOfLines, aTextLimit,
		aEdwinFlags, aFontControlFlags, aFontNameFlags);

	CParaFormat paraFormat;
	TParaFormatMask paraFormatMask;
	TFontSpec fontspec = iContainer->Latin12()->FontSpecInTwips();
	TCharFormat charFormat( fontspec.iTypeface.iName, 115 );
	TCharFormatMask charFormatMask;

	paraFormatMask.SetAttrib(EAttLineSpacingControl);
	paraFormatMask.SetAttrib(EAttLineSpacing);
	paraFormatMask.SetAttrib(EAttAlignment);
	paraFormat.iLineSpacingControl = CParaFormat::ELineSpacingExactlyInTwips;
	if (fontspec.iHeight < 150) {
		paraFormat.iLineSpacingInTwips=115;
	} else {
		paraFormat.iLineSpacingInTwips = fontspec.iHeight-20;
	}
	paraFormat.iHorizontalAlignment = CParaFormat::ELeftAlign;

	l=RichText()->GlobalParaFormatLayer()->CloneL();
	l->SetL(&paraFormat, paraFormatMask);
	RichText()->SetGlobalParaFormat(l);

	charFormatMask.SetAttrib(EAttFontTypeface);
	charFormatMask.SetAttrib(EAttFontHeight);
	iColor=aColor;
	charFormat.iFontPresentation.iTextColor=aColor;
	charFormatMask.SetAttrib(EAttColor);

	cl=RichText()->GlobalCharFormatLayer()->CloneL();
	cl->SetL(charFormat, charFormatMask);
	RichText()->SetGlobalCharFormat(cl);
}

void OurOwnRichTextEditor::Reset()
{
	CALLSTACKITEM_N(_CL("OurOwnRichTextEditor"), _CL("Reset"));

	RichText()->Reset();
	AppendParagraph(*iDefaultValue, KRgbGray, CParaFormat::ELeftAlign);
	iShowingDefault=ETrue;
}

void OurOwnRichTextEditor::PrepareForFocusGainL()
{
	CALLSTACKITEM_N(_CL("OurOwnRichTextEditor"), _CL("PrepareForFocusGainL"));

	SetCursorPosL(0, EFalse);
	if (TextLength()==0) {
		Reset();
		HandleTextChangedL();
	}
	SetCursorPosL(TextLength(), ETrue);
}

void OurOwnRichTextEditor::PrepareForFocusLossL()
{
	CALLSTACKITEM_N(_CL("OurOwnRichTextEditor"), _CL("PrepareForFocusLossL"));
	if (TextLength()==0) {
		SetCursorPosL(0, EFalse);
		Reset();
		HandleTextChangedL();
		SetCursorPosL(TextLength(), ETrue);
	}
}
TKeyResponse OurOwnRichTextEditor::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	if (iShowingDefault && (aType == EEventKey || aType == EEventKeyDown) ) {
		SetCursorPosL(0, EFalse);
		RichText()->Reset();
		TInt pos=0;
		CParaFormat paraFormat;
		TParaFormatMask paraFormatMask;
		TFontSpec fontspec = iContainer->Latin12()->FontSpecInTwips();
		TCharFormat charFormat( fontspec.iTypeface.iName, fontspec.iHeight );
		TCharFormatMask charFormatMask;

		RichText()->GetParaFormatL(&paraFormat, paraFormatMask, pos, RichText()->DocumentLength());
		paraFormatMask.SetAttrib(EAttLineSpacingControl);
		paraFormatMask.SetAttrib(EAttLineSpacing);
		paraFormat.iLineSpacingControl = CParaFormat::ELineSpacingExactlyInTwips;
		if (fontspec.iHeight < 150) {
			paraFormat.iLineSpacingInTwips=115;
		} else {
			paraFormat.iLineSpacingInTwips = fontspec.iHeight-20;
		}
		RichText()->ApplyParaFormatL(&paraFormat, paraFormatMask, 0, RichText()->DocumentLength());
		charFormat.iFontPresentation.iTextColor=iColor;
		charFormatMask.SetAttrib(EAttColor);
		charFormatMask.SetAttrib(EAttFontTypeface);
		charFormatMask.SetAttrib(EAttFontHeight);
		RichText()->ApplyCharFormatL(charFormat, charFormatMask, 0, RichText()->DocumentLength());

		HandleTextChangedL();
		iShowingDefault=EFalse;
	}
	return CEikRichTextEditor::OfferKeyEventL(aKeyEvent, aType);
}
