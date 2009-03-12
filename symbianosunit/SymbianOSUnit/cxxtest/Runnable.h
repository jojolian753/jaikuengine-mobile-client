#ifndef __CXXTEST__RUNNABLE_H
#define __CXXTEST__RUNNABLE_H

//
// class Runnable
// Used internaly as a "smart pointer" to tests
//

namespace CxxTest 
{
    class Runnable
    {
    protected:
	friend class TestRunner;
	virtual void run( void ) = 0;
    };
}


#endif // __CXXTEST__RUNNABLE_H
