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
#include <saclient.h>

#include "context_uids.h"
#include <bautils.h>
#ifdef __S60V2__
#include <sysutil.h>
#endif
#include <APACMDLN.H>
#include <eikdll.h>
#include <flogger.h>
#include "cc_processmanagement.h"
/*
void DebugMsg(const TDesC& aMessage, RFileLogger& aDebugLog) {
	aDebugLog.Write(aMessage);
}
*/


// This is the mainloop game loop or whatever
//void mainloop(TDes8& aState, RFileLogger& aDebugLog)
void mainloop()
{
	TInt pushed=0;
	RWsSession *wsp=0; 
	wsp=new (ELeave) RWsSession;
	CleanupStack::PushL(wsp); pushed++;
	RWsSession& ws=*wsp;
	User::LeaveIfError(ws.Connect());
	CleanupClosePushL(ws); pushed++;

	{
#ifndef __WINS__
		//DebugMsg(_L("KillExeL starter"), aDebugLog);
		ProcessManagement::KillExeL(_L("c:\\system\\programs\\cl_starter.exe")/* , aDebugLog */);
#else
		//DebugMsg(_L("KillApplication starter")/* , aDebugLog */);
		ProcessManagement::KillApplicationL(ws, KUidstarter, 500);
#endif
	}
	{
		//DebugMsg(_L("KillApplication context_log")/* , aDebugLog */);
		ProcessManagement::KillApplicationL(ws, KUidcontext_log, 500);
	}
	{
		//DebugMsg(_L("KillApplication context_log")/* , aDebugLog */);
		ProcessManagement::KillApplicationL(ws, KUidContextContacts, 500);
	}
	{
		//DebugMsg(_L("KillApplication context_log")/* , aDebugLog */);
		ProcessManagement::KillApplicationL(ws, KUidContextCallLog, 500);
	}

	CleanupStack::PopAndDestroy(pushed);

}

_LIT(KTxtEPOC32EX,"SHUTTER");


void reporterror(TInt error, const TDesC8& state)
{
	RFs fs; if (fs.Connect()!=KErrNone) return;
	RFile f; 
	if (f.Replace(fs, _L("c:\\system\\data\\context\\shutter_error.txt"), EFileWrite|EFileShareAny)
		==KErrNone) {

		f.Write(_L8("Error in shutter: "));
		TBuf8<12> b; b.AppendNum(error); b.Append(_L8("\n"));
		f.Write(b);
		f.Write(state);
		f.Close();
	}
	fs.Close();
}

//TBuf8<80> state;

#ifdef __WINS__
EXPORT_C TInt InitEmulator() // mainloop function called by the emulator software
#else
GLDEF_C TInt E32Main() // mainloop function called by E32
#endif
{
#ifdef __WINS__
	__UHEAP_MARK;
#endif
	CTrapCleanup* cleanup=CTrapCleanup::New(); // get clean-up stack
	
#if 0
	RFileLogger* logp=0;
	logp=new (ELeave) RFileLogger;
	RFileLogger& log=*logp;
	TInt error=KErrNone;
	state=_L8("creating logger");
	if ((error=log.Connect())==KErrNone) {
		log.CreateLog(_L("shutter"), _L("log.txt"), EFileLoggingModeAppend);
		TRAP(error,mainloop(state, log)); // more initialization, then do example
		DebugMsg(_L("finished"), log);
		log.Close();
	}
	state=_L8("shutting downs");
	if (error!=KErrNone) reporterror(error, state);
	__ASSERT_ALWAYS(!error,User::Panic(KTxtEPOC32EX,error));
	delete logp;
#endif
	TInt error;
	TRAP(error,mainloop());
	delete cleanup;
#ifdef __WINS__
	__UHEAP_MARKEND;
#endif
	
	//CloseSTDLIB();
	User::Exit(0);
	
	return KErrNone;
}

#ifdef __WINS__

int GLDEF_C E32Dll(TDllReason)
{
	return(KErrNone);
}

#endif
