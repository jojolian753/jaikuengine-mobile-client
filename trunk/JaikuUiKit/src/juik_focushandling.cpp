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

#include "juik_focushandling.h"
#include "juik_keycodes.h"

#include "app_context.h"

class CFocusHandlingImpl : public CFocusHandling, public MContextBase 
{
public:
	~CFocusHandlingImpl() 
	{
		iFocusers.Close();
	}

	void ConstructL()
	{
	}

	virtual void AddL(MJuikControl& aControl)
	{
		iFocusers.Append( &aControl );
	}
	
	virtual TBool PreFocusGained(MJuikControl::TFocusFrom aFocusFrom)
	{
		TInt count = iFocusers.Count();
		if ( count > 0 )
			{
				TBool fromAbove = aFocusFrom == MJuikControl::EAbove;
				if ( fromAbove )
					{
						iCurrentlyFocused = 0;
					}
				else
					{
						iCurrentlyFocused = count - 1;
					}
				
				
				if ( InnerFocused()->PreFocusGained(aFocusFrom) )
					return ETrue;
				else
					{
						FindNextInnerFocus( fromAbove );
						if ( IsValidInnerFocus() )							
							return ETrue;
						else
							return EFalse;
					}
			}
		else
			{
				iCurrentlyFocused == KErrNotFound;
				return EFalse;
			}
	}

	virtual void FocusChanged(TBool aIsFocused, TDrawNow aDrawNow)
	{
		CALLSTACKITEM_N(_CL("CFocusHandlingImpl"), _CL("FocusChanged"));
		// Focus first one, if PreFocusGained hasn't been called
		if ( iCurrentlyFocused < 0 )
			PreFocusGained( MJuikControl::EAbove );
		
		if ( IsValidInnerFocus() )
			{
				const TInt c = iCurrentlyFocused;
				InnerFocused()->CoeControl()->SetFocus( aIsFocused, aDrawNow );
			}
	}
	

	void FindNextInnerFocus(TBool aFromAbove)
	{
		TInt delta = aFromAbove ? +1 : -1;
		MJuikControl::TFocusFrom from = aFromAbove ? 
			MJuikControl::EAbove : MJuikControl::EBelow;

		MJuikControl* old = InnerFocused();

		iCurrentlyFocused += delta;
		while ( 0 <= iCurrentlyFocused && iCurrentlyFocused < iFocusers.Count() )
			{
				TBool needsFocus = InnerFocused()->PreFocusGained( from );
				if ( needsFocus )
					break;
				else
					iCurrentlyFocused += delta;
			}
	}		
	
// 	TKeyResponse FocusNextInnerL(TBool aFromAbove)
// 	{
// 		FindNextInnerFocus( aFromAbove );
// 		if ( IsValidInnerFocus() )
// 			{
// 				if ( old ) 
// 					old->CoeControl()->SetFocus( EFalse ); //, ENoDrawNow );
				
// 				InnerFocused()->CoeControl()->SetFocus( ETrue ); //, ENoDrawNow );
// 				return EKeyWasConsumed;
// 			}
// 		else
// 			{
// 				iCurrentlyFocused = KErrNotFound;
// 				return EKeyWasNotConsumed;
// 			}
// 	}
	
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
		if ( aType == EEventKey )
			{
				if ( aKeyEvent.iCode == JOY_DOWN || aKeyEvent.iCode == JOY_UP )
					{
						MJuikControl* old = InnerFocused();
						if ( old )
							{
								TKeyResponse response = old->CoeControl()->OfferKeyEventL( aKeyEvent, aType );
								if ( response == EKeyWasNotConsumed )
									{
										TBool fromAbove = aKeyEvent.iCode == JOY_DOWN;
										FindNextInnerFocus( fromAbove );
										if ( IsValidInnerFocus() )
											{
												if ( old ) 
													old->CoeControl()->SetFocus( EFalse ); //, ENoDrawNow );
												
												InnerFocused()->CoeControl()->SetFocus( ETrue ); //, ENoDrawNow );
												return EKeyWasConsumed;
											}
										else
											{
												iCurrentlyFocused = KErrNotFound;
												return EKeyWasNotConsumed;
											}

									}
								else 
									return EKeyWasConsumed;
							}
					}
			}
		return EKeyWasNotConsumed;
	}
	
	virtual TBool IsValidInnerFocus() const 
	{ 
		return 0 <= iCurrentlyFocused && iCurrentlyFocused < iFocusers.Count(); 
	} 
	virtual MJuikControl* InnerFocused() const 
	{
		if ( IsValidInnerFocus() )
			return iFocusers[iCurrentlyFocused];
		else
			return NULL;
	}
	virtual void MoveInnerFocus( TInt aDelta )
	{
		TInt f = iCurrentlyFocused + aDelta;
		if ( 0 <= f && f < iFocusers.Count() )
			iCurrentlyFocused = f;
		if ( ! IsValidInnerFocus() )
			iCurrentlyFocused = KErrNotFound;
	}

 protected:
	RPointerArray<MJuikControl> iFocusers;
	TInt iCurrentlyFocused;
};
	
	EXPORT_C CFocusHandling* CFocusHandling::NewL()
{
	auto_ptr<CFocusHandlingImpl> self( new (ELeave) CFocusHandlingImpl );
	self->ConstructL();
	return self.release();
}
