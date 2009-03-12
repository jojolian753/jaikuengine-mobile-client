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


	CRotatingListImpl(MControlModel& aControlModel) : iControlModel(aControlModel), iCurrent( KErrNotFound ), iTop( KErrNotFound )
	{
	}
	
	
	~CRotatingListImpl()
	{
		delete iScrollList;
		iFreePool.ResetAndDestroy();
		iUsedPool.ResetAndDestroy();
	}
	
	static CRotatingListImpl* NewL(CCoeControl* aParent, MControlModel& aControlModel)
	{
		auto_ptr<CRotatingListImpl> self( new (ELeave) CRotatingListImpl(aControlModel) );
		self->ConstructL(aParent);
		return self.release();
	}


	void ConstructL(CCoeControl* aParent)
	{
		SetContainerWindowL( *aParent );
		iScrollList = CSoftScrollList::NewL(this);
		InitListL();
	}
	

	void InitListL()
	{
 		const TInt KFreePoolSize(12);

 		for(TInt i=0; i < KFreePoolSize; i++)
			{
				// IMPLEMENTATION SPECIFIC: Create a control
				auto_ptr<CCoeControl > c( iControlModel.CreateFreeControlL( iScrollList ) );
 				iFreePool.AppendL( c.get() );
 				c.release();
 			}
		SetFocusAndInitL(0);
	}

	void FreeAllUsedControlsL()
	{
		while ( iUsedPool.Count() > 0 )
			{
				RemoveBottomVisibleControlL();
			}
		iCurrent = KErrNotFound;
		iTop = KErrNotFound;
	}
	
	void SetFocusAndInitL(TInt aItemIndex)
	{
		FreeAllUsedControlsL();
		
		
		iCurrent = aItemIndex;
		iTop = iCurrent;
		TInt last = iCurrent + iFreePool.Count() - 1;		
		last = Limited( iCurrent, iControlModel.Count() - 1, last);
		for(TInt i = iCurrent; i <= last; i++)
			{
				auto_ptr<CCoeControl> ctrl( PopFromFreePool() );
				iControlModel.UpdateControlL( ctrl.get(), i );
				ctrl->ActivateL();
				iScrollList->AddChildL( ctrl.get() );
				iUsedPool.AppendL( ctrl.get() );
				ctrl.release();
			}
	}
	
	TInt LastIxFreePool() const
	{
		return iFreePool.Count() - 1;
	}
	
	CCoeControl* PopFromFreePool()
	{
		TInt last = LastIxFreePool();
		if ( last < 0 ) Bug( _L("Pool of free list controls is empty") ).Raise();
		
		auto_ptr<CCoeControl> c( iFreePool[last] );
		iFreePool.Remove(last);
		return c.release();
	}


	void MoveToFreePoolL( CCoeControl* aControl )
	{
		TInt ix = iUsedPool.Find( aControl );
		if ( ix < 0 )
			Bug( _L("Visible control not in used pool") ).Raise();
		iFreePool.AppendL( aControl );
		iUsedPool.Remove(ix);
	}
	

	void RemoveTopVisibleControlL()
	{
		CCoeControl* c = static_cast<CCoeControl*>( iScrollList->PopTopChildL( ) );
		iTop++;
		MoveToFreePoolL( c );
	}


	void RemoveBottomVisibleControlL()
	{
		CCoeControl* c = static_cast<CCoeControl*>( iScrollList->PopBottomChildL( ) );
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
	
	
	TBool ShouldRotate(TInt aNew)
	{
		const TInt KRange(2);
		TInt delta = aNew - iCurrent;
		return ( delta < 0 && (aNew < iTop + KRange) )
			|| ( delta > 0 && (aNew > CurrentBottomIndex() - KRange) );
	}
	
	TBool MovingUp( TInt aNew )
	{
		return aNew < iCurrent;
	}

	void ChangeFocusL(TInt aDelta)
	{
		if ( aDelta < -1 || 1 < aDelta )
			Bug( _L("Scrolling several items not supported") ).Raise();
			
		TInt newFocus = LimitedToModelRange(iCurrent + aDelta);
		
		if ( newFocus != iCurrent )
			{
				if ( ShouldRotate(newFocus) )
					{
						if ( iFreePool.Count() == 0 )
							{
								if ( MovingUp(newFocus) )
									{
										RemoveBottomVisibleControlL();
									}
								else
									{
										RemoveTopVisibleControlL();
									}
							}
						
						// fill it 
						if ( MovingUp(newFocus) )
							{
								TInt newTop =  LimitedToModelRange(iTop - 1 );
								if ( newTop != iTop )
									{
										auto_ptr<CCoeControl> ctrl( PopFromFreePool() );
										iControlModel.UpdateControlL( ctrl.get(), newTop );
										ctrl->ActivateL();						
										// put it to the top
										iScrollList->PutAndLayoutTopChildL( ctrl.get() );
										iUsedPool.InsertL( ctrl.get(), 0 );
										ctrl.release();
										iTop = newTop;
									}
							}
						else
							{
								TInt oldBottom = CurrentBottomIndex();
								if ( newFocus > oldBottom - 2 )
									{
										TInt newBottom =  LimitedToModelRange( oldBottom + 1 );
										if ( newBottom != oldBottom )
											{
												// IMPLEMENTATION SPECIFIC
												auto_ptr<CCoeControl> ctrl( PopFromFreePool() );
												iControlModel.UpdateControlL( ctrl.get(), newBottom );
												ctrl->ActivateL();						
												// put it to the bottom
												iScrollList->PutAndLayoutBottomChildL( ctrl.get() );
												iUsedPool.AppendL( ctrl.get() );
												ctrl.release();
											}
									}
							}
					}
				iCurrent = newFocus;
				TInt lbIx = iCurrent - iTop;
				iScrollList->SetFocusL( lbIx );
			}
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
							ChangeFocusL( +1 );
						}
						return EKeyWasConsumed;

					case JOY_UP:
						{
							ChangeFocusL( -1 );
						}
						return EKeyWasConsumed;
						
					default:
						return EKeyWasNotConsumed;
					}
			}
		return EKeyWasNotConsumed;
	}

	MControlModel& iControlModel;
	
	CSoftScrollList* iScrollList;
 	RPointerArray<CCoeControl> iUsedPool;
 	RPointerArray<CCoeControl> iFreePool;
	
	TInt iCurrent;
	TInt iTop;
};
	

CRotatingList* CRotatingList::NewL(CCoeControl* aParent, MControlModel& aControlModel)
{
	return CRotatingListImpl::NewL( aParent, aControlModel );
}
