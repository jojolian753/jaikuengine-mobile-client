/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/
#ifndef __CXXTEST__TESTSUITE_H
#define __CXXTEST__TESTSUITE_H

//
// class TestSuite is the base class for all test suites.
// To define a test suite, derive from this class and add
// member functions called void test*();
//

//Changed by Penrillian Ltd 14 August 2002
//Uses leaves and traps, rather than exception handling

#include <cxxtest/Runnable.h>
#include <cxxtest/TestListenerProxy.h>
#include <cxxtest/Descriptions.h>
#include <cxxtest/ValueTraits.h>
#include <e32std.h>

namespace CxxTest 
{
    class TestSuite : public TestListenerProxy
    {
    public:
		TBufC8<100> iSuiteName;
        TestSuite(const TDesC8& aSuiteName) : TestListenerProxy( 0 ), iSuiteName(aSuiteName), iMemoryLeakCells( 0 ) {}
		static TBuf8<30> ErrorReturn( TInt aError )
		{
			TBuf8<30> result;
			result.Format( _L8( "Left with code %d" ), aError );
			result.ZeroTerminate();
			return result;
		}
   
    protected:
	TInt iMemoryLeakCells;
	friend class TestRunner;
	virtual void setUp() {}
	virtual void tearDown() {}

	void AllowForMemoryLeak( TInt aCellsLeaked ) { iMemoryLeakCells += aCellsLeaked; }

#define CONVERT_DES_TO_CHAR_PTR(x)	(const char*)x.Des().PtrZ()

#       define TS_FAIL(e) failedTest(CONVERT_DES_TO_CHAR_PTR(iSuiteName), __FILE__, __LINE__, TS_AS_STRING(e) );return;

#       ifdef CXXTEST_HAVE_EH
#           define _TS_TRY TRAPD
#           define __TS_CATCH(e,f,l,m) if( e != KErrNone) { failedTest(CONVERT_DES_TO_CHAR_PTR(iSuiteName),f,l,((char*)CxxTest::TestSuite::ErrorReturn(e).Ptr())); return;}
#           define _TS_CATCH(e) __TS_CATCH(e,__FILE__,__LINE__,"")
#       else // !CXXTEST_HAVE_EH
#           define _TS_TRY
#           define __TS_CATCH(f,l,m)
#           define _TS_CATCH
#       endif // CXXTEST_HAVE_EH
        
#       define ETS_ASSERT(e)  if ( !(e) ) { failedAssert(CONVERT_DES_TO_CHAR_PTR(iSuiteName), __FILE__, __LINE__, #e);  }
        
#       define ETS_ASSERT_EQUALS(x,y) { if ( !((x) == (y)) ) \
	    failedAssertEquals(CONVERT_DES_TO_CHAR_PTR(iSuiteName), __FILE__, __LINE__, #x, #y, TS_AS_STRING(x), TS_AS_STRING(y)); }
        
#       define ETS_ASSERT_DELTA(x,y,d) { if ( ((y) < (x) - (d)) || ((y) > (x) + (d)) ) \
	    failedAssertDelta(CONVERT_DES_TO_CHAR_PTR(iSuiteName), __FILE__, __LINE__, #x, #y, #d, \
                              TS_AS_STRING(x), TS_AS_STRING(y), TS_AS_STRING(d) ); }
        
#       define ETS_ASSERT_DIFFERS(x,y) { if ( (x) == (y) ) \
	    failedAssertDiffers(CONVERT_DES_TO_CHAR_PTR(iSuiteName), __FILE__, __LINE__, #x, #y, TS_AS_STRING(x),TS_AS_STRING(y)); }

#       ifdef CXXTEST_HAVE_EH
			#define TS_ASSERT(e) {TRAPD(err, ETS_ASSERT(e)); _TS_CATCH(err); }
	        #define TS_ASSERT_EQUALS(x,y) { TRAPD (err,ETS_ASSERT_EQUALS(x,y)); _TS_CATCH(err); }
	        #define TS_ASSERT_DELTA(x,y,d) { TRAPD (err,ETS_ASSERT_DELTA(x,y,d)); _TS_CATCH(err);}
			#define TS_ASSERT_DIFFERS(x,y) {TRAPD (err,ETS_ASSERT_DIFFERS(x,y)); _TS_CATCH(err); }
	    #else
			#define TS_ASSERT(e)			ETS_ASSERT(e)
			#define TS_ASSERT_EQUALS(x,y)   ETS_ASSERT_EQUALS(x,y)
			#define TS_ASSERT_EQUALS_DESCRIPTOR( x, y ) \
				{ if ( !((x) == (y)) ) \
				failedAssertEquals(CONVERT_DES_TO_CHAR_PTR(iSuiteName), __FILE__, __LINE__, #x, #y, (const char*)CxxTest::ETS::DescriptorAsBuf8(x).Ptr(), (const char*)CxxTest::ETS::DescriptorAsBuf8(y).Ptr()); }
			#define TS_ASSERT_DELTA(x,y,d)  ETS_ASSERT_DELTA(x,y,d)
			#define TS_ASSERT_DIFFERS(x,y)  ETS_ASSERT_DIFFERS(x,y)
#       endif // CXXTEST_HAVE_EH


        
#       define TS_ASSERT_THROWS(e,t) { \
            int _ts_threw = -1; \
            TRAPD(err, e); \
            if(err == t ) { _ts_threw = 0; } else \
            if(err != 0 ) { _ts_threw = -2; } \
            if ( _ts_threw ) \
                failedAssertThrows(CONVERT_DES_TO_CHAR_PTR(iSuiteName), __FILE__, __LINE__, #e, #t, (_ts_threw == -2) ); }
        
#       define TS_ASSERT_THROWS_ANYTHING(e) { \
            int _ts_threw = -1; \
            TRAPD(err, e); \
            if(err != 0 ) { _ts_threw = 0; } \
            if ( _ts_threw ) \
                failedAssertThrows(CONVERT_DES_TO_CHAR_PTR(iSuiteName), __FILE__, __LINE__, #e, "...", false ); }
    };    
}



#endif // __CXXTEST__TESTSUITE_H
