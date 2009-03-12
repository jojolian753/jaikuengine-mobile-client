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

const TSettingItemEnabled TLocalAppSettings::KSettingItemsEnabled[] = {
	{ 0, EFalse},	// no setting 0
	{ SETTING_WAP_AP, EFalse},  
	{ SETTING_PHONENO, EFalse},
	{ SETTING_IP_AP, ETrue },	 
	{ SETTING_LAST_COMMLOG_UPLOAD, EFalse },	 
	{ SETTING_CELLMAP_CLOSED, EFalse },	 
	{ SETTING_BASEDB_VERSION, EFalse },	 
	{ SETTING_JABBER_NICK, ETrue },	 
	{ SETTING_JABBER_PASS, ETrue},
	{ SETTING_PRESENCE_ENABLE, ETrue },
	//10
	{ SETTING_LAST_BASE_STAMP, EFalse },
	{ SETTING_MEDIA_UPLOAD_ENABLE, ETrue },
	{ SETTING_UPLOAD_URLBASE, ETrue },
	{ SETTING_PREV_BASE, EFalse },
	{ SETTING_PREV_BASE_STAMP,  EFalse },
	{ SETTING_UPLOAD_TAG,  EFalse },
	{ SETTING_OPTIONS_ENABLE, ETrue },
	{ SETTING_DELETE_UPLOADED, ETrue },
	{ SETTING_LOGGING_ENABLE, ETrue },
	{ SETTING_LOCATIONSERVICE_ENABLE, ETrue },
	//20
	{ SETTING_BT_SCAN_INTERVAL, ETrue},
	{ SETTING_RECORD_ALL, ETrue },
	{ SETTING_LOG_UPLOAD_ENABLE, ETrue },
	{ SETTING_TAG_ONLY, ETrue },
	{ SETTING_PROMPT_FOR_NAMES, ETrue },
	{ SETTING_UPLOAD_PROMPT_TYPE, ETrue },
	{ SETTING_PUBLISH_URLBASE,  ETrue },
	{ SETTING_VIBRATE_ONLY, ETrue },
	{ SETTING_GPS_BT_ADDR, ETrue },
	{ SETTING_GPS_LOG_TIME, ETrue },
	//30
	{ SETTING_GPS_BT_PORT, ETrue },
	{ SETTING_PUBLISH_AUTHOR, ETrue },
	{ SETTING_SNAPSHOT_ON_SMS, ETrue },
	{ SETTING_PUBLISH_SCRIPT,  ETrue },
	{ SETTING_UPLOAD_SCRIPT, ETrue },
	{ SETTING_IGNORE_NOTIFICATIONS, ETrue},
	{ SETTING_BT_AP, EFalse},
	{ SETTING_BT_DEV_ADDR, EFalse},
	{ SETTING_BT_DEV_NAME, EFalse},
	{ SETTING_CURRENT_AP, EFalse},
	//40
	{ SETTING_USE_MMC, ETrue },
	{ SETTING_RECORD_TIME, ETrue },
	{ SETTING_PROXY, ETrue },
	{ SETTING_PROXY_PORT, ETrue },
	{ SETTING_CONTEXTNW_HOST, ETrue },
	{ SETTING_CONTEXTNW_PORT, ETrue },
	{ SETTING_CONTEXTNW_ENABLED, ETrue },
	{ SETTING_OWN_DESCRIPTION, EFalse },
	{ SETTING_OWN_DESCRIPTION_TIME, EFalse },
	{ SETTING_ALLOW_ROAMING, ETrue },
	// 50
	{ SETTING_RIGHT_SOFTKEY_CONTEXT, ETrue },
	{ SETTING_PUBLISH_TYPE, EFalse },
	{ SETTING_PUBLISH_USERNAME, EFalse },
	{ SETTING_PUBLISH_PASSWORD, EFalse },
	{ SETTING_ENABLE_LOCA_BLUEJACK, ETrue },
	{ SETTING_LOCA_BLUEJACK_MAX_MESSAGES, ETrue },
	{ SETTING_LOCA_BLUEJACK_MAX_RETRIES, ETrue },
	{ SETTING_LOCA_BLUEJACK_CONNECT_COUNT, ETrue },
	{ SETTING_PROJECT_NAME, ETrue },
	{ SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT, ETrue },
	{ SETTING_VISUALCODES_URLBASE, EFalse },
	{ SETTING_VISUALCODES_SCRIPT, EFalse },
 	{ SETTING_FLICKR_AUTH, ETrue },
        { SETTING_PUBLISH_SHARING, EFalse },
        { SETTING_PUBLISH_AUTOTAGS, EFalse },
};
