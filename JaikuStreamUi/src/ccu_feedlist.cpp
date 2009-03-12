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

#include "ccu_feedlist.h"

#include "ccu_feedcontrols.h"

#include "ccu_feedmodel.h"
#include "ccu_feeditem.h"
#include "ccu_userpics.h"
#include "ccu_jaicons.h"
#include "ccu_themes.h"
#include "ccu_timeperiod.h"
#include "ccu_activestate.h"

#include "contextcontacts.hrh"

#include <contextcontactsui.mbg>

#include "break.h"
#include "callstack.h"
#include "cc_stringtools.h"
#include "csd_feeditem.h"
#include "jabberdata.h"
#include "jaiku_layoutids.hrh"
#include "juik_subcellrenderer.h"
#include "juik_listbox.h"
#include "juik_iconmanager.h"
#include "juik_icondataprovider.h"
#include "juik_icons.h"
#include "juik_keycodes.h"
#include "juik_layout.h"
#include "juik_rotatinglist.h"
#include "symbian_auto_ptr.h"
#include "timeout.h"
#include "reporting.h"

#include <aknappui.h>
#include <aknutils.h>  
#include <aknview.h> 
#include <eiklbo.h>



class CReadMarker : public CCheckedActive, public MContextBase, public MTimeOut
{
public:
	
	static CReadMarker* NewL(MFeedModel& aFeedModel, CRotatingList& aList)
	{
		CALLSTACKITEMSTATIC_N(_CL("CReadMarker"), _CL("NewL"));
		auto_ptr<CReadMarker> self( new (ELeave) CReadMarker(aFeedModel, aList) );
		self->ConstructL();
		return self.release();
	}

	virtual ~CReadMarker()
	{
		CALLSTACKITEM_N(_CL("CReadMarker"), _CL("~CReadMarker"));
		Cancel();
		delete iTimeOut;
		SyncWriteAllMarksToDbL();
		iMarked.Close();
		iUnmarked.Close();
	}
	
	CFeedControl* GetFeedControlL(TInt aIndex) const 
	{
		MJuikControl* c = iList.GetUsedControlL( aIndex );
		if ( c )	
			{
				CFeedControl* fc = static_cast<CFeedControl*>( c->CoeControl() );
				return fc;
			}
		return NULL;
	}
	

	void MarkVisibleItemsReadL()
	{
		CALLSTACKITEM_N(_CL("CReadMarker"), _CL("MarkVisibleItemsReadL"));
		TInt first, count;
		first = KErrNotFound;
		count = 0;
		
		iList.GetFullyVisibleItemsL(first, count);
		for (TInt i=first; i < first + count; i++ )
			{
				CFeedControl* fc = GetFeedControlL( i );
 				if ( fc )	
 					{
						TInt id = fc->Id();
						MarkIdL( id );
					}
			}
		// Always mark current item (even if not fully visible)
		{
			TInt current = iList.CurrentItemIndex();
			CFeedControl* fc = GetFeedControlL( current );
			if ( fc )	
				{
					TInt id = fc->Id();
					MarkIdL( id );
				}
		}

		iTimeOut->WaitMax(1);
	}
	

//  	void WriteMarksToDbL()
//  	{
//  		CALLSTACKITEM_N(_CL("CReadList"), _CL("WriteMarksToDbL"));
//  		for (TInt i=0; i < iUnmarked.Count(); i++)
//  			{
// 				TInt id = iUnmarked[i];
// 				TInt ix = iFeedModel.GetIndex( id );
// 				if ( ix >= 0 )
// 					{
// 						iFeedModel.MarkAsRead( ix );
// 						iMarked.InsertInOrder( id );
// 					}
//  			}
//  		iUnmarked.Reset();
//  	}

 	void SyncWriteAllMarksToDbL()
 	{
 		CALLSTACKITEM_N(_CL("CReadList"), _CL("SyncWriteAllMarksToDbL"));
 		for (TInt i=0; i < iUnmarked.Count(); i++)
 			{
				TInt id = iUnmarked[i];
				TInt ix = iFeedModel.GetIndex( id );
				if ( ix >= 0 )
					{
						iFeedModel.MarkAsRead( ix );
						iMarked.InsertInOrder( id );
					}
 			}
 		iUnmarked.Reset();
 	}

private: // from CCheckedActive
	void CheckedRunL()
	{
		CALLSTACKITEM_N(_CL("CReadMarker"), _CL("CheckedRunL"));
		if ( iUnmarked.Count() )
			{
				TInt tobemarked = 0; // FIFO (better for user experience?)
				TInt id = iUnmarked[ tobemarked ];
				TInt ix = iFeedModel.GetIndex( id );
				if ( ix >= 0 )
					{
						iFeedModel.MarkAsRead( ix );
						iUnmarked.Remove( tobemarked );
						iMarked.InsertInOrder( id );
					}
			}

		if ( iUnmarked.Count() )
			{
				CompleteSelf();
			}
	}
	
	void CompleteSelf()
	{
		CALLSTACKITEM_N(_CL("CReadMarker"), _CL("CompleteSelf"));
		TRequestStatus* pStat = &iStatus;
		User::RequestComplete(pStat, KErrNone);
		SetActive();
	}


	void DoCancel()
	{
	}

private: // MTimeOut
	virtual void expired(CBase* aSource)
	{
		CALLSTACKITEM_N(_CL("CReadMarker"), _CL("expired"));
 		if ( aSource == iTimeOut )
			{
				if ( ! IsActive() && iUnmarked.Count() )
					CompleteSelf();
			}
	}
	
private: // own 
	CReadMarker(MFeedModel& aFeedModel, CRotatingList& aList) : 
		CCheckedActive(EPriorityIdle, _L("ReadMarker")), iFeedModel(aFeedModel), iList(aList) 
	{
		CActiveScheduler::Add(this);
	}

	
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CReadMarker"), _CL("ConstructL"));

		iTimeOut = CTimeOut::NewL(*this);

	}
	
 	void MarkIdL(TInt aId)
	{
		CALLSTACKITEM_N(_CL("CReadMarker"), _CL("MarkIdL"));

		if ( iMarked.FindInOrder( aId ) == KErrNotFound )
			{
				TInt err = iUnmarked.InsertInOrder( aId );
				if ( err != KErrNone && err != KErrAlreadyExists )
					User::Leave(err);
			}
	}

private:
	RArray<TInt> iUnmarked;
	RArray<TInt> iMarked;
	
	CTimeOut* iTimeOut;
	
	MFeedModel& iFeedModel;
	CRotatingList& iList;
};


/** 
 * Responsibility of this is event handling. No drawing here.
 */

class CFeedListImpl : 
	public CFeedList, 
	public MContextBase, 
	public MFeedModel::MObserver, 
	public CRotatingList::MControlModel,
	public CFeedControl::MMinimumSizeObserver,
	public CSoftScrollList::MObserver
// 	, public MEikScrollBarObserver
{
public:
	CFeedListImpl(CAknView& aView, 
				  MFeedModel& aFeedModel,
				  MControlFactory& aFactory,
				  CActiveState& aActiveState,
				  CFeedList::MEventObserver* aObserver,
				  TInt aFlags)
		: iFeedModel( aFeedModel ),
		  iActiveState( aActiveState ),
		  iView( aView ),
		  iFactory( aFactory ),
		  iEventObserver( aObserver ),
		  iFirstDraw( ETrue ),
		  iFlags( aFlags )
	{
		if ( iEventObserver )
			iOwnsEventObserver = EFalse;
	}
	
	void ConstructL(CCoeControl* aParent)
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("ConstructL"));
		SetContainerWindowL( *aParent );
		
		if ( ! iEventObserver )
			{
				iEventObserver = new (ELeave) CFeedList::COpenItemOnClick(iActiveState, iView);
				iOwnsEventObserver = ETrue;
			}
		
		InitListBoxL();
		
// 		ConstructScrollBarL();
		if ( ! (iFlags & CFeedList::ENoReadMarking) )
			iReadMarker = CReadMarker::NewL(iFeedModel, *iList);
		
		iFeedModel.AddObserverL( this );
	}
	

	~CFeedListImpl()
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("~CFeedListImpl"));
		iFeedModel.RemoveObserver( this );

		if (iReadMarker) iReadMarker->MarkVisibleItemsReadL();
		if (iList) iList->RemoveObserver( *this );
		delete iList;
		delete iReadMarker;

		if ( iOwnsEventObserver )
			delete iEventObserver;

// 		delete iSBFrame;
	}

// 	TAknDoubleSpanScrollBarModel iVModel; // model for double span type scrollbar
// 	CEikScrollBarFrame* iSBFrame;
// // 	TAknDoubleSpanScrollBarModel iDoubleSpanAttributes;
// 	TEikScrollBarModel iSBModel;

// 	void HandleScrollEventL( CEikScrollBar* aScrollBar, TEikScrollEvent aEventType )
// 	{
		
// 	}
 

// 	void ConstructScrollBarL()
// 	{
// 		iSBModel = TEikScrollBarModel(100, 0, 0);
// 		iSBFrame = new (ELeave) CEikScrollBarFrame(this, this, ETrue);
// 		iSBFrame->SetScrollBarVisibilityL(CEikScrollBarFrame::EOn, CEikScrollBarFrame::EAuto);
// 		//iSBFrame->SetScrollBarManagement(CEikScrollBar::EVertical, CEikScrollBarFrame::EFloating);
// 		iSBFrame->Tile(&iSBModel);
// 		iSBFrame->DrawScrollBarsNow();

// // 		iSBFrame = new ( ELeave ) CEikScrollBarFrame( this, NULL );
// // 		CAknAppUiBase* appUi = iAvkonAppUi;
	   

// // 		if( AknLayoutUtils::DefaultScrollBarType( appUi ) ==
// // 			CEikScrollBarFrame::EDoubleSpan )
// // 			{
// // 				// window-owning scrollbar, non-remote, vertical, non-horizontal
// // 				iSBFrame->CreateDoubleSpanScrollBarsL( ETrue, EFalse, ETrue, EFalse );
// // 				iSBFrame->SetTypeOfVScrollBar( CEikScrollBarFrame::EDoubleSpan );
// // 				// setting DoubleSpan attributes via public setter functions of
// // 				// TAknDoubleSpanScrollBarModel iDoubleSpanAttributes
// // 				iDoubleSpanAttributes.SetScrollSpan( 20 );
// // 				iDoubleSpanAttributes.SetFocusPosition( 3 );
// // 				iDoubleSpanAttributes.SetWindowSize( 4 );
// // 				iDoubleSpanAttributes.SetFieldSize( 2 );
// // 				iDoubleSpanAttributes.SetFieldPosition( 10 );
				
// // 				// updating model using specific vertical handle
// // 				iSBFrame->VerticalScrollBar()->SetModel( &iDoubleSpanAttributes );				
// // 			}
// // 		else
// // 			{
// // // 				iSBFrame->SetTypeOfVScrollBar( CEikScrollBarFrame::EArrowHead );
// // // 				// setting ArrowHead attributes via public members of 
// // // 				// TEikScrollBarModel iArrowAttributes
// // // 				iArrowAttributes.iThumbPosition = ActualElement();
// // // 				iArrowAttributes.iThumbSpan = 10;
// // // 				iArrowAttributes.iScrollSpan = ElementCount();

// // 			}
// 	}

	void InitControlModelL(CCoeControl* /*aParent*/)
	{
		// can create free controls here
	}
	
  	virtual void UpdateControlL( TInt aModelIndex, MJuikControl* aControl ) 
  	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("UpdateControlL"));
 		CFeedControl* ctrl = static_cast<CFeedControl*>( aControl->CoeControl() );
 		refcounted_ptr<CBBFeedItem> item( iFeedModel.GetItemL( aModelIndex ) );
 		ctrl->InitL( item.get() ); 	
	}
	

	MJuikControl* GetControlL( TInt aModelIndex, CCoeControl* aParent )
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("GetControlL"));
		TInt id = iFeedModel.GetId( aModelIndex );
 		refcounted_ptr<CBBFeedItem> item( iFeedModel.GetItemL( aModelIndex ) );
		auto_ptr<CFeedControl> ctrl( iFactory.GetControlL(aModelIndex, id, item.get(), aParent) );
		ctrl->SetMinimumSizeObserverL( this );
		return ctrl.release();
	}
	
	void FreeControlL( MJuikControl* aControl )
	{		
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("FreeControlL"));
		CFeedControl* c = static_cast<CFeedControl*>( aControl->CoeControl() );
		c->SetMinimumSizeObserverL( NULL );
		iFactory.FreeControlL( c );
	}
		
	virtual TInt Count() const
	{
		return iFeedModel.Count();
	}

	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{	
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("OfferKeyEventL"));
// 		TBuf<100> tmpBuf;
// 		tmpBuf.Format( _L("FeedList: type %d, code %d"), aType, aKeyEvent.iCode );
// 		Reporting().DebugLog( tmpBuf );
		if (!iList) return EKeyWasNotConsumed;				

		if ( aType == EEventKey )
			{
				if ( ( aKeyEvent.iCode == JOY_UP || aKeyEvent.iCode == JOY_DOWN ) )
					{
						TKeyResponse response = iList->OfferKeyEventL(aKeyEvent, aType);
						StoreItemIdL();
						if ( iReadMarker ) iReadMarker->MarkVisibleItemsReadL();
						return response;
					}

				if (aKeyEvent.iCode==KEY_CALL) 
					{
						if ( iActiveState.ActiveContact().GetId() != KNullContactId  )
							{
								iView.HandleCommandL( EContextContactsCmdCall );
								return EKeyWasConsumed;
							}
					}
			}
		
		return iList->OfferKeyEventL(aKeyEvent, aType);
	}
	
	CFeedControl* GetFeedControlL(TInt aIndex) const 
	{
		MJuikControl* c = iList->GetUsedControlL( aIndex );
		if ( c )	
			{
				CFeedControl* fc = static_cast<CFeedControl*>( c->CoeControl() );
				return fc;
			}
		return NULL;
	}
			

	TKeyResponse ItemClickedL()
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("ItemClickedL"));
		TInt ix = iList->CurrentItemIndex();
		CFeedControl* fc = GetFeedControlL( ix );
		if ( fc )
			{
				if ( iEventObserver->ItemClickedL(ix, fc) )
					return EKeyWasConsumed; //response;
			}
		return EKeyWasNotConsumed;
	}

	
	virtual void HandleListEventL(CSoftScrollList::TEvent aEvent, TInt /*aIndex*/) 
	{
		switch ( aEvent )
			{
			case CSoftScrollList::EItemClicked:
				ItemClickedL();
				break;
			}
	}


	void StoreItemIdL()
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("StoreItemIdL"));
		if ( iList )
			{
				TInt ix = iList->CurrentItemIndex();
				if (ix >= 0)
					{
						// SPECIFIER: if [0,extra top item count] StoreItemIdL(); 
						//            else ix = ix - extra top item cou
						TUint id = iFeedModel.GetId( ix );
						iActiveState.ActiveItem().SetL(id);
					}
			} 
	}
	
	
	TInt CountComponentControls() const { return 1; }

	CCoeControl* ComponentControl(TInt aIndex) const { 
		if ( aIndex == 0 )
			return iList; 
		return NULL;
	} 
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("SizeChanged"));
		TRect rect = Rect();
		iList->SetRect( rect );
	}
	
	
	virtual void ItemChanged(TInt aIndex, MFeedNotify::TEvent aEvent )
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("ItemChanged"));
		
		if ( ! iList )
			return;
		
		switch ( aEvent )
			{
			case MFeedNotify::EAdded:
				{
					TInt oldIx = iList->CurrentItemIndex();
					iList->HandleItemAddedL( aIndex );
// 					if ( oldIx == 0)
// 						{
// 							iList->SetFocusL(0);
// 							StoreItemIdL();
// 						}
				}
				break;
			case MFeedNotify::EFeedItemDeleted:					
				{
				}
				break;
			default:
				// do nothing
				break;
			}
	}
	
	
	void InitListBoxL()
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("InitListBoxL"));
		iList = CRotatingList::NewL( this, *this );
		iList->AddObserverL( *this );
	}

	void SetCurrentItemToIdL(TInt aId)
	{
		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("SetCurrentItemToIdL"));		
		TInt ix = iFeedModel.GetIndex( aId );
		iActiveState.ActiveItem().SetL( aId );
		StoreItemIdL();
		iList->SetFocusAndInitL( ix );
	}
	
	
// 	void SetCurrentItemL(TInt aIx, TInt aId=KErrNotFound)
// 	{
// 		CALLSTACKITEM_N(_CL("CFeedListImpl"), _CL("SetCurrentItemL"));		
// 		TInt id = aId;
// 		if ( id == KErrNotFound )
// 			id = iFeedModel.GetId( aIx ); 
// 		iActiveState.ActiveItem().SetL( id );
// 		StoreItemIdL();
// 		iList->SetFocusAndInitL( aIx );
// 	}
	
	void Draw(const TRect& /*aRect*/)  const
	{
		if ( iFirstDraw && iReadMarker )
			{
				iReadMarker->MarkVisibleItemsReadL();
				iFirstDraw = EFalse;
			}
	}

	
	virtual void MinimumSizeChangedL(CFeedControl* /*aSource*/, TSize /*aNewSize*/)
	{
		SizeChanged();
		DrawDeferred();
	}
	
private:
	MFeedModel& iFeedModel;
	MControlFactory& iFactory;

	CRotatingList* iList;
	
	CActiveState& iActiveState;
	CAknView& iView;

	MEventObserver* iEventObserver;
	TBool iOwnsEventObserver;

	CReadMarker* iReadMarker;

	mutable TBool iFirstDraw;

	TInt iFlags;
};


EXPORT_C TBool CFeedList::COpenItemOnClick::ItemClickedL(TInt /*aIndex*/, CFeedControl* aControl)
{
	if ( aControl == NULL )
		return EFalse;
	
	TInt id = aControl->Id();
	if ( id >= 0 )
		{
			iActiveState.ActiveItem().SetL(id);
			
			refcounted_ptr<CBBFeedItem> item( iActiveState.ActiveItem().GetL() );
			if ( !item.get() ) 
				return EFalse;

			if ( item->iGroupChildCount > 0 )
				{
					iView.HandleCommandL( EContextContactsCmdFeedGroupView );
				}
			else
				{
					iView.HandleCommandL( EContextContactsCmdCommentsView );
				}
			return ETrue;
		}
	return EFalse;
}



EXPORT_C CFeedList* CFeedList::NewL(CAknView& aView, 
									CCoeControl* aParent, 
									MFeedModel& aFeedModel,
									MControlFactory& aFactory,
									CActiveState& aActiveState,
									CFeedList::MEventObserver* aObserver,
									TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CFeedList"), _CL("NewL"));
	auto_ptr<CFeedListImpl> self( new (ELeave) CFeedListImpl(aView, aFeedModel, aFactory, aActiveState, aObserver, aFlags) );
	self->ConstructL(aParent);
	return self.release();
}
