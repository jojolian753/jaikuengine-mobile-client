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

#include "BlackBoardServer.h"
#include "BlackBoardServerSession.h"
#include "BlackBoard_cs.h"
#include <E32SVR.H>
#include <basched.h>

#include "app_context.h"
#include "app_context_impl.h"
#include <flogger.h>
#include "symbian_auto_ptr.h"
#include "callstack.h"

#include "permanent.h"
#include "break.h"
#include "server_startup.h"
#include "reporting.h"
#include <charconv.h>
#include "inserts.h"

void Log(const TDesC& msg);

bool ClientAlive(TInt ThreadId);

#pragma warning(disable:4706)

CBlackBoardServer::CBlackBoardServer(TInt aPriority, MApp_context& Context) : SERVER_CLASS(aPriority), MContextBase(Context)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("CBlackBoardServer"));


	
}

CBlackBoardServer::~CBlackBoardServer()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("~CBlackBoardServer"));
	CBlackBoardServerSession* session=0;
	iSessionIter.SetToFirst();
	while( (session = reinterpret_cast<CBlackBoardServerSession*>(iSessionIter++)) ) {
		session->Exiting();
	}

	delete iSubDuplicate;
	delete iPermanent;
	delete iSubscriptions;
	delete iTupleStore;
	delete iDb;
}


CBlackBoardServer* CBlackBoardServer::NewL(MApp_context& Context, TBool aRunAsServer)
{
	CALLSTACKITEM2_N(_CL("CBlackBoardServer"), _CL("NewL"), &Context);


	CBlackBoardServer* ret = new (ELeave) CBlackBoardServer(EPriorityNormal, Context);
    	CleanupStack::PushL(ret);
    	ret->ConstructL(aRunAsServer);
    	CleanupStack::Pop();
    	return ret;
}

void CBlackBoardServer::ConstructL(TBool aRunAsServer)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("ConstructL"));

#ifdef __WINS__
	TInt dummy;
	TBreakItem ti(0, dummy);
#endif
	iNextNonPermanentId=0xf000000U;

	iSubDuplicate=CGenericIntMap::NewL();

	{
		CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("iDb"));
		iDb=CDb::NewL(AppContext(), _L("TUPLE"), EFileRead|EFileWrite|EFileShareAny);
	}
	{
		CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("TupleStore"));
		iTupleStore=CTupleStore::NewL(*iDb, AppContext(), *this);
	}
	{
		CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("Subscriptions"));
		iSubscriptions=CSubscriptions::NewL();
	}
	{
		CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("Permanent"));
		iPermanent=CPermanentSubscriptions::NewL(AppContext(), this);
	}
	
	CC_TRAPD(err, iPermanent->ReadSubscriptionsL());

	if (err!=KErrNone) {
		//todo: log
	}

	auto_ptr<CInserts> inserts(CInserts::NewL(AppContext(), this));
	CC_TRAP(err, inserts->ReadInsertsL());

	if (err!=KErrNone) {
		//todo: log
	}

	if (aRunAsServer) {
		StartL(KBlackBoardServerName);
	}
}

#ifndef __IPCV2__
CSharableSession* CBlackBoardServer::NewSessionL(const TVersion& aVersion) const
#else
SESSION_CLASS* CBlackBoardServer::NewSessionL(const TVersion& aVersion, const MESSAGE_CLASS &aMessage) const
#endif
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("NewSessionL"));


	// check version
	if (!User::QueryVersionSupported(TVersion(KBlackBoardServMajorVersionNumber,
                                                KBlackBoardServMinorVersionNumber,
                                                KBlackBoardServBuildVersionNumber),
                                                aVersion))
	{
		User::Leave(KErrNotSupported);
	}
	
	// create new session
#ifndef __IPCV2__
	RThread client = Message().Client();
	return CBlackBoardServerSession::NewL(client, *const_cast<CBlackBoardServer*> (this));
#else
	return CBlackBoardServerSession::NewL(*const_cast<CBlackBoardServer*> (this));
#endif
}


void CBlackBoardServer::IncrementSessions()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("IncrementSessions"));


    	iSessionCount++;
}

void CBlackBoardServer::DecrementSessions()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("DecrementSessions"));


    	iSessionCount--;
    	if (iSessionCount <= 0)
	{
      		iSessionCount =0;
		CActiveScheduler::Stop();
	}    
}

TInt CBlackBoardServer::RunError(TInt aError)
{
	return aError;
}

TInt CBlackBoardServer::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("CheckedRunError"));

	if (aError==KErrCorrupt) return aError;

	if (aError == KErrBadDescriptor)
	{
        	// A bad descriptor error implies a badly programmed client, so panic it;
        	// otherwise report the error to the client
        	PanicClient(Message(), EBBBadDescriptor);
	}
	else
	{
		Message().Complete(aError);
	}

	//
	// The leave will result in an early return from SERVER_CLASS::RunL(), skipping
	// the call to request another message. So do that now in order to keep the
	// server running.
	ReStart();

	return KErrNone;	// handled the error fully
}

void CBlackBoardServer::PanicClient(const MESSAGE_CLASS& aMessage, TBlackBoardServPanic aPanic)
{
	CALLSTACKITEMSTATIC_N(_CL("CBlackBoardServer"), _CL("PanicClient"));


	aMessage.Panic(KBlackBoardServer, aPanic);
}

void CBlackBoardServer::PanicServer(TBlackBoardServPanic aPanic)
{
	CALLSTACKITEMSTATIC_N(_CL("CBlackBoardServer"), _CL("PanicServer"));


    	User::Panic(KBlackBoardServer, aPanic);
}

TInt CBlackBoardServer::GetPermanentError()
{
	return iPermanentError;
}

void CBlackBoardServer::RunL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("RunL"));

	AppContext().CallStackMgr().ResetCallStack();
	if (NoSpaceLeft()) {
		iPermanentError=KErrDiskFull;
		delete iTupleStore; iTupleStore=0;
		delete iDb; iDb=0;
	}

	CC_TRAPD(err, SERVER_CLASS::RunL());
	if (err!=KErrNone) {
		TBuf<40> msg;
		msg.Format(_L("Error in RunL: %d"), err);
		auto_ptr<HBufC> s(AppContext().CallStackMgr().GetFormattedCallStack(_L("BBS")));
		Log(*s);
		//Log(msg);
		User::LeaveIfError(CheckedRunError(err));
	}

	AppContext().CallStackMgr().ResetCallStack();
}

void write_to_output(RFile& f, const TDesC& str, TDes8& buf, CCnvCharacterSetConverter* CC)
{
	buf.Zero();
	TInt len, pos=0;
	len=buf.MaxLength();
	while (pos < str.Length()) {
		TInt real_len;
		if (pos+len > str.Length()) {
			real_len=str.Length()-pos;
		} else {
			real_len=len;
		}
		CC->ConvertFromUnicode(buf, str.Mid(pos, real_len));
		pos+=len;
		f.Write(buf);
	}
}

#if defined(__S60V3__) && defined(__WINS__)
#include "allocator.h"
IMPORT_C void AllocateContextCommonExceptionData();
#endif

_LIT(KName, "Blackboardserver");
void CBlackBoardServer::ThreadFunctionL()
{
	RSemaphore process_semaphore;
	TInt err=process_semaphore.OpenGlobal(KName);
	if (err==KErrNotFound) {
		err=process_semaphore.CreateGlobal(KName, 1);
	}
	User::LeaveIfError(err);
	err=process_semaphore.Wait(30*1000*1000);
	if (err!=KErrNone) {
		process_semaphore.Close();
		User::Leave(err);
	}
	{
		RThread me; me.RenameMe(KName);
	}
#ifndef __S60V3__
	#if defined (__WINS__)
	UserSvr::ServerStarted();
	#endif
#endif
#if defined(__S60V3__) && defined(__WINS__)
	User::Free(User::Alloc(512*1024));
	RAllocator* orig=0;
	//FIXME3RD
	//orig=SwitchToLoggingAllocator();
#endif
#if defined(__S60V3__) && defined(__WINS__)
// force an exception, so that the relevant
// runtime structures are allocated before we
// call __UHEAP__MARK 
{
	TRAPD(ignore, {
		TBuf<1> b;
		User::Leave(1);
	});
	AllocateContextCommonExceptionData();
	
}
#endif

	__UHEAP_MARK;

	{
		auto_ptr<CApp_context> c(CApp_context::NewL(false, _L("BlackBoardServer")));
#ifndef __S60V3__
#  ifndef __WINS__
		c->SetDataDir(_L("c:\\system\\data\\context\\"), false);
#  endif
#endif
		TInt count=0;
again:
		TRAPD(err, InnerThreadFunctionL(c.get()));

		if (err!=KErrNone) {
			TBuf<50> msg=_L("Error in blackboard server: ");
			msg.AppendNum(err);
			msg.Append(_L("\n"));
			Log(msg);
			auto_ptr<HBufC> stack(c->CallStackMgr().GetFormattedCallStack(_L("AppUi")));
			if (stack.get()) 
			{
				Log(*stack);
			}

			if (err!=KErrDiskFull && err!=KLeaveExit) {
				RFile f; TInt err2=f.Replace(c->Fs(), _L("c:\\data\\context\\blackboard-crash.txt"),
					EFileWrite);
				TBuf8<64> buf;
				if (err2==KErrNone) {
					write_to_output(f, msg, buf, c->CC());
					if (stack.get()) write_to_output(f, *stack, buf, c->CC());
					f.Close();
				}
			}
		}

		if ( err==KErrCorrupt ) {
			//c->Reporting().ShowGlobalNote( EAknGlobalErrorNote, _L("Context Settings "
			c->Reporting().ShowGlobalNote( 4, _L("Jaiku Settings "
				L"have become corrupted and been deleted. You will have to rerun Welcome."
				));
#ifndef __WINS__
			c->Fs().Delete(_L("c:\\system\\data\\context\\tuple.db") );
			c->Fs().Delete(_L("c:\\data\\context\\tuple.db") );
#else
			c->Fs().Delete(_L("c:\\data\\context\\tuple.db") );
			c->Fs().Delete(_L("tuple.db") );
#endif
			++count;
			if (count==1) goto again;
		}
	}
	__UHEAP_MARKEND;
	
	process_semaphore.Signal(); process_semaphore.Close();
#if defined(__S60V3__) && defined(__WINS__)
	if (orig) SwitchBackAllocator(orig);
#endif
}

class CLeavingScheduler : public CBaActiveScheduler {
public:
	virtual void Error(TInt aError) const {
		User::Leave(aError);
	}
};

void CBlackBoardServer::InnerThreadFunctionL(MApp_context* Context)
{
	CALLSTACKITEM2_N(_CL("CBlackBoardServer"), _CL("InnerThreadFunctionL"), Context);
	// make sure the call stack item is not on the stack
	// after app context is deleted

    	// Construct active scheduler
    	auto_ptr<CActiveScheduler> activeScheduler(new (ELeave) CLeavingScheduler);

    	// Install active scheduler
    	// We don't need to check whether an active scheduler is already installed
    	// as this is a new thread, so there won't be one
    	CActiveScheduler::Install(activeScheduler.get());

	{
    		// Construct our server
		CALLSTACKITEM2_N(_CL("CBlackBoardServer"), _CL("InnerThreadFunctionL"), Context);
		auto_ptr<CBlackBoardServer> s(CBlackBoardServer::NewL(*Context));    

		RSemaphore semaphore;
		TBuf<50> semaphorename; MakeServerSemaphoreName(semaphorename, KBlackBoardServerName);
		if (semaphore.CreateGlobal(semaphorename, 0)!=KErrNone) {
			CALLSTACKITEM2_N(_CL("CBlackBoardServer"), _CL("Semaphore"), Context);
			User::LeaveIfError(semaphore.OpenGlobal(semaphorename));
		}
		
		// Semaphore opened ok
		semaphore.Signal();
		semaphore.Close();


		// Start handling requests
		CActiveScheduler::Start();
	}		 
}

#if defined(__WINS__)
EXPORT_C TInt BlackBoardThreadFunction(TAny* aParam)
{
	return CBlackBoardServer::ThreadFunction(aParam);
}
#endif

TInt CBlackBoardServer::ThreadFunction(TAny* /*aNone*/)
{
    	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if (cleanupStack == NULL)
	{
      	PanicServer(ECreateTrapCleanup);
	}

	TInt err;
	CC_TRAP2(err, ThreadFunctionL(), 0);
    	if (err != KErrNone)
	{
        	PanicServer(ESrvCreateServer);
	}
	
    	delete cleanupStack;
    	cleanupStack = NULL;

	return KErrNone;
}

//---------------------------------------------------------------------------

void CBlackBoardServer::GetL(TTupleName& aName, TDes& aSubName,
			     TUint& aIdInto, TTupleType& aTupleTypeInto,
		TComponentName& aComponentInto,
		RADbColReadStream& aDataInto, TUint& aSizeInto,
		TTime& aExpiresInto)

{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("GetL"));

	if (!iTupleStore->FirstL(ETupleDataOrRequest, aName, aSubName, ETrue)) User::Leave(KErrNotFound);
	iTupleStore->GetCurrentL(aName, aSubName, aIdInto, aTupleTypeInto, 
		aComponentInto, aDataInto, aSizeInto, aExpiresInto);
}

void CBlackBoardServer::GetL(TComponentName& aComponent, TUint& aIdInto, TTupleType& aTupleTypeInto,
		TTupleName& aNameInto, TDes& aSubNameInto,
		RADbColReadStream& aDataInto, TUint& aSizeInto,
		TTime& aExpiresInto)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("GetL"));

	if (!iTupleStore->FirstL(ETuplePermanentSubscriptionEvent, aComponent)) {
		if (!iTupleStore->FirstL(ETupleReply, aComponent)) {
			User::Leave(KErrNotFound);
		}
	}
	iTupleStore->GetCurrentL(aNameInto, aSubNameInto, aIdInto, aTupleTypeInto, 
		aComponent, aDataInto, aSizeInto, aExpiresInto);
}

void CBlackBoardServer::GetL(TUint aId, TTupleType& aTupleTypeInto,
			     TTupleName& aName, TDes& aSubName,
	TComponentName& aComponentInto,
	RADbColReadStream& aDataInto, TUint& aSizeInto,
	TTime& aExpiresInto)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("GetL"));

	iTupleStore->SeekL(aId);
	iTupleStore->GetCurrentL(aName, aSubName, aId, aTupleTypeInto, 
		aComponentInto, aDataInto, aSizeInto, aExpiresInto);
}


void CBlackBoardServer::NotifyTupleL(TUint aId,
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponent,
			const TDesC8& aSerializedData,
			const TTime& aExpires)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("NotifyTupleL"));

#ifdef __WINS__
	TBuf<100> msg;
	msg=_L("Notify of tuple [");
	msg.AppendNum(aTupleName.iModule.iUid, EHex);
	msg.Append(_L(" "));
	msg.AppendNum(aTupleName.iId);
	msg.Append(_L("]"));
	RDebug::Print(msg);
#endif

	MBlackBoardObserver *o=0;
	TBBPriority pr;
	iSubDuplicate->Reset();
	auto_ptr< CList<MBlackBoardObserver*> > to_notify(CList<MBlackBoardObserver*>::NewL());
	for (o=iSubscriptions->FirstL(aTupleName, pr); o; o=iSubscriptions->NextL(pr)) {
		if (! iSubDuplicate->GetData( (uint32) o) ) {
			to_notify->AppendL(o);
			iSubDuplicate->AddDataL((uint32) o, (void*)1);
		}
	}

	TInt err; TInt err_ret=KErrNone;

	while(o=to_notify->Pop()) {
		CC_TRAP(err, o->NotifyL(aId, pr, ETupleDataOrRequest, aTupleName, 
			aSubName, aComponent, aSerializedData, aExpires));
		if (err!=KErrNone) err_ret=err;
	}
	User::LeaveIfError(err_ret);
}

void CBlackBoardServer::NotifyComponentL(TUint aId, TBBPriority aPriority,
					 TTupleType aTupleType, 
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponent,
			const TDesC8& aSerializedData,
			const TTime& aExpires)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("NotifyComponentL"));

	auto_ptr< CList<MBlackBoardObserver*> > to_notify(CList<MBlackBoardObserver*>::NewL());
	MBlackBoardObserver *o=0;
	TBBPriority pr;
	iSubDuplicate->Reset();
	for (o=iSubscriptions->FirstL(aComponent, pr); o; o=iSubscriptions->NextL(pr)) {
		if (! iSubDuplicate->GetData( (uint32) o) ) {
			to_notify->AppendL(o);
			iSubDuplicate->AddDataL((uint32) o, (void*)1);
		}
	}
	TInt err; TInt err_ret=KErrNone;
	while (o=to_notify->Pop()) {
		CC_TRAP(err, o->NotifyL(aId, aPriority, aTupleType, aTupleName, aSubName, 
			aComponent, aSerializedData, aExpires));
		if (err!=KErrNone) err_ret=err;
	}
	User::LeaveIfError(err_ret);
}

TInt CBlackBoardServer::PutL(const TTupleName& aTupleName, const TDesC& aSubName, 
					const TComponentName& aComponent,
					auto_ptr<HBufC8> aSerializedData, TBBPriority aPriority, 
					TBool aReplace, TUint& aIdInto, const TTime& aLeaseExpires, 
					TBool aPersist, TTupleType aTupleType,
					TBool aKeepExisting)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("PutL"));

	if (aPersist && !aReplace && NoSpaceLeft()) {
		User::Leave(KErrDiskFull);
		//return KErrNone;
	}

	if (aPersist) {
		aIdInto=iTupleStore->PutL(aTupleType, aTupleName, aSubName, aComponent,
			*aSerializedData, aPriority, aReplace, aLeaseExpires,
			aKeepExisting);
		if (aKeepExisting && aIdInto==0) return KErrNone;
	} else {
		aIdInto=iNextNonPermanentId++;
	}

	TInt err1=KErrNone, err2=KErrNone;

	if (aTupleType==ETupleDataOrRequest) {
		CC_TRAP(err1, NotifyTupleL(aIdInto, aTupleName, aSubName,
			aComponent, *aSerializedData, aLeaseExpires));
	}
	{
		CC_TRAP(err2, NotifyComponentL(aIdInto, aPriority, aTupleType, aTupleName, aSubName,
			aComponent, *aSerializedData, aLeaseExpires));
	}

	if (err1!=KErrNone) return err1;
	return err2;
}

TInt CBlackBoardServer::PutL(const TTupleName& aTupleName, const TDesC& aSubName, 
					const TComponentName& aComponent,
					const TDesC8& aSerializedData, TBBPriority aPriority, 
					TBool aReplace, TUint& aIdInto, const TTime& aLeaseExpires, 
					TBool aPersist, TTupleType aTupleType,
					TBool aKeepExisting)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("PutL2"));

	if (aPersist && !aReplace && NoSpaceLeft()) {
		//FIXME reporting?
		//User::Leave(KErrDiskFull);
		return KErrNone;
	}

	if (aPersist) {
		aIdInto=iTupleStore->PutL(aTupleType, aTupleName, aSubName, aComponent,
			aSerializedData, aPriority, aReplace, aLeaseExpires, aKeepExisting);
			
		if (aKeepExisting && aIdInto==0) return KErrNone;
	} else {
		aIdInto=iNextNonPermanentId++;
	}

	TInt err1=KErrNone, err2=KErrNone;

	if (aTupleType==ETupleDataOrRequest) {
		CC_TRAP(err1, NotifyTupleL(aIdInto, aTupleName, aSubName,
			aComponent, aSerializedData, aLeaseExpires));
	} else {
		CC_TRAP(err2, NotifyComponentL(aIdInto, aPriority, aTupleType, aTupleName, aSubName,
			aComponent, aSerializedData, aLeaseExpires));
	}

	if (err1!=KErrNone) return err1;
	return err2;
}

void CBlackBoardServer::DeleteL(const TTupleName& aName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("DeleteL"));

	iTupleStore->DeleteL(ETupleDataOrRequest, aName, aSubName);
}

void CBlackBoardServer::DeleteL(const TComponentName& aName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("DeleteL"));

	iTupleStore->DeleteL(aName);
}

void CBlackBoardServer::DeleteL(TUint aId)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("DeleteL"));

	TTupleType aTupleTypeInto; TTupleName aNameInto; TBuf<KMaxTupleSubNameLength> aSubNameInto;
	iTupleStore->DeleteL(aId, aTupleTypeInto, aNameInto, aSubNameInto);
}


void CBlackBoardServer::NotifyExistingL(MBlackBoardObserver *aSession, 
		const TTupleName& aTupleName, TBBPriority aPriority)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("NotifyExistingL"));

	TInt err, err_ret=KErrNone;
	TBool more=iTupleStore->FirstL(ETupleDataOrRequest, aTupleName, _L(""));
	while (more) {
		CC_TRAP(err, aSession->NotifyL(iTupleStore->GetCurrentIdL(), aPriority));
		if (err!=KErrNone) err_ret=err;
		more=iTupleStore->NextL();
	}
	User::LeaveIfError(err_ret);
}

void CBlackBoardServer::NotifyDeleted(const TTupleName& aTupleName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("NotifyDeleted"));

	if (iSubscriptions==0) return; // constructing

#ifdef __WINS__
	TBuf<100> msg;
	msg=_L("Deleted tuple [");
	msg.AppendNum(aTupleName.iModule.iUid, EHex);
	msg.Append(_L(" "));
	msg.AppendNum(aTupleName.iId);
	msg.Append(_L("]"));
	RDebug::Print(msg);
#endif

	MBlackBoardObserver *o=0;
	TBBPriority pr;
	iSubDuplicate->Reset();
	auto_ptr< CList<MBlackBoardObserver*> > to_notify(CList<MBlackBoardObserver*>::NewL());
	for (o=iSubscriptions->FirstL(aTupleName, pr); o; o=iSubscriptions->NextL(pr)) {
		if (! iSubDuplicate->GetData( (uint32) o) ) {
			to_notify->AppendL(o);
			iSubDuplicate->AddDataL((uint32) o, (void*)1);
		}
	}

	TInt err; TInt err_ret=KErrNone;

	while(o=to_notify->Pop()) {
		CC_TRAP(err, o->NotifyDeletedL(aTupleName, aSubName));
		if (err!=KErrNone) err_ret=err;
	}
	//User::LeaveIfError(err_ret);
}

void CBlackBoardServer::NotifyExistingL(MBlackBoardObserver *aSession, 
		const TComponentName& aComponentName, TBBPriority aPriority)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("NotifyExistingL"));

	TInt err, err_ret=KErrNone;
	TBool more=iTupleStore->FirstL(ETupleDataOrRequest, aComponentName);
	TInt count=0;
	while (more) {
		CC_TRAP(err, aSession->NotifyL(iTupleStore->GetCurrentIdL(), aPriority));
		if (err!=KErrNone) 
			err_ret=err;
		CC_TRAP(err, more=iTupleStore->NextL());
		if (err!=KErrNone) 
			err_ret=err;
		count++;
	}
	more=iTupleStore->FirstL(ETuplePermanentSubscriptionEvent, aComponentName);
	while (more) {
		CC_TRAP(err, aSession->NotifyL(iTupleStore->GetCurrentIdL(), aPriority));
		if (err!=KErrNone) err_ret=err;
		more=iTupleStore->NextL();
	}
	more=iTupleStore->FirstL(ETupleReply, aComponentName);
	while (more) {
		CC_TRAP(err, aSession->NotifyL(iTupleStore->GetCurrentIdL(), aPriority));
		if (err!=KErrNone) err_ret=err;
		more=iTupleStore->NextL();
	}
	User::LeaveIfError(err_ret);
}

TInt CBlackBoardServer::AddNotificationL(MBlackBoardObserver *aSession, 
		const TTupleName& aTupleName,
		TBool aGetExisting, TBBPriority aPriority)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("AddNotificationL"));

	iSubscriptions->AddNotificationL(aSession, aTupleName, aPriority);
	TInt err=KErrNone;

	if (aGetExisting) 
		CC_TRAP(err, NotifyExistingL(aSession, aTupleName, aPriority));
	return err;
}

void CBlackBoardServer::DeleteNotificationL(MBlackBoardObserver *aSession, 
	const TTupleName& aTupleName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("DeleteNotificationL"));

	iSubscriptions->DeleteNotificationL(aSession, aTupleName);
}

TInt CBlackBoardServer::AddNotificationL(MBlackBoardObserver *aSession, 
	const TComponentName& aComponentName, 
	TBool aGetExisting, TBBPriority aPriority)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("AddNotificationL"));

	iSubscriptions->AddNotificationL(aSession, aComponentName, aPriority);
	TInt err=KErrNone;

	if (aGetExisting) 
		CC_TRAP(err, NotifyExistingL(aSession, aComponentName, aPriority));
	return err;
}

void CBlackBoardServer::DeleteNotificationL(MBlackBoardObserver *aSession, 
	const TComponentName& aComponentName)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("DeleteNotificationL"));

	iSubscriptions->DeleteNotificationL(aSession, aComponentName);
}

void CBlackBoardServer::DeleteAllNotificationsL(MBlackBoardObserver *aSession)
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("DeleteAllNotificationsL"));

	iSubscriptions->DeleteAllSubscriptionsL(aSession);
}

void CBlackBoardServer::DeleteAllNotificationsL()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("DeleteAllNotificationsL"));

	iSubscriptions->DeleteAllSubscriptionsL();
}

void CBlackBoardServer::TerminateServer()
{
	CALLSTACKITEM_N(_CL("CBlackBoardServer"), _CL("TerminateServer"));

	CActiveScheduler::Stop();
}

void LogL(const TDesC& msg)
{
	RFileLogger iLog;
	User::LeaveIfError(iLog.Connect());
	CleanupClosePushL(iLog);
	iLog.CreateLog(_L("Context"),_L("BlackBoard"), EFileLoggingModeAppend);
	TInt i=0;
#ifdef __WINS__
	while (i<msg.Length()) {
#else
	{
#endif
		RDebug::Print(msg.Mid(i));
		iLog.Write(msg.Mid(i));
		i+=128;
	}
	
	iLog.CloseLog();
	CleanupStack::PopAndDestroy();
}

void Log(const TDesC& msg)
{
	CC_TRAPD(err, LogL(msg));
	// ignore error, not much else we can do
	// and not critical
}
