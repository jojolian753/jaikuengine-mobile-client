// CSymbianOSUnitApplication.cpp
// ----------------------------------
//
// Copyright (c) 2003-2006 Penrillian Ltd.  All rights reserved.
// Web: www.penrillian.com


#include "SymbianOSUnit.h"
#include "context_uids.h"
//#include "allocator.h"

//The function is called by the UI framework to ask for the
//application's UID. The returned value is defined by the
//constant KUidSymbianOSUnit and must match the second value
//defined in the project definition file.

TUid CSymbianOSUnitApplication::AppDllUid() const
{
	return KUidSymbianOsUnit;
}

//This function is called by the UI framework at
//application start-up. It creates an instance of the
//document class.

CApaDocument* CSymbianOSUnitApplication::CreateDocumentL()
{
	return CSymbianOSUnitDocument::NewL(*this);
}


CSymbianOSUnitApplication* CSymbianOSUnitApplication::NewL()
{
	CSymbianOSUnitApplication* app = new (ELeave) CSymbianOSUnitApplication;
	CleanupStack::PushL(app);
#ifdef __WINS__
	TRAPD(dummy, User::Leave(1));
	//app->iOriginalAllocator=SwitchToLoggingAllocator();
	char *p=(char*)User::Alloc(32*1024*1024);
	User::Free(p);
#endif
	app->ConstructL();
	CleanupStack::Pop(); //app
	return app;
}

#if defined(__WINS__) && defined(EKA2)
extern "C" {
void _DisposeAllThreadData();
}
#endif

CSymbianOSUnitApplication::~CSymbianOSUnitApplication()
{
#if defined(__WINS__) && defined(EKA2)
	_DisposeAllThreadData();
#endif
}

void CSymbianOSUnitApplication::ConstructL()
{
}

