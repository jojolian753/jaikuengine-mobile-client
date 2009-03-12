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

#include "ccu_jaicons.h"

#include "juik_iconmanager.h"
#include "juik_icons.h"
#include <jaicons.mbg>

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"


#include <akniconutils.h>
#include <gulicon.h> 
#include <fbs.h> 



//#define USE_FILE_PROVIDER 1

_LIT( KJaiconFile, "C:\\system\\data\\jaicons.mif");

struct TJaiconId 
{
	TInt iJaikuId;
	TInt iIconId;
};
	

const TInt KJaiconCount(61);
const TJaiconId KJaiconIds[KJaiconCount] =
	{
		{ 322,EMbmJaiconsBeer },
		{ 319,EMbmJaiconsCoffee },
		{ 329,EMbmJaiconsComputing },
		{ 341,EMbmJaiconsEat },
		{ 392,EMbmJaiconsHome },
		{ 399,EMbmJaiconsHurry },
		{ 400,EMbmJaiconsMorning },
		{ 363,EMbmJaiconsSleep },
		{ 367,EMbmJaiconsSong },
		{ 377,EMbmJaiconsToaster },
		{ 316,EMbmJaiconsAirplain },
		{ 388,EMbmJaiconsBike },
		{ 317,EMbmJaiconsBus },
		{ 401,EMbmJaiconsCar },
		{ 373,EMbmJaiconsLuggage },
		{ 372,EMbmJaiconsMetro },
		{ 375,EMbmJaiconsTaxi },
		{ 378,EMbmJaiconsTrain },
		{ 304,EMbmJaiconsTram },
		{ 325,EMbmJaiconsWalk },
		{ 395,EMbmJaiconsTheatre },
		{ 393,EMbmJaiconsHappy },
		{ 347,EMbmJaiconsLove },
		{ 308,EMbmJaiconsUzi },
		{ 364,EMbmJaiconsSnorkeling },
		{ 310,EMbmJaiconsBomb },
		{ 371,EMbmJaiconsStraitjacket },
		{ 389,EMbmJaiconsPils },
		{ 318,EMbmJaiconsGrumpy },
		{ 352,EMbmJaiconsMegaphone },
		{ 331,EMbmJaiconsGame },
		{ 387,EMbmJaiconsBlading },
		{ 396,EMbmJaiconsShop },
		{ 358,EMbmJaiconsRollator },
		{ 339,EMbmJaiconsFootball },
		{ 303,EMbmJaiconsLoudspeaker },
		{ 333,EMbmJaiconsDriller },
		{ 323,EMbmJaiconsBinoculars },
		{ 381,EMbmJaiconsIcecream },
		{ 394,EMbmJaiconsToiletpaper },
		{ 348,EMbmJaiconsBalloons },
		{ 354,EMbmJaiconsBook },
		{ 368,EMbmJaiconsSpraycan },
		{ 361,EMbmJaiconsScull },
		{ 326,EMbmJaiconsWallclock },
		{ 346,EMbmJaiconsHearingprotector },
		{ 328,EMbmJaiconsTv },
		{ 383,EMbmJaiconsMakeup },
		{ 391,EMbmJaiconsLifejacket },
		{ 370,EMbmJaiconsStorm },
		{ 108,EMbmJaiconsFeed_atom },
		{ 101,EMbmJaiconsFeed_blog },
		{ 102,EMbmJaiconsFeed_bookmark },
		{ 103,EMbmJaiconsFeed_bookwish },
		{ 104,EMbmJaiconsFeed_event },
		{ 105,EMbmJaiconsFeed_music },
		{ 106,EMbmJaiconsFeed_photo },
		{ 107,EMbmJaiconsFeed_place },
		{ 109,EMbmJaiconsFeed_video },
		{ 203,EMbmJaiconsJaiku_newuser },
		{ 204,EMbmJaiconsJaiku_sms },
	};


class CJaiconsImpl : public CJaicons, public MContextBase
{
public:
	CJaiconsImpl* NewL(MJuikIconManager& aIconManager)
	{
		CALLSTACKITEMSTATIC_N(_CL("CJaiconsImpl"), _CL("NewL"));
		auto_ptr<CJaiconsImpl> self( new (ELeave) CJaiconsImpl(aIconManager) );
		self->ConstructL();
		return self.release();
	}

	CJaiconsImpl(MJuikIconManager& aIconManager) : iIconManager( aIconManager )
	{
	}
	
	~CJaiconsImpl()
	{
		CALLSTACKITEM_N(_CL("CJaiconsImpl"), _CL("~CJaiconsImpl"));
		if ( iIcons ) iIcons->ResetAndDestroy();
		delete iIcons;
#ifdef USE_FILE_PROVIDER
		delete iIconFileProvider;
#endif
	}


	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CJaiconsImpl"), _CL("ConstructL"));
		iIcons = new (ELeave) CArrayPtrFlat<CGulIcon>(100);
		
		TIconID2 ids[KJaiconCount];
		for ( TInt i=0; i < KJaiconCount; i++ )
			{
				ids[i].iBitmap = KJaiconIds[i].iIconId;
				ids[i].iMask   = ids[i].iBitmap + 1;
			}
		
#ifdef USE_FILE_PROVIDER
 		iIconFileProvider = JuikIcons::LoadIconsViaFileProviderL( iIcons, KJaiconFile, &ids[0], KJaiconCount );
#else
 		JuikIcons::LoadIconsViaFileNameL( iIcons, KJaiconFile, &ids[0], KJaiconCount );
#endif

// 		for (TInt i=0; i < iIcons->Count(); i++)
// 			{
// 				JuikIcons::SetIconSizeL( *(iIcons->At(i)), TSize(30,30));
// 			}
		// 		iProviderId = iIconManager.GetNewProviderIdL();
		// 		iIconManager.SetIconsL( iProviderId, *iIcons );
	}
	
	
	TInt FindJaiconIndex(TInt aJaikuId )
	{
		CALLSTACKITEM_N(_CL("CJaiconsImpl"), _CL("FindJaiconIndex"));
		for (TInt i = 0; i < KJaiconCount; i++) 
			{ 
				if ( KJaiconIds[i].iJaikuId == aJaikuId )
					return i;
			}
		return KErrNotFound;
	}


	void RenderIconL(CGulIcon& aIcon)
	{
		if ( aIcon.Bitmap()->SizeInPixels() == TSize(0,0) )
			JuikIcons::SetIconSizeL( aIcon, TSize(30,30));			
	}
	
	CGulIcon* GetJaiconL( TInt aJaikuId )
	{
		CALLSTACKITEM_N(_CL("CJaiconsImpl"), _CL("GetJaiconL"));
		if ( aJaikuId != 0 )
			{
				TInt iconIx = FindJaiconIndex( aJaikuId );
				if ( iconIx != KErrNotFound )
					{
						CGulIcon* icon = iIcons->At(iconIx);
						RenderIconL( *icon );
						return icon;
					}
			}
		return NULL;
	}
	
private:
	MJuikIconManager& iIconManager;	

	TInt iProviderId;
	CArrayPtrFlat<CGulIcon>* iIcons; 
#ifdef USE_FILE_PROVIDER
	MAknIconFileProvider* iIconFileProvider;
#endif
};


EXPORT_C CJaicons* CJaicons::NewL(MJuikIconManager& aIconManager)
{
	CALLSTACKITEM_N(_CL("CJaicons"), _CL("NewL"));
	auto_ptr<CJaiconsImpl> self( new (ELeave) CJaiconsImpl( aIconManager ) );
	self->ConstructL();
	return self.release();
}
