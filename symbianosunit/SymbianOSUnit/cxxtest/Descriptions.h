#ifndef __CXXTEST__DESCRIPTION_H
#define __CXXTEST__DESCRIPTION_H

//
// TestDescription, SuiteDescription and WorldDescription
// hold information about tests so they can be run and reported.
//

namespace CxxTest 
{
    class Runnable;
    class TestListener;
    class TestSuite;
    class TestRunner;

    struct TestDescription
    {
	const char *file;
	unsigned line;
	const char *suite;
	const char *name;
	Runnable *test;
	int mode; 
    };

    struct SuiteDescription
    {
	const char *name;
	TestSuite *suite;
	unsigned numTests;
	const TestDescription *tests;
    };

    class WorldDescription
    {
    public:
	virtual signed int numSuites( void ) const = 0;
	virtual unsigned numTotalTests( void ) const = 0;
	virtual const SuiteDescription &suiteDescription( unsigned i ) const = 0;
	
    protected:
	friend class TestRunner;
	virtual void setUp( TestListener *l ) const = 0;
	virtual void tearDown( TestListener *l ) const = 0;
    };
}

#endif // __CXXTEST__DESCRIPTION_H

