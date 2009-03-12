// SymbianOSUnit.h
/* 
	Copyright (c) 2003-2006, Penrillian Ltd. All rights reserved 
	Web: www.penrillian.com
*/


#ifndef __SymbianOSUnit_H
#define __SymbianOSUnit_H


#include <coeccntx.h>

#include <eikenv.h>



#include <eikmenup.h>
#include <eikedwin.h>
#include <eikprogi.h> //CEikProgressInfo

#include <eikon.hrh>
#include <eiklbbut.h>
#include <eiklabel.h> //CEikLabel
#include <eikcmbut.h> //CEikTextButton

#include <SymbianOSUnit.rsg>
#include "SymbianOSUnit.hrh"

#if defined SERIES60 || defined SERIES60_3RD
	#include <eikdoc.h>
	#include <eikapp.h>
	#include <eikappui.h>
	#include <akndoc.h>
	#include <aknapp.h>
	#include <aknappui.h>
#endif

#ifdef SERIES80
	#include <eikdoc.h>
	#include <eikapp.h>
	#include <eikappui.h>
#endif

#if defined UIQ || defined UIQ3
	#include <QikApplication.h>
	#include <QikAppUi.h>
	#include <QikDocument.h>
#endif
#ifdef UIQ3
	#include <QikViewBase.h>
#endif
#include <eikdef.h>


#if defined UIQ3 
const TUid KUidSymbianOSUnit	= {0xE01FADF2}; 
#elif defined SERIES60_3RD
//const TUid KUidSymbianOSUnit	= {0xE01FADF2}; 
#else
//const TUid KUidSymbianOSUnit	= {0x101FADF2}; 
#endif

const TUid KUidNonUiTest		= {1};

// Application 
#if defined UIQ || defined UIQ3
#define APPLICATION CQikApplication
#elif defined SYMBIAN60 || defined SERIES80
#define APPLICATION CEikApplication
#elif defined SERIES60 || defined SERIES60_3RD
#define APPLICATION CAknApplication
#endif

// Document
#if defined UIQ || defined UIQ3
#define DOCUMENT CQikDocument
#elif defined SYMBIAN60 || defined SERIES80
#define DOCUMENT CEikDocument
#elif defined SERIES60 || defined SERIES60_3RD
#define DOCUMENT CAknDocument
#endif

// Document
#if defined UIQ || defined UIQ3
#define APPUI CQikAppUi
#elif defined SYMBIAN60 || defined SERIES80
#define APPUI CEikAppUi
#elif defined SERIES60 || defined SERIES60_3RD
#define APPUI CAknAppUi
#endif

////////////////////////////////////////////////////////////////////////
//
// CSymbianOSUnitApplication
//
////////////////////////////////////////////////////////////////////////

class CSymbianOSUnitApplication : public APPLICATION
{
public:
	static CSymbianOSUnitApplication* NewL();
	~CSymbianOSUnitApplication();
	
private: 
	// Inherited from class CApaApplication
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
	void ConstructL();
#ifdef UIQ3
	CApaApplication* NewApplication();
#endif
	class RAllocator* iOriginalAllocator;
};

class MUiUpdater
{
public:
	virtual void DisplayEachTestResult() = 0;
	virtual void FailedTest() = 0;
};

////////////////////////////////////////////////////////////////////////
//
// CSymbianOSUnitAppView
//
////////////////////////////////////////////////////////////////////////
class CSymbianOSUnitAppView : 
#ifdef UIQ3
	public CQikViewBase,
#else
	public CCoeControl, public MCoeView, 
#endif
public MUiUpdater
{
public:
#ifdef UIQ3
	static CSymbianOSUnitAppView* NewLC(APPUI& aAppUi);
#else
	static CSymbianOSUnitAppView* NewL(const TRect& aRect);
#endif
	~CSymbianOSUnitAppView();
	void RunSuitesL(TInt aSuite = -1);
	void ClearViewL();
	void UpdateCommandsL();
	virtual void ThreadFinished();
	
private:
#ifdef UIQ3
	CSymbianOSUnitAppView(APPUI& aAppUi);
	void ConstructL();

	// from CQikViewBase
	void ViewConstructL();
	void HandleCommandL(CQikCommand& aCommand);
	CQikCommand* DynInitOrDeleteCommandL(CQikCommand* aCommand, const CCoeControl& /*aControlAddingCommands*/);

#else
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
    void ConstructL(const TRect& aRect);
	void CreateTestOutputDisplayL(const TRect& aRect);		
	void CreateProgressInfoL(const TRect& aRect);
	void CreateSymbianOsUnitLabelL(const TRect& aRect);
	void Draw(const TRect& aRect) const;

	//MCoeView
	void ViewActivatedL(const TVwsViewId& ,TUid , const TDesC8&);
	void ViewDeactivated();
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
#endif

	TVwsViewId ViewId() const;

	// From MUiUpdater
	void DisplayEachTestResult();
	void FailedTest();

private:
	HBufC*  iTestOutput;
	CEikEdwin* iTestDisplay;
	CEikProgressInfo* iProgress;
	CEikLabel* iUnitLabel;
	TBool iFailedTest;
	TSize iSize;
	struct TRunInfo *iRunInThreadInfo;
	class CChildFinished *iChildFinished;
#ifndef UIQ3
	enum
	{								
		EUnitLabel				= 0,
		EProgress				= 1,
		ETestDisplay			= 2,
		EMaxNumberControls		= 3		
	};
#endif
};


////////////////////////////////////////////////////////////////////////
//
// CSymbianOSUnitAppUi
//
////////////////////////////////////////////////////////////////////////
class CSymbianOSUnitAppUi : public APPUI
{
public:
    void ConstructL();
	~CSymbianOSUnitAppUi();

#ifdef UIQ3
	CDesCArray* SuiteNamesLC();
#else
	virtual void HandleCommandL(TInt aCommand);
	
private:
	// Inherited from class CEikAppUi
	virtual void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
#if defined SERIES60_3RD
	void HandleStatusPaneSizeChange();
#endif

#ifndef UIQ3
	void RunTests(TInt aSuite = -1);
	TErrorHandlerResponse HandleError(TInt aError,
	     const SExtendedError& aExtErr,
	     TDes& aErrorText,
	     TDes& aContextText);
#endif
	void RunSuiteByName(const TDesC& aName);
	
private:
	CSymbianOSUnitAppView* iAppView;
	CAsyncCallBack* iRun;
	TInt	iCbCount;
#endif
};


////////////////////////////////////////////////////////////////////////
//
// CSymbianOSUnitDocument
//
////////////////////////////////////////////////////////////////////////
class CSymbianOSUnitDocument : public DOCUMENT
{
public:	
	~CSymbianOSUnitDocument();
	static CSymbianOSUnitDocument* NewL(APPLICATION& aApp);

private: 
    // Inherited from CEikDocument
	CEikAppUi* CreateAppUiL();
	CSymbianOSUnitDocument(APPLICATION& aApp);
	void ConstructL();
	
};
#endif

