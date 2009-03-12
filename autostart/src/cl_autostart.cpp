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

#include <apmrec.h>
#include <apmstd.h>
#include "cl_autostart.h"
#include <flogger.h>
#include "context_uids.h"

#ifdef __S60V2__
#include <sysutil.h>
#include <bautils.h>
#endif

CclAutostart::CclAutostart()
:CApaDataRecognizerType(KUidcl_autostart, CApaDataRecognizerType::ENormal)
{
	iCountDataTypes = 1;
}

TUint CclAutostart::PreferredBufSize()
{
	return 0;
}

TDataType CclAutostart::SupportedDataTypeL(TInt /*aIndex*/) const
{
	return TDataType();
}

void CclAutostart::DoRecognizeL(const TDesC& /*aName*/, const TDesC8&
			      /*aBuffer*/)
{
	// this function is never called
}

void CclAutostart::StartThread()
{
	TInt res = KErrNone;

	RFileLogger iLog;
	iLog.Connect();
	iLog.CreateLog(_L("Autostart"),_L("Autostart"),EFileLoggingModeAppend);
	iLog.Write(_L("Starting thread."));
	iLog.CloseLog();
	iLog.Close();
	
	//create a new thread for starting our application
	RThread * startAppThread;
	startAppThread = new RThread();
	
	User::LeaveIfError( res = startAppThread->Create(
		_L("Autostart starter"),
		CclAutostart::StartAppThreadFunction,
		KDefaultStackSize,
		KMinHeapSize,
		KMinHeapSize,
		NULL,
		EOwnerThread) );
	
	startAppThread->SetPriority(EPriorityNormal/*EPriorityLess*/);
	
	startAppThread->Resume();
	
	startAppThread->Close();
}


TInt CclAutostart::StartAppThreadFunction(TAny* /*aParam*/)
{
	
	User::After( TTimeIntervalMicroSeconds32(300*1000) );
#if 0
	//wait 5 seconds...
	RTimer timer; // The asynchronous timer and ...
	TRequestStatus timerStatus; // ... its associated request status
	timer.CreateLocal(); // Always created for this thread.
	// get current time (microseconds since 0AD nominal Gregorian)
	TTime time;
	time.HomeTime();
	// add ten seconds to the time
	TTimeIntervalSeconds timeIntervalSeconds(45);
	time += timeIntervalSeconds;
	// issue and wait
	timer.At(timerStatus,time);
	User::WaitForRequest(timerStatus);
#endif
	
	
	// create a TRAP cleanup
	CTrapCleanup * cleanup = CTrapCleanup::New();
	TInt err;
	if( cleanup == NULL )
	{
		err = KErrNoMemory;
	}
	else
	{
		TRAP( err, StartAppThreadFunctionL() );
	}
	delete cleanup;
	
	
	if (err!=KErrNone) 
		User::Panic(_L("autostart"), err);
	return err;
}

void CclAutostart::StartApp(TDes& fnAppPath, const TDesC& aArg)
{
	RProcess server;

	if (server.Create(fnAppPath, aArg) != KErrNone) {
		fnAppPath.Replace(0, 1, _L("e"));
		User::LeaveIfError(server.Create(fnAppPath, aArg));
	}
	server.Resume();
	server.Close();

}

void CclAutostart::StartAppThreadFunctionL()
{
#ifdef __WINS__
	const TUid starter_uid= KUidstarter;
	RApaLsSession ls;
	User::LeaveIfError(ls.Connect());
	CleanupClosePushL(ls);
	_LIT(filen, ""); // dummy
	TThreadId dummy;
	User::LeaveIfError( ls.StartDocument(filen, starter_uid, dummy) );
	CleanupStack::PopAndDestroy();
#else
	TFileName fnAppPath = _L("c:\\system\\programs\\cl_starter.exe");
	StartApp(fnAppPath, KNullDesC);

#  ifdef __S60V2__
	HBufC8* agent=SysUtil::UserAgentStringL();
	CleanupStack::PushL(agent);
	if ( (*agent).Left(8).Compare(_L8("NokiaN70")) == 0) {
		RFs fs; fs.Connect();
		CleanupClosePushL(fs);
		if ( 
			CONTEXT_UID_CONTEXTCONTACTS == 0x101F4CCE  && (
			BaflUtils::FileExists(fs, _L("c:\\system\\apps\\contextcontacts\\contextcontacts.app") )  ||
			BaflUtils::FileExists(fs, _L("e:\\system\\apps\\contextcontacts\\contextcontacts.app") ) )
		) {
			TBuf<30> cmdline=_L("contacts*");
			cmdline.AppendNum( (TInt) CONTEXT_UID_CONTEXTCONTACTS );
			StartApp(fnAppPath, cmdline);
		}
		if ( 
			BaflUtils::FileExists(fs, _L("c:\\system\\apps\\contextcalllog\\contextcalllog.app") ) ||
			BaflUtils::FileExists(fs, _L("e:\\system\\apps\\contextcalllog\\contextcalllog.app") )
		) {
			TBuf<30> cmdline=_L("call_log*");
			cmdline.AppendNum( (TInt) CONTEXT_UID_CONTEXTCALLLOG );
			StartApp(fnAppPath, cmdline);
		}
		CleanupStack::PopAndDestroy();
	}
	CleanupStack::PopAndDestroy();
#  endif
#endif

}

EXPORT_C CApaDataRecognizerType* CreateRecognizer()
{
	CApaDataRecognizerType* thing = new CclAutostart();
	
	//start thread for our application
	CclAutostart::StartThread();
	return thing;
}

// DLL entry point
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
{
	return KErrNone;
}
