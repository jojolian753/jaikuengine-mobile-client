// SymbianOSUnitApp.cpp
// -------------------
//
// Copyright (c) 2003-2006 Penrillian Ltd.  All rights reserved.
// Web: www.penrillian.com
//



#include "SymbianOSUnit.h"
#if defined UIQ3 || defined SERIES60_3RD
#include <eikstart.h>
#endif

//             The entry point for the application code. It creates
//             an instance of the CApaApplication derived
//             class, CSymbianOSUnitApplication.
//
EXPORT_C CApaApplication* NewApplication()
{
	CApaApplication* application = NULL;
	TRAPD(err, application = CSymbianOSUnitApplication::NewL());
	return application;
}


#if defined UIQ3 || defined SERIES60_3RD

#if defined(__WINS__)
IMPORT_C void AllocateContextCommonExceptionData();
#endif

TInt RunTestsInThread(TAny*) {
	TRAPD(dummy, User::Leave(-1));
#if defined(__WINS__)
	AllocateContextCommonExceptionData();
#endif
	return EikStart::RunApplication(NewApplication);
}

#define AUTOMATIC_TEST_RUN

GLDEF_C TInt E32Main()
{
#ifdef AUTOMATIC_TEST_RUN
	User::After( 10*1000*1000 );
	RThread thread; TInt err;
	err=thread.Create(_L("unittests2"), 
		&RunTestsInThread, // thread's main function
		64*1024, /* stack */
		256*1024, /* min heap */
		2048*1024, /* max heap */
		0,
		EOwnerProcess);
	if (err!=KErrNone) return err;
	thread.SetPriority(EPriorityNormal);
	TRequestStatus s;
	thread.Logon(s);
	thread.Resume();
	User::WaitForRequest(s);
	TExitCategoryName n=thread.ExitCategory();
	TInt reason=thread.ExitReason();
	TExitType exittype=thread.ExitType();
	thread.Close();
	if (exittype==EExitPanic) {
		User::Panic( n, reason);
	}
	return reason;

#else
#  ifdef __WINS__
	TRAPD(dummy, User::Leave(-1));
	AllocateContextCommonExceptionData();
#  endif
	
	return EikStart::RunApplication(NewApplication);
#endif
}

#else

GLDEF_C TInt E32Dll(TDllReason)
{
	return KErrNone;
}

#endif
