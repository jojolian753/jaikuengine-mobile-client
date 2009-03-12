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

#include "ccu_userpics.h"

#include "csd_userpic.h"
#include "cu_buffer_icon.h"
#include <contextcontactsui.mbg>

#include "juik_iconmanager.h"
#include "juik_icons.h"
#include "break.h"
#include "reporting.h"
#include "icons.h"

#include <gulicon.h>
#include <akniconarray.h>
#include <fbs.h>

// FIXME: this is for icon demo hack only
#include "juik_layout.h"


class CUserPicInfo : public CBase, public MContextBase
{
public:
	static CUserPicInfo* NewL(const TDesC& aNick, CFbsBitmap* aBmp)
	{
		CALLSTACKITEMSTATIC_N(_CL("CUserPicInfo"), _CL("NewL"));
		auto_ptr<CUserPicInfo> self( new (ELeave) CUserPicInfo(aBmp) );
		self->ConstructL( aNick );
		return self.release();
	}
	
	
	CUserPicInfo(CFbsBitmap* aBmp) : 
		iBitmap( aBmp ),
		iIconId( KErrNotFound ) {}
	
	void ConstructL(const TDesC& aNick)
	{
		iNick.Copy(aNick);
	}
	
	~CUserPicInfo()
	{
		CALLSTACKITEM_N(_CL("CUserPicInfo"), _CL("~CUserPicInfo"));
		delete iBitmap;	
		delete iOldBitmap;
	}
	
	
	void ReplacePicL(CFbsBitmap* aNewBitmap) 
	{
		CALLSTACKITEM_N(_CL("CUserPicInfo"), _CL("ReplacePicL"));
		if (iOldBitmap) 
			delete iOldBitmap;
		iOldBitmap = iBitmap;
		iBitmap = aNewBitmap;
		
	}
	
	void ReleaseOldPicL()
	{
		CALLSTACKITEM_N(_CL("CUserPicInfo"), _CL("ReleaseOldPicL"));
		delete iOldBitmap;
		iOldBitmap = NULL;
	}

	CJabberData::TNick iNick;
	CFbsBitmap* iBitmap;
	TInt iIconId;

	CFbsBitmap* iOldBitmap;
};




EXPORT_C CUserPics* CUserPics::NewL(CJabberPics& aJabberPics, 	CJabberData& aJabberData, MJuikIconManager& aIconManager)
{
	CALLSTACKITEMSTATIC_N(_CL("CUserPics"), _CL("NewL"));
	auto_ptr<CUserPics> self( new (ELeave) CUserPics( aJabberPics, aJabberData, aIconManager) );
	self->ConstructL();
	return self.release();
}


EXPORT_C CUserPics::~CUserPics()
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("~CUserPics"));
 	iUserPicInfos.ResetAndDestroy();
 	delete iBBSession;
	iObservers.Close();
	iDummyIcons.ResetAndDestroy();
}


EXPORT_C TInt CUserPics::GetIconIndexL(TInt aContactId)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("GetIconIndexL(aContactId)"));	
	CJabberData::TNick nick;
 	iJabberData.GetJabberNickL( aContactId, nick );
 	return GetIconIndexL( nick );
}


TBool CUserPics::LoadPictureL(const TDesC& aNick)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("LoadPictureL"));	
	auto_ptr<CFbsBitmap> bitmap( iJabberPics.GetPicL( aNick ) );
	if ( bitmap.get() )
		{
			auto_ptr<CUserPicInfo> info( CUserPicInfo::NewL(aNick, bitmap.release()) );
			iUserPicInfos.Append( info.get() );
			CUserPicInfo* infoP = info.release();
			AddToIconManagerL( *infoP );

			return ETrue;
		}
	return EFalse;
}


EXPORT_C TInt CUserPics::GetIconIndexL(const TDesC& aNick)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("GetIconIndexL(aNick)"));
	CUserPicInfo* info = GetPicInfoL( aNick );
	if ( ! info  ) 
		{
			 if ( LoadPictureL(aNick) )
				 {
					 info = GetPicInfoL( aNick );
				 }
		}
	
	if ( info )
		return iIconManager.GetListBoxIndexL( iProviderId, info->iIconId );
	else 
		return KErrNotFound;
}


EXPORT_C CGulIcon* CUserPics::GetIconL(const TDesC& aNick)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("GetIconIndexL(aNick)"));	
	TInt ix = GetIconIndexL( aNick );
	if ( ix >= 0 )
		{
			return iIconManager.GetIconArray()->At( ix );
		}
	else
		return NULL;
}

EXPORT_C CGulIcon* CUserPics::GetIconL(TInt aContactId)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("GetIconIndexL(aContactId)"));	
	TInt ix = GetIconIndexL( aContactId );
	if ( ix >= 0 )
		{
			return iIconManager.GetIconArray()->At( ix );
		}
	else
		return NULL;
}


void CUserPics::AddCurrentIconsToManagerL()
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("AddCurrentIconsToManagerL"));	
	for (TInt i=0; i < iUserPicInfos.Count(); i++)
		{
			AddToIconManagerL( *(iUserPicInfos[i]) );
		}
}

EXPORT_C void CUserPics::AddObserverL( MUserPicObserver& aObserver )
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("AddObserverL"));
	iObservers.Append( &aObserver );
}


EXPORT_C void CUserPics::RemoveObserverL( MUserPicObserver& aObserver )
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("RemoveObserverL"));
	for ( TInt i=0; i < iObservers.Count(); i++ )
		{
			if ( iObservers[i] == & aObserver )
				{
					iObservers.Remove( i );
					break;
				}
		}
}

CFbsBitmap* CUserPics::LoadBitmapFromPicL(const CBBUserPic& aPic)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("LoadBitmapFromPicL"));
	if (aPic.iMbm().Length() <= 0)
		{
			User::Leave(KErrUnderflow);
		}
	
	TInt iconIndex = 0;	
	TInt bitmapCount = NumberOfBitmapsL( aPic.iMbm() );
	if (0 <= iImageSize && iImageSize < bitmapCount )
		{
			iconIndex = iImageSize;
		}
	
	auto_ptr<CFbsBitmap> bmp(NULL);
	bmp.reset(LoadBitmapL(aPic.iMbm(), iconIndex));
	return bmp.release();
}


void CUserPics::NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
						  const TComponentName& aComponentName, const MBBData* aData)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("NewValueL"));

	// Subname contains nick. It can be found from data object also.

 	if ( aName == KUserPicTuple )
 		{
 			TBool is_new;
 			// 1 get pic 
 			const CBBUserPic* pic =  bb_cast<CBBUserPic>( aData );
 			if (!pic) {
 				iBBSession->DeleteL(aId);
 				return;
 			}

			auto_ptr<CFbsBitmap> bmp(NULL);
 			CC_TRAPD(err, bmp.reset(LoadBitmapFromPicL(*pic)));
			
			if (err!=KErrNone) {
				if (err!=KErrNoMemory) iBBSession->DeleteL(aId);
 				TBuf<100> msg=_L("error loading bitmap for nick ");
 				msg.Append(aSubName);
 				msg.Append(_L(" "));
 				msg.AppendNum(err);
 				Reporting().UserErrorLog(msg);
 				return;
 			}
			
			TBool isverified=pic->iPhoneNumberIsVerified();

 			// 2 store bmp for nick
			TPtrC nick = pic->iNick();
			iJabberPics.SetPicL( nick, *bmp, is_new );
			
 			// 3 cache locally
 			CUserPicInfo* info = GetPicInfoL( nick );
 			if ( info ) 
 				{	
					// Replace icon 
					
					// in a sense, this is a transaction, that should be rolled back, if something leaves
					// TRANSACTION BEGINS
					info->ReplacePicL( bmp.release() );
					ReplaceInIconManagerL( *info );
					info->ReleaseOldPicL();
					// TRANSACTION END
				}
			else
				{
					// Add icon
					
					// TRANSACTION BEGINS
					auto_ptr<CUserPicInfo> infoV( CUserPicInfo::NewL(nick, bmp.release() ) );
					iUserPicInfos.Append( infoV.get() );
					info=infoV.release();
					AddToIconManagerL( *info );
					// TRANSACTION END
				}
						
   			// remove bitmap from message queue 
   			iBBSession->DeleteL( aId, ETrue );
			
			for ( TInt i = 0; i < iObservers.Count(); i++ )
				{
					iObservers[i]->UserPicChangedL( nick, is_new );
				}
		}
}


void CUserPics::DeletedL(const TTupleName& aName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("DeletedL"));
	// we don't care about deletions, because we do them ourself 
}



CUserPics::CUserPics(CJabberPics& aJabberPics, 	CJabberData& aJabberData, MJuikIconManager& aIconManager) : 
	iJabberPics(aJabberPics), iJabberData(aJabberData), iIconManager( aIconManager )
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("CUserPics"));
}


void CUserPics::ConstructL()
{	
 	CALLSTACKITEM_N(_CL("CUserPics"), _CL("ConstructL"));
	
	TSize sz = MJuikLayout::ScreenSize();
	if ( sz.iWidth == 416 || sz.iHeight == 416 )
		iImageSize = CUserPics::EDouble;
	else if ( sz.iWidth == 800 && sz.iHeight == 352 )
		iImageSize = CUserPics::EQvgaPortrait;
	else if ( sz.iWidth  == 240 && sz.iHeight == 320 )
		iImageSize = CUserPics::EQvgaPortrait;
	else
		iImageSize = CUserPics::ELegacy;
	

	iProviderId = iIconManager.GetNewProviderIdL();
	// Commented out, because we use lazy loading!	
// 	// Fetch all existing user pics from database
// 	auto_ptr< CArrayPtr<CFbsBitmap> > bitmaps( new (ELeave) CArrayPtrFlat<CFbsBitmap>(10) );	
// 	auto_ptr< CDesCArray > nicks( new (ELeave) CDesCArrayFlat(10) );
// 	iJabberPics.GetAllPicsL( *nicks, *bitmaps );

// 	// Create user pic infos for them 
// 	for ( TInt i = 0; i < nicks->Count(); i++ )
// 		{
			
// 			auto_ptr<CUserPicInfo> info( CUserPicInfo::NewL(nicks->MdcaPoint(i), bitmaps->At(i)) );					
// 			iUserPicInfos.Append( info.release() );
// 		}
// 	AddCurrentIconsToManagerL();

 	iBBSession= BBSession()->CreateSubSessionL(this);
 	iBBSession->AddNotificationL(KUserPicTuple, ETrue);
}


CGulIcon* CUserPics::CreateIconL(CUserPicInfo& aInfo)
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("CreateIconL"));
	auto_ptr<CGulIcon> icon( CGulIcon::NewL() );
	icon->SetBitmapsOwnedExternally( ETrue );
	icon->SetBitmap( aInfo.iBitmap );
	icon->SetMask( CommonMaskL() );
	return icon.release();
}


TInt CUserPics::FindPicInfoL( const TDesC& aNick )
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("FindPicInfoL"));

	for ( TInt i = 0; i < iUserPicInfos.Count(); i++)
		{
			CUserPicInfo* info = iUserPicInfos[i];
			if ( CJabberData::EqualNicksL( info->iNick, aNick ) )
				{
					return i;
				}
		}
	return KErrNotFound;
}


CUserPicInfo* CUserPics::GetPicInfoL( const TDesC& aNick )
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("GetPicInfoL"));
	TInt ix = FindPicInfoL( aNick );
	if ( ix >= 0 ) return iUserPicInfos[ix];
	else           return NULL;
}


void CUserPics::AddToIconManagerL( CUserPicInfo& aInfo )
{	
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("AddToIconManagerL"));
	
	auto_ptr<CGulIcon> icon( CreateIconL(aInfo) );
	iIconManager.AddIconL( iProviderId, icon.release());
	aInfo.iIconId = FindPicInfoL(aInfo.iNick);												 
}


void CUserPics::ReplaceInIconManagerL( CUserPicInfo& aInfo )
{
	CALLSTACKITEM_N(_CL("CUserPics"), _CL("ReplaceInIconManagerL"));

	auto_ptr<CGulIcon> icon( CreateIconL( aInfo ) );
	TInt ix = aInfo.iIconId;
	if ( ix != KErrNotFound )
		{
			iIconManager.ReplaceIconL( iProviderId, ix, icon.release() );
		}
}

EXPORT_C CGulIcon& CUserPics::DummyIconL(TSize aSize)
{
	for (TInt i=0; i < iDummyIcons.Count(); i++)
		{
			CGulIcon* icon = iDummyIcons[i];
			if ( icon->Bitmap()->SizeInPixels() == aSize )
				return *icon;
		}
	// not found, add it and return
	
	// Load dummy icon
	const TIconID iconId = 
		_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
						EMbmContextcontactsuiDummybuddy, 
						EMbmContextcontactsuiDummybuddy_mask );
	auto_ptr<CGulIcon> icon( JuikIcons::LoadSingleIconL(iconId) );
	JuikIcons::SetIconSizeL( *icon, aSize );
	iDummyIcons.AppendL(icon.get());
	CGulIcon* result = icon.release();
	return *result;
}

CFbsBitmap* CUserPics::CommonMaskL()
{
	return NULL;
	//return iDummyIcons->At(0)->Mask();
}
