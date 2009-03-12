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

#include "cn_networkerror.h"
#include "app_context.h"
#include "app_context_impl.h"
#include "settings.h"

#define SETTING_IDENTIFICATION_ERROR 90
#define SETTING_LAST_CONNECTION_SUCCESS 91
#define SETTING_LAST_CONNECTION_ATTEMPT 92
#define SETTING_LAST_CONNECTION_ERROR 93
#define SETTING_LAST_CONNECTION_REQUEST 94
#define SETTING_PRESENCE_ENABLE 9
#define SETTING_LATEST_CONNECTION_REQUEST 95

EXPORT_C void CNetworkError::ConnectionRequestedL()
{
	MSettings& sett=GetContext()->Settings();
	TTime previous=TTime(0);
	sett.GetSettingL(SETTING_LAST_CONNECTION_REQUEST, previous);
	if (previous==TTime(0)) {
		sett.WriteSettingL(SETTING_LAST_CONNECTION_REQUEST, GetTime());
	}
	sett.WriteSettingL(SETTING_LATEST_CONNECTION_REQUEST, GetTime());
}

EXPORT_C void CNetworkError::ConnectionSuccessL()
{
	MSettings& sett=GetContext()->Settings();
	sett.WriteSettingL(SETTING_LAST_CONNECTION_SUCCESS, GetTime());
	sett.WriteSettingL(SETTING_LAST_CONNECTION_ATTEMPT, TTime(0));
	sett.WriteSettingL(SETTING_LAST_CONNECTION_REQUEST, TTime(0));
	sett.WriteSettingL(SETTING_LATEST_CONNECTION_REQUEST, TTime(0));
}

EXPORT_C void CNetworkError::ResetSuccessL()
{
	MSettings& sett=GetContext()->Settings();
	sett.WriteSettingL(SETTING_LAST_CONNECTION_ATTEMPT, TTime(0));
	sett.WriteSettingL(SETTING_LAST_CONNECTION_SUCCESS, TTime(0));
}

EXPORT_C void CNetworkError::TryingConnectionL()
{
	MSettings& sett=GetContext()->Settings();
	TTime previous=TTime(0);
	sett.GetSettingL(SETTING_LAST_CONNECTION_ATTEMPT, previous);
	if (previous==TTime(0)) {
		sett.WriteSettingL(SETTING_LAST_CONNECTION_ATTEMPT, GetTime());
	}
}

EXPORT_C void CNetworkError::ResetRequestedL()
{
	MSettings& sett=GetContext()->Settings();
	sett.WriteSettingL(SETTING_LAST_CONNECTION_REQUEST, TTime(0));
	sett.WriteSettingL(SETTING_LATEST_CONNECTION_REQUEST, TTime(0));
}

EXPORT_C void CNetworkError::ResetTryingL()
{
	MSettings& sett=GetContext()->Settings();
	sett.WriteSettingL(SETTING_LAST_CONNECTION_ATTEMPT, TTime(0));
}

#ifdef __DEV__
#define DEBUG_NWERROR 1
#include "reporting.h"
#endif

void GetCurrentStateAndTimeL(TBool& aIsError,
	MNetworkErrorObserver::TErrorDisplay& aMode, TDes& aMessageInto,
	TInt& aWaitSecondsForNext)
{
	MSettings& sett=GetContext()->Settings();
	TTime tried_since=TTime(0);
	TTime last_request=TTime(0), latest_request=TTime(0);
	TTime success=TTime(0);
	TBool enabled=ETrue;
	aWaitSecondsForNext=0;

	sett.GetSettingL(SETTING_LAST_CONNECTION_SUCCESS, success);
	sett.GetSettingL(SETTING_LAST_CONNECTION_ATTEMPT, tried_since);
	sett.GetSettingL(SETTING_LAST_CONNECTION_REQUEST, last_request);
	sett.GetSettingL(SETTING_LATEST_CONNECTION_REQUEST, latest_request);
	sett.GetSettingL(SETTING_PRESENCE_ENABLE, enabled);
#ifdef DEBUG_NWERROR
	MReporting& rep=GetContext()->Reporting();
	{
		TInt prints[] = { SETTING_LAST_CONNECTION_SUCCESS, SETTING_LAST_CONNECTION_ATTEMPT,
			SETTING_LAST_CONNECTION_REQUEST, SETTING_LATEST_CONNECTION_REQUEST,
			SETTING_PRESENCE_ENABLE, -1 };
		rep.UserErrorLog(_L("DEBUG_NWERROR"));
		TBuf<40> msg;
		for (TInt* p=prints; *p>0; p++) {
			msg.Zero(); msg=_L("SETT: ");
			msg.AppendNum(*p);
			rep.UserErrorLog(msg);
			msg.Zero();
			if (sett.GetSettingL(*p, msg)) {
				rep.UserErrorLog(msg);
			} else {
				rep.UserErrorLog(_L("not set"));
			}
		}
	}
#endif
	
	if (!enabled) {
		aIsError=EFalse;
		aMessageInto.Zero();
		aWaitSecondsForNext=0;
		return;
	}
	
	TTime now=GetTime();
	TTimeIntervalSeconds allowed_after_request(70);
	
	aMode=MNetworkErrorObserver::EAmbient;
	if ( success==TTime(0) && last_request!=TTime(0) && last_request + allowed_after_request < now ) {
		aIsError=ETrue;
		aMode=MNetworkErrorObserver::EIntrusive;
	} else if ( last_request + allowed_after_request < now && last_request!=TTime(0) && 
			latest_request!=TTime(0) && success < latest_request 
			&& success + TTimeIntervalMinutes(5) < now ) {
		aIsError=ETrue;
	} else if ( tried_since!=TTime(0) && tried_since +TTimeIntervalMinutes(30) < now ) {
		aIsError=ETrue;
	} else {
		aIsError=EFalse;
	}

	TTimeIntervalSeconds wait; TBool do_wait=EFalse;
	
	if ( (success==TTime(0) || success < last_request) && 
			last_request!=TTime(0) && last_request + allowed_after_request > now ) {
		now.SecondsFrom( last_request + allowed_after_request, wait);
		do_wait=ETrue;
	} else if ( success < latest_request && success + TTimeIntervalMinutes(5) > now ) {
		now.SecondsFrom( success + TTimeIntervalMinutes(5), wait);
		do_wait=ETrue;
	} else if ( tried_since!=TTime(0) && tried_since + TTimeIntervalMinutes(30) < now ) {
		now.SecondsFrom( tried_since + TTimeIntervalMinutes(30), wait);
		do_wait=ETrue;
	}
	if (do_wait) {
		aWaitSecondsForNext=wait.Int();
		// can't be bothered to test which way the SecondsFrom goes
		if (aWaitSecondsForNext<0) aWaitSecondsForNext*=-1;
		if (aWaitSecondsForNext==0) aWaitSecondsForNext=1;
	} else {
		aWaitSecondsForNext=0;
	}
	
#ifdef DEBUG_NWERROR
	{
		TBuf<100> msg=_L("RES: ");
		if (aIsError) msg.Append(_L(" ERR "));
		else msg.Append(_L(" NOERR "));
		msg.Append(_L(" WAIT "));
		msg.AppendNum(aWaitSecondsForNext);
		rep.UserErrorLog(msg);
	}
#endif	

	if (aIsError) {
		aMessageInto.Zero();
		sett.GetSettingL(SETTING_LAST_CONNECTION_ERROR, aMessageInto);
		if (aMessageInto.Length()==0) {
			aMessageInto=_L("Cannot connect to Jaiku");
		}
	}
}

EXPORT_C void CNetworkError::StaticGetCurrentStateL(TBool& aIsError,
	MNetworkErrorObserver::TErrorDisplay& aMode, TDes& aMessageInto)
{
	TInt ignored;
	GetCurrentStateAndTimeL(aIsError, aMode, aMessageInto, ignored);
}

#include "timeout.h"

class CNetworkErrorImpl : public CNetworkError, public MContextBase,
	public MSettingListener, public MTimeOut {
public:
	enum TState { EReset, EError, ESuccess };
	TState iState;
	CNetworkErrorImpl(MNetworkErrorObserver& aObserver) : iObserver(aObserver) { }
	void ConstructL() {
		Settings().NotifyOnChange(SETTING_LAST_CONNECTION_SUCCESS, this);
		Settings().NotifyOnChange(SETTING_LAST_CONNECTION_ATTEMPT, this);
		Settings().NotifyOnChange(SETTING_LAST_CONNECTION_ERROR, this);
		Settings().NotifyOnChange(SETTING_LAST_CONNECTION_REQUEST, this);
		Settings().NotifyOnChange(SETTING_LATEST_CONNECTION_REQUEST, this);
		Settings().NotifyOnChange(SETTING_PRESENCE_ENABLE, this);
		
		iTimer=CTimeOut::NewL(*this, CActive::EPriorityIdle);
		iTimer->Wait(1);
	}
	virtual void GetCurrentStateL(TBool& aIsError,
			MNetworkErrorObserver::TErrorDisplay& aMode, TDes& aMessageInto) {
		TInt ignored;
		GetCurrentStateAndTimeL(aIsError, aMode, aMessageInto, ignored);
		iMessage=aMessageInto;
	}
	void SettingChanged(TInt aSetting) {
		if ( aSetting==SETTING_LAST_CONNECTION_REQUEST) {
			iState=EReset;
		}
		if ( aSetting==SETTING_LAST_CONNECTION_ERROR) {
			if (iState==EError) {
				Settings().GetSettingL(SETTING_LAST_CONNECTION_ERROR, iMessage);
				iObserver.NetworkError(MNetworkErrorObserver::EAmbient, iMessage);
				return;
			}
		}
		iTimer->Wait(1);
	}
	void CheckState() {
		MNetworkErrorObserver::TErrorDisplay mode;
		TInt wait=0;
		TBool error;
		GetCurrentStateAndTimeL(error, mode, iMessage, wait);
		if (!error && wait) {
			iTimer->Wait(wait);
		} else {
			iTimer->Reset();
		}
		if (error) {
			if (iState!=EError) {
				iState=EError;
				iObserver.NetworkError(mode, iMessage);
			} 
		} else {
			if (iState!=ESuccess) {
				iState=ESuccess;
				iObserver.NetworkSuccess();
			}
		}
	}
	void expired(CBase*) {
		CheckState();
	}
	~CNetworkErrorImpl() {
		delete iTimer;
		Settings().CancelNotifyOnChange(SETTING_LAST_CONNECTION_SUCCESS, this);
		Settings().CancelNotifyOnChange(SETTING_LAST_CONNECTION_ATTEMPT, this);
		Settings().CancelNotifyOnChange(SETTING_LAST_CONNECTION_ERROR, this);
		Settings().CancelNotifyOnChange(SETTING_LAST_CONNECTION_REQUEST, this);
		Settings().CancelNotifyOnChange(SETTING_LATEST_CONNECTION_REQUEST, this);
		Settings().CancelNotifyOnChange(SETTING_PRESENCE_ENABLE, this);
	}
	MNetworkErrorObserver& iObserver;
	CTimeOut*	iTimer;
	TBuf<100>	iMessage;
};

#include "symbian_auto_ptr.h"
EXPORT_C CNetworkError* CNetworkError::NewL(MNetworkErrorObserver& aObserver)
{
	auto_ptr<CNetworkErrorImpl> ret(new (ELeave) CNetworkErrorImpl(aObserver));
	ret->ConstructL();
	return ret.release();
}
