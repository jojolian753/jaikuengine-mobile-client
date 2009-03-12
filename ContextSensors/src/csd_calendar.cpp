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

#include "csd_calendar.h"

EXPORT_C const TTypeName& TBBCalendarEvent::Type() const
{
	return KCalendarEventType;
}

EXPORT_C TBool TBBCalendarEvent::Equals(const MBBData* aRhs) const
{
	const TBBCalendarEvent* rhs=bb_cast<TBBCalendarEvent>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBCalendarEvent::StaticType()
{
	return KCalendarEventType;
}

EXPORT_C const MBBData* TBBCalendarEvent::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iStartTime;
	case 1:
		return &iDescription;
	case 2:
		return &iEndTime;
	case 3:
		return &iEventId;
	default:
		return 0;
	}
}

_LIT(KDescription, "description");
_LIT(KStartTime, "start_time");
_LIT(KEndTime, "end_time");
_LIT(KEventId, "eventid");

EXPORT_C TBBCalendarEvent::TBBCalendarEvent(const TDesC& aName) : 
	TBBCompoundData(aName),
	iDescription(KDescription), iStartTime(KStartTime), iEndTime(KEndTime),
	iEventId(KEventId) { }

EXPORT_C bool TBBCalendarEvent::operator==(const TBBCalendarEvent& aRhs) const
{
	return (
		iDescription==aRhs.iDescription &&
		iStartTime()==aRhs.iStartTime() &&
		iEndTime()==aRhs.iEndTime() &&
		iEventId()==aRhs.iEventId() 
		);
}

_LIT(KSemicolon, ";");

EXPORT_C const TDesC& TBBCalendarEvent::StringSep(TUint aBeforePart) const
{
	if (aBeforePart>0 && aBeforePart<=3)
		return KSemicolon;
	else
		return KNullDesC;
}

EXPORT_C TBBCalendarEvent& TBBCalendarEvent::operator=(const TBBCalendarEvent& aRhs)
{
	iDescription()=aRhs.iDescription();
	iStartTime()=aRhs.iStartTime();
	iEndTime()=aRhs.iEndTime();
	iEventId()=aRhs.iEventId();

	return *this;
}

EXPORT_C MBBData* TBBCalendarEvent::CloneL(const TDesC& Name) const
{
	TBBCalendarEvent* ret=new (ELeave) TBBCalendarEvent(Name);
	*ret=*this;
	return ret;
}

EXPORT_C const TTypeName& TBBCalendar::Type() const
{
	return KCalendarType;
}

EXPORT_C TBool TBBCalendar::Equals(const MBBData* aRhs) const
{
	const TBBCalendar* rhs=bb_cast<TBBCalendar>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBCalendar::StaticType()
{
	return KCalendarType;
}

EXPORT_C const MBBData* TBBCalendar::Part(TUint aPartNo) const
{
	switch (aPartNo) {
	case 0:
		return &iPrevious;
	case 1:
		return &iCurrent;
	case 2:
		return &iNext;
	default:
		return 0;
	}
}

_LIT(KPrevious, "previous");
_LIT(KCurrent, "current");
_LIT(KNext, "next");

EXPORT_C TBBCalendar::TBBCalendar() : TBBCompoundData(KCalendar), 
	iPrevious(KPrevious), iCurrent(KCurrent), iNext(KNext), 
	iCreatedVersion(2), iUseVersion(2) { }

EXPORT_C TBBCalendar::TBBCalendar(TUint aVersion) : TBBCompoundData(KCalendar), 
	iPrevious(KPrevious), iCurrent(KCurrent), iNext(KNext), 
	iCreatedVersion(aVersion), iUseVersion(2) { }


EXPORT_C void TBBCalendar::InternalizeL(RReadStream& aStream)
{
	iUseVersion=iCreatedVersion;
	TBBCompoundData::InternalizeL(aStream);
	iUseVersion=2;
}

EXPORT_C bool TBBCalendar::operator==(const TBBCalendar& aRhs) const
{
	return (
		iPrevious==aRhs.iPrevious &&
		iCurrent==aRhs.iCurrent &&
		iNext == aRhs.iNext
		);
}

_LIT(KHash, "#");

EXPORT_C const TDesC& TBBCalendar::StringSep(TUint aBeforePart) const
{
	if (aBeforePart>0 && aBeforePart<=2)
		return KHash;
	else
		return KNullDesC;
}

EXPORT_C TBBCalendar& TBBCalendar::operator=(const TBBCalendar& aRhs)
{
	iPrevious=aRhs.iPrevious;
	iCurrent=aRhs.iCurrent;
	iNext = aRhs.iNext;

	return *this;
}

EXPORT_C MBBData* TBBCalendar::CloneL(const TDesC& ) const
{
	TBBCalendar* ret=new (ELeave) TBBCalendar;
	*ret=*this;
	return ret;
}

EXPORT_C MNestedXmlHandler* TBBCalendar::FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
	HBufC*& aBuf, TBool aCheckType)
{
	MNestedXmlHandler* ret=TBBCompoundData::FromXmlL(aParent, aParser, aBuf, aCheckType);
	ret->iIgnoreOffset=ETrue;
	return ret;
}

EXPORT_C void TBBCalendar::IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType) const
{
	TTimeIntervalSeconds offs=aBuf->iOffset;
	aBuf->iOffset=TTimeIntervalSeconds(0);
	TRAPD(err, TBBCompoundData::IntoXmlL(aBuf, aIncludeType));
	aBuf->iOffset==offs;
	User::LeaveIfError(err);
}
