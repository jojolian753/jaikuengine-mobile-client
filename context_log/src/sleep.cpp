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

#include "sleep.h"
#include "csd_profile.h"
#include "presence_publisher.h"
#include "timeout.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"
#include "contextvariant.hrh"

#ifndef __S60V3__
class CProfileAPI : public CBase {
public:
        enum TProErrorCode
            {
            EPro0=0
           ,EPro1
            };
        IMPORT_C static CProfileAPI* NewL(TBool);
        IMPORT_C virtual ~CProfileAPI();
        IMPORT_C TProErrorCode SetProfileName( TInt aTableId );
};
#else
#  if __USE_PLUGIN_APIS__ && !defined(__WINS__)
#include <mprofileengine.h>
#  endif
#endif

class CSleepProfileImpl : public CSleepProfile, public Mlogger, public MTimeOut, public MContextBase {
public:
	TBBProfile iCurrentProfile;
	TInt iProfileBeforeSleep;
	TTime iWentToSleep;
	PresencePublisher::CPresencePublisher* iPublisher;
	CTimeOut* iTimer;

	TBool IsSleepProfile(const TBBProfile& profile) {
		return (profile.iProfileName().CompareF(_L("sleep"))==0);
	}
	CSleepProfileImpl(PresencePublisher::CPresencePublisher* aPublisher) : iPublisher(aPublisher) { }
	void ConstructL() {
		Mlogger::ConstructL( *this );
		iTimer=CTimeOut::NewL(*this);
		SubscribeL(KProfileTuple);
		SubscribeL(KIdleTuple);
		MBBData* existing=0;
		iBBSubSessionNotif->GetL( KProfileTuple, KNullDesC, existing, ETrue);
		bb_auto_ptr<MBBData> ep(existing);
		Settings().GetSettingL(SETTING_PROFILE_BEFORE_SLEEP, iProfileBeforeSleep);
		Settings().GetSettingL(SETTING_WENT_TO_SLEEP, iWentToSleep);
		if (existing) {
			const CBBSensorEvent* event=bb_cast<CBBSensorEvent>(existing);
			TBuf<256> descr;
			TRAPD(ignored, Settings().GetSettingL(SETTING_OWN_DESCRIPTION, descr));
			if (event) {
				const TBBProfile* profile=bb_cast<TBBProfile>(event->iData());
				if (profile) {
					iCurrentProfile=*profile;
					if (! IsSleepProfile(iCurrentProfile) ) {
						iProfileBeforeSleep=iCurrentProfile.iProfileId();
						Settings().WriteSettingL(SETTING_PROFILE_BEFORE_SLEEP, iProfileBeforeSleep);
						if (descr.CompareF(_L("Sleeping"))==0) {
							WokenUp();
						}
					} else {
						if (descr.CompareF(_L("Sleeping"))==0) {
							if (iPublisher) iPublisher->SuspendConnection();
						} else {
							Sleeping();
						}
					}
				}
			}
		}
	}
	~CSleepProfileImpl() {
		delete iTimer;
	}

	void expired(CBase*) {
		if (iPublisher) iPublisher->SuspendConnection();
	}
	void Sleeping() {
		iWentToSleep=GetTime();
		Settings().WriteSettingL(SETTING_OWN_DESCRIPTION_TIME, iWentToSleep);
		Settings().WriteSettingL(SETTING_OWN_DESCRIPTION, _L("Sleeping"));
		iTimer->Wait(15);
	}
	void ChangeProfile() {
#ifndef __S60V3__
		auto_ptr<CProfileAPI> api(CProfileAPI::NewL(EFalse));
		api->SetProfileName(iProfileBeforeSleep);
#else
#  if __USE_PLUGIN_APIS__ && !defined(__WINS__)
		MProfileEngine* profileEngine = 0;
		TRAPD(err, profileEngine = CreateProfileEngineL());
		if (profileEngine) {
			TRAP(err, profileEngine->SetActiveProfileL(iProfileBeforeSleep));
			profileEngine->Release();
		}
#  endif
#endif
	}
	void WokenUp() {
		iTimer->Cancel();
		TTime now; now=GetTime();
		Settings().WriteSettingL(SETTING_OWN_DESCRIPTION_TIME, now);
		Settings().WriteSettingL(SETTING_OWN_DESCRIPTION, _L("Awake"));
		if (iPublisher) iPublisher->ResumeConnection();
		if (IsSleepProfile(iCurrentProfile)) {
			ChangeProfile();
		}
	}
	virtual void NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, 
		const CBBSensorEvent& aData) {

			TBool was_sleeping=IsSleepProfile(iCurrentProfile);
			const TBBProfile* profile=bb_cast<TBBProfile>(aData.iData());
			if (profile) {
				iCurrentProfile=*profile;
				TBool is_sleeping=IsSleepProfile(*profile);
				if (was_sleeping && !is_sleeping) {
					WokenUp();
				}
				if (!was_sleeping && is_sleeping) {
					Sleeping();
				}
			}
			const TBBUserActive* active=bb_cast<TBBUserActive>(aData.iData());
			if (was_sleeping && active && active->iActive()) {
				TTime now; now=GetTime();
#if __DEV__
				if (iWentToSleep+TTimeIntervalMinutes(2) < now) {
#else
				if (iWentToSleep+TTimeIntervalMinutes(30) < now) {
#endif
					WokenUp();
				}
			}
	}
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName) { }
};

CSleepProfile* CSleepProfile::NewL(PresencePublisher::CPresencePublisher* aPublisher)
{
	auto_ptr<CSleepProfileImpl> ret(new (ELeave) CSleepProfileImpl(aPublisher));
	ret->ConstructL();
	return ret.release();
}
