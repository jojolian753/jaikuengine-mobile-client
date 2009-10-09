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

#ifndef CONTEXT_UIDS_H_INCLUDED
#define CONTEXT_UIDS_H_INCLUDED 1
/*
 * UIDs for Context components
 *
 * *app.h files should include this, instead of
 * defining the value
 * these values are used in MMP files via cpp and
 * in pkg files via update_sis_versions.pl
 *
 * range 1: 0x101FBAD0 to 0x101FBAD3 - USED UP
 */
#include "context_uids.rh"

//const TUid KUidcontextbook = { CONTEXT_UID_CONTEXTBOOK };
const TUid KUidcontext_log = { CONTEXT_UID_CONTEXT_LOG };
const TUid KUidcl_autostart = { CONTEXT_UID_CL_AUTOSTART };
const TUid KUidautostart = { CONTEXT_UID_CL_AUTOSTART };
const TUid KUidstarter = { CONTEXT_UID_STARTER };

/*
 * range 2: 0x10200C6B to 0x10200C6E - USED UP
 */
const TUid KUidcontextserver = { CONTEXT_UID_CONTEXTSERVER };
const TUid KUidcontextclient = { CONTEXT_UID_CONTEXTCLIENT };
const TUid KUidcontextcommon = { CONTEXT_UID_CONTEXTCOMMON };
//const TUid KUidrecognizer = { CONTEXT_UID_RECOGNIZER };

/*
 * range 3: 0x10204B9C to 0x10204BAF
 */

//const TUid KUiddbtest = { CONTEXT_UID_DBTEST };
//const TUid KUidGnubox = { CONTEXT_UID_GNUBOX };
//const TUid KUidContextNote = { CONTEXT_UID_CONTEXTNOTE };
//const TUid KUidContextFetch = { CONTEXT_UID_CONTEXTFETCH };
//const TUid KUidContext = { CONTEXT_UID_CONTEXT };
const TUid KUidContextNotify = { CONTEXT_UID_CONTEXTNOTIFY };
const TUid KUidContextNotifyClient = { CONTEXT_UID_CONTEXTNOTIFYCLIENT };
const TUid KUidContextContacts = { CONTEXT_UID_CONTEXTCONTACTS };
//const TUid KUidContextCallLog = { CONTEXT_UID_CONTEXTCALLLOG };
const TUid KUidBlackBoardData = { CONTEXT_UID_BLACKBOARDDATA };
const TUid KUidBlackBoardServer = { CONTEXT_UID_BLACKBOARDSERVER };

/*
 * range 4
 */

const TUid KUidBlackBoardClient = { CONTEXT_UID_BLACKBOARDCLIENT };
const TUid KUidBlackBoardFactory = { CONTEXT_UID_BLACKBOARDFACTORY };
//const TUid KUidBlackBoardClientTest = { CONTEXT_UID_BLACKBOARDCLIENTTEST };
//const TUid KUidBlackBoard = { CONTEXT_UID_BLACKBOARD };
const TUid KUidContextSensors = { CONTEXT_UID_CONTEXTSENSORS };
const TUid KUidContextSensorFactory = { CONTEXT_UID_SENSORDATAFACTORY };
const TUid KUidContextNetwork = { CONTEXT_UID_CONTEXTNETWORK };
const TUid KUidContextCommon2 = { CONTEXT_UID_CONTEXTCOMMON2 };
const TUid KUidContextMedia = { CONTEXT_UID_CONTEXTMEDIA };
const TUid KUidContextMediaData = { CONTEXT_UID_CONTEXTMEDIADATA };
const TUid KUidContextMediaFactory = { CONTEXT_UID_CONTEXTMEDIAFACTORY };
//const TUid KUidContextMediaApp = { CONTEXT_UID_CONTEXTMEDIAAPP };
const TUid KUidContextSensorData = { CONTEXT_UID_CONTEXTSENSORDATA };
const TUid KUidContextUI = { CONTEXT_UID_CONTEXTUI };
const TUid KUidContextMediaUi = { CONTEXT_UID_CONTEXTMEDIAUI };
const TUid KUidContextFlickr = { CONTEXT_UID_CONTEXT_LOG };
const TUid KUidContextLoca = { CONTEXT_UID_CONTEXT_LOG };
const TUid KUidJaikuSettings = { CONTEXT_UID_CONTEXT_LOG };
const TUid KUidMeaning = { CONTEXT_UID_CONTEXT_LOG };


//const TUid KUidContextMenu = {CONTEXT_UID_CONTEXTMENU };
const TUid KUidContextShutter = { CONTEXT_UID_SHUTTER };
const TUid KUidShutter = { CONTEXT_UID_SHUTTER };

//const TUid KUidGsmAudio = { CONTEXT_UID_GSMAUDIO };
//const TUid KUidGsmAudioTest = { CONTEXT_UID_GSMAUDIO_TEST };
const TUid KUidContextContactsUi = { CONTEXT_UID_CONTEXTCONTACTSUI };
//const TUid KUidrautatieasema_auth_token = { CONTEXT_UID_FLICKR_AUTH };
//const TUid KUidS80Avkon = { CONTEXT_UID_S80AVKON };
//const TUid KUidNetmon = { CONTEXT_UID_NETMON };
//const TUid KUidNetmon2 = { CONTEXT_UID_NETMON };
//const TUid KUidHwtricks = { CONTEXT_UID_HWTRICKS };
//const TUid KUidContextBlue = { CONTEXT_UID_CONTEXTBLUE };
//const TUid KUidBlackBoardRemote = { CONTEXT_UID_BLACKBOARDREMOTE };
const TUid KUidContextCommSensors = { CONTEXT_UID_CONTEXTCOMMSENSORS };
//const TUid KUidContextPython = { CONTEXT_UID_CONTEXTPYTHON };
const TUid KUidContextStarter = { CONTEXT_UID_CONTEXTSTARTER };
const TUid KUidContextWelcome = { CONTEXT_UID_CONTEXTWELCOME };
//const TUid KUidJaikuDevHelper = { CONTEXT_UID_JAIKUDEVHELPER };
const TUid KUidContextTransfer = { CONTEXT_UID_CONTEXTTRANSFER };
const TUid KUidContextMediaPlugin = { CONTEXT_UID_CONTEXTMEDIAPLUGIN };
const TUid KUidJaikuUiKit = { CONTEXT_UID_JAIKUUIKIT };
//const TUid KUidContextCleaner = { CONTEXT_UID_CLEANER };
//const TUid KUidCleaner = { CONTEXT_UID_CLEANER };
const TUid KUidJaikuTool = { CONTEXT_UID_JAIKUTOOL };

const TUid KUidSymbianOsUnit = { CONTEXT_UID_SYMBIANOSUNIT };
const TUid KUidMeaningApp = { CONTEXT_UID_MEANINGAPP };

const TUid KUidJaikuCacher = { CONTEXT_UID_JAIKUCACHER };
const TUid KUidJaikuCacherClient = { CONTEXT_UID_JAIKUCACHERCLIENT };

const TUid KUidContextWelcomeUi = { CONTEXT_UID_CONTEXTWELCOMEUI };

const TUid KUidContextUiLoft = { CONTEXT_UID_UILOFT };

const TUid KUidJaikuStreamUi = { CONTEXT_UID_JAIKUSTREAMUI };

const TUid KUidContextPython = { CONTEXT_UID_CONTEXTPYTHON };

const TUid KUidKeyEventsServer = { CONTEXT_UID_KEYEVENTSERVER };
const TUid KUidKeyEventsClient = { CONTEXT_UID_KEYEVENTCLIENT };
/*
 * external
 */
const TUid KUidexpat = { CONTEXT_UID_EXPAT };
const TUid KUidContextexpat = { CONTEXT_UID_EXPAT };

const TUid KUidTaskList = { CONTEXT_UID_TASKLIST };

#endif
