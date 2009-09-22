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

// Concepts:
// !Profile API!
// !Getting current profile settings!
#include "log_profile.h"
#include "symbian_auto_ptr.h"
#include <e32std.h>
#define INTERVAL 30

#ifndef __S60V3__
#  ifndef __S60V2__
#include <saclient.h>
#  endif
#include <profileapi.h>
#ifndef NO_PROFILEAPI_H
#else
typedef TInt32 TContactItemId;
class CProfileAPI : public CBase {
public:
	enum TProErrorCode
	    {
	    EPro0=0
	   ,EPro1
	    };
	IMPORT_C static CProfileAPI* NewL(TBool);
	IMPORT_C virtual ~CProfileAPI();
	IMPORT_C TProErrorCode GetProfileActiveName(TPtr, TInt*);
	IMPORT_C TProErrorCode GetProfileMultiData( TDes&, TDes&, TInt&,
					       TInt&, TBool&, TInt&, CArrayFixFlat<TContactItemId>*,
					       TInt&, TInt);
};
#endif
#include "hal.h"
#else
#include <centralrepository.h>
#include <profileenginesdkcrkeys.h>
#endif

Clog_profile::Clog_profile(MApp_context& Context) : 
#if defined(__S60V3__) || !defined(__S60V2__)
	CCheckedActive(CCheckedActive::EPriorityIdle, _L("Clog_profile")), 
#endif
	Mlog_base_impl(Context, KProfile, KProfileTuple, 5*24*60*60) { }

void Clog_profile::ConstructL()
{
	CALLSTACKITEM_N(_CL("Clog_profile"), _CL("ConstructL"));

	Mlog_base_impl::ConstructL();
#ifndef __S60V3__
#  ifndef __S60V2__
	iSysAgent=new (ELeave) RSystemAgent;
	aConditions=new (ELeave) CArrayFixFlat<TSysAgentCondition>(sizeof(TSysAgentCondition));
	aConditions->SetReserveL(1);
#  else
	iSettings=new (ELeave) CArrayFixFlat<SettingInfo::TSettingID>(sizeof(SettingInfo::TSettingID));
	iSettings->SetReserveL(4);
	iSettingInfo=CSettingInfo::NewL(this);
#  endif
	TInt uid;
#  ifdef __S60V2__
	iUserCreatableProfiles=ETrue;
	HAL::Get(HAL::EMachineUid, uid);
	if (uid==0x101FB3DD ||
		uid==0x101F4FC3 ||
		uid==0x101F466A ) {
		/* on 1st ed and 2nd ed no FP only: 7650, 3650, 3660 and 6600 */

		iUserCreatableProfiles=EFalse;
	}
#  else
	CActiveScheduler::Add(this);
#  endif
#else
	iRepository=CRepository::NewL(KCRUidProfileEngine);
	CActiveScheduler::Add(this);
#endif
	
	listen_for_notification();

	post_new_value(get_value());
}

void Clog_profile::listen_for_notification()
{
#if defined(__S60V3__) || !defined(__S60V2__)
	CALLSTACKITEM_N(_CL("Clog_profile"), _CL("listen_for_notification"));

	Cancel();

	get_profile_via_profilapi();
	prev_profile=iProfile;
#ifndef __S60V3__

	if (! iAgentIsOpen ) {
		TInt ret=iSysAgent->Connect();
		if (ret!=KErrNone) {
			iWait->Wait(5);
			return;
		} else {
			iAgentIsOpen=true;
		}
	}

	TSysAgentCondition cond(KUidProfile, prev_profile.iProfileId(), ESysAgentNotEquals);
	if (aConditions->Count()==0) {
		aConditions->AppendL(cond);
	} else {
		aConditions->At(0)=cond;
	}
	TInt ret=iSysAgent->NotifyOnCondition(*aConditions, iStatus);
#else
	TInt ret=iRepository->NotifyRequest(0, 0, iStatus);
#endif
	if (ret==KErrNone) {
		SetActive();
	} else {
		iStatus=ret;
		iWait->Wait(5);
	}
#else
	get_profile_via_profilapi();
	prev_profile=iProfile;

	iSettings->Reset();
	iSettings->AppendL(SettingInfo::EActiveProfile);
	iSettings->AppendL(SettingInfo::ERingingType);
	iSettings->AppendL(SettingInfo::ERingingVolume);
	iSettings->AppendL(SettingInfo::EVibratingAlert);
	TInt err=iSettingInfo->NotifyChanges(*iSettings);
	if (err!=KErrNone) {
		iWait->Wait(5);
	}
#endif
}

void Clog_profile::expired(CBase* )
{
	CALLSTACKITEM_N(_CL("Clog_profile"), _CL("expired"));
#if defined(__S60V3__) || !defined(__S60V2__)
	CheckedRunL();
#else
	iSettings->Reset();
	iSettings->AppendL(SettingInfo::EActiveProfile);
	iSettings->AppendL(SettingInfo::ERingingType);
	iSettings->AppendL(SettingInfo::ERingingVolume);
	iSettings->AppendL(SettingInfo::EVibratingAlert);
	TInt err=iSettingInfo->NotifyChanges(*iSettings);
	if (err!=KErrNone) {
		iWait->Wait(5);
	}
#endif
}

EXPORT_C Clog_profile* Clog_profile::NewL(MApp_context& Context)
{
	CALLSTACKITEMSTATIC_N(_CL("Clog_profile"), _CL("NewL"));

	auto_ptr<Clog_profile> ret(new (ELeave) Clog_profile(Context));
	ret->ConstructL();
	return ret.release();
}

#if defined(__S60V3__) || !defined(__S60V2__)
void Clog_profile::DoCancel()
{
	CALLSTACKITEM_N(_CL("Clog_profile"), _CL("DoCancel"));

#ifndef __S60V3__
	iSysAgent->NotifyEventCancel();
#else
	iRepository->NotifyCancel(0, 0);
#endif
}
#endif

EXPORT_C Clog_profile::~Clog_profile()
{
	CALLSTACKITEM_N(_CL("Clog_profile"), _CL("~Clog_profile"));

	delete iWait;
#if defined(__S60V3__) || !defined(__S60V2__)
	Cancel();
#endif

#ifndef __S60V3__
#  ifndef __S60V2__
	if (iAgentIsOpen) iSysAgent->Close();
	delete iSysAgent;
	delete aConditions;
#  else
	delete iSettingInfo;
	delete iSettings;
#  endif
#else
	delete iRepository;
#endif

}

#if defined(__S60V3__) || !defined(__S60V2__)
TInt Clog_profile::CheckedRunError(TInt /*aError*/)
{
	CALLSTACKITEM_N(_CL("Clog_profile"), _CL("CheckedRunError"));

	if (iErrorCount>10) {
		return -1010;
	}

	Cancel();
	listen_for_notification();
	return KErrNone;
}

void Clog_profile::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("Clog_profile"), _CL("CheckedRunL"));

	if (iStatus!=KErrNone) {
		if (iErrorCount>10) {
			User::Leave(-1010);
		}
		iErrorCount++;
#ifndef __S60V3__
		if (iAgentIsOpen) {
			iSysAgent->Close();
			iAgentIsOpen=true;
		}
#else
		delete iRepository; iRepository=0;
		iRepository=CRepository::NewL(KCRUidProfileEngine);
#endif
	} else {
		iErrorCount=0;
	}

	iProfile.iProfileId()=get_profile_via_profilapi();

	if (prev_profile==iProfile) {
		listen_for_notification();
		return;
	}

	prev_profile=iProfile;

	post_new_value(&iProfile);

	listen_for_notification();
}
#else
void Clog_profile::HandleNotificationL(SettingInfo::TSettingID aID, const TDesC &  aNewValue )
{
	iProfile.iProfileId()=get_profile_via_profilapi();
	if (prev_profile==iProfile) {
		return;
	}

	prev_profile=iProfile;
	post_new_value(&iProfile);
}

#endif

TInt Clog_profile::get_profile_via_profilapi()
{
	CALLSTACKITEM_N(_CL("Clog_profile"), _CL("get_profile_via_profilapi"));

	TInt profileUid=0;
#ifndef __S60V3__
#  if !defined(__WINS__) || defined(__S60V2__)

	auto_ptr<CProfileAPI> profile(CProfileAPI::NewL(EFalse));
	auto_ptr<HBufC> name(HBufC::NewL(64));
	//name->Des().FillZ(30); name->Des().Zero();
	TPtr16 p=name->Des();
	if (profile->GetProfileActiveName(p, &profileUid)!= 0)
		return -1; // Get active profile


#    ifndef __S60V2__
	if (profileUid<0 || profileUid>10) return -1;
#    else
	if (profileUid<0 || (!iUserCreatableProfiles && profileUid>6)) 
		return -1;
#    endif
	iProfile.iProfileId()=profileUid;
	iProfile.iProfileName()=(*name).Left(iProfile.iProfileName().MaxLength());
	
	TFileName dummy1, dummy2;
	TInt dummy3, dummy4;
	auto_ptr<CArrayFixFlat<TContactItemId> > dummy6(new (ELeave) CArrayFixFlat<TContactItemId>(1));
	profile->GetProfileMultiData(dummy1, dummy2, iProfile.iRingingType(), iProfile.iRingingVolume(), iProfile.iVibra(), 
		dummy3, dummy6.get(),
		dummy4, -1);
#  else
	iProfile.iVibra()=true;
	iProfile.iRingingType()=3;
	iProfile.iRingingVolume()=2;
	iProfile.iProfileName()=_L("Genéräl");
#  endif
#else
	iProfile.iProfileName().Zero();
	User::LeaveIfError(iRepository->Get(KProEngActiveProfile, profileUid));
	iProfile.iProfileId()=profileUid;
	User::LeaveIfError(iRepository->Get(KProEngActiveRingingType, iProfile.iRingingType()));
	User::LeaveIfError(iRepository->Get(KProEngActiveRingingVolume, iProfile.iRingingVolume()));

	TUint32 profileKey= profileUid << 24;
	/*if (profileUid >= 30) {
		profileKey = 0x1e000000;
	}*/
	TInt err_vibra=iRepository->Get(profileKey | 0xA, iProfile.iVibra());
	TInt err_name=iRepository->Get(profileKey | 0x2, iProfile.iProfileName());
	if (iProfile.iProfileName().Length()<=1) {
		//FIXME:LOCALIZATION
		switch(profileUid) {
		case 0:
			iProfile.iProfileName()=_L("General");
			break;
		case 1:
			iProfile.iProfileName()=_L("Silent");
			break;
		case 2:
			iProfile.iProfileName()=_L("Meeting");
			break;
		case 3:
			iProfile.iProfileName()=_L("Outdoor");
			break;
		case 4:
			iProfile.iProfileName()=_L("Pager");
			break;
		case 5:
			iProfile.iProfileName()=_L("Offline");
			break;
		default:
			iProfile.iProfileName()=_L("Unknown");
			break;
		}
	}

#endif
	return profileUid;
}

EXPORT_C const CBBSensorEvent& Clog_profile::get_value()
{
	iEvent.iData.SetValue(&iProfile);
	iEvent.iStamp()=GetTime();

	return iEvent;
}

EXPORT_C int Clog_profile::get_current_profile() { return prev_profile.iProfileId(); }
