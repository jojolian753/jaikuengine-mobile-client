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

#include "rotatinglist.h"

#include "juik_scrolllist.h"
#include "juik_keycodes.h"
#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "errorhandling.h"

#include <eiklabel.h>
#include <COECNTRL.H>

class CRotatingListImpl  : public CRotatingList, public MContextBase
{
public:

	// models N items
	// has M controls
	// K controls are are drawn (part of CJuikScrollList)
	// L controls are (partly) visible
	

	// when current item is changed
	// change [drawn controls] so that there is k items before and k items after current item 
	//   this means we have a mapping "list index" -> "control"  
	//   and "control" -> "list index" 
	//   remove controls that are not rendering list item in range [c-k, c+k]
	//   add controls for missing items 
	
	// if controls for items are homogenic, a pool of controls can exists
	// and adding and removing controls are just an act of taking a control from free pool,
	// calling it's update function with data of new item and adding control to right position
	// in drawn control list. 
	
	// when adding and removing items, scrolllist should not relayout or redraw itself
	// preferably, 
	//    when control becomes visible, head or tail is layed out.
	// 
	// 
	// drawn items 
	// current item
	// control pool
	
// 	void ItemChangedL()
// 	{
// 		TInt c = iCurrentDataItem;
// 		TInt k = 5; 
// 		TInt lower = c - k; 
// 		lower = Max(0, lower);
// 		TInt upper = c + k; 
// 		upper = Min(iModel->DataCount() - 1, upper);


// 		// Remove unused controls
// 		for (TInt i = 0; i < iUsedControls.Count(); i++)
// 			{
// 				TControlIndex p = iUsedControls[i];
// 				if (  lower <= p.iDataIndex && p.iDataIndex <= upper )
// 					{
// 						// ok
// 					}
// 				else
// 					{
// 						iScrollList->RemoveChildL( i );
// 						iUsedControls[i].iDataIndexRemove[i];
// 					}
// 			}
// 	}
	
// 	

	class CListItemControl : public CCoeControl
	{
	public:
		virtual void UpdateL(const TDesC& aData) = 0;
	};
	
	class CTextControl : public CListItemControl
	{
	public:
		static CTextControl* NewL(CCoeControl* aParent)
		{
			auto_ptr<CTextControl> self( new (ELeave) CTextControl );
			self->ConstructL(aParent);
			return self.release();
		}

		CTextControl() {}
		
		~CTextControl() 
		{
			delete iLabel;
		}

		TSize MinimumSize()
		{
			return iLabel->MinimumSize();
		}

		void ConstructL(CCoeControl* aParent)
		{
			SetContainerWindowL( *aParent );
			
			iLabel = new (ELeave) CEikLabel;
			iLabel->SetContainerWindowL( *this );
			
		}

		TInt CountComponentControls() const
		{
			return 1;
		}
	
		CCoeControl* ComponentControl(TInt aIndex) const
		{
			return iLabel;
		}

		void SizeChanged()
		{
			TRect r = Rect();
			iLabel->SetRect( r );
		}

		void UpdateL(const TDesC& aData)
		{
			iLabel->SetTextL( aData );
		}
		
		CEikLabel* iLabel;
	};



	~CRotatingListImpl()
	{
		delete iScrollList;
		iFreePool.ResetAndDestroy();
		iUsedPool.ResetAndDestroy();
		delete iModel;
	}
	
	static CRotatingListImpl* NewL(CCoeControl* aParent)
	{
		auto_ptr<CRotatingListImpl> self( new (ELeave) CRotatingListImpl );
		self->ConstructL(aParent);
		return self.release();
	}


	void ConstructL(CCoeControl* aParent)
	{
		SetContainerWindowL( *aParent );
		iScrollList = CSoftScrollList::NewL(this);
		InitModelL();
		InitListL();
	}
	
	TInt DrawControlCount() const
	{
		return iScrollList->ItemCount();
	}

	TInt Ray() const
	{
		return 12;
	}

	TInt StaticRay() const
	{
		return 5;
	}

	void InitListL()
	{
		const TInt KFreePoolSize(30);
		
		for(TInt i=0; i < KFreePoolSize; i++)
			{
				auto_ptr<CTextControl > c( CTextControl::NewL(iScrollList) );
				iFreePool.AppendL( c.get() );
				c.release();
			}
	
		
		iCurrent = 20;
		iViewPortMiddle = iCurrent;
		iTop = TopEdge();
		TInt bottom = BottomEdge();
		for(TInt i= iTop; i <= bottom; i++)
			{
				auto_ptr<CTextControl> ctrl( PopFromFreePool() );
				ctrl->UpdateL( iModel->MdcaPoint(i) );
				ctrl->ActivateL();
				iScrollList->AddChildL( ctrl.get() );
				iUsedPool.AppendL( ctrl.get() );
				ctrl.release();
			}
		SetFocusL(iCurrent);
	}

	TInt LastIxFreePool() const
	{
		return iFreePool.Count() - 1;
	}
	
	CTextControl* PopFromFreePool()
	{
		TInt last = LastIxFreePool();
		if ( last < 0 ) Bug( _L("Pool of free list controls is empty") ).Raise();
		
		auto_ptr<CTextControl> c( iFreePool[last] );
		iFreePool.Remove(last);
		return c.release();
	}


	void MoveToFreePoolL( CTextControl* aControl )
	{
		TInt ix = iUsedPool.Find( aControl );
		if ( ix < 0 )
			Bug( _L("Visible control not in used pool") ).Raise();
		iFreePool.AppendL( aControl );
		iUsedPool.Remove(ix);
	}
	

	void RemoveTopVisibleControlL()
	{
		CTextControl* c = static_cast<CTextControl*>( iScrollList->PopTopChildL( ) );
		MoveToFreePoolL( c );
	}


	void RemoveBottomVisibleControlL()
	{
		CTextControl* c = static_cast<CTextControl*>( iScrollList->PopBottomChildL( ) );
		MoveToFreePoolL( c );
	}


	void SetFocusL(TInt aIndex)
	{
		iCurrent = aIndex;
		CalculateControlsAndSetFocusL();
	}


	void CalculateControlsL() 
	{
		// Remove extra top items 
		TInt newTop = TopEdge();
		if (newTop == iTop)
			return;
		
		for (TInt i = 0; i < newTop - iTop; i++) 
			{ 
				RemoveTopVisibleControlL();
			}
		iTop = Min(newTop, iTop);

		// Remove extra bottom items 
		TInt newBottom = BottomEdge();
		TInt currentBottom = iTop + iScrollList->ItemCount();
		for (TInt i = 0; i < currentBottom - newBottom; i++)
			{
				RemoveBottomVisibleControlL();	
			}
		
		TBool added = EFalse;
		// Add extra top items
		for (TInt i=iTop - 1; i >= newTop; i--)
			{
				auto_ptr<CTextControl> ctrl( PopFromFreePool() );
				ctrl->UpdateL( iModel->MdcaPoint(i) );
				ctrl->ActivateL();
				iScrollList->InsertChildL( 0, ctrl.get() );
				iUsedPool.AppendL( ctrl.get() );
				ctrl.release();
				added = ETrue;
			}
		iTop = newTop;

		// Add extra bottom items 
		currentBottom = iTop + iScrollList->ItemCount();
		for (TInt i=currentBottom + 1; i <= newBottom; i++)
			{
				auto_ptr<CTextControl> ctrl( PopFromFreePool() );
				ctrl->UpdateL( iModel->MdcaPoint(i) );
				ctrl->ActivateL();
				iScrollList->AddChildL( ctrl.get() );
				iUsedPool.AppendL( ctrl.get() );
				ctrl.release();
				added = ETrue;
			}
		
		for (TInt i=0; i < iFreePool.Count(); i++)
			{
				iFreePool[i]->MakeVisible(EFalse);
			}
		
		for (TInt i=0; i < iUsedPool.Count(); i++)
			{
				iUsedPool[i]->MakeVisible(ETrue);
			}	
		
		TInt lbIx = iCurrent - iTop;
		iScrollList->SetCurrentItem( lbIx );
		iScrollList->ReLayoutL();
		//SizeChanged();
	}
	

	void CalculateControlsAndSetFocusL()
	{
		if ( iCurrent < TopEdge() || BottomEdge() < iCurrent )
			{
				iViewPortMiddle = iCurrent;
				CalculateControlsL();
			} 
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
	
	

// 	const TInt KRadar(6);

 	TInt Limited(TInt aMin, TInt aMax, TInt aValue)
 	{
 		TInt r = Max( aMin, aValue );
 		r = Min( aMax, aValue );
 		return r;
 	}


	TInt TopEdge()
 	{
 		TInt e = iViewPortMiddle - Ray();
 		e = Limited(0, iModel->Count() - 1, e);
 		return e;
 	}

 	TInt BottomEdge()
 	{
 		TInt e = iViewPortMiddle + Ray();
 		e = Limited(0, iModel->Count() - 1, e);
 		return e;
 	}
	
// 	TInt iTopDrawn;
// 	TInt BottomDrawn();

	
// 	void SetCurrentItemL(TInt aIndex)
// 	{
// 		iCurrentItem = aIndex;

// 		RArray<TInt> toBeRemoved;
		
// 		TInt top = TopEdge();
// 		TInt bottom = BottomEdge();
// 		// remove all items out of radar 
// 		for (TInt i=iTopDrawn; i < top; i++)
// 			{
// 				toBeRemoved.Append(i);
// 			}
// 		for (TInt i=bottom+1; i <= BottomDrawn(); i++)
// 			{
// 				toBeRemoved.Append(i);
// 			}
		
		
// 		// 
// 	}
	

	void SizeChanged()
	{
		TRect r = Rect();
		iScrollList->SetRect(r);
	}
	
	
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
 		if ( aType == EEventKey )
 			{
 				switch ( aKeyEvent.iCode )
					{
					case JOY_DOWN:
						{
							iCurrent++;
							iCurrent = Limited(0, iModel->Count() - 1, iCurrent);
							SetFocusL( iCurrent );
						}
						return EKeyWasConsumed;

					case JOY_UP:
						{
							iCurrent--;
							iCurrent = Limited(0, iModel->Count() - 1, iCurrent);
							SetFocusL( iCurrent );
						}
						return EKeyWasConsumed;
						
					default:
						return EKeyWasNotConsumed;
					}
			}
		return EKeyWasNotConsumed;
	}
	
	void InitModelL()
	{
		const TInt KModelCount(100);
		iModel = new (ELeave) CDesCArrayFlat(100);
		for (TInt i=0; i < KModelCount; i++)
			{
				TBuf<10> num;
				num.Num(i);
				
				iModel->AppendL( num );
			}
	}


// 	void FocusChanged()
// 	{
// 	    // add controls if 
// 		TInt k = 8;
// 		TInt c = iCurrentItem;
// 		if ( 
// 	}

	CDesCArray* iModel;

	CSoftScrollList* iScrollList;
	RPointerArray<CTextControl> iUsedPool;
	RPointerArray<CTextControl> iFreePool;

	TInt iViewPortMiddle;
	TInt iCurrent;
	TInt iTop;
};


CRotatingList* CRotatingList::NewL(CCoeControl* aParent)
{
	return CRotatingListImpl::NewL( aParent );
}
