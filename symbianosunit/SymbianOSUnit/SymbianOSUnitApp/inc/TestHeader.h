/*Copyright (c) Penrillian Ltd 2003*/
#include "TestSuite.h"

/** 
Encapsulates all the tests.  
Any member function starting 'test' is interpreted as a test to be executed.
The tests will be executed in the order they are in this file.
Note that the code for the individual tests may be in different CPP files.
*/
/*************************************************************
* Each test line should contain either __FAST_TEST__ or __SLOW_TEST__
* followed by ;
* At runtime you can choose to run just the fast tests or all
* tests, including slow tests.
*************************************************************/

class CTest : public CxxTest::TestSuite
{	
public:
	CTest(const TDesC8& aSuiteName):CxxTest::TestSuite(aSuiteName){}

private:
	void setUp();
	void tearDown();

public:
	void testDummyTest();

};
