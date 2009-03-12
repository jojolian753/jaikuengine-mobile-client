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

#ifndef __CCU_PRESENCESTATUS_H__
#define __CCU_PRESENCESTATUS_H__

#include <e32std.h>

#include "ccu_contact.h"
#include "csd_presence.h"

enum TPresenceStatus
	{
		EPresenceNone = -1,
		EPresenceGray = 0,
		EPresenceRed,
		EPresenceYellow,
		EPresenceGreen
	};

TPresenceStatus PresenceStatusL( const contact& aContact, 
								 TBool aPresenceEnabled);

TPresenceStatus PresenceStatusL( const CBBPresence* p,
								 TBool aPresenceEnabled,
								 TBool aMyPresence );


IMPORT_C TBool IsOutOfDate(TTime stamp, TTime compare, TInt freshness_interval = 10 /*in minutes*/);

IMPORT_C void TimeSinceStamp(TDes& aInto, TTime stamp, TTime compare, TInt minuteInterval=10);

IMPORT_C TBool IsOffline(CBBPresence& aP, const TTime& aNow);

class CTimePeriodFormatter;

namespace Presence
{
	IMPORT_C TBool IsFromMobile(CBBPresence& aPresence);
}


namespace UserActivity
{
	IMPORT_C void LastUsed(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf);
	IMPORT_C void UserActivity(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf);
	IMPORT_C void ActivityOrLastUse(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf);
}

namespace Location
{
	IMPORT_C TBool IsMissing(const CBBPresence* aPresence);

	IMPORT_C void CurrentL(const CBBPresence* p,  
				  TDes& aBuf);
	IMPORT_C void PreviousL(const CBBPresence* p,  
				   TDes& aBuf);
	IMPORT_C void LocationL(const TDesC& neighbourhood,
				   const TDesC& city,
				   const TDesC& country, 
				   TDes& aBuf);
	IMPORT_C void TimeStamp(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf);
	IMPORT_C void PreviousTimeStamp(CBBPresence& aPresence, CTimePeriodFormatter& aFormatter, TDes& aBuf);
}

namespace Calendar
{
	IMPORT_C TBool IsMissing(const CBBPresence* aPresence);

	IMPORT_C void SharedText(TInt aSetting, TDes& aBuf);

	IMPORT_C const TBBCalendarEvent* GetEvent( const TBBCalendar& aCal );
	IMPORT_C void CurrentOrNextEventL( CBBPresence& aPresence, TDes& aBuf );
	IMPORT_C void EventTimeL( const TBBCalendarEvent& event, TDes& aBuf );
	IMPORT_C void EventDateL( const TBBCalendarEvent& event, TDes& aBuf );
	IMPORT_C void TimeAndDescriptionL(const TBBCalendarEvent& event, TDes& aBuf, TBool aShowDate=ETrue);
	IMPORT_C void NextTextL( TDes& aBuf );
	IMPORT_C void PreviousTextL( TDes& aBuf );
}


namespace Nearby
{
	IMPORT_C TBool IsMissing(const CBBPresence* aPresence);

	IMPORT_C void NotSharedText(TDes& aBuf);
	IMPORT_C void PeopleNearbyText( TDes& aBuf);

	IMPORT_C void Friends( CBBPresence& aPresence, TDes& aBuf, TBool aEmptyWhenZero );
	IMPORT_C void Others( CBBPresence& aPresence, TDes& aBuf, TBool aEmptyWhenZero );

}

#endif
