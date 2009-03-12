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

#include "ccu_streamfactories.h"

#include "ccu_feedcontrols.h"
#include "ccu_authorheadercontrol.h"
#include "ccu_feeditem.h"
#include "ccu_feedmodel.h"
#include "jsui_controlpool.h"


#include "app_context.h"

#include "contextvariant.hrh"

class CStreamFactory : public CBase, public CFeedList::MControlFactory, public MContextBase
{
public:
	CStreamFactory(TUiDelegates aDelegates, TInt aFlags) : iDelegates(aDelegates), iControlFlags(aFlags)
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("CStreamFactory"));		
	}

	void ConstructL(const TDesC& aNick)
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("ConstructL"));		
		iNick = aNick.AllocL();
	}

	virtual ~CStreamFactory()
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("ConstructL"));		
		iPostPool.ResetAndDestroy();
		iCommentPool.ResetAndDestroy();
		iRssItemPool.ResetAndDestroy();
		delete iNick;
	}
	

	TInt ControlFlags() 
	{
		return iControlFlags; 
	}


	CFeedCommentControl* GetFeedCommentL( CCoeControl& aParent )
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("GetFeedCommentL"));		

		if ( iCommentPool.Count() == 0 )
			{
				auto_ptr<CFeedCommentControl> c( CFeedCommentControl::NewL(iDelegates, ControlFlags()) );
				c->SetContainerWindowL( aParent );
				iCommentPool.PushL( c.get() );
				c.release();
			}
		auto_ptr<CFeedCommentControl> ctrl( iCommentPool.PopL() );
		return ctrl.release();
	}

#ifdef __JAIKU_PHOTO_DOWNLOAD__
	CMediaPostControl* GetMediaPostL( CCoeControl& aParent )
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("GetMediaPostL"));		
 		if ( iMediaPostPool.Count() == 0 )
 			{
				auto_ptr<CMediaPostControl> c( CMediaPostControl::NewL(aParent, iDelegates, ControlFlags()) );
				//c->SetContainerWindowL( aParent );
				iMediaPostPool.PushL( c.get() );
				c.release();
 			}		
 		auto_ptr<CMediaPostControl> ctrl( iMediaPostPool.PopL() );
		return ctrl.release();
	}
#endif // __JAIKU_PHOTO_DOWNLOAD__
	
	CPostControl* GetJaikuPostL( CCoeControl& aParent )
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("GetJaikuPostL"));		
		if ( iPostPool.Count() == 0 )
			{
				auto_ptr<CPostControl> c( CPostControl::NewL(iDelegates, ControlFlags()) );
				c->SetContainerWindowL( aParent );
				iPostPool.PushL( c.get() );
				c.release();
			}		
		auto_ptr<CPostControl> ctrl( iPostPool.PopL() );
		return ctrl.release();
	}

	
	CRssItemControl* GetFeedItemL( CCoeControl& aParent, TInt aExtraFlags = 0 )
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("GetFeedItemL"));		
		if ( iRssItemPool.Count() == 0 )
			{
				TInt flags = ControlFlags() | aExtraFlags;
				auto_ptr<CRssItemControl> c( CRssItemControl::NewL(iDelegates, flags) );
				c->SetContainerWindowL( aParent );
				iRssItemPool.PushL( c.get() );
				c.release();
			}		
		auto_ptr<CRssItemControl> ctrl( iRssItemPool.PopL() );
		return ctrl.release();
	}


	virtual CAuthorHeaderControl* CreateAuthorHeaderL(CCoeControl* aParent)
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("CreateAuthorHeaderL"));		
		TInt flags = ControlFlags() | FeedUi::EShowBuddyIcon | FeedUi::EDrawSeparator;
 		auto_ptr<CAuthorHeaderControl> c( CAuthorHeaderControl::NewL(*iNick, iDelegates, flags) );
 		c->SetContainerWindowL( *aParent );
 		return c.release();
	}
	
 	virtual CButtonControl* CreatePostJaikuButtonL(CCoeControl* aParent) 
 	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("CreatePostJaikuButtonL"));		
		_LIT(KLabel, "Post a Jaiku");
 		auto_ptr<CButtonControl> c( CButtonControl::NewL(KPostJaikuItem, KLabel, 
														 iDelegates, ControlFlags()) );
 		c->SetContainerWindowL( *aParent );
 		return c.release();
 	}


 	virtual CButtonControl* CreatePostCommentButtonL(CCoeControl* aParent) 
 	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("CreatePostCommentButtonL"));		
		_LIT(KAddComment, "Add Comment");
 		auto_ptr<CButtonControl> c( CButtonControl::NewL(KPostCommentItem, KAddComment, 
														 iDelegates, ControlFlags()) );
 		c->SetContainerWindowL( *aParent );
 		return c.release();
 	}


 	virtual CIndividualPostControl* CreateIndividualPostL(CCoeControl* aParent) 
 	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("CreateIndividualPostL"));		
 		auto_ptr<CIndividualPostControl> c( CIndividualPostControl::NewL(iDelegates, ControlFlags() | FeedUi::EDrawSeparator  ) );
 		c->SetContainerWindowL( *aParent );
 		return c.release();
 	}



 	virtual CMissingPost* CreateMissingPostL(CCoeControl* aParent) 
 	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("CreateMissingPostL"));		
 		auto_ptr<CMissingPost> c( CMissingPost::NewL(iDelegates, ControlFlags() | FeedUi::EDrawSeparator) );
 		c->SetContainerWindowL( *aParent );
 		return c.release();
 	}

  	
  	CFeedControl* GetControlL( TInt /*aModelIndex*/, TInt aId, CBBFeedItem* aItem, CCoeControl* aParent )
	{		
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("GetControlL"));		
		switch ( aId )
			{
			case KAuthorHeaderItem:
				{
					auto_ptr<CAuthorHeaderControl> c( CreateAuthorHeaderL( aParent ) );
					c->UpdateL();
					return c.release();
				}
			case KPostJaikuItem:
				{
					auto_ptr<CButtonControl> c( CreatePostJaikuButtonL( aParent ) );
					c->UpdateL();
					return c.release();
				}
			case KPostCommentItem:
				{
					auto_ptr<CButtonControl> c( CreatePostCommentButtonL( aParent ) );
					c->UpdateL();
					return c.release();
				}
			case KMissingPostItem:
				{
					if ( ! aItem ) 
						{
							// Missing posts item should be done only if there is no dummy parent
							auto_ptr<CMissingPost> c(  CreateMissingPostL( aParent ) );
							//c->UpdateL();
							return c.release();
						}
				}
			}

		if ( ! aItem ) 
			Bug( _L("Trying to create stream control for NULL item") ).Raise();
				
		if ( FeedItem::IsComment( *aItem ) )
			{
				auto_ptr<CFeedCommentControl> c( GetFeedCommentL(*aParent) );
				c->InitL( aItem );
				return c.release();
			}
		else if ( FeedItem::IsJaiku( *aItem ) )
			{				
 				auto_ptr<CPostControl> c( GetJaikuPostL(*aParent) );
 				c->InitL( aItem );
				return c.release();
			}
		else 
#ifdef __JAIKU_PHOTO_DOWNLOAD__
			if ( aItem->iThumbnailUrl().Length() > 0 )
				{
					auto_ptr< CMediaPostControl > c( GetMediaPostL(*aParent) );
					c->InitL( aItem );
					return c.release();
				}
			else
#endif // __JAIKU_PHOTO_DOWNLOAD__
				{
					auto_ptr<CRssItemControl> c( GetFeedItemL(*aParent) );
					c->InitL( aItem );
					return c.release();
				}
		return NULL;
	}
	
    // Let's keep things simple and not rotate controls 
	// #define ROTATE_CONTROLS 1

	void FreeControlL( CFeedControl* aControl )
	{
		CALLSTACKITEM_N(_CL("CStreamFactory"), _CL("FreeControlL"));		
		auto_ptr<CFeedControl> c( static_cast<CFeedControl*>( aControl ) );
		c->ResetL();
		c->SetSize( TSize(0,0) );
		if ( c->TypeName() == KPostInStreamControlType )
			{
#ifdef ROTATE_CONTROLS
				CPostControl* p = static_cast<CPostControl*>( aControl );
				iPostPool.PushL( p );
				c.release();
#endif 
			}
		else if ( c->TypeName() == KCommentInStreamControlType )
			{
#ifdef ROTATE_CONTROLS
				CFeedCommentControl* p = static_cast<CFeedCommentControl*>( aControl );
				iCommentPool.PushL( p );
				c.release();
#endif 
			}
		else if ( c->TypeName() == KRssItemInStreamControlType )
			{
#ifdef ROTATE_CONTROLS
				CRssItemControl* p = static_cast<CRssItemControl*>( aControl );
				iRssItemPool.PushL( p );
				c.release();
#endif
			}
		else
			{
				// do nothing -> delete
			}
	}
	
#ifdef __JAIKU_PHOTO_DOWNLOAD__
	RControlPool<CMediaPostControl> iMediaPostPool;
#endif 
	RControlPool<CPostControl> iPostPool;
	RControlPool<CFeedCommentControl> iCommentPool;
	RControlPool<CRssItemControl> iRssItemPool;

	TUiDelegates iDelegates;
	
	TInt iControlFlags;

	HBufC* iNick;
};




class CRssGroupStreamFactory : public CStreamFactory
{
public:
	CRssGroupStreamFactory(TUiDelegates aDelegates, TInt aFlags) : CStreamFactory(aDelegates, aFlags)
	{
	}


	
  	CFeedControl* GetControlL( TInt aModelIndex, TInt aId, CBBFeedItem* aItem, CCoeControl* aParent )
	{		
		CALLSTACKITEM_N(_CL("CRssGroupStreamFactory"), _CL("GetControlL"));		
		if ( aItem && aModelIndex == 0 )
			{
#ifdef __JAIKU_PHOTO_DOWNLOAD__
				if ( aItem->iThumbnailUrl().Length() > 0 )
					{
						auto_ptr<CMediaPostControl> c( GetMediaPostL(*aParent ) );
						c->InitL( aItem );
						return c.release();
					}
				else
#endif // __JAIKU_PHOTO_DOWNLOAD__
					{
						TInt extraFlags = FeedUi::EDrawSeparator | FeedUi::EShowBuddyIcon; 
						auto_ptr<CRssItemControl> c( GetFeedItemL(*aParent, extraFlags) );
						c->InitL( aItem );
						return c.release();
					}
			}
		
		return CStreamFactory::GetControlL( aModelIndex, aId, aItem, aParent );
	}
};



class CCommentStreamFactory : public CStreamFactory
{
public:
	CCommentStreamFactory(TUiDelegates aDelegates, TInt aFlags) : CStreamFactory(aDelegates, aFlags)
	{
	}


	
  	CFeedControl* GetControlL( TInt aModelIndex, TInt aId, CBBFeedItem* aItem, CCoeControl* aParent )
	{		
		CALLSTACKITEM_N(_CL("CCommentStreamFactory"), _CL("GetControlL"));		
		if ( aItem && aModelIndex == 0 )
			{
#ifdef __JAIKU_PHOTO_DOWNLOAD__
				if ( aItem->iThumbnailUrl().Length() > 0 )
					{
						auto_ptr<CMediaPostControl> c( GetMediaPostL(*aParent ) );
						c->InitL( aItem );
						return c.release();
					}
				else
#endif // __JAIKU_PHOTO_DOWNLOAD__
					if ( FeedItem::IsJaiku( *aItem ) )
						{
							auto_ptr<CIndividualPostControl> c( CreateIndividualPostL( aParent ) );
							c->InitL(aItem);
							return c.release();	
						}		
					else 
						{
							TInt extraFlags = FeedUi::EDrawSeparator | FeedUi::EShowBuddyIcon; 
							auto_ptr<CRssItemControl> c( GetFeedItemL(*aParent, extraFlags) );
							c->InitL( aItem );
							return c.release();
						}
			}
		
		return CStreamFactory::GetControlL( aModelIndex, aId, aItem, aParent );
	}
};

EXPORT_C CFeedList::MControlFactory* StreamFactories::EverybodysStreamL(TUiDelegates aDelegates, TInt aFlags)
{
	auto_ptr<CStreamFactory> self( new (ELeave) CStreamFactory(aDelegates, aFlags) );
	self->ConstructL( KNullDesC );
	return self.release();
}

EXPORT_C CFeedList::MControlFactory* StreamFactories::CommentStreamL(TUiDelegates aDelegates, TInt aFlags)
{
	auto_ptr<CCommentStreamFactory> self( new (ELeave) CCommentStreamFactory(aDelegates, aFlags) );
	self->ConstructL( KNullDesC );
	return self.release();
}


EXPORT_C CFeedList::MControlFactory* StreamFactories::AuthorStreamL(const TDesC& aNick, TUiDelegates aDelegates, TInt aFlags)
{
	auto_ptr<CStreamFactory> self( new (ELeave) CStreamFactory(aDelegates, aFlags) );
	self->ConstructL(aNick);
	return self.release();
}

EXPORT_C CFeedList::MControlFactory* StreamFactories::RssGroupStreamL(TUiDelegates aDelegates, TInt aFlags)
{
	auto_ptr<CRssGroupStreamFactory> self( new (ELeave) CRssGroupStreamFactory(aDelegates, aFlags) );
	self->ConstructL( KNullDesC );
	return self.release();
}
