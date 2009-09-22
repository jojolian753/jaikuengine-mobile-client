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

#include "break.h"
#include "app_context.h"
#include "app_context_impl.h"
#include "app_context_fileutil.h"
#include "settings.h"

#include <f32file.h>
#include <charconv.h>
#include <etel.h>
#ifndef __S60V3__
#include "stripped_etelbgsm.h"
#include <saclient.h>
#endif
#include <w32std.h>
#include "list.h"
#include <stdlib.h> // for CloseSTDLIB()

#ifndef __S80__
#include <aknglobalnote.h>
#endif

#include <coemain.h>
#include <flogger.h>
#include <bautils.h>
#include "context_uids.h"
#ifdef __WINS__
#include <apgcli.h>
#include <apgtask.h>
#endif
#include "raii.h"
#include "symbian_auto_ptr.h"
#include "raii_apgcli.h"
#include "raii_f32file.h"
#include "raii_e32std.h"
#include "reporting.h"
#include "pausable.h"
#include "errorinfo.h"
#include "callstack.h"
#include <avkon.rsg>
#include <e32math.h>
#include "datacounter.h"
#include "cc_recovery.h"

#include "testsupport_impl.h"

#ifndef __S60V1__
#include <Etel3rdParty.h>
#endif

//#define DEBUG_CALLSTACK 1

class TDummyRecovery : public MRecovery {
public:
	virtual void RegisterComponent( TUid , TInt , TInt , 
		const TDesC& ) { }
	virtual void SetState(TUid , TInt , TState ) { }
	virtual TInt GetErrorCount(TUid , TInt ) { return -1; }
	virtual void ResetErrorCount(TUid , TInt ) { }
	virtual void ReportError(TUid , TInt ,
		const class MErrorInfo* ) { }
	virtual TState GetState(TUid , TInt ) {
		return EUnknown;
	}
};


class CApp_contextImpl : public CApp_context, public MDiskSpace, public MReporting,
	public MErrorInfoManager, public MCallStack, public MDataCounter, public TDummyRecovery {
public:
	virtual ~CApp_contextImpl();
	virtual RFs&	Fs();
#if !defined(__S60V3__)
	virtual RSystemAgent&	SysAgent();
	virtual RTelServer&	TelServer();
	virtual RBasicGsmPhone&	Phone();
#endif
#if defined(__S60V3__) || defined(__S60V2FP3__)
	virtual CTelephony& Telephony();
#endif

	virtual CCnvCharacterSetConverter* CC();
	virtual bool NoSpaceLeft();
	void CheckSpaceAndNotifiers();
	virtual void SetFileLog(MPausable* log);
	virtual const TDesC& DataDir();
	virtual const TDesC& AppDir();
	virtual void SetDataDir(const TDesC& Dir, bool UseMMC);
	virtual void SetAppDir(const TDesC& Dir);
	virtual MSettings& Settings(); 
	virtual void SetActiveErrorReporter(MActiveErrorReporter* Reporter);
	virtual MActiveErrorReporter* GetActiveErrorReporter();
	virtual void SetSettings(MSettings* Settings);  // takes ownership
	virtual RWsSession& Ws();
	virtual void ReportActiveError(const TDesC& Source,
		const TDesC& Reason, TInt Code);
	virtual const char* CallStack();

	TInt PushCallStack(const TDesC8& Class, const TDesC8& Func, char *&aCallStackZero, char *&aCallStackCurrent);
	TPtr8 PopCallStack(char	*&aCallStackZero, char *&aCallStackCurrent);
	void ResetCallStack(char *&aCallStackZero, char *&aCallStackCurrent);
	virtual TInt PushCallStack(const TDesC8& Name);
	virtual TInt PushCallStack(const TDesC8& Class, const TDesC8& Func);
	virtual TPtr8 PopCallStack();
	virtual void ResetCallStack();
	virtual TInt PushAllocCallStack(const TDesC8& Name);
	virtual TInt PushAllocCallStack(const TDesC8& Class, const TDesC8& Func);
	virtual TPtr8 PopAllocCallStack();
	virtual void ResetAllocCallStack();
	virtual TInt CallStackDepth() const;

	virtual HBufC* GetFormattedCallStack(const TDesC& Prefix);
	virtual void IterateStack(MCallStackVisitor& aVisitor, const char* aStack=0);
	virtual void ShowGlobalNote(TInt notetype, const TDesC& msg);
	virtual TPtrC8 GetCurrentClass();

	virtual MErrorInfo* GetLastErrorInfo();
	virtual MErrorInfo* GetLastErrorInfo(TErrorCode aCode);
	virtual void ResetLastErrorInfo();
	virtual void SetBBSession(CBBSession* BBSession);
	virtual CBBSession* BBSession();
	virtual void SetBBDataFactory(MBBDataFactory* aFactory);
	virtual MBBDataFactory* BBDataFactory();
	virtual MReporting& Reporting();
	virtual MErrorInfoManager& ErrorInfoMgr();
	virtual MCallStack& CallStackMgr();
	virtual void SetDebugCallstack(TBool aDoDebug) {
		iDebugCallStack=aDoDebug;
	}
	virtual MTestSupport& TestSupport() {
		return iTestSupport;
	}

	virtual void SetDebugLog(const TDesC& aDir, const TDesC& aFile);
	TBuf<128> iDebugBuffer;
	virtual void DebugLog(const TDesC8& aMessage);
	virtual void DebugLog(const TDesC& aMessage);
	virtual void DebugLog(const TDesC& aMessage, TInt aCode);
        virtual void UserErrorLog(const TDesC& aMessage);
        virtual void UserErrorLog(const TDesC& aMessage, TInt aCode);
        virtual void UserErrorLog(const TDesC& aMessage, const TDesC& aMessagePart);
        virtual void UserErrorLog(const TDesC& aMessage,
                const TDesC& aMessagePart, const TDesC& aMessagePart2);
	void DebugLogL(const TDesC& aMessage);

	//FIXME
	void SetHBufC(HBufC*& aBuf, const TStringArg& aString, TInt& aCount, TInt& aPos) {
		TInt len=aString.Length();
		if (len==0) aCount=0;
		else aCount=1;

		aPos=0;
		if (!aBuf) {
			aBuf=HBufC::New(aString.Length());
		} else {
			aBuf->Des().Zero();
			if (aBuf->Des().MaxLength() < len) {
				HBufC* tmp=HBufC::New(len);
				if (tmp) {
					delete aBuf;
					aBuf=tmp;
				}
			}
		}
		if (aBuf) {
			TPtr b=aBuf->Des();
			aString.AppendToString(b);
		}
	}
	void AppendHBufC(HBufC*& aBuf, const TStringArg& aString, TInt& aCount, TInt& aPos) {
		TInt len=aString.Length();
		if (!aBuf) {
			aBuf=HBufC::New(len);
		} else {
			if (aBuf->Des().MaxLength()-aBuf->Des().Length() < len) {
				CC_TRAPD(err, aBuf=aBuf->ReAllocL(aBuf->Des().Length()+len));
			}
		}
		if (aBuf) {
			TInt formatpos=KErrNotFound;
			TBuf<10> format=_L("%");
			format.AppendNum(aCount++);
			if (aPos>=0) {
				formatpos=aBuf->Des().Mid(aPos).Find(format);
			}
			if ( formatpos==KErrNotFound ) {
				aPos=0;
				TPtr b=aBuf->Des();
				aString.AppendToString(b);
			} else {
				formatpos+=aPos;
				aPos=formatpos+format.Length();
				HBufC* rest=0;
				CC_TRAPD(err, rest=(*aBuf).Mid(aPos).AllocL());
				if (rest) {
					auto_ptr<HBufC> resta(rest);
					aBuf->Des().SetLength(formatpos);
					TPtr b=aBuf->Des();
					aString.AppendToString(b);
					aPos=(*aBuf).Length();
					aBuf->Des().Append(*rest);
				} else {
					// skip insert if out of memory
					format.Fill('.');
				}
			}
		} else {
			++aCount;
		}
	}
	HBufC	*iUserMsg, *iTechMsg;
	HBufC	*iReportingMsg;
	TErrorCode	iErrorCode;
	TErrorType	iErrorType;
	TSeverity	iSeverity;
	MErrorInfo	*iLastErrorInfo;
	TInt		iUserMsgCount, iTechMsgCount;
	TInt		iUserMsgPos, iTechMsgPos;
	TBuf<128>	iIndent;
	virtual void StartErrorWithInfo(TErrorCode aErrorCode,
		const TStringArg& aUserMessage, const TStringArg& aTechMessage,
		TErrorType aErrorType, TSeverity aSeverity) { 

		iErrorCode=aErrorCode;
		iErrorType=aErrorType;
		iSeverity=aSeverity;
		SetHBufC(iUserMsg, aUserMessage, iUserMsgCount, iUserMsgPos);
		SetHBufC(iTechMsg, aTechMessage, iTechMsgCount, iTechMsgPos);
	}
	virtual MErrorInfoManager& UserMsg(const TStringArg& aArg) { 
		AppendHBufC(iUserMsg, aArg, iUserMsgCount, iUserMsgPos); 
		return *this; }
	virtual MErrorInfoManager& TechMsg(const TStringArg& aArg) { 
		AppendHBufC(iTechMsg, aArg, iTechMsgCount, iTechMsgPos); 
		return *this; 
	}
	virtual MErrorInfoManager& ErrorCode(TErrorCode aErrorCode) { 
		iErrorCode=aErrorCode;
		return *this; 
	}
	virtual MErrorInfoManager& Severity(TSeverity aSeverity) { 
		iSeverity=aSeverity;
		return *this; 
	}
	virtual MErrorInfoManager& ErrorType(TErrorType aErrorType) { 
		iErrorType=aErrorType;
		return *this; 
	}
	virtual void Raise() {
		Get();
		User::Leave(iErrorCode.iCode);
	}
	virtual void Log() {
	}
	virtual MErrorInfo* Get() {
		HBufC* callstack=GetFormattedCallStack(KNullDesC);
		MErrorInfo* tmp=0;
		CC_TRAPD(err, tmp=CErrorInfo::NewL(iErrorCode, iUserMsg, iTechMsg, callstack,
			iErrorType, iSeverity, iLastErrorInfo));
		if (tmp) {
			if (iLastErrorInfo) iLastErrorInfo->Release();
			iLastErrorInfo=tmp;
			iUserMsg=0;
			iTechMsg=0;
		} else {
			delete callstack;
		}
		return iLastErrorInfo;
	}

	CList<MBusyIndicator*>	*iBusyIndicators; TBool iIsBusy;
	virtual TBool AddBusyIndicator(MBusyIndicator *aIndicator) 
	{
		iBusyIndicators->AppendL(aIndicator);
		return iIsBusy;
	}
	virtual void RemoveBusyIndicator(MBusyIndicator *aIndicator) 
	{
		CList<MBusyIndicator*>::Node *n=iBusyIndicators->iFirst;
		while(n) {
			if (n->Item==aIndicator) {
				iBusyIndicators->DeleteNode(n, ETrue);
				return;
			}
			n=n->Next;
		}
	}
	virtual TBool ShowBusy(TBool aIsBusy)
	{
		TBool tmp=iIsBusy;
		CList<MBusyIndicator*>::Node* n=iBusyIndicators->iFirst;
		while(n) {
			n->Item->ShowBusy(aIsBusy);
			n=n->Next;
		}
		iIsBusy=aIsBusy;
		return tmp;
	}
	TBuf<128> iState;
	virtual void SetState(const TDesC& aState) {
		if (aState.Length()<=128) {
			iState=aState;
		} else {
			iState=aState.Left(128);
		}
	}
	virtual void TakeOwnershipL(CBase* aObject) {
		TInt i = iOwnedObjects.Count()-1;
		iOwnedObjects[i]=aObject;
		User::LeaveIfError(iOwnedObjects.Append(0));
	}
	void(*iRunOnShutdown)(void);
	virtual void RunOnShutdown( void(*function)(void) ) {
		iRunOnShutdown=function;
	}
	virtual void SetDataCounter(MDataCounter* aCounter) {
		iDataCounter=aCounter;
	}
	virtual MDataCounter& DataCounter() {
		if (iDataCounter) return *iDataCounter;
		return *this;
	}
	virtual void GetInitialCountL(const struct TTupleName&,
					 const TDesC&,
					 TInt& aValue) { aValue=0; }

	virtual void SetNewCountL(const struct TTupleName&,
					 const TDesC&,
					 TInt) { }
	MRecovery* iRecovery;
	virtual void SetRecovery(MRecovery* aRecovery) {
		iRecovery=aRecovery;
		if (iRecovery && iInitialComponentUid.iUid!=0) {
			TInt errors=0;
			iRecovery->SetState(iInitialComponentUid, iInitialComponentId, MRecovery::EFailed);
			errors=iRecovery->GetErrorCount(iInitialComponentUid, iInitialComponentId);
			if (errors>2) {
				iRecovery->SetState(iInitialComponentUid, iInitialComponentId, MRecovery::EDisabled);
			}
		}
	}
	virtual MRecovery& Recovery() {
		if (iRecovery) return *iRecovery;
		else return *this;
	}


	MJuikLayout* iLayout;

	virtual void SetLayout(MJuikLayout* aLayout) 
	{
		ASSERT( ! iLayout );
		iLayout = aLayout;
	}

	virtual MJuikLayout& Layout() const
	{
		ASSERT( iLayout );
		return *iLayout;
	}


	MJuikIconManager* iIconManager;

	virtual void SetIconManager(MJuikIconManager* aIconManager) 
	{
		ASSERT( ! iIconManager );
		iIconManager = aIconManager;
	}

	virtual MJuikIconManager& IconManager() const
	{
		ASSERT( iIconManager );
		return *iIconManager;
	}


	virtual void SetCurrentComponent(TUid aComponentUid, TInt aComponentId);
	virtual void GetCurrentComponent(TUid& aComponentUid, TInt& aComponentId);
	
	TUid iInitialComponentUid; TInt iInitialComponentId;
	virtual void GetInitialComponent(TUid& aComponentUid, TInt& aComponentId) {
		aComponentUid=iInitialComponentUid;
		aComponentId=iInitialComponentId;
	}
	virtual TBool IsExiting(const char* aStack) {
		if (!aStack) aStack=iCallStackZero;
		else aStack+=4;
		if (!aStack) return EFalse;
		return *(TInt*)(aStack+iCallStackSize-20);
	}
	virtual void SetIsExiting(TBool aValue) {
		if (!iCallStackZero) return;
		*(TInt*)(iCallStackZero+iCallStackSize-20)=aValue;
	}

private:
	CApp_contextImpl();
	void ConstructL(bool aFsOnly, const TDesC& Name);
	virtual void DiskSpaceThreshold(TInt aDrive);

	RFs	iFs;
#ifndef __S60V3__
	RSystemAgent	iSysAgent; bool hasSysAgent;
#endif
#if !defined(__S60V3__)
	RTelServer	iTelServer; bool hasTelServer;
#  ifndef __WINS__
	RBasicGsmPhone	iPhone; bool hasPhone;
#  endif
#endif
#if defined(__S60V3__) || defined(__S60V2FP3__)
	CTelephony*	iTelephony;
#endif
	CCnvCharacterSetConverter* cc; 
	TInt64		iDiskSpaceLimit;
	TInt		iDrive[2];
	bool		iSpaceLeft;
#ifndef __S80__
	CAknGlobalNote *globalNote;
#endif
	MPausable	*iFileLog;
	MSettings	*iSettings;
	MActiveErrorReporter	*iReporter;
	TFileName	iDataDir;
	TFileName	iAppDir;
	bool		iFsOnly;
	bool		iUseMMC;

	RChunk		iCallStackChunk; int iCallStackSize; bool has_chunk;
	char		*iCallStackZero, *iCallStackCurrent;
#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	char		*iAllocCallStackZero, *iAllocCallStackCurrent;
#endif

	CDiskSpaceNotifier *iDiskSpaceNotifier[2];

	CBBSession	*iBBSession;

	TBuf<50>	iDebugDir, iDebugFile;

	RFileLogger iDebugLog; TBool iDebugLogIsOpen;
	TBool		iDebugCallStack;

	MBBDataFactory*	iBBDataFactory;
	class MDataCounter* iDataCounter;

	RPointerArray<CBase>	iOwnedObjects;
	friend class CApp_context;
	TBuf<50>	iChunkName;
	virtual const TDesC& Name() { return iChunkName; }


#ifdef __WINS__
	TPtrC8		iFullCallStack;
	virtual const TDesC8& FullCallStackBuffer() const {
		return iFullCallStack;
	}
#endif
	TTestSupport	iTestSupport;
};

// Concepts:
// !Low disk space notification!
class CDiskSpaceNotifier: public CCheckedActive
{
public:
	static CDiskSpaceNotifier* NewL(RFs& fs, TInt aDrive, MDiskSpace* aNotifier, TInt64 aThreshold);
	~CDiskSpaceNotifier();
private:
	CDiskSpaceNotifier(RFs& fs, TInt aDrive, MDiskSpace* aNotifier, TInt64 aThreshold);
	void ConstructL();
	void CheckedRunL();
	void DoCancel();

	MDiskSpace* iNotifier;
	RFs& iFs;
	TInt iDrive;
	TInt64 iThreshold;
#ifdef __WINS__
	TBool	iExists;
#endif
};

EXPORT_C TTime GetTime() { 
	TTime t; t.UniversalTime();
#if 0
	// switched to using UTC - MR 20061101
	t+=TTimeIntervalHours(3);
#endif
	return t; 
}

EXPORT_C TTime ContextToLocal(TTime aTime) {
	TLocale locale;
	TTimeIntervalSeconds universalTimeOffset(locale.UniversalTimeOffset());
	TTime ret=aTime;
#if 0
	// switched to using UTC - MR 20061101
	ret-=TTimeIntervalHours(3);
#endif
	ret+=universalTimeOffset;
	return ret;
}


EXPORT_C void print_stacktrace(RFs& fs, MApp_context* context, const TDesC& cat, TInt error)
{
	if (!context) return;
	RFile f;
	if (f.Open(fs, _L("C:\\crash.txt"), 
		EFileShareAny | EFileStreamText | EFileWrite)!=KErrNone) {
		if(f.Create(fs, _L("C:\\crash.txt"), 
			EFileShareAny | EFileStreamText | EFileWrite)!=KErrNone) {
			return;
		}
	}
	CleanupClosePushL(f);

	TInt pos=0;
	f.Seek(ESeekEnd, pos);

	TBuf<200> msg;
	msg.Format(_L("context_log crashed with error %d\ncategory "), error);
	TPtrC8 msg2( (TText8*)msg.Ptr(), msg.Length()*2);
	f.Write(msg2);
	TPtrC8 msg3( (TText8*)cat.Ptr(), cat.Length()*2);
	f.Write(msg3);
	f.Write(_L8("\ntrace:\n"));

	TPtr8 n=context->CallStackMgr().PopCallStack();
	while (n.Ptr()) {
		msg.Zero(); msg.Copy(n);
		f.Write(n);
		n=context->CallStackMgr().PopCallStack();
	}
	CleanupStack::PopAndDestroy();
}

EXPORT_C void exceptionhandler(TExcType t)
{
	MApp_context* c;
	c=MApp_context::Static();
	if (c) {
		print_stacktrace(c->Fs(), c, _L("Exception"), t);
	}
	User::Panic(_L("EXCEPTION"), t);
}

MApp_context::~MApp_context()
{
}

CApp_contextImpl::CApp_contextImpl()
#ifdef __WINS
	: iFullCallStack(0, 0)
#endif
{
}

void CApp_contextImpl::ConstructL(bool aFsOnly, const TDesC& Name)
{

	iIndent.SetLength(iIndent.MaxLength());
	iIndent.Fill( TChar(' ') );
	iIndent.SetLength(0);

	RFileLogger iLog;
	TInt log_err=iLog.Connect();
	if (log_err==KErrNone) {
		CleanupClosePushL(iLog);
		iLog.CreateLog(_L("Context"),_L("CApp_contextImpl.txt"),EFileLoggingModeAppend);
		iLog.Write(_L("constructing"));
	}

	iFsOnly=aFsOnly;

	iDiskSpaceLimit=500*1024; // 500k
	iDrive[0]=2; // C:
	iFileLog=0;

	User::LeaveIfError(iFs.Connect());
	{
		
		TInt drive; TBuf<2> driveletter;
		//TBuf<30> privatep;
#ifndef __WINS__
		RProcess me;
		driveletter=me.FileName().Left(2);
#  ifndef EKA2
		if (driveletter[0]=='z' || driveletter[0]=='Z') {
			auto_ptr<HBufC> cmd(HBufC::NewL(me.CommandLineLength()));
			TPtr cmdp=cmd->Des();
			me.CommandLine(cmdp);
			driveletter=cmd->Left(2);
		}
#  endif
		if (driveletter[0]=='z' || driveletter[0]=='Z') {
			driveletter[0]='c';
		}
#else
		driveletter=_L("c:");
		//driveletter=_L("e:");
#endif
		User::LeaveIfError(RFs::CharToDrive(driveletter[0], drive));
		//User::LeaveIfError(iFs.CreatePrivatePath(drive));
		//User::LeaveIfError(iFs.PrivatePath(privatep));
		iDataDir=driveletter;
#ifndef __S60V3__
		iDataDir.Append(_L("\\system"));
#else
		iUseMMC=ETrue;
#endif
		iDataDir.Append(_L("\\data\\Context\\"));
		BaflUtils::EnsurePathExistsL(Fs(), iDataDir);
		
#ifdef FUNCTION_LEVEL_STATISTICS
		BaflUtils::EnsurePathExistsL(Fs(), _L("c:\\Logs\\Context\\"));
#endif
		iAppDir=iDataDir;
		//iDataDir.Append(privatep);
	}
	iSpaceLeft=true;
	iBusyIndicators=CList<MBusyIndicator*>::NewL();

	iChunkName=Name.Left(iChunkName.MaxLength());
	TInt exists;
	if (Name.Length() && ( (exists=iCallStackChunk.OpenGlobal(Name, EFalse))==KErrNone || 
		iCallStackChunk.CreateGlobal(Name, 4096, 4096)==KErrNone)) {
		has_chunk=true;
		iCallStackZero=iCallStackCurrent=(char*)iCallStackChunk.Base() + 4;
		iCallStackSize=iCallStackChunk.Size();
		*(TInt*)(iCallStackZero+iCallStackSize-20)=0;
		TUint* uidp=(TUint*)(iCallStackZero+iCallStackSize-16);
		TInt* idp=(TInt*)(iCallStackZero+iCallStackSize-12);
		if (exists==KErrNone) {
			iInitialComponentUid=TUid::Uid(*uidp);
			iInitialComponentId=*idp;
		}
		*uidp=0;
		*idp=0;
	} else {
		has_chunk=false;
		iCallStackZero=new char[4096];
		if (iCallStackZero) iCallStackZero += 4;
		iCallStackCurrent=iCallStackZero;
		iCallStackSize=4096;
	}

#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	iAllocCallStackZero=(char*)User::Alloc(4096);
	if (iAllocCallStackZero) iAllocCallStackZero += 4;
	iAllocCallStackCurrent=iAllocCallStackZero;
#  if defined(__WINS__)
	if (iAllocCallStackZero)
		iFullCallStack.Set( (const TUint8*)iAllocCallStackZero-4, 4096);
#  endif
#endif
	if (!aFsOnly) {
#ifndef __S80__
		globalNote=CAknGlobalNote::NewL();
#endif
	}
	

	User::LeaveIfError(iOwnedObjects.Append(0));
	// only the first app context in a thread gets put
	// in the Tls, so that we can use an instance to look
	// at another thread's call stack
	if (Dll::Tls()==0) Dll::SetTls(this);

	if (log_err==KErrNone) {
		iLog.Write(_L("constructed"));

		iLog.CloseLog();
		CleanupStack::PopAndDestroy();
	}
}

void CApp_contextImpl::SetBBSession(CBBSession* BBSession)
{
	iBBSession=BBSession;
}

CBBSession* CApp_contextImpl::BBSession()
{
	return iBBSession;
}

void CApp_contextImpl::SetBBDataFactory(MBBDataFactory* aFactory)
{
	iBBDataFactory=aFactory;
}

MBBDataFactory* CApp_contextImpl::BBDataFactory()
{
	return iBBDataFactory;
}

void CApp_contextImpl::SetDebugLog(const TDesC& aDir, const TDesC& aFile)
{
	iDebugDir=aDir;
	iDebugFile=aFile;
	if (iDebugLogIsOpen) {
		iDebugLog.Close();
		iDebugLogIsOpen=EFalse;
	}
	if (iDebugLog.Connect()==KErrNone) {
		iDebugLogIsOpen=ETrue;
		iDebugLog.CreateLog(iDebugDir, iDebugFile, EFileLoggingModeAppend);
		iDebugLog.Write(_L("started log"));
	}
}

void CApp_contextImpl::DebugLogL(const TDesC& msg)
{
	TInt i=0;
	TInt len;
	if (msg.Length()==0) return;
	while (i<msg.Length()) {
		len=msg.Length()-i;
		if (len>128) len=128;
		iDebugLog.Write(msg.Mid(i, len));
		i+=128;
	}	
}

void CApp_contextImpl::DebugLog(const TDesC8& msg)
{
	if (!iDebugLogIsOpen) return;
	if (msg.Length()==0) return;
	TInt i=0;
	TInt len;
	while (i<msg.Length()) {
		len=msg.Length()-i;
		if (len>128) len=128;
		iDebugBuffer.Copy(msg.Mid(i, len));
		iDebugLog.Write(iDebugBuffer);
		i+=128;
	}
}

void CApp_contextImpl::DebugLog(const TDesC& aMessage)
{
#ifdef __WINS__
	RDebug::Print(aMessage.Left(128));
#endif
	if (!iDebugLogIsOpen) return;
	CC_TRAPD(err, DebugLogL(aMessage));
}

void CApp_contextImpl::DebugLog(const TDesC& aMessage, TInt aCode)
{
#ifdef __WINS__
	TBuf<12> num; num.AppendNum(aCode);
	RDebug::Print(aMessage.Left(128));
	RDebug::Print(num);
#endif
	if (!iDebugLogIsOpen) return;
#ifndef __WINS__
	TBuf<12> num; num.AppendNum(aCode);
#endif
	CC_TRAPD(err, DebugLogL(aMessage));
	CC_TRAP(err, DebugLogL(num));
}

void CApp_contextImpl::UserErrorLog(const TDesC& aMessage, TInt aCode)
{
	if (iReporter) {
		TInt c, p;
		SetHBufC(iReportingMsg, aMessage, c, p);
		AppendHBufC(iReportingMsg, aCode, c, p);

		if (iReportingMsg)
			iReporter->LogFormatted(*iReportingMsg);
	}
	DebugLog(aMessage, aCode);
}
void CApp_contextImpl::UserErrorLog(const TDesC& aMessage)
{
	if (iReporter) {
		iReporter->LogFormatted(aMessage);
	}
	DebugLog(aMessage);
}
void CApp_contextImpl::UserErrorLog(const TDesC& aMessage, const TDesC& aMessagePart)
{
	if (iReporter) {
		TInt c, p;
		SetHBufC(iReportingMsg, aMessage, c, p);
		AppendHBufC(iReportingMsg, aMessagePart, c, p);

		if (iReportingMsg)
			iReporter->LogFormatted(*iReportingMsg);
	}
	DebugLog(aMessage);
	DebugLog(aMessagePart);
}
void CApp_contextImpl::UserErrorLog(const TDesC& aMessage,
        const TDesC& aMessagePart, const TDesC& aMessagePart2)
{
	if (iReporter) {
		TInt c, p;
		SetHBufC(iReportingMsg, aMessage, c, p);
		AppendHBufC(iReportingMsg, aMessagePart, c, p);
		AppendHBufC(iReportingMsg, aMessagePart2, c, p);

		if (iReportingMsg)
			iReporter->LogFormatted(*iReportingMsg);
	}
	DebugLog(aMessage);
	DebugLog(aMessagePart);
	DebugLog(aMessagePart2);
}

CApp_contextImpl::~CApp_contextImpl()
{
	iReporter=0;
	
	RFileLogger iLog;
	TInt log_err=iLog.Connect();
	if (log_err==KErrNone) {
		iLog.CreateLog(_L("Context"),_L("CApp_contextImpl.txt"),EFileLoggingModeAppend);
		iLog.Write(_L("deleting"));
	}
	if (Dll::Tls()==this) Dll::SetTls(0);

	// Destroy objects in reverse order so that dependendent objects can be inserted
	for (TInt i=iOwnedObjects.Count() - 1; i >= 0; i--)
		{
			CBase* obj = iOwnedObjects[i];
			delete obj;
		}
	iOwnedObjects.Close();

	delete iBusyIndicators;

	delete iDiskSpaceNotifier[0];
	delete iDiskSpaceNotifier[1];

	delete iSettings;
#ifndef __S80__
	delete globalNote;
#endif

	if (!has_chunk) {
		if (iCallStackZero) {
			iCallStackZero-=4;
			delete iCallStackZero;
		}
	} else {
		//iCallStackZero-=4;
		//*(TInt*)iCallStackZero=0;
		iCallStackChunk.Close();
	}
	iCallStackZero=iCallStackCurrent=0;
#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	if (iAllocCallStackZero) {
		iAllocCallStackZero-=4;
		User::Free(iAllocCallStackZero);
	}
	iAllocCallStackZero=iAllocCallStackCurrent=0;
#endif

	if (!iFsOnly) {
	}

#ifndef __S60V3__
#if !defined(__WINS__) && !defined(__S60V2__)
	if (hasPhone) iPhone.Close();
#  if !defined(__WINS__)
	_LIT(KGsmModuleName, "phonetsy.tsy");
#  else
	_LIT(KGsmModuleName, "mm.tsy");
#endif
	if (hasTelServer) {
		iTelServer.UnloadPhoneModule( KGsmModuleName );
		iTelServer.Close();
	}
#endif
	if (hasSysAgent) iSysAgent.Close();
#endif


#if defined(__S60V3__) || defined(__S60V2FP3__)
	delete iTelephony;
#endif // V3

	delete cc;
	iFs.Close();

	if (iLastErrorInfo) iLastErrorInfo->Release();
	iLastErrorInfo=0;

	if (iDebugLogIsOpen) {
		iDebugLog.CloseLog();
		iDebugLog.Close();
	}

	delete iUserMsg; delete iTechMsg;
	delete iReportingMsg;
	//CloseSTDLIB();
	if (iRunOnShutdown) { (*iRunOnShutdown)(); }

	if (log_err==KErrNone) {
		iLog.Write(_L("deleted"));

		iLog.CloseLog();
		iLog.Close();
	}

}

EXPORT_C CApp_context* CApp_context::NewL(bool aFsOnly, const TDesC& Name)
{
	CApp_contextImpl* ret;
	ret=new (ELeave) CApp_contextImpl();
	CleanupStack::PushL(ret);
	ret->ConstructL(aFsOnly, Name);
	CleanupStack::Pop();
	return ret;
}

CApp_context::~CApp_context()
{
}

RFs& CApp_contextImpl::Fs()
{
	return iFs;
}

#if !defined(__S60V3__)
RSystemAgent& CApp_contextImpl::SysAgent()
{
	if (!hasSysAgent) {
		User::LeaveIfError(iSysAgent.Connect());
		hasSysAgent=true;
	}

	return iSysAgent;
}
#endif

#if !defined(__S60V3__)
RTelServer& CApp_contextImpl::TelServer()
{
#if !defined(__WINS__) || defined(__S60V2__)
	if (!hasTelServer) {
		User::LeaveIfError(iTelServer.Connect());
#  ifndef __WINS__
		_LIT(KGsmModuleName, "phonetsy.tsy");
#  else
		_LIT(KGsmModuleName, "mm.tsy");
#  endif
		TInt err;
		err=iTelServer.LoadPhoneModule( KGsmModuleName );
		if (err==KErrNone) {
			hasTelServer=true;
		} else {
			iTelServer.Close();
			User::Leave(err);
		}
	}
#endif
	return iTelServer;

}

RBasicGsmPhone& CApp_contextImpl::Phone()
{
#ifndef __WINS__
#if !defined(__WINS__) || defined(__S60V2__)
	if (!hasPhone) {
		RTelServer::TPhoneInfo info;
		User::LeaveIfError( TelServer().GetPhoneInfo( 0, info ) );
		User::LeaveIfError( iPhone.Open( iTelServer, info.iName ) );
		hasPhone=true;
	}
#endif

	return iPhone;
#else
	return *(RBasicGsmPhone*)0;
#endif
}
#endif

#if defined(__S60V3__) || defined(__S60V2FP3__)
CTelephony&	CApp_contextImpl::Telephony()
{
	if (!iTelephony) {
		iTelephony=CTelephony::NewL();
	}
	return *iTelephony;
}
#endif // V3

CCnvCharacterSetConverter* CApp_contextImpl::CC()
{
	if (!cc) {
		cc=CCnvCharacterSetConverter::NewL();
		cc->PrepareToConvertToOrFromL(KCharacterSetIdentifierIso88591, iFs);
	}
	return cc;
}

void CApp_contextImpl::SetFileLog(MPausable* log)
{
	iFileLog=log;
	if (iFileLog && !iSpaceLeft) iFileLog->pause();
}


void CApp_contextImpl::DiskSpaceThreshold(TInt /*aDrive*/)
{
	int v;
	iSpaceLeft=true;
	TBuf<100> msg;

	Reporting().DebugLog(_L("DiskSpaceThreshold"));
	if (!HasMMC(iFs)) {
		Reporting().DebugLog(_L("!HasMMC"));
		// kill app if on MMC
		if (iAppDir.Left(1).CompareF(_L("e"))==0)
		{
			Reporting().DebugLog(_L("exiting"));
			User::Leave(KLeaveExit);
		}
		Reporting().DebugLog(_L("not exiting"));
	} else {
		Reporting().DebugLog(_L("HasMMC"));
	}
	CheckSpaceAndNotifiers();
#ifndef __S80__
	TAknGlobalNoteType notetype;
#endif
	if (!iSpaceLeft) {
		if (iFileLog) iFileLog->pause();
		msg.Append(_L("WARNING! No space left for logs. Please delete some pictures and other media."));
#ifndef __S80__
		notetype=EAknGlobalErrorNote;
#endif
	} else {
		if (iFileLog) iFileLog->unpause();
		msg.Append(_L("Logging continues."));	
#ifndef __S80__
		notetype=EAknGlobalInformationNote;
#endif
	}
#ifndef __S80__
	if (globalNote) {
		//FIXME: where should this be done and if -MR 20061219
		//CC_TRAPD(err, globalNote->ShowNoteL(notetype, msg));
	}
#endif
}

void CApp_contextImpl::ShowGlobalNote(TInt ntype, const TDesC& msg)
{
#ifndef __S80__
	TAknGlobalNoteType notetype=(TAknGlobalNoteType)ntype;
	if (!globalNote) {
		TRAPD(err, globalNote=CAknGlobalNote::NewL());
	}
	if (globalNote) {
		globalNote->SetSoftkeys(R_AVKON_SOFTKEYS_OK_EMPTY);
		CC_TRAPD(err, globalNote->ShowNoteL(notetype, msg));
	}
#endif
}
void CDiskSpaceNotifier::ConstructL()
{
	CActiveScheduler::Add(this);
	iStatus=KRequestPending;
#ifndef __WINS__
	iFs.NotifyDiskSpace(iThreshold, iDrive, iStatus);
#else
	iExists=BaflUtils::FileExists(iFs, _L("c:\\data\\full.txt"));
	iFs.NotifyChange(ENotifyEntry, iStatus, _L("c:\\data\\"));
#endif
	SetActive();
}

CDiskSpaceNotifier* CDiskSpaceNotifier::NewL(RFs& fs, TInt aDrive, MDiskSpace* aNotifier, TInt64 aThreshold)
{
	auto_ptr<CDiskSpaceNotifier> ret(new (ELeave) CDiskSpaceNotifier(fs, aDrive, aNotifier, aThreshold));
	ret->ConstructL();
	return ret.release();
}

CDiskSpaceNotifier::~CDiskSpaceNotifier()
{
	Cancel();
}

CDiskSpaceNotifier::CDiskSpaceNotifier(RFs& fs, TInt aDrive, MDiskSpace* aNotifier, TInt64 aThreshold) : 
	CCheckedActive(EPriorityNormal, _L("CDiskSpaceNotifier")), iNotifier(aNotifier), iFs(fs), iDrive(aDrive),
	iThreshold(aThreshold)
{
}

void CDiskSpaceNotifier::CheckedRunL()
{
	TInt err=iStatus.Int();
	if (err==KErrCorrupt) {
		return;
	}
	iStatus=KRequestPending;
#ifndef __WINS__
	iFs.NotifyDiskSpace(iThreshold, iDrive, iStatus);
#else
	iFs.NotifyChange(ENotifyEntry, iStatus, _L("c:\\data\\"));
#endif
	SetActive();
#ifndef __WINS__
	if (err==KErrNone) {
		iNotifier->DiskSpaceThreshold(iDrive);
	}
#else
	TBool exists=BaflUtils::FileExists(iFs, _L("c:\\data\\full.txt"));
	TBool existed=iExists;
	iExists=exists;
	if (exists!=existed) {
		iNotifier->DiskSpaceThreshold(iDrive);
	}
#endif
}

void CDiskSpaceNotifier::DoCancel()
{
#ifndef __WINS__
	iFs.NotifyDiskSpaceCancel();
#else
	iFs.NotifyChangeCancel();
#endif
}

void CApp_contextImpl::CheckSpaceAndNotifiers()
{
	if (!iDiskSpaceNotifier[0]) {
		iDrive[0]=2;
		iDiskSpaceNotifier[0]=CDiskSpaceNotifier::NewL(iFs, 2, this, iDiskSpaceLimit);
	}

	if (!iDiskSpaceNotifier[1] && HasMMC(iFs)) {
#if !defined(__WINS__) || defined(__S60V2FP3__) || defined(__S60V3__)
		{
		iDrive[1]=-1;
		if (iUseMMC && HasMMC(iFs)) {
			// memory card
			iDrive[1]=4;
			iDiskSpaceNotifier[1]=CDiskSpaceNotifier::NewL(iFs, 4, this, iDiskSpaceLimit);
		}
		}
#endif
	}
	if (iDiskSpaceNotifier[1] && !HasMMC(iFs)) {
		iDrive[1]=-1;
		delete iDiskSpaceNotifier[1];
		iDiskSpaceNotifier[1]=0;
	}
#ifndef __WINS__
	int v;
	for(v=0; v<2; v++) {
		if (iDrive[v] < 0) continue;
		TVolumeInfo volinfo;
		if (iFs.Volume(volinfo, iDrive[v])==KErrNone) {
			if (volinfo.iFree<=iDiskSpaceLimit) {
				iSpaceLeft=false;
			}
		}
	}
#else
	if (BaflUtils::FileExists(iFs, _L("c:\\data\\full.txt"))) {
		iSpaceLeft=false;
	}
#endif
}

bool CApp_contextImpl::NoSpaceLeft()
{
#if defined(__WINS__) && defined(__DISKSPACE_TEST__)
	static TTime starttime=TTime(0);
	if (starttime==TTime(0)) {
		starttime=GetTime();
	} else {
		TTime now=GetTime();
		TTimeIntervalSeconds seconds=0;
		TInt err=now.SecondsFrom(starttime, seconds);
		if (err!=KErrNone || abs(seconds.Int())>10) return true;
	}
#endif
	if (iFsOnly) return false;

	if (!iDiskSpaceNotifier[0]) {
		CheckSpaceAndNotifiers();
	}

	return !iSpaceLeft;
}

const TDesC& CApp_contextImpl::DataDir()
{
	return iDataDir;
}

void CApp_contextImpl::SetDataDir(const TDesC& Dir, bool UseMMC)
{
#ifndef __S60V3__
	iUseMMC=UseMMC;
#else
	iUseMMC=ETrue;
#endif
	BaflUtils::EnsurePathExistsL(iFs, Dir);
	if (UseMMC && HasMMC(iFs)) {
		// memory card

		TFileName f=Dir;
		f.Replace(0, 1, _L("E"));
		CC_TRAPD(err, BaflUtils::EnsurePathExistsL(iFs, f));
	}
	iDataDir=Dir;
}

const TDesC& CApp_contextImpl::AppDir()
{
	return iAppDir;
}

void CApp_contextImpl::SetAppDir(const TDesC& Dir)
{
#ifdef __S60V3__
	return;
#endif
	iAppDir=Dir;
}

void CApp_contextImpl::SetSettings(MSettings* Settings)
{
	iSettings=Settings;
}

MSettings& CApp_contextImpl::Settings()
{
	return *iSettings;
}


RWsSession& CApp_contextImpl::Ws()
{
	return CCoeEnv::Static()->WsSession();
}

const char* CApp_contextImpl::CallStack()
{
	return iCallStackCurrent;
}

void CApp_contextImpl::SetActiveErrorReporter(MActiveErrorReporter* Reporter)
{
	iReporter=Reporter;
}

MActiveErrorReporter* CApp_contextImpl::GetActiveErrorReporter()
{
	return iReporter;
}

void CApp_contextImpl::ReportActiveError(const TDesC& Source,
	const TDesC& Reason, TInt Code)
{
	if (iReporter) {
		iReporter->ReportError(Source, Reason, Code);
	} else {
		User::Leave(Code);
		/*
		TFileName fn=_L("c:\\context_error.txt");
		{
			auto_ptr<HBufC> stack(GetFormattedCallStack(_L("Call stack")));
			RAFile f; f.ReplaceLA(Fs(), fn, EFileWrite);
			f.Write(_L8("\xff\xfe"));

			TBuf<40> m=_L("Unhandled error: ");
			m.AppendNum(Code); m.Append(_L(" "));
			TPtrC8 p;
			p.Set( (TUint8*)m.Ptr(), m.Size());
			f.Write(p);
			p.Set( (TUint8*)Reason.Ptr(), Reason.Size());
			f.Write(p);
			m=_L("\nin ");
			p.Set( (TUint8*)m.Ptr(), m.Size());
			f.Write(p);

			p.Set( (TUint8*)Source.Ptr(), Source.Size());
			f.Write(p);
			if (stack.get()) {
				TPtrC stackp=stack->Des();
				m=_L("\n");
				p.Set( (TUint8*)m.Ptr(), m.Size());
				f.Write(p);
				p.Set( (TUint8*)stackp.Ptr(), stackp.Size());
				f.Write(p);
			}
		}
		RAApaLsSession session; session.ConnectLA();
		TUid uid;
		TDataType dataType;
		session.AppForDocument(fn, uid, dataType);
		TThreadId threadId;
		User::LeaveIfError(session.StartDocument(fn, dataType, threadId));
		//FIXME: this should stop the active scheduler, to-be-tested
		User::Leave(Code);
		*/
	}
}

EXPORT_C MApp_context* MApp_context::Static()
{
	return (CApp_context*)Dll::Tls();
}

EXPORT_C CApp_context* CApp_context::Static()
{
	return (CApp_context*)Dll::Tls();
}

EXPORT_C const TDesC& MContextBase::DataDir()
{
	return iContext->DataDir();
}

EXPORT_C const TDesC& MContextBase::AppDir()
{
	return iContext->AppDir();
}

EXPORT_C MSettings& MContextBase::Settings()
{
	return iContext->Settings();
}


EXPORT_C bool MContextBase::NoSpaceLeft()
{
	return iContext->NoSpaceLeft();
}

EXPORT_C MContextBase::MContextBase() : iContext(MApp_context::Static())
{
}

EXPORT_C MContextBase::MContextBase(MApp_context& Context) : iContext(&Context)
{
}


EXPORT_C MContextBase::~MContextBase()
{
}

EXPORT_C RFs& MContextBase::Fs()
{
	return iContext->Fs();
}

#ifndef __S60V3__
EXPORT_C RSystemAgent& MContextBase::SysAgent()
{
	return iContext->SysAgent();
}
#endif

#if !defined(__S60V3__)
EXPORT_C RTelServer& MContextBase::TelServer()
{
	return iContext->TelServer();
}

EXPORT_C RBasicGsmPhone& MContextBase::Phone()
{
	return iContext->Phone();
}
#endif

#if defined(__S60V3__) || defined(__S60V2FP3__)
EXPORT_C CTelephony& MContextBase::Telephony()
{
	return iContext->Telephony();
}
#endif

EXPORT_C MApp_context& MContextBase::AppContext()
{
	return *iContext;
}
EXPORT_C MApp_context_access& MContextBase::AppContextAccess()
{
	return *iContext;
}

EXPORT_C CCnvCharacterSetConverter* MContextBase::CC()
{
	return iContext->CC();
}

EXPORT_C RWsSession& MContextBase::Ws()
{
	return iContext->Ws();
}

EXPORT_C void MContextBase::ReportActiveError(const TDesC& Source,
	const TDesC& Reason, TInt Code)
{
	iContext->ReportActiveError(Source, Reason, Code);
}

EXPORT_C MApp_context* GetContext()
{
	return MApp_context::Static();
}

EXPORT_C MApp_context* MContextBase::GetContext() const
{
	return iContext;
}

EXPORT_C CBBSession* MContextBase::BBSession()
{
	return iContext->BBSession();
}

_LIT(KSep, "::");
_LIT8(KSep8, "::");

TInt CApp_contextImpl::PushCallStack(const TDesC8& Name)
{
	return PushCallStack(KNullDesC8, Name);
}

TInt CApp_contextImpl::PushAllocCallStack(const TDesC8& Name)
{
#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	return PushAllocCallStack(KNullDesC8, Name);
#else
	return KErrNone;
#endif
}

_LIT8(KSpace8, " ");

TPtrC8 CApp_contextImpl::GetCurrentClass()
{
	if (iCallStackCurrent==iCallStackZero) {
		return TPtrC8(0, 0);
	}
	TInt* pd=(TInt*)(iCallStackCurrent - 4);
	char* p= iCallStackZero + *pd;
	TInt len=(iCallStackCurrent - p - 4);
	TPtr8 ret( (TUint8*)(p), len, len + 
		(iCallStackZero+iCallStackSize-iCallStackCurrent-8) );
	TInt seppos=ret.Find(KSep8);
	if (seppos!=KErrNotFound) {
		if (seppos > ret.Length() ) {
			ret.SetLength(0);
		} else {
			ret.SetLength( seppos );
		}
	}
	return ret;
}

TInt CApp_contextImpl::PushCallStack(const TDesC8& Class, const TDesC8& Func)
{
	return PushCallStack(Class, Func, iCallStackZero, iCallStackCurrent);
}

TInt CApp_contextImpl::PushAllocCallStack(const TDesC8& Class, const TDesC8& Func)
{
#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	return PushCallStack(Class, Func, iAllocCallStackZero, iAllocCallStackCurrent);
#else
	return KErrNone;
#endif
}

TInt CApp_contextImpl::PushCallStack(const TDesC8& Class, const TDesC8& Func, char *&aCallStackZero, 
	char *&aCallStackCurrent)
{
#if 0 && defined(__WINS__)
	if (aCallStackZero==iCallStackZero) {
			TBuf<50> tmp;
			TBuf<178> msg=iIndent.Left(60);
			msg.Append(_L("-> "));
			tmp.Copy(Class.Left(50));
			msg.Append(tmp);
			msg.Append(_L("::"));
			tmp.Copy(Func.Left(50));
			msg.Append(tmp);

			RDebug::Print(msg);
	}
#endif
	if (aCallStackZero==iCallStackZero) if (iIndent.Length()<iIndent.MaxLength()) iIndent.SetLength(iIndent.Length()+1);
	if (aCallStackZero==iCallStackZero) if (iDebugCallStack) {
		if (iDebugLogIsOpen) {
			TBuf<50> tmp;
			TBuf<178> msg=iIndent.Left(60);
			msg.Append(_L("-> "));
			tmp.Copy(Class.Left(50));
			msg.Append(tmp);
			msg.Append(_L("::"));
			tmp.Copy(Func.Left(50));
			msg.Append(tmp);
			DebugLog(msg);
		}
	}
	if (!aCallStackZero) return KErrNotFound;
	TInt Length=Class.Length()+Func.Length()+2;
	if (aCallStackCurrent+Length+64 > aCallStackZero+iCallStackSize) return KErrNoMemory;

	TInt len, added;
	len=Length;
	// align on 4 byte boundaries
	added=(4-(len % 4));
	if (added==4) added=0;
	len+=added;

	TPtr8 pt( (TText8*)aCallStackCurrent, len);
	pt=Class;
	pt.Append(KSep8);
	pt.Append(Func);

	while(added>0) {
		pt.Append(KSpace8);
		added--;
	}

	TInt* pp=(TInt*) (aCallStackCurrent+len);
#ifdef DEBUG_CALLSTACK
	//FIXME 8->16
	TBuf<256> msg;
	msg.Format(_L("PushCallStack name at %d length %d link at %d next at %d: %S::%S"),
		(TInt)aCallStackCurrent, TInt(len), 
		(TInt)pp, (TInt)(aCallStackCurrent+len + 4), &Class, &Func);
	RDebug::Print(msg);
#endif

	*pp=aCallStackCurrent-aCallStackZero;
	aCallStackCurrent+=len + 4;

	*((TInt*) (aCallStackZero-4)) = (aCallStackCurrent - aCallStackZero);

	return KErrNone;
}

void CApp_contextImpl::SetCurrentComponent(TUid aComponentUid, TInt aComponentId)
{
	if (!iCallStackZero) return;
	TUint* uidp=(TUint*)(iCallStackZero+iCallStackSize-16);
	TInt* idp=(TInt*)(iCallStackZero+iCallStackSize-12);
	*uidp=aComponentUid.iUid;
	*idp=aComponentId;
}

void CApp_contextImpl::GetCurrentComponent(TUid& aComponentUid, TInt& aComponentId)
{
	aComponentUid=TUid::Uid(0);
	aComponentId=0;
	if (!iCallStackZero) return;
	TUint* uidp=(TUint*)(iCallStackZero+iCallStackSize-16);
	TInt* idp=(TInt*)(iCallStackZero+iCallStackSize-12);
	aComponentUid=TUid::Uid(*uidp);
	aComponentId=*idp;
}

TPtr8 CApp_contextImpl::PopAllocCallStack()
{
#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	return PopCallStack(iAllocCallStackZero, iAllocCallStackCurrent);
#else
	return TPtr8(0, 0);
#endif
}

TPtr8 CApp_contextImpl::PopCallStack()
{
	return PopCallStack(iCallStackZero, iCallStackCurrent);
}

TPtr8 CApp_contextImpl::PopCallStack(char *&aCallStackZero, char *&aCallStackCurrent)
{
	if (aCallStackCurrent==aCallStackZero) {
		return TPtr8(0, 0);
	}
	TInt* pd=(TInt*)(aCallStackCurrent - 4);
	char* p= aCallStackZero + *pd;
	TInt len=(aCallStackCurrent - p - 4);
	TPtr8 ret( (TUint8*)(p), len, len + 
		(aCallStackZero+iCallStackSize-aCallStackCurrent-8) );

	if (aCallStackZero==iCallStackZero && iIndent.Length()>0)
		iIndent.SetLength(iIndent.Length()-1);
#if 0 && defined(__WINS__)
	{
		TBuf<128> msg=iIndent.Left(60);
		msg.Append(_L("<- "));
		TBuf<50> tmp; tmp.Copy(ret.Left(50));
		msg.Append(tmp);
		RDebug::Print(msg);
	}
#endif
	if (aCallStackZero==iCallStackZero  && iDebugCallStack) {
		if (iDebugLogIsOpen) {
			TBuf<128> msg=iIndent.Left(60);
			msg.Append(_L("<- "));
			TBuf<50> tmp; tmp.Copy(ret.Left(50));
			msg.Append(tmp);
			DebugLog(msg);
		}
	}
#ifdef DEBUG_CALLSTACK
	TBuf<200> msg;
	msg.Format(_L("PopCallStack name at %d length %d link at %d next at %d: %S"),
		(TInt)p, TInt(len*2), 
		(TInt)pd, (TInt)(aCallStackCurrent), &ret);
	RDebug::Print(msg);
#endif

	aCallStackCurrent=p;
	*((TInt*) (aCallStackZero-4)) = (aCallStackCurrent - aCallStackZero);
	return ret;
}

void CApp_contextImpl::ResetCallStack()
{
	ResetCallStack(iCallStackZero, iCallStackCurrent);
}

void CApp_contextImpl::ResetAllocCallStack()
{
#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	ResetCallStack(iCallStackZero, iCallStackCurrent);
#endif
}

void CApp_contextImpl::ResetCallStack(char *&aCallStackZero, char *&aCallStackCurrent)
{
	aCallStackCurrent=aCallStackZero;
	*((TInt*) (aCallStackZero-4)) = 0;
	if (aCallStackZero==iCallStackZero) iIndent.Zero();
}


TInt CApp_contextImpl::CallStackDepth() const
{
	return iIndent.Length();
}


class TFormattedStack : public MCallStackVisitor {
public:
	TFormattedStack(HBufC* Into, const TDesC& Prefix) : iInto(Into), iPrefix(Prefix) { }
private:
	virtual void BeginStack()
	{
		iInto->Des().Zero();
		iInto->Des().Append(iPrefix);
		iInto->Des().Append(_L(":"));
	}
	virtual void VisitItem(const TDesC& Name)
	{
		iInto->Des().Append(Name);
		iInto->Des().Append(_L("|"));
	}
	virtual void EndStack()
	{
	}

	HBufC* iInto;
	const TDesC& iPrefix;
};

HBufC* CApp_contextImpl::GetFormattedCallStack(const TDesC& prefix)
{
	const char* aStack=iCallStackZero-4;
	const char* current=aStack+4+*(TInt*)aStack;
	aStack+=4;

	if (current==aStack) return 0;
	HBufC* ret=0;
	TInt len=((current-aStack)) + prefix.Length() + 1;
	CC_TRAPD(err, ret=HBufC::NewL( len ));
	if (err!=KErrNone) return 0;

	TFormattedStack f(ret, prefix);
	IterateStack(f);

#ifdef __WINS__
	if (ret->Des().Length()>128) {
		RDebug::Print(ret->Des().Left(128));
	} else {
		RDebug::Print(ret->Des());
	}
#endif
	return ret;
}


void CApp_contextImpl::IterateStack(MCallStackVisitor& aVisitor, const char* aStack)
{
	if (aStack==0) aStack=iCallStackZero-4;

	TBuf<202> item;
	aVisitor.BeginStack();
	const char* current=aStack+4+*(TInt*)aStack;
	aStack+=4;
	while (current!=aStack) {
		const char *prev=current -4;
		TInt* p=(TInt*)(prev);
		current=aStack + *p;
		TInt len=(prev-current);
		TPtrC8 n( (TUint8*)current, len);
		item.Copy(n);
		aVisitor.VisitItem(item);
	}
	aVisitor.EndStack();
}
		
MErrorInfo* CApp_contextImpl::GetLastErrorInfo(TErrorCode aCode)
{
	if (aCode.iCode==0) {
		if (iLastErrorInfo) iLastErrorInfo->Release();
		iLastErrorInfo=0;
		return 0;
	}
	if (! iLastErrorInfo ) {
		auto_ptr<HBufC> stack(0);
		CC_TRAPD(err, stack.reset( GetFormattedCallStack(KNullDesC) ) );
		CC_TRAP(err, iLastErrorInfo=CErrorInfo::NewL(aCode, 0, 0, stack.get(),
			EBug, EError, 0));
		if (iLastErrorInfo) stack.release();
	}
	return iLastErrorInfo;
}

MErrorInfo* CApp_contextImpl::GetLastErrorInfo()
{
	return iLastErrorInfo;
}

MReporting& CApp_contextImpl::Reporting()
{
	return *this;
}

MErrorInfoManager& CApp_contextImpl::ErrorInfoMgr()
{
	return *this;
}
MCallStack& CApp_contextImpl::CallStackMgr()
{
	return *this;
}

void CApp_contextImpl::ResetLastErrorInfo()
{
	if (iLastErrorInfo) iLastErrorInfo->Release();
	iLastErrorInfo=0;
}

// Concepts:
// !Debug call stack!

#if defined(__WINS__) && !defined(__S60V3__)
void TCallStackItem::PopCleanupItem(void* aPtr) {
	TCallStackItem* i=(TCallStackItem*)aPtr;
	if (i && ! i->iPopped) {
		if (i->iCallStack) i->iCallStack->PopAllocCallStack();
		i->iPopped=ETrue;
	}
}
#endif

EXPORT_C TCallStackItem::TCallStackItem(const TDesC8& Name, MApp_context* Context)
{
#ifdef __LEAVE_EQUALS_THROW__
	iUnwinding=std::uncaught_exception();
#endif
#ifdef FUNCTION_LEVEL_STATISTICS
	iDoStats = ETrue;
	iAppContext=Context;
#endif
#if defined(__WINS__) && !defined(__S60V3__)
	iPopped=ETrue;
#endif
	if (!Context) {
		iCallStack=0;
#if defined(FUNCTION_LEVEL_STATISTICS)
		iAppContext=0;
#endif
		return;
	}
	iCallStack=&(Context->CallStackMgr());

	TInt err=iCallStack->PushCallStack(Name.Left(100));
#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	if (err==KErrNone) err=iCallStack->PushAllocCallStack(Name.Left(100));
#endif
#if defined(__WINS__) && !defined(__S60V3__)
	iPopped=EFalse;
	CleanupStack::PushL(TCleanupItem(PopCleanupItem, (void*)this));
#endif
	if (err!=KErrNone) {
		iCallStack=0;
#if defined(FUNCTION_LEVEL_STATISTICS)
		iAppContext=0;
#endif
	} else {
#ifdef FUNCTION_LEVEL_STATISTICS		
		iStartTicks=User::TickCount();
#endif
	}
}


EXPORT_C TCallStackItem::TCallStackItem(const TDesC8& Class, const TDesC8& Func, 
										MApp_context* Context, TBool aDoStats)
{
#ifdef __LEAVE_EQUALS_THROW__
	iUnwinding=std::uncaught_exception();
#endif
#ifdef FUNCTION_LEVEL_STATISTICS
	iDoStats = aDoStats;
	if ( iDoStats )
		iAppContext=Context;
#endif
#if defined(__WINS__) && !defined(__S60V3__)
	iPopped=ETrue;
#endif
	if (!Context) {
		iCallStack=0;
#ifdef FUNCTION_LEVEL_STATISTICS
		iAppContext=0;
#endif
		return;
	}
	iCallStack=&(Context->CallStackMgr());
	
	TInt err=iCallStack->PushCallStack(Class.Left(100), Func.Left(100));
#if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
# if ! defined(__WINS__)
	if ( iDoStats )
# endif 
		{
			if (err==KErrNone) err=iCallStack->PushAllocCallStack(Class.Left(100), Func.Left(100));
		}
#endif

#if defined(__WINS__)
#  if !defined(__S60V3__)
	iPopped=EFalse;
	CleanupStack::PushL(TCleanupItem(PopCleanupItem, (void*)this));
#  endif
#endif
	if (err!=KErrNone) {
		iCallStack=0;
#if defined(FUNCTION_LEVEL_STATISTICS)
		iAppContext=0;
#endif
	} else {
#ifdef FUNCTION_LEVEL_STATISTICS
		if ( iDoStats )
			iStartTicks=User::TickCount();
#endif
	}
}

EXPORT_C TCallStackItem::~TCallStackItem()
{
#if defined(__WINS__) && !defined(__S60V3__)
	if (! iPopped ) {
		CleanupStack::Pop();
		if (iCallStack) iCallStack->PopAllocCallStack();
		iPopped=ETrue;
	}
#else
#  if defined(__WINS__) || defined(FUNCTION_LEVEL_STATISTICS)
	TPtr8 m(0, 0);
#    if ! defined(__WINS__) 
	   if ( iDoStats )
#    endif
		   {
			   if (iCallStack) m.Set(iCallStack->PopAllocCallStack());
		   }
#  endif
#endif
	if (!iCallStack) return;

#ifdef FUNCTION_LEVEL_STATISTICS
	if ( iDoStats )
		{
			TUint elapsed=User::TickCount()-iStartTicks;
			if (elapsed>0 && m.Length()>0 && (m.Length()+26 < m.MaxLength()) ) {
				m.Append(_L(": "));
				m.AppendNum(elapsed);
				m.Append(_L(" ELAPSED"));
				iAppContext->Reporting().DebugLog(m);
			}
		}
#endif
#ifdef __LEAVE_EQUALS_THROW__
	// don't pop the stack on exception, so that
	// the source of the error can be read
	if (std::uncaught_exception()!=iUnwinding) return;
#endif
	iCallStack->PopCallStack();
}

EXPORT_C void MoveIntoDataDirL(MApp_context& Context, const TDesC& FileName)
{
	TFileName appfile, datafile;

	if (Context.AppDir().Length()==0 || Context.DataDir().Length()==0) return;

	appfile.Format(_L("%S%S"), &Context.AppDir(), &FileName);
	datafile.Format(_L("%S%S"), &Context.DataDir(), &FileName);

	// if appdir and datadir are the same
	if (! appfile.CompareF(datafile)) return;

	if (BaflUtils::FileExists(Context.Fs(), appfile)) {
		User::LeaveIfError(BaflUtils::CopyFile(Context.Fs(), appfile, datafile));
		User::LeaveIfError(BaflUtils::DeleteFile(Context.Fs(), appfile));
	}
}

#ifdef __WINS__

TInt StarterThread(TAny* aParam)
{
	TLibraryFunction functionWinsMain = (TLibraryFunction)aParam;
	functionWinsMain();
	return 0;
}

EXPORT_C void StartStarterL(const TDesC& StackName, TUid AppUid, bool /*CheckForRunning*/, RWsSession& /*Ws*/)
#else
EXPORT_C void StartStarterL(const TDesC& StackName, TUid AppUid, bool CheckForRunning)
#endif
{
	TBuf<100> cmdline=StackName;;
	cmdline.Append(_L("*")); cmdline.AppendNum((TInt)AppUid.iUid);
#if defined(__WINS__) && !defined(__S60V3__)
	TFindThread f(_L("contextphone-starter*"));
	TFullName t;
	if (f.Next(t)==KErrNone) { 
		return;
	}
	RAThread serverthread;
	RLibrary lib;
	TInt err=lib.Load(_L("z:\\system\\apps\\starter\\starter.app"));
	User::LeaveIfError(err);
	TLibraryFunction functionWinsMain = lib.Lookup(1);
	TName threadName(_L("contextphone-starter"));
	threadName.AppendNum(Math::Random(), EHex);
#ifndef EKA2
	serverthread.CreateLA(threadName,   // create new server thread
		&StarterThread, // thread's main function
		KDefaultStackSize,
		functionWinsMain,  // parameters
		&lib,
		NULL,
		0x1000,
		0x1000000,
		EOwnerProcess);
#else
	serverthread.CreateLA(threadName,   // create new server thread
		&StarterThread, // thread's main function
		KDefaultStackSize,
		0x1000,
		0x1000000,
		functionWinsMain,  // parameters
		EOwnerProcess);
#endif
	
	lib.Close();    // if successful, server thread has handle to library now
	serverthread.SetPriority(EPriorityMore);
	serverthread.Resume();

#else
#ifndef __S60V3__
	TFileName fnAppPath = _L("c:\\system\\programs\\cl_starter.exe");
#else
	TFileName fnAppPath = _L("cl_starter.exe");
#endif
#ifndef EKA2

	{
		TFindProcess f(_L("*"));
		TFullName t;
		RProcess r;
		while (f.Next(t)==KErrNone) {
			r.Open(t);
			CleanupClosePushL(r);
			//LogAppEvent(t); LogAppEvent(r.FileName());
			if (r.FileName().Length()>0 && (! r.FileName().Mid(1).CompareF( fnAppPath.Mid(1) )) ) {
				// already running
				bool running=false;
				if (CheckForRunning && r.CommandLineLength()==0) running=true;
				else {
					HBufC* c=HBufC::NewLC(r.CommandLineLength());
					TPtr16 d=c->Des();
					r.CommandLine(d);
					if (! (d.Compare(cmdline)) ) {
						running=true;
					}
					CleanupStack::PopAndDestroy();
				}
				if (running) {
					CleanupStack::PopAndDestroy();
					return;
				}
			}
			CleanupStack::PopAndDestroy();
		}
	}
#else
	TBuf<100> semap=_L("CC_starter_");
	semap.Append(cmdline);
	semap.Replace(semap.Locate('*'), 1, _L(" "));
	RSemaphore s; 
	if (s.OpenGlobal(semap)==KErrNone) {
		s.Close();
		return;
	}
#endif

	{
		RProcess server;

		if (server.Create(fnAppPath, cmdline) != KErrNone) {
			fnAppPath.Replace(0, 1, _L("e"));
			User::LeaveIfError(server.Create(fnAppPath, cmdline));
		}
		CleanupClosePushL(server);
		server.Resume();

		CleanupStack::PopAndDestroy(); // server
	}
#endif
}

EXPORT_C TBool HasMMC(RFs& aFs)
{
	TDriveInfo i;
#if defined(__S60V3__) && defined(__WINS__)
	TInt tries=0;
	static TBool has_seen=EFalse;
again:
#endif
	if (aFs.Drive(i, EDriveE)==KErrNone) {
		if (i.iType!=EMediaNotPresent &&
			i.iType!=EMediaUnknown &&
			i.iType!=EMediaCdRom &&
			i.iType!=EMediaRom) {
			// memory card
#ifdef __WINS__
			has_seen=ETrue;
#endif
			return ETrue;
		}
#if defined(__S60V3__) && defined(__WINS__)
		if (!has_seen && tries<20) {
			User::After(300*1000);
			goto again;
		}
#endif
	}
	return EFalse;
}

EXPORT_C MReporting& MContextBase::Reporting()
{
	return iContext->Reporting();
}

EXPORT_C MBBDataFactory* MContextBase::BBDataFactory()
{
	return iContext->BBDataFactory();
}

EXPORT_C MDataCounter& MContextBase::DataCounter()
{
	return iContext->DataCounter();
}

EXPORT_C MErrorInfoManager& MContextBase::ErrorInfoMgr()
{
	return iContext->ErrorInfoMgr();
}

EXPORT_C MCallStack& MContextBase::CallStackMgr()
{
	return iContext->CallStackMgr();
}

EXPORT_C MRecovery& MContextBase::Recovery()
{
	return iContext->Recovery();
}

EXPORT_C MJuikLayout& MContextBase::Layout() const
{
	return iContext->Layout();
}

EXPORT_C MJuikIconManager& MContextBase::IconManager() const
{
	return iContext->IconManager();
}

EXPORT_C MTestSupport& MContextBase::TestSupport()
{
	return iContext->TestSupport();
}


#ifdef __S60V2FP3__
extern "C"
{
EXPORT_C int _fltused() { return 0; }
}
#endif

#if defined(__WINS__) && defined(__S60V3__)
EXPORT_C void AllocateContextCommonExceptionData()
{
	TRAPD(dummy, { TBuf<1> b; User::Leave(-1); });
}
#endif

#if defined(__WINS__) && defined(EKA2)
extern "C" {
void _DisposeAllThreadData();
}
EXPORT_C void DeAllocateContextCommonExceptionData()
{
	_DisposeAllThreadData();
}
#endif

#ifdef __S60V3__
class RHeap2 : public RHeap {
public:
    TInt MinCell() { return iMinCell; }
    void SetMinCell(TInt aNewValue) { iMinCell=aNewValue; }
};

EXPORT_C void SwitchToBetterHeap(const TDesC& aName) 
{
    TUint16* nameb=new TUint16[256];
    if (!nameb) User::Exit(KErrNoMemory);
    TPtr16 name(nameb, 0, 256);
    name=aName;
    RThread me;
    name.AppendNum(me.Id());
    
    const TDesC& namec=name;
    
    RHeap *h=User::ChunkHeap(&namec, 64*1024, 6*1024*1024, 64*1024);
    if (!h) return;
    RHeap2 *h2=(RHeap2*)h;
    h2->SetMinCell(32);
    User::SwitchHeap(h);
}
#endif
