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

#include "presencemaintainer.h"
#include "csd_cell.h"
#include "csd_profile.h"
#include "csd_bluetooth.h"
#include "csd_base.h"
#include "csd_idle.h"
#include "csd_gps.h"
#include "csd_presence.h"
#include "csd_current_app.h"
#include "btlist.h"
//#include "cm_autotaglist.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"
#include "break.h"
#include "csd_unread.h"
#include "contextcommon.h"

#include "app_context_impl.h"
#include "reporting.h"
void DebugLog(const TDesC& aMsg) {
	GetContext()->Reporting().DebugLog(aMsg);
}


MPresenceMaintainer::MPresenceMaintainer(MApp_context& aContext,CBTDeviceList* aBuddyList,
				         CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, 
				         CBTDeviceList *aPDABTs) : 
	MContextBase(aContext),
	iBuddyList(aBuddyList), iLaptopBTs(aLaptopBTs), iDesktopBTs(aDesktopBTs), iPDABTs(aPDABTs) { }

void MPresenceMaintainer::ConstructL()
{
	CALLSTACKITEM_N(_CL("MPresenceMaintainer"), _CL("ConstructL"));
	Mlogger::ConstructL(AppContextAccess());
	iBBSubSessionNotif->AddNotificationL(KAnySensorEvent, EFalse);
	SubscribeL(KIncomingPresence);

	if (iLaptopBTs) iLaptopBTs->AddObserver(this);
	if (iBuddyList) iBuddyList->AddObserver(this);
	if (iPDABTs) iPDABTs->AddObserver(this);
	if (iDesktopBTs) iDesktopBTs->AddObserver(this);

	iData=CBBPresence::NewL();
	iData->iGenerated()=ETrue;
	iNonsignificantTimer=CTimeOut::NewL(*this);

	MBBData *gps_event_data=0;
	TInt err;
	CC_TRAP(err, iBBSubSessionNotif->GetL(KLastKnownGpsTuple, KNullDesC, gps_event_data, ETrue));
	if (gps_event_data) {
		CALLSTACKITEM_N(_CL("MPresenceMaintainer"), _CL("gps"));
		const CBBSensorEvent* aEvent=bb_cast<CBBSensorEvent>(gps_event_data);
		if (aEvent) {
			const TGpsLine* gps=bb_cast<TGpsLine>(aEvent->iData());
			if (gps) {
				iData->iGps=*gps;
				iData->iGpsStamp()=aEvent->iStamp();
			}
		}
	}
	delete gps_event_data;

}

MPresenceMaintainer::~MPresenceMaintainer()
{
	if (iLaptopBTs) iLaptopBTs->RemoveObserver(this);
	if (iBuddyList) iBuddyList->RemoveObserver(this);
	if (iPDABTs) iPDABTs->RemoveObserver(this);
	if (iDesktopBTs) iDesktopBTs->RemoveObserver(this);

	delete iData;
	delete iNonsignificantTimer;
}

// Mlogger

void MPresenceMaintainer::DeletedL(const TTupleName& aName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("MPresenceMaintainer"), _CL("DeletedL"));

	const TTupleName& tuplename=aName;
	TBool frozenChanged=EFalse;

	if (tuplename==KCellIdTuple) {
		iData->iCellId=TBBCellId(KCell);
		iData->iCellName().Zero();
		return;
	} else if (tuplename==KCellNameTuple) {
		iData->iCellName().Zero();
		return;
	} else if (tuplename==KUserGivenContextTuple) {
		iData->iUserGiven.iDescription().Zero();
		iData->iUserGiven.iSince()=GetTime();
		//Settings().WriteSettingL(SETTING_OWN_DESCRIPTION, iData->iUserGiven.iDescription());
		//Settings().WriteSettingL(SETTING_OWN_DESCRIPTION_TIME, iData->iUserGiven.iSince());
		DebugLog(_L("PM: del usergiven"));

	} else if (tuplename==KProfileTuple) {
		iData->iProfile=TBBProfile();
		DebugLog(_L("PM: del profile"));
	} else if (tuplename==KIdleTuple ) {
		iData->iUserActive=TBBUserActive();
		DebugLog(_L("PM: del useractive"));
	} else if (tuplename==KLastKnownGpsTuple ) {
		iData->iGps=TGpsLine();
		return;
	} else if (tuplename==KBaseTuple) {
		bb_auto_ptr<TBBBaseInfo> b(new (ELeave) TBBBaseInfo());
		iData->iBaseInfo=*b;
		DebugLog(_L("PM: del base"));
	} else if (tuplename==KCalendarTuple) {
		iData->iCalendar=TBBCalendar();
		if (iFrozen) iFrozen->iCalendar=TBBCalendar();
		frozenChanged=ETrue;
		DebugLog(_L("PM: del calendar"));

	} else if (tuplename==KCountryNameTuple) {
		iData->iCountry().Zero();

		DebugLog(_L("PM: del countryname"));
	} else if (tuplename==KCityNameTuple) {
		iData->iCity().Zero();

		DebugLog(_L("PM: del cityname"));
	} else if (tuplename==KBluetoothTuple) {
		iData->iDevices->Reset();
		iData->iBuddyDevices->Reset();
		iData->iNeighbourhoodInfo=TBBNeighbourhoodInfo();
		DebugLog(_L("PM: del bluetooth"));
	} else if (tuplename==KAlarmTuple) {
		iData->iAlarm()=TTime(0);
		return;
	} else if (tuplename==KUnreadTuple) {
		iData->iUnread=TBBUnread();
		return;
	} else {
		return;
	}

	iLastUpdateTime=GetTime();
	iNonsignificantTimer->Reset();
	GotSignificantChange(frozenChanged);
}

void MPresenceMaintainer::ContentsChanged(CBase* )
{
	if (! EvaluateBt(iData->iDevices) ) return;

	DebugLog(_L("PM: contentschanged"));
	iLastUpdateTime=GetTime();
	iNonsignificantTimer->Reset();
	GotSignificantChange(EFalse, EFalse);
}

void MPresenceMaintainer::NewValueL(TUint , const TTupleName& aName, const TDesC& aSubName, 
	const TComponentName& /*aComponent*/,
	const MBBData* aData) {
	const CBBSensorEvent* e=bb_cast<CBBSensorEvent>(aData);
	if (e) {
		NewSensorEventL(aName, aSubName, *e);
	} else if (aName==KIncomingPresence) {
 		const CBBPresence* presence=bb_cast<CBBPresence>( aData );
	
		if (!presence) return;

#ifdef __WINS__
		TBool verified=presence->iPhoneNumberIsVerified();
#endif
		const CBBBtDeviceList* devs=presence->iBuddyDevices;
		if (!devs) return;
		const TBBBtDeviceInfo *node=devs->First();
		if (!node) return;
		if (iBuddyList) {
			if (! iBuddyList->ContainsDevice( node->iMAC() ) ) {
				if (presence->iFirstName().Length()>0) {
					iBuddyList->AddDeviceL( presence->iFirstName(), node->iMAC() );
				} else if (presence->iLastName().Length()>0) {
					iBuddyList->AddDeviceL( presence->iLastName(), node->iMAC() );
				} else {
					iBuddyList->AddDeviceL( aSubName, node->iMAC() );
				}
			}
		}
	}
}

void MPresenceMaintainer::NewSensorEventL(const TTupleName& aName, 
					 const TDesC& aSubName, const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("MPresenceMaintainer"), _CL("NewSensorEventL"));

	if (aEvent.iPriority()!=CBBSensorEvent::VALUE || !aEvent.iData()) return;

	const TTupleName& tuplename=aName;
	TTime time=aEvent.iStamp();
	TBool frozenChanged=EFalse;
	TBool quickrate=EFalse;

	if (tuplename==KCellIdTuple) {
		const TBBCellId* cell=bb_cast<TBBCellId>(aEvent.iData());
		if (!cell) return;
		iData->iCellId=*cell;
		return;
	} else if (tuplename==KGivenCellNameTuple) {
		iNewNameComing=ETrue;
		return;
	} else if (tuplename==KCellNameTuple) {
		TBuf<50> prev_name=iData->iCellName().Left(50);
		iData->iCellName().Zero();
		const TBBLongString* name=bb_cast<TBBLongString>(aEvent.iData());
		if (name) iData->iCellName()=name->Value();
		if (iNewNameComing) {
			DebugLog(_L("PM: newname"));
			iNewNameComing=EFalse;
		} else {
			if ( (!name && prev_name.Length()>0) || 
					name->Value().Left(50).Compare(prev_name)) {

				if ( iData->iBaseInfo.iCurrent.IsSet() &&
					 iData->iBaseInfo.iCurrent.iLeft()==TTime(0)) {
						// we are in a base
						return;
				}
				TTime now; now=GetTime();
				DebugLog(_L("PM: cellname, timer"));
				if ( iLastUpdateTime + TTimeIntervalMinutes(10) > now ) {
					iNonsignificantTimer->WaitMax( 10 );
				} else {
					iNonsignificantTimer->WaitMax( 10*60 );
				}
			}
			return;
		}
		DebugLog(_L("PM: cellname"));
	} else if (tuplename==KUserGivenContextTuple) {
		const TBBUserGiven * user_given=bb_cast<TBBUserGiven>(aEvent.iData());
		if (!user_given) return;
		quickrate=ETrue;
		iData->iUserGiven=*user_given;
		DebugLog(_L("PM: usergiven"));
		//Settings().WriteSettingL(SETTING_OWN_DESCRIPTION, user_given->iDescription());
		//Settings().WriteSettingL(SETTING_OWN_DESCRIPTION_TIME, user_given->iSince());

	} else if (tuplename==KProfileTuple) {
		const TBBProfile* profile=bb_cast<TBBProfile>(aEvent.iData());
		if (profile) {
			if (iData->iProfile.Equals(profile)) return;

			quickrate=ETrue;
			iData->iProfile=*profile;
		} else {
			iData->iProfile=TBBProfile();
		}
		DebugLog(_L("PM: profile"));
	} else if (tuplename==KIdleTuple ) {
		const TBBUserActive* act=bb_cast<TBBUserActive>(aEvent.iData());
		if (!act) return;
		iIdle=!(act->iActive());
		EvaluateSuspendL();
		if (iData->iUserActive.Equals(act)) return;
		iData->iUserActive=*act;
		DebugLog(_L("PM: useractivity"));
	} else if (tuplename==KCurrentAppTuple) {
		const TBBCurrentApp* app=bb_cast<TBBCurrentApp>(aEvent.iData());
		if ( app && app->iUid()==KUidContextContacts.iUid ) {
			iInJaiku=ETrue;
		} else {
			iInJaiku=EFalse;
		}
		EvaluateSuspendL();
		return;
	} else if (tuplename==KLastKnownGpsTuple ) {
		const TGpsLine* gps=bb_cast<TGpsLine>(aEvent.iData());
		if (!gps) return;
		iData->iGps=*gps;
		iData->iGpsStamp()=aEvent.iStamp();
		return;
	} else if (tuplename==KBaseTuple) {
		const TBBBaseInfo* basep=bb_cast<TBBBaseInfo>(aEvent.iData());
		if (basep) {
			bb_auto_ptr<TBBBaseInfo> base( bb_cast<TBBBaseInfo>(basep->CloneL(basep->Name())) );

			if ( base->iPreviousStay.iLeft() < iUnfreezeTime ) {
			
				base->iPreviousStay.iBaseName().Zero();
				base->iPreviousStay.iLeft()=iUnfreezeTime;
			}
			if ( base->iPreviousVisit.iLeft() < iUnfreezeTime ) {
				base->iPreviousVisit.iBaseName().Zero();
				base->iPreviousVisit.iLeft()=iUnfreezeTime;
			}
			if ( base->iCurrent.iEntered() < iUnfreezeTime ) {
				base->iCurrent.iEntered()=iUnfreezeTime;
			}
			if (iData->iBaseInfo.iCurrent.IsSet() &&
				iData->iBaseInfo.iCurrent.iLeft()==TTime(0)) {
					iNonsignificantTimer->Reset();
			}
			if (iData->iBaseInfo.Equals(base.get())) return;
			iData->iBaseInfo=*base;
		} else {
			bb_auto_ptr<TBBBaseInfo> base(new (ELeave) TBBBaseInfo());
			iData->iBaseInfo=*base;
		}
		DebugLog(_L("PM: base"));
	} else if (tuplename==KCalendarTuple) {
		const TBBCalendar* cal=bb_cast<TBBCalendar>(aEvent.iData());
		if (!iFrozen) {
			if (cal) {
				if (iData->iCalendar.Equals(cal)) return;
				iData->iCalendar=*cal;
			} else {
				iData->iCalendar=TBBCalendar();
			}
		} else {
			if (cal) {
				iData->iCalendar=*cal;
				if (iFrozen->iCalendar.Equals(cal)) return;
				iFrozen->iCalendar=*cal;
			} else {
				iFrozen->iCalendar=TBBCalendar();
			}
			frozenChanged=ETrue;
		}
		DebugLog(_L("PM: calendar"));

	} else if (tuplename==KCountryNameTuple) {
		const TBBShortString* country=bb_cast<TBBShortString>(aEvent.iData());
		if (country) {
			if (iData->iCountry == *country) return;
			iData->iCountry()=(*country)();
		} else {
			iData->iCountry().Zero();
		}
		DebugLog(_L("PM: country"));
	} else if (tuplename==KCityNameTuple) {
		const TBBFixedLengthStringBase* City=bb_cast<TBBShortString>(aEvent.iData());
		if (!City) City=bb_cast<TBBLongString>(aEvent.iData());
		if (City) {
			if (iData->iCity == *City) return;
			iData->iCity()=City->Value();
		} else {
			iData->iCity().Zero();
		}
		DebugLog(_L("PM: city"));
	} else if (tuplename==KAlarmTuple) {
		const TBBTime* alarm=bb_cast<TBBTime>(aEvent.iData());
		if (alarm) {
			if (iData->iAlarm()==(*alarm)()) return;
			iData->iAlarm()=(*alarm)();
		} else {
			iData->iAlarm()=TTime(0);
		}
		return; // alarm not propagated
		DebugLog(_L("PM: alarm"));
	} else if (tuplename==KUnreadTuple) {
		const TBBUnread* unread=bb_cast<TBBUnread>(aEvent.iData());
		if (unread) {
			if (iData->iUnread==*unread) return;
			iData->iUnread=*unread;
		} else {
			iData->iUnread=TBBUnread();
		}
		return; // unread not propagated
		DebugLog(_L("PM: unread"));
	} else if (tuplename==KBluetoothTuple) {
		const CBBBtDeviceList* devs=bb_cast<CBBBtDeviceList>(aEvent.iData());
		if (!devs) return;

		iData->SetDevices(devs);
		if (! EvaluateBt(devs) ) return;
		DebugLog(_L("PM: bluetooth"));
	} else {
		/*TBuf<30> msg=_L("unhandled tuple ");
		msg.AppendNum(tuplename.iModule.iUid, EHex);
		msg.Append(_L(" "));
		msg.AppendNum(tuplename.iId);
		DebugLog(msg);*/
		return;
	}

	iLastUpdateTime=GetTime();
	iNonsignificantTimer->Reset();
	GotSignificantChange(frozenChanged, quickrate);
}

_LIT(KBuddyDev, "buddy");

TBool MPresenceMaintainer::EvaluateBt(const CBBBtDeviceList* devs)
{
	TUint prev_buddies=iData->iNeighbourhoodInfo.iBuddies();
	TUint prev_others=iData->iNeighbourhoodInfo.iOtherPhones();
	TUint prev_laptops=iData->iNeighbourhoodInfo.iLaptops();
	TUint prev_desktops=iData->iNeighbourhoodInfo.iDesktops();
	TUint prev_pdas=iData->iNeighbourhoodInfo.iPDAs();
	iData->iNeighbourhoodInfo.iBuddies()=0;
	iData->iNeighbourhoodInfo.iOtherPhones()=0;
	iData->iNeighbourhoodInfo.iLaptops()=0;
	iData->iNeighbourhoodInfo.iDesktops()=0;
	iData->iNeighbourhoodInfo.iPDAs()=0;

	iData->iBuddyDevices->Reset();

	TInt num=0;
	for (const TBBBtDeviceInfo *node=devs->First(); node; node=devs->Next()) {
		if (num==0) {
			iData->iBuddyDevices->AddItemL( bb_cast<TBBBtDeviceInfo>(node->CloneL(KCSDBt)) );
			num++;
			continue;
		}
		num++;
		if ( iBuddyList && iBuddyList->ContainsDevice(node->iMAC()) ) {
			iData->iBuddyDevices->AddItemL( bb_cast<TBBBtDeviceInfo>(node->CloneL(KCSDBt)) );
			++iData->iNeighbourhoodInfo.iBuddies();
		} else if ( iLaptopBTs && iLaptopBTs->ContainsDevice(node->iMAC()) ) {
			++iData->iNeighbourhoodInfo.iLaptops();
		} else if ( iDesktopBTs && iDesktopBTs->ContainsDevice(node->iMAC()) ) {
			++iData->iNeighbourhoodInfo.iDesktops();
		} else if ( iPDABTs && iPDABTs->ContainsDevice(node->iMAC()) ) {
			++iData->iNeighbourhoodInfo.iPDAs();
		} else if ( node->iMajorClass() == 2 ) {
			++iData->iNeighbourhoodInfo.iOtherPhones();
		} 
	}
#ifdef __WINS__
	iData->iNeighbourhoodInfo.iLaptops()=1;
	iData->iNeighbourhoodInfo.iDesktops()=1;
	iData->iNeighbourhoodInfo.iPDAs()=1;
#endif
	if (iData->iNeighbourhoodInfo.iBuddies()==prev_buddies &&
		iData->iNeighbourhoodInfo.iOtherPhones()==prev_others &&
		iData->iNeighbourhoodInfo.iLaptops()==prev_laptops &&
		iData->iNeighbourhoodInfo.iDesktops()==prev_desktops &&
		iData->iNeighbourhoodInfo.iPDAs()==prev_pdas) return EFalse;

	return ETrue;
}

void MPresenceMaintainer::add_cellid_name(const TBBCellId& cellid, const TDesC& name)
{
	if (cellid==iData->iCellId) iData->iCellName()=name;
}

void MPresenceMaintainer::expired(CBase* Source)
{
	DebugLog(_L("PM: non-significant timer"));
	iLastUpdateTime=GetTime();
	GotSignificantChange(EFalse);
}

const CBBPresence* MPresenceMaintainer::Data() const
{
	return iData;
}

CPresenceMaintainer::CPresenceMaintainer(MApp_context& aContext, CBTDeviceList* aBuddyList,
				  CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, 
				  CBTDeviceList *aPDABTs) :
	MPresenceMaintainer(aContext, aBuddyList, aLaptopBTs, aDesktopBTs,
		aPDABTs) { }

class CPresenceMaintainerImpl : public CPresenceMaintainer {
private:
	virtual void GotSignificantChange(TBool, TBool) {
		
	}
	CPresenceMaintainerImpl(MApp_context& aContext, CBTDeviceList* aBuddyList,
					  CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, 
					  CBTDeviceList *aPDABTs) :
	CPresenceMaintainer(aContext, aBuddyList, aLaptopBTs, aDesktopBTs,
		aPDABTs) { }

	friend class CPresenceMaintainer;
	friend class auto_ptr<CPresenceMaintainerImpl>;
	friend class CJaikuTest;
};

CPresenceMaintainer* CPresenceMaintainer::NewL(MApp_context& aContext, CBTDeviceList* aBuddyList,
				  CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, 
				  CBTDeviceList *aPDABTs)
{
	auto_ptr<CPresenceMaintainerImpl> ret(new (ELeave) CPresenceMaintainerImpl(aContext, aBuddyList, aLaptopBTs, aDesktopBTs,
		aPDABTs) );
	ret->ConstructL();
	return ret.release();
}
