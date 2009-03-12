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

#include "server_startup.h"

#include <e32math.h>
#include <e32std.h>
#include "app_context.h"
#include "raii_mutex.h"
#include "raii_e32std.h"

#ifdef __WINS__
static const TUint KServerMinHeapSize =  0x1000;  //  4K
static const TUint KServerMaxHeapSize = 0x1000000;  // 256K
#endif

#if defined(__WINS__)
void CreateServerProcessLA(const TDesC& aServerName, TThreadFunction aFunction,
				RAThread& aThreadInto);
#else
#  if defined(__WINS__)
void CreateServerProcessLA(const TDesC& aServerName, TUid aServerUid3, 
				    const TDesC& aServerFileName,
				RAThread& aThreadInto);
#  else
void CreateServerProcessLA(const TDesC& aServerName, TUid aServerUid3, 
				    const TDesC& aServerFileName,
				RAProcess& aThreadInto);
#  endif
#endif

EXPORT_C void MakeServerMutexName(TDes& aInto, const TDesC& aServerName)
{
	aInto=aServerName; aInto.Append(_L("mutex"));
}

EXPORT_C void MakeServerSemaphoreName(TDes& aInto, const TDesC& aServerName)
{
	aInto=aServerName; aInto.Append(_L("semap"));
}

#if defined(__WINS__)
EXPORT_C void StartServerL(const TDesC& aServerName, TThreadFunction aFunction)
#else
EXPORT_C void StartServerL(const TDesC& aServerName, TUid aServerUid3, const TDesC& aServerFileName)
#endif
{
	CALLSTACKITEM_N(_CL(""), _CL("StartServer"));

	TBuf<50> mutexname; MakeServerMutexName(mutexname, aServerName);
	RAMutex mutex; mutex.GlobalLA(mutexname);
	
	TFindServer findServer(aServerName);
	TFullName name;
	
	if (findServer.Next(name) == KErrNone)
        {
		return;
        }
	
#if defined(__WINS__)
	RAThread serverthread;
#else
	RAProcess serverthread;
#endif
	TBuf<50> semaphorename; MakeServerSemaphoreName(semaphorename, aServerName);
	RSemaphore semaphore;
	if (semaphore.CreateGlobal(semaphorename, 0)!=KErrNone) {
		User::LeaveIfError(semaphore.OpenGlobal(semaphorename));
	}

#if defined(__WINS__)
	CreateServerProcessLA(aServerName, aFunction, serverthread);
#else
	CreateServerProcessLA(aServerName, aServerUid3, aServerFileName, serverthread);
#endif
	
	TTimeIntervalMicroSeconds32 w(10*1000);
	TInt i=0;
	TInt result=KErrTimedOut;
	while (i < 17) { 
		// a couple of minutes, in case there's some db conversion or the like
#ifdef __S60V3__
		if (semaphore.Wait(w.Int())==KErrNone) {
#else
		if ( semaphore.Count()>0) {
#endif
			result=KErrNone;
			break;
		}
		TExitType etype=serverthread.ExitType();
		if (etype!=EExitPending) {
			// server died
			result=KErrServerTerminated;
			break;
		}
#ifndef __S60V3__
		User::After(w);
#endif
		w=w.Int()*2;
		i++;
	}
	if (result!=KErrNone) {
		User::Leave(result);
	}

	semaphore.Close();
}

#if defined(__WINS__)
void CreateServerProcessLA(const TDesC& aServerName, TThreadFunction aFunction,
				RAThread& aThreadInto)
#else
#  if defined(__WINS__)
void CreateServerProcessLA(const TDesC& aServerName, TUid aServerUid3, 
				    const TDesC& aServerFileName,
				RAThread& aThreadInto)
#  else
void CreateServerProcessLA(const TDesC& aServerName, TUid aServerUid3, 
				    const TDesC& aServerFileName,
				RAProcess& aThreadInto)
#  endif
#endif
{
	CALLSTACKITEM_N(_CL(""), _CL("CreateServerProcess"));

	TInt result;
	
	
#if defined(__WINS__)
	
//#  if !defined(EKA2)
#if 0
	const TUidType serverUid(KNullUid, KNullUid, aServerUid3);
	RLibrary lib;
	TInt err=lib.Load(aServerFileName, _L("z:\\system\\programs"), serverUid);
	if (err!=KErrNone) {
		err=lib.Load(aServerFileName);
	}
	User::LeaveIfError(err);
	
	//  Get the WinsMain function
	TLibraryFunction functionWinsMain = lib.Lookup(1);
	
	//  Call it and cast the result to a thread function
	TThreadFunction serverThreadFunction = reinterpret_cast<TThreadFunction>(functionWinsMain());
#  else
	TThreadFunction serverThreadFunction = aFunction;
#  endif
	
	TName threadName(aServerName);
	
	// Append a random number to make it unique
	threadName.AppendNum(Math::Random(), EHex);
	
#  if 0
//#  if !defined(EKA2)
	aThreadInto.CreateLA(threadName,   // create new server thread
		serverThreadFunction, // thread's main function
		KDefaultStackSize,
		NULL,  // parameters
		&lib,
		NULL,
		KServerMinHeapSize,
		KServerMaxHeapSize,
		EOwnerProcess);
	lib.Close();    // if successful, server thread has handle to library now
	
#  else
	aThreadInto.CreateLA(threadName, serverThreadFunction, 
		KDefaultStackSize,
		KServerMinHeapSize, KServerMaxHeapSize, 0, EOwnerProcess);

#  endif
	
	aThreadInto.SetPriority(EPriorityMore);
	
#else
	
	const TUidType serverUid(KNullUid, KNullUid, aServerUid3);
	aThreadInto.CreateLA(aServerFileName, _L(""), serverUid);
	
#endif
	
	aThreadInto.Resume();
	
}


