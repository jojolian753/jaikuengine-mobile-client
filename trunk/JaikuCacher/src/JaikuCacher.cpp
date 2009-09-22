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

// Concepts:
// !Shared memory!

#include "JaikuCacher.h"
#include "JaikuCacherSession.h"
#include "CacherCommon.h"
#include <E32SVR.H>
#include <basched.h>

#include <flogger.h>
#include "app_context.h"
#include "break.h"
#include "server_startup.h"
#include "phonebook_static.h"
#include "raii_s32file.h"
#include <CPbkContactEngine.h>
#include <CPbkContactChangeNotifier.h>
#include <CPbkContactIter.h>
#include "symbian_auto_ptr.h"
#include "app_context_impl.h"
#include <s32mem.h>
#include "reporting.h"
#include "symbian_tree.h"

void Log(const TDesC& msg);

bool ClientAlive(TInt ThreadId);

#pragma warning(disable:4706)

class CCacheChunk : public CBase, public MContextBase {
public:
	static CCacheChunk* NewL(const TDesC& aName, TInt aSize) {
		auto_ptr<CCacheChunk> ret(new (ELeave) CCacheChunk);
		ret->ConstructL(aName, aSize);
		return ret.release();
	}
	RChunk	iChunk, iTemp;
	TPtr8 iPtr, iTempPtr;
	RDesWriteStream iWriteStream;
	~CCacheChunk() {
		Release();
	}
	void GrowL() {
		iVersion++;
		if (iTemp.Handle()) {
			iTemp.Close(); iTemp.SetHandle(0);
		}
		iTemp=iChunk;
		iTempPtr.Set(iPtr);
		iChunk.SetHandle(0);
		CreateChunkL(iTemp.Size()*2);
		iWriteStream.WriteL(iTempPtr);
		iTemp.Close(); iTemp.SetHandle(0);
	}
	const TDesC& Name() { return iName; }
	const TDesC8& Data() { return iPtr; }
private:
	TName iName, iGivenName;
	TInt iVersion;
	CCacheChunk(): iPtr(0, 0), iTempPtr(0, 0) { }
	void Release() {
		iWriteStream.Close();
		if (iChunk.Handle()) {
			iChunk.Close(); iChunk.SetHandle(0);
		}
	}
	void CreateChunkL(TInt aSize) {
		iName=iGivenName; iName.Append(_L(".")); iName.AppendNum(iVersion);
		if (aSize<4096) aSize=4096;
		TInt err=iChunk.CreateGlobal(iName, aSize, aSize);
		if (err) {
			User::Leave(err);
		}
		iPtr.Set(iChunk.Base(), 0, aSize);
		iWriteStream.Open(iPtr);
	}
	void ConstructL(const TDesC& aName, TInt aSize) {
		iGivenName=aName;
		CreateChunkL(aSize);
	}
};

CJaikuCacher::CJaikuCacher(TInt aPriority) : SERVER_CLASS(aPriority)
{

}

void CJaikuCacher::GetCurrentContactsNameAndGeneration(TDes& aHashNameInto, TDes& aListNameInto, TInt& aGenerationInto)
{
	if (!iContactListChunk || !iContactHashChunk) {
		ReadFromDbL(ETrue);
	}
	aHashNameInto=iContactHashChunk->Name();
	aListNameInto=iContactListChunk->Name();
	aGenerationInto=iContactsGeneration;
}

CJaikuCacher::~CJaikuCacher()
{
	delete iTimer;
	delete iContactIterator; iContactIterator=0;
	delete iContactChangeNotifier;
	delete iContactEngine;
	delete iContactListChunk; delete iNextContactListChunk;
	delete iContactHashChunk; delete iNextContactHashChunk;
	delete seen_contacts;
}

CJaikuCacher* CJaikuCacher::NewL()
{
	CJaikuCacher* JaikuCacherer = new (ELeave) CJaikuCacher(EPriorityNormal);
	CleanupStack::PushL(JaikuCacherer);
	JaikuCacherer->ConstructL();
	CleanupStack::Pop(JaikuCacherer);
	return JaikuCacherer;
}

//#include <eikenv.h>

void CJaikuCacher::ConstructL()
{
	iContactCount=100;
	iTimer=CTimeOut::NewL(*this, CActive::EPriorityIdle);
	ReadFromFileL();
	iTimer->WaitShort(100);
	seen_contacts=CGenericIntMap::NewL();
	StartL(KJaikuCacherName);
}

void CJaikuCacher::SetContactsDbName(const TDesC& aDb)
{
#ifndef __S60V3__
	iContactsDbName=aDb;
#else
	iContactsDbName=_L("c:DBS_100065FF_unittests.cdb");
#endif
	ReleaseEngine();
	ReadFromDbL(ETrue);
}

void CJaikuCacher::ReleaseEngine()
{
	delete iContactIterator; iContactIterator=0;
	delete iContactChangeNotifier; iContactChangeNotifier=0;
	delete iContactEngine; iContactEngine=0;
}

void CJaikuCacher::ReadFromFileL()
{
	iNameBuffer=DataDir();
	iNameBuffer.Append(_L("contacts_cache.dat"));
	TEntry e;
	TInt err=Fs().Entry(iNameBuffer, e);
	if (err==KErrNone) {
		RAFileReadStream f; f.OpenLA(Fs(), iNameBuffer, EFileRead);
		TInt valid; 
		valid=f.ReadInt32L();
		if (valid) {
			TInt hash_size=f.ReadInt32L();
			TInt list_size=f.ReadInt32L();
			if (hash_size>0 && list_size>0) {
				iContactCount=(list_size)/(100);
				if (iContactCount<50) iContactCount=50;
				
				iNameBuffer=_L("JaikuCacherContactsHash");
				iNameBuffer.AppendNum(1);
				iNextContactHashChunk=CCacheChunk::NewL(iNameBuffer, hash_size);
				iNameBuffer=_L("JaikuCacherContactsList");
				iNameBuffer.AppendNum(1);
				iNextContactListChunk=CCacheChunk::NewL(iNameBuffer, list_size);
				
				f.ReadL( iNextContactHashChunk->iChunk.Base(), hash_size );
				f.ReadL( iNextContactListChunk->iChunk.Base(), list_size );
				
				SwapContactsChunks();
				iContactsGeneration=1;
			}
		}
	}
}

#include "raii_f32file.h"

void CJaikuCacher::SwapContactsChunks()
{
	delete iContactHashChunk; iContactHashChunk=0;
	delete iContactListChunk; iContactListChunk=0;
	iNextContactHashChunk->iWriteStream.CommitL();
	iNextContactListChunk->iWriteStream.CommitL();
	
	iContactHashChunk=iNextContactHashChunk; iNextContactHashChunk=0;
	iContactListChunk=iNextContactListChunk; iNextContactListChunk=0;
}
	
void CJaikuCacher::WriteToFileL()
{
	iNameBuffer=DataDir();
	iNameBuffer.Append(_L("contacts_cache.dat"));
	RAFile f; f.ReplaceLA(Fs(), iNameBuffer, EFileWrite);
	User::LeaveIfError(f.SetSize(0));
	User::LeaveIfError(f.Write( TPckgBuf<TInt>(0) ));
	User::LeaveIfError(f.Write( TPckgBuf<TInt>( iContactHashChunk->Data().Length() ) ));
	User::LeaveIfError(f.Write( TPckgBuf<TInt>( iContactListChunk->Data().Length() ) ));
	User::LeaveIfError(f.Write( iContactHashChunk->Data() ));
	User::LeaveIfError(f.Write( iContactListChunk->Data() ));
	User::LeaveIfError(f.Flush());
	TInt pos=0;
	User::LeaveIfError(f.Seek(ESeekStart, pos));
	User::LeaveIfError(f.Write( TPckgBuf<TInt>(1) ));
	User::LeaveIfError(f.Flush());
}

template< typename T, typename T2 >
void WriteToCacheChunkL(CCacheChunk* aCacheChunk, void (RWriteStream::*f)( T ), T2 aData) {
	TInt err=KErrNone;
	do {
		RWriteStream& c=aCacheChunk->iWriteStream;
		c.CommitL();
		TRAP(err, (c.*f)(aData));
		if (err==KErrOverflow) aCacheChunk->GrowL();
	} while(err==KErrOverflow);
	User::LeaveIfError(err);
}

// Concepts:
// !Accessing the phonebook (contacts database)!
void CJaikuCacher::ReadFromDbL(TBool aForceSync)
{
	iContactsChangePending=EFalse;
	seen_contacts->Reset();
	
	if (iContactsState==EContactsAsyncReading) {
		iContactsGeneration++;
	}
	iNameBuffer=_L("JaikuCacherContactsHash");
	iNameBuffer.AppendNum(iContactsGeneration+1);
	iNextContactHashChunk=CCacheChunk::NewL(iNameBuffer, (iContactCount+2)*20);
	iNameBuffer=_L("JaikuCacherContactsList");
	iNameBuffer.AppendNum(iContactsGeneration+1);
	iNextContactListChunk=CCacheChunk::NewL(iNameBuffer, (iContactCount+2)*(3*100+4));
	
	delete iContactIterator; iContactIterator=0;
	if (!iContactEngine) {
		if (iContactsDbName.Length()>0) {
			iContactEngine=CPbkContactEngine::NewL(iContactsDbName, EFalse, &Fs());
		} else {
			iContactEngine=CPbkContactEngine::NewL(&Fs());
		}
		iContactChangeNotifier=iContactEngine->CreateContactChangeNotifierL(this);
	}
	iContactIterator=iContactEngine->CreateContactIteratorLC(ETrue);
	CleanupStack::Pop();
	iCurrentContact=iContactIterator->FirstL();
	WriteToCacheChunkL(iNextContactListChunk, &RWriteStream::WriteInt32L, iContactEngine->NameDisplayOrderL());
	iContactCount=0;
	if (iContactsGeneration==0 || aForceSync) {
		while(! ReadNextFromDbL()) 
			;
		SwapContactsChunks();
		WriteToFileL();
		iContactsGeneration++;
	} else {
		iContactsState=EContactsAsyncReading;
		iTimer->Wait(0);
	}
}

TBool CJaikuCacher::ReadNextFromDbL()
{
	if (iCurrentContact==KNullContactId) {
		WriteToCacheChunkL(iNextContactListChunk, &RWriteStream::WriteInt32L, 0);
		WriteToCacheChunkL(iNextContactHashChunk, &RWriteStream::WriteInt32L, 0);
		return ETrue;
	}
	if (seen_contacts->GetData(iCurrentContact)) {
		User::Leave(KErrAlreadyExists);
	}
	seen_contacts->AddDataL( iCurrentContact, (void*)1 );
	iContactCount++;
	
	CPbkContactItem *item = 0;
	item = iContactIterator->CurrentL();
	TInt err;
	void (RWriteStream::*write_desc)(const TDesC&)=&RWriteStream::WriteL;
	void (RWriteStream::*write_desc8)(const TDesC8&)=&RWriteStream::WriteL;
	const TInt* fieldIds=PhoneNumberFields();
	
	if (item) {			
		for ( TInt i=0; fieldIds[i] != EPbkFieldIdNone; i++ ) {
			if (GetPhoneNumberHash(item, fieldIds[i], iPhoneHash)) {
				WriteToCacheChunkL(iNextContactHashChunk, &RWriteStream::WriteInt32L, iCurrentContact);
				WriteToCacheChunkL(iNextContactHashChunk, write_desc8, iPhoneHash);
			}
		}
		
		WriteToCacheChunkL(iNextContactListChunk, &RWriteStream::WriteInt32L, iCurrentContact);
		GetNamesForContactL(item, iFirst, iLast, iExtra);
		TBuf<100>* to_write[]={ &iFirst, &iLast, &iExtra, 0 }; // const TDesC* gives wrong results
		TInt i;
		for (i=0; to_write[i]; i++) {
			TInt len=to_write[i]->Length();
			WriteToCacheChunkL(iNextContactListChunk, &RWriteStream::WriteInt32L, len);
			if (len>0) {
				WriteToCacheChunkL(iNextContactListChunk, write_desc, *(to_write[i]));
			}
		}
	}
	iCurrentContact=iContactIterator->NextL();
	if (iCurrentContact==KNullContactId) {
		WriteToCacheChunkL(iNextContactListChunk, &RWriteStream::WriteInt32L, 0);
		WriteToCacheChunkL(iNextContactHashChunk, &RWriteStream::WriteInt32L, 0);
		return ETrue;
	}
	return EFalse;
}

void CJaikuCacher::HandleDatabaseEventL(TContactDbObserverEvent aEvent)
{
	if (iContactsState==EContactsIdle) {
		ReadFromDbL(EFalse);
	} else {
		iContactsChangePending=ETrue;
	}
}

void CJaikuCacher::expired(CBase*)
{
	if (iContactsState==EContactsIdle) {
		ReadFromDbL(EFalse);
	} else {
		TBool done_reading=EFalse;
		TRAPD(err, done_reading=ReadNextFromDbL());
		if (err==KErrAlreadyExists) {
			delete iNextContactListChunk; iNextContactListChunk=0;
			delete iNextContactHashChunk; iNextContactHashChunk=0;
			ReleaseEngine();
			iContactsState=EContactsIdle;
			iTimer->Wait(1);
			return;
		} else if (err!=KErrNone) {
			User::Leave(err);
		}
		
		if (done_reading) {
			SwapContactsChunks();
			iContactsState=EContactsIdle;
			iContactsGeneration++;
			NotifySessions(EContactsChanged);
			if (iContactsChangePending) {
				iTimer->Wait(0);
			}
			WriteToFileL();
		} else {
			iTimer->Wait(0);
		}
	}
}

#ifndef __IPCV2__
SESSION_CLASS* CJaikuCacher::NewSessionL(const TVersion& aVersion) const
#else
SESSION_CLASS* CJaikuCacher::NewSessionL(const TVersion& aVersion, const MESSAGE_CLASS &aMessage) const
#endif
{
	// check version
	if (!User::QueryVersionSupported(TVersion(KJaikuCacherMajorVersionNumber,
		KJaikuCacherMinorVersionNumber,
		KJaikuCacherBuildVersionNumber),
		aVersion))
	{
		User::Leave(KErrNotSupported);
	}

	// create new session
#ifndef __IPCV2__
	RThread client = Message().Client();
	return CJaikuCacherSession::NewL(client, *const_cast<CJaikuCacher*> (this));
#else
	return CJaikuCacherSession::NewL(*const_cast<CJaikuCacher*> (this));
#endif
}


void CJaikuCacher::IncrementSessions()
{
	iSessionCount++;
}

void CJaikuCacher::DecrementSessions()
{
	iSessionCount--;
	if (iSessionCount <= 0)
	{
		iSessionCount =0;
		CActiveScheduler::Stop();
	}    
}

TInt CJaikuCacher::CheckedRunError(TInt aError)
{
	if (aError == KErrBadDescriptor)
	{
		// A bad descriptor error implies a badly programmed client, so panic it;
		// otherwise report the error to the client
		PanicClient(Message(), EBadDescriptor);
	}
	else
	{
		Message().Complete(aError);
	}

	//
	// The leave will result in an early return from CServer::RunL(), skipping
	// the call to request another message. So do that now in order to keep the
	// server running.
	ReStart();

	return KErrNone;	// handled the error fully
}

void CJaikuCacher::PanicClient(const MESSAGE_CLASS& aMessage, TJaikuCacherPanic aPanic)
{
	aMessage.Panic(KJaikuCacher, aPanic);
}

void CJaikuCacher::PanicServer(TJaikuCacherPanic aPanic)
{
	User::Panic(KJaikuCacher, aPanic);
}

void CJaikuCacher::RunL()
{
	CC_TRAPD(err, SERVER_CLASS::RunL());
	if (err!=KErrNone) {
		TBuf<20> msg;
		msg.Format(_L("Error in RunL: %d"), err);
		Reporting().DebugLog(msg);
		CheckedRunError(err);
	}
}


_LIT(KName, "JaikuCacher");
void CJaikuCacher::ThreadFunctionL()
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

	{ RThread me; me.RenameMe(KName); }
	
#if defined (__WINS__) && !defined(EKA2)
	UserSvr::ServerStarted();
#endif

	{
		auto_ptr<CApp_context> c(CApp_context::NewL(false, _L("JaikuCacher")));
#ifndef __S60V3__
#  ifndef __WINS__
		c->SetDataDir(_L("c:\\system\\data\\context\\"), false);
#  endif
#endif
		
		auto_ptr<CActiveScheduler> activeScheduler(new (ELeave) CBaActiveScheduler);
		CActiveScheduler::Install(activeScheduler.get());

		// Construct our server
		auto_ptr<CJaikuCacher> notif(CJaikuCacher::NewL());

		RSemaphore semaphore;
		TBuf<50> semaphorename; MakeServerSemaphoreName(semaphorename, KJaikuCacherName);
		if (semaphore.CreateGlobal(semaphorename, 0)!=KErrNone) {
			User::LeaveIfError(semaphore.OpenGlobal(semaphorename));
		}

		// Semaphore opened ok
		semaphore.Signal();
		semaphore.Close();

		// Start handling requests
		CActiveScheduler::Start();
	}

	process_semaphore.Signal(); process_semaphore.Close();
}

#if defined(__WINS__)
EXPORT_C TInt JaikuCacherThreadFunction(TAny* aParam)
{
	return CJaikuCacher::ThreadFunction(aParam);
}
#endif

TInt CJaikuCacher::ThreadFunction(TAny* /*aNone*/)
{
	__UHEAP_MARK;
	
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	TInt err;
	/*
	CEikonEnv* eik=new (ELeave) CEikonEnv;
	if (eik == NULL)
	{
		PanicServer(ECreateTrapCleanup);
	}
	CC_TRAP(err, eik->ConstructL(EFalse));
	*/

	CC_TRAP2(err, ThreadFunctionL(), ::GetContext());
	if (err != KErrNone)
	{
		PanicServer(ESrvCreateServer);
	}

	/*
	eik->DestroyEnvironment();
	*/
	delete cleanupStack;
	
	return KErrNone;
	
	__UHEAP_MARKEND;

}


//---------------------------------------------------------------------------

void CJaikuCacher::TerminateJaikuCacher()
{
	NotifySessions(ETerminated);
	//Log();
	CActiveScheduler::Stop();
}

void CJaikuCacher::NotifySessions(TEvent aEvent)
{
	CJaikuCacherSession* session=0;

	iSessionIter.SetToFirst();
	while( (session = reinterpret_cast<CJaikuCacherSession*>(iSessionIter++)) ) {
		session->NotifyEvent(aEvent);
	}
}

void CJaikuCacher::ReportError(TJaikuCacherRqstComplete aErrorType, TDesC & aErrorCode, TDesC & aErrorValue)
{
	CJaikuCacherSession* session=0;

	iSessionIter.SetToFirst();
	while( (session = reinterpret_cast<CJaikuCacherSession*>(iSessionIter++)) ) {
		session->ReportError(aErrorType, aErrorCode, aErrorValue);
	}
}

void CJaikuCacher::CancelRequest(const MESSAGE_CLASS &aMessage)
{
	// there's nothing to cancel (at least yet)
	switch(aMessage.Function()) {
	default:
		break;
	}
}
