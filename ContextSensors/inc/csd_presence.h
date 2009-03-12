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

#ifndef CONTEXT_CSD_PRESENCE_H_INCLUDED
#define CONTEXT_CSD_PRESENCE_H_INCLUDED 1

#include "csd_cell.h"
#include "csd_base.h"
#include "csd_idle.h"
#include "csd_profile.h"
#include "csd_bluetooth.h"
#include "csd_gps.h"
#include "csd_calendar.h"
#include "csd_unread.h"
#include "csd_md5hash.h"
#include "symbian_refcounted_ptr.h"

const TTupleName KUserGivenContextTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 33 };
_LIT(KPresence, "presencev2");

const TTupleName KAlarmTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 41 };
_LIT(KAlarm, "alarm");

const TTypeName KPresenceType1 = { { CONTEXT_UID_SENSORDATAFACTORY }, 31, 1, 0 };
const TTypeName KPresenceType2= { { CONTEXT_UID_SENSORDATAFACTORY }, 31, 2, 0 };
const TTypeName KPresenceType3 = { { CONTEXT_UID_SENSORDATAFACTORY }, 31, 3, 0 };
const TTypeName KPresenceType4 = { { CONTEXT_UID_SENSORDATAFACTORY }, 31, 4, 0 };
const TTypeName KPresenceType5 = { { CONTEXT_UID_SENSORDATAFACTORY }, 31, 5, 0 };
const TTypeName KPresenceType6 = { { CONTEXT_UID_SENSORDATAFACTORY }, 31, 6, 0 };
const TTypeName KPresenceType = { { CONTEXT_UID_SENSORDATAFACTORY }, 31, 7, 0 };
const TTupleName KPresenceTuple = { { CONTEXT_UID_CONTEXTSENSORS }, 31 };

#if 1
#include "csd_usergiven.h"
#else
const TTypeName KUserGivenType = { { CONTEXT_UID_SENSORDATAFACTORY }, 33, 1, 0 };
class TBBUserGiven : public TBBCompoundData {
public:
	TBBLongString		iDescription;
	TBBTime			iSince;

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;

	IMPORT_C TBBUserGiven(const TDesC& aName);

	IMPORT_C bool operator==(const TBBUserGiven& aRhs) const;
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C TBBUserGiven& operator=(const TBBUserGiven& aBattery);
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
};
#endif

class CBBPresence : public CBase, public TBBCompoundData, public MRefCounted {
public:
	TBBCellId	iCellId;
	TBBBaseInfo	iBaseInfo;
	TBBUserActive	iUserActive;
	TBBProfile	iProfile;
	CBBBtDeviceList*	iDevices;
	TGpsLine	iGps;
	TBBNeighbourhoodInfo iNeighbourhoodInfo;
	TBBUserGiven	iUserGiven;
	TBBTime		iSentTimeStamp;
	TBBCalendar	iCalendar;
	TBBUnread	iUnread;
	TBBTime		iAlarm;
	TBBLongString	iCity;
	TBBShortString	iCountry;
	TBBLongString	iCellName;
	TBBTime		iGpsStamp;
	TBBMD5Hash	iPhoneNumberHash;
	TBBLongString iFirstName;
	TBBTime		iCitySince;
	TBBTime		iCountrySince;
	CBBBtDeviceList*	iBuddyDevices;
	TBBLongString iLastName;
	TBBBool		iPhoneNumberIsVerified;
	TBBBool		iGenerated;
	TBBInt		iConnectivityModel;
	TBBBool		iInCall;

	TBool		iSent;

	IMPORT_C void SetDevices(const CBBBtDeviceList* aList); // clones the list

	// From MRefCounted
	IMPORT_C void AddRef() const;
	IMPORT_C void Release() const;


	IMPORT_C void ConstructL();

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C virtual TBool Equals(const MBBData* aRhs) const;

	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C const MBBData* Part(TUint aPartNo) const;
	IMPORT_C CBBPresence();
	IMPORT_C CBBPresence(TUint aVersion);
	IMPORT_C virtual const TDesC& StringSep(TUint aBeforePart) const;

	IMPORT_C bool operator==(const CBBPresence& aRhs) const;
	IMPORT_C MBBData* CloneL(const TDesC& Name) const;
	IMPORT_C CBBPresence& operator=(const CBBPresence& aRhs);

	IMPORT_C static CBBPresence* NewL(TUint aVersion);
	IMPORT_C static CBBPresence* NewL();
	virtual ~CBBPresence();
	IMPORT_C virtual void InternalizeL(RReadStream& aStream);

	IMPORT_C void MinimalXmlL(MBBExternalizer* aBuf);
private:
	mutable TUint	iRefCount;
	TUint	iUseVersion, iCreatedVersion;
};

#endif
