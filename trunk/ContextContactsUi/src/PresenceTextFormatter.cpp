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

#include "PresenceTextFormatter.h"

#include "ccu_presencestatus.h"
#include "ccu_timeperiod.h"

#include "csd_presence.h" 
#include "cc_stringtools.h"

EXPORT_C CPresenceTextFormatter* CPresenceTextFormatter::NewL()
{
	CALLSTACKITEMSTATIC_N( _CL("CPresenceTextFormatter"), _CL("NewL") );
	auto_ptr<CPresenceTextFormatter> self( new (ELeave) CPresenceTextFormatter(NULL) );
	self->ConstructL();
	return self.release();
}

EXPORT_C CPresenceTextFormatter* CPresenceTextFormatter::NewL(CTimePeriodFormatter& aPeriodFormatter)
{
	CALLSTACKITEMSTATIC_N( _CL("CPresenceTextFormatter"), _CL("NewL") );
	auto_ptr<CPresenceTextFormatter> self( new (ELeave) CPresenceTextFormatter(&aPeriodFormatter) );
	self->ConstructL();
	return self.release();
}


EXPORT_C CPresenceTextFormatter::~CPresenceTextFormatter()
{
	CALLSTACKITEM_N( _CL("CPresenceTextFormatter"), _CL("~CPresenceTextFormatter") );
	if ( iPeriodFormatterOwned )
		{
			delete iPeriodFormatter;
		}
}


CPresenceTextFormatter::CPresenceTextFormatter(CTimePeriodFormatter* aPeriodFormatter) 
	: iPeriodFormatter(aPeriodFormatter), iPeriodFormatterOwned( EFalse )
{
	CALLSTACKITEM_N( _CL("CPresenceTextFormatter"), _CL("CPresenceTextFormatter") );
}


void CPresenceTextFormatter::ConstructL()
{
	CALLSTACKITEM_N( _CL("CPresenceTextFormatter"), _CL("ConstructL") );
	// Read localized strings once here
	if ( ! iPeriodFormatter )
		{
			iPeriodFormatter = CTimePeriodFormatter::NewL();
			iPeriodFormatterOwned = ETrue;
		}
}

// FIXME: Put this to resource for localization 
_LIT( KPresenceIn_, "In "); 
_LIT( KPresence_in_, " in "); 
_LIT( KComma, ", "); 
_LIT( KSpace, " "); 
_LIT( KPresenceOffline, "Offline");
//_LIT( KPresenceOffline, "Rich Presence not active");


EXPORT_C void CPresenceTextFormatter::LocationL(const CBBPresence* p,  
									   TDes& aBuf)
{
	if ( !p )
		{
			//aBuf.Append( KPresenceOffline );
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


EXPORT_C void CPresenceTextFormatter::PresenceTextL(const CBBPresence* p, 
										   TDes& aBuf, 
										   TBool aUseLong, 
										   TInt aPresenceMaxLen)
{
	CALLSTACKITEM_N( _CL("CPresenceTextFormatter"), _CL("PresenceTextL") );
	
	// No presence 
	if ( !p || ! p->iSent )
		{
			aBuf.Append( KPresenceOffline );
		}
	// Has presence 
	else
		{	
			// Out of date? -> Insert "2 hours ago: "
			TTime stamp = p->iSentTimeStamp();
			TTime now = GetTime();
			if ( IsOutOfDate( stamp, now ) ) 
				{
					InsertAgoTextL( stamp, now, aBuf );
				}
			
			// Presence line 
			TBool hasPresence = EFalse;			
			TPtrC msg( p->iUserGiven.iDescription.iValue );
			if ( msg.Length() )
				{
					hasPresence = ETrue;					
					TPtrC truncated( !aUseLong && aPresenceMaxLen >= 0 ? 
									 msg.Left( aPresenceMaxLen ) :
									 msg );
					aBuf.Append( truncated );
				}
			else
				{
					const TBBCalendar& cal = p->iCalendar;
					TPtrC calEvent( cal.iCurrent.iDescription() );
					if ( calEvent.Length() ) 
						{
							hasPresence = ETrue;
							TPtrC truncated( !aUseLong && aPresenceMaxLen >= 0 ? 
											 calEvent.Left( aPresenceMaxLen ) :
											 calEvent );
							aBuf.Append( truncated );
						}
				}
	
			// Location
			TPtrC location;
			iTmpBuf.Zero();
			LocationL( p, iTmpBuf );
			location.Set(iTmpBuf);
			
			// Combine presence line & location and put prefix or midfix
			TBool hasLoc = location.Length() > 0;

			if ( hasPresence && hasLoc )
				{
					aBuf.Append( KPresence_in_ );
				}
			else if ( hasLoc )
				{
					aBuf.Append( KPresenceIn_ );
				}
	
			if ( hasLoc	 )
				{
					aBuf.Append( location );
				}	
		}
}


EXPORT_C void CPresenceTextFormatter::LongTextL(const CBBPresence* p, 
										TDes& aBuf)
{
	CALLSTACKITEM_N( _CL("CPresenceTextFormatter"), _CL("LongTextL") );
	PresenceTextL( p, aBuf, ETrue, -1 );
}


void CPresenceTextFormatter::ShortTextL(const CBBPresence* p, 
										TDes& aBuf, 
										TInt aPresenceMaxLen)
{
 	CALLSTACKITEM_N( _CL("CPresenceTextFormatter"), _CL("ShortTextL") );
	PresenceTextL( p, aBuf, EFalse, aPresenceMaxLen );
// 	// No presence 
// 	if ( !p || ! p->iSent )
// 		{
// 			aBuf.Append( KPresenceOffline );
// 		}
// 	// Has presence 
// 	else
// 		{	
// 			// Out of date? -> Insert "2 hours ago: "
// 			TTime stamp = p->iSentTimeStamp();
// 			TTime now = GetTime();
// 			if ( IsOutOfDate( stamp, now ) ) 
// 				{
// 					InsertAgoTextL( stamp, now, aBuf );
// 				}
			
// 			// Presence line 
// 			TBool hasPresence = EFalse;			
// 			TPtrC msg( p->iUserGiven.iDescription.iValue );
// 			if ( msg.Length() )
// 				{
// 					hasPresence = ETrue;
// 					TPtrC truncated( aPresenceMaxLen >= 0 ? 
// 									 msg.Left( aPresenceMaxLen ) :
// 									 msg );
// 					aBuf.Append( truncated );
// 				}
// 			else
// 				{
// 					const TBBCalendar& cal = p->iCalendar;
// 					TPtrC calEvent( cal.iCurrent.iDescription() );
// 					if ( calEvent.Length() ) 
// 						{
// 							hasPresence = ETrue;
// 							TPtrC truncated( aPresenceMaxLen >= 0 ? 
// 											 calEvent.Left( aPresenceMaxLen ) :
// 											 calEvent );
// 							aBuf.Append( truncated );
// 						}
// 				}
	
// 			// Location
// 			TPtrC location;
// 			if ( p->iBaseInfo.iCurrent.IsSet() )
// 				{
// 					location.Set( p->iBaseInfo.iCurrent.iBaseName() );		
// 				}
// 			else if ( p->iCellName().Length() )
// 				{
// 					location.Set(  p->iCellName() );
// 				}
// 			else 
// 				{
// 					TPtrC city;
// 					TPtrC country;	
// 					city.Set( p->iCity() );
// 					country.Set( p->iCountry() );
// 					if ( city.Length() )
// 						location.Set( city );
// 					else 
// 						location.Set( country );
// 				}

// 			// Combine presence line & location and put prefix or midfix

// 			TBool hasLoc = location.Length();

// 			if ( hasPresence && hasLoc )
// 				{
// 					aBuf.Append( KPresence_in_ );
// 				}
// 			else if ( hasLoc )
// 				{
// 					aBuf.Append( KPresenceIn_ );
// 				}
	
// 			if ( hasLoc	 )
// 				{
// 					aBuf.Append( location );
// 				}	
// 		}
}


// _LIT( KPresence_over_1_year_ago, "Over 1 year ago: ");
// _LIT( KPresence_X_months_ago1,   "%d month ago: ");
// _LIT( KPresence_X_months_agoN,   "%d months ago: ");
// _LIT( KPresence_X_weeks_ago1,     "%d week ago: ");
// _LIT( KPresence_X_weeks_agoN,     "%d weeks ago: ");
// _LIT( KPresence_X_days_ago1,     "%d day ago: ");
// _LIT( KPresence_X_days_agoN,     "%d days ago: ");
// _LIT( KPresence_X_hours_ago1,    "%d hour ago: ");
// _LIT( KPresence_X_hours_agoN,    "%d hours ago: ");
// _LIT( KPresence_X_mins_ago1,     "%d min ago: ");
// _LIT( KPresence_X_mins_agoN,     "%d mins ago: ");

_LIT( KDoubleColon,  ": ");
void CPresenceTextFormatter::InsertAgoTextL( TTime& stamp, TTime& now, TDes& aBuf )
{
	CALLSTACKITEM_N( _CL("CPresenceTextFormatter"), _CL("InsertAgoTextL") );
	TTimePeriod period = TTimePeriod::BetweenL( stamp, now, 10 );
	iPeriodFormatter->AgoTextL( period, aBuf );
	SafeAppend( aBuf, KDoubleColon );
}
