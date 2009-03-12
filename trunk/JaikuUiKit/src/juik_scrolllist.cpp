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

#include "juik_scrolllist.h"

#include "juik_sizer.h"
#include "juik_animation.h"
#include "juik_keycodes.h"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"

#include <eikenv.h>
// Following three are related to double-buffering onlyo
#include <bitdev.h>
#include <bitstd.h>
#include <gdi.h>

#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknsutils.h> 

// Comment following out if you want to enable soft scrolling
// Durrently soft-scrolling doesn't work because double-buffering is missing
// Double-buffering can't be done with CCoeControl's because you can't force controls
// to draw different graphics context than CWindowGc. 
//#define SOFT_SCROLLING

class CSoftScrollListImpl : public CSoftScrollList,
							public MContextBase
#ifdef  SOFT_SCROLLING
						  , public CJuikAnimation::MAnimated
#endif

{
public:
    static CSoftScrollListImpl* NewL(CCoeControl* aParent, TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CSoftScrollListImpl"), _CL("NewL"));
		auto_ptr<CSoftScrollListImpl> self( new (ELeave) CSoftScrollListImpl(aFlags) );
		self->ConstructL( aParent );
		return self.release();
	}
    
    ~CSoftScrollListImpl() 
    {
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("~CSoftScrollListImpl"));
// 	delete iBufferBitmap;
// 	delete iBufferDevice;
// 	delete iBufferGc;
	delete iSizer;
	iControls.Reset();
	iObservers.Close();

#ifdef SOFT_SCROLLING
	delete iAnimation;
#endif
    }
	
    CSoftScrollListImpl(TInt aFlags) : iChildrenDrawFocus( aFlags & EChildrenDrawFocus), 
									   iFlags(aFlags), 
									   iCurrentItem(KErrNotFound) {}

//     CWsBitmap* iBufferBitmap;
//     CFbsBitmapDevice* iBufferDevice;
//     CBitmapContext* iBufferGc;


	virtual void AddObserverL( MObserver& aObserver ) 
	{
		iObservers.AppendL( &aObserver );
	}
	
	virtual void RemoveObserver( MObserver& aObserver ) 
	{
		TInt ix = iObservers.Find( &aObserver );
		if ( ix >= 0 ) iObservers.Remove( ix );
	}

	void NotifyObserversL( TEvent aEvent, TInt aIndex )
	{
		for (TInt i=0; i < iObservers.Count(); i++ )
			{
				iObservers[i]->HandleListEventL( aEvent, aIndex );
			}
	}

    void InitDoubleBufferL()
    {
// 	if ( ! iBufferBitmap )
// 	    {
// 		iBufferBitmap = new (ELeave) CWsBitmap( iEikonEnv->WsSession() );
// 		TDisplayMode displayMode = iEikonEnv->ScreenDevice()->DisplayMode();
// 		TSize sz = Rect().Size();
// 		User::LeaveIfError( iBufferBitmap->Create( sz, displayMode ) );
		
//  		iBufferDevice = CFbsBitmapDevice::NewL(iBufferBitmap);
// 		User::LeaveIfError( iBufferDevice->CreateBitmapContext( iBufferGc ) );
// 	    }
    }
    
	void ConstructL(CCoeControl* aParent)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ConstructL"));
		//CreateWindowL();
		SetContainerWindowL( *aParent );

#ifdef SOFT_SCROLLING
		iAnimation = CJuikAnimation::NewL();
		InitDoubleBufferL();
#endif
		InitControlsL();
	}

	
	void PutAndLayoutTopChildL(MJuikControl* aChild)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("PutAndLayoutTopChildL"));
		InsertChildL( 0, aChild );
		iSizer->LayoutTopChildL();
	}

	void PutAndLayoutBottomChildL(MJuikControl* aChild)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("PutAndLayoutBottomChildL"));
		AddChildL( aChild );
		iSizer->LayoutBottomChildL();
	}

	void InsertAndLayoutChildrenL(TInt aPos, MJuikControl* aChild)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("InsertAndLayoutChildrenL"));
		InsertChildL( aPos, aChild );
		iSizer->LayoutChildL(aPos);
	}

    void AddChildL(MJuikControl* aChild)
    {
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("AddChildL"));
		iSizer->AddL( *aChild, 0, Juik::EExpand ); // Juik::EAlignCenterHorizontal );
		iControls.AppendL( aChild );
    }

	
    void InsertChildL(TInt aPos, MJuikControl* aChild)
    {
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("InsertChildL"));
		iSizer->InsertL( aPos, *aChild, 0, Juik::EExpand ); // Juik::EAlignCenterHorizontal );
		iControls.InsertL( aChild, aPos );
		if ( iCurrentItem == KErrNotFound )
			{
				iCurrentItem = 0;
			}
		else if ( iCurrentItem >= aPos )
			{
				iCurrentItem++;
			}
    }
	
 	MJuikControl* PopChildL(TInt aIndex)
 	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("PopChildL"));
		MJuikControl* c = iControls[aIndex];
		iSizer->RemoveL(aIndex);
		iControls.Remove(aIndex);
		if ( iControls.Count() == 0)
			iCurrentItem = KErrNotFound;
		else
			{
				if ( aIndex < iCurrentItem )
					iCurrentItem--;
				
				iCurrentItem = Max(iCurrentItem,0 );
				iCurrentItem = Min(iCurrentItem, iControls.Count() - 1);
			}
		return c;
 	}

 	MJuikControl* PopTopChildL()
 	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("PopTopChildL"));
		TInt top = 0;
		return PopChildL( top );
 	}

 	MJuikControl* PopBottomChildL()
 	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("PopBottomChildL"));
		TInt last = iControls.Count() - 1;
		return PopChildL( last );
 	}
	
	TInt CountComponentControls() const
	{
	    CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("CountComponentControl"));
		return iControls.Count(); //iLabels.Count() + iImages.Count(); 
	}
	
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ComponentControl"));
		return iControls[aIndex]->CoeControl();
	}
	
	void ReLayoutL()
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ReLayoutL"));
		TRect r = Rect();
		iSizer->SetMinSize( r.Size() );
		TSize sz;
		{
			CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ReLayoutL.1"));
			sz = iSizer->MinSize();
		}
		{
			CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ReLayoutL.2"));
			iSizer->SetDimensionL( r.iTl, sz );// r.Size() );
		}
		{
			CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ReLayoutL.3"));
			if ( iCurrentItem != KErrNotFound )
				SetScrollPositionToItemL( iCurrentItem );
		}
	}

	void PositionChanged()
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("PositionChanged"));
		if (iSizer) 
			{
				TPoint p = Position();
				iSizer->SetPositionL( p );
			}	
	}
	
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("SizeChanged"));
		ReLayoutL();
	}

	void UpdateBufferL()
	{
		
	}


	void Draw(const TRect& aRect) const
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("Draw"));

		CWindowGc& gc = SystemGc();

		MAknsSkinInstance* skin = AknsUtils::SkinInstance();
		MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );
		AknsDrawUtils::Background( skin, cc, this, gc, aRect );
		
		if ( ! iChildrenDrawFocus )
			{
				if ( iCurrentItem >= 0 && iCurrentItem < iSizer->ItemCount() )
					{
						
						// Non-skin focus
						TRect focusRect = iSizer->GetItemL( iCurrentItem ).Rect();
						TRect innerRect = focusRect;
						innerRect.Shrink(4,4); 
						
						MAknsSkinInstance* skin = AknsUtils::SkinInstance();
						TBool skinnedFocus = AknsDrawUtils::DrawFrame( skin, gc, focusRect, innerRect, 
																	   KAknsIIDQsnFrList, 
																	   KAknsIIDQsnFrListCenter);
						if (!skinnedFocus)
							{
								gc.SetPenStyle( CGraphicsContext::ENullPen );
								gc.SetBrushColor( TRgb(200, 200, 200, 100) );
								gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
								gc.DrawRect( focusRect );
							}
					}
			}

	}
	
	void ChangeCurrentItemL(TInt aDelta)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ChangeCurrentItemL"));
		if ( iCurrentItem >= 0 )
			SetFocusL( iCurrentItem + aDelta );
	}

	void SetCurrentItem(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("SetCurrentItem"));
		TInt old = iCurrentItem;
		iCurrentItem = aIndex;
		iCurrentItem = Max(iCurrentItem, 0);
		iCurrentItem = Min(iCurrentItem, ItemCount() - 1 );		
		if ( iChildrenDrawFocus )
			{
				if ( old >= 0 && old != iCurrentItem && old < iControls.Count() )
					iControls[old]->CoeControl()->SetFocus( EFalse, ENoDrawNow );
				if ( iCurrentItem >= 0)
					{
						TBool fromAbove = old < 0 || old - iCurrentItem < 0;
						MJuikControl* c = iControls[iCurrentItem];
						c->PreFocusGained( fromAbove ? EAbove : EBelow );
						c->CoeControl()->SetFocus( ETrue, ENoDrawNow );
					}
			}
	}

    void SetFocusL(TInt aIndex)
    {
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("FocusChildL"));
		TInt oldIx = iCurrentItem;
		SetCurrentItem( aIndex );
		TInt newIx = iCurrentItem;
		TBool fromAbove = newIx - oldIx >= 0;
		ScrollToFocusedImplL(fromAbove);
    }

	TBool Includes(const TRect& a, const TRect b)
	{
		return ( a.iTl.iY <= b.iTl.iY && b.iBr.iY <= a.iBr.iY );
	}
	



	TInt CalculateScrollPositionL( TInt aIndex, TBool aFromAbove = ETrue )
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("CalculateScrollPositionL"));
		TRect itemR = iSizer->GetItemL( aIndex ).Rect();
		TRect viewR = Rect();
		TInt oldY = iSizer->ScrollPosition(); 
		TInt delta = 0;
		
		if ( ! Includes( viewR, itemR ) )
			{
				if ( aFromAbove )
					{
						delta = viewR.iBr.iY - itemR.iBr.iY;
						TRect r = itemR;
						r.Move( 0, delta );
						if ( ! Includes( viewR, r ) )
							delta = viewR.iTl.iY - itemR.iTl.iY;
					}
				else 
					{
						delta = viewR.iTl.iY - itemR.iTl.iY;
						TRect r = itemR;
						r.Move( 0, delta );
						if ( ! Includes( viewR, r ) )
							delta = viewR.iBr.iY - itemR.iBr.iY;
					}
			}
// 		if ( itemR.iTl.iY < viewR.iTl.iY ) 
// 			{
// 				delta = viewR.iTl.iY - itemR.iTl.iY ;
// 			}
// 		else if ( itemR.iBr.iY > viewR.iBr.iY ) 
// 			{
// 				delta = viewR.iBr.iY - itemR.iBr.iY;
// 			}
		
		TInt scrollPos = oldY + delta;
// 		if ( itemR.iTl.iY < scrollPos && aFromAbove )
// 			scrollPos = itemR.iTl.iY;
// 		if ( itemR.iBr.iY > scrollPos + viewR.Height() && ! aFromAbove )
// 			scrollPos = itemR.iBr.iY - viewR.Height();
		return scrollPos;
	}

	void SetScrollPositionToItemL( TInt aIndex )
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("SetScrollPositionToItemL"));
		if ( aIndex < 0 || iControls.Count() <= aIndex )
			{
				Bug(_L("Index out of range")).TechMsg(_L("aIndex %1"), aIndex).Raise();
			}
		
		TInt scrollPos = CalculateScrollPositionL( aIndex );
 		if ( scrollPos != iSizer->ScrollPosition() )
			{
				iSizer->SetScrollPositionL( scrollPos );
			}
	}

	TBool ScrollCurrentL( TBool aDown )
	{
		const TInt KSharedY(15);

		TRect itemR = iSizer->GetItemL( iCurrentItem ).Rect();
		TRect viewR = Rect();
		TInt oldY = iSizer->ScrollPosition(); 
		TInt delta = 0;


		// 
		// 

		TInt topDelta = viewR.iTl.iY - itemR.iTl.iY;
		TInt bottomDelta = viewR.iBr.iY - itemR.iBr.iY;
		TInt fullScroll = viewR.Height() - KSharedY;
		if ( aDown && bottomDelta < 0 )
			{ // still something to show 
				
				delta = Max( bottomDelta, - fullScroll );
			}
		else if ( ! aDown && topDelta > 0 )
			{ // still something to show 					
				delta = Min( topDelta, fullScroll );
			}

		
		if ( delta )
			{
				TInt newY = oldY + delta;
				iSizer->SetScrollPositionL( newY );
				DrawDeferred();
				return ETrue;
			}
		else
			return EFalse;
	}
	
	void ScrollToFocusedL()
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ScrolLToFocusedL"));
		ScrollToFocusedImplL(ETrue);
	}
	void ScrollToFocusedImplL(TBool aFromAbove)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("ScrolLToFocusedImplL"));
		if ( iCurrentItem < 0 )
			return;

		TInt oldY = iSizer->ScrollPosition(); 

		TInt newY = CalculateScrollPositionL( iCurrentItem, aFromAbove );
		if (oldY != newY)
			{
				
#ifdef SOFT_SCROLLING
				iScrollAnim.iStartY = oldY;
				iScrollAnim.iEndY   = newY;
				StartScrollL();
#else
				iSizer->SetScrollPositionL( newY );
				DrawDeferred();
#endif
			}
		else
			{
				DrawDeferred();
			}
	}

	TInt CurrentItemIndex() const
	{
		return iCurrentItem;
	}

	TInt ItemCount() const 
	{
		return iSizer->ItemCount();
	}

	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("OfferKeyEventL"));
//  		TBuf<100> tmpBuf;
// 		tmpBuf.Format( _L("\t\tScrollList: type %d, code %d"), aType, aKeyEvent.iCode );
// 		Reporting().DebugLog( tmpBuf );

		TInt current = CurrentItemIndex();
		if ( 0 <= current && current <= iControls.Count() )
			{				
				TKeyResponse response = iControls[ current ]->CoeControl()->OfferKeyEventL( aKeyEvent, aType );
				if ( response == EKeyWasConsumed ) return response;
			}
		
 		if ( aType == EEventKey )
 			{
 				switch ( aKeyEvent.iCode )
					{
					case JOY_DOWN:
						{
							TBool scrolled = ScrollCurrentL( ETrue );
							if ( ! scrolled )
								{
									if ( iFlags & EListChangesFocus )
										ChangeCurrentItemL( +1 );
									else
										NotifyObserversL( EMoveFocusDown, iCurrentItem );
								}
						}
						return EKeyWasConsumed;

					case JOY_UP:
						{
							TBool scrolled = ScrollCurrentL( EFalse );
							if ( ! scrolled )
								{
									if ( iFlags & EListChangesFocus )
										ChangeCurrentItemL( -1 );
									else
										NotifyObserversL( EMoveFocusUp, iCurrentItem );
								}
						}
						return EKeyWasConsumed;

 					case JOY_CLICK:
 						{
 							NotifyObserversL( CSoftScrollList::EItemClicked, iCurrentItem );
 						}
 						return EKeyWasConsumed;
					default:
						return EKeyWasNotConsumed;
					}
			}
		return EKeyWasNotConsumed;
	}

	MJuikControl* At(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("GetItemL"));
		return iControls[aIndex];
	}
	
private:
#ifdef SOFT_SCROLLING
	
	void StartScrollL() 
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("StartScrollL"));
		iScrollDuration = TTimeIntervalMicroSeconds(300 * 1000);
		iAnimation->Stop();
		iAnimation->StartL( *this, iScrollDuration );
	}


	// From CJuikAnimation::MAnimated

	virtual void ProgressL(TReal aProgress) 
	{
		//iEikonEnv->WsSession().Flush();
		TInt d = aProgress * iScrollAnim.Delta();
		TInt y = iScrollAnim.iStartY + d;
		iSizer->SetScrollPositionL( y );
		DrawDeferred();		
	}
	
	virtual void FinishedL(TReal aProgress) 
	{
		TInt y = iScrollAnim.iEndY;
		if ( aProgress < 1.0 )
			{
				TInt d = aProgress * iScrollAnim.Delta();
				y = iScrollAnim.iStartY + d;
			}
		iSizer->SetScrollPositionL( y );
	}
	
	struct TScrollAnimDetails
	{
		TInt iStartY;
		TInt iEndY;
		TInt Delta() { return iEndY - iStartY; }
	} iScrollAnim;
	
	TTimeIntervalMicroSeconds iScrollDuration;
	CJuikAnimation* iAnimation;	
#endif
	
	void InitControlsL()
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("InitControlsL"));
		//iSizer = Juik::CreateBoxSizerL(Juik::EVertical);
		iSizer = Juik::CreateFixedWidthSizerL();

	}

	
	void GetFullyVisibleItemsL(TInt& aFirst, TInt& aCount)
	{
		CALLSTACKITEM_N(_CL("CSoftScrollListImpl"), _CL("GetFullyVisibleItemsL"));
		aFirst = KErrNotFound;
		aCount = 0;

		TRect viewR = Rect();
		TInt viewTop = viewR.iTl.iY;
		TInt viewBottom = viewR.iBr.iY;

		if ( viewR.Height() == 0 ) 
			return;
		
		for( TInt i=0; i < iControls.Count(); i++)
			{
				TRect itemR = iControls[i]->CoeControl()->Rect();
				TInt top = itemR.iTl.iY;
				TInt bottom = itemR.iBr.iY;
				if ( itemR.Height() > 0 )
					if ( viewTop <= top &&  bottom <= viewBottom )
						{
							aCount++;
							if ( aFirst == KErrNotFound )
								aFirst = i;
						}
			}
	}


	virtual const CCoeControl* CoeControl() const { return this; }
	CCoeControl* CoeControl() { return this; }


	TBool iChildrenDrawFocus;
	TInt iFlags;
	TInt iCurrentItem;

	
	RPointerArray<MObserver> iObservers;
	RPointerArray<MJuikControl> iControls;
	MJuikScrollableSizer* iSizer;

};

EXPORT_C CSoftScrollList* CSoftScrollList::NewL(CCoeControl* aParent, TBool aChildrenDrawFocus)
{
	CALLSTACKITEMSTATIC_N(_CL("CSoftScrollList"), _CL("NewL"));
	return CSoftScrollListImpl::NewL( aParent, aChildrenDrawFocus );
};
