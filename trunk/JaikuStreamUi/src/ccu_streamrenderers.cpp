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

#include "ccu_streamrenderers.h"

#include "ccu_feedfoundation.h"
#include "ccu_staticicons.h"

#include "juik_gfxstore.h"
#include "juik_icons.h"
#include "juik_layout.h"
#include "jaiku_layoutids.hrh"

#include <contextcontactsui.mbg>




class CBubbleRendererImpl : public CBubbleRenderer, public MContextBase, public MFeedFoundation
{
public:
	static CBubbleRendererImpl* NewL(const TUiDelegates& aDelegates, TBool aDrawTail)
	{
		CALLSTACKITEMSTATIC_N(_CL("CBubbleRendererImpl"), _CL("NewL"));
		auto_ptr<CBubbleRendererImpl> self( new (ELeave) CBubbleRendererImpl(aDelegates, aDrawTail) );
		self->ConstructL( );
		return self.release();
	}

	CBubbleRendererImpl(const TUiDelegates& aDelegates, TBool aDrawTail) : MFeedFoundation( aDelegates ), iDrawTail(aDrawTail) {}
   

	enum 
		{
			ETopLeft = 0,
			ETopRight,
			EBottomLeft,
			EBottomRight,
			ETopMiddle,
			EBottomMiddle,
			ELeftEdge,
			ERightEdge,
			EBubbleGraphicCount
		};
	

	enum TIconType
		{
			EJaikuGfx,
			EHiliteGfx,
			ECommentGfx
		};

	CGulIcon* GetIconL( CJuikGfxStore::TGfxDef aDef )
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("GetIconL"));
		return Graphics().GetColoredIconL( aDef );
	}

	TComponentName IconName(TIconType aType, TInt aIconIndex) const
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("IconName"));
		TInt id = EGfxPostBubble;
		if ( aType == EHiliteGfx )
			id = EGfxFocusBubble;
		else if ( aType == ECommentGfx )
			id = EGfxCommentBubble;

		id += aIconIndex;
		TComponentName name = { { CONTEXT_UID_CONTEXTCONTACTSUI }, id };
		return name;	
	}


	void LoadIconsL(TInt aWidth)
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("LoadIconsL"));
		ReleaseIcons();
		
		TRgb color = BubbleColor();
		TRgb bubbleColor = BubbleColor();
		TRgb focusColor = BubbleFocusColor();

		TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_bubble, 
													  LI_feed_bubble__corner);		
		TSize cornerSize = lay.Size();
		lay = Layout().GetLayoutItemL(LG_feed_bubble, 
									  LI_feed_bubble__top);		

		TSize hEdgeSize = cornerSize; // lay.Size() ;
		hEdgeSize.iWidth = aWidth - (2 * cornerSize.iWidth);

		lay = Layout().GetLayoutItemL(LG_feed_bubble, 
									  LI_feed_bubble__left);		
		TSize vEdgeSize = lay.Size();

		const TInt KGfxElementCount(8);
		
		TRgb c = bubbleColor;
		TIconType t = EJaikuGfx;
		CJuikGfxStore::TGfxDef bubbleDefs[KGfxElementCount] = {
			CJuikGfxStore::TGfxDef( IconName( t, ETopLeft),     EMbmContextcontactsuiJaiku_u1, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopRight),     EMbmContextcontactsuiJaiku_u3, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomLeft ),  EMbmContextcontactsuiJaiku_d1, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomRight ), EMbmContextcontactsuiJaiku_d3, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopMiddle ),   EMbmContextcontactsuiJaiku_u2, hEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomMiddle ),EMbmContextcontactsuiJaiku_d2, hEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ELeftEdge ),    EMbmContextcontactsuiJaiku_m1, vEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ERightEdge ),   EMbmContextcontactsuiJaiku_m3, vEdgeSize, c ),
		};


				
		c = focusColor;
		t = EHiliteGfx;
		CJuikGfxStore::TGfxDef hiliteDefs[KGfxElementCount] = {
			CJuikGfxStore::TGfxDef( IconName( t, ETopLeft ),     EMbmContextcontactsuiHi_u1, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopRight ),     EMbmContextcontactsuiHi_u3, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomLeft ),  EMbmContextcontactsuiHi_d1, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomRight ), EMbmContextcontactsuiHi_d3, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopMiddle ),   EMbmContextcontactsuiHi_u2, hEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomMiddle ),EMbmContextcontactsuiHi_d2, hEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ELeftEdge ),    EMbmContextcontactsuiHi_m1, vEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ERightEdge ),   EMbmContextcontactsuiHi_m3, vEdgeSize, c ),
		};


		c = bubbleColor;
		t = ECommentGfx;
		CJuikGfxStore::TGfxDef commentDefs[KGfxElementCount] = {
			CJuikGfxStore::TGfxDef( IconName( t, ETopLeft ),     EMbmContextcontactsuiCom_u1, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopRight ),     EMbmContextcontactsuiCom_u3, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomLeft ),  EMbmContextcontactsuiCom_d1, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomRight ), EMbmContextcontactsuiCom_d3, cornerSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopMiddle ),   EMbmContextcontactsuiCom_u2, hEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomMiddle ),EMbmContextcontactsuiCom_d2, hEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ELeftEdge ),    EMbmContextcontactsuiCom_m1, vEdgeSize, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ERightEdge ),   EMbmContextcontactsuiCom_m3, vEdgeSize, c ),
		};
			
		for ( TInt i=0; i < KGfxElementCount; i++)
			{
				iBubbleGraphics.AppendL( GetIconL( bubbleDefs[i] ) );
				iFocusGraphics.AppendL(  GetIconL( hiliteDefs[i] ) );
				iCommentGraphics.AppendL( GetIconL( commentDefs[i] ) );
			}		
	}

	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("ConstructL"));
		iBubbleWidth = 0;
	}

	~CBubbleRendererImpl()
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("~CBubbleRendererImpl"));
		ReleaseIcons();
	}

	void ReleaseIcons()
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("ReleaseIcons"));
		for (TInt i=0; i < iBubbleGraphics.Count(); i++)
			{
				iDelegates.iFeedGraphics->ReleaseIcon( iBubbleGraphics[i] );
			}
		iBubbleGraphics.Reset();

		for (TInt i=0; i < iFocusGraphics.Count(); i++)
			{
				iDelegates.iFeedGraphics->ReleaseIcon( iFocusGraphics[i] );
			}
		iFocusGraphics.Reset();

		for (TInt i=0; i < iCommentGraphics.Count(); i++)
			{
				iDelegates.iFeedGraphics->ReleaseIcon( iCommentGraphics[i] );
			}
		iCommentGraphics.Reset();
	}
	
	void SetBubbleWidth(TInt aWidth)
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("SetBubbleWidth"));
		if ( aWidth != iBubbleWidth )
			{
				if ( aWidth > 0 ) 
					{
						iBubbleWidth = aWidth;
						LoadIconsL( iBubbleWidth );
					}
			}
	}
	
	
	
	TRgb SelectBubbleColor(TBubbleType aType) const
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("SelectBubbleColor"));
		switch ( aType )
			{
			case ENormal: return BubbleColor();
			case EFocused: return BubbleFocusColor();
			case EComment: return CommentBubbleColor();
			case EFocusedComment: return BubbleFocusColor();// CommentBubbleFocusColor();
			}
		return KRgbWhite;
	}

	const RPointerArray<CGulIcon>& SelectBubbleGfx(TBubbleType aType) const
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("SelectBubbleGfx"));
		switch ( aType )
			{
			case ENormal: return iBubbleGraphics;
			case EFocused: return iFocusGraphics;
			case EComment: return iCommentGraphics;
			case EFocusedComment: return iFocusGraphics;
			}
		return iBubbleGraphics;
	}

	void DrawEdge(CWindowGc& aGc, TPoint edgeTop, TInt edgeH, CGulIcon* icon) const
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("DrawEdge"));
		TBool inverse = EFalse;

		TInt brokenBottomH = 20;

		TSize sz = icon->Bitmap()->SizeInPixels();
		TSize edgeSize= TSize(sz.iWidth, Min(edgeH, sz.iHeight - brokenBottomH));
		TPoint p = edgeTop;
		TInt remainingH = edgeH;
                aGc.SetBrushStyle(CGraphicsContext::ENullBrush);

		if ( iDrawTail )
			{
				aGc.BitBltMasked( p, icon->Bitmap(), TRect(TPoint(0,0),edgeSize), icon->Mask(), inverse);
				p += TSize(0, edgeSize.iHeight);
				remainingH -= edgeSize.iHeight;
			}

		// if edge is larger than bitmap, use tail of bitmap to draw remaining parts
		{
			// Select fille part from the middle of edge graphic. 
			// This could use bottom half, but there is some crap at the end of graphic. 
			TInt fillerH = sz.iHeight / 2;
			TRect fillerRect(TPoint(0, (sz.iHeight - fillerH) / 2), TSize( sz.iWidth, fillerH ) );
			
			// Draw remaining part of edge with filler graphic 
			while ( remainingH > 0 ) 
				{
					if ( remainingH < fillerRect.Height() )
						{ // last part, draw only what is needed
							fillerRect.SetHeight( remainingH );
							aGc.BitBltMasked( p, icon->Bitmap(), fillerRect, icon->Mask(), inverse); 
							break;
						}
					else
						{
							aGc.BitBltMasked( p, icon->Bitmap(), fillerRect, icon->Mask(), inverse); 
							remainingH -= fillerH;
							p += TSize( 0, fillerH );
						}
				}
		}
	}

	
	void DrawBubble(const TRect& aRect, CWindowGc& aGc, TBubbleType aType) const
	{
		CALLSTACKITEM_N(_CL("CBubbleRendererImpl"), _CL("DrawBubble"));
 		if ( aType == ENone ) return;
                aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
		
		TRect r = aRect;

		TPoint tl = r.iTl;
		TPoint tr = TPoint(r.iBr.iX, r.iTl.iY);
		TPoint bl = TPoint(r.iTl.iX, r.iBr.iY);
		TPoint br = r.iBr;
				
		const RPointerArray<CGulIcon>& gArray = SelectBubbleGfx( aType );		

		TBool inverse = EFalse;
		TInt index = -1;

		index = ETopLeft;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = tl;
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}

		index = ETopRight;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = tr - TSize(sz.iWidth, 0);
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}

		index = EBottomRight;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = br - sz;
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}

		index = EBottomLeft;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = bl - TSize(0, sz.iHeight);
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}

		TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_bubble, 
													  LI_feed_bubble__corner);		
		TSize cornerSize = lay.Size();
		
		index = ETopMiddle;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = tl + TSize(cornerSize.iWidth,0);
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}

		index = EBottomMiddle;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = bl + TSize(cornerSize.iWidth, - cornerSize.iHeight);
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}


		TInt h = r.Size().iHeight;
		TInt cH = cornerSize.iHeight;
		TInt edgeH = h - 2*cH;

		TPoint edgeTop = tl + TSize(0,cH);
		index = ELeftEdge;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				DrawEdge( aGc, edgeTop, edgeH, icon );
			}

		edgeTop = tr + TSize(0,cH);
		index = ERightEdge;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TInt edgeW = icon->Bitmap()->SizeInPixels().iWidth;
				edgeTop -= TSize( edgeW, 0 );
				DrawEdge( aGc, edgeTop, edgeH, icon );
			}
		
		// Draw center 
		TBool drawCenter = aType == EFocused || aType == EFocusedComment || aType == EComment;
		TRgb color = SelectBubbleColor( aType );

  		if ( drawCenter )
  			{
				TRect r = aRect;
				r.Shrink( cornerSize );
 				
				aGc.SetPenColor( color );
				aGc.SetBrushColor( color );
 				aGc.SetPenStyle( CGraphicsContext::ESolidPen );
 				aGc.SetBrushStyle( CGraphicsContext::ESolidBrush );
				
  				aGc.DrawRect(r);
 			}


	}    
private:
	TInt iBubbleWidth;
	RPointerArray<CGulIcon> iBubbleGraphics;
	RPointerArray<CGulIcon> iFocusGraphics;
	RPointerArray<CGulIcon> iCommentGraphics;

	TBool iDrawTail;
};


CBubbleRenderer* CBubbleRenderer::NewL(const TUiDelegates& aDelegates, TBool aDrawTail)
{
	CALLSTACKITEMSTATIC_N(_CL("CBubbleRenderer"), _CL("NewL"));
	return CBubbleRendererImpl::NewL(aDelegates, aDrawTail);
}
	


class CButtonRendererImpl : public CButtonRenderer, public MContextBase, public MFeedFoundation
{
public:
	static CButtonRendererImpl* NewL(const TUiDelegates& aDelegates)
	{
		CALLSTACKITEMSTATIC_N(_CL("CButtonRendererImpl"), _CL("NewL"));
		auto_ptr<CButtonRendererImpl> self( new (ELeave) CButtonRendererImpl(aDelegates) );
		self->ConstructL( );
		return self.release();
	}

	CButtonRendererImpl(const TUiDelegates& aDelegates) : MFeedFoundation( aDelegates ) {}
   
	
	
	CGulIcon* GetIconL(CJuikGfxStore::TGfxDef aDef)
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("GetIconL"));
		return Graphics().GetColoredIconL( aDef );
	}



	enum 
		{
			ETopLeft = 0,
			ETopRight,
			EBottomLeft,
			EBottomRight,
			ETopMiddle,
			EBottomMiddle,
			EGraphicCount
		};
	

	enum TIconType
		{
			ENonFocusGfx,
			EFocusGfx,
		};

	TComponentName IconName(TIconType aType, TInt aIconIndex) const
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("IconName"));
		TInt id = EGfxFocusedButton;
		if ( aType == ENonFocusGfx )
			id = EGfxNonFocusedButton;

		id += aIconIndex;
		TComponentName name = { { CONTEXT_UID_CONTEXTCONTACTSUI }, id };
		return name;	
	}


	void LoadIconsL(TInt aWidth)
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("LoadIconsL"));
		ReleaseIcons();
		
		TRgb color = BubbleColor();
		TRgb focusColor = BubbleFocusColor();

		TSize u1 = Layout().GetLayoutItemL(LG_feed_button, 
										   LI_feed_button__u1).Size();
		TSize u3 = Layout().GetLayoutItemL(LG_feed_button, 
										   LI_feed_button__u3).Size();
		TSize u2 = Layout().GetLayoutItemL(LG_feed_button, 
										   LI_feed_button__u2).Size();
		u2.iWidth = aWidth - (u1.iWidth + u3.iWidth);


		TSize d1 = Layout().GetLayoutItemL(LG_feed_button, 
										   LI_feed_button__d1).Size();
		TSize d3 = Layout().GetLayoutItemL(LG_feed_button, 
										   LI_feed_button__d3).Size();
		TSize d2 = Layout().GetLayoutItemL(LG_feed_button, 
										   LI_feed_button__d2).Size();
		d2.iWidth = aWidth - (d1.iWidth + d3.iWidth);

		const TInt KGfxElementCount(6);
		
		TRgb c = color;
		TIconType t = ENonFocusGfx;
		CJuikGfxStore::TGfxDef nonFocusDefs[KGfxElementCount] = {
			CJuikGfxStore::TGfxDef( IconName( t, ETopLeft),      EMbmContextcontactsuiButton_u1, u1, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopRight),     EMbmContextcontactsuiButton_u3, u3, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomLeft ),  EMbmContextcontactsuiButton_d1, d1, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomRight ), EMbmContextcontactsuiButton_d3, d3, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopMiddle ),   EMbmContextcontactsuiButton_u2, u2, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomMiddle ),EMbmContextcontactsuiButton_d2, d2, c ),
		};
									
		
		
		c = focusColor;
		t = EFocusGfx;
		CJuikGfxStore::TGfxDef focusDefs[KGfxElementCount] = {
			CJuikGfxStore::TGfxDef( IconName( t, ETopLeft),      EMbmContextcontactsuiButton_hi_u1, u1, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopRight),     EMbmContextcontactsuiButton_hi_u3, u3, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomLeft ),  EMbmContextcontactsuiButton_hi_d1, d1, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomRight ), EMbmContextcontactsuiButton_hi_d3, d3, c ),
			CJuikGfxStore::TGfxDef( IconName( t, ETopMiddle ),   EMbmContextcontactsuiButton_hi_u2, u2, c ),
			CJuikGfxStore::TGfxDef( IconName( t, EBottomMiddle ),EMbmContextcontactsuiButton_hi_d2, d2, c ),
		};

		
		for ( TInt i=0; i < KGfxElementCount; i++)
			{
				iNonFocusGraphics.AppendL( GetIconL( nonFocusDefs[i] ) );
				iFocusGraphics.AppendL(  GetIconL( focusDefs[i] ) );
			}		
	}

	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("ConstructL"));
		iWidth = 0;
	}

	~CButtonRendererImpl()
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("~CButtonRendererImpl"));
		ReleaseIcons();
	}

	void ReleaseIcons()
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("ReleaseIcons"));
		for (TInt i=0; i < iNonFocusGraphics.Count(); i++)
			{
				iDelegates.iFeedGraphics->ReleaseIcon( iNonFocusGraphics[i] );
			}
		iNonFocusGraphics.Reset();

		for (TInt i=0; i < iFocusGraphics.Count(); i++)
			{
				iDelegates.iFeedGraphics->ReleaseIcon( iFocusGraphics[i] );
			}
		iFocusGraphics.Reset();
	}
	
	void SetWidth(TInt aWidth)
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("SetWidth"));
		if ( aWidth != iWidth )
			{
				if ( aWidth > 0 ) 
					{
						iWidth = aWidth;
						LoadIconsL( iWidth );
					}
			}
	}
	
	
	
	TRgb SelectBubbleColor(TBool aFocused) const
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("SelectBubbleColor"));
		if ( aFocused )
			return BubbleFocusColor();
		else 
			return BubbleColor();
	}


	
	void DrawButton(const TRect& aRect, CWindowGc& aGc, TBool aFocused) const
	{
		CALLSTACKITEM_N(_CL("CButtonRendererImpl"), _CL("DrawBubble"));
		TRect r = aRect;
                aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
		
		TPoint tl = r.iTl;
		TPoint tr = TPoint(r.iBr.iX, r.iTl.iY);
		TPoint bl = TPoint(r.iTl.iX, r.iBr.iY);
		TPoint br = r.iBr;
		
		TBool focused = aFocused;
		
		const RPointerArray<CGulIcon>& gArray = focused ? iFocusGraphics : iNonFocusGraphics;
		
		TBool inverse = EFalse;
		TInt index = -1;
		
		index = EBottomRight;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = br - sz;
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}

		index = EBottomLeft;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = bl - TSize(0, sz.iHeight);
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}

		TSize bottomCornerSize = Layout().GetLayoutItemL(LG_feed_button, 
														 LI_feed_button__d1).Size();		
		TInt upperH = r.Height();
		upperH -= bottomCornerSize.iHeight;
		
		index = ETopLeft;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize iconSize = icon->Bitmap()->SizeInPixels();
				TSize sz = TSize(iconSize.iWidth, upperH);
				TPoint p = tl;
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}

		index = ETopRight;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize iconSize = icon->Bitmap()->SizeInPixels();
				TSize sz = TSize(iconSize.iWidth, upperH);
				TPoint p = tr - TSize(sz.iWidth, 0);
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}


		TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_button, 
													  LI_feed_button__u1);		
		TSize cornerSize = lay.Size();
		
		index = ETopMiddle;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize iconSize = icon->Bitmap()->SizeInPixels();
				TSize sz = TSize(iconSize.iWidth, upperH);
				TPoint p = tl + TSize(cornerSize.iWidth,0);
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}
		

		lay = Layout().GetLayoutItemL(LG_feed_button, 
									  LI_feed_button__d1);		
		cornerSize = lay.Size();
		
		index = EBottomMiddle;
		if ( index < gArray.Count()  )
			{
				CGulIcon* icon = gArray[index];
				TSize sz = icon->Bitmap()->SizeInPixels();
				TPoint p = bl + TSize(cornerSize.iWidth, - cornerSize.iHeight);
				aGc.BitBltMasked( p, icon->Bitmap(), sz, icon->Mask(), inverse);
			}
	}

private:
	TInt iWidth;
	RPointerArray<CGulIcon> iNonFocusGraphics;
	RPointerArray<CGulIcon> iFocusGraphics;
};


CButtonRenderer* CButtonRenderer::NewL(const TUiDelegates& aDelegates)
{
	CALLSTACKITEMSTATIC_N(_CL("CButtonRenderer"), _CL("NewL"));
	return CButtonRendererImpl::NewL(aDelegates);
}
