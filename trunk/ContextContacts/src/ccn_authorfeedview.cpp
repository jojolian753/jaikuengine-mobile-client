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

#include "ccn_authorfeedview.h"

#include "ccn_commentsview.h"
#include <contextcontacts.rsg>
#include "contextcontacts.hrh"

#include "app_context_impl.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"
#include "ccu_activestate.h"

#include "ccu_feedlist.h"
#include "ccu_streamfactories.h"

#include "ccu_contactview_base.h"
#include "ccu_feedmodel.h"
#include "ccu_mainbgcontainer.h"
#include "ccu_posterui.h"
#include "cu_common.h"
#include "jabberdata.h"
#include "juik_keycodes.h"
#include "timeout.h"


#include <akndef.h>
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknviewappui.h>



class CAuthorFeedViewImpl :
	public CAuthorFeedView,
	public MContactViewBase,
	public MTimeOut
	
{
public:
	CAuthorFeedViewImpl(CCommentsView& aCommentsView) : iCommentsView( aCommentsView ), MContactViewBase(*static_cast<CJaikuViewBase*>(this))
	{
	}
	

	~CAuthorFeedViewImpl() 
	{
		CC_TRAPD(err, ReleaseViewImpl());
		if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
	
	void ReleaseViewImpl()
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("ReleaseViewImpl"));
		RemoveContainerL();
	}


	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("ConstructL"));
		BaseConstructL( R_FEEDBYAUTHOR_VIEW );
	}

	TUid Id() const {
		return KAuthorFeedView;
	}


	void RealDoActivateL(const TVwsViewId& aPrevViewId,TUid /*aCustomMessageId*/,const TDesC8& /*aCustomMessage*/)
	{		
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("RealDoActivateL"));
		iCommentsView.SetParentViewL( ViewId() );
 		UpdateStatusPaneL();
		CreateContainerL();
	}
			
	void RealDoDeactivateL()
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("RealDoDeactivateL"));
		RemoveContainerL();
		MarkAllAsReadL();
	}
	

	void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("DynInitMenuPaneL"));

		TBool failedToConstruct = ! iList;
		CommonMenus().DynInitMenuPaneL(aResourceId, aMenuPane, failedToConstruct);
		if ( failedToConstruct )
			return;
		
		TBool isUser = JabberData().IsUserNickL( iNick );
		SetItemDimmedIfExists(aMenuPane, EContextContactsCmdSetUserDesc, !isUser );
		SetItemDimmedIfExists(aMenuPane, EContextContactsCmdAppNameLocation, !isUser );
		
		refcounted_ptr<CBBFeedItem> item( ActiveState().ActiveItem().GetL() );
		TBool itemFocused = item.get() != NULL;
		SetItemDimmedIfExists(aMenuPane, EContextContactsCmdPostComment, !itemFocused );
	}


	void HandleCommandL(TInt aCommand)
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("HandleCommandL"));
		switch ( aCommand )
			{
			case EAknSoftkeyBack:
				ActivateParentViewL();
				break;
			case EContextContactsCmdSetUserDesc:
				{
					PosterUi().PostJaikuL( this );
					TBool isUserNick = JabberData().IsUserNickL( iNick );
					if ( isUserNick && iList )
						iList->SetCurrentItemToIdL(KPostJaikuItem);				
					else
						{
							// FIXME: Show "Jaiku posted" note?
						}
					break;
				}
			case EContextContactsCmdMarkAllAsRead:
				{
					if ( iNick.Length() > 0 )
						FeedStorage().MarkAsRead( iNick );
				}
				break;
			case EContextContactsCmdPostComment:
				{
					refcounted_ptr<CBBFeedItem> item( ActiveState().ActiveItem().GetL() );
					if (item.get())
						{
							PosterUi().PostCommentL( *item, this );
						}
					else 
						User::Leave(KErrNotFound);
				}
				break;
			default:
				AppUi()->HandleCommandL( aCommand );
				break;
			}
	}


	class CListEventHandler : public CFeedList::COpenItemOnClick, public MContextBase
	{
	public:
		CListEventHandler(class CActiveState& aActiveState,class CAknView& aView) : 
			CFeedList::COpenItemOnClick(aActiveState, aView) {}
		
		virtual TBool ItemClickedL(TInt aIndex, CFeedControl* aControl)
		{
			CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl::CListEventHandler"), _CL("ItemClickedL"));		
			if (aControl && aControl->Id() == KAuthorHeaderItem)
				{
					iView.HandleCommandL( EContextContactsCmdDisplayRichPresence );
					return ETrue;
				}
			if (aControl && aControl->Id() == KPostJaikuItem)
				{
					iView.HandleCommandL( EContextContactsCmdSetUserDesc );
					return ETrue;
				}
			
			return CFeedList::COpenItemOnClick::ItemClickedL( aIndex, aControl );
		}
	};


	void CreateContainerL()
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("CreateContainerL"));
		RemoveContainerL();
		iContainer = CMainBgContainer::NewL( this, ClientRect(), ThemeColors(), ProgressBarModel() );
		SetBaseContainer( iContainer );

		ActiveState().ActiveContact().GetNickL( iNick );
 		CJabberData::TransformToUiNickL( iNick );
// 		FeedStorage().MarkAsRead( iNick );

		if ( ! iFeedModel )
			{
				TBool isUserNick = JabberData().IsUserNickL( iNick );
				iFeedModel = CreateAuthorFeedModelL(iNick, FeedStorage(), isUserNick);
			}
		if ( ! iControlFactory )
			iControlFactory = StreamFactories::AuthorStreamL( iNick, UiDelegates(), FeedUi::EHideAuthorName | FeedUi::EShowParentInfo );
		if ( ! iListEventHandler)
			iListEventHandler = new (ELeave) CListEventHandler( ActiveState(), *this );

		iList = CFeedList::NewL( *this, iContainer, *iFeedModel, *iControlFactory, ActiveState(), iListEventHandler, CFeedList::ENoReadMarking);
		TUint id = ActiveState().ActiveItem().GetId();
		if ( id != KNullItemId )
			iList->SetCurrentItemToIdL( id );
		iContainer->SetContentL( iList );

		iContainer->MakeVisible(ETrue);
		iContainer->ActivateL();

		AppUi()->AddToStackL( *this, iContainer );

		if ( ! iTimeout )
			iTimeout = CTimeOut::NewL(*this);
		iTimeout->WaitShort(400);
	}
	
	virtual void expired(CBase* aSource)
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("expired"));
 		if ( aSource == iTimeout )
			{
				MarkAllAsReadL();
			}
	}

	void MarkAllAsReadL()
	{		
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("MarkAllAsReadL"));
		TInt streamCount;
		TInt unreadCount;
		streamCount = unreadCount = 0;
		FeedStorage().GetCountsByAuthorL( iNick, streamCount, unreadCount );
		// mark unconditionally, since there's been bugs that leave
		// unread items
		//if ( unreadCount > 0 )
			FeedStorage().MarkAsRead( iNick );
	}
	
	void RemoveContainerL()
	{
		CALLSTACKITEM_N(_CL("CAuthorFeedViewImpl"), _CL("RemoveContainerL"));
		if ( iContainer )
			{
				AppUi()->RemoveFromStack( iContainer );
				delete iContainer;
				iContainer = NULL;
				SetBaseContainer( NULL );
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

		if ( iTimeout )
			{
			delete iTimeout;
			iTimeout = NULL;
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
	CCommentsView& iCommentsView;

	CMainBgContainer* iContainer;

	CFeedList* iList;

	MFeedModel* iFeedModel;
	CFeedList::MControlFactory* iControlFactory;
	CListEventHandler* iListEventHandler;
	
	CJabberData::TNick iNick;

	CTimeOut* iTimeout;
};


EXPORT_C CAuthorFeedView* CAuthorFeedView::NewL(CCommentsView& aCommentsView)
{
	CALLSTACKITEMSTATIC_N(_CL("CAuthorFeedView"), _CL("NewL"));
	auto_ptr<CAuthorFeedViewImpl> self(new (ELeave) CAuthorFeedViewImpl(aCommentsView));
	//self->ConstructL();
	return self.release();
}
