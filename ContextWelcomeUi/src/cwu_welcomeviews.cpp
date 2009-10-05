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

#include "cwu_welcomeviews.h"

#include <ContextWelcomeUi.rsg>
#include <contextwelcomeui.mbg>
#include "ContextWelcomeUi.hrh"

#include "callstack.h"
#include "break.h"
#include "juik_layout.h"
#include "juik_icons.h"
#include "jaiku_layoutids.hrh"
#include "symbian_auto_ptr.h"
#include "reporting.h"

#include <AknBidiTextUtils.h>
#include <AknIconArray.h> 
#include <aknutils.h>
#include <aknviewappui.h>
#include <aknwaitdialog.h>

#include <eiklabel.h>
#include <gulicon.h>


static void DrawIconL(CWindowGc& aGc, CGulIcon& aIcon, const TJuikLayoutItem& aL, TBool aDoCenter=ETrue) 
{
	CALLSTACKITEMSTATIC_N(_CL(""), _CL("DrawIconL"));
	CFbsBitmap* bmp = aIcon.Bitmap();
	CFbsBitmap* mask = aIcon.Mask();
	
	aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
	// center 
	TInt bmpW = bmp->SizeInPixels().iWidth;
	TInt areaW = aL.Size().iWidth;
	
	TInt dx = 0;
	TInt dy = 0;
	if ( aDoCenter && bmpW < areaW )
		{
			dx = (areaW - bmpW) / 2;
		}


	TPoint tl = aL.TopLeft();
	tl += TPoint(dx,dy);
	TRect r(TPoint(0,0), bmp->SizeInPixels());
 	if ( mask )
 		{
 			aGc.BitBltMasked( tl, bmp, r, mask, ETrue);
 		}
 	else
 		{
			aGc.BitBlt( tl, bmp, r);
  		}
}



CWelcomePageBase::CWelcomePageBase() : iHeaderGraphicsIndex(KErrNotFound)
{
}

CWelcomePageBase::~CWelcomePageBase()
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("~CWelcomePageBase"));
	iControls.Close(); // Controls are owned by subclasses
	delete iMainText;
	delete iTitleText;
	delete iLeftSoftkey;
	delete iRightSoftkey;
	delete iIcons;	
	delete iBodyText;
}


enum TWelcomeIconIndex {
	EJaikuLogo = 0,
	EWelcomeToJaiku,
	EHeaderBackground,
	EHeaderLogo,
	ECongratulationsLogo,
	ECongratulationsText
};


const TInt KIconCount = 14;
const TIconID KIconIds[KIconCount]= {
	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_jaiku_logo,
					EMbmContextwelcomeuiWelcome_jaiku_logo_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_text_welcometojaiku,
					EMbmContextwelcomeuiWelcome_text_welcometojaiku_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_background,
					EMbmContextwelcomeuiWelcome_header_background_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_logo,
					EMbmContextwelcomeuiWelcome_header_logo_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_peacehand,
					EMbmContextwelcomeuiWelcome_peacehand_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_text_congratulations,
					EMbmContextwelcomeuiWelcome_text_congratulations_mask ),

 	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_text_accesspoint,
					EMbmContextwelcomeuiWelcome_header_text_accesspoint_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_text_bluetooth,
					EMbmContextwelcomeuiWelcome_header_text_bluetooth_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_text_calendar,
					EMbmContextwelcomeuiWelcome_header_text_calendar_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_text_login,
					EMbmContextwelcomeuiWelcome_header_text_login_mask ),
	
	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_text_yournumber,
					EMbmContextwelcomeuiWelcome_header_text_yournumber_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_text_internet,
					EMbmContextwelcomeuiWelcome_header_text_internet_mask ),

	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_text_autostart,
					EMbmContextwelcomeuiWelcome_header_text_autostart_mask ),
					
	_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
					EMbmContextwelcomeuiWelcome_header_battery_usage,
					EMbmContextwelcomeuiWelcome_header_battery_usage_mask ),
};

const TJuikLayoutId KLayoutIds[KIconCount] = 
	{
		{ LG_welcome_page, LI_welcome_page__jaiku_logo },
		{ LG_welcome_page, LI_welcome_page__welcometojaiku },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_bg },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_logo },
		{ LG_welcome_congratulations_page, LI_welcome_congratulations_page__logo },
		{ LG_welcome_congratulations_page, LI_welcome_congratulations_page__text },

		{ LG_welcome_selection_page, LI_welcome_selection_page__header_text },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_text },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_text },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_text },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_text },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_text },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_text },
		{ LG_welcome_selection_page, LI_welcome_selection_page__header_text },
	};

const TBool KStretchIcon[KIconCount] =
	{
		EFalse,
		EFalse,
		ETrue,
		EFalse,
		EFalse,
		EFalse,

		EFalse,
		EFalse,
		EFalse,
		EFalse,
		EFalse,
		EFalse,
		EFalse,
		EFalse,
	};

TRgb Gray()
{
	return TRgb(100,100,100);
}

TRgb DarkGray()
{
	return TRgb(0x33,0x33,0x33);
}

const CFont* FocusFont()
{
	return CEikonEnv::Static()->DenseFont();
}

const CFont* NonFocusFont()
{
	return CEikonEnv::Static()->DenseFont();
}


void CWelcomePageBase::ConstructL(	MPageObserver& aObserver )
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("ConstructL"));

	iObserver = &aObserver;
	CreateWindowL();	

	iPageRect = TRect( TPoint(0,0), MJuikLayout::ScreenSize() );
		
	iIcons = new (ELeave) CAknIconArray(KIconCount);

	Reporting().DebugLog(_L("LoadIcons"));
	JuikIcons::LoadIconsL( iIcons, KIconIds, KIconCount );

	TJuikLayoutItem l = 
		Layout().GetLayoutItemL(LG_welcome_selection_page, LI_welcome_selection_page__body_text);
		
	iMainText = new (ELeave) CEikLabel();
	iMainText->SetContainerWindowL( *this );
	iMainText->SetFont( l.Font() );
	if ( iLayoutStyle == ESelectionLayout ) 
		{
			iMainText->OverrideColorL(EColorLabelText, DarkGray());	
			//iMainText->SetLabelAlignment( ELayoutAlignCenter );
		}
	else if ( iLayoutStyle == EWelcomeLayout )
		{
			iMainText->OverrideColorL(EColorLabelText, Gray());	
			iMainText->SetLabelAlignment( ELayoutAlignCenter );
		}
	else if ( iLayoutStyle == ECongratulationsLayout )
		{
			iMainText->OverrideColorL(EColorLabelText, Gray());	
			iMainText->SetLabelAlignment( ELayoutAlignCenter );
		}
	else
		{
			ASSERT( EFalse );
		}

	iControls.Append(iMainText);

	
	l = 
		Layout().GetLayoutItemL(LG_welcome_softkeys,
								 LI_welcome__leftsoftkey);
	iLeftSoftkey = new (ELeave) CEikLabel();
	iLeftSoftkey->SetContainerWindowL( *this );
	iLeftSoftkey->SetFont( l.Font() );
	iLeftSoftkey->OverrideColorL(EColorLabelText, DarkGray());	
	iLeftSoftkey->SetTextL( _L("Continue") );
	iControls.Append(iLeftSoftkey);

	l = 
		Layout().GetLayoutItemL(LG_welcome_softkeys,
								 LI_welcome__rightsoftkey);
	iRightSoftkey = new (ELeave) CEikLabel();
	iRightSoftkey->SetContainerWindowL( *this );
	iRightSoftkey->SetFont( l.Font() );
	iRightSoftkey->OverrideColorL(EColorLabelText, DarkGray());	
	iRightSoftkey->SetTextL( _L("") );
	iControls.Append(iRightSoftkey);

	

}

TInt TextTagToIconIndex( CWelcomePageBase::THeaderText aTextTag )
{
	switch ( aTextTag )
		{
		case CWelcomePageBase::EHeaderText_Login: 
		case CWelcomePageBase::EHeaderText_AccessPoint:
		case CWelcomePageBase::EHeaderText_YourNumber:
		case CWelcomePageBase::EHeaderText_Calendar:
		case CWelcomePageBase::EHeaderText_Bluetooth:
		case CWelcomePageBase::EHeaderText_Internet:
		case CWelcomePageBase::EHeaderText_AutoStart:
		case CWelcomePageBase::EHeaderText_BatteryUsage:
			return aTextTag + 6;
		}
	return KErrNotFound;
}



void CWelcomePageBase::SetHeaderTextL( CWelcomePageBase::THeaderText aTextTag )
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("SetHeaderTextL"));
	if ( aTextTag == CWelcomePageBase::EHeaderText_NoText )
		{
			iHeaderGraphicsIndex = KErrNotFound;
		}
	else
		{
		iHeaderGraphicsIndex = TextTagToIconIndex(aTextTag);
		if (iHeaderGraphicsIndex < 0) User::Leave( KErrNotFound );
		}
}


void CWelcomePageBase::UpdateLayoutsL()
{
    iPageRect = TRect( TPoint(0,0), MJuikLayout::ScreenSize() );
    WrapBodyTextL();
    SetRect(iPageRect);
}


void CWelcomePageBase::SetMainTextL( const TDesC& aTxt )
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("SetMainTextL"));
	delete iBodyText;
	iBodyText = NULL;
	iBodyText = aTxt.AllocL();
	WrapBodyTextL();

}

void CWelcomePageBase::WrapBodyTextL()
{
  CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("WrapBodyTextL"));
  TJuikLayoutItem parent = TJuikLayoutItem(iPageRect);
  
  TJuikLayoutItem l;
	if ( iLayoutStyle == ESelectionLayout )
		{
			l = parent.Combine(Layout().GetLayoutItemL(LG_welcome_selection_page,
													   LI_welcome_selection_page__body_text));
		}
	else if ( iLayoutStyle == EWelcomeLayout )
		{
			l = parent.Combine(Layout().GetLayoutItemL(LG_welcome_page,
													   LI_welcome_page__body_text));
		}
	else if ( iLayoutStyle == ECongratulationsLayout )
		{
			l = parent.Combine(Layout().GetLayoutItemL(LG_welcome_congratulations_page,
													   LI_welcome_congratulations_page__body_text));
		}
	else
		{
			ASSERT(EFalse);
		}
	auto_ptr<HBufC> wrapped( HBufC::NewL( iBodyText->Length() + 100 ) );

	//const TInt KLineHeight = 12;
	const TInt KLineWidth = l.Rect().Size().iWidth;
	const TInt KLines = 9; //l.Rect().Size().iHeight / KLineHeight;
	
	auto_ptr< CArrayFix<TInt> > lineWs( new CArrayFixFlat<TInt>(5) );
	lineWs->AppendL(KLineWidth, KLines);
	const CFont* font = iMainText->Font();
	TPtr des = wrapped->Des();
	AknBidiTextUtils::ConvertToVisualAndWrapToStringL( *iBodyText,
							  *lineWs,
							  *font, 
							  des,
							  EFalse);
	iMainText->SetTextL( *wrapped );

}

enum TSoftkeyLocation {
	ESoftkeyTopLeft,
	ESoftkeyTopRight,
	ESoftkeyBottomLeft,
	ESoftkeyBottomRight
};
	

TRect TransferedToCorner( TSoftkeyLocation aLocation, const TRect& aRect ) 
{
	TSize screenSz = MJuikLayout::ScreenSize();
	TSize rectSz = aRect.Size();

	TInt xM = aRect.iTl.iX;
	TInt yM = aRect.iTl.iY;

	TInt lX = xM;
	TInt tY = yM;

	TInt rX = screenSz.iWidth  - rectSz.iWidth - xM;
	TInt bY = screenSz.iHeight - rectSz.iHeight - yM;
		
	TPoint p;
	switch ( aLocation )
		{
		case ESoftkeyTopLeft:
			p = TPoint( lX, tY );
			break;
		case ESoftkeyTopRight:
			p = TPoint( rX, tY );
			break;
		case ESoftkeyBottomLeft:
			p = TPoint( lX, bY);
			break;
		case ESoftkeyBottomRight:
			p = TPoint( rX, bY);
			break;
		};
	return TRect( p, rectSz );
}

void CWelcomePageBase::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("SizeChanged"));
	TJuikLayoutItem parent(Rect());
	TJuikLayoutItem l;

	for (TInt i=0; i < KIconCount; i++)
		{
			TJuikLayoutId id = KLayoutIds[i];
			l = parent.Combine(Layout().GetLayoutItemL(id.iGroup, id.iItem));
			AknIconUtils::SetSize( iIcons->At(i)->Bitmap(), l.Rect().Size(), 
								   KStretchIcon[i] ? EAspectRatioNotPreserved : EAspectRatioPreservedAndUnusedSpaceRemoved);
		}
   
	if ( iLayoutStyle == ESelectionLayout )
		{
			l = parent.Combine(Layout().GetLayoutItemL(LG_welcome_selection_page,
														LI_welcome_selection_page__body_text ));
		}
	else if ( iLayoutStyle == EWelcomeLayout )
		{
			l = parent.Combine(Layout().GetLayoutItemL(LG_welcome_page,
														LI_welcome_page__body_text));
		}
	else if ( iLayoutStyle == ECongratulationsLayout )
		{
			l = parent.Combine(Layout().GetLayoutItemL(LG_welcome_congratulations_page,
													   LI_welcome_congratulations_page__body_text));
		}
	else 
		{
			ASSERT(EFalse);
		}
	iMainText->SetRect( l.Rect() );

	
	
	// Layout depending on softkey placement.

	l = parent.Combine(Layout().GetLayoutItemL(LG_welcome_softkeys,
												LI_welcome__leftsoftkey) );
	TRect leftR = l.Rect();

	l = parent.Combine(Layout().GetLayoutItemL(LG_welcome_softkeys,
												LI_welcome__rightsoftkey) );
	TRect rightR = l.Rect(); 
	

	// ELayoutAlignNone, ELayoutAlignCenter, ELayoutAlignLeft, ELayoutAlignRight, ELayoutAlignBidi } defined in Avkon.hrh

	TInt lkAlign = ELayoutAlignCenter;
	TInt rkAlign = ELayoutAlignCenter;
	
	switch ( AknLayoutUtils::CbaLocation() )
		{
		case AknLayoutUtils::EAknCbaLocationBottom: 
			rightR = TransferedToCorner( ESoftkeyBottomRight, rightR );
			leftR  = TransferedToCorner( ESoftkeyBottomLeft, leftR  );

			lkAlign = ELayoutAlignLeft;
			rkAlign = ELayoutAlignRight;
			break;
		case AknLayoutUtils::EAknCbaLocationRight:
			rightR = TransferedToCorner( ESoftkeyTopRight, rightR );
			leftR  = TransferedToCorner( ESoftkeyBottomRight, leftR  );
			
			lkAlign = ELayoutAlignRight;
			rkAlign = ELayoutAlignRight;
			break;
		case AknLayoutUtils::EAknCbaLocationLeft: 
			rightR = TransferedToCorner( ESoftkeyTopLeft, rightR );
			leftR  = TransferedToCorner( ESoftkeyBottomLeft, leftR  );

			lkAlign = ELayoutAlignLeft;
			rkAlign = ELayoutAlignLeft;
			break;
		}
	iLeftSoftkey->SetRect( leftR );
	iLeftSoftkey->SetLabelAlignment( lkAlign );

	iRightSoftkey->SetRect( rightR );
	iRightSoftkey->SetLabelAlignment( rkAlign );
}

	
void CWelcomePageBase::Draw( const TRect& /*aRect*/ ) const
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("Draw"));
	CWindowGc& gc = SystemGc();
	gc.SetPenColor(KRgbWhite);
	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect( Rect() );
	
 	TJuikLayoutItem parent(Rect());
	TJuikLayoutItem l;

	if ( iLayoutStyle == ESelectionLayout )
		{
			DrawGraphicsL(gc, EHeaderBackground, parent);
			DrawGraphicsL(gc, EHeaderLogo, parent);
			if ( iHeaderGraphicsIndex != KErrNotFound ) DrawGraphicsL(gc, iHeaderGraphicsIndex, parent, EFalse);
		}
	else if ( iLayoutStyle == EWelcomeLayout )
		{
			DrawGraphicsL(gc, EJaikuLogo, parent);
			DrawGraphicsL(gc, EWelcomeToJaiku, parent);
		}
	else if ( iLayoutStyle == ECongratulationsLayout )
		{
			DrawGraphicsL(gc, ECongratulationsLogo, parent);
			DrawGraphicsL(gc, ECongratulationsText, parent);
		}
	else
		{
			ASSERT(EFalse);
		}
}

void CWelcomePageBase::DrawGraphicsL(CWindowGc& aGc, TInt aIndex, const TJuikLayoutItem& aParent, TBool aDoCenter) const
{	
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("DrawGraphicsL"));
	CGulIcon* icon = iIcons->At(aIndex);
	TJuikLayoutId id = KLayoutIds[aIndex];
	TJuikLayoutItem l = aParent.Combine( Layout().GetLayoutItemL( id.iGroup, id.iItem ) );
	DrawIconL(aGc, *icon, l, aDoCenter);
}


void CWelcomePageBase::DrawRectL(CWindowGc& aGc, const TRgb& aColor, const TJuikLayoutItem& aL) const
{
	aGc.SetBrushColor( aColor );
	aGc.DrawRect( aL.Rect() );	
}


TInt CWelcomePageBase::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("CountComponentControls"));
	return iControls.Count();
}

CCoeControl* CWelcomePageBase::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("ComponentControl"));
	return iControls[aIndex];
}

void CWelcomePageBase::LeftSoftKeyL()
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("LeftSoftKeyL"));
	iObserver->LeftSoftKeyL( KErrNotFound );
}

void CWelcomePageBase::RightSoftKeyL()
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("RightSoftKeyL"));
	iObserver->RightSoftKeyL( KErrNotFound );
}

TBool CWelcomePageBase::IsE90() {
	TSize e90size(800,352);
	TBool e90 = EFalse;
	CEikAppUi* appui = (CEikAppUi*)iEikonEnv->AppUi();
	if (appui->ApplicationRect() == e90size) {
	  e90 = ETrue;
	}
	return e90;
}


TKeyResponse CWelcomePageBase::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CWelcomePageBase"), _CL("OfferKeyEventL"));
	const TBool e90 = IsE90();
    if( aType == EEventKey )
		{
 			if ( aKeyEvent.iCode == EKeyDevice0 )
 				{
 					if (!e90) LeftSoftKeyL();
 					else RightSoftKeyL();
					return EKeyWasConsumed;
				}
			else if ( aKeyEvent.iCode == EKeyDevice1 )
 				{
 					if (!e90) RightSoftKeyL();
 					else LeftSoftKeyL();
					return EKeyWasConsumed;
				}
			else if ( aKeyEvent.iCode == EKeyDevice3)
				{
					LeftSoftKeyL();
					return EKeyWasConsumed;
				}


// 					return EKeyWasConsumed;
// 				}
// 			else if ( aKeyEvent.iCode == EKeyRightArrow )
// 				{
// 					iObserver->RightSoftKeyL( KErrNotFound );
// 					return EKeyWasConsumed;
// 				}
		}
	return EKeyWasNotConsumed;
}


//
// CWelcomeViewBase
//

CWelcomeViewBase* CWelcomeViewBase::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CWelcomeViewBase"), _CL("NewL"));
	auto_ptr<CWelcomeViewBase> self( new (ELeave) CWelcomeViewBase );
	self->ConstructL();
	return self.release();
}

void CWelcomeViewBase::ConstructL()
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("ConstructL"));
	//iCurrentPage = -1;
	iInStack = EFalse;
	iIsDone = EFalse;
	BaseConstructL(R_WELCOMEUI_VIEW);
}


TUid CWelcomeViewBase::Id() const
{
	// Can't use callstack because this functions is called 
	// during view destruction
	// CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("Id"));
	return KWelcomeUiViewId;
}


CWelcomeViewBase::~CWelcomeViewBase()
{
	CC_TRAPD(err, ReleaseCWelcomeViewBase());
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
}


void CWelcomeViewBase::ReleaseCWelcomeViewBase()
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("ReleaseCWelcomeViewBase"));
	delete iWaitDialog;
	if ( iPage && iInStack )
		{
			AppUi()->RemoveFromViewStack( *this, iPage ); 
		}
	delete iPage;			
}

void CWelcomeViewBase::HidePageL(TBool aHide ) // FIXME: rename or split to two functions? 
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("HidePageL"));
	if ( iPage )
		{
			iPage->MakeVisible( !aHide );
		}
}

void CWelcomeViewBase::SetPageL(CWelcomePageBase* aPage)
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("SetPageL"));
	if ( iPage )
		{
			ChangePageL( aPage );
		}
	else
		{
			SetFirstPageL( aPage ); 
		}
}


void CWelcomeViewBase::ShowWaitDialogL( TInt aNoteResource )
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("ShowWaitDialogL"));
	if ( ! iWaitDialog )
		{
			iWaitDialog = new(ELeave)CAknWaitDialog( (REINTERPRET_CAST(CEikDialog**,&iWaitDialog)));
			iWaitDialog->SetTone( CAknNoteDialog::EConfirmationTone );
			iWaitDialog->ExecuteLD( aNoteResource );
		}
}


void CWelcomeViewBase::StopWaitDialogL()
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("StopWaitDialogL"));
	if ( iWaitDialog )
		{
			iWaitDialog->ProcessFinishedL();
		}
}

MObjectProvider& CWelcomeViewBase::ObjectProviderL()
{
	return *this;
}


void CWelcomeViewBase::SetFirstPageL( CWelcomePageBase* aPage )
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("SetFirstPageL"));
	iPage = aPage;
	iIsDone = EFalse;
}


void CWelcomeViewBase::ChangePageL( CWelcomePageBase* aPage )
{	
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("ChangePageL"));

	auto_ptr<HBufC> stack( CallStackMgr().GetFormattedCallStack(KNullDesC) );
	Reporting().UserErrorLog(*stack);

	auto_ptr<CWelcomePageBase> oldPage( iPage );
 	iPage = aPage;
	
	iPage->MakeVisible( ETrue );
 	AppUi()->AddToStackL( *this, iPage );
	iInStack = ETrue;	
    iIsDone = EFalse;
	if ( oldPage.get() )
		{
			oldPage->MakeVisible(EFalse);
			AppUi()->RemoveFromViewStack( *this, oldPage.get() );
		}

	Reporting().UserErrorLog(_L("ChangePageL is over") );
}


void CWelcomeViewBase::DoActivateL(const TVwsViewId& /*aPrevViewId*/,
								 TUid /*aCustomMessageId*/,
								 const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("DoActivateL"));



	auto_ptr<HBufC> stack( CallStackMgr().GetFormattedCallStack(KNullDesC) );
	Reporting().UserErrorLog(*stack);


	if ( iPage )
		{
 			iPage->MakeVisible( ETrue );
 			AppUi()->AddToStackL( *this, iPage );
			iInStack = ETrue;
		}

	Reporting().UserErrorLog(_L("DoActivateL is over") );
}


void CWelcomeViewBase::HandleResourceChangeL( TInt aType )
{
    if ( aType == KEikDynamicLayoutVariantSwitch )
	{
	    if ( iPage )
		{
		    iPage->UpdateLayoutsL();
		    iPage->DrawDeferred();
		}
	}
}


void CWelcomeViewBase::DoDeactivate()
{
	//CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("DoDeactivate"));
	if (iPage)
		{
			AppUi()->RemoveFromViewStack( *this, iPage ); 
			iInStack = EFalse;
			iPage->MakeVisible(EFalse);

		}
}


void CWelcomeViewBase::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CWelcomeViewBase"), _CL("HandleCommandL"));
	AppUi()->HandleCommandL(aCommand);
}





//
// CWelcomeInfoPage 
//


CWelcomeInfoPage* CWelcomeInfoPage::NewL(MObjectProvider& aMopParent, 
										 const TDesC& aText,
										 MPageObserver& aObserver,
										 CWelcomePageBase::THeaderText aHeader, 
										 const TDesC& aSoftkeyText,
										 TBool aCongratulationsLayout)
{
	CALLSTACKITEMSTATIC_N(_CL("CWelcomeInfoPage"), _CL("NewL"));
	auto_ptr<CWelcomeInfoPage> self( new (ELeave) CWelcomeInfoPage);
	self->ConstructL(aMopParent, aText, aObserver, aSoftkeyText, aCongratulationsLayout);
	self->SetHeaderTextL( aHeader );
	return self.release();
}

CWelcomeInfoPage::CWelcomeInfoPage() 
{
	iLayoutStyle = CWelcomePageBase::CWelcomePageBase::ECongratulationsLayout;
	CALLSTACKITEM_N(_CL("CWelcomeInfoPage"), _CL("CWelcomeInfoPage"));
}

CWelcomeInfoPage::~CWelcomeInfoPage()
{
	CALLSTACKITEM_N(_CL("CWelcomeInfoPage"), _CL("~CWelcomeInfoPage"));
}

void CWelcomeInfoPage::ConstructL(MObjectProvider& aMopParent, 
								  const TDesC& aText,
								  MPageObserver& aObserver,
								  const TDesC& aSoftkeyText,
								  TBool aCongratulationsLayout)
{
	CALLSTACKITEM_N(_CL("CWelcomeInfoPage"), _CL("ConstructL"));
	if ( aCongratulationsLayout )
		{
		iLayoutStyle = CWelcomePageBase::ECongratulationsLayout;
		}
	else
		{
		iLayoutStyle = CWelcomePageBase::ESelectionLayout;
		}


	SetMopParent( &aMopParent );
	CWelcomePageBase::ConstructL( aObserver );	
	_LIT( KContinue, "Continue") ;
	TPtrC softkey( aSoftkeyText.Length() ? aSoftkeyText : KContinue );
	
	iLeftSoftkey->SetTextL( softkey );
	iRightSoftkey->SetTextL( _L("") );
	SetMainTextL(aText);	
	SetRect( iPageRect );
	ActivateL();
}



//
// CWelcomeIntroPage 
//


CWelcomeIntroPage* CWelcomeIntroPage::NewL(MObjectProvider& aMopParent, 
										 const TDesC& aText,
										 MPageObserver& aObserver)
{
	CALLSTACKITEMSTATIC_N(_CL("CWelcomeIntroPage"), _CL("NewL"));
	auto_ptr<CWelcomeIntroPage> self( new (ELeave) CWelcomeIntroPage);
	self->ConstructL(aMopParent, aText, aObserver);
	return self.release();
}

CWelcomeIntroPage::CWelcomeIntroPage() 
{
	iLayoutStyle = CWelcomePageBase::EWelcomeLayout;
	CALLSTACKITEM_N(_CL("CWelcomeIntroPage"), _CL("CWelcomeIntroPage"));
}

CWelcomeIntroPage::~CWelcomeIntroPage()
{
	CALLSTACKITEM_N(_CL("CWelcomeIntroPage"), _CL("~CWelcomeIntroPage"));
}

void CWelcomeIntroPage::ConstructL(MObjectProvider& aMopParent, 
								   const TDesC& aText,
								   MPageObserver& aObserver)
{
	CALLSTACKITEM_N(_CL("CWelcomeIntroPage"), _CL("ConstructL"));
	SetMopParent( &aMopParent );
	CWelcomePageBase::ConstructL( aObserver );	
	iLeftSoftkey->SetTextL( _L("Continue") );
	iRightSoftkey->SetTextL( _L("") );
	SetMainTextL(aText);	
	SetRect( iPageRect );
	ActivateL();
}



class CSelectionList : public CCoeControl, public MContextBase
{
private:
	RPointerArray<CEikLabel> iLabels;
	CArrayPtrFlat<CGulIcon>* iIcons;

	TInt iFocusedItem;
public:
	static CSelectionList* NewL(CCoeControl& aParent, MDesCArray& aSelections)
	{
		CALLSTACKITEMSTATIC_N(_CL("CSelectionList"), _CL("NewL"));
		auto_ptr<CSelectionList> self( new(ELeave) CSelectionList);
		self->ConstructL(aParent, aSelections);
		return self.release();
	}
	
	CSelectionList() {}

	~CSelectionList() 
	{
		CALLSTACKITEM_N(_CL("CSelectionList"), _CL("~CSelectionList"));
		iLabels.ResetAndDestroy();
		if ( iIcons) iIcons->ResetAndDestroy();
		delete iIcons;
			
	}

	TInt Count() 
	{
		return CountComponentControls();
	}
	
	void ConstructL(CCoeControl& aParent, MDesCArray& aSelections)
	{
		CALLSTACKITEM_N(_CL("CSelectionList"), _CL("ConstructL"));
		SetContainerWindowL( aParent );
		const TInt KBgIconCount = 2;
		const TIconID KBgIconIds[KBgIconCount]= {
			_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
							EMbmContextwelcomeuiWelcome_focus_active,
							EMbmContextwelcomeuiWelcome_focus_active_mask ),
			_INIT_T_ICON_ID("C:\\system\\data\\contextwelcomeui.mif", 
							EMbmContextwelcomeuiWelcome_focus_inactive,
							EMbmContextwelcomeuiWelcome_focus_inactive_mask ),
		};
		iIcons = new (ELeave) CArrayPtrFlat<CGulIcon>(KBgIconCount);
		JuikIcons::LoadIconsL( iIcons, KBgIconIds, KBgIconCount );

		// bg and label layouts
		
		TJuikLayoutItem textL = Layout().GetLayoutItemL( LG_welcome_selection_listbox, LI_welcome_selection_listbox__item_text);
		for (TInt i=0; i < aSelections.MdcaCount(); i++)
			{
				auto_ptr<CEikLabel> label( new (ELeave) CEikLabel() );
				label->SetContainerWindowL( *this );
				label->SetFont( NonFocusFont() );
				label->OverrideColorL(EColorLabelText, Gray());	
				
				label->SetTextL( aSelections.MdcaPoint(i) );
				iLabels.AppendL( label.release() );
			}	   
		iLabels[iFocusedItem]->OverrideColorL(EColorLabelText, KRgbWhite);	
		iLabels[iFocusedItem]->SetFont( FocusFont() );	
		
	}
	
	
	TInt CountComponentControls() const
	{
		return iLabels.Count();
	}

	CCoeControl *ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CSelectionList"), _CL("ComponentControl"));
		return iLabels[aIndex];
	}
	
	void Draw( const TRect& /*aRect*/ ) const
	{
		CALLSTACKITEM_N(_CL("CSelectionList"), _CL("Draw"));
		MJuikLayout& layout = Layout();

		CWindowGc& gc = SystemGc();

		// draw items 
		TJuikLayoutItem itemL = Layout().GetLayoutItemL( LG_welcome_selection_listbox, LI_welcome_selection_listbox__item);
		TInt H = itemL.h;
		TRect pRect = Rect();
		for (TInt i=0; i < iLabels.Count(); i++)
			{
				CGulIcon* icon = iIcons->At( (i == iFocusedItem) ? 0 : 1 );
				TJuikLayoutItem parent( pRect );
				TJuikLayoutItem l = parent.Combine( itemL );
				DrawIconL(gc, *icon, l);
				pRect.Move(0, H);
			}		
	}

	TInt CurrentItemIndex()
	{
		return iFocusedItem;
	}


	void SizeChanged()  
	{
		CALLSTACKITEM_N(_CL("CSelectionList"), _CL("SizeChanged"));
		MJuikLayout& layout = Layout();
		TJuikLayoutItem parent( Rect() );

		// Label layouts
		TJuikLayoutItem itemL = Layout().GetLayoutItemL( LG_welcome_selection_listbox, LI_welcome_selection_listbox__item);
		TJuikLayoutItem textL =Layout().GetLayoutItemL( LG_welcome_selection_listbox, LI_welcome_selection_listbox__item_text);
		TInt H = itemL.h;
		TRect parentRect = Rect();
		for ( TInt i=0; i < iLabels.Count(); i++)
			{	
				TJuikLayoutItem p( parentRect );
				TJuikLayoutItem item = p.Combine( itemL ).Combine( textL );
				iLabels[i]->SetRect(item.Rect());
				parentRect.Move(0,H);
			}
		
		// Icon sizes, same size for all
		TJuikLayoutItem l = parent.Combine( itemL );
		
		for ( TInt i=0; i < iIcons->Count(); i++)
			{
				
				AknIconUtils::SetSize( iIcons->At(i)->Bitmap(), l.Rect().Size(), EAspectRatioNotPreserved); 
			}
	}

	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
	{
		CALLSTACKITEM_N(_CL("CSelectionList"), _CL("OfferKeyEventL"));
		if( aType == EEventKey && ( aKeyEvent.iCode == EKeyDownArrow || aKeyEvent.iCode == EKeyUpArrow) )
			{
				iLabels[iFocusedItem]->OverrideColorL(EColorLabelText, Gray());	
				iLabels[iFocusedItem]->SetFont( NonFocusFont() );
				if ( aKeyEvent.iCode == EKeyDownArrow ) iFocusedItem++;
				else if ( aKeyEvent.iCode == EKeyUpArrow ) iFocusedItem--;
				TInt max = iLabels.Count() - 1;
				if (iFocusedItem > max) iFocusedItem = max;
				if (iFocusedItem < 0 ) iFocusedItem = 0;
				iLabels[iFocusedItem]->OverrideColorL(EColorLabelText, KRgbWhite);
				iLabels[iFocusedItem]->SetFont(	FocusFont() );
				DrawDeferred();
				return EKeyWasConsumed;
			}
		return EKeyWasNotConsumed;
	}
};



CWelcomeSelectionPage* CWelcomeSelectionPage::NewL(MObjectProvider& aMopParent,
 												   const TDesC& aText,
												   MDesCArray& aSelections,
												   MPageObserver& aObserver,
												   CWelcomePageBase::THeaderText aHeader,
												   TBool aCongratulationsLayout)
{
	CALLSTACKITEMSTATIC_N(_CL("CWelcomeSelectionPage"), _CL("NewL"));
	auto_ptr<CWelcomeSelectionPage> self( new (ELeave) CWelcomeSelectionPage(aCongratulationsLayout));
	self->ConstructL(aMopParent, aText, aSelections, aObserver);
	self->SetHeaderTextL( aHeader );
	return self.release();
}

CWelcomeSelectionPage::CWelcomeSelectionPage(TBool aCongratulationsLayout) 
{
	if ( aCongratulationsLayout )
		{
		iLayoutStyle = CWelcomePageBase::ECongratulationsLayout;
		}
	else
		{
		iLayoutStyle = CWelcomePageBase::ESelectionLayout;
		}
	CALLSTACKITEM_N(_CL("CWelcomeSelectionPage"), _CL("CWelcomeSelectionPage"));
}

CWelcomeSelectionPage::~CWelcomeSelectionPage()
{
	CALLSTACKITEM_N(_CL("CWelcomeSelectionPage"), _CL("~CWelcomeSelectionPage"));
	delete iList;
}


void CWelcomeSelectionPage::ConstructL(MObjectProvider& aMopParent, 
									   const TDesC& aText,
									   MDesCArray& aSelections,
									   MPageObserver& aObserver)
{
	CALLSTACKITEM_N(_CL("CWelcomeSelectionPage"), _CL("ConstructL"));
	SetMopParent( &aMopParent );
	CWelcomePageBase::ConstructL( aObserver );	
	SetMainTextL(aText);
	iLeftSoftkey->SetTextL( _L("Select") );
	iRightSoftkey->SetTextL( _L("") );
	
	iList = CSelectionList::NewL( *this, aSelections );
	iControls.Append(iList);
	
	SetRect( iPageRect );
	ActivateL();
}



void CWelcomeSelectionPage::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CWelcomeSelectionPage"), _CL("SizeChanged"));
	CWelcomePageBase::SizeChanged();
	TJuikLayoutItem parent(Rect());
	TJuikLayoutItem l = 
		parent.Combine(Layout().GetLayoutItemL(LG_welcome_selection_listbox,LI_welcome_selection_listbox__listbox));	
	TJuikLayoutItem itemL = Layout().GetLayoutItemL( LG_welcome_selection_listbox, LI_welcome_selection_listbox__item);

	TInt H = iList->Count() * itemL.h;
	   
	l.y -= H;
	l.h = H;

	iList->SetRect( l.Rect() );
}

void CWelcomeSelectionPage::LeftSoftKeyL()
{
	CALLSTACKITEM_N(_CL("CWelcomeSelectionPage"), _CL("LeftSoftKeyL"));
	iObserver->LeftSoftKeyL( iList->CurrentItemIndex() );
}

void CWelcomeSelectionPage::RightSoftKeyL()
{
	CALLSTACKITEM_N(_CL("CWelcomeSelectionPage"), _CL("RightSoftKeyL"));
	iObserver->RightSoftKeyL( iList->CurrentItemIndex() );
}


TKeyResponse CWelcomeSelectionPage::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CWelcomeSelectionPage"), _CL("OfferKeyEventL"));
	const TBool e90 = IsE90();
	const TInt left_softkey_code = !e90 ? EKeyDevice0 : EKeyDevice1;
	const TInt right_softkey_code = !e90 ? EKeyDevice1 : EKeyDevice0;
    if( aType == EEventKey )
		{
			if ( aKeyEvent.iCode == EKeyDownArrow || aKeyEvent.iCode == EKeyUpArrow ) 
				{
					return iList->OfferKeyEventL(aKeyEvent, aType);
				}
			else if ( aKeyEvent.iCode == EKeyDevice3)
				{
					iObserver->SelectedItemL( iList->CurrentItemIndex() );
					return EKeyWasConsumed;
				}
 			else if ( aKeyEvent.iCode == left_softkey_code )
 				{
					iObserver->LeftSoftKeyL( iList->CurrentItemIndex() );
					return EKeyWasConsumed;
				}
			else if ( aKeyEvent.iCode == right_softkey_code )
 				{
					iObserver->RightSoftKeyL( iList->CurrentItemIndex() );
					return EKeyWasConsumed;
				}
		}
	return CWelcomePageBase::OfferKeyEventL( aKeyEvent, aType );
}

