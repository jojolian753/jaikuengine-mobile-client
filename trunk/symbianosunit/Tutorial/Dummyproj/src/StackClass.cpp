#include <e32base.h>
#include "StackClass.h"

CStack::CStack()
{
	iSize=0;
}

CStack::push(TInt aValue)
{
	iItems[iSize++]=aValue;
}

TInt CStack::pop()
{
	return iItems[--iSize];
}

TInt CStack::CurrentSize()
{
	return iSize;
}