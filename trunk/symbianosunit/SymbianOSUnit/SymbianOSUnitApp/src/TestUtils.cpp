/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/
#include "TestUtils.h"

#include <e32math.h>
#include <basched.h>
/** 
	Utility functions for testing.
*/
namespace TestUtils
{
	
/** Callback for WaitForActiveSchedulerStopL */
static TInt StopTimer( void* aPointer )
{
	*(static_cast<TBool*>(aPointer)) = ETrue;
	CActiveScheduler::Stop();
	return 0;
}

/**
Wait up to aTimeoutSeconds for a CActiveScheduler::Stop call.
Answer ETrue if found.
*/
TBool WaitForActiveSchedulerStopL( TInt aTimeoutSeconds )
{   // All other events have priority, so debugging works.
	CPeriodic* timer = CPeriodic::NewL( EPriorityNull );
	CleanupStack::PushL(timer);
	
	TBool timedOut = EFalse;
	timer->Start( aTimeoutSeconds * 1000000, 10000000, TCallBack( StopTimer, &timedOut) );
	CActiveScheduler::Start();
	CleanupStack::PopAndDestroy(); // timer
	return !timedOut;
}

static const TUint KServerMinHeapSize =  0x1000;  //  4K
static const TUint KServerMaxHeapSize = 0x1000000; 
_LIT( KSemaphoreName, "TestThreadSem" );
_LIT( KThreadName, "SOSUnitExtra" );


static TInt ThreadEntryFunction( TAny* aParameter )
{
	TCallBack* callBack = static_cast<TCallBack*>( aParameter );
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	ASSERT( cleanupStack );
	CActiveScheduler::Install( new CBaActiveScheduler );
    TRAPD(err, (callBack->iFunction)( callBack->iPtr ));
    ASSERT(err == KErrNone);
	delete CActiveScheduler::Current();
    delete cleanupStack;
	return 0;
}

void RunInSeparateThreadL( const TCallBack& aFunctionToRun, RThread& testThread,
	TThreadWaitOption aWaitForExit)
{

	TName threadName(KThreadName);
	// Append a random number to make it unique
	threadName.AppendNum(Math::Random(), EHex);
	User::LeaveIfError( testThread.Create(threadName,   // create new testThread thread
							 ThreadEntryFunction, // thread's main function
							 32768,
							 16*1024,
							 1024*1024,
							 const_cast<TCallBack*>(&aFunctionToRun),
							 EOwnerProcess) );

	// WARNING - Thread isn't closed if not wait for exit.

	if (aWaitForExit == EWaitForExit )
	{
		CleanupClosePushL( testThread );
		// Wait for thread to finish:
		RUndertaker undertaker;		
		User::LeaveIfError( undertaker.Create() );
		CleanupClosePushL( undertaker );
		TRequestStatus rs;
		TInt handleOfDeadThread;
		User::LeaveIfError( undertaker.Logon( rs, handleOfDeadThread ) );
		testThread.Resume();
		TName deadThreadName;
		do 
		{
			User::WaitForRequest( rs ); // Thread died.
			RThread deadThread;
			deadThread.SetHandle( handleOfDeadThread );
			deadThreadName = deadThread.Name();
			deadThread.Close();
		}
		while ( deadThreadName != threadName );
		CleanupStack::PopAndDestroy( 2 ); // undertaker, testThread.
	}
}


} // namespace TestUtils