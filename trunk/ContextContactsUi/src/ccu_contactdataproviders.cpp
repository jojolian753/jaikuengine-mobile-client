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

#include "ccu_contactdataproviders.h"

#include "ccu_contact.h"
#include "ccu_userpics.h"
#include "ccu_presencestatus.h"
#include "ccu_timeperiod.h"
#include "ccu_staticicons.h"
#include "ccu_presencestatus.h"
#include "ccu_storage.h"
#include "ccu_streamstatscacher.h"
#include "ccu_themes.h"
#include "PresenceTextFormatter.h"
#include "phonebook.h"

#include <contextcontactsui.mbg>

#include "break.h"
#include "cl_settings.h"
#include "csd_calendar.h"
#include "juik_icons.h"
#include "juik_iconmanager.h"
#include "juik_layout.h"
#include "jaiku_layoutids.hrh"
#include "icons.h"
#include "settings.h"

#include <akniconarray.h>
#include <akniconutils.h>

#include "scalableui_variant.h"

static const TInt KBuddyProviderIconCount(1);
static const TIconID KBuddyProviderIconIds[1]= {
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiDummybuddy,
					EMbmContextcontactsuiDummybuddy_mask )
};


static const TInt KStatusProviderIconCount(4);
static const TIconID KStatusProviderIconIds[4]= {
#ifdef __SCALABLEUI_VARIANT__
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiLight_green,  EMbmContextcontactsuiLight_green_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiLight_red,	   EMbmContextcontactsuiLight_red_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
				    EMbmContextcontactsuiLight_yellow, EMbmContextcontactsuiLight_yellow_mask ),
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiLight_gray,   EMbmContextcontactsuiLight_gray_mask ),
#else 
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiLight_green,  EMbmContextcontactsuiLight_mask ), 
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiLight_red,	    EMbmContextcontactsuiLight_mask ), 
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
				    EMbmContextcontactsuiLight_yellow,  EMbmContextcontactsuiLight_mask ), 
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiLight_gray,    EMbmContextcontactsuiLight_mask ), 
#endif // __SCALABLEUI_VARIANT__
};

static const TInt KUnreadProviderIconCount(1);
static const TIconID KUnreadProviderIconIds[KUnreadProviderIconCount]= {
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiIcon_unread,
					EMbmContextcontactsuiIcon_unread_mask )
};




//
// Aargh. I hate C++ without closures. Following classes are mainly single simple functions
// pretending to be closures
//  
namespace ContactDataProviders 
{
		
	class CContactDataProviderBase : public CBase, public MContextBase, public MContactDataProvider 
	{
		CArrayPtr<CGulIcon>* GetIconsL()
		{
			return NULL;
		}
	};


	class CContactIconDataProvider : public CContactDataProviderBase
	{
	public:
		CContactIconDataProvider(MJuikIconManager& aIconManager) : iIconManager(aIconManager) {}

		~CContactIconDataProvider()
		{
			delete iIconFile;
			iIconIds.Close();
		}

		void BaseConstructL(const TDesC& aFile, TInt aIconId)
		{
			iIconFile = aFile.AllocL();
			iIconIds.AppendL( aIconId );
		}
		
		void BaseConstructL(const TIconID* aIconDefs, TInt aIconNb)
		{
			TPtrC fileName( (TText*) aIconDefs[0].iMbmFile );
			iIconFile = fileName.AllocL();
			for (TInt i=0; i < aIconNb; i++)
				{
					iIconIds.AppendL( aIconDefs[i].iBitmap );
				}
		}

		CArrayPtr<CGulIcon>* GetIconsL()
		{
			auto_ptr< CArrayPtr<CGulIcon> > icons( new (ELeave) CArrayPtrFlat<CGulIcon>(10));
			MStaticIconProvider* provider = iIconManager.GetStaticIconProviderL( *iIconFile );
			for (TInt i=0; i < iIconIds.Count(); i++)
				{
					CGulIcon* icon = provider->GetIconL( iIconIds[i] );
					icons->AppendL( icon );
				}
			return icons.release();
		}

		TInt GetIconIndexL(const TDesC& aFile, TInt aIconId) const
		{
			return iIconManager.GetStaticIconProviderL( aFile )->GetListBoxIndexL( aIconId );
		}

		
		MJuikIconManager& iIconManager;
		HBufC* iIconFile;
		RArray<TInt> iIconIds;
	};

	class CNameP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon ) aCon->AppendName(aBuf, iLastNameFirst);
		}
		
	public:
		static CNameP* NewL(TBool aLastNameFirst) 
		{
			return new (ELeave) CNameP(aLastNameFirst);
		}

	private:
		CNameP(TBool aLastNameFirst) : iLastNameFirst(aLastNameFirst) {}
		TBool iLastNameFirst;
	};
	
	class CExtraNameP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon ) aBuf.Append( aCon->ExtraName() );
		}
	};
	
	
	
	class CPresenceAndLocationP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon ) iFormatter->LongTextL(aCon->presence, aBuf);
		}
		
	public:
		static CPresenceAndLocationP* NewL(CTimePeriodFormatter& aPeriodFormatter)
		{
			auto_ptr<CPresenceAndLocationP> self( new (ELeave) CPresenceAndLocationP() );
			self->ConstructL(aPeriodFormatter);
			return self.release();
		}
		
		~CPresenceAndLocationP() 
		{
			delete iFormatter;
		};
		
	private:
		CPresenceAndLocationP() {};
		void ConstructL(CTimePeriodFormatter& aPeriodFormatter)
		{
			iFormatter = CPresenceTextFormatter::NewL(aPeriodFormatter);
		}
		
		CPresenceTextFormatter* iFormatter;	
	};


	class CEmptyP : public CContactDataProviderBase					
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{		
		}
	};


	class CBuddyIconP : public CContactIconDataProvider
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{	
			if ( aCon && aCon->has_nick )
				{
					TInt ix = iUserPics.GetIconIndexL( aCon->id );			
					if ( ix == KErrNotFound )
						{
							ix = DummyIconIndexL();
						}
					TBuf<4>icon;
					icon.Format(_L("%03d"), ix );
					aBuf.Append(icon);
				}
		}
		
	public: 
		static CBuddyIconP* NewL(MJuikIconManager& aIconManager, CUserPics& aUserPics)
		{
			auto_ptr<CBuddyIconP> self( new (ELeave) CBuddyIconP(aIconManager, aUserPics) );
			self->ConstructL();
			return self.release();
		}
		
		CBuddyIconP( MJuikIconManager& aIconManager, CUserPics& aUserPics ) : 
			CContactIconDataProvider(aIconManager), 
			iUserPics(aUserPics) {}
		
		void ConstructL( )
		{
			BaseConstructL(KBuddyProviderIconIds, KBuddyProviderIconCount);
		}
		
		TInt DummyIconIndexL() const
		{
			return GetIconIndexL( StaticIcons::ContextContactsUiIconFile(), EMbmContextcontactsuiDummybuddy );
		}
		
	private:
		CUserPics& iUserPics;
	};


	class CStatusIconP : public CContactIconDataProvider, public MSettingListener
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{	
			
			TInt ix = KErrNotFound; 
			if ( aCon )
				{
					ix = PresenceIconIndex( PresenceStatusL( *aCon, iPresenceEnabled ) );
				}
			
			if ( ix >= 0 )
				{
					TBuf<4> icon;
					icon.Format( _L("%02d"), ix );
					aBuf.Append( icon );
				}			
		}
		
	public: 
		static CStatusIconP* NewL(MJuikIconManager& aIconManager)
		{
			auto_ptr<CStatusIconP> self( new (ELeave) CStatusIconP(aIconManager) );
			self->ConstructL();
			return self.release();
		}
		
		CStatusIconP( MJuikIconManager& aIconManager ) : 
			CContactIconDataProvider(aIconManager) {}
		

		~CStatusIconP() 
		{
			Settings().CancelNotifyOnChange( SETTING_PRESENCE_ENABLE, this );
		}

		void ConstructL( )
		{
			BaseConstructL( KStatusProviderIconIds,  KStatusProviderIconCount);
			
			Settings().GetSettingL( SETTING_PRESENCE_ENABLE, iPresenceEnabled );
			Settings().NotifyOnChange( SETTING_PRESENCE_ENABLE, this );
		}

		
		TInt IconIndex(TInt aIconName) const
		{
			return GetIconIndexL( StaticIcons::ContextContactsUiIconFile(), aIconName );
		}
		
		TInt PresenceIconIndex(TPresenceStatus aStatus)
		{
			switch ( aStatus )
				{
				case EPresenceNone: 
					return KErrNotFound;
				case EPresenceGray:
					return IconIndex( EMbmContextcontactsuiLight_gray );
				case EPresenceRed: 
					return IconIndex( EMbmContextcontactsuiLight_red );
				case EPresenceYellow:
					return IconIndex( EMbmContextcontactsuiLight_yellow );
				case EPresenceGreen: 
					return IconIndex( EMbmContextcontactsuiLight_green );
				default: 
					// FIXME: throw an error or warning
					return KErrNotFound;
				}
		}

		// From: 
		void SettingChanged(TInt aSetting) 
		{
			if ( aSetting == SETTING_PRESENCE_ENABLE )
				{
					Settings().GetSettingL( SETTING_PRESENCE_ENABLE, iPresenceEnabled );
				}
		}
		
		TBool IsPresenceEnabled() const
		{
			return iPresenceEnabled;
		}
		
		
	private:
		TBool iPresenceEnabled;
	};



	class CMarkerP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{			
		}
	};

	class CUserActivityP : public CContactDataProviderBase
	{
	public:
		CTimePeriodFormatter* iPeriodFormatter;

		static CUserActivityP* NewL()
		{
			auto_ptr<CUserActivityP> self( new (ELeave) CUserActivityP);
			self->ConstructL();
			return self.release();
		}
		
		~CUserActivityP()
		{
			delete iPeriodFormatter;
		}
		
		void ConstructL()
		{
			iPeriodFormatter = CTimePeriodFormatter::NewL();
		}

		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence )
				{			
					CBBPresence* p = aCon->presence;
					UserActivity::ActivityOrLastUse( *p, *iPeriodFormatter, aBuf );
				}
		}
	};
	

	class CPresenceLineP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence ) aBuf.Append( aCon->presence->iUserGiven.iDescription() );
		}
	};


	class CPresenceLineTStampP : public CContactDataProviderBase
	{
	public:
		CTimePeriodFormatter* iPeriodFormatter;

		static CPresenceLineTStampP* NewL()
		{
			auto_ptr<CPresenceLineTStampP> self( new (ELeave) CPresenceLineTStampP);
			self->ConstructL();
			return self.release();
		}
		
		~CPresenceLineTStampP()
		{
			delete iPeriodFormatter;
		}
		
		void ConstructL()
		{
			iPeriodFormatter = CTimePeriodFormatter::NewL();
		}
		
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence) 
				{
					CBBPresence* p = aCon->presence;
					TTime now = GetTime();					
					TTime presencelineTstamp = p->iUserGiven.iSince();					

					TTimePeriod period = TTimePeriod::BetweenL( presencelineTstamp, now );

					if ( IsOffline(*p, now ) )
						{
							iPeriodFormatter->AgoTextL( period, aBuf );
						}
					else
						{
							if ( period.iUnit == TTimePeriod::EJust ) 
								{
									aBuf.Append( _L("Just posted") );
								}
							else
								{
									iPeriodFormatter->SinceTextL( period, aBuf );
								}
						}
				}
		}
	};
	


	
	class CLocationP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon ) iFormatter->LocationL(aCon->presence, aBuf);
		}
		
	public:
		static CLocationP* NewL()
		{
		auto_ptr<CLocationP> self( new (ELeave) CLocationP );
		self->ConstructL();
		return self.release();
		}
		
		~CLocationP() 
		{
			delete iFormatter;
		};
		
	private:
		CLocationP() {};
		void ConstructL()
		{
			iFormatter = CPresenceTextFormatter::NewL();
		}
		
		CPresenceTextFormatter* iFormatter;	
	};
	


	class CLocationTStampP : public CContactDataProviderBase
	{
	public:
		CTimePeriodFormatter* iPeriodFormatter;

		static CLocationTStampP* NewL()
		{
			auto_ptr<CLocationTStampP> self( new (ELeave) CLocationTStampP);
			self->ConstructL();
			return self.release();
		}
		
		~CLocationTStampP()
		{
			delete iPeriodFormatter;
		}
		
		void ConstructL()
		{
			iPeriodFormatter = CTimePeriodFormatter::NewL();
		}
		
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence) 
				{
					CBBPresence* p = aCon->presence;
					Location::TimeStamp(*p, *iPeriodFormatter, aBuf);
				}
		}
	};
	
	
	class CCalendarTitleP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence)
				{
					Calendar::CurrentOrNextEventL( *(aCon->presence), aBuf );
				}
		}
	};
	
	
	class CCalendarDateTimeP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence)
				{
					// Current
					const TBBCalendar& cal = aCon->presence->iCalendar;
					const TBBCalendarEvent* event = Calendar::GetEvent( cal );
					if ( event )
						{
							Calendar::EventDateL( *event, aBuf );
							Calendar::EventTimeL( *event, aBuf );
						}
				}
		}
	};

		

	class CNearbyHeaderP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence ) aBuf.Append( _L("People Nearby") );
		}
	};

	class CNearbyPeopleP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence ) 
				{
					TInt friends = aCon->presence->iNeighbourhoodInfo.iBuddies();
					TInt others =   aCon->presence->iNeighbourhoodInfo.iOtherPhones();
					TBuf<4> num;
					num.Num(friends);
					aBuf.Append( num );
					aBuf.Append( _L(" friends, ") );

					num.Num(others);
					aBuf.Append( num );
					aBuf.Append( _L(" others") );
				}
		}
	};
	

	class CIdentityP : public CContactDataProviderBase
	{
	public:
		static CIdentityP* NewL(const TDesC& aMsg)
		{
			auto_ptr<CIdentityP> self( new (ELeave) CIdentityP );
			self->ConstructL( aMsg );
			return self.release();
		}

		~CIdentityP()
		{
			delete iMsg;
		}

		void ConstructL( const TDesC& aMsg )
		{
			iMsg = aMsg.AllocL();
		}
			
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			aBuf.Append( *iMsg );
		}

		HBufC* iMsg;
	};
	

	class CStaticIconP : public CContactIconDataProvider
	{
	public:
		static CStaticIconP* NewL(MJuikIconManager& aIconManager, const TDesC& aFile, TInt aIconId)
		{
			auto_ptr<CStaticIconP> self( new (ELeave) CStaticIconP(aIconManager) );
			self->BaseConstructL(aFile, aIconId);
			return self.release();
		}
		
		CStaticIconP( MJuikIconManager& aIconManager) : 
			CContactIconDataProvider(aIconManager) {}
		

		void SubcellDataL(contact* aCon, TDes& aBuf)
		{				
			TInt ix = GetIconIndexL( *iIconFile, iIconIds[0] );
			TBuf<4> icon;
			icon.Format( _L("%02d"), ix );
			aBuf.Append( icon );
		}
	};


	class CProfileNameP : public CContactDataProviderBase
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{
			if ( aCon && aCon->presence ) 
				{
					CBBPresence* p = aCon->presence;
					TTime now = GetTime();
					if ( IsOffline(*p, now) )
						{					
							_LIT( KOutOfDate, "Offline");
							aBuf.Append( KOutOfDate );
						}
					else
						{
							aBuf.Append( p->iProfile.iProfileName() );
						}
				}
		}
	};


	class CUnreadStreamIconP : public CContactIconDataProvider
	{
	public:
		void SubcellDataL(contact* aCon, TDes& aBuf)
		{	
			TInt ix = KErrNotFound; 
			
			if ( aCon && aCon->has_nick )
				{
					TInt unread = iDelegates.iStreamStats->UnreadCountL( *aCon );
					if ( unread > 0 )
						ix = IconIndex( EMbmContextcontactsuiIcon_unread );
				}
			
			if ( ix >= 0 )
				{
					TBuf<4> icon;
					icon.Format( _L("%02d"), ix );
					aBuf.Append( icon );
				}			
		}
		
		
	public: 
		static CUnreadStreamIconP* NewL(MJuikIconManager& aIconManager, TContactUiDelegates& aDelegates)
		{
			auto_ptr<CUnreadStreamIconP> self( new (ELeave) CUnreadStreamIconP(aIconManager, aDelegates) );
			self->ConstructL();
			return self.release();
		}
		

		CUnreadStreamIconP( MJuikIconManager& aIconManager, TContactUiDelegates& aDelegates ) : 
			CContactIconDataProvider(aIconManager), 
			iDelegates(aDelegates) {}
		
		
		~CUnreadStreamIconP() 
		{
		}
		
		void ConstructL( )
		{
			BaseConstructL( KUnreadProviderIconIds,  KUnreadProviderIconCount );
			TRgb c = iDelegates.iThemeColors->GetColorL( CThemeColors::EPrimaryText );			
			auto_ptr< CArrayPtr<CGulIcon> > icons = GetIconsL();
			AknIconUtils::SetIconColor( icons->At(0)->Bitmap(), c );
		}
		
		
		TInt IconIndex(TInt aIconName) const
		{
			return GetIconIndexL( StaticIcons::ContextContactsUiIconFile(), aIconName );
		}

		
		
	private:
		TContactUiDelegates iDelegates;
	};
		
	EXPORT_C MContactDataProvider* NameL(TBool aLastNameFirst) { return CNameP::NewL(aLastNameFirst); } 
	EXPORT_C MContactDataProvider* ExtraNameL() { return new (ELeave) CExtraNameP(); }
		
	EXPORT_C MContactDataProvider* PresenceLineL() { return new (ELeave) CPresenceLineP(); }
	EXPORT_C MContactDataProvider* EmptyL() { return new (ELeave) CEmptyP; }

	EXPORT_C MContactDataProvider* BuddyIconL(MJuikIconManager& aIconManager, CUserPics& aUserPics) 
	{ 
		return CBuddyIconP::NewL( aIconManager, aUserPics );
	}

	EXPORT_C MContactDataProvider* StatusIconL(MJuikIconManager& aIconManager) 
	{ 
		return CStatusIconP::NewL( aIconManager );
	}
	
	EXPORT_C MContactDataProvider* MarkerL() { return new (ELeave) CMarkerP; }


	EXPORT_C MContactDataProvider* UserActivityL() { return CUserActivityP::NewL(); }
	EXPORT_C MContactDataProvider* LocationL() { return CLocationP::NewL(); }
	EXPORT_C MContactDataProvider* LocationTStampL() { return CLocationTStampP::NewL(); }
	EXPORT_C MContactDataProvider* PresenceLineTStampL() { return CPresenceLineTStampP::NewL(); }

	EXPORT_C MContactDataProvider* CalendarTitleL() { return new (ELeave) CCalendarTitleP; }
	EXPORT_C MContactDataProvider* CalendarDateTimeL() { return new (ELeave) CCalendarDateTimeP; }

	EXPORT_C MContactDataProvider* PresenceAndLocationL(CTimePeriodFormatter& aPeriodFormatter) { return CPresenceAndLocationP::NewL(aPeriodFormatter); }


	EXPORT_C MContactDataProvider* NearbyHeaderL() { return new (ELeave) CNearbyHeaderP; }
	EXPORT_C MContactDataProvider* NearbyPeopleL() { return new (ELeave) CNearbyPeopleP; }

	EXPORT_C MContactDataProvider* IdentityL(const TDesC& aMsg) { return CIdentityP::NewL( aMsg );  }
	EXPORT_C MContactDataProvider* IdentityL(TInt aValue) { 
		TBuf<10> v; 
		v.Num(aValue);
		return CIdentityP::NewL( v ); 
	}


	EXPORT_C MContactDataProvider* StaticIconL(MJuikIconManager& aIconManager, const TDesC& aFile, TInt aIconId) 
	{ return CStaticIconP::NewL( aIconManager, aFile, aIconId ); }


	EXPORT_C MContactDataProvider* ProfileNameL() { return new (ELeave) CProfileNameP; }

	
	EXPORT_C MContactDataProvider* UnreadStreamIconL(MJuikIconManager& aIconManager, TContactUiDelegates& aDelegates) 
	{ 
		return CUnreadStreamIconP::NewL( aIconManager, aDelegates );
	}
}

