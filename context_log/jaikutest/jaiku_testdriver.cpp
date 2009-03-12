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

#include <e32def.h>
#include "break.h"
#include "testdriver_base.cpp"
#include "raii_f32file.h"

#include "concretedata.h"
#include "app_context_impl.h"
#include "db.h"
#include "cl_settings.h"
#include "bb_settings.h"
#include "cl_settings_impl.h"
#include <basched.h>

#include <c32comm.h>

#include <cdbcols.h>
#include <commdb.h>
#include "cbbsession.h"

#include "presencemaintainer.cpp"

#ifdef __S60V3__
#include "allocator.h"
#endif

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

class CJaikuTest {
public:
static void run_tests_innerL()
{
	TNoDefaults nd;
	auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("locatest")));
	auto_ptr<CBaActiveScheduler> sched(new (ELeave) CBaActiveScheduler);
	CActiveScheduler::Install(sched.get());

	c->SetDataDir(_L("c:\\system\\data\\context\\"), false);
	c->SetSettings(CBlackBoardSettings::NewL(*c, nd, KCLSettingsTuple));
	auto_ptr<CBBDataFactory> f(CBBDataFactory::NewL());
	c->SetBBDataFactory(f.get());
	auto_ptr<CBBSession> sess(CBBSession::NewL(*c, f.get()));
	c->SetBBSession(sess.get());
	c->SetActiveErrorReporter(output);

	auto_ptr<CPresenceMaintainer> pmp(CPresenceMaintainer::NewL(*c,
		0, 0, 0, 0));
	CPresenceMaintainerImpl* pm=(CPresenceMaintainerImpl*)pmp.get();
	_LIT(cn, "cellname");
	TBBLongString cellname(_L("cell1"), cn);
	CBBSensorEvent e(cn, KCellNameTuple, f.get());
	e.iData.SetOwnsValue(EFalse);
	e.iData.SetValue(&cellname);
	e.iStamp=TTime(100);
	pm->NewSensorEventL(KCellNameTuple, KNullDesC, e);

	TInt isactive=0;
	if (pm->iNonsignificantTimer->IsActive()) isactive=1;
	TEST_EQUALS( 1, isactive, _L("nonsig1") );

	{
		TBBBaseInfo b;
		b.iCurrent.iEntered=TTime(100);
		b.iCurrent.iLeft=TTime(0);
		b.iCurrent.iBaseName=_L("cell2");
		e.iData.SetValue(&b);
		pm->NewSensorEventL(KBaseTuple, KNullDesC, e);
		if (pm->iNonsignificantTimer->IsActive()) isactive=1;
		else isactive=0;
		TEST_EQUALS( 0, isactive, _L("nonsig2") );
	}

	cellname()=_L("cell3");
	e.iStamp=TTime(100);
	e.iData.SetValue(&cellname);
	pm->NewSensorEventL(KCellNameTuple, KNullDesC, e);
	if (pm->iNonsignificantTimer->IsActive()) isactive=1;
	else isactive=0;
	TEST_EQUALS( 0, isactive, _L("nonsig3") );

	cellname()=_L("cell4");
	e.iStamp=TTime(110);
	e.iData.SetValue(&cellname);
	pm->NewSensorEventL(KCellNameTuple, KNullDesC, e);
	if (pm->iNonsignificantTimer->IsActive()) isactive=1;
	else isactive=0;
	TEST_EQUALS( 0, isactive, _L("nonsig4") );

	{
		TBBBaseInfo b;
		b.iCurrent.iEntered=TTime(100);
		b.iCurrent.iLeft=TTime(110);
		b.iCurrent.iBaseName=_L("cell2");
		e.iData.SetValue(&b);
		pm->NewSensorEventL(KBaseTuple, KNullDesC, e);
		if (pm->iNonsignificantTimer->IsActive()) isactive=1;
		else isactive=0;
		TEST_EQUALS( 0, isactive, _L("nonsig5") );
	}

	cellname()=_L("cell3");
	e.iStamp=TTime(120);
	e.iData.SetValue(&cellname);
	pm->NewSensorEventL(KCellNameTuple, KNullDesC, e);
	if (pm->iNonsignificantTimer->IsActive()) isactive=1;
	else isactive=1;
	TEST_EQUALS( 1, isactive, _L("nonsig6") );

}
};

#include "cc_component.h"

class TLogRecovery : public MRecovery {
public:
	virtual void RegisterComponent( TUid aComponentUid, TInt aComponentId, TInt aVersion, 
		const TDesC& aHumanReadableFunctionality) { }
	TState iState;
	virtual void SetState(TUid aComponentUid, TInt aComponentId, TState aState) { 
		iState=aState;
	}
	TState GetState(TUid aComponentUid, TInt aComponentId) {
		return iState;
	}
	TInt iErrorCount;
	virtual TInt GetErrorCount(TUid aComponentUid, TInt aComponentId) { return iErrorCount; }
	virtual void ResetErrorCount(TUid aComponentUid, TInt aComponentId) { iErrorCount=0; }
	virtual void ReportError(TUid aComponentUid, TInt aComponentId,
		const class MErrorInfo* aErrorInfo) {
			if (iErrorInfo) iErrorInfo->Release(); iErrorInfo=0;
			iErrorInfo=aErrorInfo->CreateCopyL();
	}
	~TLogRecovery() {
		if (iErrorInfo) iErrorInfo->Release();
	}

	MErrorInfo* iErrorInfo;
	TLogRecovery() : iErrorCount(0), iState(EUnknown), iErrorInfo(0) { }
};

_LIT(KComponent1, "Component1");
class CComponent1 : public CComponentBase {
public:
	CComponent1() : CComponentBase(KComponent1) { }
	virtual void ComponentId(TUid& aComponentUid, TInt& aComponentId, TInt& aVersion) {
		aComponentUid=TUid::Uid(1);
		aComponentId=1;
		aVersion=1;
	}
	virtual const TDesC& Name() { return KComponent1; }
	virtual const TDesC& HumanReadableFunctionality() { return KComponent1; }
	virtual void StopL() {
		CActiveScheduler::Stop();
	}
	virtual void StartL() {
		CALLSTACKITEM_N(_CL("CComponent1"), _CL("StartL"));
		User::Leave(-2);
	}
	virtual void ComponentRunL() {
		User::Panic(_L("SHOULDNT HAPPEN"), 1);
	}
	virtual void ComponentCancel() { }
	~CComponent1() { Cancel(); }
};

_LIT(KDyingComponent, "DyingComponent");
class CDyingComponent : public CComponentBase {
public:
	CDyingComponent() : CComponentBase(KComponent1) { }
	virtual void ComponentId(TUid& aComponentUid, TInt& aComponentId, TInt& aVersion) {
		aComponentUid=TUid::Uid(5);
		aComponentId=1;
		aVersion=1;
	}
	virtual const TDesC& Name() { return KDyingComponent; }
	virtual const TDesC& HumanReadableFunctionality() { return KDyingComponent; }
	virtual void StopL() {
		CActiveScheduler::Stop();
	}
	virtual void StartL() {
		CALLSTACKITEM_N(_CL("CDyingComponent"), _CL("StartL"));
		User::Panic(_L("DIEDIEDIE"), 1);
	}
	virtual void InnerConstructL() {
		MRecovery::TState t=GetState();
		if (t==MRecovery::EDisabled) User::Leave(-3);
	}
	virtual void ComponentRunL() {
		User::Panic(_L("SHOULDNT HAPPEN"), 1);
	}
	virtual void ComponentCancel() { }
	~CDyingComponent() { Cancel(); }
};

class CStop : public CActive {
public:
	void ConstructL() {
		CActiveScheduler::Add(this);
		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrNone);
		SetActive();
	}
	void RunL() {
		CActiveScheduler::Stop();
	}
	void DoCancel() { }
	~CStop() {
		Cancel();
	}
	CStop() : CActive(CActive::EPriorityStandard) { }
};

#include "raii_e32std.h"
#include "bb_recovery.h"

class CComponentTest  {
public:
static void run_tests_innerL()
{
	TNoDefaults nd;
	auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("jaikutest")));
	auto_ptr<CBaActiveScheduler> sched(new (ELeave) CBaActiveScheduler);
	CActiveScheduler::Install(sched.get());

	c->SetDataDir(_L("c:\\system\\data\\context\\"), false);
	c->SetSettings(CBlackBoardSettings::NewL(*c, nd, KCLSettingsTuple));
	auto_ptr<CBBDataFactory> f(CBBDataFactory::NewL());
	c->SetBBDataFactory(f.get());
	auto_ptr<CBBSession> sess(CBBSession::NewL(*c, f.get()));
	c->SetBBSession(sess.get());
	c->SetActiveErrorReporter(output);
	
	{
		auto_ptr<CComponent1> c1( new (ELeave) CComponent1 );
		c1->ConstructL();
		CActiveScheduler::Start();
		TEST_EQUALS( 1, c1->iErrorCount, _L("comp1:1") );
	}
	{
		TLogRecovery r;
		c->SetRecovery(&r);
		auto_ptr<CComponent1> c1( new (ELeave) CComponent1 );
		c1->ConstructL();
		CActiveScheduler::Start();
		TEST_EQUALS( 1, c1->iErrorCount, _L("comp2:1") );
		TEST_EQUALS( (TInt)MRecovery::ERestarting, (TInt)r.iState, _L("recovery:1") );
		CActiveScheduler::Start();
		TEST_EQUALS( 2, c1->iErrorCount, _L("comp2:2") );
		TEST_EQUALS( (TInt)MRecovery::ERestarting, (TInt)r.iState, _L("recovery:2") );
		
		CActiveScheduler::Start();
		TEST_EQUALS( 3, c1->iErrorCount, _L("comp2:3") );
		TEST_EQUALS( (TInt)MRecovery::ERestarting, (TInt)r.iState, _L("recovery:3") );

		CActiveScheduler::Start();
		TEST_EQUALS( 4, c1->iErrorCount, _L("comp2:4") );
		TEST_EQUALS( (TInt)MRecovery::EFailed, (TInt)r.iState, _L("recovery:4") );
		TInt stack_found=r.iErrorInfo->StackTrace().FindF(_L("StartL"));
		TEST_NOT_EQUALS(KErrNotFound, stack_found, _L("stack:1"));
		
		c->SetRecovery(0);
	}
	
	CActiveScheduler::Install(0);
}

static void run_tests_inner2L()
{
	RAChunk stackchunk;
	stackchunk.CreateGlobalLA(_L("jaikutest"), 4096, 4096);
	{
		*(TInt*)stackchunk.Base()=0;
		char* iCallStackZero=(char*)stackchunk.Base() + 4;
		TInt iCallStackSize=stackchunk.Size();
		TUint* uidp=(TUint*)(iCallStackZero+iCallStackSize-16);
		TInt* idp=(TInt*)(iCallStackZero+iCallStackSize-12);
		*uidp=0;
		*idp=0;
	}

	{
		auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("jaikutest")));
		TUid gotuid; TInt gotid;
		c->GetCurrentComponent(gotuid, gotid);
		TEST_EQUALS ( (TInt)gotuid.iUid, 0, _L("recovery2:0:1") );
		TEST_EQUALS ( (TInt)gotid, 0, _L("recovery2:0:2") );
		c->SetCurrentComponent(TUid::Uid(1), 3);
		c->GetCurrentComponent(gotuid, gotid);
		TEST_EQUALS ( (TInt)gotuid.iUid, 1, _L("recovery2:1:1") );
		TEST_EQUALS ( (TInt)gotid, 3, _L("recovery2:1:2") );
	}
	{
		auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("jaikutest")));
		TUid gotuid; TInt gotid;
		c->GetCurrentComponent(gotuid, gotid);
		TEST_EQUALS ( (TInt)gotuid.iUid, 0, _L("recovery2:2:1") );
		TEST_EQUALS ( (TInt)gotid, 0, _L("recovery2:2:2") );
		c->GetInitialComponent(gotuid, gotid);
		TEST_EQUALS ( (TInt)gotuid.iUid, 1, _L("recovery2:3:1") );
		TEST_EQUALS ( (TInt)gotid, 3, _L("recovery2:3:2") );
	}
}
static void run_tests_inner3L()
{
	TNoDefaults nd;
	auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("jaikutest")));
	auto_ptr<CBaActiveScheduler> sched(new (ELeave) CBaActiveScheduler);
	CActiveScheduler::Install(sched.get());

	c->SetDataDir(_L("c:\\system\\data\\context\\"), false);
	c->SetSettings(CBlackBoardSettings::NewL(*c, nd, KCLSettingsTuple));
	auto_ptr<CBBDataFactory> f(CBBDataFactory::NewL());
	c->SetBBDataFactory(f.get());
	auto_ptr<CBBSession> sess(CBBSession::NewL(*c, f.get()));
	c->SetBBSession(sess.get());
	c->SetActiveErrorReporter(output);
	
	c->BBSession()->DeleteL(KComponentVersionTuple, KNullDesC, ETrue);
	c->BBSession()->DeleteL(KComponentFunctionalityTuple, KNullDesC, ETrue);
	c->BBSession()->DeleteL(KComponentNameTuple, KNullDesC, ETrue);
	c->BBSession()->DeleteL(KComponentStateTuple, KNullDesC, ETrue);
	c->BBSession()->DeleteL(KComponentErrorCountTuple, KNullDesC, ETrue);
	c->BBSession()->DeleteL(KComponentErrorInfoTuple, KNullDesC, ETrue);
	
	auto_ptr<CBBRecovery> rec(CBBRecovery::NewL());
	MRecovery::TState state=rec->GetState( TUid::Uid(1), 4 );
	TEST_EQUALS( (TInt)state, (TInt)MRecovery::EUnknown, _L("recovery3:0:1"));
	rec->SetState(TUid::Uid(1), 4, MRecovery::ERunning );
	state=rec->GetState( TUid::Uid(1), 4 );
	TEST_EQUALS( (TInt)state, (TInt)MRecovery::ERunning, _L("recovery3:0:2"));
	
	rec->SetState(TUid::Uid(1), 4, MRecovery::EFailed);
	state=rec->GetState( TUid::Uid(1), 4 );
	TEST_EQUALS( (TInt)state, (TInt)MRecovery::EFailed, _L("recovery3:0:3"));
	TInt errors=rec->GetErrorCount( TUid::Uid(1), 4 );
	TEST_EQUALS( errors, 1, _L("recovery3:0:4"));
}

static void run_thread_4L() {
	auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("jaikutest2")));
	auto_ptr<CBaActiveScheduler> sched(new (ELeave) CBaActiveScheduler);
	CActiveScheduler::Install(sched.get());

	c->SetDataDir(_L("c:\\system\\data\\context\\"), false);
	auto_ptr<CBBDataFactory> f(CBBDataFactory::NewL());
	c->SetBBDataFactory(f.get());
	auto_ptr<CBBSession> sess(CBBSession::NewL(*c, f.get()));
	c->SetBBSession(sess.get());
	auto_ptr<CBBRecovery> rec(CBBRecovery::NewL());
	c->SetRecovery(rec.get());
	
	auto_ptr<CDyingComponent> d(new (ELeave) CDyingComponent);
	d->ConstructL();
	CActiveScheduler::Start();
}

static TInt run_thread_4(TAny*) {
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	CC_TRAPD(err, run_thread_4L());
	delete cleanupStack;
	return 0;
}

static void run_tests_inner4L()
{
	RAChunk stackchunk;
	stackchunk.CreateGlobalLA(_L("jaikutest2"), 4096, 4096);
	{
		*(TInt*)stackchunk.Base()=0;
		char* iCallStackZero=(char*)stackchunk.Base() + 4;
		TInt iCallStackSize=stackchunk.Size();
		TUint* uidp=(TUint*)(iCallStackZero+iCallStackSize-16);
		TInt* idp=(TInt*)(iCallStackZero+iCallStackSize-12);
		*uidp=0;
		*idp=0;
	}
	auto_ptr<CApp_context> c(CApp_context::NewL(true, _L("jaikutest")));
	auto_ptr<CBaActiveScheduler> sched(new (ELeave) CBaActiveScheduler);
	CActiveScheduler::Install(sched.get());

	TNoDefaults nd;
	c->SetDataDir(_L("c:\\system\\data\\context\\"), false);
	c->SetSettings(CBlackBoardSettings::NewL(*c, nd, KCLSettingsTuple));
	auto_ptr<CBBDataFactory> f(CBBDataFactory::NewL());
	c->SetBBDataFactory(f.get());
	auto_ptr<CBBSession> sess(CBBSession::NewL(*c, f.get()));
	c->SetBBSession(sess.get());
	auto_ptr<CBBRecovery> rec(CBBRecovery::NewL());
	c->SetRecovery(rec.get());
	
	MRecovery::TState state=MRecovery::EUnknown;
	while (state!=MRecovery::EDisabled) {
		RAThread thread;
		thread.CreateLA(_L("jaikutest3"),
				&run_thread_4,
				20*1024,
				128*1024,
				32*1048*1024,
				0);
		thread.SetPriority(EPriorityNormal);
		TRequestStatus s;
		thread.Logon(s);
		thread.Resume();
		User::WaitForRequest(s);
	
		state=rec->GetState( TUid::Uid(5), 1 );
	}
	TEST_EQUALS( (TInt)state, (TInt)MRecovery::EDisabled, _L("recovery4:0:1"));
}

};

class CStackTest : public CBase, public MContextBase {
public:
	void Leave() {
		CALLSTACKITEM_N(_CL("CStackTest"), _CL("Leave"));
		User::Leave(-1);
	}
	void CallLeaving() {
		CALLSTACKITEM_N(_CL("CStackTest"), _CL("CallLeaving"));
		CC_TRAPD(ignored, Leave);
	}
	~CStackTest() {
		CALLSTACKITEM_N(_CL("CStackTest"), _CL("~CStackTest"));
		CallLeaving();
	}
	static void run_destroyL() {
		CALLSTACKITEMSTATIC_N(_CL("CStackTest"), _CL("run_destroyL"));
		auto_ptr<CStackTest> o(new (ELeave) CStackTest);
		User::Leave(-2);
	}
	static void run_tests_innerL() {
		CALLSTACKITEMSTATIC_N(_CL("CStackTest"), _CL("CallLeaving"));
		TRAPD(err, run_destroyL());
		TEST_EQUALS( err, -2, _L("stacktest"));
	}
};


class MRunBasic : public MRunnable {
	void run() { CJaikuTest::run_tests_innerL(); }
};

void LeavingL() {
	TBuf<1> b;
	User::Leave(-2);
}

IMPORT_C void AllocateContextCommonExceptionData();

void run_testsL()
{
#if defined(__S60V3__) && defined(__WINS__)
	class RAllocator* iOriginal=SwitchToLoggingAllocator();
#endif
	{
		// force allocation of exception support before
		// memory leak checking
		AllocateContextCommonExceptionData();
		TRAPD(dummy, { TBuf<1> b; User::Leave(-1); });
		char* c=(char*)User::Alloc(16*1024);
		User::Free(c);
	}
	User::__DbgMarkStart(RHeap::EUser);
	{
		RFs fs;
		User::LeaveIfError(fs.Connect());
		output=new (ELeave) MOutput;
		output->cons=Console::NewL(_L("test"),TSize(KConsFullScreen, KConsFullScreen));
		User::LeaveIfError(
			output->foutput.Replace(fs, _L("c:\\locatest.txt"), EFileWrite));
		output->foutput.Write( _L8("\xff\xfe") );

		//CJaikuTest::run_tests_innerL();
		//MRunBasic r;
		//test_oom(r);
		//CComponentTest::run_tests_inner4L();
		//CComponentTest::run_tests_inner3L();
		//CComponentTest::run_tests_inner2L();
		//CComponentTest::run_tests_innerL();
		CStackTest::run_tests_innerL();

		TBuf<30> b=_L("OK: "); b.AppendNum(ok); 
		b.Append(_L("/")); b.AppendNum(ok+not_ok); b.Append(_L("\n"));
		output->Write(b);
		output->Getch();
		delete output->cons;
		output->foutput.Close();
		delete output;
		fs.Close();
	}
	User::__DbgMarkEnd(RHeap::EUser,0);
#if defined(__S60V3__) && defined(__WINS__)
	if (iOriginal) SwitchBackAllocator(iOriginal);
#endif
}

TInt run_test_thread(TAny*)
{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	CC_TRAPD(err, run_testsL());
	delete cleanupStack;
	return 0;
}
void run_tests()
{
	User::After(15000000); // pause to allow for ws start
	RThread thread;
	TInt err=thread.Create(_L("jaikutest2"),
			&run_test_thread,
			20*1024,
			128*1024,
			32*1048*1024,
			0);
	if (err==KErrNone) {
		thread.SetPriority(EPriorityNormal);
		TRequestStatus s;
		thread.Logon(s);
		thread.Resume();
		User::WaitForRequest(s);
	}	
	//User::After(350000000); // pause to allow you to see any panic notes
}
