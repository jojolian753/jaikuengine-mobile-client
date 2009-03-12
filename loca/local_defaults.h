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

#ifndef CL_LOCAL_SETTINGS_H_INCLUDED
#define CL_LOCAL_SETTINGS_H_INCLUDED 1

#define DEFAULT_PRESENCE_ENABLE	0
#define DEFAULT_UPLOAD_URLBASE L"http://db.cs.helsinki.fi/~mraento/cgi-bin/"
#define DEFAULT_OPTIONS_ENABLE 1
#define DEFAULT_DELETE_UPLOADED 1
#define DEFAULT_LOGGING_ENABLE 1
#define DEFAULT_LOCATIONSERVICE_ENABLE 0
#define DEFAULT_BT_SCAN_INTERVAL 5
#define DEFAULT_RECORD_ALL 0
#define DEFAULT_LOG_UPLOAD_ENABLE 1
#define DEFAULT_PICTURE_ENABLE 0
#define DEFAULT_TAG_ONLY 0
#define DEFAULT_PROMPT_FOR_NAMES 0
#define DEFAULT_UPLOAD_PROMPT_TYPE 0
#define DEFAULT_SNAPSHOT_ON_SMS 0
#define DEFAULT_UPLOAD_SCRIPT L"put20.pl"
#define DEFAULT_USE_MMC 1

#undef NO_BT_AUTOPOWER
#define REALLY_ONLY_LOGGING 1
#undef DO_LOCATIONING
#define NO_PRESENCE 1
#define LOCA 1
#define NO_BASES 1
#define DONT_LOG_EVENTS_TO_FILE 1
#define DO_SMS_STATUS 1
#define CONTEXTNW 1

#if defined(WINS) || defined(__WINS__)
#define DEFAULT_CONTEXTNW_HOST L"192.168.1.68"
#define DEFAULT_CONTEXTNW_PORT 2000
#else
#define DEFAULT_CONTEXTNW_HOST L"loca.uiah.fi"
#define DEFAULT_CONTEXTNW_PORT 6200
#endif

#define DEFAULT_CONTEXTNW_ENABLED 1
#define DEFAULT_ENABLE_LOCA_BLUEJACK ETrue
#define DEFAULT_LOCA_BLUEJACK_MAX_MESSAGES 3
#define DEFAULT_LOCA_BLUEJACK_MAX_RETRIES 50
#define DEFAULT_LOCA_BLUEJACK_CONNECT_COUNT 5

#endif
