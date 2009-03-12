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

#include "cl_settings.h"
#include "cl_settings_impl.h"
#include "local_defaults.h"

const TSettingItem TClSettings::KClSettings[] = {
	{ 0, EEmpty, 0, -1, EFalse },	// no setting 0
	{ SETTING_WAP_AP, EAP, 0, -1, EFalse }, // not in use
	{ SETTING_PHONENO, EString, 0, -1, EFalse },	// not in use
	{ SETTING_IP_AP, EAP, 0, -1, EFalse },	 
	{ SETTING_LAST_COMMLOG_UPLOAD, ETime, 0, 0, ETrue },	 
	{ SETTING_CELLMAP_CLOSED, EBool, 0, 1, ETrue },	 
	{ SETTING_BASEDB_VERSION, EInt, 0, 4, ETrue },	 
	{ SETTING_JABBER_NICK, EString, 0, -1, EFalse },	 
	{ SETTING_JABBER_PASS, EString, 0, -1, EFalse },	 
	{ SETTING_PRESENCE_ENABLE, EBool, 0, DEFAULT_PRESENCE_ENABLE, ETrue },
	//10
	{ SETTING_LAST_BASE_STAMP, ETime, 0, -1, EFalse },
#ifdef DEFAULT_MEDIA_UPLOADER
	{ SETTING_MEDIA_UPLOAD_ENABLE, EEnum, 0, DEFAULT_MEDIA_UPLOADER, ETrue },
#else
	{ SETTING_MEDIA_UPLOAD_ENABLE, EEnum, 0, 0, ETrue },
#endif
	{ SETTING_UPLOAD_URLBASE, EString, DEFAULT_UPLOAD_URLBASE, -1, ETrue },
	{ SETTING_PREV_BASE, EString, 0, -1, EFalse },
	{ SETTING_PREV_BASE_STAMP, ETime, 0, -1, EFalse },
	{ SETTING_UPLOAD_TAG, EString, 0, -1, EFalse },
	{ SETTING_OPTIONS_ENABLE, EBool, 0, DEFAULT_OPTIONS_ENABLE, ETrue },
	{ SETTING_DELETE_UPLOADED, EBool, 0, DEFAULT_DELETE_UPLOADED, ETrue },
	{ SETTING_LOGGING_ENABLE, EBool, 0, DEFAULT_LOGGING_ENABLE, ETrue },
	{ SETTING_LOCATIONSERVICE_ENABLE, EBool, 0, DEFAULT_LOCATIONSERVICE_ENABLE, ETrue },
	//20
	{ SETTING_BT_SCAN_INTERVAL, EInt, 0, DEFAULT_BT_SCAN_INTERVAL, ETrue },
	{ SETTING_RECORD_ALL, EBool, 0, DEFAULT_RECORD_ALL, ETrue },
	{ SETTING_LOG_UPLOAD_ENABLE, EBool, 0, DEFAULT_LOG_UPLOAD_ENABLE, ETrue },
	{ SETTING_TAG_ONLY, EBool, 0, DEFAULT_TAG_ONLY, ETrue },
	{ SETTING_PROMPT_FOR_NAMES, EBool, 0, DEFAULT_PROMPT_FOR_NAMES, ETrue },
	{ SETTING_UPLOAD_PROMPT_TYPE, EEnum, 0, DEFAULT_UPLOAD_PROMPT_TYPE, ETrue },
#ifdef DEFAULT_PUBLISH_URLBASE
	{ SETTING_PUBLISH_URLBASE, EString, DEFAULT_PUBLISH_URLBASE, -1, ETrue },
#else
	{ SETTING_PUBLISH_URLBASE, EString, DEFAULT_UPLOAD_URLBASE, -1, ETrue },
#endif
	{ SETTING_VIBRATE_ONLY, EBool, 0, ETrue, ETrue },
	{ SETTING_GPS_BT_ADDR, EString8, 0, -1, EFalse },
	{ SETTING_GPS_LOG_TIME, EInt, 0, 15, ETrue },
	//30
	{ SETTING_GPS_BT_PORT, EInt, 0, -1, EFalse },
	{ SETTING_PUBLISH_AUTHOR, EString, 0, -1, EFalse },
	{ SETTING_SNAPSHOT_ON_SMS, EBool, 0, DEFAULT_SNAPSHOT_ON_SMS, ETrue },
#ifdef DEFAULT_PUBLISH_SCRIPT
	{ SETTING_PUBLISH_SCRIPT, EString, DEFAULT_PUBLISH_SCRIPT, -1, ETrue },
#else
	{ SETTING_PUBLISH_SCRIPT, EString, DEFAULT_UPLOAD_SCRIPT, -1, ETrue },
#endif
	{ SETTING_UPLOAD_SCRIPT, EString, DEFAULT_UPLOAD_SCRIPT, -1, ETrue },
	{ SETTING_IGNORE_NOTIFICATIONS, EBool, 0, EFalse, ETrue},
	{ SETTING_BT_AP, EAP, 0, -1, EFalse},
	{ SETTING_BT_DEV_ADDR, EString8, 0, -1, EFalse},
	{ SETTING_BT_DEV_NAME, EString, 0, -1, EFalse},
	{ SETTING_CURRENT_AP, EAP, 0, -1, EFalse},
	//40
	{ SETTING_USE_MMC, EBool, 0, DEFAULT_USE_MMC, ETrue },
	{ SETTING_RECORD_TIME, EInt, 0, 30, ETrue },
#ifdef DEFAULT_PROXY
	{ SETTING_PROXY, EString, DEFAULT_PROXY, -1, ETrue },
#else
	{ SETTING_PROXY, EString, 0, -1, EFalse },
#endif
#ifdef DEFAULT_PROXY_PORT
	{ SETTING_PROXY_PORT, EInt, 0, DEFAULT_PROXY_PORT, ETrue },
#else
	{ SETTING_PROXY_PORT, EInt, 0, -1, EFalse },
#endif
#ifdef DEFAULT_CONTEXTNW_HOST
	{ SETTING_CONTEXTNW_HOST, EString, DEFAULT_CONTEXTNW_HOST, 0, ETrue },
#else
	{ SETTING_CONTEXTNW_HOST, EString, 0, 0, EFalse },
#endif
#ifdef DEFAULT_CONTEXTNW_PORT
	{ SETTING_CONTEXTNW_PORT, EInt, 0, DEFAULT_CONTEXTNW_PORT, ETrue },
#else
	{ SETTING_CONTEXTNW_PORT, EInt, 0, -1, EFalse },
#endif
#ifdef DEFAULT_CONTEXTNW_ENABLED
	{ SETTING_CONTEXTNW_ENABLED, EBool, 0, DEFAULT_CONTEXTNW_ENABLED, ETrue },
#else
	{ SETTING_CONTEXTNW_ENABLED, EBool, 0, -1, EFalse },
#endif
	{ SETTING_OWN_DESCRIPTION, EString, 0, -1, EFalse },
	{ SETTING_OWN_DESCRIPTION_TIME, ETime, 0, 0, EFalse },
	{ SETTING_ALLOW_ROAMING, EBool, 0, EFalse, ETrue },
#ifdef DEFAULT_RIGHT_SOFTKEY_CONTEXT
	{ SETTING_RIGHT_SOFTKEY_CONTEXT, EBool, 0, DEFAULT_RIGHT_SOFTKEY_CONTEXT, ETrue },
#else
	{ SETTING_RIGHT_SOFTKEY_CONTEXT, EBool, 0, EFalse, ETrue },
#endif
#ifdef DEFAULT_PUBLISH_TYPE
	{ SETTING_PUBLISH_TYPE, EEnum, 0, DEFAULT_PUBLISH_TYPE, ETrue },
#else
	{ SETTING_PUBLISH_TYPE, EEnum, 0, MEDIA_PUBLISH_CONTEXT, ETrue },
#endif
	{ SETTING_PUBLISH_USERNAME, EString, 0, -1, EFalse },
	{ SETTING_PUBLISH_PASSWORD, EString, 0, -1, EFalse },
#ifdef DEFAULT_ENABLE_LOCA_BLUEJACK
	{ SETTING_ENABLE_LOCA_BLUEJACK, EBool, 0, DEFAULT_ENABLE_LOCA_BLUEJACK, ETrue },
#else
	{ SETTING_ENABLE_LOCA_BLUEJACK, EBool, 0, EFalse, ETrue },
#endif
#ifdef DEFAULT_LOCA_BLUEJACK_MAX_MESSAGES
	{ SETTING_LOCA_BLUEJACK_MAX_MESSAGES, EInt, 0, DEFAULT_LOCA_BLUEJACK_MAX_MESSAGES, ETrue },
#else
	{ SETTING_LOCA_BLUEJACK_MAX_MESSAGES, EInt, 0, 3, ETrue },
#endif
#ifdef DEFAULT_LOCA_BLUEJACK_MAX_RETRIES
	{ SETTING_LOCA_BLUEJACK_MAX_RETRIES, EInt, 0, DEFAULT_LOCA_BLUEJACK_MAX_RETRIES, ETrue },
#else
	{ SETTING_LOCA_BLUEJACK_MAX_RETRIES, EInt, 0, 10, ETrue },
#endif
#ifdef DEFAULT_LOCA_BLUEJACK_CONNECT_COUNT
	{ SETTING_LOCA_BLUEJACK_CONNECT_COUNT, EInt, 0, DEFAULT_LOCA_BLUEJACK_CONNECT_COUNT, ETrue },
#else
	{ SETTING_LOCA_BLUEJACK_CONNECT_COUNT, EInt, 0, 5, ETrue },
#endif
#ifdef DEFAULT_PROJECT_NAME
	{ SETTING_PROJECT_NAME, EString, DEFAULT_PROJECT_NAME, -1, ETrue },
#else
	{ SETTING_PROJECT_NAME, EString, 0, -1, EFalse },
#endif
#ifdef DEFAULT_LOCA_BLUEJACK_MESSAGE_TIMEOUT
	{ SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT, EInt, 0, DEFAULT_LOCA_BLUEJACK_MESSAGE_TIMEOUT, ETrue },
#else
	{ SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT, EInt, 0, 45, ETrue },
#endif
#ifdef DEFAULT_VISUALCODES_URLBASE
	{ SETTING_VISUALCODES_URLBASE, EString, DEFAULT_VISUALCODES_URLBASE, -1, ETrue },
#else
	{ SETTING_VISUALCODES_URLBASE, EString, DEFAULT_UPLOAD_URLBASE, -1, ETrue },
#endif
#ifdef DEFAULT_VISUALCODES_SCRIPT
	{ SETTING_VISUALCODES_SCRIPT, EString, DEFAULT_VISUALCODES_SCRIPT, -1, ETrue },
#else
	{ SETTING_VISUALCODES_SCRIPT, EString, DEFAULT_UPLOAD_SCRIPT, -1, ETrue },
#endif
	{ SETTING_FLICKR_AUTH, EString8, 0, -1, EFalse },
	{ SETTING_PLAYBACK_VOLUME, EInt, 0, DEFAULT_PLAYBACK_VOLUME, ETrue },
	{ SETTING_PUBLISH_SHARING, EInt, 0, 0 /* CCMPost::EPublic */, ETrue},
	{ SETTING_PUBLISH_AUTOTAGS, EInt, 0, 0xffffff, ETrue},
#ifndef DEFAULT_CALENDAR_SHARING
#define DEFAULT_CALENDAR_SHARING SHARE_CALENDAR_NONE
#endif
	{ SETTING_CALENDAR_SHARING, EEnum, 0, DEFAULT_CALENDAR_SHARING, ETrue },
	{ SETTING_SHOW_AUTOTAGS, EBool, 0, ETrue, ETrue },
	{ SETTING_SNAPSHOT_INTERVAL, EInt, 0, 0, ETrue },
	{ SETTING_SHOW_TAGS, EBool, 0, ETrue, ETrue },
	{ SETTING_FIXED_PRIVACY, EEnum, 0, -1, ETrue },
	{ SETTING_LAST_CONNECTION_TIME, ETime, 0, 0, EFalse },
	{ SETTING_LAST_CONNECTION_BYTECOUNT, EInt, 0, 0, EFalse },
	{ SETTING_LAST_TIMESYNC, ETime, 0, 0, EFalse },
	{ SETTING_AUTH_SEQ, EInt, 0, 1, ETrue },
	{ SETTING_WINS_IMEI, EString, 0, 0, EFalse },
	{ SETTING_PRESENCE_EXPIRATION_TIME, EInt, 0, 10950, ETrue },
	{ SETTING_JABBER_ADDR, EString, DEFAULT_JABBER_ADDR, -1, ETrue },
	{ SETTING_JABBER_PORT, EInt, 0, DEFAULT_JABBER_PORT, ETrue },
	{ SETTING_PROFILE_BEFORE_SLEEP, EInt, 0, 0, EFalse },
	{ SETTING_WENT_TO_SLEEP, ETime, 0, 0, EFalse },
	{ SETTING_JABBER_PASS_SHA1, EString, 0, 0, EFalse },
	{ SETTING_DEVICE_ID, EString, 0, 0, EFalse },
	{ SETTING_USER_EMAIL, EString, 0, 0, EFalse },
	{ SETTING_USER_FIRSTNAME, EString, 0, 0, EFalse },
	{ SETTING_USER_LASTNAME, EString, 0, 0, EFalse },
	{ SETTING_ALLOW_NETWORK_ACCESS, EBool, 0, 0, ETrue },
	{ SETTING_ALLOW_NETWORK_ONCE, EBool, 0, 0, ETrue },
	{ SETTING_ACCEPTED_PRIVACY_STMT_VERSION, EInt, 0, 0, EFalse },
	{ SETTING_ACCEPTED_NETWORK_ACCESS, EBool, 0, 0, EFalse },
	{ SETTING_IDENTIFICATION_ERROR, EString, 0, 0, EFalse },
	{ SETTING_LAST_CONNECTION_SUCCESS, ETime, 0, 0, EFalse },
	{ SETTING_LAST_CONNECTION_ATTEMPT, ETime, 0, 0, EFalse },
	{ SETTING_LAST_CONNECTION_ERROR, EString, 0, 0, EFalse },
	{ SETTING_LAST_CONNECTION_REQUEST, ETime, 0, 0, EFalse },
	{ SETTING_LATEST_CONNECTION_REQUEST, ETime, 0, 0, EFalse },
	{ SETTING_PROGRESS_INCOMING, EInt, 0, 0, EFalse },
	{ SETTING_PROGRESS_OUTGOING, EInt, 0, 0, EFalse },
	{ SETTING_FLICKR_NSID, EString, 0, 0, EFalse },
	{ SETTING_DONT_DOUBLE_CONFIRM_DELETES, EBool, 0, 0, ETrue },
	{ SETTING_SHOW_WARNING_ON_BACK, EBool, 0, 1, ETrue },
    { SETTING_CONNECTIVITY_MODEL, EEnum, 0, 0, ETrue },
	{ SETTING_DOWNLOAD_IMAGES_MODE, EEnum, 0, 0, ETrue },
	{ SETTING_DOWNLOAD_IMAGES_YESNO, EBool, 0, 1, ETrue },
};

EXPORT_C const TSettingItem TClSettings::GetKClSettings(TInt Setting)
{
	if (Setting>SETTINGS_COUNT) Setting=0;
	return KClSettings[Setting];
}

EXPORT_C bool TClSettings::GetDefaultL(TInt Setting, TDes& Value) const
{
	const TSettingItem i=KClSettings[Setting];
	if (i.iSettingNo!=Setting) User::Leave(-1025);

	if (i.iStringDefault) {
		Value=TPtrC((TText*)i.iStringDefault);
	} else {
		Value.Zero();
	}
	return i.iDefaultExists;
}

EXPORT_C bool TClSettings::GetDefaultL(TInt Setting, TDes8& Value) const
{
	const TSettingItem i=KClSettings[Setting];
	if (i.iSettingNo!=Setting) User::Leave(-1025);

	if (i.iStringDefault) {
		Value=TPtrC8((TText8*)i.iStringDefault);
	} else {
		Value.Zero();
	}
	return i.iDefaultExists;
}

EXPORT_C bool TClSettings::GetDefaultL(TInt Setting, TInt& Value) const
{
	const TSettingItem i=KClSettings[Setting];
	if (i.iSettingNo!=Setting) User::Leave(-1025);

	Value=i.iIntDefault;
	return i.iDefaultExists;
}

EXPORT_C bool TClSettings::GetDefaultL(TInt Setting, TTime& Value) const
{
	const TSettingItem i=KClSettings[Setting];
	if (i.iSettingNo!=Setting) User::Leave(-1025);

	Value=i.iIntDefault;
	return i.iDefaultExists;
}
