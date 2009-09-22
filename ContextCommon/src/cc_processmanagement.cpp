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

#include "cc_processmanagement.h"

#include <w32std.h>
#include <APACMDLN.H>
#include <eikdll.h>
#include <flogger.h>
#include <apgcli.h>
#include <apgtask.h>
#include <bautils.h>


//#define DEBUG 1

#ifdef DEBUG
#define LOG(x) f.Write(x)
#else
#define LOG(x)
#endif

// Concepts:
// !Killing applications and processes!
// !Starting applications!
namespace ProcessManagement 
{
	EXPORT_C void KillApplicationL(RWsSession& aWs, TUid aUid, TInt aRetryInterval )
	{
		TTime wait_until; wait_until.HomeTime();
		wait_until+=TTimeIntervalSeconds(15);
	
		TApaTaskList taskList( aWs );

		for(;;) {
			TApaTask task = taskList.FindApp(aUid);
			if(! task.Exists()) {
				break;
			}
			TTime now; now.HomeTime();
			if (now < wait_until) {
				task.EndTask();
				//DebugMsg(_L("waiting..."), aDebugLog);
				User::After(TTimeIntervalMicroSeconds32(aRetryInterval*1000) );
			} else {
				break;
			}
		}
		TApaTask task = taskList.FindApp(aUid);
		if( task.Exists()) {
#ifdef __S60V3__
			task.KillTask();
#else
			RThread t;
			if (t.Open(task.ThreadId())==KErrNone) {
				//DebugMsg(_L("killing"), aDebugLog);
				t.Kill(2003);
				t.Close();
			}
#endif
		}
	}


	EXPORT_C void KillExeL(const TDesC& aProcessName)
	{
		//DebugMsg(_L("finding processes"), aDebugLog);
		TFindProcess f(_L("*"));
		TFullName *tp=new (ELeave) TFullName;
		CleanupStack::PushL(tp);
		TFullName& t=*tp;
		RProcess r;
		TInt len=aProcessName.Length();
		if (len==0) User::Leave(KErrArgument);
		while (f.Next(t)==KErrNone) {
			r.Open(t);
			CleanupClosePushL(r);
			if (r.FileName().Length()>=len && 
				r.FileName().Left(1).CompareF(_L("z"))!=0 && 
				r.FileName().Mid(1).CompareF(aProcessName.Mid(1))==0) {
				//DebugMsg(_L("killing"), aDebugLog);
				r.Kill(2003);
			}
			CleanupStack::PopAndDestroy();
		}
		CleanupStack::PopAndDestroy();
	}



	EXPORT_C void StartApplicationL(const TUid& aUid, const TDesC& aDocumentName, TInt aRetryInterval, TApaCommand aCommand)
	{
		TUid appToBeStarted = aUid;
		TInt pushed=0;
#ifdef DEBUG
		RFs fs; User::LeaveIfError(fs.Connect()); 
		CleanupClosePushL(fs); pushed++;
		RFile f; User::LeaveIfError(f.Replace(fs, _L("c:\\processmanagement.txt"), EFileWrite));
		CleanupClosePushL(f); pushed++;

		f.Write(_L8("starting\n"));
#endif
		TInt retries=0;
		LOG(_L8("new'ing ls session..."));
		RApaLsSession *lsp;
		lsp=new (ELeave) RApaLsSession;
		LOG(_L8("\n"));
		CleanupStack::PushL(lsp); ++pushed;
		RApaLsSession& ls(*lsp);

		while(retries<80) {
			TInt i_pushed=0;
			LOG(_L8("connecting ls session..."));
			TInt err=ls.Connect();
			LOG(_L8(": "));
			TBuf8<12> msg; msg.Num(err); msg.Append(_L("\n"));
			LOG(msg);
			if (err==KErrNone) {
				CleanupClosePushL(ls); ++i_pushed;
				pushed+=i_pushed;
				break;
			} else {
				CleanupStack::PopAndDestroy(i_pushed);
			}
			LOG(_L8("waiting..."));
			retries++;
			User::After(TTimeIntervalMicroSeconds32(aRetryInterval*1000));
			LOG(_L8("done.\n"));
		}
		LOG(_L8("ls session created\n"));

		LOG(_L8("creating info..."));
		TApaAppInfo* infop=0;
		infop=new (ELeave) TApaAppInfo;
		LOG(_L8("1 "));
		CleanupStack::PushL(infop); ++pushed;
		LOG(_L8("2 "));
		TApaAppInfo& info(*infop);
		LOG(_L8("done\n"));
	
		RWsSession ws;
		User::LeaveIfError(ws.Connect());
		CleanupClosePushL(ws); ++pushed;
		TApaTaskList tl(ws);
		TApaTask app_task=tl.FindApp(aUid);
		TBool exists=app_task.Exists();
		while( !exists && retries<80) {
			TInt pushed=0;

			LOG(_L8("getting info..."));
			TInt err=ls.GetAppInfo(info, appToBeStarted);
			LOG(_L8("done: "));
			TBuf8<12> msg; msg.Num(err); msg.Append(_L("\n"));
			LOG(msg);
			if (err==KErrNone) {
#ifndef __S60V3__
				CApaCommandLine* cmd=CApaCommandLine::NewLC(info.iFullName); pushed++;
#else
				CApaCommandLine* cmd=CApaCommandLine::NewLC(); pushed++;
				cmd->SetExecutableNameL(info.iFullName);
#endif
				cmd->SetCommandL( aCommand );
				if ( aDocumentName.Length() > 0 )
					{					
					cmd->SetDocumentNameL(aDocumentName);
					}
#ifndef __S60V3__
				TRAP(err, EikDll::StartAppL(*cmd));
#else
				err=ls.StartApp(*cmd);
#endif
				CleanupStack::PopAndDestroy(pushed);
				LOG(_L8("StartAppL: "));
				msg.Num(err); msg.Append(_L("\n"));
				LOG(msg);
				if (err==KErrNone) break;	
			} else {
				LOG(_L8("popping..."));
				CleanupStack::PopAndDestroy(pushed);
				LOG(_L8("done\n"));
			}
			LOG(_L8("waiting..."));
			retries++;
			User::After(TTimeIntervalMicroSeconds32(aRetryInterval*1000));
			LOG(_L8("done.\n"));
		}
		LOG(_L8("done\n"));
		CleanupStack::PopAndDestroy(pushed);
	}

	EXPORT_C TBool IsAutoStartEnabled(RFs& aFs, TChar aDrive)
	{
		TFileName* fnp=new (ELeave) TFileName;
		CleanupStack::PushL(fnp);
		TFileName& fn=*fnp;
		fn.Append(aDrive);
		fn.Append(_L(":"));
#ifndef __S60V3__
		fn.Append(_L("\\system"));
#endif
		fn.Append(_L("\\Data\\Context\\enable_autostart.dat"));

		TBool ret=BaflUtils::FileExists(aFs, fn);
		CleanupStack::PopAndDestroy();
		return ret;
	}
	EXPORT_C void SetAutoStartEnabledL(RFs& aFs, TChar aDrive, TBool aEnabled)
	{
		TFileName* fnp=new (ELeave) TFileName;
		CleanupStack::PushL(fnp);
		TFileName& fn=*fnp;
		fn.Append(aDrive);
		fn.Append(_L(":"));
#ifndef __S60V3__
		fn.Append(_L("\\system"));
#endif
		fn.Append(_L("\\Data\\Context\\enable_autostart.dat"));
		if (!aEnabled) {
			TInt err=aFs.Delete(fn);
			if (err!=KErrNone && err!=KErrNotFound && err!=KErrPathNotFound) User::Leave(err);
		} else {
			BaflUtils::EnsurePathExistsL(aFs, fn);
			RFile f; User::LeaveIfError(f.Replace(aFs, fn, EFileWrite));
			f.Close();
		}
		CleanupStack::PopAndDestroy();
	}
	EXPORT_C TBool IsRunningL(RWsSession& aWs, const TUid& aAppUid)
	{
		TApaTaskList taskList( aWs );

		TApaTask task = taskList.FindApp(aAppUid);
		return  task.Exists();
	}
}
