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

#ifndef __BLACKBOARDCLIENTSESSION_H__
#define __BLACKBOARDCLIENTSESSION_H__

#include <e32base.h>
#include "bbdata.h"
#include "blackboard_cs.h"

/*
 * You can always have an outstanding WaitForNotify,
 * but the rest have to be waited upon before calling
 * another one
 */

class RBBClient : public RSessionBase {
public:
	IMPORT_C RBBClient();
	IMPORT_C TInt Connect();
	IMPORT_C TVersion Version() const;
	
	IMPORT_C void TerminateBlackBoardServer(TRequestStatus& aStatus);
	IMPORT_C void Put(TTupleName aTupleName, const TDesC& aSubName, 
			const TComponentName aComponent,
			const TDesC8& aSerializedData, TBBPriority aPriority, 
			TBool aReplace, TUint& aIdInto, TRequestStatus& aStatus,
			const TTime& iLeaseExpires, 
			TBool aPersist=ETrue, TBool aNotifySender=EFalse,
			TBool aIsReply=EFalse, TBool aKeepExisting=EFalse);
	IMPORT_C void Get(const TTupleName& aName, const TDesC& aSubName, 
			TFullArgs& aMeta, TDes8& aSerializedData,
			TRequestStatus& aStatus);
	IMPORT_C void Get(const TComponentName& aName,
			TFullArgs& aMeta, TDes8& aSerializedData,
			TRequestStatus& aStatus);

	IMPORT_C void AddNotificationL(const TTupleName& aTupleName, 
			TBool aGetExisting, TBBPriority aPriority,
			TRequestStatus& aStatus);
	IMPORT_C void AddNotificationL(const TComponentName& aComponentName, 
			TRequestStatus& aStatus);

	IMPORT_C void WaitForNotify(TFullArgs& aMeta, TDes8& aSerializedData, 
		TRequestStatus& aStatus);
	IMPORT_C void Delete(TUint id, 
		TRequestStatus& aStatus);
	IMPORT_C void Delete(TTupleName aTupleName, const TDesC& aSubName,
		TRequestStatus& aStatus);
	IMPORT_C void Delete(TComponentName aComponentName,
		TRequestStatus& aStatus);


	IMPORT_C void CancelNotify();
	IMPORT_C void CancelOther();
	IMPORT_C void Close();
private: 
	void	MakeIdPtr(TUint& aId);
	void	MakeFullPtr(TFullArgs& aFull, TPtr8& aInto);
#ifndef __S60V2__
	void SendReceive(TInt aFunction, TRequestStatus& aStatus);
	void SendReceive(TInt aFunction);
	using RSessionBase::SendReceive;
#endif

	TFullArgs	*iArgs;
	TPckg<TFullArgs> *iArgPckg;

	TTupleArgs	*iTupleArg;
	TPckg<TTupleArgs> *iTupleArgPckg;

	TPtr8  iIdPtr;
	TPtr8  iFullPtr, iNotifyFullPtr;

	TTupleName	iTupleName;
	TPckg<TTupleName> iTupleNamePckg;

	TComponentName	iComponentName;
	TPckg<TComponentName> iComponentNamePckg;
};

#endif

