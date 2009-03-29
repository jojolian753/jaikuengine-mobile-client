// Copyright (c) 2007-2009 Google Inc.
// Copyright (c) 2006-2007 Jaiku Ltd.
// Copyright (c) 2002-2006 Mika Raento and Renaud Petit
//
// This software is licensed at your choice under either 1 or 2 below.
//
// 1. MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// 2. Gnu General Public license 2.0
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//
// This file is part of the JaikuEngine mobile client.

#include "cwu_welcome.h"

#include "cwu_welcomeaction.h"
#include "cwu_welcomeviews.h"

#include "cc_processmanagement.h"
#include "juik_iconmanager.h"
#include "reporting.h"
#include "symbian_auto_ptr.h"
#include "cn_networkerror.h"

#include <aknviewappui.h>


class CWelcomeControllerImpl : public CWelcomeController, public MContextBase, public MWelcomeActionObserver {
public:
	CWelcomeControllerImpl(CAknViewAppUi& aAppUi);
	virtual ~CWelcomeControllerImpl();
	void ConstructL();

public: // from CWelcomeController
	virtual void StartL();
	virtual void HandleResourceChangeL( TInt aType );
	
private: // from MWelcomeActionObserverL()
	virtual void ActionReadyL(CWelcomeAction& aAction);
	virtual void QuitWelcomeL(CWelcomeAction& aAction);
	
private:	
	CWelcomeViewBase* View() { return iView; }
	CAknViewAppUi& ViewAppUi() { return iAppUi; }
	
	void NextStep();
	void DoStepL();

	void ShowFirstPageL( );
	void FinishWelcomeL();		
	void KillStarterL( );


	void CompleteSelf();

private: // from CCheckedActive
	TInt CheckedRunError(TInt aError);
	void CheckedRunL();
	void DoCancel();
private:
	CAknViewAppUi& iAppUi;
	CWelcomeViewBase* iView;

	TInt iStepIx;

	RPointerArray<CWelcomeAction> iActions;

// 	class CWelcomeAction* iPrivacyStatement;
// 	class CWelcomeAction* iIntroPage;
// 	class CWelcomeAction* iCalendarSelection;
// 	class CWelcomeAction* iBluetoothSelection;
// 	class CWelcomeAction* iUserSelection;
// 	class CWelcomeAction* iApSelection;
// 	class CWelcomeAction* iNickAndPassword;
// 	class CWelcomeAction* iFinalPage;
	
	CJuikIconManager* iIconManager;
};
_LIT(KActiveName, "WelcomeController");
CWelcomeController::CWelcomeController() : CCheckedActive(EPriorityStandard, KActiveName) {}

	
CWelcomeControllerImpl::CWelcomeControllerImpl(CAknViewAppUi& aAppUi) : iAppUi(aAppUi)
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("CWelcomeControllerImpl"));
}

typedef CWelcomeAction* (*welcomeFactoryFunction) (void);

static const TInt KActionCount=11;
static const welcomeFactoryFunction actionFactories[ KActionCount ] =
	{
		&CreatePrivacyStatementActionL,
		&CreateIntroPageL,
		&CreateNetworkAccessActionL,
		&CreateNickAndPasswordActionL,
		&CreateUserSelectionActionL,

		&CreateCalendarSharingActionL,		
		&CreateBluetoothQueryActionL,
		&CreateBatteryQueryActionL,
		&CreateApSelectionActionL,
		&CreateAutoStartupActionL,			
		&CreateFinalPageL,
	};

static const TInt KFirstViewStep=1;
static const TInt KStartBackgroundStep=3; // NickAndPassword
static const TInt KStartJaikuServiceStep= KActionCount - 2; // CreateAutoStartupActionL

void CWelcomeControllerImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("ConstructL"));
	iIconManager = CJuikIconManager::NewL();

	iStepIx = -1;

	auto_ptr<CWelcomeAction> action(NULL); 

	
	for (TInt i=0; i < KActionCount; i++)
		{
			action.reset( (*actionFactories[i])() );
			action->AddObserverL( *this );
			iActions.AppendL( action.release() );
		}

	CActiveScheduler::Add(this);

	ProcessManagement::KillApplicationL(CEikonEnv::Static()->WsSession(), KUidContextContacts);	
	ProcessManagement::KillApplicationL(CEikonEnv::Static()->WsSession(), KUidcontext_log);

	KillStarterL();
}


CWelcomeControllerImpl::~CWelcomeControllerImpl()
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("~CWeolcomeControllerImpl"));
	Cancel();
	iActions.ResetAndDestroy();
	delete iIconManager;
}


TInt  CWelcomeControllerImpl::CheckedRunError(TInt aError)
{
	if ( aError == KLeaveExit )
		{
			Reporting().DebugLog(_L("WelcomeController: exiting"), aError );
			return aError;
		}
	else 
		{
			Reporting().DebugLog(_L("WelcomeController step "), iStepIx ) ;
			Reporting().DebugLog(_L("failed with "), aError ) ;
			return aError;
		};
}

void CWelcomeControllerImpl::CheckedRunL()
{
	DoStepL();
}

void CWelcomeControllerImpl::DoCancel()
{
}


void CWelcomeControllerImpl::NextStep()
{
	if (IsActive()) return;
	iStepIx++;
	CompleteSelf();
}

void CWelcomeControllerImpl::CompleteSelf()
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("CompleteSelf"));
	TRequestStatus* pStat = &iStatus;
	User::RequestComplete(pStat, KErrNone);
	SetActive();
}


// State handling
void CWelcomeControllerImpl::ActionReadyL(CWelcomeAction& /*aAction*/)
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("ActionReadyL"));
	// assert action == icurrentaction
	NextStep();
}


// State handling
void CWelcomeControllerImpl::QuitWelcomeL(CWelcomeAction& /*aAction*/)
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("QuitWelcomeL"));
	// assert action == icurrentaction
	FinishWelcomeL();
}

void CWelcomeControllerImpl::HandleResourceChangeL( TInt aType )
{
	if ( View() ) {
          View()->HandleResourceChangeL( aType );
        } else if (aType == KEikDynamicLayoutVariantSwitch) {

          iActions[iStepIx]->HandleOrientationChangeL();
        }
}

void CWelcomeControllerImpl::StartL()
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("StartL"));
	iStepIx = -1;
	NextStep();
}

void CWelcomeControllerImpl::DoStepL( )
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("DoStepL"));
	Reporting().DebugLog(_L("doing step"), iStepIx);

	if ( iStepIx < KActionCount )
		{
			if ( iStepIx == KStartJaikuServiceStep )
				{
					CNetworkError::ConnectionRequestedL();
					ProcessManagement::StartApplicationL(KUidcontext_log, KNullDesC, 200, EApaCommandBackground);
				}

			if (iStepIx < KStartBackgroundStep)
				{ // start background activities for this step
					iActions[iStepIx]->StartBackgroundActivitiesL();
				}
			else if ( iStepIx == KStartBackgroundStep )
				{ // start background activities for rest of steps
					for (TInt i=iStepIx; i < KActionCount; i++)
						{
							iActions[i]->StartBackgroundActivitiesL();
						}
				}

			if ( iStepIx == KFirstViewStep )
				{
					iView = CWelcomeViewBase::NewL();				
					iActions[iStepIx]->ShowUiL(*iView);
					ViewAppUi().AddViewL( iView ); 
					ViewAppUi().ActivateLocalViewL( iView->Id() );
				}
			else
				{
					iActions[iStepIx]->ShowUiL(*iView);
				}
		}
	else
		{
			FinishWelcomeL();
		}
}


void CWelcomeControllerImpl::FinishWelcomeL()
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("CloseApplicationL"));
	ViewAppUi().HandleCommandL( EEikCmdExit );
}

 

void CWelcomeControllerImpl::KillStarterL( )
{
	CALLSTACKITEM_N(_CL("CWelcomeControllerImpl"), _CL("KillStarterL"));
#ifndef __WINS__
	ProcessManagement::KillExeL(_L("c:\\system\\programs\\cl_starter.exe"));
#else
	RWsSession ws;
	User::LeaveIfError( ws.Connect() );
	CleanupClosePushL( ws );
	ProcessManagement::KillApplicationL(ws, KUidstarter, 200);
	CleanupStack::PopAndDestroy( &ws );
#endif
}



// Factory method

EXPORT_C CWelcomeController* CWelcomeController::NewL( CAknViewAppUi& aAppUi ) 
{
	CALLSTACKITEMSTATIC_N(_CL("CWelcomeController"), _CL("NewL"));
	auto_ptr<CWelcomeControllerImpl> self( new (ELeave) CWelcomeControllerImpl( aAppUi ) );
	self->ConstructL();
	return self.release();
}
