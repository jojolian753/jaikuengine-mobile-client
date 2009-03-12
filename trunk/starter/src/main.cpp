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

#include <e32base.h>
#include <e32cons.h>
#include <stdlib.h>
#include <apgcli.h>
#include <apgtask.h>
#include <w32std.h>
#ifndef __S60V3__
#include <saclient.h>
#endif

#include "app_context.h"
#include "app_context_impl.h"
#include "file_output_base.h"

#include "context_uids.h"
#include "callstack.h"
#include <bautils.h>
#ifdef __S60V2__
#include <sysutil.h>
#endif
#include <APACMDLN.H>
#include <eikdll.h>
#include "contextvariant.hrh"

const TUid KUidPhone = { 0x100058b3 };
const TUid KUidMenu = {  0x101f4cd2 };

const TInt KUidToBackgroundValue=0x2004;
const TUid KUidToBackground={KUidToBackgroundValue};

class TFileStack : public MCallStackVisitor {
public:
	TFileStack(Cfile_output_base& aInto) : iInto(aInto) { }
private:
	virtual void BeginStack()
	{
		iInto.log_message(_L8("Stack trace: "));
	}
	virtual void VisitItem(const TDesC& Name)
	{
		iInto.log_message(Name);
	}
	virtual void EndStack()
	{
	}

	Cfile_output_base& iInto;
};

const TInt KUidSimCStatusValue = 0x100052E9;
const TUid KUidSimCStatus = {KUidSimCStatusValue};
enum TSASimCStatus
        {
        ESACSimInitWait,
        ESACSimLockOperative,
        ESACSimPinVerifyRequired,
        ESACSimPermanentlyBlocked,
        ESACSimRemoved,
        ESACSimRejected,
        ESACSimBlocked,
        ESACSimOk
        };

enum TSWStartupReason
        {
    // Normal startup reasons (100..149)

    // Nothing set the (default value).
    ESWNone = 100,

    // Restore Factory Settings (Normal)
    ESWRestoreFactorySet = 101,

    // Language Switched
    ESWLangSwitch = 102,

    // Warranty transfer
    ESWWarrantyTransfer = 103,

    // Possibly needed for handling
    // power off & charger connected use case.
    ESWChargerConnected = 104,

    // Restore Factory Settings (Deep)
    ESWRestoreFactorySetDeep = 105
        };

enum TWD2StartupReason
{		
	ENormal,							/* Normal start-up */
	EAlarm,								/* RTC alarm */
	EMidnightAlarm,						/* RTC midnight alarm, not available to applications */
	EHiddenReset,						/* Hidden reset, not available to applications as start-up reason*/
	ECharger,							/* Charger connected */
	ETest,								/* Test mode */
	ELocal,								/* Local mode. Not available to applications, applications will be in Test*/
	ESelftestFail,						/* Not available to applications */
	ENotReadFromHardware,				/* Not available to applications */
	ENotKnown							/* Not available to applications */
};

enum TSWState
{
    // Normal states (200..249)

    // Null value
    ESWStateUnknown = 200,  

    // Initialising UIKON & AVKON etc.
    ESWStateInitialising = 201,

    // Starting up applications (the very first startup)
    ESWStartingUp = 202,

    // Normal state, derived from main startup reason.
    ESWStateNormal = 203,

    // Shutting down the device.
    ESWStateShuttingDown = 204,

    // Alarm state, derived from startup main reason
    ESWStateAlarm = 205,
    
    // Charging state, derived from main startup reason.
    ESWStateCharging = 206,

    // Switching to normal state from Alarm and Charging.
    // This is for state transition without Sw reset. 
    ESWStateSwitchingToNormal = 207,

    // Shutting down done. This is the last state.
    ESWStateShutDown = 208,

    // Switching to alarm state (not used at the moment)
    ESWStateSwitchingToAlarm = 209,

    // Switching to alarm state (not used at the moment)
    ESWStateSwitchingToCharging = 210,

    // Self tests are OK.
    ESWStateSelfTestOK = 211,

    // Startup phase is OK.
    // This means all the startup activities in Symbian OS side are OK.
    ESWStateStartupOK = 212,

    // Phone has been started in light mode.
    ESWStatePhoneLightIdle = 213,

    // Phone is in security mode (SIM card removed)
    ESWStatePhoneLightSecurity = 214,
    
    // SW startup reasons: test, warranty transfer, restore factory set
    // are not visible to applications (at least not yet)

    // Error states (250..299)
    // Self test failed (fatal error)
    // Recovery: Display "Contact Service" message
    ESWStateSelfTestFail = 250,
    
    // Startup failed for non-critical application/server
    // Recovery: None (a message could be displayed)
    ESWStateStartupError = 251,

    // Startup failed for a critical application/server (fatal error)
    // Recovery: Display "Contact Service" message
    ESWStateFatalStartupError = 252,

    // Mounting FFS to C: drive failed, FFS mounted on RAM.
    // Recovery: optional possibility to format the FFS
    ESWStateFatalFFSError = 253
       
    // More startup error states can be added here
};


class SysStartup {
public:
	IMPORT_C static TInt ShutdownAndRestart( const class TUid& aSource, TSWStartupReason aReason);
        IMPORT_C static TInt GetStartupReasons(
            TWD2StartupReason& aMainReason, TSWStartupReason& aDetailReason);
        IMPORT_C static TSWState State();
};

#ifndef __S60V3__
TBool IsInRom(const TApaTask& task)
{
	RThread t; 
	User::LeaveIfError(t.Open(task.ThreadId()));
	CleanupClosePushL(t);
	RProcess p;
	User::LeaveIfError( t.Process(p) );
	CleanupClosePushL(p);
	TBool ret=EFalse;
	HBufC* cmd=HBufC::NewLC(p.CommandLineLength());
	TPtr cmdp=cmd->Des();
	p.CommandLine(cmdp);
	if (cmd->Left(1).CompareF(_L("z"))==0) ret=ETrue;
	CleanupStack::PopAndDestroy(3);
	return ret;
}
#else
TBool IsInRom(const TApaTask&)
{
	return EFalse;
}
#endif


void Reboot()
{
	TUid t={0x101FBAD3};
	SysStartup::ShutdownAndRestart( t, ESWNone);
}

#ifndef __S60V3__
TBuf8<30> msgr;

TBool Recover( RFs& fs, Cfile_output_base* fo ) 
{
	TInt err;
	TInt pushed=0;

	RFile f; TInt err2=f.Replace(fs, _L("c:\\system\\data\\context\\recovered.txt"),
		EFileWrite);
	if (err2==KErrNone) {
		CleanupClosePushL(f);
		pushed++;
	}
	if (BaflUtils::FileExists(fs, _L("c:\\system\\data\\ScShortcutEngine.bak")) ) {
		err=BaflUtils::CopyFile(fs, _L("c:\\system\\data\\ScShortcutEngine.bak"),
			_L("c:\\system\\data\\ScShortcutEngine.ini"));
		msgr=_L8("copy returns: ");
	} else {
		err=fs.Delete(_L("c:\\system\\data\\ScShortcutEngine.ini"));
		msgr=_L8("delete returns: ");
	}
	msgr.AppendNum(err);
	if (fo) fo->log_message(msgr);

	if (err==KErrNone && err2==KErrNone) {
		f.Write(_L8("shortcuts\n"));
	}
	CleanupStack::PopAndDestroy(pushed);
	User::After(TTimeIntervalMicroSeconds32(300*1000*1000));

	return (err==KErrNone);
}

#endif
//#define BOOT_DEBUG 1
//#define SHORTCUT_REPLACEMENT 1

#include "connectioninit.h"
#include "cc_processmanagement.h"

// This is the mainloop game loop or whatever
void mainloop(TDes8& aState)
{
	TInt pushed=0;
	TBool autostarted=EFalse;

	HBufC* cmdh=0;
#ifndef __WINS__
	aState=_L8("cmdh");
#  ifdef __S60V3__
	TInt len=User::CommandLineLength();
	if (len>0) {
		cmdh=HBufC::NewLC( len ); ++pushed;
	} else {
		cmdh=HBufC::NewLC( 40 ); ++pushed;
	}
	TPtr16 cmd=cmdh->Des();
	if (len>0) {
		User::CommandLine(cmd);
	} else {
		autostarted=ETrue;
		cmd=_L("context_log*");
		cmd.AppendNum( (TInt)KUidcontext_log.iUid );		
	}
#  else
	RProcess p=RProcess();
	cmdh=HBufC::NewLC( p.CommandLineLength() ); ++pushed;
	TPtr16 cmd=cmdh->Des();
	p.CommandLine(cmd);
#  endif
#else
	cmdh=HBufC::NewLC(30);  ++pushed;
	TPtr16 cmd=cmdh->Des();
	cmd=_L("context_log*");
	cmd.AppendNum( (TInt)KUidcontext_log.iUid );
#endif

	TBool N70=EFalse;
#if defined(__S60V2__) && !defined(__WINS__) && !defined(__S60V3__)
	{
		HBufC8* agent=SysUtil::UserAgentStringL();
		if ( (*agent).Left(8).Compare(_L8("NokiaN70"))==0) N70=ETrue;
		delete agent;
	}
#endif

#ifdef START_CONTEXT_LOG
	_LIT(Kstartapp_chunkname, "context_log");
	const TUid Kstartapp_uid = { CONTEXT_UID_CONTEXT_LOG };
#else
#  ifdef START_CONTEXTCONTACTS
	_LIT(Kstartapp_chunkname, "contacts");
	const TUid Kstartapp_uid = { CONTEXT_UID_CONTEXTCONTACTS };
#  endif
#endif

	TBuf<30> *chunknamep=new (ELeave) TBuf<30>;
	CleanupStack::PushL(chunknamep); ++pushed;

	TBuf<30>& chunkname=*chunknamep;
	TUid appuid;

	aState=_L8("appcontext::newl");
	CApp_context* ctx=CApp_context::NewL(true);
	CleanupStack::PushL(ctx); ++pushed;
	
	if (cmd.Length()==0 || autostarted) {	
		CConnectionOpener::CreateBootFileL(ctx->Fs(), ctx->DataDir()[0]);
	}

#ifndef __S60V3__
	aState=_L8("setdatadir");
	ctx->SetDataDir(_L("c:\\system\\data\\context\\"), false);
#endif
	if (ctx->NoSpaceLeft()) {
		CleanupStack::PopAndDestroy(pushed);
		return;
	}
	TFileName *fn=new (ELeave) TFileName;
	++pushed; CleanupStack::PushL(fn);
	TFileName &iWatchFileName=*fn;
	iWatchFileName=ctx->DataDir();
	iWatchFileName.Append(_L("watch-context_log.dat"));

/*
#ifndef __WINS__
	if (DOSReason==ECharger || SWReason==ESWChargerConnected) {
		User::After(TTimeIntervalMicroSeconds32(10*1000*1000));
		TUid t={0x101FBAD3};
		SysStartup::ShutdownAndRestart( t, ESWNone);
		return;
	}
#endif
*/
	if (cmd.Length() > 0) {
		TLex l(cmd);
		l.Mark();
		while (l.Peek()!='*' && !l.Eos()) {
			l.Inc();
		}
		chunkname=l.MarkedToken();
		l.Inc();
		TInt uid;
		User::LeaveIfError( l.Val(uid) );
		appuid.iUid=uid;
	} 
	if (cmd.Length()==0 || autostarted) {
		aState=_L8("fileexists_disable");
		RProcess me;
		if ( ! ProcessManagement::IsAutoStartEnabled(ctx->Fs(), me.FileName()[0]) ) {
#ifdef __S60V3__
			// wait so that the phone doesn't think we crashed
			User::After( 30*1000*1000 ); 
#endif
			CleanupStack::PopAndDestroy(pushed); 
			return;
		}
		appuid=Kstartapp_uid;
		chunkname=Kstartapp_chunkname;
		cmd=Kstartapp_chunkname;
		cmd.Append(_L("*"));
		cmd.AppendNum( (TInt)Kstartapp_uid.iUid );
	}

#ifdef EKA2
	TBuf<100>* semapp=new (ELeave) TBuf<100>;
	CleanupStack::PushL(semapp); ++pushed;
	TBuf<100>& semap=*semapp;
	semap=_L("CC_starter_");
	semap.Append(cmd);
	semap.Replace(semap.Locate('*'), 1, _L(" "));
	RSemaphore s; 
	if (s.CreateGlobal(semap, EOwnerThread)!=KErrNone) {
		CleanupStack::PopAndDestroy(pushed);
		return;
	}
	CleanupClosePushL(s); ++pushed;
#endif
	

	TInt aMaxLogCount=0;
	if ( BaflUtils::FileExists(ctx->Fs(), _L("c:\\system\\apps\\contextflickr\\contextflickr.app")) ) {
		aMaxLogCount=5;
	} 
	if ( BaflUtils::FileExists(ctx->Fs(), _L("e:\\system\\apps\\contextflickr\\contextflickr.app")) ) {
		aMaxLogCount=5;
	} 

	Cfile_output_base *fo=0;
#ifdef BOOT_DEBUG
#ifdef  __S60V3__
	ctx->SetDataDir(_L("e:\\system\\data\\context\\"), false);
#endif
#endif
	TInt recent_logs=0;
	if (cmd.Length()==0) {
		TBuf<30> name=_L("starter-");
		name.Append(chunkname);
		aState=_L8("fo=");
		fo=Cfile_output_base::NewL(*ctx, name, true, false, 5, &recent_logs);
		CleanupStack::PushL(fo); ++pushed;
	}
#ifdef BOOT_DEBUG
	{
		TBuf8<50> msg;
		TWD2StartupReason DOSReason;
		TSWStartupReason SWReason;
		SysStartup::GetStartupReasons(DOSReason, SWReason);
		msg=_L8("reasons: DOS ");
		msg.AppendNum(DOSReason);
		msg.Append(_L8(", SW "));
		msg.AppendNum(SWReason);
		if (fo) fo->log_message(msg);
	}
#endif
#ifdef SHORTCUT_REPLACEMENT
	if (recent_logs>2) {
		if (fo) fo->log_message(_L8("many recent log files"));
		TEntry* entry=new (ELeave) TEntry;	
		CleanupStack::PushL(entry); ++pushed;
		if (ctx->Fs().Entry(_L("c:\\system\\data\\ScShortcutEngine.ini"), *entry)==KErrNone) {
			TTime now; now.HomeTime(); now-=TTimeIntervalDays(1);
			if ( entry->iModified > now ) {
				if (fo) fo->log_message(_L8("recent scshortcutengine.ini"));
				//FIXME: notify
				if (Recover(ctx->Fs(), fo)) Reboot();
			}
			
		}
		CleanupStack::PopAndDestroy(); --pushed;
	}
#endif

	RTimer timer;
	CConsoleBase* console=0;
	TTime time;
	RApaLsSession ls;
	RWsSession ws;
	aState=_L8("ws.connect");
	if (fo) fo->log_message(aState);
	User::LeaveIfError(ws.Connect());
	CleanupClosePushL(ws); ++pushed;
	TRequestStatus app_status, timer_status;
	TApaTaskList tl(ws);

	aState=_L8("timer.create");
	if (fo) fo->log_message(aState);
	timer.CreateLocal();
	CleanupClosePushL(timer); ++pushed;
	aState=_L8("ls.connect");
	if (fo) fo->log_message(aState);
	User::LeaveIfError(ls.Connect());
	CleanupClosePushL(ls); ++pushed;

	if (1 || cmd.Length()==0) {
		// may be started by autostart with a command line now
		TBuf<30> name=_L("starter-");
		name.Append(chunkname);
		aState=_L8("fo=");
		if (!fo) {
			fo=Cfile_output_base::NewL(*ctx, name, true, false, 5);
			CleanupStack::PushL(fo); ++pushed;
		}

		//console=Console::NewL(_L("starter"),TSize(KConsFullScreen,KConsFullScreen));
		//CleanupStack::PushL(console); ++pushed;

#if !defined(__S60V3__) && !defined(__WINS__)
// this is started by startup list management on 3rd, which
// automatically waits until the right time
		if (fo) fo->log_message(_L8("waiting for phone start"));
		if (fo && N70) fo->log_message(_L8("N70"));
		// don't start app until phone has actually booted up
#  if !defined(__WINS__)
		RSystemAgent	iAgent;
		aState=_L8("agent.connect");
		User::LeaveIfError(iAgent.Connect());
		CleanupClosePushL(iAgent);
		while ( iAgent.GetState(KUidSimCStatus) != ESACSimOk ) {
#  else
		while (! tl.FindApp(KUidMenu).Exists()) {
#  endif
#  ifdef SHORTCUT_REPLACEMENT
#    ifndef __WINS__
			TSWState s=SysStartup::State();
			if (s==250 || s==252) {
				if (Recover(ctx->Fs(), fo)) {
					Reboot();
				}
			}
#    endif
#  endif
			timer.After(timer_status, 
				TTimeIntervalMicroSeconds32(3*1000*1000));
			User::WaitForRequest(timer_status);
		}
#  ifndef __WINS__
			CleanupStack::PopAndDestroy(); // iAgent
#  endif
#else
   		User::After(3*1000*1000);
#endif // !__S60V3__
		if (fo) fo->log_message(_L("phone started"));
	}

	RChunk stackchunk;
	aState=_L8("openglobal");
	if (fo) fo->log_message(aState);
	TInt cerr=stackchunk.OpenGlobal(chunkname, EFalse);
	if (cerr!=KErrNone) {
		aState=_L8("createglobal");
		if (fo) fo->log_message(aState);
		User::LeaveIfError(stackchunk.CreateGlobal(chunkname, 4096, 4096));
		*(TInt*)stackchunk.Base()=0;
		char* iCallStackZero=(char*)stackchunk.Base() + 4;
		TInt iCallStackSize=stackchunk.Size();
		TUint* uidp=(TUint*)(iCallStackZero+iCallStackSize-16);
		TInt* idp=(TInt*)(iCallStackZero+iCallStackSize-12);
		*uidp=0;
		*idp=0;
		*(TInt*)(iCallStackZero+iCallStackSize-20)=0;
	}
	CleanupClosePushL(stackchunk); ++pushed;

	TThreadId app_threadid;

	//wake up on thread death
	RThread app_thread;
	bool thread_is_open=false;

#ifndef __WINS__
	//TTimeIntervalHours wait_interval(1);
	TTimeIntervalMicroSeconds32 wait_interval(10*60*1000*1000);
#  ifndef __DEV__
	TTimeIntervalHours restart_interval(24);
#  else
	TTimeIntervalHours restart_interval(24);
	//TTimeIntervalMinutes restart_interval(30);
#  endif
	//TTimeIntervalMinutes restart_interval(1);
	//TInt restart_on_count=2;
#else
	//TTimeIntervalSeconds wait_interval(5);
	TTimeIntervalMicroSeconds32 wait_interval(2*60*1000*1000);
	TTimeIntervalMinutes restart_interval(24);
#endif

	int wait_count=0; // we wait 10 mins at a time for watchdog setting,
			// so count to 24*6...

	TTimeIntervalMicroSeconds32 wait_until;
	time.HomeTime();
#ifdef __WINS__
	wait_until=1*1000*1000;
	time += TTimeIntervalSeconds(1);
#else
	wait_until=1*1000*1000;
	time += TTimeIntervalSeconds(10);
#endif
	if (console) {
		TBuf<30> dt;
		time.FormatL(dt, _L("%F%Y-%M-%D %H:%T:%S\n"));
		console->Printf(dt);
	}

	bool done=false; bool restarting=false; bool doing_restart=false;
	bool logged_on=false;
	int restart_count=0, nw_restart_count=0; 
	int RESTART_MAX_NOSTART=5;
	int RESTART_MAX=RESTART_MAX_NOSTART;
#ifdef __DEV__
	RESTART_MAX=RESTART_MAX_NOSTART=1;
#endif
	TInt restart_time=30;

	TTime startedApp; startedApp.HomeTime();
	startedApp-=TTimeIntervalSeconds(10);
	
	if (cmd.Length()==0) {
		if (fo) fo->log_message(_L8("waiting initial"));
		timer.After(timer_status, wait_until);
		if (fo) fo->log_message(_L8("waited"));
		app_status=KRequestPending;
		doing_restart=restarting=true;
	} else {

		TBool app_exists=EFalse;
		TThreadId app_thread_id;
		TInt count=0;
		for(;;) {
			TApaTask app_task=tl.FindApp(appuid);
			if (app_task.Exists()) {
				if ( N70 && IsInRom(app_task)) {
					if (fo) fo->log_message(_L("killing built-in app"));
					app_task.KillTask();
				} else {
					app_thread_id=app_task.ThreadId();
					app_exists=ETrue;
				}
				break;
			}
			if (count>0) break;
			count++;
			User::After( 1000*1000 );
			wait_until=wait_until.Int()-1000*1000;
		}
		if (!app_exists) {
			wait_until=wait_until.Int()+1000*1000;
			if (fo) fo->log_message(_L8("waiting initial2"));
			timer.After(timer_status, wait_until);
			if (fo) fo->log_message(_L8("waited"));
			app_status=KRequestPending;
			doing_restart=restarting=true;
#ifdef __WINS__
			restart_time=0;
#else
			restart_time=1;
#endif
		} else {
			if (fo) fo->log_message(_L8("waiting initial3"));
			timer.After(timer_status, wait_interval);
			if (fo) fo->log_message(_L8("waited"));
			aState=_L8("app_thread.open");
			User::LeaveIfError(app_thread.Open(app_thread_id));
			thread_is_open=true;
			app_status=KRequestPending;
			app_thread.Logon(app_status); logged_on=true;
			doing_restart=restarting=false;
			restart_time=15;
		}
	}

	bool extra_debug=false;
#ifdef BOOT_DEBUG
	extra_debug=true;
#endif
	while(!done) {
		if (extra_debug && fo) fo->log_message(_L8("waiting"));
wait:
		User::WaitForRequest(timer_status, app_status);
		if (timer_status!=KRequestPending && !doing_restart && !restarting) {
			if (extra_debug && fo) fo->log_message(_L8("timer expired"));
			//RDebug::Print(_L("timer!"));
			if (console) {
				TBuf<30> dt;
				time.FormatL(dt, _L("%F%Y-%M-%D %H:%T:%S\n"));
				console->Printf(_L("%d "), timer_status.Int());
				console->Printf(dt);
			}
			TBool hung=EFalse;
			
			if (timer_status!=KErrNone) {
				// let context_log update the timestamp when the time 
				// moves forward
				User::After( TTimeIntervalMicroSeconds32(60*1000*1000) );
			}

			if (appuid==KUidcontext_log) {
				TEntry e;
				TInt err=ctx->Fs().Entry(
					iWatchFileName, e);
				TTime now; 
#ifndef __S60V3__
				now.HomeTime();
#else
				now.UniversalTime();
#endif
				now-=wait_interval;
				if (err!=KErrNone || e.iModified < now) {
					hung=ETrue;
					if (!fo) {
						TBuf<100> name=_L("starter-");
						name.Append(chunkname);
						fo=Cfile_output_base::NewL(*ctx, name, true, false, 5);
						CleanupStack::PushL(fo); ++pushed;
					}
					TDateTime dt=now.DateTime();
					TBuf<100> msg;
					msg.Append(_L("now ")); msg.AppendNum(dt.Hour());
					msg.Append(_L(":")); msg.AppendNum(dt.Minute());
					dt=e.iModified.DateTime();
					msg.Append(_L(" filetime ")); msg.AppendNum(dt.Hour());
					msg.Append(_L(":")); msg.AppendNum(dt.Minute());
					fo->log_message(msg);
					if (fo) fo->log_message(_L8("app hung"));
				}
			}
			TTime now; now.HomeTime();
			if (hung || (now > startedApp+restart_interval) ) {
				if (fo) fo->switch_file();
				TApaTask app_task=tl.FindApp(appuid);
				if (!app_task.Exists()) {
					if (fo) fo->log_message(_L8("app already dead"));
					restarting=true;
					doing_restart=true;
				} else {
					if (fo) fo->log_message(_L8("shutting down app "));
					
					ProcessManagement::KillApplicationL(ws, appuid);
					restart_count=0;

					if (console) console->Printf(_L("shut app\n"));
				}
				restarting=true;
				wait_count=0;
			} else {
				RESTART_MAX=50;
				wait_count++;
			}
			timer.After(timer_status,
				TTimeIntervalMicroSeconds32(60*1000*1000));
		} 
		if (timer_status!=KRequestPending && restarting && !doing_restart) {
			TApaTask app_task=tl.FindApp(appuid);
			if (app_task.Exists()) app_task.KillTask();
			timer.After( timer_status, wait_interval );
		}
		if (app_status!=KRequestPending || doing_restart) {
			TInt etype=0, ereason=0;
			TExitCategoryName ecat;
			if (app_status!=KRequestPending) {
				TApaTask app_task=tl.FindApp(appuid);
				TBool exists=app_task.Exists();
				if ( N70 && exists && IsInRom(app_task)) {
					if (fo) fo->log_message(_L("killing built-in app"));
					app_task.KillTask();
					User::After(TTimeIntervalMicroSeconds32(500*1000));
					exists=EFalse;
				}
				if (exists) {
					app_thread.Close();
					TThreadId app_thread_id=app_task.ThreadId();
					aState=_L8("app_thread.open_2");
					User::LeaveIfError(app_thread.Open(app_thread_id));
					app_status=KRequestPending;
					app_thread.Logon(app_status); logged_on=true;
					etype=ereason=0;
					restarting=false;
					time.HomeTime();
					time+=wait_interval;
					timer.Cancel();
					timer.At(timer_status, time);
					goto wait;
				} else {
					if (fo) fo->log_message(_L8("app stopped"));
					if (console) console->Printf(_L("app stopped\n"));
					logged_on=false;
					etype=app_thread.ExitType();
					ereason=app_thread.ExitReason();
					ecat=app_thread.ExitCategory();
					app_thread.Close(); thread_is_open=false;

					if (etype==0 && appuid==KUidcontext_log && ereason!=KErrDiskFull) {
						// check that it actually got started
						TEntry e;
						TInt err=ctx->Fs().Entry(
							iWatchFileName, e);
						TTime home; home.HomeTime();
						TTime univ; univ.UniversalTime();
						TTimeIntervalHours offset;
						home.HoursFrom(univ, offset);
						if (err!=KErrNone || (e.iModified+offset) < startedApp) {
							etype=2;
							ecat=_L("DIDNT START");
							if (!fo) {
								TBuf<100> name=_L("starter-");
								name.Append(chunkname);
								fo=Cfile_output_base::NewL(*ctx, name, true, false, 5);
								CleanupStack::PushL(fo); ++pushed;
							}
							TDateTime dt=e.iModified.DateTime();
							TBuf<100> msg;
							msg.Append(_L("watch err ")); msg.AppendNum(err);
							msg.Append(_L(" time ")); msg.AppendNum(dt.Hour());
							msg.Append(_L(":")); msg.AppendNum(dt.Minute());
							fo->log_message(msg);
						}
					}
					if (etype!=0 || ereason!=0) {
						if (!fo) {
							TBuf<100> name=_L("starter-");
							name.Append(chunkname);
							fo=Cfile_output_base::NewL(*ctx, name, true, false, 5);
							CleanupStack::PushL(fo); ++pushed;
						}
						TBuf8<100> msg=_L8("reasons: ");
						msg.AppendNum(etype); msg.Append(_L8(", "));
						msg.AppendNum(ereason); msg.Append(_L8(", "));
						msg.Append(ecat);
						fo->log_message(msg);
						TFileStack fs(*fo);
						TBool exiting=ctx->CallStackMgr().IsExiting((char*)stackchunk.Base());
						ctx->CallStackMgr().IterateStack(fs, (char*)stackchunk.Base());
						*(TInt*)stackchunk.Base()=0;
						if (exiting) {
							done=true;
							if (fo) fo->log_message(_L8("app crashed when shutting down"));
						}
					}
				}
			}
			
			TBool mmc_removed=( ctx->AppDir().Left(1).CompareF(_L("e"))==0 && !HasMMC(ctx->Fs()));
			if (mmc_removed) {
				fo=0;
			}
			if (!mmc_removed && !done && (etype!=0 || (etype==0 && ( ereason==2003 || ereason==2004 || (ereason!=0 && ereason!=KErrDiskFull) ) ) || restarting) && restart_count<RESTART_MAX) {

				if (ereason==2004) nw_restart_count++;
#ifndef __WINS__
				if (ereason==2004 && nw_restart_count > 1 ) {
					// network's f*cked up
					// better restart the whole phone
					if (!fo) {
						TBuf<100> name=_L("starter-");
						name.Append(chunkname);
						fo=Cfile_output_base::NewL(*ctx, name, true, false, 5);
						CleanupStack::PushL(fo); ++pushed;
					}
					if (fo) fo->log_message(_L8("restarting phone"));
					delete fo; fo=0;
					timer.Cancel();
					TTimeIntervalMicroSeconds32 w(30*1000*1000);
					timer.After(timer_status, w);
					User::WaitForRequest(timer_status);
					Reboot();
					TTimeIntervalMicroSeconds32 w2(30*1000*1000);
					timer.After(timer_status, w2);
					User::WaitForRequest(timer_status);
				}
#endif
				if (fo) fo->log_message(_L8("restarting app"));
				time.HomeTime();
				time+=TTimeIntervalSeconds(restart_time);
				restart_time=30;
				timer.Cancel();
				timer.At(timer_status, time);
				User::WaitForRequest(timer_status);
				RESTART_MAX=RESTART_MAX_NOSTART;

				TInt err;
#ifndef __S60V3__
				if (!N70 || (appuid!=KUidContextContacts && appuid!=KUidContextCallLog) ) {
#endif
					TApaAppInfo* infop=0;
					infop=new (ELeave) TApaAppInfo;
					TApaAppInfo& info(*infop);
					err=ls.GetAppInfo(info, appuid);
					if (err==KErrNone) {
#ifndef __S60V3__
						auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(info.iFullName));
#else
						auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL());
						cmd->SetExecutableNameL(info.iFullName);
#endif
						cmd->SetCommandL(EApaCommandBackground);
#ifndef __S60V3__
						TRAP(err, app_threadid=EikDll::StartAppL(*cmd));
#else
						err=ls.StartApp(*cmd, app_threadid);
#endif
					}
					delete infop;
#ifndef __S60V3__
				} else {
					TApaTask app_task=tl.FindApp(appuid);
					TBool exists=app_task.Exists();
					if ( N70 && exists && IsInRom(app_task)) {
						if (fo) fo->log_message(_L("killing built-in app"));
						app_task.KillTask();
						User::After(TTimeIntervalMicroSeconds32(500*1000));
					}
					TFileName *fnp=new (ELeave) TFileName;
					TFileName& fn=*fnp;
					if ( appuid == KUidContextContacts ) {
						fn=_L("c:\\system\\apps\\contextcontacts\\contextcontacts.app");
					} else {
						fn=_L("c:\\system\\apps\\contextcalllog\\contextcalllog.app");
					}
					if (! BaflUtils::FileExists(ctx->Fs(), fn) ) {
						fn.Replace(0, 1, _L("e"));
					}
					auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(fn));
					cmd->SetCommandL(EApaCommandBackground);
					//err=ls.StartApp(*cmd);
					TRAP(err, app_threadid=EikDll::StartAppL(*cmd));
					delete fnp;
				}
#endif
				restart_count++;
				if (err==KErrNone) {
					startedApp.HomeTime();
					startedApp-=TTimeIntervalSeconds(10);
					if (fo) fo->log_message(_L8("success in restarting app"));
					if (logged_on) {
						app_thread.LogonCancel(app_status);
						logged_on=false;
					}
					if (thread_is_open) {
						app_thread.Close();
						thread_is_open=false;
					}
	
					err=app_thread.Open(app_threadid);

					if (err==KErrNone) {
						thread_is_open=true;
						app_thread.Logon(app_status);
						logged_on=true;
						restarting=false;
						doing_restart=false;
						
						time.HomeTime();
						time+=wait_interval;
						timer.At(timer_status, time);
					}
					else
					{
						if (fo) fo->log_message(_L8("test"));
					}
				} 
				if (err!=KErrNone) {
					TBuf<30> msg;
					msg.Format(_L("Error starting %d\n"), err);
					if (console) {
						console->Printf(msg);
					}
					if (fo) fo->log_message(msg);
					doing_restart=true;
					time.HomeTime();
					time+=TTimeIntervalSeconds(30);
					timer.Cancel();
					timer.At(timer_status, time);
					restarting=true;
				}
			} else {
				// user shutdown
				done=true;
			}
		}
	}
	if (fo) fo->log_message(_L8("stopping"));
	if (logged_on) app_thread.LogonCancel(app_status);
	if (thread_is_open) app_thread.Close();
	if (console) {
		console->Getch(); // get and ignore character
	}
	CleanupStack::PopAndDestroy(pushed); //console, ws, ls, timer
}

_LIT(KTxtEPOC32EX,"STARTER");

void reporterror(TInt error, const TDesC8& state)
{
	RFs fs; if (fs.Connect()!=KErrNone) return;
	RFile f; 
	if (f.Replace(fs, _L("c:\\data\\context\\starter_error.txt"), EFileWrite|EFileShareAny)
		==KErrNone) {

		f.Write(_L8("Error in starter: "));
		TBuf8<12> b; b.AppendNum(error); b.Append(_L8("\n"));
		f.Write(b);
		f.Write(state);
		f.Close();
	}
	fs.Close();
}

#if defined(__WINS__) && !defined(__S60V3__)

EXPORT_C TInt InitEmulator() // mainloop function called by the emulator software
{
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New(); // get clean-up stack
	TBuf8<80> state;
	TRAPD(error,mainloop(state)); // more initialization, then do example
	if (error!=KErrNone) reporterror(error, state);
	__ASSERT_ALWAYS(!error,User::Panic(KTxtEPOC32EX,error));
	delete cleanup; // destroy clean-up stack
	__UHEAP_MARKEND;
	
	//CloseSTDLIB();
	User::Exit(0);
	
	return KErrNone;
}

int GLDEF_C E32Dll(TDllReason)
{
	return(KErrNone);
}

#else

GLDEF_C TInt E32Main() // mainloop function called by E32
{
	{
		RThread me; me.RenameMe(_L("starter"));
	}
#ifdef __WINS__
	User::After( 15*1000*1000 );
#endif
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New(); // get clean-up stack
	TBuf8<80> state;
	TRAPD(error,mainloop(state)); // more initialization, then do example
	if (error!=KErrNone) reporterror(error, state);
	if (error!=KErrNotReady && error!=KErrNoMemory && error!=KErrDiskFull) {
		__ASSERT_ALWAYS(!error,User::Panic(KTxtEPOC32EX,error));
	}
	delete cleanup; // destroy clean-up stack
	__UHEAP_MARKEND;
	
	//CloseSTDLIB();
	User::Exit(0);
	
	return KErrNone; // and return
}

#endif

