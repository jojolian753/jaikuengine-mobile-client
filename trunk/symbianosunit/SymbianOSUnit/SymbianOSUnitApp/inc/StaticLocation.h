/* 
	Copyright (c) 2003-2006, Penrillian Ltd. All rights reserved 
	Web: www.penrillian.com
*/

#ifndef _STATIC_LOCATION_H
#define _STATIC_LOCATION_H
#include <coemain.h>

/**
	Magic class to implement static variables
	e.g. 
	static CLoggerStaticData& StaticDataL() 
	{ return CStaticLocation<CLoggerStaticData, KUidPenrillianLogger>::GetL(); } 

*/
template <class TYPE, TInt UID> class CStaticLocation: public CCoeStatic
{
	TYPE iLocation;	
	CStaticLocation() : CCoeStatic( TUid::Uid(UID) ) {}

public:	
	
	/**
	Answer a location to hold a static variable of type TYPE.
	UID must be a unique ID (preferably allocated by Symbian).
	*/
	static TYPE& GetL()
	{
		CStaticLocation<TYPE, UID>* staticData = static_cast<CStaticLocation<TYPE, UID>* >(CCoeEnv::Static(TUid::Uid(UID)));
		if (staticData)
			return staticData->iLocation;
		staticData = new (ELeave) CStaticLocation<TYPE, UID>;
		return staticData->iLocation;
	}
};
#endif