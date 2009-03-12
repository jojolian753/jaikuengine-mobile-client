/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/
#ifndef TESTHEADER_H
#define TESTHEADER_H
#include "TestSuite.h"

/* 

  Encapsulates all the tests. 
  
	Any member function starting 'test' is interpreted as a test to be executed.
	
	  The tests will be executed in the order they are in this file.
	  
		Note that the code for the individual tests may be in different CPP files.
		
*/


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

#endif // TESTHEADER_H
