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

#if !defined(LOG_PROFILE_H_INCLUDED)

#define LOG_PROFILE_H_INCLUDED

#include "i_log_source.h"
#include "log_base_impl.h"

#include "app_context.h"
#include "timeout.h"
#include "csd_profile.h"

#if !defined(__S60V3__) && defined(__S60V2__)
#include <settinginfo.h>
#endif

class Clog_profile : 
#if defined(__S60V3__) || !defined(__S60V2__)
	public CCheckedActive, 
#else
	public CBase, public MSettingInfoObserver,
#endif
	public Mlog_base_impl, public MTimeOut{
public:
	IMPORT_C ~Clog_profile();

	IMPORT_C static Clog_profile* NewL(MApp_context& Context);
	IMPORT_C int get_current_profile();
	IMPORT_C const CBBSensorEvent& get_value();
private:
#if defined(__S60V3__) || !defined(__S60V2__)
	void DoCancel();
	virtual TInt CheckedRunError(TInt aError);
	void CheckedRunL();
#else
	void HandleNotificationL(SettingInfo::TSettingID aID, const TDesC &  aNewValue );
#endif

	void expired(CBase* Source);

	Clog_profile(MApp_context& Context);
	void ConstructL();

	TBBProfile prev_profile;
	TBBProfile iProfile;

	TInt get_profile_via_profilapi();
	void listen_for_notification();
	TInt	iErrorCount;
	CTimeOut *iWait;
#ifndef __S60V3__
	TBuf<10> iVibraDescr;
#  ifndef __S60V2__
	CArrayFixFlat<class TSysAgentCondition>* aConditions;
	class RSystemAgent	*iSysAgent; bool iAgentIsOpen;
#  else
	class CSettingInfo* iSettingInfo;
	CArrayFixFlat<SettingInfo::TSettingID>* iSettings;
	TBool	iUserCreatableProfiles;
#  endif
#else
	class CRepository* iRepository;
#endif
};

#endif
