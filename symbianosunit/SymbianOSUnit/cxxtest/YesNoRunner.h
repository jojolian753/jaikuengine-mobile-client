#ifndef __CXXTEST__YESNORUNNER_H
#define __CXXTEST__YESNORUNNER_H

//
// The YesNoRunner is a simple TestListener that
// just returns true iff all tests passed.
//

//Changed by Penrillian Ltd 14 August 2002
//Make explicit reference to base class ctor to avoid lint warning

#include <cxxtest/TestRunner.h>
#include <cxxtest/TestListener.h>
#include <cxxtest/CountingListenerProxy.h>

namespace CxxTest 
{
    class YesNoRunner : public TestListener
    {
	CountingListenerProxy _counter;
	
    public:        
	static bool runAllTests( void )
	{
	    YesNoRunner ynr;
	    TestRunner::runAllTests( &ynr );
	    return ynr.allTestsPassed();
	}

        YesNoRunner() :TestListener(), _counter(this) {}
        TestListener *listener( void ) { return &_counter; }        
        
	bool allTestsPassed( void )
	{
	    return _counter.failedTestsNo() == 0;
	}
    };
}

#endif // __CXXTEST__YESNORUNNER_H
