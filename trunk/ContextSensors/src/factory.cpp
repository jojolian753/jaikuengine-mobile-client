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

#include "bbtypes.h"
#include "csd_bluetooth.h"
#include "csd_cell.h"
#include "csd_current_app.h"
#include "csd_event.h"
#include "csd_gps.h"
#include "csd_profile.h"
#include "csd_system.h"
#include "csd_battery.h"
#include "csd_idle.h"
#include "csd_presence.h"
#include "csd_loca.h"
#include "csd_lookup.h"
#include "csd_cellnaming.h"
#include "csd_userpic.h"
#include "csd_sentinvite.h"
#include "csd_servermessage.h"
#include "csd_buildinfo.h"
#include "csd_feeditem.h"
#include "csd_streamdata.h"
#include "csd_streamcomment.h"
#include "csd_threadrequest.h"
#include "csd_connectionstate.h"

_LIT(KDefaultStringSep, " ");

class TConcreteDataFactory : public MBBDataFactory {
	virtual MBBData* CreateBBDataL(const TTypeName& aType, const TDesC& aName, MBBDataFactory* aTopLevelFactory) {

		if (aType==KBluetoothInfoType) {
			return new (ELeave) TBBBtDeviceInfo();
		} else if (aType==KBluetoothListType) {
			return CBBBtDeviceList::NewL();
		} else if (aType==KBluetoothNameType) {
			return new (ELeave) TBBBluetoothName(aName);
		} else if (aType==KBluetoothAddrType) {
			return new (ELeave) TBBBluetoothAddress(aName);
		} else if( aType==KBluetoothNeighboursType) {
			return new (ELeave) TBBNeighbourhoodInfo;
		} else if( aType==KBluetoothNeighboursType1) {
			return new (ELeave) TBBNeighbourhoodInfo(1);

		} else if (aType==KCellIdType) {
			return new (ELeave) TBBCellId(KCell);
		} else if (aType==KShortNetworkNameType) {
			return new (ELeave) TBBShortNetworkName(aName);
		
		} else if (aType==KCurrentAppType) {
			return new (ELeave) TBBCurrentApp();
		
		} else if (aType==KEventType) {
			return new (ELeave) CBBSensorEvent(aName, KNoTuple, aTopLevelFactory);
		} else if (aType==KTupleType) {
			return new (ELeave) CBBTuple(aTopLevelFactory);
		} else if( aType==KSubNameType) {
			return new (ELeave) TBBTupleSubName(aName);
		} else if( aType==KTupleMetaType) {
			return new (ELeave) TBBTupleMeta(0, 0, KNullDesC);

		} else if( aType==KGpsLineType ) {
			return new (ELeave) TGpsLine();

		} else if( aType==KProfileType ) {
			return new (ELeave) TBBProfile();

		} else if ( aType==KSysEventType) {
			return new (ELeave) TBBSysEvent(aName);

		} else if ( aType==KBatteryType) {
			return new (ELeave) TBBBattery(aName);

		} else if ( aType==KIdleType) {
			return new (ELeave) TBBUserActive;

		} else if ( aType==KPresenceType) {
			return CBBPresence::NewL();
		} else if ( aType==KPresenceType1) {
			return CBBPresence::NewL(1);
		} else if ( aType==KPresenceType2) {
			return CBBPresence::NewL(2);
		} else if ( aType==KPresenceType3) {
			return CBBPresence::NewL(3);
		} else if ( aType==KPresenceType4) {
			return CBBPresence::NewL(4);
		} else if ( aType==KPresenceType5) {
			return CBBPresence::NewL(5);
		} else if ( aType==KPresenceType6) {
			return CBBPresence::NewL(6);

		} else if ( aType==KUserGivenType) {
			return new (ELeave) TBBUserGiven(aName);
		
		} else if ( aType==KBaseInfoType) {
			return new (ELeave) TBBBaseInfo();
		} else if ( aType==KBaseVisitType) {
			return new (ELeave) TBBBaseVisit(aName);

		} else if ( aType==KLocationType) {
			return new (ELeave) TBBLocation();

		} else if ( aType==KLocaMessageStatusType) {
			return new (ELeave) TBBLocaMsgStatus;

		} else if ( aType==KCalendarEventType) {
			return new (ELeave) TBBCalendarEvent(aName);
		} else if ( aType==KCalendarType1) {
			return new (ELeave) TBBCalendar(1);
		} else if ( aType==KCalendarType) {
			return new (ELeave) TBBCalendar;

		} else if ( aType==KUnreadType) {
			return new (ELeave) TBBUnread;

		} else if ( aType==KLookupType1) {
			return CBBLookup::NewL(1);
		} else if ( aType==KLookupType) {
			return CBBLookup::NewL();

		} else if ( aType==KCellNamingType ) {
			return new (ELeave) TBBCellNaming;

		} else if ( aType==KUserPicType ) {
			return new (ELeave) CBBUserPic;
		} else if ( aType==KSentInviteType ) {
			return new (ELeave) TBBSentInvite;
		} else if ( aType==KServerMessageType) {
			return new (ELeave) TBBServerMessage;
		} else if ( aType==KBuildInfoType) {
			return new (ELeave) TBBBuildInfo(aName);
		} else if ( aType==KFeedItemType) {
			return new (ELeave) CBBFeedItem;

		} else if ( aType==KStreamDataType) {
			return new (ELeave) CBBStreamData;
		} else if ( aType==KStreamCommentType) {
			return new (ELeave) CBBStreamComment;
			
		} else if ( aType==KThreadRequestType) {
			return new (ELeave) TBBThreadRequest;
			
		} else if ( aType==KConnectionStateType) {
			return new (ELeave) CBBConnectionState;
			
		} else {
			User::Leave(KErrNotSupported);
		}

		return 0;
	}
	virtual ~TConcreteDataFactory() { }
	virtual void ConstructL() { }
};

EXPORT_C MBBDataFactory* CreateDataFactory()
{
	return new TConcreteDataFactory;
}
