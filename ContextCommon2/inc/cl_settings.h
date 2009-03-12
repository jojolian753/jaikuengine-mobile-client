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

#ifndef CL_SETTINGS_H_INCLUDED
#define CL_SETTINGS_H_INCLUDED 1

// don't use 'IAP', since there is
// a macro with that name in cdbcols.h

#define SETTING_WAP_AP	1
#define SETTING_PHONENO 2
#define SETTING_IP_AP   3
#define SETTING_LAST_COMMLOG_UPLOAD 4
#define SETTING_CELLMAP_CLOSED 5
#define SETTING_BASEDB_VERSION 6
#define SETTING_JABBER_NICK 7
#define SETTING_JABBER_PASS 8
#define SETTING_PRESENCE_ENABLE 9
#define SETTING_LAST_BASE_STAMP 10
#define SETTING_MEDIA_UPLOAD_ENABLE 11
#define SETTING_UPLOAD_URLBASE 12
#define SETTING_PREV_BASE 13
#define SETTING_PREV_BASE_STAMP 14
#define SETTING_UPLOAD_TAG 15
#define SETTING_OPTIONS_ENABLE 16
#define SETTING_DELETE_UPLOADED	17
#define SETTING_LOGGING_ENABLE	18
#define SETTING_LOCATIONSERVICE_ENABLE 19
#define SETTING_BT_SCAN_INTERVAL 20
#define SETTING_RECORD_ALL 21
#define SETTING_LOG_UPLOAD_ENABLE 22
#define SETTING_TAG_ONLY 23
#define SETTING_PROMPT_FOR_NAMES 24
#define SETTING_UPLOAD_PROMPT_TYPE 25
#define SETTING_PUBLISH_URLBASE 26
#define SETTING_VIBRATE_ONLY 27
#define SETTING_GPS_BT_ADDR 28
#define SETTING_GPS_LOG_TIME 29
#define SETTING_GPS_BT_PORT 30
#define SETTING_PUBLISH_AUTHOR 31
#define SETTING_SNAPSHOT_ON_SMS 32
#define SETTING_PUBLISH_SCRIPT 33
#define SETTING_UPLOAD_SCRIPT 34
#define SETTING_IGNORE_NOTIFICATIONS 35
#define SETTING_BT_AP 36
#define SETTING_BT_DEV_ADDR 37
#define SETTING_BT_DEV_NAME 38
#define SETTING_CURRENT_AP 39
#define SETTING_USE_MMC 40
#define SETTING_RECORD_TIME 41
#define SETTING_PROXY 42
#define SETTING_PROXY_PORT 43
#define SETTING_CONTEXTNW_HOST 44
#define SETTING_CONTEXTNW_PORT 45
#define SETTING_CONTEXTNW_ENABLED 46
#define SETTING_OWN_DESCRIPTION 47
#define SETTING_OWN_DESCRIPTION_TIME 48
#define SETTING_ALLOW_ROAMING 49
#define SETTING_RIGHT_SOFTKEY_CONTEXT 50
#define SETTING_PUBLISH_TYPE 51
#define SETTING_PUBLISH_USERNAME 52
#define SETTING_PUBLISH_PASSWORD 53
#define SETTING_ENABLE_LOCA_BLUEJACK 54
#define SETTING_LOCA_BLUEJACK_MAX_MESSAGES 55
#define SETTING_LOCA_BLUEJACK_MAX_RETRIES 56
#define SETTING_LOCA_BLUEJACK_CONNECT_COUNT 57
#define SETTING_PROJECT_NAME 58
#define SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT 59
#define SETTING_VISUALCODES_URLBASE 60
#define SETTING_VISUALCODES_SCRIPT 61
#define SETTING_FLICKR_AUTH 62
#define SETTING_PLAYBACK_VOLUME 63
#define SETTING_PUBLISH_SHARING 64
#define SETTING_PUBLISH_AUTOTAGS 65
#define SETTING_CALENDAR_SHARING 66
#define SETTING_SHOW_AUTOTAGS 67
#define SETTING_SNAPSHOT_INTERVAL 68
#define SETTING_SHOW_TAGS 69
#define SETTING_FIXED_PRIVACY 70
#define SETTING_LAST_CONNECTION_TIME 71
#define SETTING_LAST_CONNECTION_BYTECOUNT 72
#define SETTING_LAST_TIMESYNC 73
#define SETTING_AUTH_SEQ 74
#define SETTING_WINS_IMEI 75
#define SETTING_PRESENCE_EXPIRATION_TIME 76 // specified in days 
#define SETTING_JABBER_ADDR 77 
#define SETTING_JABBER_PORT 78 
#define SETTING_PROFILE_BEFORE_SLEEP 79
#define SETTING_WENT_TO_SLEEP 80
#define SETTING_JABBER_PASS_SHA1 81
#define SETTING_DEVICE_ID 82
#define SETTING_USER_EMAIL 83
#define SETTING_USER_FIRSTNAME 84
#define SETTING_USER_LASTNAME 85
#define SETTING_ALLOW_NETWORK_ACCESS 86
#define SETTING_ALLOW_NETWORK_ONCE 87
#define SETTING_ACCEPTED_PRIVACY_STMT_VERSION 88
#define SETTING_ACCEPTED_NETWORK_ACCESS 89
#define SETTING_IDENTIFICATION_ERROR 90
#define SETTING_LAST_CONNECTION_SUCCESS 91
#define SETTING_LAST_CONNECTION_ATTEMPT 92
#define SETTING_LAST_CONNECTION_ERROR 93
#define SETTING_LAST_CONNECTION_REQUEST 94
#define SETTING_LATEST_CONNECTION_REQUEST 95
#define SETTING_PROGRESS_INCOMING 96
#define SETTING_PROGRESS_OUTGOING 97
#define SETTING_FLICKR_NSID 98
#define SETTING_DONT_DOUBLE_CONFIRM_DELETES 99
#define SETTING_SHOW_WARNING_ON_BACK 100
#define SETTING_CONNECTIVITY_MODEL 101
#define SETTING_DOWNLOAD_IMAGES_MODE 102
#define SETTING_DOWNLOAD_IMAGES_YESNO 103

#define SETTINGS_COUNT 104 // real count +1 for array bounds

#define MAX_ADDRESS_LEN 30
#define MAX_URL_LEN	100
#define MEDIA_PUBLISHER_DISABLED 0
#define MEDIA_PUBLISHER_CL 1
#define MEDIA_PUBLISHER_CM 2

#define MEDIA_PUBLISH_CONTEXT 0
#define MEDIA_PUBLISH_FLICKR 1

#define SHARE_CALENDAR_FULL 0
#define SHARE_CALENDAR_FREEBUSY 1
#define SHARE_CALENDAR_NONE 2

#define CONNECTIVITY_WHEN_ACTIVITY_ONLY 0
#define CONNECTIVITY_ALWAYS_ON 1

#ifndef SETTING_DEFINES_ONLY
#include "settings.h"
#endif

#endif
