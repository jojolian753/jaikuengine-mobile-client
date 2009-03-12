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

#include "ccu_authorheadercontrol.h"

#include "PresenceTextFormatter.h"
#include "cb_presence.h"
#include "ccu_presencestatus.h"
#include "ccu_timeperiod.h"
#include "ccu_staticicons.h"
#include "ccu_feedcontrols.h"
#include "ccu_feedmodel.h"
#include "ccu_streamrenderers.h"
#include "ccu_themes.h"
#include "ccu_trafficlight.h"
#include "ccu_buddyicon.h"
#include "ccu_uidelegates.h"

#include <contextcontactsui.mbg>

#include "cc_stringtools.h"
#include "juik_fonts.h"
#include "juik_iconmanager.h"
#include "juik_image.h"
#include "juik_label.h"
#include "juik_sizer.h"
#include "juik_sizercontainer.h"
#include "juik_layoutitem.h"
#include "juik_layout.h"
#include "juik_gfxstore.h"
#include "jaiku_layoutids.hrh"


#include <aknsdrawutils.h> 
#include <aknsutils.h> 


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
	ELocationLabel,
	ECalendarLabel,
	EPost,
	EMainSizer,
	ESeparator,
	//EAuthorNameLabel,
	//ELastUsedLabel
	};
	


class CAuthorHeaderControlImpl : public CAuthorHeaderControl, 
								 public MContextBase, 
								 public MPresenceListener
{
public:
	static CAuthorHeaderControlImpl* NewL(const TDesC& aNick,
										  const TUiDelegates& aDelegates,
										  TInt aFlags
										  )
	{
		CALLSTACKITEMSTATIC_N(_CL("CAuthorHeaderControlImpl"), _CL("NewL"));
		auto_ptr<CAuthorHeaderControlImpl> self( new (ELeave) CAuthorHeaderControlImpl(aDelegates, aFlags) );
		self->ConstructL(aNick);
		return self.release();
	}
	
	CAuthorHeaderControlImpl(const TUiDelegates& aDelegates,TInt /*aFlags*/) 
		: CAuthorHeaderControl(aDelegates)
	{
	}

	virtual const TTypeName& TypeName() const 
	{
		return KAuthorHeaderControlType;
	}

	CJuikSizerContainer& Content() const
	{
		return *iContainer;
	}

// 	void AddOnelinerL( )
// 	{

// 	}
	TInt iRunningId;


	 
	void ConstructL(const TDesC& aNick) 
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("ConstructL"));		
		iRunningId = 100;
		iMargin = Layout().GetLayoutItemL(LG_feed_controls_margins, LI_feed_controls_margins__author_header).Margins();			

		iNick = aNick;
		
		TInt id = JabberData().GetContactIdL( iNick );
		if ( id != KErrNotFound )
			{
				iFirstName = JabberData().GetFirstNameL(id);
				iLastName = JabberData().GetLastNameL(id);
			}

		if ( !iFirstName )
			iFirstName = HBufC::NewL(1);
			
		if ( ! iLastName )
			iLastName = HBufC::NewL(1);
		
		iFullName = HBufC::NewL( iFirstName->Length() + iLastName->Length() + 10 );

		
		//BaseConstructL();
		iFormatter = CPresenceTextFormatter::NewL();

		{
			iContainer = CJuikSizerContainer::NewL();
			iContainer->SetContainerWindowL(*this);
		}
				
		
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
				Content().SetRootSizerL( sizer );
			}
			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );				 
				Content().AddSizerL( sizer, EMainSizer );
				Content().GetRootSizerL()->AddL( *sizer, 0, Juik::EExpand | Juik::EAlignCenterHorizontal ); 
			}

			{
				iBuddyIconMgr = CBuddyIconMgr::NewL(iNick, iDelegates);
				CJuikImage* image = iBuddyIconMgr->GetControl();
				Content().AddControlL( image, iRunningId++ );
				Content().GetSizerL( EMainSizer )->AddL( *image, 0, Juik::EExpandNot | Juik::EAlignCenterVertical ); 
			}
			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );
				Content().AddSizerL( sizer, EContentSizer );
				Content().GetSizerL( EMainSizer )->AddL( *sizer, 1, Juik::EExpand ); 
				
				{
					{
						MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );
						Content().AddSizerL( sizer, ENameSizer );
						Content().GetSizerL( EContentSizer )->AddL( *sizer, 0, Juik::EExpand ); 
					}
					
					{
						MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );
						Content().AddSizerL( sizer, iRunningId++ );
						Content().GetSizerL( ENameSizer )->AddL( *sizer, 1, Juik::EExpand ); 
						
						{
							iNickLabel = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
														   JuikFonts::Normal(), Content() );
							Content().AddControlL( iNickLabel, iRunningId++ );
							sizer->AddL( *iNickLabel, 0, Juik::EExpand ); 
						}
						
						{
							iNameLabel = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
														   JuikFonts::Tiny(), Content() );
							Content().AddControlL( iNameLabel, iRunningId++ );
							sizer->AddL( *iNameLabel, 0, Juik::EExpand ); 
						}
					}
					
					{
						iTrafficLightMgr = CTrafficLightMgr::NewL(iNick, iDelegates);
						CJuikImage* image = iTrafficLightMgr->GetControl();
						Content().AddControlL( image, iRunningId++ );
						Content().GetSizerL( ENameSizer )->AddL( *image, 0, Juik::EExpandNot  ); 
					}					
					
				}
				



				{
					iLastUsed = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
												  JuikFonts::Tiny(), Content() );
					Content().AddControlL( iLastUsed, iRunningId++ );
					Content().GetSizerL( EContentSizer )->AddL( *iLastUsed, 0, Juik::EExpand ); 
				}
				
				{
					iLocation = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
											   JuikFonts::Tiny(), Content() );
					Content().AddControlL( iLocation, iRunningId++ );
					Content().GetSizerL( EContentSizer )->AddL( *iLocation, 0, Juik::EExpand ); 
				}
				
 				{
 					iCalendar = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
												  JuikFonts::Tiny(), Content() );
  					Content().AddControlL( iCalendar, iRunningId++ );
  					Content().GetSizerL( EContentSizer )->AddL( *iCalendar, 0, Juik::EExpand ); 
				}
				


			}

			{
				TSize iconSize  = Layout().GetLayoutItemL(LG_feed_controls, 
														  LI_feed_controls__separator).Size();
				
				TComponentName name = { { CONTEXT_UID_CONTEXTCONTACTSUI }, EGfxSeparator };
				iSeparatorIcon = Graphics().GetColoredIconL(name, 
															EMbmContextcontactsuiSeparator, iconSize,
															ThemeColors().GetColorL(CThemeColors::EPrimaryText)); 
				
				iSeparator = CJuikImage::NewL( iSeparatorIcon, iconSize);
				Content().AddControlL( iSeparator, iRunningId );
				iSeparator->iMargin = Layout().GetLayoutItemL(LG_feed_controls_margins, 
															  LI_feed_controls_margins__author_header_buddy).Margins();
				iSeparator->SetContainerWindowL( Content() );
				Content().GetRootSizerL()->AddL( *iSeparator, 0, Juik::EAlignCenterHorizontal ); 
			}


			PresenceHolder().AddListener( this );
	}
	
	
	TUint Id() const { return KAuthorHeaderItem; }
	 

	~CAuthorHeaderControlImpl()
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("~CAuthorHeaderControlImpl"));
		PresenceHolder().RemoveListener( this );
		delete iContainer;
		delete iFormatter;
		iDelegates.iFeedGraphics->ReleaseIcon( iSeparatorIcon );
		iSeparatorIcon = NULL;

		delete iTrafficLightMgr;
		delete iBuddyIconMgr;

		delete iFirstName;
		delete iLastName;
		delete iFullName;
	}


	
	void UpdateL() 
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("UpdateL"));
		ZeroL();
		UpdateNameL();
		UpdatePresenceDataL();
	}



	void UpdateNameL()
	{

		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("UpdateNameL"));
		iWorkBuf.Zero();
		iWorkBuf.Append( iNick );
		CJabberData::TransformToUiNickL( iWorkBuf );		
		iNickLabel->UpdateTextL( iWorkBuf );
		
		
		iFullName->Des().Zero();
		if ( iFirstName->Length() > 0 || iLastName->Length() > 0)
			{
				TPtr des = iFullName->Des();
				SafeAppend( des, *iFirstName );
				SafeAppend( des, _L(" ") );
				SafeAppend( des, *iLastName );
			};
		
		if ( iFullName->Length() > 0 )
			iNameLabel->UpdateTextL( *iFullName );
		else
			iNameLabel->ZeroL();
	}
	

	void ClearPresenceDataL()
	{
		iLocation->ZeroL();
		iCalendar->ZeroL();
		iLastUsed->ZeroL();
	}
	
	void UpdatePresenceDataL()
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("UpdatePresenceDataL"));
				
		TInt id = JabberData().GetContactIdL( iNick );
		if ( id == KErrNotFound )
			{
				ClearPresenceDataL();
				return;
			}

		CBBPresence* p = PresenceHolder().GetPresence( id );
		if (!p)
			{
				ClearPresenceDataL();
				return;
			}
				
		{ // Last used
			iWorkBuf.Zero();
			TTime now = GetTime();
			
			TTime since = p->iSentTimeStamp();
			
			if ( IsOffline(*p, now) )
				{
					if ( Presence::IsFromMobile(*p) )
						{
							UserActivity::LastUsed( *p, PeriodFormatter(), iWorkBuf );
						}
				}
			else 
				{
					UserActivity::UserActivity(*p, PeriodFormatter(), iWorkBuf);
				}
			
			if ( iWorkBuf.Length() )
				iLastUsed->UpdateTextL( iWorkBuf );
		}

		
		{ // Location
			iWorkBuf.Zero();
			iFormatter->LocationL(p, iWorkBuf);
			if ( iWorkBuf.Length() )
				iLocation->UpdateTextL( iWorkBuf );
		}
		
		{ // Calendar
			iWorkBuf.Zero();
			const TBBCalendarEvent* event = Calendar::GetEvent( p->iCalendar );
			if ( event && event->iDescription().Length() > 0 )
				{
					if (*event == p->iCalendar.iNext)
						{
							Calendar::NextTextL( iWorkBuf );
							Calendar::EventDateL( *event, iWorkBuf );
						}
					Calendar::EventTimeL( *event, iWorkBuf );
					iWorkBuf.Append( _L(" ") );
					iWorkBuf.Append( event->iDescription() );
					if ( iWorkBuf.Length() )
						iCalendar->UpdateTextL( iWorkBuf );
				}
			else
				{
					iCalendar->ZeroL();
				}
			
		}
	}

	

	virtual void PresenceChangedL(TInt aContactId, CBBPresence* /*aInfo*/) 
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("PresenceChanged"));
		TInt id = JabberData().GetContactIdL( iNick );
		if ( id == aContactId )
			{
				UpdateL();
				MinimumSizeChangedL();
				DrawDeferred();
			}
	}
	
	virtual void Notify(const TDesC & /*aMessage*/) 
	{
		
	}

	
	void UpdateL(CBBFeedItem& /*aItem*/)
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("UpdateL"));
	}
	
	
	TInt CountComponentControls() const
	{
	    CALLSTACKITEM_N(_CL("CAuthorHeaderControl"), _CL("CountComponentControl"));
		return 1;
	}
	
	
	CCoeControl* ComponentControl(TInt /*aIndex*/) const
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControl"), _CL("ComponentControl"));
		return iContainer;
	}

	TSize MinimumSize()
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControl"), _CL("MinimumSize"));
		TSize sz = iContainer->MinimumSize();
		sz += iMargin.SizeDelta();
		return sz;
	}

	void PositionChanged()
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControl"), _CL("PositionChanged"));
		TPoint p = Position() + TSize(iMargin.iLeft, iMargin.iTop);
		iContainer->SetPosition( p );
	}
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControl"), _CL("SizeChanged"));
		TRect r = Rect();
		TRect inner = iMargin.InnerRect(r);
		iContainer->SetRect(inner);
	}


	void Draw(const TRect& /*aRect*/) const
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("Draw"));
		 if ( IsFocused() )
			 {
				 CWindowGc& gc = SystemGc();

				 TRect focusRect = Rect();
				 TInt focusH = focusRect.Height();
 				 if ( iSeparator )
 					 focusH -= iSeparator->MinimumSize().iHeight + iMargin.iBottom;
				 focusRect.SetHeight( Max(0, focusH) );  
				 
				 TRect innerRect = focusRect;
				 innerRect.Shrink(4,4); 
				 
				 MAknsSkinInstance* skin = AknsUtils::SkinInstance();
				 TBool skinnedFocus = AknsDrawUtils::DrawFrame( skin, gc, focusRect, innerRect, 
																KAknsIIDQsnFrList, KAknsIIDQsnFrListCenter);
				 if (!skinnedFocus)
					{
						gc.SetPenStyle( CGraphicsContext::ENullPen );
						gc.SetBrushColor( TRgb(200, 200, 200, 100) );
						gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
						gc.DrawRect( focusRect );
					}
			 }
	}


	void ZeroL()
	{
		CALLSTACKITEM_N(_CL("CAuthorHeaderControlImpl"), _CL("ZeroL"));

		if ( iNickLabel ) iNickLabel->ZeroL();
		if ( iMetadata ) iMetadata->ZeroL();
		if ( iLastUsed ) iLastUsed->ZeroL();
		if ( iLocation ) iLocation->ZeroL();
		if ( iCalendar ) iCalendar->ZeroL();
	}
	

	private:	
	
	CJuikLabel* iNickLabel;
	CJuikLabel* iNameLabel;
	CJuikLabel* iMetadata;
	CJuikLabel* iLastUsed;
	CJuikLabel* iLocation;
	CJuikLabel* iCalendar;


	CBuddyIconMgr* iBuddyIconMgr;
	CTrafficLightMgr* iTrafficLightMgr;

	CJabberData::TNick iNick;
	HBufC* iFirstName;
	HBufC* iLastName;
	HBufC* iFullName;

	CJuikSizerContainer* iContainer;

	TBuf<1000> iWorkBuf;
	CPresenceTextFormatter* iFormatter;

	TMargins8 iMargin;

	CGulIcon* iSeparatorIcon;
	CJuikImage* iSeparator;

	};
	
	
CAuthorHeaderControl* CAuthorHeaderControl::NewL(const TDesC& aNick,
												 const TUiDelegates& aDelegates,
												 TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CAuthorHeaderControlImpl"), _CL("NewL"));
	return CAuthorHeaderControlImpl::NewL( aNick, aDelegates, aFlags );
}

