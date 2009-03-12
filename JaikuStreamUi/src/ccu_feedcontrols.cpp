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

#include "ccu_feedcontrols.h"

#include "jsui_photoholder.h"
#include "jsui_button.h"

#include "ccu_storage.h"
#include "ccu_activestate.h"
#include "ccu_feeditem.h"
#include "ccu_feedmodel.h"
#include "ccu_jaicons.h"
#include "ccu_launchbrowser.h"
#include "ccu_staticicons.h"
#include "ccu_streamrenderers.h"
#include "ccu_userpics.h"
#include "ccu_timeperiod.h"
#include "ccu_themes.h"



//#include "contextcontacts.hrh"
#include "juik_animation.h"
#include "juik_fonts.h"
#include "juik_icons.h"
#include "juik_image.h"
#include "juik_iconmanager.h"
#include "juik_keycodes.h"
#include "juik_label.h"
#include "juik_layoutitem.h"
#include "juik_layout.h"
#include "jaiku_layoutids.hrh"
#include "juik_photo.h"
#include "juik_sizer.h"
#include "juik_sizercontainer.h"
#include "juik_scrolllist.h"
#include "juik_multilabel.h"
#include "juik_urlmultilabel.h"


#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "csd_feeditem.h"
#include "cc_stringtools.h"
#include "raii_array.h"
#include "reporting.h"

#include <contextcontactsui.mbg>

#include <aknbiditextutils.h>
#include <akniconarray.h> 
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknsutils.h> 
#include <aknutils.h>
#include <aknview.h>
#include <eikimage.h> 
#include <eiklabel.h>
#include <gulcolor.h> 
#include <eikenv.h>

#include <gulicon.h>

#include "contextvariant.hrh"



enum TPostContentIds {
	EContentSizer = 0,
	EJaicon,
	EContentLabel, 	
	EContentLabel2, 
	EMetadataSizer,
	EMetadataLabel,
	ECommentIndicator,
	ENameLabel,
	ENameSizer,
	EBuddyIcon,
	ETrafficLight,
	ELocationLabel,
	ECalendarLabel,
	EPost,
	EMainSizer,
	ESeparator,
	EAuthorNameLabel,
	ELastUsedLabel
	};
	

static void SwapColors(TRgb& x, TRgb& y)
{
	// switch
	TRgb tmp = x;
	x = y;
	y = tmp;;
}



class CPostControlImpl : public CPostControl 
{
public:
	static CPostControlImpl* NewL(const TUiDelegates& aDelegates,
								  TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CPostControlImpl"), _CL("NewL"));
		auto_ptr<CPostControlImpl> self( new (ELeave) CPostControlImpl(aDelegates, aFlags) );
		self->ConstructL();
		return self.release();
	}

	CPostControlImpl(const TUiDelegates& aDelegates,TInt aFlags) : CPostControl(aDelegates, aFlags)
	{
	}

	virtual const TTypeName& TypeName() const 
	{
		return KPostInStreamControlType;
	}


	virtual CBubbledStreamItem::TBubbleType BubbleType() const
	{
		return CBubbledStreamItem::ENormal;
	}

	 
	void ConstructL() 
	{
		CALLSTACKITEM_N(_CL("CPostControlImpl"), _CL("ConstructL"));		

		BaseConstructL();

		BubbleContent().SetDebugId(20000);
		//iWrapper = CMultiLabelWrapper::NewL();
		// content
		{			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
				BubbleContent().SetRootSizerL( sizer );
			}
		
			{
				iJaicon = CreateJaiconL(BubbleContent());
				iPost = CMultiLabel::NewL( iJaicon, 
										   PrimaryTextColor(), PrimaryHighlightTextColor());
 				BubbleContent().AddControlL( iPost, EPost );
				BubbleContent().GetRootSizerL()->AddL( *iPost, 0, Juik::EExpand ); 
			}

			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
				BubbleContent().AddSizerL( sizer, EMetadataSizer );
				BubbleContent().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
			}
			
			
			{
				iMetadata = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
											  JuikFonts::Tiny(), BubbleContent() );
				BubbleContent().AddControlL( iMetadata, EMetadataLabel );
				BubbleContent().GetSizerL( EMetadataSizer )->AddL( *iMetadata, 1, Juik::EExpandNot ); 
				 
				iCommentIndicator = CCommentIndicator::NewL( iDelegates );
				BubbleContent().AddControlL( iCommentIndicator, ECommentIndicator );
				BubbleContent().GetSizerL( EMetadataSizer )->AddL( *iCommentIndicator, 0, Juik::EExpandNot | Juik::EAlignBottom);
			}
		}
	}
	 

	~CPostControlImpl()
	{
	}


	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{	
// 		TBuf<100> tmpBuf;
// 		tmpBuf.Format( _L("\t\t\tPost: type %d, code %d"), aType, aKeyEvent.iCode );
// 		Reporting().DebugLog( tmpBuf );
		
		if ( ! ( iFlags & FeedUi::EFocusInsideControl ) )
			{
				return EKeyWasNotConsumed;
			}
			
// 		if ( aType == EEventKey )
// 			{
// 				if ( aKeyEvent.iCode == JOY_DOWN )
// 					{
// 					}
// 			}
		return EKeyWasNotConsumed;
	}


	void UpdateMetadataL(CBBFeedItem& aItem) 
	{
		CALLSTACKITEM_N(_CL("CPostControlImpl"), _CL("UpdateMetadataL"));
		TTime now = GetTime();					
		TTime tstamp =  aItem.iCreated();

		TTimePeriod period = TTimePeriod::BetweenL( tstamp, now );
		iWorkBuf.Zero();
		if ( ! (iFlags & FeedUi::EHideAuthorName ) )
			{
				//iWorkBuf.Append( _L("by ") );
				iWorkBuf.Append( aItem.iAuthorNick() );
				iWorkBuf.Append( _L(" ") );
			}
		PeriodFormatter().AgoTextL(period, iWorkBuf);
		if ( aItem.iLocation().Length() > 0 )
			{
				iWorkBuf.Append( _L(" in ") );
				iWorkBuf.Append( aItem.iLocation()  );
			}
		 
		iMetadata->UpdateTextL( iWorkBuf );
	}

	void UpdateContentL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CPostControlImpl"), _CL("UpdateContentL"));
		iPost->UpdateTextL( aItem.iContent() );
		if ( aItem.iFromServer() )
			{
				iPost->SetColorsL( iServerColor, PrimaryHighlightTextColor() );
			}
		else 
			{
				iPost->SetColorsL( iLocalColor, SecondaryHighlightTextColor() );
			}
	}

	void ZeroL()
	{
		CALLSTACKITEM_N(_CL("CPostControlImpl"), _CL("ZeroL"));

		ZeroBaseL();
		iMetadata->ZeroL();
		iJaicon->ClearL();
		iPost->ZeroL();
		iCommentIndicator->ZeroL();
	}


	void UpdateL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CPostControlImpl"), _CL("UpdateL"));

		//ZeroL(); // this is plain wrong, because these call CJuikLabel::ZeroL which setsize to 0,0.
#ifdef __DEV__
		{
			TInt id = aItem.iLocalDatabaseId;
			BubbleContent().SetDebugId( 20000 + id );
			iPost->SetDebugId( 30000 + id );
			iMetadata->SetDebugId( 40000 + id );
		}
#endif

		UpdateBaseL( aItem );
		UpdateMetadataL( aItem );
		UpdateJaiconL( *iJaicon, aItem );
		UpdateContentL( aItem );
		iCommentIndicator->UpdateL( aItem );
	}
	
	 
	
private:	
	CJuikLabel* iMetadata;
	CJuikImage* iJaicon;
	CMultiLabel* iPost;
	CCommentIndicator* iCommentIndicator;
};


CPostControl* CPostControl::NewL(const TUiDelegates& aDelegates,
								 TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CPostControl"), _CL("NewL"));
	return CPostControlImpl::NewL( aDelegates, aFlags );
}


class CFeedCommentControlImpl : public CFeedCommentControl 
{
public:
	static CFeedCommentControlImpl* NewL(const TUiDelegates& aDelegates,
										 TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CFeedCommentControlImpl"), _CL("NewL"));
		auto_ptr<CFeedCommentControlImpl> self( new (ELeave) CFeedCommentControlImpl(aDelegates, aFlags) );
		self->ConstructL();
		return self.release();
	}

	
	CFeedCommentControlImpl(const TUiDelegates& aDelegates, TInt aFlags) : CFeedCommentControl(aDelegates, aFlags)
	{
	}


	virtual const TTypeName& TypeName() const 
	{
		return KCommentInStreamControlType;
	}


	virtual CBubbledStreamItem::TBubbleType BubbleType() const
	{
		return CBubbledStreamItem::EComment;
	}


	void ConstructL() 
	{
		CALLSTACKITEM_N(_CL("CFeedCommentControlImpl"), _CL("ConstructL"));		
		
		BaseConstructL();
		// content
		
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			BubbleContent().SetRootSizerL( sizer );
		}
		
		 
		TRgb focusTextColor = PrimaryHighlightTextColor();
		TRgb focusBgColor =  BubbleFocusColor();
// 		if ( iFlags & FeedUi::EDrawFullFocus )
// 			SwapColors( focusTextColor, focusBgColor );
		
		TBool parseurls = iFlags & FeedUi::EFocusInsideControl;
		if ( parseurls )
			{				
				iUrlContent = CUrlMultiLabel::NewL( NULL, PrimaryTextColor(), focusTextColor,
													parseurls, BubbleContent() );
				BubbleContent().AddControlL( iUrlContent, EContentLabel );
				iUrlContent->EnableFocusBackgroundL( focusBgColor );
				BubbleContent().GetRootSizerL()->AddL( *iUrlContent, 0, Juik::EExpand );
				if ( iFocusHandling ) iFocusHandling->AddL( *iUrlContent );
			}
		else 
			{
				iBasicContent = CJuikLabel::NewL( PrimaryTextColor(), focusTextColor,
											 JuikFonts::Normal(), BubbleContent() );
				BubbleContent().AddControlL( iBasicContent, EContentLabel );
				BubbleContent().GetRootSizerL()->AddL( *iBasicContent, 0, Juik::EExpand );
			}
		
		
		iMetadata = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
									  JuikFonts::Tiny(), BubbleContent() );
		BubbleContent().AddControlL( iMetadata, EMetadataLabel );
		BubbleContent().GetRootSizerL()->AddL( *iMetadata, 0, Juik::EExpand ); 
	}
	
	
	
	~CFeedCommentControlImpl()
	{
	}
	
	

	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{	
// 		TBuf<100> tmpBuf;
// 		tmpBuf.Format( _L("\t\t\tPost: type %d, code %d"), aType, aKeyEvent.iCode );
// 		Reporting().DebugLog( tmpBuf );
		
		if ( ! iFocusHandling )
			return EKeyWasNotConsumed;
		
		
		if ( aKeyEvent.iCode == JOY_DOWN 
			 || aKeyEvent.iCode == JOY_UP )
			{
				return CBubbledStreamItem::OfferKeyEventL( aKeyEvent, aType );
				
			}

		if ( ! iUrlContent )
			return EKeyWasNotConsumed;
		

		if ( aKeyEvent.iCode == JOY_CLICK )
			{
				if ( iUrlContent->IsFocused() )
					{
						LaunchBrowserL( iUrlContent->FocusedUrlL() );
						return EKeyWasConsumed;
					}
			}
		return EKeyWasNotConsumed;
	}
	
// 	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
// 	{	
// // 		TBuf<100> tmpBuf;
// // 		tmpBuf.Format( _L("\t\t\tPost: type %d, code %d"), aType, aKeyEvent.iCode );
// // 		Reporting().DebugLog( tmpBuf );
		
// 		if ( ! ( iFlags & FeedUi::EFocusInsideControl ) )
// 			{
// 				return EKeyWasNotConsumed;
// 			}
			
// // 		if ( aType == EEventKey )
// // 			{
// // 			}
// 		return EKeyWasNotConsumed;
// 	}

	void UpdateCommentMetadataL( CBBFeedItem& aItem )
	{
		CALLSTACKITEM_N(_CL("CFeedCommentControlImpl"), _CL("UpdateCommentMetadataL"));
		TTime now = GetTime();					
		TTime tstamp =  aItem.iCreated();
		
		TTimePeriod period = TTimePeriod::BetweenL( tstamp, now );
		iWorkBuf.Zero();
		if ( iFlags & FeedUi::EShowParentInfo )
			{
				TPtrC parentTitle = aItem.iParentTitle().Left(20);
				if ( ! (iFlags & FeedUi::EHideAuthorName ) )
					{
						iWorkBuf.Append( _L("From ") );
						iWorkBuf.Append( aItem.iAuthorNick() );
						iWorkBuf.Append( _L(" in ") );
					}
				else
					{
						iWorkBuf.Append( _L("In ") );
					}
				iWorkBuf.Append( _L("response to \"") );
				iWorkBuf.Append( parentTitle );
				if ( aItem.iParentTitle().Length() > parentTitle.Length() )
					iWorkBuf.Append( KEllipsis );
				iWorkBuf.Append( _L("\" ") );
			}
		else
			{
				if ( ! (iFlags & FeedUi::EHideAuthorName ) )
					{
						iWorkBuf.Append( _L("Comment from ") );
						iWorkBuf.Append( aItem.iAuthorNick() );
						iWorkBuf.Append( _L(" ") );
					}
				else
					{
						iWorkBuf.Append( _L("Commented ") );
					}
			}
		PeriodFormatter().AgoTextL(period, iWorkBuf);
		iMetadata->UpdateTextL( iWorkBuf );
	}		


	void ZeroL()
	{
		CALLSTACKITEM_N(_CL("CFeedCommentControlImpl"), _CL("ZeroL"));
		ZeroBaseL();
		iMetadata->ZeroL();
		if ( iBasicContent ) iBasicContent->ZeroL();
		if ( iUrlContent ) iUrlContent->ZeroL();
	}

	void UpdateCommentL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CFeedCommentControl"), _CL("UpdateCommentL"));
		
#ifdef __DEV__
		{
			TInt id = aItem.iLocalDatabaseId;
#ifdef JUIK_DEBUGGING_ENABLED
			Reporting().DebugLog( _L("Update comment:"), id );
			Reporting().DebugLog( aItem.iContent().Left(20) );

#endif
			BubbleContent().SetDebugId( 60000 + id );
			if ( iBasicContent ) iBasicContent->SetDebugId( 70000 + id );
			if ( iUrlContent )   iUrlContent->SetDebugId( 70000 + id );
			iMetadata->SetDebugId( 80000 + id );
		}
#endif 

		TInt KFixedPartLength = 10;
		TInt length = aItem.iContent().Length() + aItem.iParentAuthorNick().Length() + KFixedPartLength; 
		auto_ptr<HBufC> content( HBufC::NewL( length ) );
		{
			TPtr ptr = content->Des();
			if ( iFlags & FeedUi::EShowParentInfo )
				{
					SafeAppend( ptr, _L("(to ") );
					SafeAppend( ptr, aItem.iParentAuthorNick() );
					SafeAppend( ptr, _L(") ") );
				}
			SafeAppend( ptr, aItem.iContent() );
		}

		TRgb focusTextColor = PrimaryHighlightTextColor();
		TRgb focusBgColor =  BubbleFocusColor();
// 		if ( iFlags & FeedUi::EDrawFullFocus )
// 			SwapColors( focusTextColor, focusBgColor );

		if ( iBasicContent )
			{
				
				iBasicContent->UpdateTextL( *content );
				if ( aItem.iFromServer() )
					iBasicContent->SetColorsL( iServerColor, focusTextColor );
				else 
					iBasicContent->SetColorsL( iLocalColor, SecondaryHighlightTextColor() );
			}
		
		if ( iUrlContent )
			{
				iUrlContent->UpdateTextL( *content );
				if ( aItem.iFromServer() )
					iUrlContent->SetColorsL( iServerColor, focusTextColor );
				else
					iUrlContent->SetColorsL( iLocalColor, SecondaryHighlightTextColor() );
			}
	}

	void UpdateL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CFeedCommentControlImpl"), _CL("UpdateL"));

		//ZeroL();		
		UpdateBaseL( aItem );
		UpdateCommentL( aItem );
		UpdateCommentMetadataL( aItem );
	}
	
private:
	CJuikLabel* iBasicContent; // created, but not owned
	CUrlMultiLabel* iUrlContent; // created, but not owned
	CJuikLabel* iMetadata; // created, but not owned
};


CFeedCommentControl* CFeedCommentControl::NewL(const TUiDelegates& aDelegates,
											   TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CFeedCommentControl"), _CL("NewL"));
	return CFeedCommentControlImpl::NewL( aDelegates, aFlags );
}


class CRssItemControlImpl : public CRssItemControl
{
public:
	static CRssItemControlImpl* NewL(const TUiDelegates& aDelegates,
									 TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CRssItemControlImpl"), _CL("NewL"));
		auto_ptr<CRssItemControlImpl> self( new (ELeave) CRssItemControlImpl(aDelegates, aFlags) );
		self->ConstructL();
		return self.release();
	}


	CRssItemControlImpl(const TUiDelegates& aDelegates,TInt aFlags) : CRssItemControl(aDelegates, aFlags)
	{
	}


	void UpdateContentL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CRssItemControlImpl"), _CL("UpdateContentL"));

		TRgb focusTextColor = PrimaryHighlightTextColor();
		TRgb focusBgColor =  CommentBubbleFocusColor();
// 		if ( iFlags & FeedUi::EDrawFullFocus )
// 			SwapColors( focusTextColor, focusBgColor );


		iPost->UpdateTextL( aItem.iContent() );
		if ( aItem.iFromServer() )
			{
				iPost->SetColorsL( iServerColor, focusTextColor );
			}
		else 
			{
				iPost->SetColorsL( iLocalColor, SecondaryHighlightTextColor() );
			}
	}


	void ConstructL() 
	{
		CALLSTACKITEM_N(_CL("CRssItemControlImpl"), _CL("ConstructL"));		

		BaseConstructL();
		// content
			
		
		// content
		{			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
				BubbleContent().SetRootSizerL( sizer );
			}
			 
			{

				TRgb focusTextColor = PrimaryHighlightTextColor();
				TRgb focusBgColor =  CommentBubbleFocusColor();
// 				if ( iFlags & FeedUi::EDrawFullFocus )
// 					SwapColors( focusTextColor, focusBgColor );				
				
				iJaicon = CreateJaiconL(BubbleContent());
				iPost = CMultiLabel::NewL( iJaicon, 
										   PrimaryTextColor(), focusTextColor );
				if (iFlags & FeedUi::EFocusInsideControl)
					iPost->EnableFocusBackgroundL( focusBgColor );				
 				BubbleContent().AddControlL( iPost, EPost );
				BubbleContent().GetRootSizerL()->AddL( *iPost, 0, Juik::EExpand ); 
				if ( iFocusHandling ) iFocusHandling->AddL( *iPost );
			}
			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
				BubbleContent().AddSizerL( sizer, EMetadataSizer );
				BubbleContent().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
			}
			
			
			{
				iMetadata = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
											  JuikFonts::Tiny(), BubbleContent() );
				BubbleContent().AddControlL( iMetadata, EMetadataLabel );
				BubbleContent().GetSizerL( EMetadataSizer )->AddL( *iMetadata, 1, Juik::EExpandNot ); 
				
				iCommentIndicator = CCommentIndicator::NewL( iDelegates );
				BubbleContent().AddControlL( iCommentIndicator, ECommentIndicator );
				BubbleContent().GetSizerL( EMetadataSizer )->AddL( *iCommentIndicator, 0, Juik::EExpandNot | Juik::EAlignBottom);
			}
		}
	}
	

	~CRssItemControlImpl()
	{
	}


	virtual const TTypeName& TypeName() const 
	{
		return KRssItemInStreamControlType;
	}


	virtual CBubbledStreamItem::TBubbleType BubbleType() const
	{
		return CBubbledStreamItem::ENoBubble;
	}
	

	void UpdateFeedMetadataL( CBBFeedItem& aItem ) 
	{
		CALLSTACKITEM_N(_CL("CRssItemControlImpl"), _CL("UpdateFeedMetadataL"));
		TTime now = GetTime();					
		TTime tstamp =  aItem.iCreated();
			
		TTimePeriod period = TTimePeriod::BetweenL( tstamp, now );
		iWorkBuf.Zero();
		if ( aItem.iGroupChildCount > 0 )
			{
				iWorkBuf.Append( _L("and ") );
				TBuf<10> num;
				num.Num( aItem.iGroupChildCount );
				iWorkBuf.Append( num );
				iWorkBuf.Append( _L(" others ") );
			}

		if ( ! (iFlags & FeedUi::EHideAuthorName ) )
			{
				iWorkBuf.Append( _L("fetched from ") );
				iWorkBuf.Append( aItem.iAuthorNick() );
				iWorkBuf.Append( _L("'s feeds ") );
			}
		else
			{				
				iWorkBuf.Append( _L("fetched from feeds ") );
			}
		PeriodFormatter().AgoTextL(period, iWorkBuf);
		iMetadata->UpdateTextL( iWorkBuf );
	}


	void ZeroL()
	{
		ZeroBaseL();
		iMetadata->ZeroL();
		iPost->ZeroL();
		iCommentIndicator->ZeroL();
	}

	void UpdateL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CRssItemControlImpl"), _CL("UpdateL"));
		//ZeroL();

		UpdateBaseL( aItem );
		UpdateFeedMetadataL( aItem );
		UpdateContentL( aItem );
		UpdateJaiconL(*iJaicon, aItem );
		iCommentIndicator->UpdateL( aItem );
	}
	


	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{	
		CALLSTACKITEM_N(_CL("CRssItemControlImpl"), _CL("OfferKeyEventL"));
		// 		TBuf<100> tmpBuf;
		// 		tmpBuf.Format( _L("\t\t\tPost: type %d, code %d"), aType, aKeyEvent.iCode );
		// 		Reporting().DebugLog( tmpBuf );

		if ( ! iFocusHandling )
			return EKeyWasNotConsumed;
		
		if ( aKeyEvent.iCode == JOY_DOWN 
			 || aKeyEvent.iCode == JOY_UP )
			{
				return CBubbledStreamItem::OfferKeyEventL( aKeyEvent, aType );
			}
		
		if ( ! iPost )
			return EKeyWasNotConsumed;
		
// 		// No url 
// 		if ( iFeedItem->iLinkedUrl().Length() == 0 )
// 			return EKeyWasNotConsumed;
		

		if ( aType == EEventKey )
			{
				if ( aKeyEvent.iCode == JOY_CLICK )
					{
						if ( iPost->IsFocused() )
							{
								LaunchBrowserL( iFeedItem->iLinkedUrl() );
								return EKeyWasConsumed;
							}
					}
			}
		return EKeyWasNotConsumed;
	}
	
private:	
	CMultiLabel* iPost;
	CJuikLabel* iMetadata;
	CJuikImage* iJaicon;

	CCommentIndicator* iCommentIndicator;
};


CRssItemControl* CRssItemControl::NewL(const TUiDelegates& aDelegates,
									   TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CRssItemControl"), _CL("NewL"));
	return CRssItemControlImpl::NewL( aDelegates, aFlags );
}





class CButtonControlImpl : public CButtonControl
{
public:
	static CButtonControlImpl* NewL(TInt aId, const TDesC& aText,
									const TUiDelegates& aDelegates,
									TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CButtonControlImpl"), _CL("NewL"));
		auto_ptr<CButtonControlImpl> self( new (ELeave) CButtonControlImpl(aId, aDelegates, aFlags) );
		self->ConstructL(aText);
		return self.release();
	}
	
	CButtonControlImpl(TInt aId, const TUiDelegates& aDelegates,TInt aFlags) 
		: CButtonControl(aDelegates, aFlags), iId(aId)
	{
	}
	
	virtual const TTypeName& TypeName() const 
	{
		return KButtonControlType;
	}
	
	TUint Id() const { return iId; }
	
	void ConstructL(const TDesC& aText) 
	{
		CALLSTACKITEM_N(_CL("CButtonControlImpl"), _CL("ConstructL"));		
		BaseConstructL();
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			Content().SetRootSizerL( sizer );
		}
		
		TInt runningId = 2000;
		{
			iButton = CJsuiButton::NewL( aText, iDelegates, iFlags );
			Content().AddControlL( iButton, runningId++ );
			Content().GetRootSizerL()->AddL( *iButton, 0, Juik::EExpand | Juik::EAlignCenterHorizontal);
		}
	}
	
	~CButtonControlImpl()
	{
	}

	void FocusChanged(TDrawNow aDrawNow)
	{
		CALLSTACKITEM_N(_CL("CButtonControlImpl"), _CL("FocusChanged"));
		iContainer->SetFocus(IsFocused(), aDrawNow);
	}
	

	void UpdateL() 
	{
		CALLSTACKITEM_N(_CL("CButtonControlImpl"), _CL("UpdateL"));
	}
	
	
	void UpdateL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CButtonControlImpl"), _CL("UpdateL"));
		if ( iButton->IsHidden() ) iButton->SetHidden( EFalse );
	}
	

	void ZeroL()
	{
		iButton->SetHidden( ETrue );
	}


private:	
	TInt iId;
	CJsuiButton* iButton;
};

	
CButtonControl* CButtonControl::NewL(TInt aId, const TDesC& aText,
									 const TUiDelegates& aDelegates,
									 TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CButtonControl"), _CL("NewL"));
	return CButtonControlImpl::NewL( aId, aText, aDelegates, aFlags );
}



class CIndividualPostControlImpl : public CIndividualPostControl 
{
public:
	static CIndividualPostControlImpl* NewL(const TUiDelegates& aDelegates,
								  TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CIndividualPostControlImpl"), _CL("NewL"));
		auto_ptr<CIndividualPostControlImpl> self( new (ELeave) CIndividualPostControlImpl(aDelegates, aFlags) );
		self->ConstructL();
		return self.release();
	}

	CIndividualPostControlImpl(const TUiDelegates& aDelegates,TInt aFlags) : CIndividualPostControl(aDelegates, aFlags)
	{
	}

	TUint Id() const { return iId; }


	virtual const TTypeName& TypeName() const 
	{
		return KIndividualPostControlType;
	}


	virtual CBubbledStreamItem::TBubbleType BubbleType() const
	{
		return CBubbledStreamItem::ENormal;
	}

	 
	void ConstructL() 
	{
		CALLSTACKITEM_N(_CL("CIndividualPostControlImpl"), _CL("ConstructL"));		

		BaseConstructL();
		//iWrapper = CMultiLabelWrapper::NewL();
		// content
		{			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
				BubbleContent().SetRootSizerL( sizer );
			}
			
			{
				iAuthorName = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
												JuikFonts::Small(), BubbleContent() );
				BubbleContent().AddControlL( iAuthorName, EAuthorNameLabel );
				BubbleContent().GetRootSizerL()->AddL( *iAuthorName, 0, Juik::EExpandNot ); 
			}


			{

				TRgb focusTextColor = PrimaryHighlightTextColor();
				TRgb focusBgColor =  CommentBubbleFocusColor();
// 				if ( iFlags & FeedUi::EDrawFullFocus )
// 					SwapColors( focusTextColor, focusBgColor );
				
				iJaicon = CreateJaiconL(BubbleContent());
				TBool parseurls = iFlags & FeedUi::EFocusInsideControl;
				iPost = CUrlMultiLabel::NewL( iJaicon, PrimaryTextColor(), focusTextColor, 
											  parseurls, BubbleContent() );
 				BubbleContent().AddControlL( iPost, EPost );
				iPost->EnableFocusBackgroundL( focusBgColor );
				BubbleContent().GetRootSizerL()->AddL( *iPost, 0, Juik::EExpand ); 
				if (iFocusHandling) iFocusHandling->AddL( *iPost );
			}
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
				BubbleContent().AddSizerL( sizer, EMetadataSizer );
				BubbleContent().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
			}
			
			
			{
				iMetadata = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
											  JuikFonts::Tiny(), BubbleContent() );
				BubbleContent().AddControlL( iMetadata, EMetadataLabel );
				BubbleContent().GetSizerL( EMetadataSizer )->AddL( *iMetadata, 1, Juik::EExpandNot ); 
			}			

		}
	}
	 

	~CIndividualPostControlImpl()
	{
	}

		
	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{	
		CALLSTACKITEM_N(_CL("CIndividualPostControlImpl"), _CL("OfferKeyEventL"));

// 		TBuf<100> tmpBuf;
// 		tmpBuf.Format( _L("\t\t\tPost: type %d, code %d"), aType, aKeyEvent.iCode );
// 		Reporting().DebugLog( tmpBuf );
		
		if ( ! iFocusHandling )
			return EKeyWasNotConsumed;
		
		if ( aKeyEvent.iCode == JOY_DOWN 
			 || aKeyEvent.iCode == JOY_UP )
			{
				return CBubbledStreamItem::OfferKeyEventL( aKeyEvent, aType );
			}
		
		if ( aType == EEventKey )
			{
				if ( aKeyEvent.iCode == JOY_CLICK )
					{
						if ( iPost->IsFocused() )
							{
								LaunchBrowserL( iPost->FocusedUrlL() );
								return EKeyWasConsumed;
							}
					}
			}
		return EKeyWasNotConsumed;
	}
	
	void UpdateAuthorNameL(CBBFeedItem& aItem) 
	{
		CALLSTACKITEM_N(_CL("CIndividualPostControlImpl"), _CL("UpdateMetadataL"));
		iWorkBuf.Zero();
		iWorkBuf.Append( aItem.iAuthorNick() );
		iWorkBuf.Append( _L(" said:") );		 
		iAuthorName->UpdateTextL( iWorkBuf );
	}


	void UpdateMetadataL(CBBFeedItem& aItem) 
	{
		CALLSTACKITEM_N(_CL("CIndividualPostControlImpl"), _CL("UpdateMetadataL"));
		iWorkBuf.Zero();

		TTime tstamp =  aItem.iCreated();
		if ( tstamp != Time::NullTTime() )
			{
				TTime now = GetTime();					
				TTimePeriod period = TTimePeriod::BetweenL( tstamp, now );

				PeriodFormatter().AgoTextL(period, iWorkBuf);
				if ( aItem.iLocation().Length() > 0 )
					{
						iWorkBuf.Append( _L(" in ") );
						iWorkBuf.Append( aItem.iLocation()  );
					}
			}
		else
			{
				iWorkBuf.Append( _L("Full post is not available") );
			}
		
		iMetadata->UpdateTextL( iWorkBuf );
	}

	void UpdateContentL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CIndividualPostControlImpl"), _CL("UpdateContentL"));
		iPost->UpdateTextL( aItem.iContent() );
		if ( aItem.iFromServer() )
			{
				TRgb focusTextColor = PrimaryHighlightTextColor();
				TRgb focusBgColor =  CommentBubbleFocusColor();
// 				if ( iFlags & FeedUi::EDrawFullFocus )
// 					SwapColors( focusTextColor, focusBgColor );
				
				iPost->SetColorsL( iServerColor, focusTextColor );
			}
		else 
			{
				iPost->SetColorsL( iLocalColor, SecondaryHighlightTextColor() );
			}
	}


	void ZeroL()
	{
		ZeroBaseL();
		iAuthorName->ZeroL();
		iMetadata->ZeroL();
		iPost->ZeroL();
		iJaicon->ClearL();
	}

	void UpdateL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CIndividualPostControlImpl"), _CL("UpdateL"));
		//ZeroL();

		UpdateBaseL( aItem );
		UpdateAuthorNameL( aItem );
		UpdateMetadataL( aItem );
		UpdateJaiconL( *iJaicon, aItem );
		UpdateContentL( aItem );
	}
	
	
private:	
	CJuikLabel* iMetadata;
	CJuikImage* iJaicon;
	CUrlMultiLabel* iPost;
	CJuikLabel* iAuthorName;
};


CIndividualPostControl* CIndividualPostControl::NewL(const TUiDelegates& aDelegates,
													 TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CIndividualPostControl"), _CL("NewL"));
	return CIndividualPostControlImpl::NewL( aDelegates, aFlags );
}



class CMissingPostImpl : public CMissingPost 
{
public:
	static CMissingPostImpl* NewL(const TUiDelegates& aDelegates,
								  TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CMissingPostImpl"), _CL("NewL"));
		auto_ptr<CMissingPostImpl> self( new (ELeave) CMissingPostImpl(aDelegates, aFlags) );
		self->ConstructL();
		return self.release();
	}

	CMissingPostImpl(const TUiDelegates& aDelegates,TInt aFlags) : CMissingPost(aDelegates, aFlags)
	{
	}

	TUint Id() const { return KMissingPostItem; }

	virtual const TTypeName& TypeName() const 
	{
		return KMissingPostControlType;
	}


	virtual CBubbledStreamItem::TBubbleType BubbleType() const
	{
		return CBubbledStreamItem::ENoBubble;
	}

	 
	void ConstructL() 
	{
		CALLSTACKITEM_N(_CL("CMissingPostImpl"), _CL("ConstructL"));		

		BaseConstructL();
		{			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
				BubbleContent().SetRootSizerL( sizer );
			}
			
			{
				iAuthorName = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
												JuikFonts::Small(), BubbleContent() );
				BubbleContent().AddControlL( iAuthorName, EAuthorNameLabel );
				BubbleContent().GetRootSizerL()->AddL( *iAuthorName, 0, Juik::EExpandNot ); 
			}


			{
// 				iJaicon = CreateJaiconL(BubbleContent());
				TBool parseurls = iFlags & FeedUi::EFocusInsideControl;
				iPost = CUrlMultiLabel::NewL( NULL, PrimaryTextColor(), PrimaryHighlightTextColor(), 
											  parseurls, BubbleContent() );
 				BubbleContent().AddControlL( iPost, EPost );
				iPost->EnableFocusBackgroundL( BubbleFocusColor() );
				BubbleContent().GetRootSizerL()->AddL( *iPost, 0, Juik::EExpand ); 
				if ( iFocusHandling ) iFocusHandling->AddL( *iPost );
			}
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
				BubbleContent().AddSizerL( sizer, EMetadataSizer );
				BubbleContent().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
			}
			
			
			{
				iMetadata = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
											  JuikFonts::Tiny(), BubbleContent() );
				BubbleContent().AddControlL( iMetadata, EMetadataLabel );
				iMetadata->UpdateTextL( _L("Original post is missing from the phone") ); 

				BubbleContent().GetSizerL( EMetadataSizer )->AddL( *iMetadata, 1, Juik::EExpandNot ); 
			}			

		}

// 		{			
// 			{
// 				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
// 				BubbleContent().SetRootSizerL( sizer );
// 			}
			
// 			{
// 				iLabel = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
// 										   JuikFonts::Normal(), BubbleContent() );
				
// 				BubbleContent().AddControlL( iLabel, EAuthorNameLabel );
// 				iLabel->UpdateTextL( _L("Original post is missing from the phone") ); 
// 				BubbleContent().GetRootSizerL()->AddL( *iLabel, 0, Juik::EExpandNot ); 
// 			}

// 		}
// 	}
	}

		
	 

	~CMissingPostImpl()
	{
	}


	void UpdateL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CMissingPostImpl"), _CL("UpdateL"));
	}
	


	
	void UpdateViaChildL(CBBFeedItem& aChildItem)
	{
		CALLSTACKITEM_N(_CL("CMissingPostImpl"), _CL("UpdateViaChildL"));
		

		iWorkBuf.Zero();
		iWorkBuf.Append( aChildItem.iParentTitle() );
		iWorkBuf.Append( _L(" said:") );		 
		iAuthorName->UpdateTextL( iWorkBuf );
		
		iPost->UpdateTextL( aChildItem.iParentTitle() );
		
		iMetadata->UpdateTextL( _L("Real item is missing from mobile client") );
	}
	
	
	void ZeroL()
	{
		iMetadata->ZeroL();
		iAuthorName->ZeroL();
		iPost->ZeroL();
	}
	

private:	
	CJuikLabel* iMetadata;
	CUrlMultiLabel* iPost;
	CJuikLabel* iAuthorName;
};


CMissingPost* CMissingPost::NewL(const TUiDelegates& aDelegates,
								 TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CMissingPost"), _CL("NewL"));
	return CMissingPostImpl::NewL( aDelegates, aFlags );
}


#ifdef __JAIKU_PHOTO_DOWNLOAD__


class CMediaPostControlImpl2 : public CMediaPostControl
{
public:
	static CMediaPostControlImpl2* NewL(CCoeControl& aParent,
										const TUiDelegates& aDelegates,
										TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CMediaPostControlImpl2"), _CL("NewL"));
		auto_ptr<CMediaPostControlImpl2> self( new (ELeave) CMediaPostControlImpl2(aDelegates, aFlags) );
		self->ConstructL(aParent);
		return self.release();
	}

	CMediaPostControlImpl2(const TUiDelegates& aDelegates,TInt aFlags) : CMediaPostControl(aDelegates, aFlags)
	{
	}

	virtual const TTypeName& TypeName() const 
	{
		return KPostInStreamControlType;
	}

	virtual CBubbledStreamItem::TBubbleType BubbleType() const
	{
		return CBubbledStreamItem::ENoBubble;
	}

	TSize MinimumSize()
	{
		TSize sz = CMediaPostControl::MinimumSize();
		return sz;
	}

	void SizeChanged()
	{
		TRect r = Rect();
		CMediaPostControl::SizeChanged();
	}


	TInt iRunningId;

	void ConstructL(CCoeControl& aParent) 
	{
		CALLSTACKITEM_N(_CL("CMediaPostControlImpl2"), _CL("ConstructL"));		

		SetContainerWindowL( aParent );

		if ( iFlags & FeedUi::EShowFullImage ) 
			iFlags &= ~ FeedUi::EShowBuddyIcon;

		BaseConstructL();

		BubbleContent().SetDebugId(20000);
		iRunningId = 5000;
		//iWrapper = CMultiLabelWrapper::NewL();
		// content
		{			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
				BubbleContent().SetRootSizerL( sizer );
			}
		
			
			{
				iPost = CMultiLabel::NewL( NULL, 
										   PrimaryTextColor(), PrimaryHighlightTextColor());
 				BubbleContent().AddControlL( iPost, EPost );
				BubbleContent().GetRootSizerL()->AddL( *iPost, 0, Juik::EExpand ); 
			}

			{
				iPhotoHolder = CPhotoHolder::NewL( *this, iDelegates, iFlags, BubbleContent() );
 				BubbleContent().AddControlL( iPhotoHolder, iRunningId++ );
				BubbleContent().GetRootSizerL()->AddL( *iPhotoHolder, 0, Juik::EExpand ); 
				if ( iFocusHandling ) iFocusHandling->AddL( *iPhotoHolder );
			}

// 			if ( iFocusHandling ) 
// 				{
// 					iFeedLink = CJuikLabel::NewL( SecondaryTextColor(), 
// 												  SecondaryHighlightTextColor(),
// 												  JuikFonts::Tiny(), 
// 												  BubbleContent() );
// 					TRgb focusBgColor =  BubbleFocusColor();
// 					iFeedLink->EnableFocusBackgroundL( focusBgColor );
// 					BubbleContent().AddControlL( iFeedLink, iRunningId++ );
// 					BubbleContent().GetRootSizerL()->AddL( *iFeedLink, 0, 
// 														   Juik::EExpand );
// 					if ( iFocusHandling ) iFocusHandling->AddL( *iFeedLink );
// 				}
			
			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
				BubbleContent().AddSizerL( sizer, EMetadataSizer );
				BubbleContent().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand ); 
			}
			
			
			{
				iMetadata = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
											  JuikFonts::Tiny(), BubbleContent() );
				BubbleContent().AddControlL( iMetadata, EMetadataLabel );
				BubbleContent().GetSizerL( EMetadataSizer )->AddL( *iMetadata, 1, Juik::EExpandNot ); 
				
				iCommentIndicator = CCommentIndicator::NewL( iDelegates );
				BubbleContent().AddControlL( iCommentIndicator, ECommentIndicator );
				BubbleContent().GetSizerL( EMetadataSizer )->AddL( *iCommentIndicator, 0, Juik::EExpandNot | Juik::EAlignBottom);
			}
		}
	}
	 

	~CMediaPostControlImpl2()
	{
	}


	virtual TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{	
// 		TBuf<100> tmpBuf;
// 		tmpBuf.Format( _L("\t\t\tPost: type %d, code %d"), aType, aKeyEvent.iCode );
// 		Reporting().DebugLog( tmpBuf );
		
		if ( ! iFocusHandling )
			{
				return EKeyWasNotConsumed;
			}

		if ( aKeyEvent.iCode == JOY_DOWN 
			 || aKeyEvent.iCode == JOY_UP )
			{
				return CBubbledStreamItem::OfferKeyEventL( aKeyEvent, aType );
			}

	   
		if ( aType == EEventKey )
			{
				if ( aKeyEvent.iCode == JOY_CLICK )
					{
						if ( iPhotoHolder->IsFocused() )
							{
								return iPhotoHolder->OfferKeyEventL( aKeyEvent, aType );
							}
						else if ( iFeedLink && iFeedLink->IsFocused() )
							{
								LaunchBrowserL( iFeedItem->iLinkedUrl() );
								return EKeyWasConsumed;
							}
					}
			}
		return EKeyWasNotConsumed;
	}


	void UpdateFeedMetadataL( CBBFeedItem& aItem ) 
	{
		CALLSTACKITEM_N(_CL("CMediaPostControlImpl2"), _CL("UpdateFeedMetadataL"));
		TTime now = GetTime();					
		TTime tstamp =  aItem.iCreated();
			
		TTimePeriod period = TTimePeriod::BetweenL( tstamp, now );
		iWorkBuf.Zero();
		if ( aItem.iLocation().Length() > 0 )
			{
				iWorkBuf.Append( _L("in ") );
				iWorkBuf.Append( aItem.iLocation()  );
				iWorkBuf.Append( _L(" ") );
			}
		
		if ( ! (iFlags & FeedUi::EFocusInsideControl )  )
			{
				if ( aItem.iGroupChildCount > 0 )
					{
						iWorkBuf.Append( _L("and ") );
						TBuf<10> num;
						num.Num( aItem.iGroupChildCount );
						iWorkBuf.Append( num );
						iWorkBuf.Append( _L(" others ") );
					}
				
				if ( ! (iFlags & FeedUi::EHideAuthorName ) )
					{
						iWorkBuf.Append( _L("fetched from ") );
						iWorkBuf.Append( aItem.iAuthorNick() );
						iWorkBuf.Append( _L("'s feeds ") );
					}
				else
					{				
						iWorkBuf.Append( _L("fetched from feeds ") );
					}
			}
		
		

		PeriodFormatter().AgoTextL(period, iWorkBuf);
		iMetadata->UpdateTextL( iWorkBuf );


		if ( iFeedLink )
			{
				iWorkBuf.Zero();
				iWorkBuf.Append( _L("fetched from ") );
				iWorkBuf.Append( aItem.iAuthorNick() );
				iWorkBuf.Append( _L("'s photo feed") );
				iFeedLink->UpdateTextL( iWorkBuf );
			}
	}


	void UpdateContentL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CMediaPostControlImpl2"), _CL("UpdateContentL"));
		iPost->UpdateTextL( aItem.iContent() );

		if ( aItem.iFromServer() )
			{
				iPost->SetColorsL( iServerColor, PrimaryHighlightTextColor() );
			}
		else 
			{
				iPost->SetColorsL( iLocalColor, SecondaryHighlightTextColor() );
			}
	}

	void ZeroL()
	{
		CALLSTACKITEM_N(_CL("CMediaPostControlImpl2"), _CL("ZeroL"));

		ZeroBaseL();
		iPhotoHolder->ZeroL();
		iMetadata->ZeroL();
		if (iFeedLink) iFeedLink->ZeroL();
		iPost->ZeroL();
		iCommentIndicator->ZeroL();
	}


	void UpdateL(CBBFeedItem& aItem)
	{
		CALLSTACKITEM_N(_CL("CMediaPostControlImpl2"), _CL("UpdateL"));

		//ZeroL(); // this is plain wrong, because these call CJuikLabel::ZeroL which setsize to 0,0.
#ifdef __DEV__
		{
			TInt id = aItem.iLocalDatabaseId;
			BubbleContent().SetDebugId( 20000 + id );
			iPost->SetDebugId( 30000 + id );
			iMetadata->SetDebugId( 40000 + id );
		}
#endif

		UpdateBaseL( aItem );
		iPhotoHolder->UpdateL( aItem );
		UpdateFeedMetadataL( aItem );
		UpdateContentL( aItem );
		iCommentIndicator->UpdateL( aItem );
	}
	
	
private:	
	CPhotoHolder* iPhotoHolder;
 	CJuikLabel* iMetadata;
 	CJuikLabel* iFeedLink;
	CMultiLabel* iPost;
	CCommentIndicator* iCommentIndicator;
};


CMediaPostControl* CMediaPostControl::NewL(CCoeControl& aParent,
										   const TUiDelegates& aDelegates,
										   TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CMediaPostControl"), _CL("NewL"));
	return CMediaPostControlImpl2::NewL( aParent, aDelegates, aFlags );
}

#endif // __JAIKU_PHOTO_DOWNLOAD__
