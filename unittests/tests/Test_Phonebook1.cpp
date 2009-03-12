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

#include "Test_Phonebook1.h"

#include "utcontactengine.h"

#include "symbian_auto_ptr.h"

#include <badesca.h>
#include <cntdb.h>
#include <cpbkcontactitem.h>
#include <pbkfields.hrh>
#include <tpbkcontactitemfield.h>
#include "phonebook.h"
#include "testutils.h"
#include "app_context_impl.h"

enum { EFirst = 0, ELast, EPhoneNumber };

#define NO_OF_CONTACTS_TO_CREATE_1	100
#define NO_OF_CONTACTS_TO_CREATE_2	100
#define NO_OF_CONTACTS_NOT_DELETED	180

void DoChangesL()
{
	auto_ptr<CApp_context> appc(0);
	appc.reset(CApp_context::NewL(false, _L("unittest_changes")));
	auto_ptr<CUTContactEngine> iEngine(CUTContactEngine::NewL());

	auto_ptr<CContactIdArray> to_remove( CContactIdArray::NewL() );
	
	TBuf<50> first, last, phoneno;
	int i;
	for (i=0; i<NO_OF_CONTACTS_TO_CREATE_1; i++) {
	
		first=_L("f"); first.AppendNum(i);
		last=_L("l"); last.AppendNum(i);
		phoneno=_L("050"); phoneno.AppendNum(i);
		TContactItemId id=iEngine->StoreContactL(first, last, phoneno);
		if (i % 10 == 0) {
			to_remove->AddL(id);
		}
		User::After( 50 );
	}
	for (i=NO_OF_CONTACTS_TO_CREATE_1; i<NO_OF_CONTACTS_TO_CREATE_1+NO_OF_CONTACTS_TO_CREATE_2; i++) {
		first=_L("f"); first.AppendNum(i);
		last=_L("l"); last.AppendNum(i);
		phoneno=_L("050"); phoneno.AppendNum(i);
		TContactItemId id=iEngine->StoreContactL(first, last, phoneno);
		if (i % 10 == 0) {
			to_remove->AddL(id);
		}
		User::After( 50*1000 );
	}
	iEngine->PbkEngine().DeleteContactsL( *to_remove );
}

#define CONTACTS_CREATED_1 100
#define CONTACTS_CREATED_2 200

void DoChanges2L()
{
	auto_ptr<CApp_context> appc(0);
	appc.reset(CApp_context::NewL(false, _L("unittest_changes")));
	auto_ptr<CUTContactEngine> iEngine(CUTContactEngine::NewL());

	auto_ptr<CContactIdArray> to_remove( CContactIdArray::NewL() );
	
	// Add and mark all to be removed
	TBuf<50> first, last, phoneno;
	int i;
	for (i=0; i<CONTACTS_CREATED_1; i++) {
	
		first=_L("f"); first.AppendNum(i);
		last=_L("l"); last.AppendNum(i);
		phoneno=_L("050"); phoneno.AppendNum(i);
		TContactItemId id=iEngine->StoreContactL(first, last, phoneno);
		to_remove->AddL(id);
		User::After( 50 );
	}
	
	// Add and remove 
	for (i=CONTACTS_CREATED_1; i<CONTACTS_CREATED_1 + CONTACTS_CREATED_2; i++) {
		first=_L("f"); first.AppendNum(i);
		last=_L("l"); last.AppendNum(i);
		phoneno=_L("050"); phoneno.AppendNum(i);
		TContactItemId id=iEngine->StoreContactL(first, last, phoneno);
		
		TInt toBeRemoved = to_remove->Count();
		if (toBeRemoved > 0)
			{
				
				TInt lastIx = toBeRemoved - 1; 
				TContactItemId id = (*to_remove)[lastIx];
				to_remove->Remove(lastIx);
				iEngine->PbkEngine().DeleteContactL(id); // Remove last
			}
		User::After( 50 );
	}
}

TInt ChangeThread(void*)
{
	CleanupStack* cl=new CleanupStack;
	TRAPD(err, DoChangesL());
	return KErrNone;
}


class CStopActive : public CActive {
public:
	CStopActive(RThread& aThread) : iThread(aThread), CActive(EPriorityNormal) { }
	RThread& iThread;
	void ConstructL() { 
		CActiveScheduler::Add(this);
		iThread.Logon(iStatus);
		SetActive();
	}
	void RunL() {
		CActiveScheduler::Stop();
	}
	void DoCancel() {
		iThread.LogonCancel(iStatus);
	}
	~CStopActive() {
		Cancel();
	}
};

#include "raii_e32std.h"



void CTest_Phonebook1::testLargeChanges()
{
  auto_ptr<phonebook> pb1(new (ELeave) phonebook(AppContext(), 0, 0));
  pb1->ConstructL();
  auto_ptr<CStopActive> stop(0);
  
  {
	RThread runner;
  	TestUtils::RunInSeparateThreadL(ChangeThread, runner, TestUtils::ENoWaitForExit);
  	CleanupClosePushL(runner);
  	stop.reset( new (ELeave) CStopActive(runner) );
  	stop->ConstructL();
  	runner.Resume();
    
  	// first wait for the thread to finish its work
#ifdef __WINS__
  	TS_ASSERT(TestUtils::WaitForActiveSchedulerStopL(15));
#else
  	TS_ASSERT(TestUtils::WaitForActiveSchedulerStopL(45));
#endif
  	CleanupStack::PopAndDestroy();
  }
  
  // then let the phonebook pick up the changes
  TestUtils::WaitForActiveSchedulerStopL(4);
  auto_ptr<phonebook> pb2(new (ELeave) phonebook(AppContext(), 0, 0));
  pb2->ConstructL();
  TS_ASSERT_EQUALS(pb1->Count(), NO_OF_CONTACTS_NOT_DELETED);
  TS_ASSERT(TestEqualPhoneBooks(pb1.get(), pb2.get()));
}


TInt ChangeThread2(void*)
{
	CleanupStack* cl=new CleanupStack;
	TRAPD(err, DoChanges2L());
	return KErrNone;
}

void CTest_Phonebook1::testLargeChanges2()
{
  auto_ptr<phonebook> pb1(new (ELeave) phonebook(AppContext(), 0, 0));
  pb1->ConstructL();
  auto_ptr<CStopActive> stop(0);
  
  {
	RThread runner;
  	TestUtils::RunInSeparateThreadL(ChangeThread2, runner, TestUtils::ENoWaitForExit);
  	CleanupClosePushL(runner);
  	stop.reset( new (ELeave) CStopActive(runner) );
  	stop->ConstructL();
  	runner.Resume();
    
  	// first wait for the thread to finish its work
  	TestUtils::WaitForActiveSchedulerStopL(10);
  	CleanupStack::PopAndDestroy();
  }
  
  // then let the phonebook pick up the changes
  TestUtils::WaitForActiveSchedulerStopL(2);
  auto_ptr<phonebook> pb2(new (ELeave) phonebook(AppContext(), 0, 0));
  pb2->ConstructL();
  TS_ASSERT(TestEqualPhoneBooks(pb1.get(), pb2.get()));
}



void CTest_Phonebook1::setUp()
{
  iEngine = 0;
  MContextTestBase::setUp();

  iEngine = CUTContactEngine::NewL();
  iEngine->RemoveAllContactsL();
}


void CTest_Phonebook1::tearDown()
{
  delete iEngine; iEngine=0;

  MContextTestBase::tearDown();
}


CPbkContactEngine& CTest_Phonebook1::PbkEngine()
{
  return iEngine->PbkEngine();
}


CContactDatabase& CTest_Phonebook1::CntDb()
{
  return PbkEngine().Database();
}
