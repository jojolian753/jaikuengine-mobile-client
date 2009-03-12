/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/
#ifndef __TestUtils_h__
#define __TestUtils_h__

#include <e32base.h>
#include <e32std.h>

namespace TestUtils
{

	TBool WaitForActiveSchedulerStopL( TInt aTimeoutSeconds );
	enum TThreadWaitOption { EWaitForExit, ENoWaitForExit }; 
	void RunInSeparateThreadL( const TCallBack& aFunctionToRun, RThread& testThread, 
		TThreadWaitOption aWaitForExit = EWaitForExit );
}

#endif // __TestUtils_h__