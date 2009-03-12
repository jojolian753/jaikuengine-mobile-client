/* 
	Copyright (c) 2003-2006, Penrillian Ltd. All rights reserved 
	Web: www.penrillian.com
*/

#ifndef __project_h__
#define __project_h__

#include <e32std.h>

#include "LoggingSupport.h"

/**
	Push the parameter onto the cleanup stack.  Answers it.
	(Useful if you want to use a creation L function as an LC one.
*/
template <class T> T* CleanupL( T* aT ) 
{ CleanupStack::PushL( aT ); return aT; }


#endif
