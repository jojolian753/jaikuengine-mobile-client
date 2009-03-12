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

#ifndef __BLACKBOARDSERVER__
#define __BLACKBOARDSERVER__

#include <e32base.h>
#include "app_context.h"
#include "bbdata.h"
#include "blackboard_cs.h"
#include <d32dbms.h>
#include "tuplestore.h"
#include "subscriptions.h"
#include "symbian_tree.h"
#include "compat_server.h"

class MBlackBoardObserver {
public:
	virtual void NotifyL(TUint aId, TBBPriority aPriority,
			TTupleType aTupleType,
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponent,
			const TDesC8& aSerializedData,
			const TTime& aExpires) = 0;
	virtual void NotifyDeletedL(const TTupleName& aTupleName, const TDesC& aSubName) = 0;
	virtual void NotifyL(TUint aId, TBBPriority aPriority) = 0;
};

class CBlackBoardServer : public SERVER_CLASS, public MContextBase, public MNotifyDeleted
{
public:

	static CBlackBoardServer* NewL(MApp_context& Context, TBool aRunAsServer=ETrue);
	~CBlackBoardServer();

	static TInt ThreadFunction(TAny* aNone);
	
	void IncrementSessions();
	void DecrementSessions();

private:
	void RunL();
	TInt RunError(TInt aError);
	TInt CheckedRunError(TInt aError);

	CBlackBoardServer(TInt aPriority, MApp_context& Context) ;

	void ConstructL(TBool aRunAsServer);

	static void PanicClient(const MESSAGE_CLASS& aMessage, TBlackBoardServPanic aReason);
	static void PanicServer(TBlackBoardServPanic aReason);
	static void ThreadFunctionL();
	static void InnerThreadFunctionL(MApp_context* Context);

	//-------------------------------------------------------------
	// communications ...
	//-------------------------------------------------------------
	
public:
	void GetL(TTupleName& aName, TDes& aSubName, TUint& aIdInto, 
		TTupleType& aTupleTypeInto,
		TComponentName& aComponentInto,
		RADbColReadStream& aDataInto, TUint& aSizeInto,
		TTime& aExpiresInto);

	void GetL(TComponentName& aComponent, TUint& aIdInto, TTupleType& aTupleTypeInto,
		TTupleName& aNameInto, TDes& aSubNameInto,
		RADbColReadStream& aDataInto, TUint& aSizeInto,
		TTime& aExpiresInto);

	void GetL(TUint aId, TTupleType& aTupleTypeInto,
		TTupleName& aName, TDes& aSubName,
		TComponentName& aComponentInto,
		RADbColReadStream& aDataInto, TUint& aSizeInto,
		TTime& aExpiresInto);

	TInt GetPermanentError();


	// this Leave's if storing the tuple fails and returns
	// an error code if notification fails for some
	TInt PutL(const TTupleName& aTupleName, const TDesC& aSubName, const TComponentName& aComponent,
		 auto_ptr<HBufC8> aSerializedData, TBBPriority aPriority, TBool aReplace, TUint& aIdInto,
		 const TTime& aLeaseExpires, 
		 TBool aPersist=ETrue, TTupleType aTupleType=ETupleDataOrRequest,
		 TBool aKeepExisting=EFalse);
	TInt PutL(const TTupleName& aTupleName, const TDesC& aSubName, const TComponentName& aComponent,
		 const TDesC8& aSerializedData, TBBPriority aPriority, TBool aReplace, TUint& aIdInto,
		 const TTime& aLeaseExpires, 
		 TBool aPersist=ETrue, TTupleType aTupleType=ETupleDataOrRequest,
		 TBool aKeepExisting=EFalse);
	void DeleteL(const TTupleName& aName, const TDesC& aSubName);
	void DeleteL(const TComponentName& aName);
	void DeleteL(TUint aId);

	TInt AddNotificationL(MBlackBoardObserver *aSession, const TTupleName& aTupleName, 
		TBool aGetExisting, TBBPriority aPriority);
	void DeleteNotificationL(MBlackBoardObserver *aSession, const TTupleName& aTupleName);
	TInt AddNotificationL(MBlackBoardObserver *aSession, const TComponentName& aComponentName, 
		TBool aGetExisting, TBBPriority aPriority);
	void DeleteNotificationL(MBlackBoardObserver *aSession, 
		const TComponentName& aComponentName);

	void DeleteAllNotificationsL(MBlackBoardObserver *aSession);
	void DeleteAllNotificationsL();

	void TerminateServer();
private:
	// these continue on errors but return the last error
	void NotifyTupleL(TUint aId, 
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponent,
			const TDesC8& aSerializedData,
			const TTime& aLeaseExpires);
	void NotifyComponentL(TUint aId, TBBPriority aPriority,
			TTupleType aTupleType,
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponent,
			const TDesC8& aSerializedData,
			const TTime& aLeaseExpires);
	void NotifyExistingL(MBlackBoardObserver *aSession, 
		const TTupleName& aTupleName, TBBPriority aPriority);
	void NotifyExistingL(MBlackBoardObserver *aSession, 
		const TComponentName& aComponentName, TBBPriority aPriority);

	virtual void NotifyDeleted(const TTupleName& aTupleName, const TDesC& aSubName);

	TInt iSessionCount;
#ifndef __IPCV2__
	CSharableSession *	NewSessionL(const TVersion &aVersion) const;
#else
	SESSION_CLASS *	NewSessionL(const TVersion &aVersion, const MESSAGE_CLASS &aMessage) const;
#endif

	CDb		*iDb;
	CTupleStore	*iTupleStore;
	CSubscriptions	*iSubscriptions;
	CGenericIntMap	*iSubDuplicate;

	class CPermanentSubscriptions* iPermanent;
	TUint	iNextNonPermanentId;
	TInt	iPermanentError;
};

#endif
