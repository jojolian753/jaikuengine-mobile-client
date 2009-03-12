#ifndef __CXXTEST_TESTRUNNER_H
#define __CXXTEST_TESTRUNNER_H

//
// TestRunner is the class that runs all the tests.
// To use it, create an object that implements the TestListener
// interface and call TestRunner::runAllTests( &myListener );
// 

//Changed by Penrillian Ltd 14 August 2002
//Arguments names cahnged to avoid compiler warnings 
//Use of _TS_TRY and _TS_CATCH changed to reflect implementation using leave
 
#include <cxxtest/Runnable.h>
#include <cxxtest/TestListenerProxy.h>
#include <cxxtest/Descriptions.h>
#include <cxxtest/TestSuite.h>
#include "Logger.h"
#include "break.h"
#include "app_context.h"

const TInt KRunAllSuites = -1;

namespace CxxTest 
{
    class TestRunner : public TestListenerProxy
    {
    public:
        TestRunner( TestListener *l, MUiUpdater * aUI, RHeap *aTestHeap	 ) : 
        	TestListenerProxy( l->listener(),aUI ), iTestHeap(aTestHeap) {}

        static void runAllTests( TestListener *alistener, WorldDescription & world, 
        	MUiUpdater * aUI, TInt aSuite, RHeap *aTestHeap )
        {
            runWorld( alistener->listener(), world, aUI, aSuite, aTestHeap );
        }

        static void runWorld( TestListener *alistener, const WorldDescription &world, MUiUpdater * aUI, 
        	TInt aSuite, RHeap *aTestHeap  )
        {
            TestRunner( alistener->listener(),aUI, aTestHeap ).runWorld( world, aSuite );
        }
    
    private:
    	RHeap* iTestHeap;
        
        void runWorld( const WorldDescription &wd, TInt aSuite )
        {
            enterWorld( wd, aSuite );
            wd.setUp( this );
			if(aSuite != KRunAllSuites && aSuite < wd.numSuites())
			{
				checkAndRunSuite(wd, aSuite);
			}
			else
			{
				for ( TInt i = 0; i < wd.numSuites(); ++ i )
				{
					checkAndRunSuite(wd, i);
				}
			}
            wd.tearDown( this );
            leaveWorld( wd, aSuite );
        }

		void checkAndRunSuite(const WorldDescription &wd, TInt aSuite)
		{
			if ( wd.suiteDescription(aSuite).suite )
                    runSuite( wd.suiteDescription(aSuite) );
		}
    
        void runSuite( const SuiteDescription &sd )
        {
        	// force allocation of exception support before memory leak tracking
        	TRAPD(dummy, User::Leave(1));
        	
            enterSuite( sd );
            sd.suite->setListener( this );
			iContinue = true;
            for ( unsigned i = 0; i < sd.numTests && iContinue; ++ i ) {
				TInt err;
				TRAP(err, sd.suite->setUp());
				sd.suite->tearDown();

				RThread me;				
				TInt handles_at_start, handles_at_end, dummy;
				me.HandleCount(dummy, handles_at_start);

				TInt allocatedHeapCells = User::Heap().Count();
				TInt lostCells=0;
				const char* testName = sd.tests[i].name;
				//__UHEAP_MARK;
				{
				
					Logger::Log( "", testName );
					Logger::Log( "********** Test: %s **********", testName );
					{
		                CC_TRAP(err, 
		                	{
		                	sd.suite->setUp();
		                	});
		                if (err==KErrNone) {
		                	CC_TRAP(err, {
		                		runTest( sd.tests[i] );
		                	});
		                }
		                {
		                	sd.suite->tearDown();
		                }
	                	
		                }
				}
				//__UHEAP_MARKEND;
				
				me.HandleCount(dummy, handles_at_end);
				if (handles_at_end != handles_at_start) {
					User::Panic(_L("UT Lost handles"), handles_at_end-handles_at_start);
				}
				//lostCells = User::Heap().Count() - allocatedHeapCells;
				//User::Heap().__DbgMarkCheck(ETrue, allocatedHeapCells, 
				//	(const unsigned char*)__FILE__, __LINE__);
				if (lostCells != 0 && lostCells != sd.suite->iMemoryLeakCells )
				{
					failedTest(sd.tests[i].suite, testName, lostCells, "Memory Leak");
				}
				if (err!=KErrNone) {
					failedTest(sd.tests[i].suite, testName, err, "Leave");
				}
				testCompleted();
            }
            leaveSuite( sd );
        }

        void runTest( const TestDescription &td )
        {
            enterTest( td );
			TRAPD(err, {
				td.test->run(); 
			});

			if (err == -1003)  // EPOC Exit application.
			{
				iContinue = false;
			}
			else if(err != 0)
			{ 
				TBuf8<30> result;
				result.Format( _L8( "Left with code %d" ), err );
				result.ZeroTerminate();
				failedTest(td.suite, td.name,td.line,(const char*)result.Ptr());
			} 

            leaveTest( td );
        }
		bool iContinue;
    };
}


#endif // __CXXTEST_TESTRUNNER_H
