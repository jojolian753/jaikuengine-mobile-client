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

#include "csd_sensorevent.h"


EXPORT_C const TTypeName& CBBSensorEvent::Type() const
{
	return KSensorEventType;
}

EXPORT_C const TTypeName& CBBSensorEvent::StaticType()
{
	return KSensorEventType;
}

EXPORT_C TBool CBBSensorEvent::Equals(const MBBData* aRhs) const
{
	const CBBSensorEvent* rhs=bb_cast<CBBSensorEvent>(aRhs);
	return (rhs && *rhs==*this);
}
EXPORT_C MBBDataFactory* CBBSensorEvent::Factory()
{
	return iFactory;
}

EXPORT_C const TTupleName& CBBSensorEvent::TupleName() const
{
	return iTupleName;
}


EXPORT_C MBBData* CBBSensorEvent::CloneL(const TDesC&) const
{
	bb_auto_ptr<CBBSensorEvent> ret(new (ELeave) CBBSensorEvent(iName(), iTupleName, iFactory));
	*ret=*this;
	return ret.release();
}
_LIT(KStamp, "datetime");
_LIT(KPriority, "priority");
_LIT(KName, "eventname");
_LIT(KData, "eventdata");

EXPORT_C CBBSensorEvent::CBBSensorEvent(const TDesC& aName, const TTupleName& aTupleName, MBBDataFactory* aFactory, TTime aStamp, MBBData* aData, TInt aPriority) : TBBCompoundData(KSensorEvent) , iName(aName, KName), iTupleName(aTupleName), iFactory(aFactory), iStamp(aStamp, KStamp), iData(KData, aFactory, aData), iPriority(aPriority, KPriority) { }

EXPORT_C bool CBBSensorEvent::operator==(const CBBSensorEvent& aRhs) const
{
	return (iStamp == aRhs.iStamp &&
	iPriority == aRhs.iPriority &&
	iName == aRhs.iName &&
	iData.Equals(aRhs.iData()));
}

EXPORT_C const MBBData* CBBSensorEvent::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iStamp;
	case 1:
		return &iPriority;
	case 2:
		return &iName;
	case 3:
		return &iData;
	default:
		return 0;
	}
}

_LIT(KSensorEventSpace, " ");
EXPORT_C const TDesC& CBBSensorEvent::StringSep(TUint aBeforePart) const { return KSensorEventSpace; }

EXPORT_C MBBData* CBBSensorEvent::GetPart(const TDesC& aName, const TTypeName& aType, TUint& aPartNoInto)
{
	MBBData* p=TBBCompoundData::GetPart(aName, aType, aPartNoInto);
	if (!p || p==&iData) {
		iData.SetValue(iFactory->CreateBBDataL(aType, KData, iFactory));
		aPartNoInto=3;
		p=&iData;
	}
	return p;
}
EXPORT_C CBBSensorEvent& CBBSensorEvent::operator=(const CBBSensorEvent& aRhs)
{
	iStamp=aRhs.iStamp;
	iPriority=aRhs.iPriority;
	iName=aRhs.iName;
	if (aRhs.iData()) {
		iData.SetValue(aRhs.iData()->CloneL(aRhs.iData()->Name()));
	} else {
		iData.SetValue(0);
	}
	iData.SetOwnsValue(ETrue);
	return *this;
}
