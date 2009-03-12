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

#include "ccn_commentsview.h"

#include <contextcontacts.rsg>

#include "app_context_impl.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"
#include "ccu_activestate.h"

#include "ccu_feedlist.h"
#include "ccu_feedmodel.h"
#include "ccu_streamfactories.h"


#include "ccu_feeditem.h"
#include "ccu_launchbrowser.h"
#include "ccu_mainbgcontainer.h"
#include "ccu_posterui.h"
#include "ccu_storage.h"
#include "cu_common.h"
#include "contextcontacts.hrh"
#include "emptytext.h"
#include "jabberdata.h"

#include <contextcontacts.rsg>

#include <akndef.h>
#include <aknviewappui.h>


class CCommentsViewImpl : public CCommentsView
{
public:
	CCommentsViewImpl(CPoster& aPoster) : iPoster(aPoster)
	{
	}
	
	
	~CCommentsViewImpl() 
	{
		CC_TRAPD(err, ReleaseViewImpl());
		if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
	
	void ReleaseViewImpl()
	{
		CALLSTACKITEM_N(_CL("CCommentsViewImpl"), _CL("ReleaseViewImpl"));
		RemoveContainerL();
	}

	
	
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CCommentsViewImpl"), _CL("ConstructL"));
		BaseConstructL( R_COMMENTS_VIEW );
	}
	
	TUid Id() const {
		return KCommentsView;
	}
	

	void RealDoActivateL(const TVwsViewId& aPrevViewId,TUid /*aCustomMessageId*/,const TDesC8& /*aCustomMessage*/)
	{
		CALLSTACKITEM_N(_CL("CCommentsViewImpl"), _CL("RealDoActivateL"));
				
		CreateContainerL();
	}
		
	
	void RealDoDeactivateL()
	{
		CALLSTACKITEM_N(_CL("CCommentsViewImpl"), _CL("RealDoDeactivateL"));
		RemoveContainerL();
	}
	

	void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
	{
		CALLSTACKITEM_N(_CL("CCommentsViewImpl"), _CL("DynInitMenuPaneL"));
		TBool failedToConstruct = ! iList;
		CommonMenus().DynInitMenuPaneL(aResourceId, aMenuPane, failedToConstruct);
		if ( failedToConstruct )
			return;
		
		refcounted_ptr<CBBFeedItem> item( NULL );
		if ( iRootItem.Length() > 0 )
			item.reset( FeedStorage().GetByGlobalIdL( iRootItem, ETrue) );
		
		TBool canDownload = item.get() && 
			item->iMediaDownloadState() == CBBFeedItem::ENotDownloading ||
			item->iMediaDownloadState() == CBBFeedItem::EQueued ||
			item->iMediaDownloadState() == CBBFeedItem::EDownloadErrorFailed ||
			item->iMediaDownloadState() == CBBFeedItem::EDownloadErrorRetrying;
		SetItemDimmedIfExists(aMenuPane, EContextContactsCmdDownloadImage, !canDownload );
	}


	void HandleCommandL(TInt aCommand)
	{
		CALLSTACKITEM_N(_CL("CCommentsViewImpl"), _CL("HandleCommandL"));
		switch ( aCommand )
			{
			case EAknSoftkeyBack:
				{
					// We want control to return to same item where we dived to this item
// 					refcounted_ptr<CBBFeedItem> item( NULL );
// 					if ( iRootItem.Length() > 0 )
// 						item.reset( FeedStorage().GetByGlobalIdL( iRootItem, ETrue) );
					
 					if ( iOpenedFromItem != KNullItemId )
 						{
 							ActiveState().ActiveItem().SetL( iOpenedFromItem );
 						}
 					else
 						{
 							ActiveState().ActiveItem().ClearL();
 						}
					ActivateParentViewL();
					break;
				}
			case EContextContactsCmdPostComment:
				{
					//refcounted_ptr<CBBFeedItem> item( FeedStorage().GetByGlobalIdL( iRootItem ) );
					PosterUi().PostCommentL( iRootItem, this );
					if ( iList )
						iList->SetCurrentItemToIdL(KPostCommentItem);				
				}
				break;
			case EContextContactsCmdOpenUrl:
				{
					OpenUrlL();
				}
				break;
			case EContextContactsCmdMarkAllAsRead:
				{
					if ( iRootItem.Length() > 0 )
						FeedStorage().MarkAsRead( iRootItem );
				}
				break;
			case EContextContactsCmdDownloadImage:
				{
					refcounted_ptr<CBBFeedItem> item( NULL );
 					if ( iRootItem.Length() > 0 )
 						item.reset( FeedStorage().GetByGlobalIdL( iRootItem, ETrue) );
					if ( item.get() && 
						 item->iMediaDownloadState() == CBBFeedItem::ENotDownloading ||
						 item->iMediaDownloadState() == CBBFeedItem::EQueued ||
						 item->iMediaDownloadState() == CBBFeedItem::EDownloadErrorFailed ||
						 item->iMediaDownloadState() == CBBFeedItem::EDownloadErrorRetrying )
						{
							FeedStorage().DownloadMediaForFeedItemL( item.get(), ETrue );
						}
				}
				break;
			default:
				AppUi()->HandleCommandL( aCommand );
				break;
			}
	}


	void OpenUrlL()
	{
		refcounted_ptr<CBBFeedItem> item( FeedStorage().GetByGlobalIdL( iRootItem ) );
		if ( item->iLinkedUrl().Length() )
			{
				LaunchBrowserL( item->iLinkedUrl() );
			}
		else if ( item->iStreamUrl().Length() )
			{
				LaunchBrowserL( item->iStreamUrl() );
			}
		else
			{
				LaunchBrowserL( _L("http://www.jaiku.com") );
			}
	}
	
	TBool OpenUrlFromItemL(CFeedControl&  aControl)
	{
		refcounted_ptr<CBBFeedItem> item( FeedStorage().GetByGlobalIdL( iRootItem ) );
		if ( item->iLocalDatabaseId == aControl.Id() )
			{
				OpenUrlL();
				return ETrue;
			}
		return ETrue;
	}


	TBool OpenAuthorViewFromItemL(CFeedControl&  aControl)
	{
		if ( iFeedModel )
			{
				TInt index = iFeedModel->GetIndex( aControl.Id() );
				refcounted_ptr<CBBFeedItem> item( iFeedModel->GetItemL( index ) );
				if ( item.get() )
					{
						CJabberData::TNick nick;
						nick.Copy( item->iAuthorNick() );
						CJabberData::CanonizeNickL( nick );
						ActiveState().ActiveContact().SetActiveNickL( nick ); 
						ActiveState().ActiveItem().ClearL();
						HandleCommandL( EContextContactsCmdPersonStream );
						return ETrue;
					}
			}
		return EFalse;
	}

	class CListEventHandler : public CFeedList::COpenItemOnClick
	{
	public:
		CListEventHandler(class CActiveState& aActiveState,CCommentsViewImpl& aView) : 
			CFeedList::COpenItemOnClick(aActiveState, aView), iCommentsView(aView) {}
		
		virtual TBool ItemClickedL(TInt aIndex, CFeedControl* aControl)
		{
			if (aControl && aControl->Id() == KPostCommentItem)
				{
					iView.HandleCommandL( EContextContactsCmdPostComment);
					return ETrue;
				}
			else if ( aControl )
				{
					return iCommentsView.OpenAuthorViewFromItemL( *aControl );
					//return iCommentsView.OpenUrlFromItemL( *aControl );
				}
			return ETrue; // eat all other clicks
			//return CFeedList::COpenItemOnClick::ItemClickedL( aIndex, aControl );
		}
		CCommentsViewImpl& iCommentsView;
	};


	void CreateContainerL()
	{
		CALLSTACKITEM_N(_CL("CCommentsViewImpl"), _CL("CreateContainerL"));
		RemoveContainerL();


		refcounted_ptr<CBBFeedItem> item( ActiveState().ActiveItem().GetL() );
		
		iOpenedFromItem = KNullItemId;
		if ( item.get() )
			iOpenedFromItem = item->iLocalDatabaseId;
		
		if ( item.get() && FeedItem::IsComment( *item ) )
			{
				iRootItem.Zero();
				iRootItem = item->iParentUuid();
				item.reset( FeedStorage().GetByGlobalIdL( item->iParentUuid(), ETrue ) );
			}
		if ( item.get() )
			iRootItem = item->iUuid();
		

		iContainer = CMainBgContainer::NewL( this, ClientRect(), ThemeColors(), ProgressBarModel() );

		if ( ! iFeedModel )
			iFeedModel = CreateCommentListModelL(iRootItem, FeedStorage(), ProgressBarModel());

		if ( ! iControlFactory )
			{
				TInt flags = FeedUi::EShowBuddyIcon 
					| FeedUi::EFocusInsideControl
					| FeedUi::EShowFullImage
					| FeedUi::EDrawFullFocus;
				iControlFactory = StreamFactories::CommentStreamL( UiDelegates(), flags );
			}
																   
		if ( ! iListEventHandler)
			iListEventHandler = new (ELeave) CListEventHandler( ActiveState(), *this );

		iList = CFeedList::NewL( *this, iContainer, *iFeedModel, *iControlFactory, ActiveState(), iListEventHandler);
		TUint id = ActiveState().ActiveItem().GetId();
		if ( id != KNullItemId )
			iList->SetCurrentItemToIdL( id );

 		iContainer->SetContentL( iList );

		iContainer->MakeVisible(ETrue);
		iContainer->ActivateL();

		AppUi()->AddToStackL( *this, iContainer );
	}

	void RemoveContainerL()
	{
		CALLSTACKITEM_N(_CL("CCommentsViewImpl"), _CL("RemoveContainerL"));
		if ( iContainer )
			{
				AppUi()->RemoveFromStack( iContainer );
				delete iContainer;
				iContainer = NULL;
			}
 		if ( iList )
 			{
 				delete iList;
 				iList = NULL;
 			}
		if ( iControlFactory)
			{

				delete iControlFactory;
				iControlFactory = NULL;
			}
		if ( iFeedModel )
			{
				delete iFeedModel;
				iFeedModel = NULL;
			}
		if ( iListEventHandler )
			{
				delete iListEventHandler;
				iListEventHandler = NULL;
			}
	}
	
	void HandleResourceChange( TInt aType )
	{
	if ( aType == KEikDynamicLayoutVariantSwitch )
		{
			if ( iContainer )
				{
					TRect r = ClientRect();
					iContainer->SetRect( r );
				}
		}
	}


	
private:
	CMainBgContainer* iContainer;


	CFeedList* iList;
	MFeedModel* iFeedModel;

	CPoster& iPoster;
	TGlobalId iRootItem;

	CFeedList::MControlFactory* iControlFactory;
	CListEventHandler* iListEventHandler;

	TUint iOpenedFromItem;

};


EXPORT_C CCommentsView* CCommentsView::NewL(CPoster& aPoster)
{
	CALLSTACKITEMSTATIC_N(_CL("CCommentsView"), _CL("NewL"));
 	auto_ptr<CCommentsViewImpl> self(new (ELeave) CCommentsViewImpl(aPoster));
	//self->ConstructL();
	return self.release();
}
