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

#include "alerter.h"
#include "app_context.h"
#include "settings.h"
#include "symbian_auto_ptr.h"

#include <MdaAudioSamplePlayer.h>

#define SETTING_VIBRATE_ONLY	27
#include <eikenv.h>

#ifndef __S60V3__
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
#else
#include <centralrepository.h>
#include <profileenginesdkcrkeys.h>
#endif // 3rd

class CAlerterImpl : public CAlerter, public MContextBase, public MMdaAudioPlayerCallback
{
private:
	CAlerterImpl(MApp_context& Context);
	void ConstructL();

	virtual void ShortAlert();
	virtual void AlertUser();
	virtual void StopAlert();
	~CAlerterImpl();

	virtual void PlayAlert(TBool aShort);
	virtual void MapcInitComplete(TInt aError, 
		const TTimeIntervalMicroSeconds& aDuration);
	virtual void MapcPlayComplete(TInt aError);

	int get_current_profile();
	CMdaAudioPlayerUtility* iPlayer;
	bool	ringing;
	
	friend class CAlerter;
	friend class auto_ptr<CAlerterImpl>;
};

EXPORT_C CAlerter* CAlerter::NewL(MApp_context& Context)
{
	auto_ptr<CAlerterImpl> ret(new (ELeave) CAlerterImpl(Context));
	ret->ConstructL();
	return ret.release();
}

CAlerterImpl::CAlerterImpl(MApp_context& Context) : MContextBase(Context)
{
}

void CAlerterImpl::ConstructL()
{
}

int CAlerterImpl::get_current_profile()
{
#ifndef __WINS__
	TInt id;
#ifndef __S60V3__
	auto_ptr<CProfileAPI> profile(CProfileAPI::NewL(EFalse));
	auto_ptr<HBufC> name(HBufC::NewL(30));
	TPtr16 p=name->Des();

	if (profile->GetProfileActiveName(p, &id)!= 0)
		return -1; // Get active profile
#else
	auto_ptr<CRepository> repo(CRepository::NewL(KCRUidProfileEngine));
	TInt value;
	if (repo->Get( KProEngActiveProfile, value ) != KErrNone) return -1;
	id=value-1;
#endif

	if (id<0 || id>10) return -1;

	return id;
#else
	return 0;
#endif
}

void CAlerterImpl::AlertUser()
{
	PlayAlert(EFalse);
}

void CAlerterImpl::ShortAlert()
{
	PlayAlert(ETrue);
}


void CAlerterImpl::PlayAlert(TBool aShort)
{
#ifndef __WINS__
	if (iPlayer) return;
	TFileName tone=DataDir();

	int profile=get_current_profile();

	switch (profile) {
	case 2:
		// meeting
		if (!aShort) {
			tone.Append(_L("silent.rng"));
		} else {
			tone.Append(_L("shortsilent.rng"));
		}
		break;
	case 1:
		return;
		// silent
		break;
	default:
		{
		// general, outdoor
		TBool only_vibrate=true;
		Settings().GetSettingL(SETTING_VIBRATE_ONLY, only_vibrate);
		if (only_vibrate) {
			if (!aShort) {
				tone.Append(_L("silent.rng"));
			} else {
				tone.Append(_L("shortsilent.rng"));
			}
		} else {
			if (!aShort) {
				tone.Append(_L("tone.rng"));
			} else {
				tone.Append(_L("shorttone.rng"));
			}
		}		
		break;
		}
	}

	iPlayer=CMdaAudioPlayerUtility::NewFilePlayerL(tone, *this,
		EMdaPriorityMax,(TMdaPriorityPreference)0x01340001); /* EAknAudioPrefIncomingCall */
#endif
}

void CAlerterImpl::StopAlert()
{
#ifndef __WINS__
	if (ringing) {
		iPlayer->Stop();
		ringing=false;
	} else {
		delete iPlayer; iPlayer=0;
	}
#endif
}

CAlerterImpl::~CAlerterImpl()
{
	delete iPlayer; iPlayer=0;
}
void CAlerterImpl::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration)
{
	if (aError==KErrNone) {
		iPlayer->SetVolume(iPlayer->MaxVolume()*0.75);
		iPlayer->Play();
		ringing=true;
	} else {
		delete iPlayer; iPlayer=0;
		ringing=false;
	}
}
void CAlerterImpl::MapcPlayComplete(TInt aError)
{
	ringing=false;
	delete iPlayer; iPlayer=0;
}
