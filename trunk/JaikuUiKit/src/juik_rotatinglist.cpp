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

#include "juik_rotatinglist.h"

#include "juik_scrolllist.h"
#include "juik_keycodes.h"
#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "errorhandling.h"
#include "reporting.h"

#include <eiklabel.h>
#include <COECNTRL.H>


class CRotatingListImpl  : public CRotatingList, public CSoftScrollList::MObserver, public MContextBase
{
public:


	CRotatingListImpl(MControlModel& aControlModel) : iControlModel(aControlModel), iCurrent( KErrNotFound ), iTop( KErrNotFound )
	{
	}
	
	
	~CRotatingListImpl()
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("~CRotatingListImpl"));
		iScrollList->RemoveObserver( *this );
		delete iScrollList;
		iUsedPool.ResetAndDestroy();
	}
	
	static CRotatingListImpl* NewL(CCoeControl* aParent, MControlModel& aControlModel)
	{
		CALLSTACKITEMSTATIC_N(_CL("CRotatingListImpl"), _CL("NewL"));
		auto_ptr<CRotatingListImpl> self( new (ELeave) CRotatingListImpl(aControlModel) );
		self->ConstructL(aParent);
		return self.release();
	}

	TInt CurrentItemIndex() const
	{
		return iCurrent;
	}

	void ConstructL(CCoeControl* aParent)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("ConstructL"));
		SetContainerWindowL( *aParent );
		const TInt KFlags = CSoftScrollList::EChildrenDrawFocus;
		iScrollList = CSoftScrollList::NewL(this, KFlags);
		iScrollList->AddObserverL( *this );

		iControlModel.InitControlModelL( iScrollList );
		if ( iControlModel.Count() > 0 )
			SetFocusAndInitL( 0, EFalse );
	}
	

	void FreeAllUsedControlsL()
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("FreeAllUsedControlsL"));
		while ( iUsedPool.Count() > 0 )
			{
				RemoveBottomVisibleControlL();
			}
		iCurrent = KErrNotFound;
		iTop = KErrNotFound;
	}

	TInt MaxUsedControls() const
	{
		return 8;
	}

	void CreateControlsFromScratchL(TInt aItemIndex)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("CreateControlsFromScratchL"));
		FreeAllUsedControlsL();

		const TInt KUsedCount = MaxUsedControls();
		const TInt KRay = KUsedCount / 2;
		TInt focused = aItemIndex;

		// calculate top
		iTop = LimitedToModelRange( focused - KRay );
		
		TInt last = iTop + KUsedCount - 1;		
		last = LimitedToModelRange( last );

		for(TInt i = iTop; i <= last; i++)
			{
				MJuikControl* ctrl = GetUpdatedControlL( i );
				iScrollList->AddChildL( ctrl );
			}
	}
	
	void SetFocusAndInitL(TInt aItemIndex)
	{
		SetFocusAndInitL( aItemIndex, ETrue );
	}

	void SetFocusAndInitL(TInt aItemIndex, TBool aDoLayout)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("SetFocusAndInitL"));
		if ( aItemIndex == iCurrent )
			return;

		CreateControlsFromScratchL(aItemIndex);
		if ( aDoLayout )
			SizeChanged();

		SetFocusL(aItemIndex);
	}
	

	virtual void AddObserverL( CSoftScrollList::MObserver& aObserver )
	{
		iScrollList->AddObserverL( aObserver );
	}
	
	virtual void RemoveObserver( CSoftScrollList::MObserver& aObserver ) 
	{
		iScrollList->RemoveObserver( aObserver );
	}
	
	void MoveToFreePoolL( MJuikControl* aControl )
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("MoveToFreePoolL"));
		TInt ix = iUsedPool.Find( aControl );
		if ( ix < 0 )
			Bug( _L("Visible control not in used pool") ).Raise();
		iControlModel.FreeControlL( aControl );
		iUsedPool.Remove(ix);
	}
	

	void RemoveTopVisibleControlL()
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("RemoveTopVisibleControlL"));
		MJuikControl* c = iScrollList->PopTopChildL( );
		iTop++;
		MoveToFreePoolL( c );
	}


	void RemoveBottomVisibleControlL()
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("RemoveBottomVisibleControlL"));
		MJuikControl* c = iScrollList->PopBottomChildL( );
		MoveToFreePoolL( c );
	}

 	TInt Limited(TInt aMin, TInt aMax, TInt aValue) const
 	{
		TInt r = aValue;
 		if ( r < aMin ) return aMin;
 		if ( r > aMax ) return aMax;
 		return r;
 	}
	

	TInt LimitedToModelRange(TInt aValue) const
	{
		return Limited(0, iControlModel.Count() - 1, aValue);
	}


	TInt CurrentBottomIndex() const
	{
		if ( iTop == KErrNotFound ) return KErrNotFound;
		else return iTop + iUsedPool.Count() - 1; 
	}
	

	MJuikControl* GetUpdatedControlL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("GetUpdatedControlL"));
		auto_ptr<MJuikControl> ctrl( iControlModel.GetControlL( aIndex, iScrollList ) );
		TInt lbIx = iTop >= 0 ? aIndex - iTop : 0;
		lbIx = Max(0,lbIx);
		lbIx = Min(lbIx, iUsedPool.Count() );
		iUsedPool.InsertL( ctrl.get(), lbIx );
		return ctrl.release();
	}


	TBool ShouldRotateDown(TInt aNew)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("ShouldRotateDown"));
		if ( iCurrent == KErrNotFound || iTop == KErrNotFound )
			Bug( _L("ShouldRotate called when not ready") ).Raise();
		
		const TInt KRange(2);
		TInt bottom = CurrentBottomIndex();
		return (aNew > bottom - KRange) && bottom < iControlModel.Count() - 1;
	}
	
	TBool ShouldRotateUp(TInt aNew)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("ShouldRotateUp"));
		if ( iCurrent == KErrNotFound || iTop == KErrNotFound )
			Bug( _L("ShouldRotate called when not ready") ).Raise();
		
		const TInt KRange(2);
 		return (aNew < iTop + KRange) && iTop > 0;
	}
	
	TBool MovingUp( TInt aNew )
	{
		return aNew < iCurrent;
	}
	
	void RotateL(TBool aFromBottomToTop)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("RotateL"));
		if ( iUsedPool.Count() >= MaxUsedControls()  )
			{
				if ( aFromBottomToTop )
					{
						RemoveBottomVisibleControlL();
					}
				else
					{
						RemoveTopVisibleControlL();
					}
			}
		
		// fill it 
		if ( aFromBottomToTop )
			{
				TInt newTop =  LimitedToModelRange(iTop - 1 );
				if ( newTop != iTop )
					{
						// put it to the top
						MJuikControl* ctrl = GetUpdatedControlL( newTop );
						iScrollList->PutAndLayoutTopChildL( ctrl );
						ctrl->CoeControl()->ActivateL();
						iTop = newTop;
					}
			}
		else
			{
				TInt oldBottom = CurrentBottomIndex();
				TInt newBottom =  LimitedToModelRange( oldBottom + 1 );
				if ( newBottom != oldBottom )
					{
						MJuikControl* ctrl = GetUpdatedControlL( newBottom );
						iScrollList->PutAndLayoutBottomChildL( ctrl );
						ctrl->CoeControl()->ActivateL();
					}
			}
	}
	

	void InsertNewInTheMiddleL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("InsertNewInTheMiddleL"));
		
		if ( iUsedPool.Count() >= MaxUsedControls()  )
			{
				TInt deltaTop = Abs( iCurrent - iTop);
				TInt deltaBottom = Abs( iCurrent - CurrentBottomIndex());
				if ( deltaTop < deltaBottom )
					{
						if ( CurrentBottomIndex() < aIndex )
							return; // no point to remove 
						RemoveBottomVisibleControlL();
					}
				else
					{
						if ( aIndex <= iTop )
							return; // no point to remove 
						RemoveTopVisibleControlL();
					}
			}
		TInt lbIx = aIndex - iTop;
		if ( 0 <= lbIx && lbIx <= (CurrentBottomIndex() + 1) )
			{
				MJuikControl* ctrl = GetUpdatedControlL( aIndex );
				iScrollList->InsertAndLayoutChildrenL(lbIx, ctrl);
				ctrl->CoeControl()->ActivateL();
			}
	}

	
	void ChangeFocusL(TInt aDelta)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("ChangeFocusL"));
		if ( aDelta < -1 || 1 < aDelta )
			Bug( _L("Scrolling several items not supported") ).Raise();
		if ( iCurrent == KErrNotFound )
			return;

		TInt newFocus = LimitedToModelRange(iCurrent + aDelta);
		if ( newFocus != iCurrent )
			{
				TBool up = MovingUp(newFocus);
				TBool down = !up;
				if ( up && ShouldRotateUp(newFocus) )
					{
						RotateL( ETrue );
					}
				else if ( down && ShouldRotateDown(newFocus) )
					{
						RotateL( EFalse );
					}
				SetFocusL( newFocus );
			}
	}
	
	void SetFocusL(TInt aNewFocus)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("SetFocusL"));
		iCurrent = aNewFocus;
		TInt lbIx = iCurrent - iTop;
		iScrollList->SetFocusL( lbIx );
	}

	TInt CountComponentControls() const
	{
		return 1;
	}
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		return iScrollList;
	}
	


	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("SizeChanged"));
		TRect r = Rect();
		iScrollList->SetRect(r);
	}
	
	TBool IsEmpty() 
	{
		return iControlModel.Count() == 0;
	}

	virtual void HandleListEventL(CSoftScrollList::TEvent aEvent, TInt aIndex) 
	{
		if ( IsEmpty() )
			return;
		
		switch (aEvent) 
			{
			case CSoftScrollList::EMoveFocusUp:			
				ChangeFocusL( -1 );
				break;
			case CSoftScrollList::EMoveFocusDown:			
				ChangeFocusL( +1 );
				break;
			}
	}


	
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("OfferKeyEventL"));
//  		TBuf<100> tmpBuf;
// 		tmpBuf.Format( _L("\tRotatingList: type %d, code %d"), aType, aKeyEvent.iCode );
// 		Reporting().DebugLog( tmpBuf );		
		return iScrollList->OfferKeyEventL(aKeyEvent, aType);
	}

	MJuikControl* GetUsedControlL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("GetUsedControlL"));
		if (iTop == KErrNotFound)
			return NULL;

		if (iTop <= aIndex && aIndex <= CurrentBottomIndex())
			{
				TInt lbIx = aIndex - iTop;
				MJuikControl* c = iUsedPool[lbIx];
				return c;
			}
		return NULL;
	}
	
	void HandleItemUpdatedL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("HandleItemUpdatedL"));
		// 		if ( iTop == KErrNotFound)
		// 			return;
		
		// 		if ( iTop <= aIndex && aIndex <= CurrentBottomIndex() )
		// 			{
		// 				TInt lbIx = aIndex - iTop;
		// 				MJuikControl* c = iUsedPool[lbIx];
		// 				iControlModel.UpdateControlL( aIndex, c );
		// 				SizeChanged();
		// 				c->DrawDeferred();
		// 				// FIXME: this doesn't relayout! 
		// 			}
	}

	void UpdateAndRedrawItemL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("RedrawItemL"));
		if ( iTop == KErrNotFound)
			return;

		if ( iTop <= aIndex && aIndex <= CurrentBottomIndex() )
			{
				TInt lbIx = aIndex - iTop;
				MJuikControl* c = iUsedPool[lbIx];
				iControlModel.UpdateControlL( aIndex, c );
				c->CoeControl()->DrawDeferred();
				// FIXME: this doesn't relayout! 
			}
	}


	void HandleItemRemovedL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("HandleItemRemovedL"));
	}

	void HandleItemAddedL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("HandleItemAddedL"));
		if ( iCurrent == KErrNotFound )
			{
				SetFocusAndInitL(0);
				return;
			}

		if ( aIndex <= iCurrent ) iCurrent++;
		if ( aIndex <= iTop ) iTop++;
		
		if ( iTop < aIndex && aIndex <= CurrentBottomIndex() )
			{
				InsertNewInTheMiddleL(aIndex);
				SetFocusL( iCurrent ); //iScrollList->DrawDeferred(); //DrawNow(); //Deferred();
			}
	}

	void GetFullyVisibleItemsL(TInt& aFirst, TInt& aCount)
	{
		CALLSTACKITEM_N(_CL("CRotatingListImpl"), _CL("GetFullyVisibleItemsL"));
		aFirst = KErrNotFound;
		aCount = 0;
		iScrollList->GetFullyVisibleItemsL( aFirst, aCount );
		if ( aFirst != KErrNotFound )
			{
				aFirst = iTop + aFirst;
			}
	}

	MControlModel& iControlModel;
	
	CSoftScrollList* iScrollList;
 	RPointerArray<MJuikControl> iUsedPool;
	
	TInt iCurrent;
	TInt iTop;
};
	

EXPORT_C CRotatingList* CRotatingList::NewL(CCoeControl* aParent, MControlModel& aControlModel)
{
	CALLSTACKITEMSTATIC_N(_CL("CRotatingList"), _CL("NewL"));
	return CRotatingListImpl::NewL( aParent, aControlModel );
}
