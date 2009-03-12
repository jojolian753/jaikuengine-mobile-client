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
#include "app_local_settings.h"
#include "contextvariant.hrh"

const TInt TLocalAppSettings::KSettingItemsEnabled[] = {
	SETTING_IP_AP,	 
	SETTING_JABBER_NICK,	 
	SETTING_JABBER_PASS,
	SETTING_PRESENCE_ENABLE,
	//10
	SETTING_MEDIA_UPLOAD_ENABLE,
#ifndef __JAIKU__
	SETTING_UPLOAD_URLBASE,
	SETTING_OPTIONS_ENABLE,
	SETTING_DELETE_UPLOADED,
	SETTING_LOGGING_ENABLE,
#endif
	SETTING_LOCATIONSERVICE_ENABLE,
	//20
	SETTING_BT_SCAN_INTERVAL,
#ifndef __JAIKU__
	SETTING_RECORD_ALL,
	SETTING_LOG_UPLOAD_ENABLE,
	SETTING_TAG_ONLY,
#endif
	SETTING_PROMPT_FOR_NAMES,
#ifndef __JAIKU__
	SETTING_UPLOAD_PROMPT_TYPE,
#endif
	SETTING_PUBLISH_URLBASE,
	SETTING_VIBRATE_ONLY,
	SETTING_GPS_LOG_TIME,
	//30
	SETTING_GPS_BT_PORT,
	SETTING_PUBLISH_AUTHOR,
#ifndef __JAIKU__
	SETTING_SNAPSHOT_ON_SMS,
#endif
	SETTING_PUBLISH_SCRIPT,
#ifndef __JAIKU__
	SETTING_UPLOAD_SCRIPT,
	SETTING_IGNORE_NOTIFICATIONS,

	//40
	SETTING_USE_MMC,
	SETTING_RECORD_TIME,
#endif
	SETTING_PROXY,
	SETTING_PROXY_PORT,
#ifndef __JAIKU__
	SETTING_CONTEXTNW_HOST,
	SETTING_CONTEXTNW_PORT,
	SETTING_CONTEXTNW_ENABLED,
	SETTING_ALLOW_ROAMING,
#endif
	// 50
	SETTING_RIGHT_SOFTKEY_CONTEXT,
#ifndef __JAIKU__
	SETTING_ENABLE_LOCA_BLUEJACK,
	SETTING_LOCA_BLUEJACK_MAX_MESSAGES,
	SETTING_LOCA_BLUEJACK_MAX_RETRIES,
	SETTING_LOCA_BLUEJACK_CONNECT_COUNT,
	SETTING_PROJECT_NAME,
	SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT,
#endif

	SETTING_FREEBUSY_ONLY,
	SETTING_SHOW_AUTOTAGS,
#ifndef __JAIKU__
	SETTING_SNAPSHOT_INTERVAL,
#endif

	-1
};
