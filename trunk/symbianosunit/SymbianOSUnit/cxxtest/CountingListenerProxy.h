#ifndef __COUNTINGLISTENERPROXY_H
#define __COUNTINGLISTENERPROXY_H

//
// The CountingListenerProxy helps writing TestListener's by
// taking care of the "dirty" work.
// See YesNoRunner and ErrorPrinter for examples.
//

//Changed by Penrillian Ltd 14 August 2002
//failedAssertDiffers now prints both values compared

#include <cxxtest/TestListenerProxy.h>

namespace CxxTest
{
    class CountingListenerProxy : public TestListenerProxy
    {
        bool _testFailed, _suiteFailed, _worldFailed;
        unsigned _failedTestsNo;

        void failed()
        {
            _testFailed = _suiteFailed = _worldFailed = true;
        }
        
    public:
        CountingListenerProxy( TestListener *l ) : TestListenerProxy(l) {}

        bool testFailed() const { return _testFailed; }
        bool suiteFailed() const { return _suiteFailed; }
        bool worldFailed() const { return _worldFailed; }
        unsigned failedTestsNo() const { return _failedTestsNo; }

	void enterWorld( const WorldDescription &wd, TInt aSuite )
        {
            _worldFailed = false;
            _failedTestsNo = 0;
            TestListenerProxy::enterWorld(wd, aSuite);
        }

        void enterSuite( const SuiteDescription &sd )
        {
            _suiteFailed = false;
            TestListenerProxy::enterSuite(sd);
        }
        
	void enterTest( const TestDescription &td )
        {
            _testFailed = false;
            TestListenerProxy::enterTest(td);
        }

        void leaveTest( const TestDescription &td )
        {
            if ( _testFailed )
                ++ _failedTestsNo;
            TestListenerProxy::leaveTest(td);
        }

        void failedTest( const char *suiteName, const char *file, unsigned line, const char *expression )
        {
            failed();
            TestListenerProxy::failedTest( suiteName, file, line, expression );
        }
        
	void failedAssert( const char *suiteName, const char *file, unsigned line, const char *expression )
        {
            failed();
            TestListenerProxy::failedAssert( suiteName, file, line, expression );
        }

	void failedAssertThrows( const char *suiteName, const char *file, unsigned line, const char *expression,
                                 const char *type, bool otherThrown )
        {
            failed();
            TestListenerProxy::failedAssertThrows( suiteName, file, line, expression, type, otherThrown );
        }
        
	void failedAssertEquals( const char *suiteName, const char *file, unsigned line,
                                 const char *xStr, const char *yStr,
                                 const char *x, const char *y )
        {
            failed();
            TestListenerProxy::failedAssertEquals( suiteName, file, line, xStr, yStr, x, y );
        }

	void failedAssertDelta( const char *suiteName, const char *file, unsigned line,
                                const char *xStr, const char *yStr, const char *dStr,
                                const char *x, const char *y, const char *d )
        {
            failed();
            TestListenerProxy::failedAssertDelta( suiteName, file, line, xStr, yStr, dStr, x, y, d );
        }
	void failedAssertDiffers( const char *suiteName, const char *file, unsigned line,
                                  const char *xStr, const char *yStr,
                                  const char *x,const char *y )
        {
            failed();
            TestListenerProxy::failedAssertDiffers( suiteName, file, line, xStr, yStr, x,y );
        }
        
        void failedExpectation( const char *suiteName, const char *file, unsigned line,
                                const char *expected, const char *found )
        {
            failed();
            TestListenerProxy::failedExpectation( suiteName, file, line, expected, found );
        }
    };
}


#endif // __COUNTINGLISTENERPROXY_H
