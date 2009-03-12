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

#include "ccn_viewnavigation.h"

#include "app_context.h"
#include "break.h"
#include "errorhandling.h"
#include "symbian_auto_ptr.h"

#include <contextcontacts.rsg>

#include <akndef.h>
#include <aknappui.h>
#include <aknnavi.h>
#include <aknnavide.h>
#include <akntabgrp.h>
#include <aknsconstants.h>
#include <eikenv.h>
#include <eikspane.h> 
#include <barsread.h> 



/**
 * View navigation models view hierachy and handles displaying tabgroups correctly 
 */
class CViewNavigationImpl : public CViewNavigation, public MContextBase
{
public:
	CAknNavigationDecorator* TabDecoratorL(TInt aResourceId )
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("TabDecoratorL"));
		CAknNavigationDecorator* result = NULL;
		
		{
			TResourceReader reader;
			iEikEnv->CreateResourceReaderLC( reader, aResourceId );
			
			result = NaviPane()->CreateTabGroupL( reader );
			CleanupStack::PopAndDestroy();  // resource reader
		}

		return result;
	}
	

	TUid SiblingViewL(TInt aDelta)
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("SiblingViewL"));
		CAknTabGroup* g = TabGroup( iCurrentGroup );
		if ( g )
			{
				TInt activeIx = g->ActiveTabIndex();
				TInt count = g->TabCount();

				TInt newIx = activeIx + aDelta;
				newIx = Max(newIx, 0);
				newIx = Min(newIx, count-1);
				
				if (newIx != activeIx)
					{
						TInt viewId = g->TabIdFromIndex( newIx );
						TUid uid = {viewId};
						return uid;
					}
			}
		return TUid::Null();
	}
	
	virtual TUid LeftViewL() 
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("LeftViewL"));
		return SiblingViewL( -1 );
	}
	
	virtual TUid RightViewL()
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("RightViewL"));
		return SiblingViewL( +1 );
	}

	
	void ConstructL()
	{				
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("ConstructL"));
		iCurrentViewId = TUid::Null();
		if ( ! iEikEnv )
			iEikEnv = CEikonEnv::Static();

		ReadTabGroupsL(EFalse);
	}
	

	void ReadTabGroupsL(TBool aManageCurrent = ETrue)
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("ReadTabGroupsL"));

		if ( aManageCurrent )
			RemoveCurrentL();

		iTabDecorators.ResetAndDestroy();
		auto_ptr<CAknNavigationDecorator> deco( TabDecoratorL(R_PRESENCE_TAB_GROUP) );
		iTabDecorators.AppendL( deco.get() );
		deco.release();
		
		deco.reset( TabDecoratorL(R_CONTACT_LEVEL_TAB_GROUP) );
		iTabDecorators.AppendL( deco.get() );
		deco.release();
		
		deco.reset( TabDecoratorL(R_MAIN_LEVEL_TAB_GROUP) );
		iTabDecorators.AppendL( deco.get() );
		deco.release(); 		
		
		if ( aManageCurrent )
			SetCurrentViewL( CurrentView() );
	}
	
	void HandleResourceChangeL( TInt aType )
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("HandleResourceChangeL"));
		switch ( aType )
			{
			case KAknsMessageSkinChange:
			case KEikDynamicLayoutVariantSwitch:
				ReadTabGroupsL();
			}
	}


	TUid CurrentView() const
	{
		return iCurrentViewId;
	}
   

	void SetCurrentViewL( TUid aViewId )
	{
		SetCurrentViewL( aViewId.iUid );
	}	

	void SetCurrentViewL( TInt aViewId )
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("SetCurrentViewL"));
		iCurrentViewId.iUid = aViewId;
		CAknTabGroup* currentGroup = TabGroup( iCurrentGroup );
		if ( currentGroup ) 
			{
				TInt ix = FindTabFromGroupL( aViewId, *currentGroup );
				if ( ix >= 0 )
					{
						currentGroup->SetActiveTabByIndex( ix );
						return;
					}
			}

		TInt gIx = 0;
		for (TInt i=0; i < GroupCount(); i++)
			{		
				CAknTabGroup* g = TabGroup(i);
				TInt ix = FindTabFromGroupL( aViewId, *g );
				if ( ix >= 0) 
					{
						ActivateTabGroupL( i, ix );
						return;
					}
			}
		if ( currentGroup )
			{
				// not found
				RemoveCurrentL();
			}
	}
	

	void ActivateTabL( TInt aTabIx )
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("ActivateTabL"));
		CAknTabGroup* g = TabGroup( iCurrentGroup );
		if (g) g->SetActiveTabByIndex( aTabIx );
		else Bug(_L("Activating tab, but there is no tab group")).Raise();
	}
	
	// if viewid is in current tab group
	//   activate it's tab
	// else 
	//   find group where current view is
	//   if found 
	//      set group to navi and activate view's tab
	//   else 
	//      remove tab group from navi

	void ActivateTabGroupL( TInt aGroupIx, TInt aTabIx )
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("ActivateTabGroupL"));
		if ( iCurrentGroup >= 0 )
			{
				RemoveCurrentL();
			}
		iCurrentGroup = aGroupIx;
		CAknTabGroup* g = TabGroup( aGroupIx );

		g->SetActiveTabByIndex( aTabIx );
		NaviPane()->PushL( *(iTabDecorators[ aGroupIx ]) );
	}
	

	void RemoveCurrentL()
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("RemoveCurrentL"));
		if ( iCurrentGroup >= 0 )
			{
				NaviPane()->Pop( iTabDecorators[ iCurrentGroup ] );
				iCurrentGroup = KErrNotFound;
			}
	}


	TInt FindTabFromGroupL( TInt aId, CAknTabGroup& aGroup )
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("FindTabFromGroupL"));
		for (TInt i=0; i < aGroup.TabCount(); i++ )
			{				
				if ( aGroup.TabIdFromIndex(i) == aId )
					return i;
			}
		return KErrNotFound;
	}


// 	TInt AskLeftViewL( TInt aViewId )
// 	{
		
// 	}


// 	TInt AskRightViewL( TInt aViewId )
// 	{
		
// 	}


	TInt GroupCount()
	{
		return iTabDecorators.Count();
	}


	CAknTabGroup* TabGroup(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("TabGroup"));
		TInt count = iTabDecorators.Count();
		if ( 0 <= aIndex && aIndex < count)
			 return static_cast<CAknTabGroup*>( iTabDecorators[ aIndex ]->DecoratedControl() );
		else 
			return NULL;
	}

	
	CAknNavigationControlContainer* NaviPane()
	{
		CALLSTACKITEM_N(_CL("CViewNavigationImpl"), _CL("NaviPane"));
		if ( ! iEikEnv )
			iEikEnv = CEikonEnv::Static();
		
		if ( ! iNaviPane )
			{
				CEikStatusPane *sp = 
					( ( CAknAppUi* ) iEikEnv->EikAppUi() )->StatusPane();
				// Fetch pointer to the default navi pane control
				iNaviPane = ( CAknNavigationControlContainer * )
					sp->ControlL( TUid::Uid( EEikStatusPaneUidNavi ) );
			}
		return iNaviPane;
	}


	~CViewNavigationImpl()
	{
		iTabDecorators.ResetAndDestroy();
	}


private:	
	CAknNavigationControlContainer* iNaviPane; // not own
	CEikonEnv* iEikEnv; // not own;

	TInt iCurrentGroup;
	RPointerArray<CAknNavigationDecorator> iTabDecorators;	
	TUid iCurrentViewId;
};

EXPORT_C CViewNavigation* CViewNavigation::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CViewNavigation"), _CL("NewL"));
	auto_ptr<CViewNavigationImpl> self( new (ELeave) CViewNavigationImpl);
	self->ConstructL();
	return self.release();
}
