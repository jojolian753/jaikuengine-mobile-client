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

#include "ccu_presencelist2.h"

#include "app_context.h"

#include "juik_sizercontainer.h"
#include "symbian_auto_ptr.h"
#include "break.h"
#include "reporting.h"

#include "ccu_buddyicon.h"
//#include "ccu_feedfoundation.h" // for Margins
#include "ccu_staticicons.h" 
#include "ccu_uidelegates.h" 
#include "ccu_themes.h" 
#include "ccu_activestate.h" 
#include "ccu_contact.h" 
#include "ccu_presencestatus.h" 
#include "ccu_trafficlight.h" 

#include "juik_iconmanager.h"
#include "juik_image.h"
#include "juik_label.h"
#include "juik_fonts.h"
#include "juik_scrolllist.h"
#include "juik_layout.h"
#include "juik_control.h"
#include "jaiku_layoutids.hrh"
#include "cl_settings.h"
#include "csd_presence.h"
#include <contextcontactsui.mbg>





class CPresenceControlBase : public CCoeControl, public MJuikControl, public MContextBase
{
public:
	enum TFlags
		{
			EMyOwnPresence = 1, 
		};

	class MMinimumSizeObserver 
	{
	public:
		virtual void MinimumSizeChangedL(CPresenceControlBase* aSource, TSize aNewSize) = 0;
	};


	CPresenceControlBase( const TUiDelegates& aDelegates, TInt aFlags ) : iDelegates( aDelegates ), iFlags(aFlags) {}
 
	~CPresenceControlBase()
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("~CPresenceControlBase"));		
		delete iContainer;
	}
	
	virtual void UpdateL( CBBPresence* aPresence ) = 0;

	virtual CPresenceList::TItemId ItemId() const = 0;
protected:


	CThemeColors& ThemeColors() const { return *(iDelegates.iThemeColors); }

	TRgb PrimaryTextColor() const
	{
		return ThemeColors().GetColorL( CThemeColors::EPrimaryText );
	}
	
	TRgb SecondaryTextColor() const
	{
		return ThemeColors().GetColorL( CThemeColors::ESecondaryText );
	}
	
	TRgb PrimaryHighlightTextColor() const
	{
		return ThemeColors().GetColorL( CThemeColors::EPrimaryHighlightText );
	}
	
	TRgb SecondaryHighlightTextColor() const
	{
		return ThemeColors().GetColorL( CThemeColors::ESecondaryHighlightText );
	}
	

	CJuikSizerContainer& Content() const
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("Content"));		
		return *iContainer;
	}

	
	void BaseConstructL() 
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("BaseConstructL"));		
		iMargin =Layout().GetLayoutItemL(LG_presence_list, LI_presence_list__item_margins).Margins();
		
		{
			iContainer = CJuikSizerContainer::NewL();
			iContainer->SetContainerWindowL(*this);
		}
	}
	
	
	void FocusChanged(TDrawNow aDrawNow)
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("FocusChanged"));
		iContainer->SetFocus(IsFocused(), aDrawNow);
	}
	
	
	TInt CountComponentControls() const
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("CountComponentControl"));
		return 1;
	}
	
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("ComponentControl"));
		return iContainer;
	}
	
	TSize MinimumSize()
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("MinimumSize"));
		TSize sz = iContainer->MinimumSize();
		sz += iMargin.SizeDelta();
		return sz;
	}
	
	void PositionChanged()
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("PositionChanged"));
		TPoint p = Position() + TSize(iMargin.iLeft, iMargin.iTop);
		iContainer->SetPosition( p );
	}
	
	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("SizeChanged"));
		TRect r = Rect();
		TRect inner = iMargin.InnerRect(r);
		if (inner.Width() < 0 ||inner.Height() < 0)
			inner.SetSize(TSize(0,0));
		
		iContainer->SetRect(inner);
	}
	

// 	void SetMinimumSizeObserverL(MMinimumSizeObserver* aObserver)
// 	{
// 		iObserver = aObserver;
// 	}
	
	
// 	void MinimumSizeChangedL()
// 	{
// 		CALLSTACKITEM_N(_CL("CFeedControl"), _CL("MinimumSizeChangedL"));
// 		if ( iObserver )
// 			{
// 				TSize sz = MinimumSize();
// 				iObserver->MinimumSizeChangedL( this, sz );
// 			}
// 	}


// 	void DrawNormalFocus(CWindowGc& aGc) const
// 	{
// 		CALLSTACKITEM_N(_CL("CPresenceControlBase"), _CL("DrawNormalFocus"));
// 		if ( IsFocused() )
// 			{
// 				// Non-skin focus
// 				TRect focusRect = Rect();
// 				TRect innerRect = focusRect;
// 				innerRect.Shrink(4,4); 
				
// 				MAknsSkinInstance* skin = AknsUtils::SkinInstance();
// 				TBool skinnedFocus = AknsDrawUtils::DrawFrame( skin, aGc, focusRect, innerRect, 
// 															   KAknsIIDQsnFrList, KAknsIIDQsnFrListCenter);
// 				if (!skinnedFocus)
// 					{
// 						aGc.SetPenStyle( CGraphicsContext::ENullPen );
// 						aGc.SetBrushColor( TRgb(200, 200, 200, 100) );
// 						aGc.SetBrushStyle( CGraphicsContext::ESolidBrush );
// 						aGc.DrawRect( focusRect );
// 					}
// 			}
// 	}
public:
	const CCoeControl* CoeControl() const { return this; } 
	CCoeControl* CoeControl() { return this; }

protected:
	TUiDelegates iDelegates;
	TInt iFlags;
	TMargins8 iMargin;
	TBuf<1000> iWorkBuf;

	CJuikSizerContainer* iContainer;	
};



class CCalendarControl : public CPresenceControlBase, public MSettingListener
{
public:
	static CCalendarControl* NewL(CCoeControl& aParent, const TUiDelegates& aDelegates, TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CCalendarControl"), _CL("NewL"));		
		auto_ptr<CCalendarControl> self( new (ELeave) CCalendarControl( aDelegates, aFlags ) );
		self->SetContainerWindowL( aParent );
		self->ConstructL();
		return self.release();
	}

	virtual CPresenceList::TItemId ItemId() const { return CPresenceList::ECalendarItem; }

	enum {
		EStaticIcon, 
		ERightHandSizer,
		ECurrentEvent,
		EBuddyIcon
	};

	CCalendarControl(const TUiDelegates& aDelegates, TInt aFlags) : CPresenceControlBase( aDelegates, aFlags ) {}
	
	virtual ~CCalendarControl()
	{
		CALLSTACKITEM_N(_CL("CCalendarControl"), _CL("~CCalendarControl"));		
		if ( iFlags & CPresenceControlBase::EMyOwnPresence )
			{
				Settings().CancelNotifyOnChange(SETTING_CALENDAR_SHARING, this);
			}

	}
	
	TInt iRunningId;
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CCalendarControl"), _CL("ConstructL"));		
		BaseConstructL();
		iRunningId = 100;
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );				 
			Content().SetRootSizerL( sizer );
		}
		
		{

			TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_presence_list, 
														  LI_presence_list__icon);
			TSize sz = lay.Size();
			MStaticIconProvider* provider = IconManager().GetStaticIconProviderL( StaticIcons::ContextContactsUiIconFile() );
			CGulIcon* icon = provider->GetIconL( EMbmContextcontactsuiCalendar );
			auto_ptr<CJuikImage> image( CJuikImage::NewL( icon, sz ) );
			image->iMargin = Layout().GetLayoutItemL(LG_presence_list, LI_presence_list__icon_margins).Margins();
			Content().AddControlL( image.get(), EStaticIcon );
			Content().GetRootSizerL()->AddL( *(image.release()), 0, Juik::EExpand ); 
		}
		
		{
			
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			Content().AddSizerL( sizer, ERightHandSizer );
			Content().GetRootSizerL()->AddL( *sizer, 1, Juik::EExpand ); 
		}
		
		{
		if ( iFlags & CPresenceControlBase::EMyOwnPresence )
			{
				iSharing = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
											 JuikFonts::Small(), Content() );
				Content().AddControlL( iSharing, iRunningId++ );
				Content().GetSizerL( ERightHandSizer )->AddL( *iSharing, 0, Juik::EExpand ); 			
			}
		
			iNext = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
									  JuikFonts::Tiny(), Content() );
			iNext->SetDebugId( 3331 );
			Content().AddControlL( iNext, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iNext, 0, Juik::EExpand ); 			
			
			iCurrent = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
										 JuikFonts::Small(), Content() );
			iCurrent->SetDebugId( 3332 );
			Content().AddControlL( iCurrent, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iCurrent, 0, Juik::EExpand ); 			
			
			iPrevious = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
										  JuikFonts::Tiny(), Content() );
			iPrevious->SetDebugId( 3333 );
			Content().AddControlL( iPrevious, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iPrevious, 0, Juik::EExpand );
		}

		if ( iFlags & CPresenceControlBase::EMyOwnPresence )
			{
				UpdateSharingCuesL();
				Settings().NotifyOnChange(SETTING_CALENDAR_SHARING, this);
			}
	}
	
	void UpdateSharingCuesL()
	{
		CALLSTACKITEM_N(_CL("CCalendarControl"), _CL("UpdateSharingCuesL"));		
		if ( iFlags & CPresenceControlBase::EMyOwnPresence )
			{
				TInt sharing = SHARE_CALENDAR_FULL;
				Settings().GetSettingL(SETTING_CALENDAR_SHARING, sharing);
				
				iWorkBuf.Zero();
				Calendar::SharedText( sharing, iWorkBuf );
				iSharing->UpdateTextL( iWorkBuf );
			}
	}
	
	void SettingChanged(TInt aSetting)
	{
		if ( aSetting == SETTING_CALENDAR_SHARING )
			{
				CALLSTACKITEM_N(_CL("CCalendarControl"), _CL("SettingChanged"));		
				UpdateSharingCuesL();
				SizeChanged();
				DrawDeferred();
			}
	}
	

   	void UpdateL(CBBPresence* aP) 
 	{ 
		CALLSTACKITEM_N(_CL("CCalendarControl"), _CL("UpdateL"));		
		if ( aP ) 
			{
				const TBBCalendar& cal = aP->iCalendar;
				
				{
					iWorkBuf.Zero();
					const TBBCalendarEvent& event = cal.iNext;
					if ( event.iDescription().Length() ) Calendar::NextTextL( iWorkBuf );
					Calendar::TimeAndDescriptionL( event, iWorkBuf);
					if ( iWorkBuf.Length() )
						iNext->UpdateTextL( iWorkBuf );
					else
						iNext->ZeroL();

				}

				{
					iWorkBuf.Zero();
					const TBBCalendarEvent& event = cal.iCurrent;
					Calendar::TimeAndDescriptionL( event, iWorkBuf, EFalse);
					if ( iWorkBuf.Length() )
						iCurrent->UpdateTextL( iWorkBuf );
					else
						iCurrent->ZeroL();

				}


				{
					iWorkBuf.Zero();
					const TBBCalendarEvent& event = cal.iPrevious;
					if ( event.iDescription().Length() ) Calendar::PreviousTextL( iWorkBuf );
					Calendar::TimeAndDescriptionL( event, iWorkBuf );
					if ( iWorkBuf.Length() )
						iPrevious->UpdateTextL( iWorkBuf );
					else
						iPrevious->ZeroL();
				}

			}
		else
			{
				iPrevious->ZeroL();
				iCurrent->ZeroL();
				iNext->ZeroL();
			}
		//MinimumSizeChangedL();
	}
	CJuikLabel* iSharing;
	CJuikLabel* iPrevious;
	CJuikLabel* iCurrent;
	CJuikLabel* iNext;
};


class CLocationControl : public CPresenceControlBase
{
public:
	static CLocationControl* NewL(CCoeControl& aParent, const TUiDelegates& aDelegates, TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CLocationControl"), _CL("NewL"));		
		auto_ptr<CLocationControl> self( new (ELeave) CLocationControl( aDelegates, aFlags ) );
		self->SetContainerWindowL( aParent );
		self->ConstructL();
		return self.release();
	}
	
	virtual CPresenceList::TItemId ItemId() const { return CPresenceList::ELocationItem; }

	enum {
		ERightHandSizer,
	};

	CLocationControl(const TUiDelegates& aDelegates, TInt aFlags) : CPresenceControlBase( aDelegates, aFlags ) {}
	
	virtual ~CLocationControl()
	{
	}
	
	TInt iRunningId;
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CLocationControl"), _CL("ConstructL"));
		BaseConstructL();
		iRunningId = 100;
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );				 
			Content().SetRootSizerL( sizer );
		}
		
		{
			TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_presence_list, 
														  LI_presence_list__icon);
			TSize sz = lay.Size();
			MStaticIconProvider* provider = IconManager().GetStaticIconProviderL( StaticIcons::ContextContactsUiIconFile() );
			CGulIcon* icon = provider->GetIconL( EMbmContextcontactsuiEarth );
			auto_ptr<CJuikImage> image( CJuikImage::NewL( icon, sz ) );
			image->iMargin = Layout().GetLayoutItemL(LG_presence_list, LI_presence_list__icon_margins).Margins();
			Content().AddControlL( image.get(), iRunningId++ );
			Content().GetRootSizerL()->AddL( *(image.release()), 0, Juik::EExpandNot ); 
		}
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			Content().AddSizerL( sizer, ERightHandSizer );
			Content().GetRootSizerL()->AddL( *sizer, 1, Juik::EExpand ); 
		}
		
		{
			
			iCurrent = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
										 JuikFonts::Small(), Content() );
			Content().AddControlL( iCurrent, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iCurrent, 0, Juik::EExpand ); 			

			iCurrentTimestamp = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
												  JuikFonts::Tiny(), Content() );
			Content().AddControlL( iCurrentTimestamp, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iCurrentTimestamp, 0, Juik::EExpand ); 			


			iPrevious = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
										  JuikFonts::Tiny(), Content() );
			Content().AddControlL( iPrevious, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iPrevious, 0, Juik::EExpand ); 			

			iPreviousTimestamp = CJuikLabel::NewL( SecondaryTextColor(), SecondaryHighlightTextColor(),
												   JuikFonts::Tiny(), Content() );
			Content().AddControlL( iPreviousTimestamp, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iPreviousTimestamp, 0, Juik::EExpand ); 			

			

		}
	}

   	void UpdateL(CBBPresence* aP) 
 	{ 
		CALLSTACKITEM_N(_CL("CLocationControl"), _CL("UpdateL"));		
// 		iPrevious->ZeroL();
// 		iPreviousTimestamp->ZeroL();
// 		iCurrent->ZeroL();
// 		iCurrentTimestamp->ZeroL();
		if ( aP ) 
			{
				TBool hasCurrent = EFalse;
				{
					iWorkBuf.Zero();
					Location::CurrentL( aP, iWorkBuf);
					if ( iWorkBuf.Length() )
						{
							iCurrent->UpdateTextL( iWorkBuf );
							hasCurrent = ETrue;
						}
					else
						iCurrent->ZeroL();
 				}

				if ( hasCurrent )
					{
						iWorkBuf.Zero();
						Location::TimeStamp(*aP, *(iDelegates.iPeriodFormatter), iWorkBuf);
						if ( iWorkBuf.Length() )
							iCurrentTimestamp->UpdateTextL( iWorkBuf );
						else
							iCurrentTimestamp->ZeroL();

					}

				
				TBool hasPrevious = EFalse;
				{
					iWorkBuf.Zero();
					Location::PreviousL( aP, iWorkBuf );
					if ( iWorkBuf.Length() )
						{
							TBuf<100> prev;
							Calendar::PreviousTextL( prev );
							iWorkBuf.Insert( 0, prev );

							//iWorkBuf.Zero();
							iWorkBuf.Append( _L(" ") );
							Location::PreviousTimeStamp(*aP, *(iDelegates.iPeriodFormatter), iWorkBuf);
						}
					if ( iWorkBuf.Length() )
						iPrevious->UpdateTextL( iWorkBuf );
					else
						iPrevious->ZeroL();


					TBool hasPrevious = EFalse; //ETrue;
				}

				if ( hasPrevious )
					{
						iWorkBuf.Zero();
						Location::PreviousTimeStamp(*aP, *(iDelegates.iPeriodFormatter), iWorkBuf);
						if ( iWorkBuf.Length() )
							iPreviousTimestamp->UpdateTextL( iWorkBuf );
						else
							iPreviousTimestamp->ZeroL();
					}

				
			}
		//MinimumSizeChangedL();
	}
	
	CJuikLabel* iPrevious;
	CJuikLabel* iPreviousTimestamp;
	CJuikLabel* iCurrent;
	CJuikLabel* iCurrentTimestamp;

};


class CNearbyControl : public CPresenceControlBase, public MSettingListener
{
public:
	static CNearbyControl* NewL(CCoeControl& aParent, const TUiDelegates& aDelegates, TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CNearbyControl"), _CL("NewL"));		
		auto_ptr<CNearbyControl> self( new (ELeave) CNearbyControl( aDelegates, aFlags ) );
		self->SetContainerWindowL( aParent );
		self->ConstructL();
		return self.release();
	}
	
	virtual CPresenceList::TItemId ItemId() const { return CPresenceList::ENearbyItem; }

	enum {
		ERightHandSizer,
		ESecondRow
	};

	CNearbyControl(const TUiDelegates& aDelegates, TInt aFlags) : CPresenceControlBase( aDelegates, aFlags ) { }
	
	virtual ~CNearbyControl()
	{
		CALLSTACKITEM_N(_CL("CNearbyControl"), _CL("~CNearbyControl"));		
		if ( iFlags & CPresenceControlBase::EMyOwnPresence )
			{
				Settings().CancelNotifyOnChange(SETTING_BT_SCAN_INTERVAL, this);
			}
	}
	
	TInt iRunningId;
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CNearbyControl"), _CL("ConstructL"));		
		BaseConstructL();
		iRunningId = 100;
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );				 
			Content().SetRootSizerL( sizer );
		}
		
		{
			TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_presence_list, 
														  LI_presence_list__icon);
			TSize sz = lay.Size();
			MStaticIconProvider* provider = IconManager().GetStaticIconProviderL( StaticIcons::ContextContactsUiIconFile() );
			CGulIcon* icon = provider->GetIconL( EMbmContextcontactsuiFriends );
			auto_ptr<CJuikImage> image( CJuikImage::NewL( icon, sz ) );
			image->iMargin = Layout().GetLayoutItemL(LG_presence_list, LI_presence_list__icon_margins).Margins();
			Content().AddControlL( image.get(), iRunningId++ );
			Content().GetRootSizerL()->AddL( *(image.release()), 0, Juik::EExpandNot ); 
		}
		
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			Content().AddSizerL( sizer, ERightHandSizer );
			Content().GetRootSizerL()->AddL( *sizer, 1, Juik::EExpand ); 
		}
		
		{
			iHeader = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
										JuikFonts::Small(), Content() );
			Content().AddControlL( iHeader, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iHeader, 0, Juik::EExpand ); 			
		}

		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );				 
			Content().AddSizerL( sizer, ESecondRow );
			Content().GetSizerL( ERightHandSizer )->AddL( *sizer, 0, Juik::EExpand ); 
		}

		{
			
			iFriends = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
										  JuikFonts::Small(), Content() );
			Content().AddControlL( iFriends, iRunningId++ );
			Content().GetSizerL( ESecondRow )->AddL( *iFriends, 0, Juik::EExpand ); 			
			
			iOthers = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
										 JuikFonts::Small(), Content() );
			iOthers->iMargin = Juik::Margins(10, 0, 0, 0);
			Content().AddControlL( iOthers, iRunningId++ );
			Content().GetSizerL( ESecondRow )->AddL( *iOthers, 0, Juik::EExpand ); 			
		}
		
		if ( iFlags & CPresenceControlBase::EMyOwnPresence )
			{
				UpdateSharingCuesL();
				Settings().NotifyOnChange(SETTING_BT_SCAN_INTERVAL, this);
				
			}
	}

	TBool iShared;
	void UpdateSharingCuesL()
	{
		CALLSTACKITEM_N(_CL("CNearbyControl"), _CL("UpdateSharingCuesL"));
		if ( iFlags & CPresenceControlBase::EMyOwnPresence )
			{
				TInt interval = 0;
				Settings().GetSettingL(SETTING_BT_SCAN_INTERVAL, interval);
				iShared = interval > 0;

				if ( ! iShared )
					{
						iWorkBuf.Zero();
						Nearby::PeopleNearbyText( iWorkBuf );
						iHeader->UpdateTextL( iWorkBuf );
						
						iFriends->ZeroL();
						iWorkBuf.Zero();
						Nearby::NotSharedText( iWorkBuf );
						iFriends->UpdateTextL( iWorkBuf );
						
						iOthers->ZeroL();
					}
				else
					{
						iFriends->UpdateTextL( _L("") );
						iOthers->UpdateTextL( _L("") );
					}
			}
	}

	void SettingChanged(TInt aSetting)
	{
		if ( aSetting == SETTING_BT_SCAN_INTERVAL)
			{
				CALLSTACKITEM_N(_CL("CNearbyControl"), _CL("SettingChanged"));
				UpdateSharingCuesL();
				SizeChanged();
				DrawDeferred();
			}
	}

   	void UpdateL(CBBPresence* aP) 
 	{ 
		CALLSTACKITEM_N(_CL("CNearbyControl"), _CL("UpdateL"));
		if ( iFlags & CPresenceControlBase::EMyOwnPresence && ! iShared )
			return;
		
		
		iFriends->ZeroL();
		iOthers->ZeroL();
		if ( aP ) 
			{
				
				TBool hasData = EFalse;
				
				{
					iWorkBuf.Zero();
					Nearby::Friends(*aP, iWorkBuf, EFalse);
					if ( iWorkBuf.Length() )
						{
							hasData = ETrue;
							iFriends->UpdateTextL( iWorkBuf );
						}
				}
				
				{
					iWorkBuf.Zero();
					Nearby::Others(*aP, iWorkBuf, EFalse);
					if ( iWorkBuf.Length() )
						{
							hasData = ETrue;
							iOthers->UpdateTextL( iWorkBuf );
						}
				}
				
				if ( hasData )
					{
						iWorkBuf.Zero();
						Nearby::PeopleNearbyText( iWorkBuf );
						iHeader->UpdateTextL( iWorkBuf );
					}
				
			}
		//MinimumSizeChangedL();
	}
		
	
	CJuikLabel* iHeader;
	CJuikLabel* iFriends;
	CJuikLabel* iOthers;
};


class CProfileControl : public CPresenceControlBase
{
public:
	static CProfileControl* NewL(CCoeControl& aParent, const TUiDelegates& aDelegates, TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CProfileControl"), _CL("NewL"));
		auto_ptr<CProfileControl> self( new (ELeave) CProfileControl( aDelegates, aFlags ) );
		self->SetContainerWindowL( aParent );
		self->ConstructL();
		return self.release();
	}
	
	virtual CPresenceList::TItemId ItemId() const { return CPresenceList::EProfileItem; }

	enum {
		EStaticIcon, 
		ERightHandSizer,
		ENameRow,
	};

	CProfileControl(const TUiDelegates& aDelegates, TInt aFlags) : CPresenceControlBase( aDelegates, aFlags ) {}
	
	virtual ~CProfileControl()
	{
		CALLSTACKITEM_N(_CL("CProfileControl"), _CL("~CProfileControl"));
		delete iBuddyIconMgr;
		delete iTrafficLightMgr;
	}
	
	TInt iRunningId;
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CProfileControl"), _CL("ConstructL"));
		BaseConstructL();
		iRunningId = 100;
		{
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );				 
			Content().SetRootSizerL( sizer );
		}
		CJabberData::TNick nick;
		iDelegates.iActiveState->ActiveContact().GetNickL( nick );
			
		{
 			iBuddyIconMgr = CBuddyIconMgr::NewL(nick, iDelegates);
 			CJuikImage* image = iBuddyIconMgr->GetControl();
 			Content().AddControlL( image, iRunningId++ );
 			Content().GetRootSizerL()->AddL( *image, 0, Juik::EExpandNot ); 
		}

		{
			
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
			Content().AddSizerL( sizer, ERightHandSizer );
			Content().GetRootSizerL()->AddL( *sizer, 1, Juik::EExpand ); 
		}


		{
			
			MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EHorizontal );				 
			Content().AddSizerL( sizer, ENameRow );
			Content().GetSizerL(ERightHandSizer)->AddL( *sizer, 1, Juik::EExpand ); 
		}


		
		{
			iName = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
									  JuikFonts::Normal(), Content() );
			Content().AddControlL( iName, iRunningId++ );
			Content().GetSizerL( ENameRow )->AddL( *iName, 1, Juik::EExpandNot );
			CJabberData::TransformToUiNickL( nick );
			iName->UpdateTextL( nick );
		}

		{
 			iTrafficLightMgr = CTrafficLightMgr::NewL(nick, iDelegates);
 			CJuikImage* image = iTrafficLightMgr->GetControl();
 			Content().AddControlL( image, iRunningId++ );
			Content().GetSizerL( ENameRow )->AddL( *image, 0, Juik::EExpandNot  ); 
		}
		
		{
			iLastUsed = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
										  JuikFonts::Small(), Content() );
			Content().AddControlL( iLastUsed, iRunningId++ );
			Content().GetSizerL( ERightHandSizer )->AddL( *iLastUsed, 0, Juik::EExpand ); 			
		}
	}

   	void UpdateL(CBBPresence* aP) 
 	{ 
		CALLSTACKITEM_N(_CL("CProfileControl"), _CL("UpdateL"));
		iLastUsed->ZeroL();
		if ( aP ) 
			{
				iWorkBuf.Zero();
				UserActivity::ActivityOrLastUse( *aP, *(iDelegates.iPeriodFormatter), iWorkBuf);
				if ( iWorkBuf.Length() )
					iLastUsed->UpdateTextL( iWorkBuf );
			}
		//MinimumSizeChangedL();
	}
	
	
	CJuikLabel* iName;
	CJuikLabel* iLastUsed;
	CBuddyIconMgr* iBuddyIconMgr;
	CTrafficLightMgr* iTrafficLightMgr;
};




class CPresenceListImpl : public CPresenceList, public MContextBase, public MActiveContactListener
{
public:
	static CPresenceListImpl* CPresenceListImpl::NewL(CCoeControl& aParent, const TUiDelegates& aDelegates)
	{
		CALLSTACKITEMSTATIC_N(_CL("CPresenceListImpl"), _CL("NewL"));
		auto_ptr<CPresenceListImpl> self( new (ELeave)  CPresenceListImpl(aDelegates) );
		self->ConstructL(aParent);
		return self.release();
	}
	
	CPresenceListImpl(const TUiDelegates& aDelegates) : iDelegates( aDelegates ) {}
   
	virtual ~CPresenceListImpl()
	{
		CALLSTACKITEM_N(_CL("CPresenceListImpl"), _CL("~CPresenceListImpl"));
		iDelegates.iActiveState->ActiveContact().RemoveListenerL( *this );
		for( TInt i=0; i < iList->ItemCount(); i++)
			delete iList->At(i);
		delete iList;
	}
	
	enum 
		{
			EProfileIndex = 0,
			ELocationIndex,
			ENearbyIndex,
			ECalendarIndex
		};

 	virtual TItemId ItemId(TInt aIndex) const 
	{
		if ( iList )
			{
				MJuikControl* ctrl = iList->At(aIndex);
				if ( ctrl )
					{
						CPresenceControlBase* c = static_cast<CPresenceControlBase*>( ctrl );
						return c->ItemId();
					}
			}
		return CPresenceList::ENone;
	}

	virtual TItemId CurrentItemId() const 
	{
		TInt ix = iList->CurrentItemIndex();
		return ItemId( ix );
	}
	
	void ConstructL(CCoeControl& aParent)
	{
		CALLSTACKITEM_N(_CL("CPresenceListImpl"), _CL("ConstructL"));
		const TInt KListFlags = CSoftScrollList::EListChangesFocus;
		iList = CSoftScrollList::NewL( &aParent, KListFlags ); 

		contact* con = iDelegates.iActiveState->ActiveContact().GetL();
		TBool isMe = con && con->is_myself;
		iControlFlags = isMe ? CPresenceControlBase::EMyOwnPresence : 0;

// 		auto_ptr<CCalendarControl> jaikupost( CCalendarControl::NewL(aParent, iDelegates) );
// 		iList->AddChildL( jaikupost.release() );

		UpdateL();

		if ( isMe ) 
			iList->SetCurrentItem(ELocationIndex);
		else
			iList->SetCurrentItem(0);
		
		iDelegates.iActiveState->ActiveContact().AddListenerL( *this );
	}


	CSoftScrollList* List() const { return iList; }

private:
	CPresenceControlBase* FindById(CPresenceList::TItemId aId)
	{
		CALLSTACKITEM_N(_CL("CPresenceListImpl"), _CL("FindById"));
		for ( TInt i=0; i < iList->ItemCount(); i++ )
			{
				MJuikControl* ctrl = iList->At(i);
				if ( ctrl )
					{
						CPresenceControlBase* c = static_cast<CPresenceControlBase*>( ctrl );
						if ( c->ItemId() == aId )
							return c;
					}
			}
		return NULL;
	}

	CPresenceControlBase* CreateControlL( CPresenceList::TItemId aId )
	{
		CALLSTACKITEM_N(_CL("CPresenceListImpl"), _CL("CreateControlL"));
		switch ( aId )
			{
			case CPresenceList::EProfileItem: 
				return CProfileControl::NewL(*iList, iDelegates, iControlFlags);
				
			case CPresenceList::ELocationItem: 
				return CLocationControl::NewL(*iList, iDelegates, iControlFlags);
				
			case CPresenceList::ENearbyItem: 
				return CNearbyControl::NewL(*iList, iDelegates, iControlFlags);
				
 			case CPresenceList::ECalendarItem: 
				return CCalendarControl::NewL(*iList, iDelegates, iControlFlags);
			} 
		Bug( _L("Presence control not implemented") ).Raise();
		return NULL;
	}


	TBool ShowOrHideItemL( CPresenceList::TItemId aId, TInt aPos, TBool aHide, CBBPresence* aPresence)
	{
		CALLSTACKITEM_N(_CL("CPresenceListImpl"), _CL("ShowOrHideItemL"));
		const TBool own = iControlFlags & CPresenceControlBase::EMyOwnPresence; // always show all for own
		TBool hide = !own && aHide;

		CPresenceControlBase* c = FindById( aId );
		if ( hide )
			{
				if ( c )
					iList->PopChildL( aPos );
				return EFalse;
			}
		else 
			{
				if ( !c ) 
					{
						auto_ptr<CPresenceControlBase> ctrl( CreateControlL( aId ) );
						c = ctrl.get();
						iList->InsertChildL( aPos, ctrl.release() );
					}
				c->UpdateL( aPresence );	   
				return ETrue;
			}
	}
	
	
	void UpdateL( CBBPresence* aPresence )
	{
		CALLSTACKITEM_N(_CL("CPresenceListImpl"), _CL("UpdateL(Presence)"));
		
		TInt runningPos = 0;

		TBool hide = EFalse; // always show profile
		TBool isShown = ShowOrHideItemL( CPresenceList::EProfileItem, runningPos, hide, aPresence );
		runningPos += isShown ? 1 : 0;

		hide = Location::IsMissing( aPresence );
		isShown = ShowOrHideItemL( CPresenceList::ELocationItem, runningPos, hide, aPresence );
		runningPos += isShown ? 1 : 0;

		hide = Nearby::IsMissing( aPresence ); 
		isShown = ShowOrHideItemL( CPresenceList::ENearbyItem, runningPos, hide, aPresence );
		runningPos += isShown ? 1 : 0;

		hide = Calendar::IsMissing( aPresence ); 
		isShown = ShowOrHideItemL( CPresenceList::ECalendarItem, runningPos, hide, aPresence );
		runningPos += isShown ? 1 : 0;
	}

	void UpdateL()
	{
		CALLSTACKITEM_N(_CL("CPresenceListImpl"), _CL("UpdateL()"));
		contact* c = iDelegates.iActiveState->ActiveContact().GetL();
		if ( c ) 
			UpdateL( c->presence );
		else
			UpdateL( NULL );
	}

private: // from MActiveContactListener
	virtual void ActiveContactChanged( TChangeType aChangeType ) 
	{
		CALLSTACKITEM_N(_CL("CPresenceListImpl"), _CL("ActiveContactChanged"));
		if ( aChangeType == EDataUpdated )
			{
				UpdateL();
				List()->ReLayoutL();
				List()->DrawDeferred();
			}
	}

private:
	TUiDelegates iDelegates;

	CSoftScrollList* iList;

	TInt iControlFlags;
};


EXPORT_C CPresenceList* CPresenceList::NewL(CCoeControl& aParent, const TUiDelegates& aDelegates)
{
	CALLSTACKITEM_N(_CL("CPresenceList"), _CL("NewL"));
	return CPresenceListImpl::NewL( aParent, aDelegates );
}
