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

#include "break.h"
#include <aknnotewrappers.h>
#include "keycapture.h"
#include "independent.h"
#include <apgtask.h> // Going background 
#include <bautils.h>
#include <w32std.h>
#include "screen.h"
#include <viewcli.h> //CVwsSessionWrapper
#include <apgwgnam.h>
#include "viewids.h"
#include "cl_settings.h"

#include "symbian_auto_ptr.h"
#include "raii_w32std.h"
#include "raii_e32std.h"
#include "raii_f32file.h"
#include "raii_array.h"
#include "raii_aknkeylock.h"
#include <eikdll.h>
#include <hal.h>

#include <sysutil.h>

enum JOYSTICK_EVENTS {
	JOY_LEFT = 0xF807,
	JOY_RIGHT = 0xF808,
	JOY_UP = 0xF809,
	JOY_DOWN = 0xF80A,
	JOY_CLICK = EKeyDevice3
};

#include "context_uids.h"
const TUid KUidPhone = { 0x100058b3 };
const TUid KUidMenu = {  0x101f4cd2 };

#ifndef __WINS__
_LIT(contextbook_filen, "c:\\system\\apps\\contextbook\\contextbook.app");
_LIT(contextcontacts_filen, "c:\\system\\apps\\contextcontacts\\contextcontacts.app");
_LIT(contextcontacts_filen2, "e:\\system\\apps\\contextcontacts\\contextcontacts.app");
_LIT(contextcalllog_filen, "c:\\system\\apps\\contextcalllog\\contextcalllog.app");
#else
_LIT(contextcontacts_filen, "z:\\system\\apps\\contextcontacts\\contextcontacts.app");
_LIT(contextbook_filen, "z:\\system\\apps\\contextbook\\contextbook.app");
_LIT(contextcalllog_filen, "z:\\system\\apps\\contextcalllog\\contextcalllog.app");
_LIT(contextcontacts_filen2, "e:\\system\\apps\\contextcontacts\\contextcontacts.app");
#endif

#define DO_RIGHTKEY 1
#define DO_TIMER 1

const TUid KCameraUid = { 0x1000593F };
const TUid KCamera2Uid = { 0x101f857a}; //6630
const TUid KVideoUid = { 0x101fa14a }; 
const TUid KVideo2Uid = { 0x101f857A}; //6630

void capturekeyL(RAWindowGroup& wg, RAArray<TInt>& ids, RAArray<TInt>& ids2, TUint keycode, TUint scancode)
{
	TInt id=wg.CaptureKey(keycode, 0, 0);
	User::LeaveIfError(id);
	User::LeaveIfError(ids.Append(id));
	if (keycode!=EKeyNo && keycode!=EKeyYes) {
		id=wg.CaptureKeyUpAndDowns(scancode, 0, 0);
		User::LeaveIfError(id);
		User::LeaveIfError(ids2.Append(id));
	}
}

void capture_keysL(RAWindowGroup& wg, RAArray<TInt>& ids, RAArray<TInt>& ids2, TBool aCaptureRightKey)
{
	if (ids.Count()>0) return;

	capturekeyL(wg, ids, ids2, EKeyLeftArrow, EStdKeyLeftArrow);
	capturekeyL(wg, ids, ids2, EKeyRightArrow, EStdKeyRightArrow);
	capturekeyL(wg, ids, ids2, JOY_CLICK, EStdKeyDevice3);
	capturekeyL(wg, ids, ids2, EKeyUpArrow, EStdKeyUpArrow);
	capturekeyL(wg, ids, ids2, EKeyDownArrow, EStdKeyDownArrow);
	capturekeyL(wg, ids, ids2, EKeyYes, EStdKeyYes);
	capturekeyL(wg, ids, ids2, EKeyNo, EStdKeyNo);

	if (aCaptureRightKey) capturekeyL(wg, ids, ids2, EKeyDevice1, EStdKeyDevice1);
}

void cancel_keysL(RAWindowGroup& wg, RAArray<TInt>& ids, RAArray<TInt>& ids2)
{
	int i=0;
	while (i<ids.Count()) {
		wg.CancelCaptureKey(ids[i++]);
	}
	ids.Reset();
	i=0;
	while (i<ids2.Count()) {
		wg.CancelCaptureKeyUpAndDowns(ids2[i++]);
	}
	ids2.Reset();
}

void do_start_keycapture(TAny* aPtr)
{
	RAWsSession ws; ws.ConnectLA();

	worker_info *wi=(worker_info*)aPtr;
	TKeycaptureArgs *args=0;
	if (wi) args=(TKeycaptureArgs*)wi->worker_args;

	TRequestStatus status=KRequestPending;
	
	RAArray<TInt> ids, ids2;
	RAWindowGroup wg(ws); wg.ConstructLA((TUint32)&wg, EFalse);

	RAAknKeyLock keylock; keylock.ConnectLA();

	TBool aDoRightKey=EFalse;

	TInt uid;
	HAL::Get(HAL::EMachineUid, uid);
	if (uid==0x101FB3DD ||
		uid==0x101F4FC3 ||
		uid==0x101F466A ) {
		/* on 1st ed and 2nd ed no FP only: 7650, 3650, 3660 and 6600 */
#ifdef DO_RIGHTKEY
#  ifndef NO_PRESENCE
	if (args) aDoRightKey=args->right_softkey_mapped;
#  endif
#endif //DO_RIGHTKEY
		}

	capture_keysL(wg, ids, ids2, aDoRightKey);

	bool book_exists=false;
	bool log_exists=false;
	TVwsViewId callid, recentid,context_logid;

	{
		RAFs fsSession; fsSession.ConnectLA();

/*
		// ContextBook activation
		if (BaflUtils::FileExists(fsSession, contextbook_filen)) {
			callid=TVwsViewId(KUidcontextbook, TUid::Uid(1));
			recentid=TVwsViewId(KUidcontextbook, TUid::Uid(2));
			book_exists=true;
		} 
		// ContextCallLog activation
		if (BaflUtils::FileExists(fsSession, contextcalllog_filen)) {
			recentid = TVwsViewId(KUidContextCallLog, TUid::Uid(8));
			log_exists=true;
		} 
*/
		// ContextContact activation
		if (BaflUtils::FileExists(fsSession, contextcontacts_filen)) {
			callid=TVwsViewId(KUidContextContacts, TUid::Uid(1));
			book_exists=true;
		}
		if (BaflUtils::FileExists(fsSession, contextcontacts_filen2)) {
			callid=TVwsViewId(KUidContextContacts, TUid::Uid(1));
			book_exists=true;
		}
		
	}

#ifdef DO_TIMER
	RATimer timer; timer.CreateLocalLA();
	TBool timer_is_active=EFalse;
	TRequestStatus timer_status=KRequestPending;
	TWsEvent prev_keypress;
	TInt	prev_down_scancode=0;
	TTimeIntervalMicroSeconds32 key_repeat_rate(250*1000);
	TTimeIntervalMicroSeconds32 key_repeat_initial(500*1000);
#endif

#ifdef DO_KEYLOCK_TIMER
	RATimer keylocktimer; keylocktimer.CreateLocalLA();
	TRequestStatus keylock_status=KRequestPending;
	TBool keylock_is_active=EFalse;
	TTimeIntervalMicroSeconds32 keylock_wait(1000*1000);
#endif

#ifndef __S60V3__
	auto_ptr<screen> screenhelp(new (ELeave) screen);
	screenhelp->ConstructL(&ws);
#endif

	auto_ptr<CVwsSessionWrapper> vws(CVwsSessionWrapper::NewL());

#ifdef CAMERA_HACK
	wg.EnableFocusChangeEvents();
#endif
	wg.EnableScreenChangeEvents();
	ws.EventReady(&status);

	wg.SetOrdinalPosition(-1);
	wg.EnableReceiptOfFocus(EFalse);
	auto_ptr<CApaWindowGroupName> wn(CApaWindowGroupName::NewL(ws));
	wn->SetHidden(ETrue);
	wn->SetWindowGroupName(wg);

	TBool is6630=EFalse;
	TInt mach;
	if ( HAL::Get(HALData::EMachineUid, mach)==KErrNone ) {
		if (mach==0x101FBB55) is6630=ETrue;
	}
	//TRequestStatus parent_thread_status;
	RAThread parent; 
	if (wi) parent.OpenLA(wi->parent_threadid);
	/*
		We'd like to actually react to parent thread death
		and try to log it, put once the main thread of a process
		is killed all other threads are too, that means this thread :-)
	*/

	TInt wgid=ws.GetFocusWindowGroup();
	auto_ptr<CApaWindowGroupName> gn(CApaWindowGroupName::NewL(ws, wgid));

	for(;;) {
		User::WaitForAnyRequest();
		
		if ( wi && (*wi->do_stop)!=KRequestPending && (*wi->do_stop)!=KErrNone) {
			break;
		}
		
#ifdef DO_TIMER
		if (timer_status.Int()==KErrNone) {
			if (timer_is_active) {
				TKeyEvent* aKeyEvent=prev_keypress.Key();
				aKeyEvent->iRepeats++;
				ws.SendEventToWindowGroup(wgid, prev_keypress);
				timer.After(timer_status, key_repeat_rate);
			}
			timer_status=KRequestPending;
		}
		if (timer_status.Int()==KErrCancel) {
			timer_status=KRequestPending;
			// skip
		}
#endif
#ifdef DO_KEYLOCK_TIMER
		if (keylock_status.Int()!=KRequestPending) {
			if ( ( ! keylock.IsKeyLockEnabled() ) && args && args->iKeyStatus &&
					*(args->iKeyStatus)==KRequestPending ) {
				TRequestStatus *sp=args->iKeyStatus;
				parent.RequestComplete(sp, KErrNone);
			}
			keylock_status=KRequestPending;
			keylock_is_active=EFalse;
			continue;
		}
#endif
		if (status.Int()!=KRequestPending && status.Int()!=KErrNone) {
			prev_down_scancode=0;
			status=KRequestPending;
			ws.EventReady(&status);
		}
		if (status.Int()==KErrNone) {
			status=KRequestPending;
			TWsEvent e;
			ws.GetEvent(e);
#ifdef DO_TIMER
			if (timer_is_active) {
				timer.Cancel();
				User::WaitForRequest(timer_status);
				timer_status=KRequestPending;
				timer_is_active=EFalse;
			}
#endif
			if ( ( ! keylock.IsKeyLockEnabled() ) &&
				e.Type() != EEventFocusGroupChanged && args && args->iKeyStatus &&
				*(args->iKeyStatus)==KRequestPending) {
				TRequestStatus *sp=args->iKeyStatus;
				parent.RequestComplete(sp, KErrNone);
			}
			if (e.Type()==EEventScreenDeviceChanged) {
				cancel_keysL(wg, ids, ids2);
				capture_keysL(wg, ids, ids2, aDoRightKey);
				ws.EventReady(&status);
				continue;
			}
			if (e.Type()==EEventKeyDown) {
				TKeyEvent* aKeyEvent=e.Key();
				prev_down_scancode=aKeyEvent->iScanCode;
			} else {
				if (e.Type()==EEventKey) {
					TKeyEvent* aKeyEvent=e.Key();
					if (aKeyEvent->iScanCode != prev_down_scancode) prev_down_scancode=0;
				} else {
					prev_down_scancode=0;
				}
			}
			
#ifdef CAMERA_HACK
			// this doesn't seem to make a difference
			if (e.Type() == EEventFocusGroupChanged) {
				wgid=ws.GetFocusWindowGroup();
				gn.reset(CApaWindowGroupName::NewL(ws, wgid));
				RThread me;
				if (		gn->AppUid()==KCameraUid ||
						gn->AppUid()==KCamera2Uid ||
						gn->AppUid()==KVideoUid ||
						gn->AppUid()==KVideo2Uid ) {

					// we can't afford to do work
					// when the camera app is being used,
					// because it otherwise can fail with -9
					//
					me.SetPriority(EPriorityAbsoluteBackground);
					if (timer_is_active) {
						timer.Cancel();
						User::WaitForRequest(timer_status);
						timer_status=KRequestPending;
						timer_is_active=EFalse;
					}
					cancel_keysL(wg, ids, ids2);
				} else {
					me.SetPriority(EPriorityAbsoluteForeground);
					capture_keysL(wg, ids, ids2, aDoRightKey);
				}
				ws.EventReady(&status);
				continue;
			}
			// and it seems that we might not get the focus group change in
			// time, but have to manually check it anyway
#endif

			TInt current_wg=ws.GetFocusWindowGroup();
			if (current_wg != wgid) {
				gn.reset(CApaWindowGroupName::NewL(ws, current_wg));
			}
			wgid=current_wg;

#ifdef DO_KEYLOCK_TIMER
			if (keylock_is_active) {
				keylocktimer.Cancel();
				User::WaitForRequest(keylock_status);
				keylock_status=KRequestPending;
				keylock_is_active=EFalse;
			}

			keylocktimer.After(keylock_status, keylock_wait);
			keylock_is_active=ETrue;
#endif

			TInt c;
			TKeyEvent* aKeyEvent=e.Key();
			c=aKeyEvent->iCode;

#ifdef DO_TIMER
			prev_keypress=e;
			if (e.Type()==EEventKey && prev_down_scancode==aKeyEvent->iScanCode) {
				if ( aKeyEvent->iScanCode == EStdKeyLeftArrow ||
					aKeyEvent->iScanCode == EStdKeyRightArrow ||
					aKeyEvent->iScanCode == EStdKeyUpArrow ||
					aKeyEvent->iScanCode == EStdKeyDownArrow) {

					// only repeat arrows, repeating softkeys breaks
					// the keypad locking behaviour

					timer.After(timer_status, key_repeat_initial);
					timer_is_active=ETrue;
				}
			} else {
				ws.SendEventToWindowGroup(wgid, e);
				ws.EventReady(&status);
				continue;
			}
#endif

			// There's no Phone app on the emulator, so
			// we test it with Menu; on the phone the joystick
			// is used for the menu so we cannot use it
#ifndef __S60V3__
#ifndef __WINS__
			if (!is6630 && 
				e.Type()==EEventKey && aKeyEvent->iRepeats==0 &&
				gn->AppUid()==KUidPhone && 
				! screenhelp->dialog_on_screen()) {
#else
			if (e.Type()==EEventKey && aKeyEvent->iRepeats==0 &&
				gn->AppUid()==KUidMenu && 
				! screenhelp->dialog_on_screen() /*&& book_exists*/) {
#endif				
				switch(aKeyEvent->iScanCode) {
#ifdef DO_RIGHTKEY
				case EStdKeyDevice1: {
					vws->ActivateView(TVwsViewId(KUidcontext_log, KUserViewId), TUid::Null(),KNullDesC8);
					//vws->ActivateView(TVwsViewId(KUidContextMenu, TUid::Uid(1)), TUid::Null(), KNullDesC8); 
					//auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("c:\\system\\apps\\contextmenu\\contextmenu.app")));
					//cmd->SetCommandL(EApaCommandRun);
					//CC_TRAPD(err, EikDll::StartAppL(*cmd));
					break;
				}
#endif
					// not necessary anymore since overriding UIDs
				case EStdKeyDevice3:
					if (callid!=TVwsViewId() && aDoRightKey)
						vws->ActivateView(callid,TUid::Null(),KNullDesC8);
					else
						ws.SendEventToWindowGroup(wgid, e);
					break;
#if 0
				case EStdKeyYes:
					if (recentid!=TVwsViewId())
						vws->ActivateView(recentid,TUid::Null(),KNullDesC8);
					else
						ws.SendEventToWindowGroup(wgid, e);
					break;
#endif
				default:
					ws.SendEventToWindowGroup(wgid, e);
					break;
				}
			} else 	
#endif
			{
				// otherwise send event to foreground app
				ws.SendEventToWindowGroup(wgid, e);
			}
			ws.EventReady(&status);
		}
		if ( wi && (*wi->do_stop)!=KRequestPending && (*wi->do_stop)!=KErrNone) {
			break;
		}
	}
	ws.EventReadyCancel();
#ifdef DO_TIMER
	if (timer_is_active) timer.Cancel();
#endif
#ifdef DO_KEYLOCK_TIMER
	if (keylock_is_active) keylocktimer.Cancel();
#endif
}

TInt start_keycapture(TAny* aPtr)
{

	CTrapCleanup *cl;
	cl=CTrapCleanup::New();

	TInt err=0;
	CC_TRAP(err,
	        do_start_keycapture(aPtr));

	delete cl;

	TTimeIntervalMicroSeconds32 w(50*1000);
	User::After(w);
	worker_info* wi=(worker_info*)aPtr;
	if (wi) wi->stopped(err);
    return err;
}
