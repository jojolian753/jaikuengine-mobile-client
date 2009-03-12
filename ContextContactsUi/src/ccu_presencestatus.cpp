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

#include "ccu_presencestatus.h"

#include "ccu_contact.h"
#include "csd_presence.h"
#include "ccu_timeperiod.h"

#include "cl_settings.h"

#include <e32math.h>

EXPORT_C TBool IsOffline(CBBPresence& aP, const TTime& aNow)
{
	TTime since = aP.iSentTimeStamp();
	TBBUserActive& activity = aP.iUserActive;
	return  
		IsOutOfDate( since, aNow ) || 
		( ! activity.iActive() && activity.iSince() == 0 );
}


TPresenceStatus PresenceStatusL( const contact& aContact, 
								 TBool aPresenceEnabled)
{
	return PresenceStatusL( aContact.presence, aPresenceEnabled, aContact.is_myself );
}

TPresenceStatus PresenceStatusL( const CBBPresence* p,
								 TBool aPresenceEnabled,
								 TBool aMyPresence )
{
	TPresenceStatus ix = EPresenceNone;
	if ( !p || ! p->iSent || ! aPresenceEnabled )
		{
			if ( aMyPresence )
				{
					ix = EPresenceGray;
				}
			else
				{
					ix = EPresenceNone;
				}
		}
	else
		{
			TTime stamp = p->iSentTimeStamp();
			TBool outOfDate = IsOutOfDate( stamp, GetTime() );
			
			// out of date 
			
			if ( outOfDate )
				{
					if ( aMyPresence )
						{
							ix = EPresenceGray;
						}
					else
						{
							ix = EPresenceNone;
						}
				}
			else
				{

					// FIXME: this a hack, but we currently have no better way to do this
					// If profiles name is empty, we consider profile to missing
					// Web updates don't have profiles, but we don't know it any other way.
					if ( p->iProfile.iProfileName().Length() == 0 )
						{
							ix = EPresenceNone;
						}
					else 
						{
							TInt ringType = p->iProfile.iRingingType();
							TBool vibraOn = p->iProfile.iVibra();
							TInt volumeOn = p->iProfile.iRingingVolume() != 0;
							
							// RED 
							if ( ( ringType == TBBProfile::ERingingTypeSilent || ! volumeOn ) && ! vibraOn )
								{
									ix = EPresenceRed;
								}
							
							// YELLOW
							else if ( (volumeOn && ringType == TBBProfile::ERingingTypeBeepOnce) || // beeponce 
									  (vibraOn && ! volumeOn ) ||	  // volume == 0 and vibra 
									  (vibraOn && ringType == TBBProfile::ERingingTypeSilent) )	 //silent and vibra
								{
									ix = EPresenceYellow;
								}
							// GREEN 
							else
								{
									ix = EPresenceGreen;
								}
							}
				}
		}
	
	return ix;
}


EXPORT_C void TimeSinceStamp(TDes& aInto, TTime stamp, TTime compare, TInt minuteInterval)
{
	CALLSTACKITEM_N(_CL(""), _CL("TimeSinceStamp"));

	TTime now=compare;
	TBuf<128> ret;

	TTimeIntervalYears years = now.YearsFrom(stamp);
	TTimeIntervalMonths months = now.MonthsFrom(stamp);
	TTimeIntervalDays days = now.DaysFrom(stamp);
	if (years.Int() > 0 )
	{
		ret.Append(_L(">30d"));
	}
	else if (months.Int() > 0 )
	{
		ret.Append(_L(">30d"));
	}
	else if (days.Int() <= 30 && days.Int()>1)
	{
		ret.AppendNum(days.Int());
		ret.Append(_L("d"));
	}
	else if (days.Int() == 1)
	{
		ret.Append(_L(">1d"));
	}
	else
	{
		TTimeIntervalHours hours;
		now.HoursFrom(stamp, hours);
		TTimeIntervalMinutes minutes;
		now.MinutesFrom(stamp, minutes);
				
		if (hours.Int() < 0 ) hours=TTimeIntervalHours(0);
		ret.Format(_L("%d:"), hours.Int());
				
		TInt min = (minutes.Int() - (hours.Int() * 60));
		if (minuteInterval==0) minuteInterval=10;

		TInt32 result = 0;
		TInt err = Math::Int(result, min/minuteInterval);
		if (err==KErrNone)
		{
			result = result*minuteInterval;	
		}
		else
		{
			result = min;
		}
		ret.AppendFormat(_L("%02d"), result);
	}

	aInto.Append(ret);
}

EXPORT_C TBool IsOutOfDate(TTime stamp, TTime compare, TInt freshness_interval)
{
	//const TInt KFreshnessInterval = 10; // in minutes 
	
	TTimeIntervalMinutes m;
	compare.MinutesFrom(stamp, m);
	if (m.Int() > freshness_interval)
	{
		return ETrue;	
	}
	else
	{
		return EFalse;
	}
}

namespace Presence
{
	EXPORT_C TBool IsFromMobile(CBBPresence& aPresence)
	{
		return aPresence.iProfile.iProfileName().Length() > 0;
	}
}

namespace UserActivity
{
	EXPORT_C void LastUsed(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf)
	{
		TTime now = GetTime();
		TTime since = aPresence.iSentTimeStamp();
		
		aBuf.Append(_L("Last update ") );
		TTimePeriod period = TTimePeriod::BetweenL(since, now);
		aFormatter.AgoTextL(period, aBuf);
	}
	
	EXPORT_C void UserActivity(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf)
	{
		TTime now = GetTime();

		if ( aPresence.iUserActive.iActive() )
			{
				aBuf.Append( _L("Currently using phone") );
			}
		else 
			{
				TTime activesince = aPresence.iUserActive.iSince();
				aBuf.Append(_L("Used phone ") );
				TTimePeriod period = TTimePeriod::BetweenL(activesince, now, 1);
				aFormatter.AgoTextL(period, aBuf);
			}
	}	
	
	
	EXPORT_C void ActivityOrLastUse(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf)
	{
		TTime now = GetTime();
		if ( IsOffline( aPresence, now) )
			{
				LastUsed( aPresence, aFormatter, aBuf );
			}
		else 
			{
				UserActivity( aPresence, aFormatter, aBuf );
			}
	}
}

_LIT(KComma, ", ");
namespace Location
{


	EXPORT_C TBool IsMissing(const CBBPresence* aPresence)
	{
		if ( aPresence ) 
			{
				const CBBPresence* p = aPresence;

				TBool city = p->iCity().Length() > 0;
				TBool country = p->iCountry().Length() > 0;
				TBool cellname = p->iCellName().Length() > 0;

				TBool current = p->iBaseInfo.iCurrent.IsSet();
				TBool previous = 
					p->iBaseInfo.iPreviousStay.IsSet() ||
					p->iBaseInfo.iPreviousVisit.IsSet();
				
				return ! city && ! country && ! cellname && ! previous && ! current;
			}
		return ETrue;
	}


	EXPORT_C void CurrentL(const CBBPresence* p,  
				  TDes& aBuf)
	{
		if ( !p )
			{
				return;
			}
	
		// Location
		TPtrC neighbourhood;
		TPtrC city;
		TPtrC country;
		
		if ( p->iBaseInfo.iCurrent.IsSet() )
			{
				neighbourhood.Set( p->iBaseInfo.iCurrent.iBaseName() );		
			}
		else 
			{
				neighbourhood.Set(  p->iCellName() );
			}
		city.Set( p->iCity() );
		country.Set( p->iCountry() );
		LocationL( neighbourhood, city, country, aBuf);
	}

	EXPORT_C void PreviousL(const CBBPresence* p,  
							TDes& aBuf)
	{
		if ( !p )
			{
				return;
			}
	
		// Location
		TPtrC neighbourhood;
		TPtrC city;
		TPtrC country;
		
		if ( p->iBaseInfo.iPreviousStay.IsSet() )
			{
				neighbourhood.Set( p->iBaseInfo.iPreviousStay.iBaseName() );		
			}
		else if ( p->iBaseInfo.iPreviousVisit.IsSet() )
			{
				neighbourhood.Set( p->iBaseInfo.iPreviousVisit.iBaseName() );		
			}
		
		city.Set( KNullDesC );
		country.Set( KNullDesC );
		LocationL( neighbourhood, city, country, aBuf);
	}	

	EXPORT_C void LocationL(const TDesC& neighbourhood,
							const TDesC& city,
							const TDesC& country, 
							TDes& aBuf)
	{		
		aBuf.Append(neighbourhood);
		if ( aBuf.Length() && ( city.Length() || country.Length() ) )
			{
				aBuf.Append(KComma);
			}
		aBuf.Append( city );
		if ( aBuf.Length() && ( country.Length() ) )
			{
				aBuf.Append(KComma);
			}
		aBuf.Append( country );
	}

	EXPORT_C void TimeStamp(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf)
	{
		if ( aPresence.iBaseInfo.iCurrent.IsSet() )
			{
				TTime now = GetTime();					
				TTime tstamp = aPresence.iBaseInfo.iCurrent.iEntered();
				TTimePeriod period = TTimePeriod::BetweenL( tstamp, now );
				
				if ( IsOffline(aPresence, now ) )
					{
						aFormatter.AgoTextL( period, aBuf );
					}
				else
					{
						if ( period.iUnit == TTimePeriod::EJust ) 
							{
								aBuf.Append( _L("Just arrived") );
							}
						else
							{
								aFormatter.SinceTextL( period, aBuf );
							}
					}
			}	
	}

	EXPORT_C void PreviousTimeStamp(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf)
	{
		TTime left;
		if ( aPresence.iBaseInfo.iPreviousStay.IsSet() )
			{
				left = aPresence.iBaseInfo.iPreviousStay.iLeft();
			}
		else if ( aPresence.iBaseInfo.iPreviousVisit.IsSet() )
			{
				left =  aPresence.iBaseInfo.iPreviousVisit.iLeft();
			}

		if (left != TTime()) 
			{
				TTime now = GetTime();					
				TTime tstamp = left;
				TTimePeriod period = TTimePeriod::BetweenL( tstamp, now );
				aBuf.Append( _L("left ") );
				aFormatter.AgoTextL( period, aBuf );
			}
	}
}

namespace Calendar
{

	EXPORT_C TBool IsMissing(const CBBPresence* aPresence)
	{
		if ( aPresence ) 
			{
				const TBBCalendar& cal = aPresence->iCalendar;		
				TBool hasPrev = cal.iPrevious.iDescription().Length() > 0;
				TBool hasCurrent = cal.iCurrent.iDescription().Length() > 0;
				TBool hasNext = cal.iNext.iDescription().Length() > 0;
				return !hasPrev && !hasCurrent && !hasNext;
			}
		return ETrue;
	}


	EXPORT_C void SharedText(TInt aSettingValue, TDes& aBuf)
	{
		switch ( aSettingValue ) 
			{
			case SHARE_CALENDAR_FULL: 
				aBuf.Append( _L("Calendar events shared") );
				break;
			case SHARE_CALENDAR_FREEBUSY:
				aBuf.Append( _L("Events shared, but title not shared") );
				break;
			case SHARE_CALENDAR_NONE:
				aBuf.Append( _L("Calendar events not shared") );
				break;				
			}
	}

	EXPORT_C const TBBCalendarEvent* GetEvent( const TBBCalendar& aCal )
	{
		TBool hasPrev = aCal.iPrevious.iDescription().Length() > 0;
		TBool hasCurrent = aCal.iCurrent.iDescription().Length() > 0;
		TBool hasNext = aCal.iNext.iDescription().Length() > 0;
		
		if ( hasCurrent )
			{
				return &(aCal.iCurrent);
			}
		else if (hasNext) 
			{
				return &(aCal.iNext);
			}
		else
			return NULL;
	}
	
	EXPORT_C void NextTextL( TDes& aBuf )
	{
		aBuf.Append( _L("Next: ") );
	}

	EXPORT_C void PreviousTextL( TDes& aBuf )
	{
		aBuf.Append( _L("Previous: ") );
	}
	
	EXPORT_C void CurrentOrNextEventL( CBBPresence& aPresence, TDes& aBuf ) 
	{
		const TBBCalendar& cal = aPresence.iCalendar;
		const TBBCalendarEvent* event = GetEvent( cal );
		if ( event )
			{
				if (*event == cal.iNext)
					{
						NextTextL( aBuf );
					}
				aBuf.Append( event->iDescription() );
			}
		else
			{
				aBuf.Append( _L("No calendar events") );
			}
	}
	

	EXPORT_C TBool OnSameDay(TTime aX, TTime aY)
	{
		TDateTime x = aX.DateTime();
		TDateTime y = aY.DateTime();	
		return x.Year() == y.Year() && x.Month()==y.Month() && x.Day()==y.Day();
	}
	
	
	EXPORT_C void EventTimeL( const TBBCalendarEvent& event, TDes& aBuf )
	{
		TBuf<20> tBuf; 
		TTime s = event.iStartTime();
		TTime e = event.iEndTime();
		s.FormatL( tBuf, _L("%F%H:%T-") );
		aBuf.Append(tBuf);
		e.FormatL( tBuf, _L("%F%H:%T") );
		aBuf.Append(tBuf);
	}

	EXPORT_C void EventDateL( const TBBCalendarEvent& event, TDes& aBuf )
	{
		TBuf<20> tBuf; 
		TTime s = event.iStartTime();
		TTime e = event.iEndTime();
		TTime now;
		now.HomeTime();
		
		if ( OnSameDay(s, now ) )
			{
				aBuf.Append(_L("Today "));
			}
		else if ( OnSameDay(s , now + TTimeIntervalDays(1)) )
			{
				aBuf.Append(_L("Tomorrow "));
			}
		else 
			{
				s.FormatL( tBuf, _L("%F%D.%M.%Y ") ); //FIXME: change to localized	
				aBuf.Append(tBuf);
			}
	}

	EXPORT_C void TimeAndDescriptionL(const TBBCalendarEvent& event, TDes& aBuf, TBool aShowDate)
	{
		if ( event.iDescription().Length() )
			{ 
				if ( aShowDate )
					{
					Calendar::EventDateL(event, aBuf);					
					//aBuf.Append( _//L(" ") );
					}

				Calendar::EventTimeL(event, aBuf);
				aBuf.Append( _L(" ") );
				aBuf.Append( event.iDescription() );
			}
	}
}


namespace Nearby
{
	EXPORT_C TBool IsMissing(const CBBPresence* aPresence)
	{
		if ( aPresence ) 
			{
				TInt friends = aPresence->iNeighbourhoodInfo.iBuddies();
				TInt others = aPresence->iNeighbourhoodInfo.iOtherPhones();
				return friends == 0 && others == 0;
			}
		return ETrue;
	}

	EXPORT_C void NotSharedText(TDes& aBuf)
	{
		aBuf.Append( _L("Not shared") );
	}

	EXPORT_C void PeopleNearbyText( TDes& aBuf)
	{
		aBuf.Append( _L("People Nearby:") );
	}

	EXPORT_C void Friends( CBBPresence& aPresence, TDes& aBuf, TBool aZeroAsEmpty )
	{
					TInt friends = aPresence.iNeighbourhoodInfo.iBuddies();
					if ( !aZeroAsEmpty || friends > 0 )
						{
							TBuf<4> num;
							num.Num(friends);
							aBuf.Append( num );
							aBuf.Append( _L(" friends") );
						}
	}

	EXPORT_C void Others( CBBPresence& aPresence, TDes& aBuf, TBool aZeroAsEmpty )
	{

					TInt others =   aPresence.iNeighbourhoodInfo.iOtherPhones();
					if ( !aZeroAsEmpty || others > 0 )
						{
							TBuf<4> num;
							num.Num(others);
							aBuf.Append( num );
							aBuf.Append( _L(" others") );
						}
	}
   
}
