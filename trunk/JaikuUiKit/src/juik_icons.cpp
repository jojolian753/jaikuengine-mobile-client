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

#include "juik_icons.h"

#include "icons.h"
#include "symbian_auto_ptr.h"
#include "app_context.h"
#include "break.h"

#include "scalableui_variant.h"

#ifdef __SCALABLEUI_VARIANT__
#include <akniconutils.h>
#endif 
 
#include <bautils.h>
#include <eikenv.h>
#include <f32file.h>
#include <gulicon.h>
#include <w32std.h>

class CIconFileProvider : public CBase, public MContextBase, public MAknIconFileProvider
{
public:
	static CIconFileProvider* NewL(const TDesC& aName)
	{
		auto_ptr<CIconFileProvider> self( new (ELeave) CIconFileProvider );
		self->ConstructL(aName);
		return self.release();
	}

	CIconFileProvider() : iRefCount(0) {}
	
	void ConstructL(const TDesC& aName)
	{
		iFile.Open( Fs(), aName, EFileShareReadersOnly);
	}
	
	~CIconFileProvider()
	{
		iFile.Close();
	}
	
	void AddRefCount()
	{
		iRefCount++;
	}

	virtual void RetrieveIconFileHandleL( RFile& aFile, const TIconFileType aType ) 
	{
		aFile.Duplicate(iFile);
	}
	
	virtual void Finished() 
	{
		iRefCount--;
		if ( iRefCount == 0 )
			iFile.Close();
	}


private:
	RFile iFile;
	TInt iRefCount;
};


	// class CIconFileHandle : public CBase, public MContextBase, 	
void FigureOutRealPathL(const TDesC& aFullPath, TDes& aRealPath, RFs& aFs) 
	{
		CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("FigureOutRealPathL"));

		if ( aFullPath == AknIconUtils::AvkonIconFileName() )
			{
				aRealPath.Copy( aFullPath );
				return;
			}
#ifdef __S60V3__
		// Change path to c:\\resource 
		TParse p; p.Set( aFullPath, 0, 0);
		aRealPath=_L("c:\\resource\\");
		aRealPath.Append(p.NameAndExt());
#else
		// Just use given path
		aRealPath=aFullPath;
#endif
		
#ifdef __WINS__
		// in WINS, read always from Z 
		aRealPath.Replace(0, 1, _L("z"));
#else
		// In device, if file doesn't exist, read from E
		TBool try_mif=EFalse;
		if (p.Ext().CompareF(_L(".mbm"))==0) try_mif=ETrue;
again:
		if (! BaflUtils::FileExists(aFs, aRealPath)) {
			aRealPath.Replace(0, 1, _L("e"));
			if (! BaflUtils::FileExists(aFs, aRealPath)) {
				if (try_mif) {
					aRealPath=_L("c:\\resource\\");
					aRealPath.Append(p.Name());
					aRealPath.Append(_L(".mif"));
					try_mif=EFalse;
					goto again;
				}
				User::Leave(KErrNotFound);
			}
		}
#endif
	}
   

	
#ifdef __SCALABLEUI_VARIANT__
CGulIcon* LoadScalableIconL( MAknIconFileProvider& aIconFileProvider, TInt aBitmap, TInt aMask, CWsScreenDevice* aScreen)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadScalableIconL"));
		auto_ptr<CFbsBitmap> bitmap(NULL);
		auto_ptr<CFbsBitmap> mask(NULL);
		
		if ( aMask != KErrNotFound ) {
			CFbsBitmap *bitmapp=0, *maskp=0;
			AknIconUtils::CreateIconL(bitmapp, maskp, aIconFileProvider, aBitmap, aMask);
			bitmap.reset(bitmapp);
			mask.reset(maskp);
		} else {
			bitmap.reset( AknIconUtils::CreateIconL(aIconFileProvider, aBitmap) );
		}
		
// 		bitmap->SetSizeInTwips(aScreen);
// 		if (mask.get()) mask->SetSizeInTwips(aScreen);
	
		auto_ptr<CGulIcon> icon(CGulIcon::NewL(bitmap.get(), mask.get()));
		bitmap.release(); mask.release();
		return icon.release();
	}

CGulIcon* LoadScalableIconL( const TDesC& aFileName, TInt aBitmap, TInt aMask, CWsScreenDevice* aScreen)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadScalableIconL"));
	auto_ptr<CFbsBitmap> bitmap(NULL);
	auto_ptr<CFbsBitmap> mask(NULL);
	
	if ( aMask != KErrNotFound ) {
		CFbsBitmap *bitmapp=0, *maskp=0;
		AknIconUtils::CreateIconL(bitmapp, maskp, aFileName, aBitmap, aMask);
		bitmap.reset(bitmapp);
		mask.reset(maskp);
	} else {
		bitmap.reset( AknIconUtils::CreateIconL(aFileName, aBitmap) );
	}
	
	bitmap->SetSizeInTwips(aScreen);
	if (mask.get()) mask->SetSizeInTwips(aScreen);
	
	auto_ptr<CGulIcon> icon(CGulIcon::NewL(bitmap.get(), mask.get()));
	bitmap.release(); mask.release();
	return icon.release();
}
#endif
	

EXPORT_C MAknIconFileProvider* JuikIcons::LoadIconsViaFileProviderL(CArrayPtrFlat<CGulIcon> * aIconList, 
																	const TDesC& aIconFile, 
																	const TIconID2* aIconDefs, TInt aNbIcons)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadIconsViaFileProviderL"));
#ifdef __SCALABLEUI_VARIANT__
	
	CEikonEnv* env=CEikonEnv::Static();
	RFs& fs=env->FsSession();
	
	CWsScreenDevice* screen=CEikonEnv::Static()->ScreenDevice();
			
	TFileName real; 
	FigureOutRealPathL( aIconFile, real, fs);				
	
	auto_ptr<CIconFileProvider> iconProvider( CIconFileProvider::NewL( real ) );

	// Loop through icon ids
	for (int i = 0; i<aNbIcons;i++)
		{
			iconProvider->AddRefCount();
			auto_ptr<CGulIcon> icon( LoadScalableIconL( *iconProvider, 
														aIconDefs[i].iBitmap, 
														aIconDefs[i].iMask, screen) );
			aIconList->AppendL(icon.get()); 
			icon.release();
			}
#endif 
	return iconProvider.release();
}


EXPORT_C void JuikIcons::LoadIconsViaFileNameL(CArrayPtrFlat<CGulIcon> * aIconList, 
											   const TDesC& aIconFile, 
											   const TIconID2* aIconDefs, TInt aNbIcons)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadIconsViaFileNameL"));
#ifdef __SCALABLEUI_VARIANT__
	
	CEikonEnv* env=CEikonEnv::Static();
	RFs& fs=env->FsSession();
	
	CWsScreenDevice* screen=CEikonEnv::Static()->ScreenDevice();
			
	TFileName real; 
	FigureOutRealPathL( aIconFile, real, fs);				
	
	// Loop through icon ids
	for (int i = 0; i<aNbIcons;i++)
		{
			auto_ptr<CGulIcon> icon( LoadScalableIconL( real, 
														aIconDefs[i].iBitmap, 
														aIconDefs[i].iMask, screen) );
			aIconList->AppendL(icon.get()); 
			icon.release();
		}
#endif 
}



void LoadScalableIconsL(CArrayPtrFlat<CGulIcon> * aIconList, const TIconID* aIconDefs, TInt aNbIcons)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadScalableIconsL"));
#ifdef __SCALABLEUI_VARIANT__
	TFileName real; 
	TFileName prev;
	
	CEikonEnv* env=CEikonEnv::Static();
	RFs& fs=env->FsSession();
	
	CWsScreenDevice* screen=CEikonEnv::Static()->ScreenDevice();
	
	// Loop through icon ids
	for (int i = 0; i<aNbIcons;i++)
		{
			
			TPtrC file((TText16*)aIconDefs[i].iMbmFile);
			
			
			// Decide file only if previous file wasn't same 
			if (prev.Compare(file)) {
				FigureOutRealPathL(file, real, fs);			
				prev=file;			
			}
			
			auto_ptr<CGulIcon> icon( LoadScalableIconL(real, aIconDefs[i].iBitmap, aIconDefs[i].iMask, screen) );
			aIconList->AppendL(icon.get()); 
			icon.release();
			}
#endif 
	
}



EXPORT_C void JuikIcons::LoadIconsL(CArrayPtrFlat<CGulIcon> * aIconList, const TIconID* aIconDefs, TInt aNbIcons)
	{
		CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadIconsL(multiple files)"));
#ifdef __SCALABLEUI_VARIANT__
		TRAPD(err, LoadScalableIconsL( aIconList, aIconDefs, aNbIcons ) );
		if ( err )
			{
				// Hack to retry
				aIconList->ResetAndDestroy();
				LoadIcons(aIconList, aIconDefs, aNbIcons);
			}

#else // not __SCALABLEUI_VARIANT__
		LoadIcons(aIconList, aIconDefs, aNbIcons);
#endif // __SCALABLEUI_VARIANT__
	} 
	

EXPORT_C CGulIcon* JuikIcons::LoadSingleIconL(const TIconID& aIconDef)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadSingleIconL(aIconDef)"));
	TPtrC fileName((TText16*)aIconDef.iMbmFile);
	TIconID2 id = { aIconDef.iBitmap, aIconDef.iMask };
	return LoadSingleIconL(fileName, id);
}

EXPORT_C CGulIcon* JuikIcons::LoadSingleIconL(const TDesC& aFileName, TIconID2 aId)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadSingleIconL(aFileName)"));
#ifdef __SCALABLEUI_VARIANT__
	
	CEikonEnv* env=CEikonEnv::Static();
	RFs& fs=env->FsSession();
	CWsScreenDevice* screen=CEikonEnv::Static()->ScreenDevice();
	
	TFileName real; 
	FigureOutRealPathL( aFileName, real, fs);				
	
	// Loop through icon ids
	auto_ptr<CGulIcon> icon( LoadScalableIconL( real, 
												aId.iBitmap, 
												aId.iMask, screen) );
	return icon.release();
#else
	return NULL;
#endif 
}


EXPORT_C CFbsBitmap* JuikIcons::LoadBitmapL(const TDesC& aFileName, TInt aId)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("LoadBitmap(aFileName)"));
#ifdef __SCALABLEUI_VARIANT__
	
	CEikonEnv* env=CEikonEnv::Static();
	RFs& fs=env->FsSession();
	CWsScreenDevice* screen=CEikonEnv::Static()->ScreenDevice();
	
	TFileName real; 
	FigureOutRealPathL( aFileName, real, fs);				
	
	
	auto_ptr<CFbsBitmap> bitmap( AknIconUtils::CreateIconL(real, aId) );
	bitmap->SetSizeInTwips(screen);
	
	return bitmap.release();
#else
	return NULL;
#endif 
}


EXPORT_C void JuikIcons::SetIconSizeL( CGulIcon& aIcon, const TSize& aSize, TScaleMode aMode)
{
	CALLSTACKITEMSTATIC_N(_CL("JuikIcons"), _CL("SetIconSizeL"));
#ifdef __SCALABLEUI_VARIANT__
	
	AknIconUtils::SetSize( aIcon.Bitmap(), aSize, aMode);
	// no need to set mask size - it's automatically set
	
#else // not  __SCALABLEUI_VARIANT__
	
	// do nothing, non-svg icons have correct size when loaded

#endif //   __SCALABLEUI_VARIANT__
}


EXPORT_C TInt JuikIcons::GetIconIndex(TInt identifier, const TIconID2* aIconDefs, TInt aNbIcons)
{
	for (int i = 0; i<aNbIcons;i++)
	{
		if ( aIconDefs[i].iBitmap == identifier )
		{
			return i;
		}
	}	
	return 0; // index of empty icon
}



EXPORT_C CFbsBitmap* JuikIcons::CreateFilledRectL(TSize aSize, TRgb aRgb)
{
	CALLSTACKITEM_N(_CL("JuikIcons"), _CL("CreateFilledRectL"));
	CWsScreenDevice* screen=CEikonEnv::Static()->ScreenDevice();
	auto_ptr<CFbsBitmap> bitmap( new (ELeave) CFbsBitmap );
	bitmap->Create( aSize, screen->DisplayMode() );
		
	auto_ptr<CFbsBitmapDevice> device( CFbsBitmapDevice::NewL(bitmap.get()));
	CGraphicsContext* tmpGc = NULL;
	User::LeaveIfError(device->CreateContext( tmpGc ));
	auto_ptr<CGraphicsContext> gc( tmpGc );
	gc->SetBrushStyle( CGraphicsContext::ESolidBrush );
	gc->SetPenStyle( CGraphicsContext::ENullPen );
	gc->SetBrushColor( aRgb );
	gc->DrawRect( TRect( TPoint(0,0), bitmap->SizeInPixels() ) );
	return bitmap.release();
}


// static void ApplyAlphaL(CFbsBitmap* aBitmap, TInt alpha)
// {
// 	CALLSTACKITEM_N(_CL("JuikIcons"), _CL("CreateFilledRectL"));
	
// 	auto_ptr<CFbsBitmapDevice> device( CFbsBitmapDevice::NewL( aBitmap ) );
// 	CGraphicsContext* tmpGc = NULL;
// 	User::LeaveIfError(device->CreateContext( tmpGc ));
// 	auto_ptr<CGraphicsContext> gc( tmpGc );

	
// // 	const TInt KCount(256);
// // 	TRgb colorMap[KCount*2];
// 	TSize sz( aBitmap->SizeInPixels() );
// 	TReal multiplier = alpha / 255.0;
// 	gc->SetBrushStyle( CGraphicsContext::ENullBrush );
// 	gc->SetPenStyle( CGraphicsContext::ESolidPen );
// 	for (TInt y=0; y < sz.iHeight; y++)
// 		for(TInt x=0; x < sz.iWidth; x++)
// 			{
// 				TPoint p(x,y);
// 				TRgb c;
// 				aBitmap->GetPixel(c, p);
// 				c.SetRed( c.Red() * multiplier );
// 				c.SetGreen( c.Green() * multiplier );
// 				c.SetBlue( c.Blue() * multiplier );
// 				gc->SetPenColor( c );
// 				gc->Plot( p );
// 			}
// }


EXPORT_C CGulIcon* JuikIcons::CreateColoredIconL(const TDesC& aFileName, TInt aIconId, TSize aSize, TRgb aRgb, TScaleMode aMode)
{
	CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("LoadIconL"));
// 	TRgb c = aRgb;
// 	c.SetAlpha(255);
	auto_ptr<CFbsBitmap> bitmap( CreateFilledRectL( aSize, aRgb ) );
	
	auto_ptr<CFbsBitmap> mask( JuikIcons::LoadBitmapL( aFileName, aIconId) );
	AknIconUtils::SetSize( mask.get(), aSize, aMode );
//  	if ( aRgb.Alpha() < 255 )
//  		ApplyAlphaL( mask.get(), aRgb.Alpha() );
	
	auto_ptr<CGulIcon> icon( CGulIcon::NewL(bitmap.release(), mask.release()) );;
	return icon.release();
}
