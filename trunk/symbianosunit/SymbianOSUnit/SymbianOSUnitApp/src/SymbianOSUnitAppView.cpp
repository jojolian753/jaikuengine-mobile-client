// CSymbianOSUnitAppView.cpp
// -------------------
/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/

#include "SymbianOSUnit.h"
#include "testdriver.h"
#ifdef UIQ3
#include <QikCommand.h>
#include <SymbianosUnit.rsg>
#include <QikCommandManager.h>
#endif
const TInt KMaxTestOutput = 2048;

_LIT(KSymbianOsUnit,"Symbian OS Unit");

#ifdef UIQ3

CSymbianOSUnitAppView* CSymbianOSUnitAppView::NewLC(CQikAppUi& aAppUi)
{
	CSymbianOSUnitAppView* self = new (ELeave) CSymbianOSUnitAppView(aAppUi);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CSymbianOSUnitAppView::CSymbianOSUnitAppView(CQikAppUi& aAppUi)
: CQikViewBase(aAppUi, KNullViewId)
{
}

CSymbianOSUnitAppView::~CSymbianOSUnitAppView()
{
	delete iTestOutput;
	delete iChildFinished; iChildFinished=0;
	delete iRunInfo; iRunInfo=0;
}

void CSymbianOSUnitAppView::ConstructL()
{
	CQikViewBase::ConstructL();
	iTestOutput = HBufC::NewL(KMaxTestOutput);
}

void CSymbianOSUnitAppView::ViewConstructL()
{
	ViewConstructFromResourceL(R_UI_CONFIGURATIONS);
	iTestDisplay = LocateControlByUniqueHandle<CEikEdwin>(ESymbianOSUnitEdwinCtrl);
	iProgress = LocateControlByUniqueHandle<CEikProgressInfo>(ESymbianOSUnitProgressCtrl);
	UpdateCommandsL();
}


void CSymbianOSUnitAppView::HandleCommandL(CQikCommand& aCommand)
{
	switch(aCommand.Id())
	{
	case ECmdRunTest:
		RunSuitesL();
		break;
		
	case EEikCmdExit:
		CQikViewBase::HandleCommandL(aCommand);
		break;
		
	default:
		RunSuitesL(aCommand.Id() - ECmdRunTest -1);
		break;
	}
}

void CSymbianOSUnitAppView::UpdateCommandsL()
{
	CDesCArray* suitesNames = static_cast<CSymbianOSUnitAppUi&>(iQikAppUi).SuiteNamesLC();

	CQikCommandManager& manager = CQikCommandManager::Static();

	if(suitesNames->MdcaCount() > 0)
	{
		manager.SetDimmed(*this, ECmdRunTest, EFalse);
		for(TInt i = 0; i < suitesNames->MdcaCount(); i++)
		{
			CQikCommand* command = CQikCommand::NewLC(ECmdRunTest + i +1);
			command->SetHandler(this);
			command->SetTextL(suitesNames->MdcaPoint(i));
			manager.InsertCommandL(*this, command);
			CleanupStack::Pop(command);
		}
	}
	CleanupStack::PopAndDestroy(suitesNames);
}

CQikCommand* CSymbianOSUnitAppView::DynInitOrDeleteCommandL(CQikCommand* aCommand, const CCoeControl& /*aControlAddingCommands*/)
{
	switch(aCommand->Id())
	{
		case EEikEdwinCmdEditCopy:
		case EEikEdwinCmdEditSelectAllPenStyle:
		{
			aCommand->SetInvisible(ETrue);
			aCommand->SetAvailable(EFalse);
			break;
		}
		
		default:
			break;
	}
	return aCommand;
}

#else  //------------------  not uiq3 ----------------------------------

CSymbianOSUnitAppView* CSymbianOSUnitAppView::NewL(const TRect& aRect)
{
	CSymbianOSUnitAppView* self = new(ELeave) CSymbianOSUnitAppView();
	CleanupStack::PushL(self);
	self->ConstructL(aRect);
	CleanupStack::Pop();
	return self;
}

CSymbianOSUnitAppView::~CSymbianOSUnitAppView()
{ 
	delete iTestOutput;
	delete iTestDisplay;
	delete iProgress;	
	delete iUnitLabel;
	iEikonEnv->EikAppUi()->RemoveFromStack(this);
	iEikonEnv->EikAppUi()->DeregisterView(*this);
}

void CSymbianOSUnitAppView::ConstructL(const TRect& aRect)
{
	CreateWindowL();
	iEikonEnv->EikAppUi()->RegisterViewL(*this);
	iEikonEnv->EikAppUi()->AddToStackL(*this, this);
	//iEikonEnv->EikAppUi()->AddToStackL(this); 	
	Window().SetBackgroundColor(KRgbGreen);
	Window().SetShadowDisabled(ETrue);
	SetRect(aRect);
	
	iTestOutput = HBufC::NewL(KMaxTestOutput);	
	CreateSymbianOsUnitLabelL(aRect);	
	CreateProgressInfoL(aRect);
	CreateTestOutputDisplayL(aRect);
	
	ActivateL();	
}

void CSymbianOSUnitAppView::CreateTestOutputDisplayL(const TRect& aRect)
{
	iTestDisplay = new (ELeave) CEikEdwin();
#if defined SERIES60 || defined SERIES60_3RD
	iTestDisplay->SetMopParent(this);
	iTestDisplay->SetAknEditorFlags(EAknEditorFlagEnableScrollBars);
#endif
	iTestDisplay->SetContainerWindowL(*this);
	iTestDisplay->CEikEdwin::ConstructL(
		CEikEdwin::ENoAutoSelection|
		CEikEdwin::ENoHorizScrolling|
		CEikEdwin::EReadOnly,
		0,				
		0,		
		0		
		);	

	CEikScrollBarFrame* scrollbar;

#if defined SERIES60 || defined SERIES60_3RD
	scrollbar = iTestDisplay->ScrollBarFrame();
#else
	scrollbar = iTestDisplay->CreateScrollBarFrameL();
#endif
	
	scrollbar->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	iTestDisplay->SetPosition(TPoint(0,iProgress->Rect().iBr.iY));
	iTestDisplay->SetSize(TSize(aRect.Width(),aRect.Height()-iProgress->Rect().Height()));
}

void CSymbianOSUnitAppView::ViewActivatedL(const TVwsViewId& ,TUid , const TDesC8&)
{
}

void CSymbianOSUnitAppView::ViewDeactivated()
{
}

TKeyResponse CSymbianOSUnitAppView::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
    if (iTestDisplay)
	{
        if (iTestDisplay->IsFocused())
		{
            return iTestDisplay->OfferKeyEventL(aKeyEvent, aType);
		}
	}
    return EKeyWasNotConsumed;
}

void CSymbianOSUnitAppView::CreateProgressInfoL(const TRect& /*aRect*/)
{
	CEikProgressInfo::SInfo snf;
	snf.iTextType = EEikProgressTextNone;	
	snf.iSplitsInBlock = 0;
	snf.iFinalValue = 1;
	snf.iHeight = 100; 
	snf.iWidth = 120; 
	
	iProgress = new (ELeave) CEikProgressInfo(snf);
#if defined SERIES60 || defined SERIES60_3RD
	iProgress->ConstructL();
#endif
	iProgress->SetPosition(TPoint(0,iUnitLabel->Size().iHeight));
	TSize minSize = iProgress->MinimumSize();
	iProgress->SetSize(TSize(minSize.iWidth, 20));
	iProgress->SetContainerWindowL(*this);
}

void CSymbianOSUnitAppView::CreateSymbianOsUnitLabelL(const TRect& aRect)
{
	iUnitLabel = new (ELeave) CEikLabel;
	iUnitLabel->SetContainerWindowL(*this);
	iUnitLabel->SetTextL(KSymbianOsUnit());
	iUnitLabel->SetEmphasis(CEikLabel::ENoEmphasis);
	TInt textHeight = iUnitLabel->Font()->HeightInPixels();			
	iUnitLabel->SetPosition(TPoint(2,0));
	iUnitLabel->SetSize(TSize(aRect.Width()/2,textHeight));
}

void CSymbianOSUnitAppView::Draw(const TRect& aRect) const
{	
	CWindowGc& gc = SystemGc();
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.SetBrushColor(iEikonEnv->ControlColor(EColorWindowBackground,*this));
	gc.SetPenColor(KRgbGray);
	gc.DrawRect(aRect);
	
#if defined SERIES60 || defined SERIES60_3RD
	// underwrite bar as red or green to show result of test:	
	if(iTestDisplay && iTestDisplay->IsFocused())
	{
		gc.SetBrushColor(iFailedTest ? KRgbRed : KRgbGreen);
		TSize size = iSize;
		size.iHeight /= 2; 
		iProgress->SetSize(size);
		size.iHeight *= 2; 
		TPoint point = iProgress->Position();
		gc.DrawRect(TRect(point,size));
	}
#endif
	
}

TInt CSymbianOSUnitAppView::CountComponentControls() const
{
	return EMaxNumberControls;
}

CCoeControl* CSymbianOSUnitAppView::ComponentControl(TInt aIndex) const
{
	switch(aIndex)
	{
	case ETestDisplay:
		{
			return iTestDisplay;
			break;
		}		
	case EProgress:
		{
			return iProgress;
			break;
		}
	case EUnitLabel:
		{
			return iUnitLabel;
			break;
		}
	}
	return NULL;
}


#endif

#include "break.h"
#include "basched.h"

struct TRunInfo {
	TInt	iSuite;
	TUint16	iOutput[2048];
	
	TRunInfo() : iSuite(0) { }
	~TRunInfo() {  };
};

void RunSuiteInThreadL(TRunInfo* aInfo)
{
	//HBufC *output=aInfo->iOutput;
	HBufC *output=HBufC::NewL(2048);
	TestDriver testDriver(output, 0);
	
	CActiveScheduler* s=new (ELeave) CBaActiveScheduler;
	CActiveScheduler::Install(s);
	
	testDriver.runAllSuitesL(aInfo->iSuite);
	
	CActiveScheduler::Install(0);
	delete s;

	TPtr outb(aInfo->iOutput, 0, 2048);
	outb=output->Des();
	outb.PtrZ();
	
	delete output;	
}

TInt RunSuiteInThread(TAny* args)
{
	CTrapCleanup *cl=0;	
	cl=CTrapCleanup::New();
	
	TRunInfo *info=(TRunInfo *)args;
	TRAPD(err, RunSuiteInThreadL(info));
	
	delete cl;
	return err;
}

#include "TestUtils.h"

class CChildFinished : public CActive {
public:
	CChildFinished() : CActive(CActive::EPriorityStandard) { }
	void ConstructL(CSymbianOSUnitAppView* aView, RThread* aThread) {
		CActiveScheduler::Add(this);
		iThread=aThread;
		iView=aView;
		iThread->Logon(iStatus);
		SetActive();
	}
	void DoCancel() {
		iThread->Kill(KErrCancel);
	}
	void RunL() {
		delete iThread; iThread=0;
		iView->ThreadFinished();
	}
	TInt RunError(TInt aError) {
		return aError;
	}
	~CChildFinished() {
		Cancel();
		delete iThread;
	}
private:
	CSymbianOSUnitAppView* iView;
	RThread* iThread;
};

void CSymbianOSUnitAppView::ThreadFinished()
{
	iTestOutput->Des()=TPtrC(iRunInThreadInfo->iOutput);
	TRAPD( ignore, iTestDisplay->SetTextL(iTestOutput) );
	
#if defined UIQ3
	RequestRelayout(this);
#else
	iTestDisplay->SetFocus(ETrue);
	DrawNow();
#endif
	delete iChildFinished; iChildFinished=0;
	delete iRunInThreadInfo; iRunInThreadInfo=0;
}

void CSymbianOSUnitAppView::RunSuitesL(TInt aSuite)
{
	if (iRunInThreadInfo) User::Leave(KErrInUse);
	
	ClearViewL();
	iTestOutput->Des().Zero();
	iFailedTest = EFalse;
#ifdef UIQ3
	iProgress->SetIndicatorColorL(KRgbGreen);
#else
	CEikProgressInfo::SLayout layout = iProgress->Layout();
	layout.iFillColor = KRgbGreen;
	iProgress->SetLayout(layout);
#endif
#ifdef __WINS__
	TInt dummy;
	TBreakItem b(0, dummy);	
#endif
	
	iChildFinished=new (ELeave) CChildFinished;
	
	iRunInThreadInfo=new (ELeave) TRunInfo;
	iRunInThreadInfo->iSuite=aSuite;
	
	RThread *child=new (ELeave) RThread;
	
	TName threadName(_L("utch"));
	// Append a random number to make it unique
	threadName.AppendNum(Math::Random(), EHex);
	User::LeaveIfError( child->Create(threadName,   // create new testThread thread
							 &RunSuiteInThread, // thread's main function
							 32768,
							 16*1024,
							 1024*1024,
							 (TAny*)iRunInThreadInfo,
							 EOwnerProcess) );
	iChildFinished->ConstructL(this, child);
	child->Resume();
	
	return;
	
	TestDriver testDriver(iTestOutput,this);
	
	testDriver.runAllSuitesL(aSuite);

	TRAPD( ignore, iTestDisplay->SetTextL(iTestOutput) );
	
#if defined UIQ3
		RequestRelayout(this);
#else
	iTestDisplay->SetFocus(ETrue);
	DrawNow();
#endif
}


void CSymbianOSUnitAppView::ClearViewL()
{	
	if(iSize.iHeight == 0)
	{
		iSize = iProgress->Size();
	}
	iTestDisplay->SetFocus(EFalse);
	iTestDisplay->SetTextL(&(KNullDesC()));
	
#ifdef UIQ3
	iProgress->ResetIndicatorColor();
	RequestRelayout(this);
#else
	iProgress->SetAndDraw(0);
	DrawNow();
#endif

}

void CSymbianOSUnitAppView::DisplayEachTestResult()
{	
	return;
	
	iProgress->IncrementAndDraw(1);	
	TRAPD( ignore, iTestDisplay->SetTextL(iTestOutput) );
	DrawNow();
}

void CSymbianOSUnitAppView::FailedTest()
{
	return;
	
	if(!iFailedTest)
	{
		iFailedTest = ETrue;
#ifdef UIQ3
		iProgress->SetIndicatorColorL(KRgbRed);
		RequestRelayout(this);
#else
		CEikProgressInfo::SLayout layout = iProgress->Layout();
		layout.iFillColor = KRgbRed;
		iProgress->SetLayout(layout);
		DrawNow();
#endif

	}
}


TVwsViewId CSymbianOSUnitAppView::ViewId() const
{
	return TVwsViewId(KUidSymbianOsUnit, KUidNonUiTest);
}

