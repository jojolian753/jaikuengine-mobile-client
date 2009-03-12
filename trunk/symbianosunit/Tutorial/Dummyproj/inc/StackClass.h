// A dummy engine class

#ifndef DUMMYENGINE_H
#define DUMMYENGINE_H

#include <e32base.h>

#define MAX_STACK_SIZE	5


class CStack: public CBase
{
	public:
		CStack();
		push(TInt aValue);
		TInt pop();
		TInt CurrentSize();

	private:
		TInt iSize;
		TInt iItems[MAX_STACK_SIZE];
};


#endif
