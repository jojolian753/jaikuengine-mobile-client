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

#include <e32cons.h>
#include <f32file.h>
#include <apgcli.h>
#include <apgtask.h>
#include <w32std.h>
#include <APACMDLN.H>
#include <eikdll.h>
#include <c32comm.h>

void init_nw()
{
#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM") // alternatively "FCOMM"
#endif
	TInt err=User::LoadPhysicalDevice(PDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists) User::Leave(err);
	err=User::LoadLogicalDevice(LDD_NAME);
	if (err!=KErrNone && err!=KErrAlreadyExists) User::Leave(err);

	User::LeaveIfError(StartC32());
}

#ifndef __S60V3__

void run_app_inner()
{
	TInt pushed=0;

	TFileName fn=_L("z:\\system\\apps\\context_log\\context_log.app");
	CApaCommandLine* cmd=CApaCommandLine::NewL(fn);
	/* RunAppInsideThread deletes the command line */
	EikDll::RunAppInsideThread(cmd);
	
	CleanupStack::PopAndDestroy(pushed);
}

TInt run_app(TAny*)
{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	TRAPD(err, run_app_inner());
	delete cleanupStack;
	return err;
}

#endif

void __cdecl RegisterWsExe(class TDesC16 const&);

typedef void (__cdecl *TRegisterWsExeFunction)(class TDesC16 const&);

void run_app_thread()
{
	/*
	 * It's crucial that we bring the fileserver, window server
	 * and networking into existence _outside_ the application
	 * thread. Some of them allocate on this thread's heap, but
	 * only once the application runs -> those allocations would
	 * be counted as memory leaks for the application.
	 */
	TInt pushed=0;
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs); pushed++;
	init_nw();
	RFile logFile; 
	User::LeaveIfError(logFile.Replace(fs, _L("c:\\alloctrace.txt"), 
		EFileWrite));
	CleanupClosePushL(logFile); pushed++;
	logFile.Write(_L8("Starting...\n"));

	RSemaphore sem;
	User::LeaveIfError(sem.CreateGlobal(_L("WsExeSem"),0));
	CleanupClosePushL(sem); pushed++;

	/*
	 * There's no WSERV.LIB on newer Nokia SDKs, so
	 * we just load wserv.dll and call the first
	 * entry point with the assumption that
	 * RegisterWsExe will keep its ordinal */

	RLibrary lib;
	User::LeaveIfError(lib.Load(_L("wserv.dll"), _L("z:\\system\\libs")));
	CleanupClosePushL(lib); pushed++;
	TRegisterWsExeFunction startWs=(TRegisterWsExeFunction)(TInt)lib.Lookup(1);
	//startWs(sem.FullName());

#ifndef __S60V3__
	RegisterWsExe(sem.FullName());
#endif

    RWsSession ws;
	TInt count=0;
	TBool done=EFalse;
	while (!done) {
		TInt err=ws.Connect();
		if (err==KErrNone) done=ETrue;
		else if (count<5) {
			User::After(TTimeIntervalMicroSeconds32(
				300*1000*1000));
			count++;
		} else User::Leave(err);
	}
    CleanupClosePushL(ws); pushed++;

	TRequestStatus s; s=KRequestPending;
	User::WaitForRequest(s);

	CleanupStack::PopAndDestroy(pushed);
}

GLDEF_C int E32Main(void)
{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	TRAPD(err, run_app_thread());
	delete cleanupStack;
	return 0;
}

//extern "C" {
	void __stdcall _E32Startup(void);
	void InstallHooks(void);

	void __stdcall _TraceStartup(void) {
		InstallHooks();
		_E32Startup();
	}
//};
