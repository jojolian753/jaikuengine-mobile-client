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

#include "blackboard_cs.h"
#include <e32math.h>
#include "context_uids.h"
#include "server_startup.h"
#include "break.h"

#include "blackboardclientsession.h"

static const TUint KDefaultMessageSlots = 2;
static const TUid KServerUid3 = { CONTEXT_UID_BLACKBOARDSERVER }; // matches UID in server/group/contextserver.mbm file

_LIT(KBlackBoardServerFilename, "BlackBoardServer");


EXPORT_C RBBClient::RBBClient() : RSessionBase(), iArgs(0), iArgPckg(0),
	iTupleArg(0), iTupleArgPckg(0), iIdPtr(0, 0, 0),
	iFullPtr(0, 0, 0), iNotifyFullPtr(0, 0, 0), iTupleNamePckg(iTupleName),
	iComponentNamePckg(iComponentName)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("RBBClient"));

	
}

EXPORT_C TInt RBBClient::Connect()
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Connect"));

	TInt result;
	
#if defined(__WINS__)
	CC_TRAP(result, StartServerL(KBlackBoardServerName, BlackBoardThreadFunction));
#else
	CC_TRAP(result, StartServerL(KBlackBoardServerName, KServerUid3, KBlackBoardServerFilename));
#endif
	if (result == KErrNone)
	{
		result = CreateSession(KBlackBoardServerName,
			Version(),
			KDefaultMessageSlots);	
	}
	iArgs=0; iArgPckg=0; iTupleArg=0; iTupleArgPckg=0;
	if (result==KErrNone) {
		iArgs=new TFullArgs;
		if (iArgs) iArgPckg=new TPckg<TFullArgs>(*iArgs);
		iTupleArg=new TTupleArgs;
		if (iTupleArg) iTupleArgPckg=new TPckg<TTupleArgs>(*iTupleArg);
		if (!iArgs || !iArgPckg || !iTupleArg || !iTupleArgPckg) {
			Close();
			result=KErrNoMemory;
		}
	}

	return result;
}

EXPORT_C TVersion RBBClient::Version() const
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Version"));

	return(TVersion(KBlackBoardServMajorVersionNumber,
		KBlackBoardServMinorVersionNumber,
		KBlackBoardServBuildVersionNumber));
}

//----------------------------------------------------------------------------------------

#ifndef __S60V2__
void RBBClient::SendReceive(TInt aFunction, TRequestStatus& aStatus)
{
	RSessionBase::SendReceive(aFunction, 0, aStatus);
}
void RBBClient::SendReceive(TInt aFunction)
{
	RSessionBase::SendReceive(aFunction, 0);
}
#endif

EXPORT_C void RBBClient::CancelOther()
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("CancelOther"));

	SendReceive(ECancelOther);
}

EXPORT_C void RBBClient::CancelNotify()
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("CancelNotify"));

	SendReceive(ECancelNotify);
}

EXPORT_C void RBBClient::TerminateBlackBoardServer(TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("TerminateBlackBoardServer"));

	SendReceive(ETerminateBlackBoardServer, aStatus);
}

void	RBBClient::MakeIdPtr(TUint& aId)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("MakeIdPtr"));

	iIdPtr.Set((TUint8*)&aId, sizeof(TUint), sizeof(TUint));
}

void	RBBClient::MakeFullPtr(TFullArgs& aFull, TPtr8& aInto)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("MakeFullPtr"));

	aInto.Set((TUint8*)&aFull, sizeof(TFullArgs), sizeof(TFullArgs));
}

EXPORT_C void RBBClient::Put(TTupleName aTupleName, const TDesC& aSubName, 
		const TComponentName aComponent,
		const TDesC8& aSerializedData, TBBPriority aPriority, 
		TBool aReplace, TUint& aIdInto, TRequestStatus& aStatus,
		const TTime& aLeaseExpires,
		TBool aPersist, TBool aNotifySender, 
		TBool aIsReply, TBool aKeepExisting)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Put"));

	if (aSubName.Length() > KMaxTupleSubNameLength) {
		TRequestStatus* s=&aStatus;
		User::RequestComplete(s, KErrTooBig);
		return;
	}
	iArgs->iLeaseExpires=aLeaseExpires;
	iArgs->iTupleName=aTupleName;
	iArgs->iSubName=aSubName;
	iArgs->iComponentName=aComponent;
	iArgs->iPriority=aPriority;
	if (aIsReply) {
		iArgs->iTupleType=ETupleReply;
	} else {
		iArgs->iTupleType=ETupleDataOrRequest;
	}

	TInt flags=0;
	if (aReplace) flags|=KArgFlagReplace;
	if (!aPersist) flags|=KArgFlagDontPersist;
	if (!aNotifySender) flags|=KArgFlagDoNotNotifySender;
	if (aKeepExisting) flags|=KArgFlagKeepExisting;

	MakeIdPtr(aIdInto);

#ifndef __S60V2__
	// IN flags, IN TFullArgs, OUT_OPT TUint id, IN data
	TAny* params[KMaxMessageArguments];
	params[0]=(void*)flags;
	params[1]=(void*)iArgPckg;
	params[2]=(void*)&iIdPtr;
	params[3]=(void*)&aSerializedData;
#else
	TIpcArgs params(flags, iArgPckg, &iIdPtr, &aSerializedData);
#endif
	SendReceive(EPut, params, aStatus);

}

EXPORT_C void RBBClient::Get(const TTupleName& aName, const TDesC& aSubName, 
		TFullArgs& aMeta, TDes8& aSerializedData,
		TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Get"));

	if (aSubName.Length() > KMaxTupleSubNameLength) {
		TRequestStatus* s=&aStatus;
		User::RequestComplete(s, KErrTooBig);
		return;
	}

	// flags, IN TTupleName|TTupleArgs, OUT TFullArgs, OUT data
	MakeFullPtr(aMeta, iFullPtr);
#ifndef __S60V2__
	TAny* params[KMaxMessageArguments];
#else
	TIpcArgs params;
#endif
	TInt flags=0;
	if (aSubName.Length()==0) {
		flags=KArgFlagNoSubName;
		iTupleName=aName;
#ifndef __S60V2__
		params[1]=(void*)&iTupleNamePckg;
#else
		params.Set(1, &iTupleNamePckg);
#endif
	} else {
		iTupleArg->iTupleName=aName;
		iTupleArg->iSubName=aSubName;
#ifndef __S60V2__
		params[1]=iTupleArgPckg;
#else
		params.Set(1, iTupleArgPckg);
#endif
	}
#ifndef __S60V2__
	params[0]=(void*)flags;
	params[2]=(void*)&iFullPtr;
	params[3]=(void*)&aSerializedData;

#else
	params.Set(0, flags);
	params.Set(2, &iFullPtr);
	params.Set(3, &aSerializedData);
#endif
	SendReceive(EGetByTuple, params, aStatus);
}

EXPORT_C void RBBClient::Get(const TComponentName& aName,
		TFullArgs& aMeta, TDes8& aSerializedData,
		TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Get"));

	// flags, IN TComponentName, OUT TFullArgs, OUT data
	MakeFullPtr(aMeta, iFullPtr);
	TInt flags=0;
	iComponentName=aName;
#ifndef __S60V2__
	TAny* params[KMaxMessageArguments];
	params[1]=(void*)&iComponentNamePckg;
	params[0]=(void*)flags;
	params[2]=(void*)&iFullPtr;
	params[3]=(void*)&aSerializedData;

#else
	TIpcArgs params(&iComponentNamePckg, flags, &iFullPtr, &aSerializedData);
#endif
	SendReceive(EGetByComponent, params, aStatus);
}

EXPORT_C void RBBClient::AddNotificationL(const TTupleName& aTupleName, 
		TBool aGetExisting, TBBPriority aPriority,
		TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("AddNotificationL"));

	// flags,  IN TTupleName.iModule.iUid, TTupleName.iId, TBBPriority
	TInt flags=0;
	if (aGetExisting) flags=KArgFlagGetExisting;
#ifndef __S60V2__
	TAny* params[KMaxMessageArguments];

	params[0]=(void*)flags;
	params[1]=(void*)aTupleName.iModule.iUid;
	params[2]=(void*)aTupleName.iId;
	params[3]=(void*)aPriority;
#else
	TIpcArgs params(flags, aTupleName.iModule.iUid, aTupleName.iId, aPriority);
#endif

	SendReceive(EAddNotifyByTupleFilter, params, aStatus);
}

EXPORT_C void RBBClient::AddNotificationL(const TComponentName& aComponentName, 
		TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("AddNotificationL"));

	// not in use,  IN TComponentName.iModule.iUid, IN TComponentName.iId, not in use
#ifndef __S60V2__
	TAny* params[KMaxMessageArguments];

	params[0]=(void*)0;
	params[1]=(void*)aComponentName.iModule.iUid;
	params[2]=(void*)aComponentName.iId;
	params[3]=(void*)0;
#else
	TIpcArgs params(0, aComponentName.iModule.iUid, aComponentName.iId, 0);
#endif

	SendReceive(EAddNotifyByComponentFilter, params, aStatus);
}

EXPORT_C void RBBClient::WaitForNotify(TFullArgs& aMeta, TDes8& aSerializedData, 
		TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("WaitForNotify"));

	// not used, not used, OUT TFullArgs, OUT data
	MakeFullPtr(aMeta, iNotifyFullPtr);
#ifndef __S60V2__
	TAny* params[KMaxMessageArguments];
	params[0]=0;
	params[1]=0;
	params[2]=(void*)&iNotifyFullPtr;
	params[3]=(void*)&aSerializedData;
#else
	TIpcArgs params(0, 0, &iNotifyFullPtr, &aSerializedData);
#endif

	SendReceive(ENotifyOnChange, params, aStatus);
}

EXPORT_C void RBBClient::Delete(TUint id, 
			TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Delete"));

	// not in use,  IN TUint, not in use, not in use

#ifndef __S60V2__
	TAny* params[KMaxMessageArguments];
	params[0]=0;
	params[1]=(void*)id;
	params[2]=0;
	params[3]=0;
#else
	TIpcArgs params(0, id, 0, 0);
#endif

	SendReceive(EDeleteById, params, aStatus);
}

EXPORT_C void RBBClient::Delete(TTupleName aTupleName, const TDesC& aSubName,
			TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Delete"));

	if (aSubName.Length() > KMaxTupleSubNameLength) {
		TRequestStatus* s=&aStatus;
		User::RequestComplete(s, KErrTooBig);
		return;
	}

	// flags,  tuplename, not in use, not in use
#ifndef __S60V2__
	TAny* params[KMaxMessageArguments];
#else
	TIpcArgs params;
#endif
	TInt flags=0;
	if (aSubName.Length()==0) {
		flags=KArgFlagNoSubName;
		iTupleName=aTupleName;
#ifndef __S60V2__
		params[1]=(void*)&iTupleNamePckg;
#else
		params.Set(1, &iTupleNamePckg);
#endif
	} else {
		iTupleArg->iTupleName=aTupleName;
		iTupleArg->iSubName=aSubName;
#ifndef __S60V2__
		params[1]=iTupleArgPckg;
#else
		params.Set(1, iTupleArgPckg);
#endif
	}
#ifndef __S60V2__
	params[0]=(void*)flags;
	params[2]=0;
	params[3]=0;
#else
	params.Set(0, flags);
#endif
	SendReceive(EDeleteByTuple, params, aStatus);
}

EXPORT_C void RBBClient::Delete(TComponentName aComponentName,
	TRequestStatus& aStatus)
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Delete"));

	// 0,  component uid, component id, not in use
#ifndef __S60V2__
	TAny* params[KMaxMessageArguments];
#else
	TIpcArgs params;
#endif
	iComponentName=aComponentName;
#ifndef __S60V2__
	params[1]=(void*)aComponentName.iModule.iUid;
	params[2]=(void*)aComponentName.iId;
#else
	params.Set(1, aComponentName.iModule.iUid);
	params.Set(2, aComponentName.iId);
#endif

#ifndef __S60V2__
	params[0]=(void*)0;
	params[3]=0;
#else
	params.Set(0, 0);
#endif
	SendReceive(EDeleteByComponent, params, aStatus);
}

EXPORT_C void RBBClient::Close()
{
	CALLSTACKITEM_N(_CL("RBBClient"), _CL("Close"));

	delete iArgs; iArgs=0;
	delete iArgPckg; iArgPckg=0;
	delete iTupleArg; iTupleArg=0;
	delete iTupleArgPckg; iTupleArgPckg=0;

	RSessionBase::Close();
}
