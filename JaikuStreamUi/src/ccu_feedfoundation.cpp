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

#include "ccu_feedfoundation.h"

#include "ccu_feeditem.h"
#include "ccu_jaicons.h"
#include "ccu_themes.h"
#include "ccu_userpics.h"
#include "ccu_staticicons.h"
#include "ccu_streamrenderers.h"

#include <contextcontactsui.mbg>

#include "csd_feeditem.h"

#include "juik_gfxstore.h"
#include "juik_fonts.h"
#include "juik_icons.h"
#include "juik_image.h"
#include "juik_label.h"
#include "juik_layout.h"
#include "juik_sizer.h"
#include "juik_sizercontainer.h"
#include "juik_keycodes.h"
#include "jaiku_layoutids.hrh"

#include "app_context.h"
#include "break.h"
#include "reporting.h"
#include "symbian_auto_ptr.h"

#include <akniconutils.h>
#include <aknsdrawutils.h> 
#include <aknbiditextutils.h> 
#include <akniconutils.h>
#include <aknsutils.h> 
#include <eikenv.h>
#include <gulcolor.h> 
#include <gulicon.h>


CFeedControl::~CFeedControl() 
{
	CALLSTACKITEM_N(_CL("CFeedControl"), _CL("~CFeedControl"));
	ClearItem();
}


void CFeedControl::SetMinimumSizeObserverL(MMinimumSizeObserver* aObserver)
{
	iObserver = aObserver;
}


void CFeedControl::MinimumSizeChangedL()
{
	CALLSTACKITEM_N(_CL("CFeedControl"), _CL("MinimumSizeChangedL"));
	if ( iObserver )
		{
			TSize sz = MinimumSize();
			iObserver->MinimumSizeChangedL( this, sz );
		}
}

CFeedControl::CFeedControl(const TUiDelegates& aDelegates) : MFeedFoundation( aDelegates )
{
	CALLSTACKITEM_N(_CL("CFeedControl"), _CL("CFeedControl"));
}


void CFeedControl::FeedItemEvent(CBBFeedItem* aItem, TEvent aEvent)
{
	CALLSTACKITEM_N(_CL("CFeedControl"), _CL("FeedItemEvent"));
	if ( iFeedItem && iFeedItem == aItem )
		{
			switch ( aEvent )
				{
				case EUnreadChanged:
					UpdateUnreadL( *iFeedItem );
					DrawDeferred();
					break;

				case EChildCountChanged:
				case EErrorUpdated:
				case EPlaceholderFilled:
				case EFeedItemUpdated:
				case EFeedItemHidden:
				case EFeedItemVisible:
					UpdateL( *iFeedItem );
					DrawDeferred();
					break;
				}
		}
}

void CFeedControl::AuthorCountEvent(const TDesC& /*aAuthor*/,
									TInt /*aNewItemCount*/, 
									TInt /*aNewUnreadCount*/) 
{
}


void CFeedControl::InitL(CBBFeedItem* aItem)
{
	CALLSTACKITEM_N(_CL("CFeedControl"), _CL("InitL"));
	SetItem(aItem);
	if (iFeedItem)
		{
			UpdateL(*iFeedItem);
		}
}

void CFeedControl::ResetL()
{
	CALLSTACKITEM_N(_CL("CFeedControl"), _CL("ResetL"));
	ClearItem();
	ZeroL();
}


void CFeedControl::SetItem(CBBFeedItem* aItem)
{
	CALLSTACKITEM_N(_CL("CFeedControl"), _CL("SetItem"));
	ClearItem();
	iFeedItem = aItem;
	if ( iFeedItem )
		{
			iFeedItem->AddRef();
			FeedStorage().SubscribeL(this);
		}
}

	
void CFeedControl::ClearItem()
{
	CALLSTACKITEM_N(_CL("CFeedControl"), _CL("ClearItem"));
	if ( iFeedItem )
		{
			iFeedItem->Release();
			iFeedItem = NULL;
			FeedStorage().UnSubscribeL(this);
		}
}




const TInt KAtomJaiconId( 108 );


	
TRgb MFeedFoundation::PrimaryTextColor() const
{
	return ThemeColors().GetColorL( CThemeColors::EPrimaryText );
}

TRgb MFeedFoundation::SpecialPrimaryTextColor() const
{
	return ThemeColors().GetColorL( CThemeColors::ESpecialPrimaryText );
}

TRgb MFeedFoundation::SecondaryTextColor() const
{
	return ThemeColors().GetColorL( CThemeColors::ESecondaryText );
}

TRgb MFeedFoundation::PrimaryHighlightTextColor() const
{
	return ThemeColors().GetColorL( CThemeColors::EPrimaryHighlightText );
}
	
TRgb MFeedFoundation::SecondaryHighlightTextColor() const
{
	return ThemeColors().GetColorL( CThemeColors::ESecondaryHighlightText );
}


TRgb MFeedFoundation::BubbleColor()  const
{
	return ThemeColors().GetColorL( CThemeColors::EBubble );
	//return ThemeColors().GetColorL( CThemeColors::ESkinHighlight );
}

TRgb MFeedFoundation::BubbleFocusColor() const
{
	return ThemeColors().GetColorL( CThemeColors::EBubbleHighlight );
}

TRgb MFeedFoundation::CommentBubbleColor()  const
{
	TRgb c = ThemeColors().GetColorL( CThemeColors::EBubble );
	c.SetAlpha( 51 );
	return c;
	//return ThemeColors().GetColorL( CThemeColors::ESkinHighlight );
}

TRgb MFeedFoundation::CommentBubbleFocusColor()  const
{
	TRgb c = BubbleFocusColor();
	//c.SetAlpha( 128 );
	return c;
	//return ThemeColors().GetColorL( CThemeColors::ESkinHighlight );
}


class CCommentIndicatorImpl : public CCommentIndicator, public MContextBase, public MFeedFoundation
{
public:	
	static CCommentIndicatorImpl* NewL(const TUiDelegates& aDelegates)
	{
		CALLSTACKITEMSTATIC_N(_CL("CCommentIndicatorImpl"), _CL("NewL"));
		auto_ptr<CCommentIndicatorImpl> self( new (ELeave) CCommentIndicatorImpl(aDelegates) );
		self->ConstructL();
		return self.release();
	}
	
	CCommentIndicatorImpl(const TUiDelegates& aDelegates) : MFeedFoundation(aDelegates), iCommentCount(0) {}
	
	~CCommentIndicatorImpl()
	{
		CALLSTACKITEM_N(_CL("CCommentIndicatorImpl"), _CL("~CCommentIndicatorImpl"));
		for (TInt i=0; i < iEdgeGraphics.Count(); i++ )
			{ 
				CGulIcon* icon = iEdgeGraphics[i];
				if ( icon ) 
					Graphics().ReleaseIcon( icon );
			}
		iEdgeGraphics.Close();
	}

	const CFont* Font() const
	{
		return JuikFonts::Small();
	}
	
	TSize MinimumSize() 
	{
		CALLSTACKITEM_N(_CL("CCommentIndicatorImpl"), _CL("MinimumSize"));
		if ( iCommentCount == 0 )
			return TSize(0,0);
		TInt digits = 1 + (iCommentCount / 10);
		TInt w = Font()->WidthZeroInPixels() * digits;
		TInt h = Font()->FontCapitalAscent();
		TInt descent = 1; // numbers don't have descent, but let's add one to make it look better
		h += descent; 
		TSize sz(w + h,h);
		sz += iMargin.SizeDelta();
		return sz;
	}
	
	TComponentName IconName(TInt aIconId)
	{
		TInt id = EGfxCommentIndicator;
		id += aIconId;
		TComponentName name = { { CONTEXT_UID_CONTEXTCONTACTSUI }, id };
		return name;	
	}

	enum 
		{
			ELeftEdge = 0,
			ERightEdge,
			EFocusLeftEdge,
			EFocusRightEdge,
			EGfxCount
		};
	
	
	void GetCorrectSizeIconsL()
	{
		CALLSTACKITEM_N(_CL("CCommentIndicatorImpl"), _CL("GetCorrectSizeIconsL"));
		
		TInt height = MinimumSize().iHeight;
		TSize iconSize = TSize( height / 2, height );
		
		if ( iconSize.iWidth <= 0 || iconSize.iHeight <= 0 )
			return;


		TRgb nonFocusColor = IndicatorColor(EFalse);
		TRgb focusColor = IndicatorColor(ETrue);
		CJuikGfxStore::TGfxDef defs[EGfxCount] = {
			CJuikGfxStore::TGfxDef( IconName( ELeftEdge ),  
									EMbmContextcontactsuiIndicator_1, iconSize, nonFocusColor ),
			CJuikGfxStore::TGfxDef( IconName( ERightEdge ),  
									EMbmContextcontactsuiIndicator_2, iconSize, nonFocusColor ),
			CJuikGfxStore::TGfxDef( IconName( EFocusLeftEdge ),  
									EMbmContextcontactsuiIndicator_1, iconSize, focusColor ),
			CJuikGfxStore::TGfxDef( IconName( EFocusRightEdge),  
									EMbmContextcontactsuiIndicator_2, iconSize, focusColor ) };
		
		if ( iEdgeGraphics.Count() == 0)
			{
				for (TInt i=0; i < EGfxCount; i++) { iEdgeGraphics.Append( NULL ); } 
			}
		
		for (TInt i = 0; i < EGfxCount; i++) 
			{
				CGulIcon* icon = iEdgeGraphics[i];
				if ( icon )
					{
						if ( icon->Bitmap() && icon->Bitmap()->SizeInPixels() == iconSize )
							continue; // all is well
						else
							{
								Graphics().ReleaseIcon( icon );
								iEdgeGraphics[i] = NULL;
							}
					}				
				iEdgeGraphics[i] = Graphics().GetColoredIconL( defs[i] );
			}
	}
	
	
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CCommentIndicatorImpl"), _CL("ConstructL"));
		iMargin = Layout().GetLayoutItemL(LG_feed_controls_margins, LI_feed_controls_margins__comment_indicator).Margins();
	}		
	
	
	void SizeChanged() 
	{
		CALLSTACKITEM_N(_CL("CCommentIndicatorImpl"), _CL("SizeChanged"));
		GetCorrectSizeIconsL();
	}

	TRgb IndicatorTextColor(TBool aFocused) const
	{
		return IndicatorColor( ! aFocused );
	}
	
	
	TRgb IndicatorColor(TBool aFocused) const
	{
		return aFocused ?  PrimaryHighlightTextColor() : BubbleFocusColor();
		
	}


	void Draw(const TRect& /*aRect*/) const
	{
		CALLSTACKITEM_N(_CL("CCommentIndicatorImpl"), _CL("Draw"));
		CWindowGc& gc = SystemGc();
                gc.SetBrushStyle(CGraphicsContext::ENullBrush);

		TRect r = iMargin.InnerRect( Rect() );
		
		TPoint tl = r.iTl;
		TPoint tr = TPoint(r.iBr.iX, r.iTl.iY);
		TBool inverse = EFalse;

		TInt leftW = 0;
		TInt rightW = 0;
		if ( iEdgeGraphics.Count() == EGfxCount )
			{
				// left 
				{
					CGulIcon* icon = iEdgeGraphics[ IsFocused() ? EFocusLeftEdge : ELeftEdge ];
					TSize sz = icon->Bitmap()->SizeInPixels();
					TPoint p = tl;
					gc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
					leftW = sz.iWidth;
				}		
				
				// right 
				{
					CGulIcon* icon = iEdgeGraphics[ IsFocused() ? EFocusRightEdge : ERightEdge  ];
					TSize sz = icon->Bitmap()->SizeInPixels();
					TPoint p = tr - TSize(sz.iWidth,0);
					gc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
					rightW = sz.iWidth;
				}		
				
			}

		{
			TRect inner = iMargin.InnerRect( Rect() );
			// Draw indicator background
			TInt h = inner.Height();
			TRect r = Juik::Margins(leftW,rightW,0,0).InnerRect(inner);
			TRgb c = IndicatorColor( IsFocused() );
			gc.SetPenColor( c );
			gc.SetBrushColor( c );
			gc.SetBrushStyle( gc.ESolidBrush );
			gc.SetPenStyle( gc.ESolidPen );
			gc.DrawRect( r );
		
			
			TBuf<10> num;
			num.Num( iCommentCount );
			
			c = IndicatorTextColor( IsFocused() );			
			gc.UseFont( Font() );
			gc.SetPenColor( c );
			gc.SetBrushStyle( gc.ENullBrush );
			// 		gc.SetPenStyle( CGraphicsContext::ESolidPen );
			TInt b = Font()->FontCapitalAscent();
			gc.DrawText( num, inner, b, CGraphicsContext::ECenter, 0);


 		}
	}
	
	void UpdateL( CBBFeedItem& aItem )
	{
		CALLSTACKITEM_N(_CL("CCommentIndicatorImpl"), _CL("UpdateL"));
		TInt count = aItem.iChildCount;
		iCommentCount = count;
	}

	void ZeroL()
	{
		CALLSTACKITEM_N(_CL("CCommentIndicatorImpl"), _CL("ZeroL"));
		iCommentCount = 0;
	}
	
private:
	RPointerArray<CGulIcon> iEdgeGraphics;
	TMargins8 iMargin;
	TInt iCommentCount;
};

CCommentIndicator* CCommentIndicator::NewL(const TUiDelegates& aDelegates)
{
	CALLSTACKITEMSTATIC_N(_CL("CCommentIndicator"), _CL("NewL"));
	auto_ptr<CCommentIndicatorImpl> self( new (ELeave) CCommentIndicatorImpl(aDelegates) );
	self->ConstructL();
	return self.release();
}



class CFocusableBuddyImpl : public CFocusableBuddy, public MContextBase, public MFeedFoundation
{
public:
	
	static CFocusableBuddyImpl* NewL( CGulIcon& aDummyIcon, const TUiDelegates& aDelegates, TBool aDontDrawFocus )
	{
		CALLSTACKITEMSTATIC_N(_CL("CFocusableBuddyImpl"), _CL("NewL"));
		auto_ptr<CFocusableBuddyImpl> self( new (ELeave) CFocusableBuddyImpl( aDummyIcon, aDelegates, aDontDrawFocus ) );
		self->ConstructL();
		return self.release();
	}

	
	CFocusableBuddyImpl( CGulIcon& aDummyIcon, const TUiDelegates& aDelegates, TBool aDontDrawFocus ) 
		: MFeedFoundation(aDelegates), iDummyIcon( aDummyIcon ), iDontDrawFocus( aDontDrawFocus ) 
	{
	}

	void ZeroL()
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("ZeroL"));
		iImage->ClearL();
	}
	
	void UpdateL( CGulIcon* aIcon )
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("UpdateL"));
		if ( aIcon ) 
			iImage->UpdateL( *aIcon );
		else
			iImage->UpdateL( iDummyIcon );
	}


	void ReadMargins()
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("ReadMargins"));
		TInt marginId = iDontDrawFocus ? LI_feed_controls_margins__buddy : LI_feed_controls_margins__focused_buddy;
		TJuikLayoutItem l = Layout().GetLayoutItemL(LG_feed_controls_margins, marginId );
		iMargin = l.Margins();
	}
	
	TSize MinimumSize() 
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("MinimumSize"));
		if ( !iImage )
			return TSize(0,0);

		TRect r = iImage->MinimumSize();
		TSize marginSize = iMargin.SizeDelta();
		return r.Size() + marginSize;
	}
	
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("ConstructL"));
		ReadMargins();
		iImage = CJuikImage::NewL(&iDummyIcon, iDummyIcon.Bitmap()->SizeInPixels() );
		iImage->SetContainerWindowL( *this );
		
	}
	
	~CFocusableBuddyImpl()
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("~CFocusableBuddyImpl"));
		delete iImage;
	}

	TInt CountComponentControls() const
	{
	    CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("CountComponentControl"));
		return 1;
	}
	
	
	CCoeControl* ComponentControl(TInt /*aIndex*/) const
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("ComponentControl"));
		return iImage;
	}


	void PositionChanged()
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("PositionChanged"));
// 		TPoint p = Position();
// 		p += TSize(iMargin.iLeft, iMargin.iTop);
// 		iImage->SetPosition( p );
	}
	

	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("SizeChanged"));
		ReadMargins();
		TRect r = Rect();

		TRect inner = iMargin.InnerRect( r );
		iImage->SetRect( inner );
	}
	
	
	void Draw(const TRect& /*aRect*/) const
	{
		CALLSTACKITEM_N(_CL("CFocusableBuddyImpl"), _CL("Draw"));
		CWindowGc& gc = SystemGc();
		if ( !iDontDrawFocus && IsFocused() )
			{
				TRect focusR = Rect();
				focusR.Shrink(1,1);
				TRect innerR = iMargin.InnerRect( focusR );
				innerR.Grow(1,1);
				gc.SetPenStyle( CGraphicsContext::ENullPen );
				gc.SetBrushColor( BubbleFocusColor() );
				gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
				gc.DrawRect( focusR );

				gc.SetPenStyle( CGraphicsContext::ESolidPen );
				gc.SetPenColor( PrimaryHighlightTextColor() );
				gc.SetBrushStyle( CGraphicsContext::ENullBrush );
				gc.DrawRect( innerR );
			}
	}   

	
private:	
	CJuikImage* iImage;
	CGulIcon& iDummyIcon;

	TBool iDontDrawFocus;
	TMargins8 iMargin;
};
		
CFocusableBuddy* CFocusableBuddy::NewL(CGulIcon& aDummyIcon, const TUiDelegates& aDelegates, TBool aDontDrawFocus)
{
	CALLSTACKITEMSTATIC_N(_CL("CFocusableBuddyImpl"), _CL("GetCorrectSizeFocusSpeakIconL"));
	return CFocusableBuddyImpl::NewL(aDummyIcon, aDelegates, aDontDrawFocus);
}



class CBubbleContainerImpl : public CBubbleContainer, public MContextBase, public MFeedFoundation, public MCoeControlBackground
{
public:
	static CBubbleContainerImpl* NewL(CBubbleRenderer& aBubbleRenderer, CBubbledStreamItem::TBubbleType aType, const TUiDelegates& aDelegates)
	{
		CALLSTACKITEMSTATIC_N(_CL("CBubbleContainerImpl"), _CL("NewL"));
		auto_ptr<CBubbleContainerImpl> self( new (ELeave) CBubbleContainerImpl(aBubbleRenderer, aType, aDelegates) );
		self->ConstructL();
		return self.release();
	}


	CBubbleContainerImpl(CBubbleRenderer& aBubbleRenderer, 
						 CBubbledStreamItem::TBubbleType aType, 
						 const TUiDelegates& aDelegates) : 
		MFeedFoundation(aDelegates), iBubbleRenderer(aBubbleRenderer), iType(aType) {}
	
	~CBubbleContainerImpl()
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("~CBubbleContainerImpl"));
		delete iContent;
	}
	
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("ConstructL"));
		TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls_margins, 
													  LI_feed_controls_margins__main);		
		iMargin = lay.Margins();

		lay = Layout().GetLayoutItemL(LG_feed_controls_margins, 
									  LI_feed_controls_margins__bubblecontent);		

		iContentMargin = lay.Margins();
		iContent = CJuikSizerContainer::NewL();
		iContent->SetContainerWindowL( *this );
		iContent->SetBackground( this );
	}
	
	CJuikSizerContainer& Content() const
	{
		return *iContent;
	}

	void UpdateUnreadL( CBBFeedItem& aItem )
	{
		iIsUnread = aItem.iIsUnread();
	}
	
	void UpdateL( CBBFeedItem& aItem )
	{
		UpdateUnreadL( aItem );
	}

	void FocusChanged(TDrawNow aDrawNow)
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("FocusChanged"));
		iContent->SetFocus(IsFocused(), aDrawNow );
	}
	
	TSize MinimumSize()
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("MinimumSize"));
		TSize sz = iContent->MinimumSize();
		sz += iContentMargin.SizeDelta();
		sz += iMargin.SizeDelta();
		return sz;
	}
	
	TInt CountComponentControls() const
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("CountComponentControl"));
		return 1;
	}


	CCoeControl* ComponentControl(TInt /*aIndex*/) const
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("ComponentControl"));
		return iContent;
	}
	
		
	void PositionChanged()
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("PositionChanged"));
//  		TPoint p = Position() + TPoint( iMargin.iLeft, iMargin.iTop );
// 		p += TSize( iContentMargin.iLeft, iContentMargin.iTop );
//  		iContent->SetPosition( p );
	}
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("SizeChanged"));
 		TRect outer = Rect();
		if ( outer.Width() == 0 || outer.Height() == 0 )
			{
				outer.SetSize(TSize(0,0));
				iContent->SetRect( outer );
			}
		else
			{
				TRect inner = iMargin.InnerRect( outer );
				iBubbleRenderer.SetBubbleWidth( inner.Size().iWidth );
				TRect content = iContentMargin.InnerRect( inner );
				iContent->SetRect( content );
			}
	}

// 	void IntroSpectio() const 
// 	{
// 		TInt count = Content().CountComponentControls();
// 		for (TInt i=0; i < count; i++) 
// 			{				
// 				CCoeControl* ctrl = ComponentControl(i);
// 				TPoint pos = ctrl->Position();
// 				TRect r = ctrl->Rect();
// 				//TBool active = ctrl->IsActivated();
// 				TBool visible = ctrl->IsVisible();
// 				TBool ownsWindow = ctrl->OwnsWindow();
// 			}
// 	}

	
	void Draw(const TRect& /*aRect*/) const
	{
		CALLSTACKITEM_N(_CL("CBubbleContainerImpl"), _CL("Draw"));
		CWindowGc& gc = SystemGc();

		CBubbleRenderer::TBubbleType bubbleType = IsFocused() ? CBubbleRenderer::EFocused : CBubbleRenderer::ENormal;
		if ( iType == CBubbledStreamItem::EComment )
			bubbleType = IsFocused() ? CBubbleRenderer::EFocusedComment : CBubbleRenderer::EComment;
		
		if ( iType == CBubbledStreamItem::ENoBubble && !IsFocused())
			bubbleType = CBubbleRenderer::ENone;
		
		TRect outer = Rect();
		TRect inner = iMargin.InnerRect( outer );		
		// 		TRect contentR = iContent->Rect();
		// 		TRect contentOuter = iMargin.OuterRect(contentR);
		iBubbleRenderer.DrawBubble(inner, gc, bubbleType );

		if ( iIsUnread )
			{
				TRgb c = ThemeColors().GetColorL(CThemeColors::EPrimaryText);
				//c.SetAlpha( 51 );
				gc.SetPenStyle( gc.ENullPen );
				gc.SetBrushStyle( gc.ESolidBrush );
				gc.SetBrushColor( c );
				
				TRect indicatorR = TJuikLayoutItem( inner ).Combine( Layout().GetLayoutItemL(LG_feed_controls, 
																							 LI_feed_controls__unread_indicator) ).Rect();
				gc.DrawRect( indicatorR );
			}
	}

	void SetContainerWindowL(const CCoeControl& aControl)
	{
		CBubbleContainer::SetContainerWindowL( aControl );
		if ( iContent ) iContent->SetContainerWindowL( *this );
	}

	virtual void Draw(CWindowGc &aGc, const CCoeControl &aControl, const TRect &aRect) const
	{		
		CBubbleRenderer::TBubbleType bubbleType = IsFocused() ? CBubbleRenderer::EFocused : CBubbleRenderer::ENormal;
		if ( iType == CBubbledStreamItem::EComment )
			bubbleType = IsFocused() ? CBubbleRenderer::EFocusedComment : CBubbleRenderer::EComment;
		
		if ( iType == CBubbledStreamItem::ENoBubble && !IsFocused())
			bubbleType = CBubbleRenderer::ENone;
		
		TRect outer = Rect();
		TRect inner = iMargin.InnerRect( outer );		
		// 		TRect contentR = iContent->Rect();
		// 		TRect contentOuter = iMargin.OuterRect(contentR);
		iBubbleRenderer.DrawBubble(inner, aGc, bubbleType );
	}

	
	CJuikSizerContainer* iContent;
	CBubbleRenderer& iBubbleRenderer;	
	CBubbledStreamItem::TBubbleType iType;
	TMargins8 iMargin;
	TMargins8 iContentMargin;
	TBool iIsUnread;
};

CBubbleContainer* CBubbleContainer::NewL(CBubbleRenderer& aBubbleRenderer, CBubbledStreamItem::TBubbleType aType, const TUiDelegates& aDelegates)
{
	CALLSTACKITEMSTATIC_N(_CL("CBubbleContainer"), _CL("NewL"));
	return CBubbleContainerImpl::NewL(aBubbleRenderer, aType, aDelegates);
}



CBubbledStreamItem::CBubbledStreamItem(const TUiDelegates& aDelegates, TInt aFlags)
	: CGeneralFeedControl(aDelegates, aFlags)
{
}

CJuikSizerContainer& CBubbledStreamItem::BubbleContent() const
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("BubbleContent"));
	return iBubbleContainer->Content();
}

enum
	{
		EBubbledFirstRow = 0,
		EBubbledSeparator,
		EBuddyIcon,
		EBubbleContainer,
		ESeparator
	};



void CBubbledStreamItem::BaseConstructL() 
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("ConstructL"));
	CGeneralFeedControl::BaseConstructL();

	// Colors to differentiate entry that has reached server and one that hasn't
	iServerColor = PrimaryTextColor();
	iLocalColor = SpecialPrimaryTextColor();;
	
	// Main sizer

	{
		MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
		Content().SetRootSizerL( sizer );
	}
	
	{
		MJuikSizer* sizer = Juik::CreateBoxSizerL(Juik::EHorizontal);
		Content().AddSizerL( sizer, EBubbledFirstRow );
		Content().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
	}
	
	// Buddy icon		
	if ( iFlags & FeedUi::EShowBuddyIcon ) 
		{
			TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls, LI_feed_controls__buddy_icon);
			CGulIcon* dummyIcon = &( UserPics().DummyIconL( lay.Size() ) );
		
			TBool wholeBubbleFocus = ! (iFlags & FeedUi::EFocusInsideControl);
			iBuddyIcon = CFocusableBuddy::NewL( *dummyIcon, iDelegates, wholeBubbleFocus );			
			Content().AddControlL(iBuddyIcon, EBuddyIcon);
			Content().GetSizerL( EBubbledFirstRow )->AddL( *iBuddyIcon, 0, Juik::EExpandNot );
			if (iFocusHandling) iFocusHandling->AddL( *iBuddyIcon );
		}
	
	// content sizer
	{
		TBool drawTails = iFlags & FeedUi::EShowBuddyIcon;
		iBubbleRenderer = CBubbleRenderer::NewL( iDelegates, drawTails );
		iBubbleContainer = CBubbleContainer::NewL(*iBubbleRenderer, BubbleType(), iDelegates);
		Content().AddControlL( iBubbleContainer, EBubbleContainer );
		Content().GetSizerL( EBubbledFirstRow )->AddL( *iBubbleContainer, 1, Juik::EExpandNot );
	}

	if ( iFlags & FeedUi::EDrawSeparator )
		{
			TSize iconSize  = Layout().GetLayoutItemL(LG_feed_controls, 
													  LI_feed_controls__separator).Size();
			
			TComponentName name = { { CONTEXT_UID_CONTEXTCONTACTSUI }, EGfxSeparator };
			iSeparatorIcon = Graphics().GetColoredIconL(name, 
														EMbmContextcontactsuiSeparator, iconSize,
														ThemeColors().GetColorL(CThemeColors::EPrimaryText));		
			iSeparator = CJuikImage::NewL( iSeparatorIcon, iconSize);
			Content().AddControlL( iSeparator, ESeparator );
			iSeparator->iMargin = Juik::Margins(0,0,0,4);
			Content().GetRootSizerL()->AddL( *iSeparator, 0, Juik::EAlignCenterHorizontal ); 
		}

	UserPics().AddObserverL( *this );
}


CBubbledStreamItem::~CBubbledStreamItem()
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("~CBubbledStreamItem"));
	UserPics().RemoveObserverL( *this );
	delete iBubbleRenderer;
	iDelegates.iFeedGraphics->ReleaseIcon( iSeparatorIcon );
	iSeparatorIcon = NULL;
}


void CBubbledStreamItem::UpdateBuddyPicL()
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("UpdateBuddyPicL"));
	if ( iBuddyIcon )
		{
			CGulIcon* icon = UserPics().GetIconL( iNick );
			iBuddyIcon->UpdateL( icon );
		}
}


void CBubbledStreamItem::ZeroBaseL()
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("ZeroBaseL"));
	iNick.Zero();
	if ( iBuddyIcon )
		{
			iBuddyIcon->ZeroL();
		}
}

CGulIcon* CBubbledStreamItem::LoadIconL(TComponentName aName, TInt aId, TSize aSize, TRgb aRgb)
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("LoadIconL"));
	return iDelegates.iFeedGraphics->LoadColoredIconL( aName, aId, aSize, aRgb );
}

CGulIcon* CBubbledStreamItem::GetIconL(TComponentName aName, TInt aId, TSize aSize, TRgb aRgb)
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("GetIconL"));
	return iDelegates.iFeedGraphics->GetColoredIconL( aName, aId, aSize, aRgb );
}


void CBubbledStreamItem::UpdateBaseL(CBBFeedItem& aItem)
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("UpdateBaseL"));
	iNick = aItem.iAuthorNick();
	iId = aItem.iLocalDatabaseId;
	UpdateBuddyPicL();
	if ( iBubbleContainer ) iBubbleContainer->UpdateL( aItem );
}

void CBubbledStreamItem::UpdateUnreadL( CBBFeedItem& aItem )
{
	iBubbleContainer->UpdateUnreadL( aItem );
}



CJuikImage* CBubbledStreamItem::CreateJaiconL(CCoeControl& aParent)
{	
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("CreateJaiconL"));
	TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls, LI_feed_controls__jaicon);			
	
	auto_ptr<CJuikImage> jaicon( CJuikImage::NewL( NULL, lay.Size() ) );
	jaicon->SetContainerWindowL( aParent );
	
	lay = Layout().GetLayoutItemL(LG_feed_controls_margins, LI_feed_controls_margins__jaicon);			
	jaicon->iMargin = lay.Margins();
	return jaicon.release();
}

CJuikImage* CBubbledStreamItem::CreateJaiconL()
{	
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("CreateJaiconL"));
	TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls, LI_feed_controls__jaicon);			
	
	auto_ptr<CJuikImage> jaicon( CJuikImage::NewL( NULL, lay.Size() ) );
	
	lay = Layout().GetLayoutItemL(LG_feed_controls_margins, LI_feed_controls_margins__jaicon);			
	jaicon->iMargin = lay.Margins();
	return jaicon.release();
}


void CBubbledStreamItem::UpdateJaiconL(CJuikImage& aJaicon, CBBFeedItem& aItem)
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("UpdateJaiconL"));
	CGulIcon* icon = Jaicons().GetJaiconL( aItem.iIconId() );
	
	if ( ! icon )
		if ( ! (FeedItem::IsJaiku( aItem ) || FeedItem::IsComment( aItem ) ) )
			icon = Jaicons().GetJaiconL( KAtomJaiconId );
	
	if ( icon )
		aJaicon.UpdateL( *icon );
	else
		aJaicon.ClearL();
}


void CBubbledStreamItem::UpdateWithContentL( CBBFeedItem& aItem, CJuikLabel* label)
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("UpdateWithContentL"));
	label->UpdateTextL( aItem.iContent() );
	if ( aItem.iFromServer() )
		label->SetColorsL( iServerColor, PrimaryHighlightTextColor() );
	else 
		label->SetColorsL( iLocalColor, SecondaryHighlightTextColor() );
}



TKeyResponse CBubbledStreamItem::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("OfferKeyEventL"));
	return CGeneralFeedControl::OfferKeyEventL( aKeyEvent, aType );
}


void CBubbledStreamItem::FocusChanged(TDrawNow aDrawNow)
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("FocusChanged"));
	CGeneralFeedControl::FocusChanged( aDrawNow );
}


void CBubbledStreamItem::UserPicChangedL( const TDesC& aNick, TBool /*aIsNew*/ ) 
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("UserPicChangedL"));		
	if ( CJabberData::EqualNicksL( aNick, iNick ) )
		{
			UpdateBuddyPicL();
			DrawDeferred();
		}
}

void CBubbledStreamItem::Draw(const TRect& /*aRect*/) const
{
	CALLSTACKITEM_N(_CL("CBubbledStreamItem"), _CL("Draw"));		
	if ( iFlags & FeedUi::EDrawFullFocus )
		{
// 			CWindowGc& gc = SystemGc();
// 			//DrawNormalFocus(gc, Rect());
// 			MJuikSizer* sizer = Content().GetSizerL( EBubbledFirstRow );
// 			if ( sizer )
// 				DrawJaikuFocus(gc, sizer->Rect());
// 			else
// 				DrawJaikuFocus(gc, Rect());
		}
}



CGeneralFeedControl::CGeneralFeedControl(const TUiDelegates& aDelegates,TInt aFlags) 
	: CFeedControl(aDelegates), iFlags(aFlags)
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("CGeneralFeedControl"));		
}


CJuikSizerContainer& CGeneralFeedControl::Content() const
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("Content"));		
	return *iContainer;
}

	
void CGeneralFeedControl::BaseConstructL() 
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("BaseConstructL"));		
	iMargin = Juik::Margins(0,0,0,0);
	
	if ( iFlags & FeedUi::EFocusInsideControl )
		iFocusHandling = CFocusHandling::NewL();

	{
		iContainer = CJuikSizerContainer::NewL();
		iContainer->SetContainerWindowL(*this);
	}
	
}
	
CGeneralFeedControl::~CGeneralFeedControl()
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("~CGeneralFeedControl"));		
	delete iContainer;
	delete iFocusHandling;
}

TBool CGeneralFeedControl::PreFocusGained(TFocusFrom aFocusFrom)
{
	if ( iFocusHandling )
		return iFocusHandling->PreFocusGained( aFocusFrom );
	else 
		return EFalse;
}


void CGeneralFeedControl::FocusChanged(TDrawNow aDrawNow)
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("FocusChanged"));
	if ( iFocusHandling )
		iFocusHandling->FocusChanged( IsFocused(), aDrawNow );
	else
		iContainer->SetFocus(IsFocused(), aDrawNow);
}



TKeyResponse CGeneralFeedControl::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)			
{
	if ( ! iFocusHandling )
		return EKeyWasNotConsumed;
	
	if ( aType == EEventKey )
		{
			if ( aKeyEvent.iCode == JOY_DOWN 
				 || aKeyEvent.iCode == JOY_UP )
				{
					TKeyResponse response = iFocusHandling->OfferKeyEventL( aKeyEvent, aType );
					DrawDeferred();
					return response;
				}
		}
	return EKeyWasNotConsumed;
}

TInt CGeneralFeedControl::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("CountComponentControl"));
	return 1;
}
	
	
CCoeControl* CGeneralFeedControl::ComponentControl(TInt /*aIndex*/) const
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("ComponentControl"));
	return iContainer;
}

TSize CGeneralFeedControl::MinimumSize()
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("MinimumSize"));
	TSize sz = iContainer->MinimumSize();
	sz += iMargin.SizeDelta();
	return sz;
}

void CGeneralFeedControl::PositionChanged()
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("PositionChanged"));
// 	TPoint p = Position() + TSize(iMargin.iLeft, iMargin.iTop);
// 	iContainer->SetPosition( p );
}
	
void CGeneralFeedControl::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("SizeChanged"));
	TRect r = Rect();
	TRect inner = iMargin.InnerRect(r);
	if (inner.Width() < 0 ||inner.Height() < 0)
		inner.SetSize(TSize(0,0));
	   
	iContainer->SetRect(inner);
}

void CGeneralFeedControl::Draw(const TRect& /*aRect*/) const
{
	if ( iFlags & FeedUi::EDrawFullFocus )
		{
// 			CWindowGc& gc = SystemGc();
// 			//DrawNormalFocus(gc, Rect());
// 			DrawJaikuFocus(gc, Rect());
		}
}


void CGeneralFeedControl::SetContainerWindowL(const CCoeControl& aControl)
{
	CFeedControl::SetContainerWindowL( aControl );
	// propagate to child
	if ( iContainer ) 
		iContainer->SetContainerWindowL( *this );
}

void CGeneralFeedControl::DrawNormalFocus(CWindowGc& aGc, const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("DrawNormalFocus"));
	if ( IsFocused() )
		{
			// Non-skin focus
			TRect focusRect = aRect;
			TRect innerRect = focusRect;
			innerRect.Shrink(4,4); 
			
			MAknsSkinInstance* skin = AknsUtils::SkinInstance();
			TBool skinnedFocus = AknsDrawUtils::DrawFrame( skin, aGc, focusRect, innerRect, 
														   KAknsIIDQsnFrList, KAknsIIDQsnFrListCenter);
			if (!skinnedFocus)
				{
					aGc.SetPenStyle( CGraphicsContext::ENullPen );
					aGc.SetBrushColor( TRgb(200, 200, 200, 100) );
					aGc.SetBrushStyle( CGraphicsContext::ESolidBrush );
					aGc.DrawRect( focusRect );
				}
		}
}


void CGeneralFeedControl::DrawJaikuFocus(CWindowGc& aGc, const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CGeneralFeedControl"), _CL("DrawJaikuFocus"));
	if ( IsFocused() )
		{
			TRect focusRect = aRect;
			aGc.SetPenStyle( CGraphicsContext::ENullPen );
			aGc.SetBrushColor( BubbleFocusColor() );
			aGc.SetBrushStyle( CGraphicsContext::ESolidBrush );
			aGc.DrawRect( focusRect );
		}
}

// TRgb CGeneralFeedControl::TextFocusColor() const
// {
// 	if ( iFlags & FeedUi::EDrawFullFocus )
// 		{
			
// 		}
// }

// TRgb CGeneralFeedControl::BgFocusColor() const
// {
	
// }
