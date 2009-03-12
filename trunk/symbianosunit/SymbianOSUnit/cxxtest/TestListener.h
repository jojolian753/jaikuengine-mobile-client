/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/
#ifndef __CXXTEST__TESTLISTENER_H
#define __CXXTEST__TESTLISTENER_H

//
// TestListener is the base class for all "listeners",
// i.e. classes that receive notifications of the
// testing process.
//

//Changed by Penrillian Ltd 14 August 2002
//Signature of failedAssertDiffers changed.
//Also, arguments all commented out to avoid compiler warnings 
#include <cxxtest/Descriptions.h>

namespace CxxTest
{
    class TestListener
    {
    public:
        virtual TestListener *listener() { return this; }
        
	virtual void enterWorld( const WorldDescription& /*desc*/, signed int /*aSuite*/ ) {}
	virtual void enterSuite( const SuiteDescription& /*desc*/ ) {}
	virtual void enterTest( const TestDescription& /*desc*/ ) {}
        virtual void failedTest( const char* /*suiteName*/, const char* /*file*/, unsigned /*line*/,
                                 const char* /*expression*/ ) {}
	virtual void failedAssert( const char* /*suiteName*/, const char* /*file*/, unsigned /*line*/,
				   const char* /*expression*/ ) {}
	virtual void failedAssertThrows( const char* /*suiteName*/, const char* /*file*/, unsigned /*line*/,
                                         const char* /*expression*/, const char* /*type*/,
                                         bool /*otherThrown*/ ) {}
	virtual void failedAssertEquals( const char* /*suiteName*/, const char* /*file*/, unsigned /*line*/,
					 const char* /*xStr*/, const char* /*yStr*/,
					 const char* /*x*/, const char* /*y*/ ) {}
	virtual void failedAssertDelta( const char* /*suiteName*/, const char* /*file*/, unsigned /*line*/,
                                        const char* /*xStr*/, const char* /*yStr*/, const char* /*dStr*/,
                                        const char* /*x*/, const char* /*y*/, const char* /*d*/ ) {}
	virtual void failedAssertDiffers( const char* /*suiteName*/, const char* /*file*/, unsigned /*line*/,
                                          const char* /*xStr*/, const char* /*yStr*/,
                                          const char* /*x*/,const char* /*y*/ ) {}
        virtual void failedExpectation( const char* /*suiteName*/, const char* /*file*/, unsigned /*line*/,
                                        const char* /*expected*/, const char* /*found*/ ) {}
	virtual void leaveTest( const TestDescription& /*desc*/ ) {}
	virtual void leaveSuite( const SuiteDescription& /*desc*/ ) {}
	virtual void leaveWorld( const WorldDescription& /*desc*/, signed int /*aSuite*/ ) {}

	virtual ~TestListener() {}
    };
}

#endif // __CXXTEST__TESTLISTENER_H
