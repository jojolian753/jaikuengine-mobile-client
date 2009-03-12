// CSymbianOSUnitAppUi.cpp
// -----------------
//
// Copyright (c) 2003-2006 Penrillian Ltd.  All rights reserved.
//Web: www.penrillian.com


#include "SymbianOSUnit.h"
#include "Logger.h"
#include "TestDriver.h"
#include <w32std.h> // RWsSession 

#define AUTOMATIC_TEST_RUN

_LIT(KTestLogsDir, "SymbianOSUnit");
const TInt KMaxCmds = 2000;

TInt RunTestsCb(TAny* aPtr) {
	CSymbianOSUnitAppUi* aui=(CSymbianOSUnitAppUi*)aPtr;
	aui->HandleCommandL(ECmdRunAutomatic);
	return KErrNone;
}
TInt ExitCb(TAny* aPtr) {
	CSymbianOSUnitAppUi* aui=(CSymbianOSUnitAppUi*)aPtr;
	aui->HandleCommandL(EEikCmdExit);
	return KErrNone;
}

TErrorHandlerResponse CSymbianOSUnitAppUi::HandleError(TInt aError,
     const SExtendedError& aExtErr,
     TDes& aErrorText,
     TDes& aContextText)
{
	User::Leave(aError);
	return ENoDisplay; // not reached
}

void CSymbianOSUnitAppUi::ConstructL()
{
#ifdef UIQ3
	CQikAppUi::ConstructL();
	AddViewL(*CSymbianOSUnitAppView::NewLC(*this));
	CleanupStack::Pop(); // view
#else
    BaseConstructL();
	iAppView = CSymbianOSUnitAppView::NewL(ClientRect());
#endif

	Logger::StartL(KTestLogsDir);
	
	// need this to support series60 scrollbar
#if defined SERIES60 || defined SERIES60_3RD
	iAppView->SetMopParent(this);

#elif defined UIQ
	CDesCArrayFlat* suiteNames = new (ELeave) CDesCArrayFlat(3);
	CleanupStack::PushL(suiteNames);
	TestDriver::TestSuiteNamesL(*suiteNames);
	if(suiteNames->MdcaCount() == 0)
	{
		CEikButtonGroupContainer* toolBar = iEikonEnv->AppUiFactory()->ToolBar();
		toolBar->DimCommand(ECmdRunTest, ETrue);
	}
	CleanupStack::PopAndDestroy(suiteNames);
#endif
	iEikonEnv->WsSession().SetAutoFlush(ETrue);
	
#ifdef AUTOMATIC_TEST_RUN
	iRun=new (ELeave) CAsyncCallBack( TCallBack(RunTestsCb, this), CActive::EPriorityIdle);
	iRun->CallBack();	
#endif
}

#ifdef UIQ3
CDesCArray* CSymbianOSUnitAppUi::SuiteNamesLC()
{
	CDesCArrayFlat* suiteNames = new (ELeave) CDesCArrayFlat(3);
	CleanupStack::PushL(suiteNames);
	TestDriver::TestSuiteNamesL(*suiteNames);
	return suiteNames;
}


#endif


CSymbianOSUnitAppUi::~CSymbianOSUnitAppUi()
{
	Logger::Finish();
#ifndef UIQ3
	delete iAppView;
	iAppView = NULL;
#endif
	delete iRun;
}


#ifndef UIQ3

void CSymbianOSUnitAppUi::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
{
	if(aResourceId == R_FILE_FIRST_MENU)
	{
		CDesCArrayFlat* suiteNames = new (ELeave) CDesCArrayFlat(3);
		CleanupStack::PushL(suiteNames);
		TestDriver::TestSuiteNamesL(*suiteNames);
		if(suiteNames->MdcaCount() == 0)
		{
			aMenuPane->SetItemDimmed(ECmdRunTest, ETrue);
		}
		else
		{
			CEikMenuPaneItem::SData data;
			data.iCascadeId = 0; 
			data.iFlags = 0; 
			for(TInt i = 0; i < suiteNames->MdcaCount(); i++)
			{
				data.iCommandId = ECmdRunTest + i +1;
				data.iText = suiteNames->MdcaPoint(i);
#if defined SERIES60 || defined SERIES60_3RD
				aMenuPane->InsertMenuItemL(data, i+1);
#elif defined UIQ || defined SERIES80
				aMenuPane->InsertMenuItemAtL(data, i+1);
#endif
			}
		}
		CleanupStack::PopAndDestroy(suiteNames);
	}

}

void CSymbianOSUnitAppUi::HandleCommandL(TInt aCommand)
{	
	switch (aCommand)
	{					
		case EEikCmdExit: 
		{
			Exit();
			break;
		}
		case ECmdRunTest:
		{
			RunTests();
			break;
		}
		case ECmdRunAutomatic:
		{
			delete iRun; iRun=0;
			if (iCbCount<15) {
				iCbCount++;
				User::After(TTimeIntervalMicroSeconds32(10*1000));
				iRun=new (ELeave) CAsyncCallBack( TCallBack(RunTestsCb, this), CActive::EPriorityIdle);
				iRun->CallBack();
				return;
			}
			/*
			RunTests();
			iRun=new (ELeave) CAsyncCallBack( TCallBack(ExitCb, this), EPriorityLow);
			iRun->CallBack();
			*/
			RunSuiteByName(_L("CTest_Downloader"));
			//RunSuiteByName(_L("CTest_Storage"));
			break;
		}
		default:
		{
			RunTests(aCommand - ECmdRunTest -1);
			break;
		}
	}
}

void CSymbianOSUnitAppUi::RunSuiteByName(const TDesC& aName)
{
	CDesCArrayFlat* suiteNames = new (ELeave) CDesCArrayFlat(3);
	CleanupStack::PushL(suiteNames);
	TestDriver::TestSuiteNamesL(*suiteNames);
	TInt pos;
	User::LeaveIfError(suiteNames->FindIsq(aName, pos));
	CleanupStack::PopAndDestroy();
	RunTests(pos);
}

void CSymbianOSUnitAppUi::RunTests(TInt aSuite)
{
	TRAPD(err, iAppView->ClearViewL() );
	TRAP(err,iAppView->RunSuitesL(aSuite));	
	if(err != KErrNone)
	{
		User::InfoPrint(_L("Leave: could not run tests"));
	}
}

#if defined SERIES60_3RD
void CSymbianOSUnitAppUi::HandleStatusPaneSizeChange()
{
	iAppView->SetRect( ClientRect() );
}
#endif

#endif //not UIQ3