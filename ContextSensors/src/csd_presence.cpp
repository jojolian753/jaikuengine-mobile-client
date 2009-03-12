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

#include "csd_presence.h"
#include <e32math.h>
#include "symbian_auto_ptr.h"

EXPORT_C CBBPresence* CBBPresence::NewL()
{
	auto_ptr<CBBPresence> ret(new (ELeave) CBBPresence);
	ret->ConstructL();
	ret->AddRef();
	return ret.release();
}
EXPORT_C CBBPresence* CBBPresence::NewL(TUint aVersion)
{
	auto_ptr<CBBPresence> ret(new (ELeave) CBBPresence(aVersion));
	ret->ConstructL();
	ret->AddRef();
	return ret.release();
}
EXPORT_C void CBBPresence::AddRef() const
{
	++iRefCount;
}

EXPORT_C void CBBPresence::Release() const
{
	--iRefCount;
	if (iRefCount<0) {
		User::Panic(_L("CSD_PRESENCE"), 1);
	}
	if (iRefCount==0) 
		delete this;
}

EXPORT_C const TTypeName& CBBPresence::Type() const
{
	return KPresenceType;
}

EXPORT_C TBool CBBPresence::Equals(const MBBData* aRhs) const
{
	const CBBPresence* rhs=bb_cast<CBBPresence>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& CBBPresence::StaticType()
{
	return KPresenceType;
}

EXPORT_C void CBBPresence::ConstructL()
{
	iDevices=CBBBtDeviceList::NewL();
	iBuddyDevices=CBBBtDeviceList::NewL();
}

EXPORT_C void CBBPresence::MinimalXmlL(MBBExternalizer* aBuf)
{
	aBuf->BeginCompound(Name(), EFalse, Type());

	iBaseInfo.IntoXmlL(aBuf, EFalse);
	iCity.IntoXmlL(aBuf, EFalse);
	iCountry.IntoXmlL(aBuf, EFalse);
	iCellId.IntoXmlL(aBuf, EFalse);
	iCellName.IntoXmlL(aBuf, EFalse);
	iUserActive.IntoXmlL(aBuf, EFalse);
	iProfile.IntoXmlL(aBuf, EFalse);
	iUserGiven.IntoXmlL(aBuf, EFalse);
	iNeighbourhoodInfo.IntoXmlL(aBuf, EFalse);
	iCalendar.IntoXmlL(aBuf, EFalse);
	//iUnread.IntoXmlL(aBuf, EFalse);
	//iAlarm.IntoXmlL(aBuf, EFalse);
	iCitySince.IntoXmlL(aBuf, EFalse);
	iCountrySince.IntoXmlL(aBuf, EFalse);
	iBuddyDevices->IntoXmlL(aBuf, EFalse);
	if (iGenerated()) iGenerated.IntoXmlL(aBuf, EFalse);
	iConnectivityModel.IntoXmlL(aBuf, EFalse);
	iInCall.IntoXmlL(aBuf, EFalse);

	aBuf->EndCompound(Name());
}

EXPORT_C const MBBData* CBBPresence::Part(TUint aPartNo) const
{
	switch(aPartNo) {
	case 0:
		return &iCellId;
	case 1:
		return &iBaseInfo;
	case 2:
		return &iUserActive;
	case 3:
		return &iProfile;
	case 4:
		return &iUserGiven;
	case 5:
		return &iGps;
	case 6:
		return &iNeighbourhoodInfo;
	case 7:
		return iBuddyDevices;
	case 8:
		return &iCalendar;
	case 9:
		return &iUnread;
	case 10:
		return &iAlarm;
	case 11:
		return &iCity;
	case 12:
		return &iCountry;
	case 13:
		return &iSentTimeStamp;
	case 14:
		return &iCellName;
	case 15:
		return &iGpsStamp;
	case 16:
		return &iPhoneNumberHash;
	case 17:
		return &iFirstName;
	case 18:
		return &iCitySince;
	case 19:
		return &iCountrySince;
	case 20:
		return &iLastName;
	case 21:
		return &iPhoneNumberIsVerified;
	case 22:
		return &iGenerated;
	case 23:
		return &iConnectivityModel;
	case 24:
		return &iInCall;
	case 25:
		return iDevices; 
	default:
		return 0;
	}
}

_LIT(KUserGiven, "usergiven");
_LIT(KSent, "sent");
_LIT(KGpsStamp, "gps_stamp");
_LIT(KPhoneNumberHash, "phonenumberhash");
_LIT(KFirstName, "firstname");
_LIT(KLastName, "lastname");
_LIT(KCitySince, "citysince");
_LIT(KCountrySince, "countrysince");
_LIT(KPhoneNumberIsVerified, "phonenumberisverified");
_LIT(KGenerated, "generated");
_LIT(KConnectivity, "connectivitymodel");
_LIT(KInCall, "incall");

EXPORT_C CBBPresence::CBBPresence(TUint aVersion) : TBBCompoundData(KPresence), iUserGiven(KUserGiven), iCellId(KCell),
	iSentTimeStamp(KSent), iCalendar(aVersion >= 3 ? 2 : 1), iAlarm(KAlarm), iCity(KCity),
	iCountry(KCountry), iCreatedVersion(aVersion), iUseVersion(7), iCellName(KCellName),
	iGpsStamp(KGpsStamp), iPhoneNumberHash(KPhoneNumberHash), iFirstName(KFirstName), iCitySince(KCitySince),
	iCountrySince(KCountrySince), iLastName(KLastName), iPhoneNumberIsVerified(KPhoneNumberIsVerified), iGenerated(KGenerated),
	iConnectivityModel(KConnectivity), iInCall(KInCall) { }
EXPORT_C CBBPresence::CBBPresence() : TBBCompoundData(KPresence), iUserGiven(KUserGiven), iCellId(KCell),
	iSentTimeStamp(KSent), iAlarm(KAlarm), iCity(KCity),
	iCountry(KCountry), iCreatedVersion(7), iUseVersion(7), iCellName(KCellName),
	iGpsStamp(KGpsStamp), iPhoneNumberHash(KPhoneNumberHash), iFirstName(KFirstName), iCitySince(KCitySince),
	iCountrySince(KCountrySince), iLastName(KLastName), iPhoneNumberIsVerified(KPhoneNumberIsVerified), iGenerated(KGenerated),
	iConnectivityModel(KConnectivity), iInCall(KInCall) { }

EXPORT_C void CBBPresence::InternalizeL(RReadStream& aStream)
{
	iUseVersion=iCreatedVersion;
	TBBCompoundData::InternalizeL(aStream);
	iUseVersion=7;
}


_LIT(KSpace, " ");

EXPORT_C const TDesC& CBBPresence::StringSep(TUint aBeforePart) const
{
	return KSpace;
}

EXPORT_C bool CBBPresence::operator==(const CBBPresence& aRhs) const
{
	return (iCellId==aRhs.iCellId &&
		iBaseInfo==aRhs.iBaseInfo &&
		iUserActive==aRhs.iUserActive &&
		iProfile==aRhs.iProfile &&
		iDevices->Equals(aRhs.iDevices) &&
		iGps==aRhs.iGps &&
		iNeighbourhoodInfo==aRhs.iNeighbourhoodInfo &&
		iUserGiven==aRhs.iUserGiven &&
		iSentTimeStamp()==aRhs.iSentTimeStamp() &&
		iSent==aRhs.iSent &&
		iCalendar==aRhs.iCalendar &&
		iUnread==aRhs.iUnread &&
		iAlarm==aRhs.iAlarm &&
		iCity == aRhs.iCity &&
		iCountry == aRhs.iCountry &&
		iCellName == aRhs.iCellName &&
		iGpsStamp() == aRhs.iGpsStamp() &&
		iPhoneNumberHash == aRhs.iPhoneNumberHash &&
		iFirstName == aRhs.iFirstName &&
		iCitySince() == aRhs.iCitySince() &&
		iCountrySince() == aRhs.iCountrySince() &&
		iLastName == aRhs.iLastName &&
		iPhoneNumberIsVerified() == aRhs.iPhoneNumberIsVerified() &&
		iGenerated() == aRhs.iGenerated() &&
		iConnectivityModel() == aRhs.iConnectivityModel() &&
		iInCall() == aRhs.iInCall()
		);
}

EXPORT_C MBBData* CBBPresence::CloneL(const TDesC& Name) const
{
	auto_ptr<CBBPresence> ret(new (ELeave) CBBPresence);
	*ret=*this;
	ret->AddRef();
	return ret.release();
}

EXPORT_C void CBBPresence::SetDevices(const CBBBtDeviceList* aList)
{
	MBBData* d=aList->CloneL(aList->Name());
	CBBBtDeviceList* l=bb_cast<CBBBtDeviceList>(d);
	if (!l) {
		delete d;
		User::Leave(KErrNotSupported);
	}
	delete iDevices;
	iDevices=l;
}

EXPORT_C CBBPresence& CBBPresence::operator=(const CBBPresence& aRhs)
{
	{
		MBBData* d=aRhs.iDevices->CloneL(aRhs.iDevices->Name());
		CBBBtDeviceList* l=bb_cast<CBBBtDeviceList>(d);
		if (!l) {
			delete d;
			User::Leave(KErrNotSupported);
		}
		delete iDevices;
		iDevices=l;
	}

	{
		MBBData* d=aRhs.iBuddyDevices->CloneL(aRhs.iBuddyDevices->Name());
		CBBBtDeviceList* l=bb_cast<CBBBtDeviceList>(d);
		if (!l) {
			delete d;
			User::Leave(KErrNotSupported);
		}
		delete iBuddyDevices;
		iBuddyDevices=l;
	}

	iCellId=aRhs.iCellId;
	iBaseInfo=aRhs.iBaseInfo;
	iUserActive=aRhs.iUserActive;
	iProfile=aRhs.iProfile;

	iGps=aRhs.iGps;
	iNeighbourhoodInfo=aRhs.iNeighbourhoodInfo;
	iUserGiven=aRhs.iUserGiven;
	iSentTimeStamp()=aRhs.iSentTimeStamp();
	iSent=aRhs.iSent;

	iCalendar=aRhs.iCalendar;
	iUnread=aRhs.iUnread;
	iAlarm()=aRhs.iAlarm();

	iCity()=aRhs.iCity();
	iCountry()=aRhs.iCountry();
	iCellName()=aRhs.iCellName();
	iGpsStamp()=aRhs.iGpsStamp();

	iPhoneNumberHash() = aRhs.iPhoneNumberHash();
	iFirstName() = aRhs.iFirstName();
	iCitySince() = aRhs.iCitySince();
	iCountrySince() = aRhs.iCountrySince();
	iLastName() = aRhs.iLastName();
	iPhoneNumberIsVerified() = aRhs.iPhoneNumberIsVerified();
	iGenerated() = aRhs.iGenerated();
	iConnectivityModel() = aRhs.iConnectivityModel();
	iInCall() = aRhs.iInCall();
	
	iReadParts=aRhs.iReadParts;
	
	return *this;
}


CBBPresence::~CBBPresence()
{
	delete iDevices;
	delete iBuddyDevices;
}

//_----------------------------------

#if 1
//#include "csd_usergiven.cpp"
#else
EXPORT_C const TTypeName& TBBUserGiven::Type() const
{
	return KUserGivenType;
}

EXPORT_C TBool TBBUserGiven::Equals(const MBBData* aRhs) const
{
	const TBBUserGiven* rhs=bb_cast<TBBUserGiven>(aRhs);
	return (rhs && *this==*rhs);
}

EXPORT_C const TTypeName& TBBUserGiven::StaticType()
{
	return KUserGivenType;
}

EXPORT_C const MBBData* TBBUserGiven::Part(TUint aPartNo) const
{
	if (aPartNo==0) return &iDescription;
	if (aPartNo==1) return &iSince;
	return 0;
}

_LIT(KDescription, "description");
_LIT(KSince, "since");

EXPORT_C TBBUserGiven::TBBUserGiven(const TDesC& aName) : TBBCompoundData(aName),
	iDescription(KDescription), iSince(KSince)
{
}

EXPORT_C bool TBBUserGiven::operator==(const TBBUserGiven& aRhs) const
{
	return (
		iDescription == aRhs.iDescription &&
		iSince == aRhs.iSince
		);
}


EXPORT_C const TDesC& TBBUserGiven::StringSep(TUint aBeforePart) const
{
	if (aBeforePart==0 || aBeforePart>1) return KNullDesC;
	return KSpace;
}

EXPORT_C TBBUserGiven& TBBUserGiven::operator=(const TBBUserGiven& aUserGiven)
{
	iDescription()=aUserGiven.iDescription();
	iSince()=aUserGiven.iSince();
	return *this;
}

EXPORT_C MBBData* TBBUserGiven::CloneL(const TDesC& Name) const
{
	TBBUserGiven* ret=new (ELeave) TBBUserGiven(Name);
	*ret=*this;
	return ret;
}
#endif
