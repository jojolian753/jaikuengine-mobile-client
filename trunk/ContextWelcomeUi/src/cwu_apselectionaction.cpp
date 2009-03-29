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

#include "cwu_welcomeaction.h"

#include "cwu_welcomeviews.h"
#include <contextwelcomeui.rsg>

#include "autoap.h"
#include "break.h"
#include "cl_settings.h"
#include "reporting.h"
#include "symbian_auto_ptr.h"

#include <aknselectionlist.h>
#include <StringLoader.h>


class CAccessPointSelection 
	: public CWelcomeAction, 
	  public MPageObserver, 
	  public MAccessPointTestResult {
private:
	CAccessPointTester*	iAccessPointTester;	
	TBool iApTestingDone;
	TInt iTestedAp;
	TInt iTestedApError;
	CDesCArray* iAps;
	RArray<TInt> iApIds;
	MWelcomeView* iView;

	TBool iFirstTry;
public:	
	static CAccessPointSelection*  NewL();
	virtual ~CAccessPointSelection();

public: // From CWelcomeAction
	void StartBackgroundActivitiesL();
	void ShowUiL(MWelcomeView& aView);
	
private: // from MAccessPointTestResult
	void Done( TInt aAp, TInt aError );	

private: // from MPageObserver
	void ProcessSelectionL(TInt aIndex);
	void SelectedItemL(TInt aIndex);
	void LeftSoftKeyL(TInt aIndex);
	void RightSoftKeyL(TInt aIndex);

private:
	CAccessPointSelection();	
	void ConstructL();
	void StoreApAndFinishL(TInt aAp);
	void ShowApQueryL();

	void SelectAndShowUiL();
	void StartTestingApL(TInt aAp);
	TInt GetApCandidateL();


	enum TState
		{
			EInit = 0,
			EWaitingForApTesting,
			EShowingNoApMsg,
			EShowingApSelectionInfo,
		} iState;
};


CAccessPointSelection*  CAccessPointSelection::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CAccessPointSelection"), _CL("NewL"));
	auto_ptr<CAccessPointSelection> self( new (ELeave) CAccessPointSelection( ) );
	self->ConstructL();
	return self.release();
}

	
CAccessPointSelection::CAccessPointSelection(): iFirstTry(ETrue) {}


void CAccessPointSelection::ConstructL()
{				
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("ConstructL"));
	iAps = new (ELeave) CDesCArrayFlat(10);
}

CAccessPointSelection::~CAccessPointSelection()
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("~CAccessPointSelection"));
	delete iAccessPointTester;
	delete iAps;
	iApIds.Close();
}

TInt AccessPointCountL( )
{	
	CALLSTACKITEMSTATIC_N(_CL("CAccessPointSelection"), _CL("AccessPointCountL"));
	auto_ptr<CAccessPointLister> lister( CAccessPointLister::NewL() );
	TInt count = 0;
	TInt id = KErrNotFound;
	TBuf<50> apName;		
	while (	lister->NextRecordL(id, apName) ) { count++; }
	return count;
}


TInt CAccessPointSelection::GetApCandidateL()
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("GetApCandidateL"));
	TBuf<50> apName;
	TInt ignore;
	TInt ap = KErrNotFound;
		
	// Try to get an access point to be tested 
	Settings().GetSettingL( SETTING_IP_AP, ap );
	Reporting().DebugLog(_L("AP from settings"), ap);


#ifndef __WINS__
	if ( ap == KErrNotFound )
		{
				
			CC_TRAP(ignore, ap = CAutoAccessPoint::GetOperaApL( apName ));
			Reporting().DebugLog(_L("AP from Opera"), ap);
			if ( ap == KErrNotFound )	
				{
					CC_TRAP(ignore, ap = CAutoAccessPoint::GetDefaultApL( apName ));
					Reporting().DebugLog(_L("AP from default"), ap);
				}
			if ( ap == KErrNotFound )	
				{
					CC_TRAP(ignore, ap = CAutoAccessPoint::GetInternetApL( apName ));
					Reporting().DebugLog(_L("AP from internet"), ap);
				}
		}
#endif

	// Make sure that ap exists 
	TUint apTmp = ap;
	if ( ap != KErrNotFound) GetApNameL(apTmp, apName);
	ap = apTmp;
	Reporting().DebugLog(_L("AP candidate"), ap);
	return ap;
}


void CAccessPointSelection::StartBackgroundActivitiesL()
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("StartBackgroundActivitiesL"));
	TInt ap = GetApCandidateL();
	if ( ap == KErrNotFound ) 
		{
			iApTestingDone = ETrue;
			iTestedAp = KErrNotFound;
			iTestedApError = KErrNotFound;
		}
	else	
		{
			StartTestingApL( ap );
		}
}

void CAccessPointSelection::StartTestingApL(TInt aAp)
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("StartTestingApL"));
	iApTestingDone = EFalse;
	iTestedAp = KErrNotFound;
	iTestedApError = KErrNotFound;
	
	if (! iAccessPointTester) 
		iAccessPointTester = CAccessPointTester::NewL( *this );
	iAccessPointTester->TestAp( aAp );
}

void CAccessPointSelection::ShowApQueryL()
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("ShowApQueryL"));
	_LIT(KListFormat, "\t%S\t\t");
	iAps->Reset();
	iApIds.Reset();
	auto_ptr<CAccessPointLister> lister(CAccessPointLister::NewL());
	TInt id; 
	TBuf<50> apname;
	TBuf<60> listitem;
	while (lister->NextRecordL(id, apname)) {
		listitem.Format(KListFormat, &apname);
		iAps->AppendL(listitem);
		iApIds.Append( id );
	}
	
	TInt index = KErrNotFound;
    CAknSelectionListDialog* dialog = CAknSelectionListDialog::NewL(index, iAps, 0);
    if (dialog->ExecuteLD(R_AP_LIST_DIALOG))
		{
			if ( index != KErrNotFound )
				{
					TInt ap = iApIds[ index ];
					StartTestingApL(ap);
					SelectAndShowUiL();
				}
			else
				{
					NotifyActionReadyL();
				}
		}
}

	
void CAccessPointSelection::ShowUiL(MWelcomeView& aView)
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("ShowUiL"));
	iView = &aView;
	SelectAndShowUiL();
}
 

void CAccessPointSelection::SelectAndShowUiL()
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("SelectAndShowUiL"));
	if ( iApTestingDone )
		{		
			iView->StopWaitDialogL();
			
			if ( AccessPointCountL() == 0 )
				{
					auto_ptr<HBufC> text( StringLoader::LoadL( R_TEXT_WELCOME_NO_APS_TEXT ));
					
					iState = EShowingNoApMsg;
					iView->SetPageL( CWelcomeInfoPage::NewL( iView->ObjectProviderL(),
															 *text, *this, CWelcomePageBase::EHeaderText_AccessPoint));
				}
			else
				{
					Reporting().UserErrorLog(_L("Access point testing error"), iTestedApError);
					if ( iTestedApError != KErrNone )
						{	
							auto_ptr<HBufC> body( NULL );
							iState = EShowingApSelectionInfo;
							auto_ptr<CDesCArray> selections( new (ELeave) CDesCArrayFlat(3) );
							if ( iFirstTry )
								{
									iFirstTry = EFalse;
									body.reset( StringLoader::LoadL( R_TEXT_WELCOME_AP_SELECTION_TEXT ) );
									auto_ptr<HBufC> sel1( StringLoader::LoadL( R_TEXT_WELCOME_AP_SELECTION_ITEM ) );
									selections->AppendL( *sel1 );
								}
							else
								{
									body.reset( StringLoader::LoadL( R_TEXT_WELCOME_AP_FAILED_TEXT ) );
									auto_ptr<HBufC> sel1( StringLoader::LoadL( R_TEXT_WELCOME_AP_SELECT_ANOTHER ) );
									auto_ptr<HBufC> sel2( StringLoader::LoadL( R_TEXT_WELCOME_AP_SELECT_LATER ) );
									selections->AppendL( *sel1 );
									selections->AppendL( *sel2 );									
								}
							iView->SetPageL( CWelcomeSelectionPage::NewL( iView->ObjectProviderL(),
																		  *body, *selections, *this, CWelcomePageBase::EHeaderText_AccessPoint) );
						}
					 else
						 {
							 StoreApAndFinishL( iTestedAp );
						 }
				}
		}
	 else
		 {
			 iState = EWaitingForApTesting;
			 iView->ShowWaitDialogL( R_WELCOMEUI_WAIT_AP_TESTING );
		 }
}

void CAccessPointSelection::Done( TInt aAp, TInt aError )
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("Done"));
	Reporting().DebugLog( _L("Ap testing result"), aError);
	iApTestingDone = ETrue;
	iTestedAp = aAp;
	iTestedApError = aError;
	if ( EWaitingForApTesting == iState )
		{
			if (!iView) User::Leave( KErrNotReady );
			SelectAndShowUiL();
		}
}
	

void CAccessPointSelection::StoreApAndFinishL(TInt aAp) 
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("StoreApAndFinishL"));
	Settings().WriteSettingL(SETTING_IP_AP, aAp);
	NotifyActionReadyL();

}
	
void CAccessPointSelection::ProcessSelectionL(TInt aIndex)
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("ProcessSelectionL"));
	if ( EShowingApSelectionInfo == iState )
		{
			if (aIndex == 0)
				{
					ShowApQueryL();
					return;
				}
		}
	NotifyActionReadyL();
}

void CAccessPointSelection::SelectedItemL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("SelectedItemL"));
	ProcessSelectionL( aIndex );
}

void CAccessPointSelection::LeftSoftKeyL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("LeftSoftKeyL"));
	ProcessSelectionL( aIndex );
}

void CAccessPointSelection::RightSoftKeyL(TInt aIndex) 
{
	CALLSTACKITEM_N(_CL("CAccessPointSelection"), _CL("RightSoftKeyL"));
}



// FACTORY METHOD
CWelcomeAction* CreateApSelectionActionL()
{
	CALLSTACKITEMSTATIC_N(_CL(""), _CL("CreateApSelectionActionL"));
	return CAccessPointSelection::NewL();
}

