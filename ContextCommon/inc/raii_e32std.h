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

#ifndef CC_RAE32STD_H_INCLUDED
#define CC_RAE32STD_H_INCLUDED 1

#include <e32std.h>
#include "raii.h"

class RAThread : public RThread, public RABase {
public:
	inline void CreateLA(const TDesC& aName,TThreadFunction aFunction,TInt aStackSize,TInt aHeapMinSize,TInt aHeapMaxSize,TAny *aPtr,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Create(aName,aFunction,aStackSize,aHeapMinSize,aHeapMaxSize,aPtr,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
#ifndef EKA2
	inline void CreateLA(const TDesC& aName,TThreadFunction aFunction,TInt aStackSize,RHeap* aHeap,TAny* aPtr,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Create(aName,aFunction,aStackSize,aHeap,aPtr,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	inline void CreateLA(const TDesC& aName,TThreadFunction aFunction,TInt aStackSize,TAny* aPtr,RLibrary* aLibrary,RHeap* aHeap, TInt aHeapMinSize,TInt aHeapMaxSize,TOwnerType aType) {
		RABase::CloseRA();
		{ TInt err=Create(aName,aFunction,aStackSize,aPtr,aLibrary,aHeap,aHeapMinSize,aHeapMaxSize,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
#endif
	inline void OpenLA(const TDesC& aFullName,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Open(aFullName,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	inline void OpenLA(const TFindThread& aFind,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Open(aFind,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	inline void OpenLA(TThreadId aID,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Open(aID,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	~RAThread() { RABase::CloseRA(); }
	void CloseInner() { RThread::Close(); }
private:
	void Close() { }
};

class RATimer : public RTimer, public RABase {
public:
	inline void CreateLocalLA() {
		RABase::CloseRA();
		{ TInt err=CreateLocal();
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	~RATimer() { RABase::CloseRA(); }
	void CloseInner() { RTimer::Close(); }
private:
	void Close() { }
};

class RAProcess : public RProcess, public RABase {
public:
	inline void CreateLA(const TDesC& aFileName,const TDesC& aCommand,const TUidType& aUidType, TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Create(aFileName,aCommand,aUidType,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	inline void CreateLA(const TDesC& aFileName,const TDesC& aCommand,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Create(aFileName,aCommand,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	inline void OpenLA(const TDesC& aName,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Open(aName,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	inline void OpenLA(const TFindProcess& aFind,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Open(aFind,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	inline void OpenLA(TProcessId aId,TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=Open(aId,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	~RAProcess() { RABase::CloseRA(); }
	void CloseInner() { RProcess::Close(); }
private:
	void Close() { }
};

class RAChunk : public RChunk, public RABase {
public:
	inline void CreateLocalLA(TInt aSize, TInt aMaxSize, TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=CreateLocal(aSize,aMaxSize,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	inline void CreateGlobalLA(const TDesC &aName, TInt aSize, TInt aMaxSize, TOwnerType aType=EOwnerProcess) {
		RABase::CloseRA();
		{ TInt err=CreateGlobal(aName,aSize,aMaxSize,aType);
		User::LeaveIfError(err); } 
		iOpen=ETrue;
#ifndef __LEAVE_EQUALS_THROW__
		PutOnStackL();
#endif
	}
	~RAChunk() { RABase::CloseRA(); }
	void CloseInner() { RChunk::Close(); }
private:
	void Close() { }
};

#endif // CC_RAE32STD_H_INCLUDED
