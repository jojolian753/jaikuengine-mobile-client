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

#include "juik_photo.h"

#include "juik_debug.h"
#include "juik_layout.h"
#include "jaiku_layoutids.hrh"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"

#include <aknsdrawutils.h>
#include <aknsutils.h>
#include <bautils.h>
#include <eikenv.h>
#include <gulutil.h>
#include <imageconversion.h>


class MLoaderCb {
public:
	virtual void Loaded(TInt aError) = 0;
};


class CJuikPhotoImpl : public CJuikPhoto, public MContextBase, public MLoaderCb {
public:

	class CLoader : public CCheckedActive, public MContextBase {
	public:

		CLoader(const TSize& aMaxSize, TSize aEmptySize) : 
			CCheckedActive(CActive::EPriorityStandard, _L("Photo loader")), 
			iMaxSize(aMaxSize), 
			iEmptySize(aEmptySize),
			iState(EInit), 
			iScaledSize(0,0) { }

		void ConstructL(MLoaderCb* aCb) {
			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("ConstructL"));
			CActiveScheduler::Add(this);
			iErrorMsg=KNullDesC; //_L("Loading...");
			iCb=aCb;
		}

		TSize ScaledSize() 
		{ 
			if ( iState == EInit ) 
				return iEmptySize;
			else
				return iScaledSize; 
		}
		
		TSize ScaleSize(TSize aPhotoSize, TSize aMaxSize) 
		{
			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("ScaleSize"));
			TReal hQ = aPhotoSize.iWidth / (TReal) aMaxSize.iWidth; 
			TReal vQ = aPhotoSize.iHeight / (TReal) aMaxSize.iHeight; 

			TInt photoV = vQ > hQ ? aPhotoSize.iHeight : aPhotoSize.iWidth;
			TInt maxV = vQ > hQ ? aMaxSize.iHeight : aMaxSize.iWidth;
			TInt resultV = photoV; 
			
			if ( maxV == 0 )
				return TSize(0,0);

			TInt scale = 1; 
			while ( resultV > maxV )
				{
					scale *= 2; 
					resultV /= 2;
				}

			return TSize( aPhotoSize.iWidth / scale, aPhotoSize.iHeight / scale);
		}
			
		
		void StartToConvertFile(const TDesC& aFilename) {
			CC_TRAPD(err, StartToConvertFileL(aFilename));
			if (err!=KErrNone) {
				iErrorMsg = _L("Error loading image: ");
				iErrorMsg.AppendNum(err);
			}
		}
		
		void StartToConvertFileL(const TDesC& aFilename) {
			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("StartToConvertFileL"));

			if ( ! BaflUtils::FileExists( Fs(), aFilename) )
				{
				iErrorMsg = _L("Image file not found");
				return;
				}

			iErrorMsg=KNullDesC; //_L("Loading...");
			
			iState=EInit;
			Cancel();
			delete iDecoder; iDecoder=0;
			delete iBitmap; iBitmap=0;
			CC_TRAPD(err, iDecoder=CImageDecoder::FileNewL(Fs(), aFilename) );
			if ( err != KErrNone )
				User::Leave( err );

			TFrameInfo frameInfo=iDecoder->FrameInfo();
 			
			TSize photoSz = frameInfo.iOverallSizeInPixels;
			
			TBool doScale = photoSz.iWidth > iMaxSize.iWidth || photoSz.iHeight > iMaxSize.iHeight;
			TSize scaledSize = photoSz;
			
			if ( doScale )
				{
					scaledSize = ScaleSize( photoSz, iMaxSize );
				}
			iScaledSize = scaledSize;
			iBitmap=new (ELeave) CFbsBitmap();
			iBitmap->Create(scaledSize, EColor64K);
			iDecoder->Convert(&iStatus, *iBitmap);

			iState=ELoading;
			
			SetActive();
		}
		void DoCancel() {
			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("DoCancel"));
			if ( iDecoder )
				iDecoder->Cancel();
		}
		void CheckedRunL() {
			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("CheckedRunL"));
			TInt aError=iStatus.Int();
			if (aError==KErrUnderflow) {
				iDecoder->ContinueConvert(&iStatus);
				SetActive();
				return;
			}
			if (aError!=KErrNone) {
				iErrorMsg = _L("Image loading error:");
				iErrorMsg.AppendNum(aError);
				iState = EError;
			} else {
				iState = EDone;
			}
			delete iDecoder; iDecoder=0;
			iCb->Loaded(aError);
		}
		
		TInt CheckedRunError(TInt aError)
		{
			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("CheckedRunError"));
			iErrorMsg = _L("Image loading error:");
			iErrorMsg.AppendNum( aError );
			iState = EError;
			delete iDecoder; iDecoder=0;
			iCb->Loaded( aError );
			return aError;
		}

		~CLoader() {
			CALLSTACKITEM_N(_CL("CJuikPhotoImpl::CLoader"), _CL("~CLoader"));
			Cancel();
			delete iDecoder;
			delete iBitmap;
		}
		
		enum TState
			{
				EError = -1,
				EInit = 0,
				ELoading,
				EDone,
			} iState;


		CImageDecoder* iDecoder;
		CFbsBitmap* iBitmap;
		MLoaderCb* iCb;
		
		TSize iScaledSize;
		TSize iMaxSize;
		TSize iEmptySize;
	public:
		TBuf<100> iErrorMsg;
	};
	
	void ConstructL(CCoeControl* aParent, const TSize& aMaxSize, const TSize& aDefaultSize) {
		CALLSTACKITEM_N(_CL("CJuikPhotoImpl"), _CL("ConstructL"));
		iMargin = Layout().GetLayoutItemL(LG_feed_controls_margins, 
										  LI_feed_controls_margins__thumbnail).Margins();
//		iMargin = Juik::Margins(0,0,0,0);
		SetContainerWindowL(*aParent);

		iMaxSize=aMaxSize;
		iDefaultSize = aDefaultSize;
		
		SetAlignment( EHCenterVCenter );
		iLoader=new (ELeave) CLoader(aMaxSize, iDefaultSize);
		iLoader->ConstructL(this);
	}

	void SetDrawBackground(TBool aDraw)
	{
		iDrawBackground = aDraw;
	}

	void SetFileL(const TDesC& aFilename) {
		CALLSTACKITEM_N(_CL("CJuikPhotoImpl"), _CL("SetFileL"));
		iLoader->StartToConvertFile(aFilename);
	}

	void AddListenerL(MListener& aListener)
	{
		CALLSTACKITEM_N(_CL("CJuikPhotoImpl"), _CL("AddListener"));
		iListeners.AppendL( &aListener );
	}

	void RemoveListener(MListener& aListener)
	{
		CALLSTACKITEM_N(_CL("CJuikPhotoImpl"), _CL("RemoveListener"));
		TInt ix = iListeners.Find( &aListener );
		if ( ix != KErrNotFound )
			iListeners.Remove( ix );
	}

	virtual void Loaded(TInt aError) {
		CALLSTACKITEM_N(_CL("CJuikPhotoImpl"), _CL("Draw"));
 		for ( TInt i=0; i < iListeners.Count(); i++ )
			{
				iListeners[i]->MediaLoaded( aError == KErrNone );
			}
//  		if ( IsReadyToDraw() ) 
// 			DrawNow();
	}

	TSize MinimumSize()
	{
		TSize imgSize = TSize(0,0);
		if ( iLoader->iState == CLoader::EDone )
			{
				TSize bmpSz = iLoader->iBitmap->SizeInPixels();
				if ( bmpSz.iHeight <= iMaxSize.iHeight && bmpSz.iWidth <= iMaxSize.iWidth  )
					imgSize = bmpSz;
				else
					imgSize = iMaxSize;
			}
		else 
			imgSize = iLoader->ScaledSize();
		
		if ( imgSize == TSize(0,0) )
			return TSize(0,0);
		else 
			return imgSize + iMargin.SizeDelta();
	}

	void SetBorderColor( const TRgb& aNormalColor )
	{
		iNormalColor = aNormalColor;		
		iFocusColor  = iNormalColor;
	}
	
	void EnableFocusColor(const TRgb& aFocusColor )
	{
		iFocusColor = aFocusColor;
	}
	
	static TBool IsInside( const TPoint& p, const TRect& r ) 
	{
		TRect dummy( p, TSize(0,0 ) );
		return r.Intersects( dummy );
	}
	
	void Draw(const TRect& aRect) const {
		CALLSTACKITEM_N(_CL("CJuikPhotoImpl"), _CL("Draw"));
		TRect fullR = Rect();
		TRect innerR = iMargin.InnerRect( Rect() );
		
 		CWindowGc& gc = SystemGc();
		
		MAknsSkinInstance* skin = AknsUtils::SkinInstance();
		MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );

 		if ( iDrawBackground )
			if ( ! AknsDrawUtils::Background( skin, cc, this, gc, aRect ) )
				{
					gc.SetBrushColor( KRgbWhite );
					gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
					gc.Clear( aRect );
				}

		TRect outer = Rect();
		TRect inner = iMargin.InnerRect( outer );

		TRect imgR; 
		if (iLoader->iState == CLoader::EDone) {
			CFbsBitmap* bmp = iLoader->iBitmap;
			
			if (!bmp) return;
			
			
			CWindowGc& gc = SystemGc();

			
			TSize bmpSize = bmp->SizeInPixels();
			TBool clip = inner.Size().iWidth < bmpSize.iWidth || inner.Size().iHeight < bmpSize.iHeight;
			
			TRect aligned = iAlignment.InnerRect( inner, bmpSize );
			imgR = aligned;
			TSize sourceSz = clip ? aligned.Size() : bmpSize;


			TRect sourceR( TPoint(0,0), sourceSz );			
			gc.BitBlt( aligned.iTl, bmp, sourceR);	
		}
		else
			{
				imgR = iAlignment.InnerRect( inner, iLoader->ScaledSize() );
			}
		

		TRgb borderColor = IsFocused() ? iFocusColor : iNormalColor;
		gc.SetBrushColor( borderColor );
		gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
		TRect borderR = iMargin.OuterRect( imgR );
		DrawUtils::ClearBetweenRects( gc, borderR, imgR );
		
		
#ifdef JUIK_BOUNDINGBOXES
		JuikDebug::DrawBoundingBox(gc ,Rect(), TRgb(0,0,200));
#endif

	}
	
	CFbsBitmap* Bitmap() {
		return iLoader->iBitmap;
	}

	~CJuikPhotoImpl() {
		CALLSTACKITEM_N(_CL("CJuikPhotoImpl"), _CL("~CJuikPhotoImpl"));
		delete iLoader;
		iListeners.Close();
	}

	const TDesC& LoadingError() 
	{
		if ( iLoader ) return iLoader->iErrorMsg;
		else return KNullDesC;
	}
	
private:
	const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }

private:

	RPointerArray<MListener> iListeners;

	TMargins8 iMargin;
	CLoader* iLoader;
	TSize iMaxSize;
	TSize iDefaultSize;

	TBool iDrawBackground;

	TRgb iFocusColor;
	TRgb iNormalColor;
};


EXPORT_C CJuikPhoto* CJuikPhoto::NewL(CCoeControl* aParent, const TSize& aMaxSize, const TSize& aDefaultSize)
{
	CALLSTACKITEMSTATIC_N(_CL("CJuikPhoto"), _CL("NewL"));
	auto_ptr<CJuikPhotoImpl> self( new (ELeave) CJuikPhotoImpl );
	self->ConstructL(aParent, aMaxSize, aDefaultSize);
	return self.release();
}
