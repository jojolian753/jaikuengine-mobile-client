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

#include "ListControl.h"

#include "juik_sizer.h"

#include "juik_icons.h"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"

#include <contextcontactsui.mbg>

#include <akniconarray.h> 
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknsutils.h> 
#include <eikimage.h> 
#include <eiklabel.h>

#include <gulcolor.h> 
#include <eikenv.h>

#include <gulicon.h>
 
enum KEYCODES {
	JOY_LEFT = 0xF807,
	JOY_RIGHT = 0xF808,
	JOY_UP = 0xF809,
	JOY_DOWN = 0xF80A,
	JOY_CLICK = 0xF845,
	KEY_CALL = 0xF862,
	KEY_CANCEL = 0xF863,
	KEY_C = 0x0008
};


class CJuikImage : public CEikImage 
{
public:
	static CJuikImage* NewL(CGulIcon* aIcon)
	{
		auto_ptr<CGulIcon> icon(aIcon);
		auto_ptr<CJuikImage> self( new (ELeave) CJuikImage(icon.get()));
		icon.release();
		self->ConstructL();
		return self.release();
	}
	
	CJuikImage( CGulIcon* aIcon ) : CEikImage(), iIcon(aIcon) {}

	TSize MinimumSize()
	{
		return TSize(10,10);
	}
		

	virtual ~CJuikImage() {
		delete iIcon;
	}

	
	void ConstructL()
	{
		SetPictureOwnedExternally(ETrue); 
		SetPicture( iIcon->Bitmap(), iIcon->Mask() );
	}

// 	TSize MinimumSize()
// 	{
// 		return TSize(0,0);
// 	}

	void SizeChanged()
	{
		TRect r = Rect();
		AknIconUtils::SetSize(iIcon->Bitmap(), r.Size());
		SetNewBitmaps(iIcon->Bitmap(), iIcon->Mask());
		CEikImage::SizeChanged();
	}

private:
	CGulIcon* iIcon; // not owned
   
};


class CListContainerImpl : public CListContainer, public MContextBase
{
public:
	static CListContainerImpl* NewL(MObjectProvider* aMopParent, const TRect& aRect)
	{
		CALLSTACKITEMSTATIC_N(_CL("CListContainerImpl"), _CL(""));
		auto_ptr<CListContainerImpl> self( new (ELeave) CListContainerImpl() );
		self->ConstructL(aMopParent, aRect);
		return self.release();
	}
	
	
 	CListContainerImpl() {}
	
	void ConstructL(MObjectProvider *aMopParent, const TRect& aRect)
	{
		CALLSTACKITEM_N(_CL("CListContainerImpl"), _CL(""));
		SetMopParent( aMopParent );
		CreateWindowL();				
		{ 
			TRect rect=aRect;
			rect.Move( 0, -rect.iTl.iY );
			iBackground=CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain,
																  rect, EFalse );
		}
		
		InitDaThangL();
		SetRect( aRect );
		ActivateL();
	}
		


	TInt CountComponentControls() const
	{
	    CALLSTACKITEM_N(_CL("CListContainerImpl"), _CL("CountComponentControl"));
		return iLabels.Count(); 
	}
	
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CListContainerImpl"), _CL("ComponentControl"));
		return iLabels[aIndex]; 
	}
	

	
	~CListContainerImpl() 
	{
		delete iBackground;
		delete iSizer;
		delete iSizer2;
		iLabels.ResetAndDestroy();
		iImages.ResetAndDestroy();
	}

	void SizeChanged()
	{
		TRect r = Rect();
		iSizer->SetDimensionL( r.iTl, r.Size() );
	}


	void Draw(const TRect& aRect) const
	{
		CWindowGc& gc = SystemGc();
		
		AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, gc, aRect );

		if ( iCurrentItem >= 0 && iCurrentItem < iLabels.Count() )
			{

				// Non-skin focus
				TRect focusRect = iSizer->GetItemL( iCurrentItem ).Rect();
				TRect innerRect = focusRect;
				innerRect.Shrink(8,8); 
				
				MAknsSkinInstance* skin = AknsUtils::SkinInstance();
				TBool skinnedFocus = AknsDrawUtils::DrawFrame( skin, gc, focusRect, innerRect, 
															   KAknsIIDQsnFrGrid, KAknsIIDDefault);
				if (!skinnedFocus)
					{
						gc.SetPenStyle( CGraphicsContext::ENullPen );
						gc.SetBrushColor( TRgb(200, 200, 200, 100) );
						gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
						gc.DrawRect( focusRect );
					}
			}
		
		
	}

	void ChangeCurrentItemL(TInt aDelta)
	{
		iCurrentItem += aDelta;
		iCurrentItem = Max(iCurrentItem, 0);
		iCurrentItem = Min(iCurrentItem, ItemCount() - 1 );
	}

	TInt ItemCount() 
	{
		return iLabels.Count();
	}

	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
 		if ( aType == EEventKey )
 			{
 				if      ( aKeyEvent.iCode==JOY_DOWN )
 					{
						ChangeCurrentItemL( +1 );
					}
				else if ( aKeyEvent.iCode == JOY_UP )
					{
						ChangeCurrentItemL( -1 );
					}
				DrawDeferred();
				return EKeyWasConsumed;
			}
		return EKeyWasNotConsumed;
	}


	
private:
	CAknsBasicBackgroundControlContext* iBackground;


	CEikLabel* CreateLabelL(const TDesC& aText, TRgb aColor)
	{
		auto_ptr<CEikLabel> label( new (ELeave) CEikLabel() );
		label->SetContainerWindowL( *this );
		// Mandatory? setup
		label->SetFont( CEikonEnv::Static()->DenseFont() );
		label->OverrideColorL( EColorLabelText, aColor );
		label->SetTextL( aText );

		TPtrC ptr( aText );
		TSize sz = label->CalcMinimumSize( ptr );
		label->SetRect( sz );
		sz = label->Size();
		return label.release();
	}

	TInt proportion;
	void CreateAndAddLabelL( const TDesC& aText, TRgb aColor )
	{
		auto_ptr<CEikLabel> l = CreateLabelL( aText, aColor );
		iSizer->AddL( *l, proportion, Juik::EExpand ); // Juik::EAlignCenterHorizontal );
		iLabels.AppendL( l.get() );
		l.release();

	}




	void InitDingDongL()
	{
		iSizer2 = Juik::CreateBoxSizerL(Juik::EHorizontal);
		
		auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(10) );
		const TInt KIconCount = 1;
		const TIconID KIconIds[KIconCount]= {
			_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
							EMbmContextcontactsuiWelcome_peacehand,
							EMbmContextcontactsuiWelcome_peacehand_mask ),
		};
		
		JuikIcons::LoadIconsL( icons.get(), KIconIds, KIconCount );
		
		while (icons->Count() > 0) 
			{
				auto_ptr<CJuikImage> image( CJuikImage::NewL( icons->At(0) ) );
				icons->Delete(0);
				image->SetContainerWindowL(*this);
				iSizer2->AddL( *image, 1, Juik::EExpand );
				iImages.AppendL( image.get() );
				image.release();
			}
		
		auto_ptr<CEikLabel> l = CreateLabelL( _L("Header1"), KRgbRed );
		iSizer2->AddL( *l, 1, Juik::EExpand ); // Juik::EAlignCenterHorizontal );
		iLabels.AppendL( l.get() );
		l.release();
		
		l.reset( CreateLabelL( _L("Header2"), KRgbBlack ) );
		iSizer2->AddL( *l, 0, Juik::EExpand ); // Juik::EAlignCenterHorizontal );
		iLabels.AppendL( l.get() );
		l.release();
	}
			 
	void InitDaThangL()
	{
		iSizer = Juik::CreateBoxSizerL(Juik::EVertical);
		InitDingDongL();

		proportion=1;
		iSizer->AddL( *iSizer2, proportion, Juik::EExpand );

		proportion=1;
		CreateAndAddLabelL(_L("Red"),KRgbRed);
		CreateAndAddLabelL(_L("Blue"),KRgbBlue);

		proportion=1;
		CreateAndAddLabelL(_L("Black"),KRgbBlack);
		CreateAndAddLabelL(_L("Yellow"),KRgbYellow);
		CreateAndAddLabelL(_L("Green"),KRgbGreen);
	}

	RPointerArray<CEikLabel> iLabels;
	MJuikSizer* iSizer;
	MJuikSizer* iSizer2;
	TInt iCurrentItem;
	RPointerArray<CJuikImage> iImages;

};


CListContainer* CListContainer::NewL(MObjectProvider* aMopParent, const TRect& aRect)
	{
		return CListContainerImpl::NewL( aMopParent, aRect );
	}
