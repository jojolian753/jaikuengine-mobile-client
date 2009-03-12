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

#include "ccu_timeperiod.h"

#include <contextcontactsui.rsg>
#include "app_context.h"
#include "break.h"
#include "errorhandling.h"
#include "symbian_auto_ptr.h"

#include <eikenv.h>
#include <badesca.h>
#include <stringloader.h>
 
EXPORT_C TTimePeriod TTimePeriod::BetweenL( const TTime& aEarlier, const TTime& aLater, TInt aRoundToMins )
{		
	return TTimePeriod::BetweenL(aEarlier, aLater, aRoundToMins, -1);
}

EXPORT_C TTimePeriod TTimePeriod::BetweenL( const TTime& aEarlier, const TTime& aLater, TInt aRoundToMins, TInt aRoundToSecs)
{
	{
		TInt years  = aLater.YearsFrom( aEarlier ).Int();
		if ( years > 0 ) { return TTimePeriod(years, TTimePeriod::EYears ); }
	}
	
	{
		TInt months  = aLater.MonthsFrom( aEarlier ).Int();
		if ( months > 0 ) { return TTimePeriod(months, TTimePeriod::EMonths ); } 
	}
	
	{
		TInt days = aLater.DaysFrom( aEarlier ).Int();
		if ( days > 0 )
			{							
				TInt weeks = days / 7;
				if (weeks > 0)
					{
						return TTimePeriod(weeks, TTimePeriod::EWeeks);
					}
				else
					{
						return TTimePeriod(days, TTimePeriod::EDays);
					}
			}
	}
	
	{
		TTimeIntervalHours hoursIv;
		if ( aLater.HoursFrom( aEarlier, hoursIv ) ) { Bug(_L("hours too far apart to fit")).Raise(); }
		TInt hours = hoursIv.Int();
		if ( hours > 0 )
		{			
			return TTimePeriod(hours, TTimePeriod::EHours);
		}
		
	}

	{
		TTimeIntervalMinutes minutesIv; 
		if ( aLater.MinutesFrom( aEarlier, minutesIv ) ) { Bug(_L("minutes too far apart to fit")).Raise(); }
		TInt minutes = minutesIv.Int();
		// round to nearest X minutes
		TInt rounded = minutes; 
		if ( minutes > aRoundToMins )
			rounded = (minutes / aRoundToMins) * aRoundToMins;			
		
		if ( rounded > 0 )
			{
				return TTimePeriod(rounded, TTimePeriod::EMinutes);
			}
	}

	if ( aRoundToSecs > 0 )
		{
			TTimeIntervalSeconds secondsIv; 
			if ( aLater.SecondsFrom( aEarlier, secondsIv ) ) { Bug(_L("seconds too far apart to fit")).Raise(); }
			TInt seconds = secondsIv.Int();
			// round to nearest X minutes
			TInt rounded = seconds;
			if ( rounded > aRoundToSecs )
				rounded = (seconds / aRoundToSecs) * aRoundToSecs;
			
			if ( rounded > 0 )
				{
					return TTimePeriod(rounded, TTimePeriod::ESeconds);
				}
		}
	return TTimePeriod(0, TTimePeriod::EJust);
}



class CTimePeriodFormatterImpl : public CTimePeriodFormatter, public MContextBase
{
public: 
	CTimePeriodFormatterImpl() {}
	
	~CTimePeriodFormatterImpl()
	{
		CALLSTACKITEM_N(_CL("CTimePeriodFormatterImpl"), _CL("~CTimePeriodFormatterImpl"));
		delete iAgoFormat;
		delete iSinceFormat;
		delete iPlurals;
		delete iSingulars;

		delete iAgoOverYear;
		delete iSinceOverYear;
		delete iAgoJust;
		delete iSinceJust;
	}
	
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CTimePeriodFormatterImpl"), _CL("ConstructL"));
		CEikonEnv* env = CEikonEnv::Static();
		iSinceFormat = StringLoader::LoadL( R_TEXT_SINCE, env );
		iAgoFormat = StringLoader::LoadL( R_TEXT_AGO, env );

		iAgoOverYear = StringLoader::LoadL( R_TEXT_OVER_YEAR_AGO, env );
		iSinceOverYear = StringLoader::LoadL( R_TEXT_OVER_YEAR_SINCE, env );
		iAgoJust = StringLoader::LoadL( R_TEXT_JUST_AGO, env );
		iSinceJust = StringLoader::LoadL( R_TEXT_JUST_SINCE, env );
		
		iSingulars = new (ELeave) CDesCArrayFlat(10);
		iPlurals = new (ELeave) CDesCArrayFlat(10);

		const TInt KUnitCount(8);
		const TInt singulars[KUnitCount] = {
			R_TEXT_YEAR, R_TEXT_MONTH, R_TEXT_WEEK,  R_TEXT_DAY, R_TEXT_HOUR, R_TEXT_MINUTE, R_TEXT_SECOND, R_TEXT_SECOND
		};
		
		const TInt plurals[KUnitCount] = {
			R_TEXT_YEARS, R_TEXT_MONTHS, R_TEXT_WEEKS,  R_TEXT_DAYS, R_TEXT_HOURS, R_TEXT_MINUTES, R_TEXT_SECONDS, R_TEXT_SECONDS 
		};
		
		for (TInt i=0; i < KUnitCount; i++)
			{
				auto_ptr<HBufC> s( StringLoader::LoadL( singulars[i], env ) );
				iSingulars->AppendL( *s );
				
				auto_ptr<HBufC> p( StringLoader::LoadL( plurals[i], env ) );
				iPlurals->AppendL( *p );
			}
	}
	
	void FormatPeriodL(const TTimePeriod& aPeriod, TDes& aBuf, TBool aSingularNumber)
	{
		CALLSTACKITEM_N(_CL("CTimePeriodFormatterImpl"), _CL("FormatPeriodL"));
		TInt ix = aPeriod.iUnit;
		if ( aPeriod.iUnit == TTimePeriod::EJust )
			return;
		
		if ( Abs(aPeriod.iValue) <= 1 ) 
			{
				aBuf.Num( aPeriod.iValue );
				aBuf.Append( TChar(' ') );
				aBuf.Append( iSingulars->MdcaPoint( ix ) );
			}
		else
			{
				aBuf.Num( aPeriod.iValue );
				aBuf.Append( TChar(' ') );
				aBuf.Append( iPlurals->MdcaPoint( ix ) );
			}
	}

	void AgoTextL(const TTimePeriod& aPeriod, TDes& aBuf) 
	{ 
		CALLSTACKITEM_N(_CL("CTimePeriodFormatterImpl"), _CL("AgoTextL"));
		iPeriodBuffer.Zero();
		iAllBuffer.Zero();
		
		// Special cases: >1 year ago and just ago
		if ( aPeriod.iUnit == TTimePeriod::EYears ) 
			{
				aBuf.Append( *iAgoOverYear );
			}
		else if ( aPeriod.iUnit == TTimePeriod::EJust )
			{
				aBuf.Append( *iAgoJust );
			}
		else
			{ // Normal case
				FormatPeriodL( aPeriod, iPeriodBuffer, EFalse );
				iAllBuffer.Format( *iAgoFormat, &iPeriodBuffer );
				aBuf.Append( iAllBuffer );
			}
	}
	
	
	void SinceTextL(const TTimePeriod& aPeriod, TDes& aBuf) 
	{ 		
		CALLSTACKITEM_N(_CL("CTimePeriodFormatterImpl"), _CL("SinceTextL"));
		iPeriodBuffer.Zero();
		iAllBuffer.Zero();

		// Special cases: >1 year ago and just
		if ( aPeriod.iUnit == TTimePeriod::EYears ) 
			{
				aBuf.Append( *iSinceOverYear );
			}
		else if ( aPeriod.iUnit == TTimePeriod::EJust )
			{
				aBuf.Append( *iSinceJust );
			}
		else
			{ // normal case
				FormatPeriodL( aPeriod, iPeriodBuffer, ETrue );
				iAllBuffer.Format( *iSinceFormat, &iPeriodBuffer );
				aBuf.Append( iAllBuffer );
			}
	}
	
	HBufC* iAgoFormat;
	HBufC* iSinceFormat;
	
	HBufC* iAgoOverYear;
	HBufC* iSinceOverYear;

	HBufC* iAgoJust;
	HBufC* iSinceJust;

	CDesCArray* iPlurals;
	CDesCArray* iSingulars;
	
	TBuf<100> iPeriodBuffer;
	TBuf<100> iAllBuffer;
};


EXPORT_C CTimePeriodFormatter* CTimePeriodFormatter::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CTimePeriodFormatter"), _CL("NewL"));
	auto_ptr<CTimePeriodFormatterImpl> self( new (ELeave) CTimePeriodFormatterImpl() );
	self->ConstructL();
	return self.release();
}
