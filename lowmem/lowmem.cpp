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

#ifndef __S60V3__
#include <apgcli.h>
#include <apgtask.h>
#include <w32std.h>
#include <APACMDLN.H>
#include <eikdll.h>
#include <c32comm.h>
#endif

//_LIT(KHeapName, "jaikusettings_heap");
//_LIT(KExeName, "jaikusettings.exe");
_LIT(KHeapName, "contextcontacts_heap");
_LIT(KExeName, "contextcontacts.exe");

#ifndef __S60V3__
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

void __cdecl RegisterWsExe(class TDesC16 const&);

typedef void (__cdecl *TRegisterWsExeFunction)(class TDesC16 const&);
#endif

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
#ifndef __S60V3__
	init_nw();
#endif
	RFile logFile; 
	User::LeaveIfError(logFile.Replace(fs, _L("c:\\lowmem.txt"), 
		EFileWrite));
	CleanupClosePushL(logFile); pushed++;
	logFile.Write(_L8("Starting...\n"));

	TBool done=EFalse;
#ifndef __S60V3__
	RSemaphore sem;
	User::LeaveIfError(sem.CreateGlobal(_L("WsExeSem"),0));
	CleanupClosePushL(sem); pushed++;
	RegisterWsExe(sem.FullName());

	/*
	 * There's no WSERV.LIB on newer Nokia SDKs, so
	 * we just load wserv.dll and call the first
	 * entry point with the assumption that
	 * RegisterWsExe will keep its ordinal */
/*
	RLibrary lib;
	User::LeaveIfError(lib.Load(_L("wserv.dll"), _L("z:\\system\\libs")));
	CleanupClosePushL(lib); pushed++;
	TRegisterWsExeFunction startWs=(TRegisterWsExeFunction)lib.Lookup(1);
	startWs(sem.FullName());

	*/


    RWsSession ws;
	TInt count=0;
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
    
#endif

	done=EFalse;
	TInt heap=20*1024;
	heap=566*1024;
	
	RChunk heapc;
	User::LeaveIfError(heapc.CreateGlobal(KHeapName, 20, 20));
	CleanupClosePushL(heapc); pushed++;
	TInt *heapp=(TInt*)heapc.Base();
	
	while (!done) {
		
		TInt pushed=0;

		TBuf8<200> msg=_L8("Running with heap size ");
		msg.AppendNum(heap/1024);
		msg.Append(_L8("kB"));
		*heapp=heap;

		logFile.Write(msg);
		TBuf<200> msg16; 
		msg16.Copy(msg);
		RDebug::Print(msg16);
		
#ifndef __S60V3__
		RThread thread;
		TInt err=thread.Create(_L("context_log"), 
			&run_app, // thread's main function
			20*1024, /* stack */
			0,  // parameters
			0,
			NULL,
			heap, /* min heap */
			heap, /* max heap */
			EOwnerProcess);
#else
		RProcess thread;
		TInt err=thread.Create(KExeName, KNullDesC);
#endif
		User::LeaveIfError(err);
#ifndef __S60V3__
		thread.SetPriority(EPriorityNormal);
#endif
		TRequestStatus s;
		thread.Logon(s);
		thread.Resume();
		User::WaitForRequest(s);
		msg=_L8(" done: ");
		msg.Append(thread.ExitCategory());
		msg.Append(_L8(" "));
		msg.AppendNum(thread.ExitReason());
		msg.Append(_L8("\n"));

		logFile.Write(msg);
		msg16.Copy(msg);
		RDebug::Print(msg16);
		/* if (thread.ExitType()==EExitKill &&
			thread.ExitReason()!=2003 &&
			thread.ExitReason()!=2004) done=ETrue; */
		thread.Close();

		CleanupStack::PopAndDestroy(pushed);
		//heap+=3*1024;
		heap+=1*1024;
		
		User::After( 100*1000 );
		//User::After( 5*60*1000*1000 );
	}

	CleanupStack::PopAndDestroy(pushed);
}

GLDEF_C int E32Main(void)
{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
#ifdef __S60V3__
	// wait for emulator startup
	//User::After( 15*100*1000 );
	User::After( 5*1000*1000 );
#endif
	TRAPD(err, run_app_thread());
	delete cleanupStack;
	return 0;
}
