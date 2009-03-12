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

#include "ccu_jaikuview_base.h"

#include "ccu_mviewnavigation.h"
#include "ccu_posterui.h"
#include "ccu_errorcontainer.h"

#include "app_context_impl.h"
#include "break.h"
#include "errorhandling.h"
#include "reporting.h"
#include "cu_errorui.h"

#include "ccu_uidelegates.h"

#include <aknviewappui.h>

EXPORT_C void CJaikuViewBase::SetDependenciesL(	CActiveState& aActiveState,
												CUserPics&   aUserPics,
												CJabberData& aJabberData,
												phonebook&        aPhonebook,
												CFeedItemStorage& aFeedStorage,
												MViewNavigation& aViewNavigation,
												CPosterUi& aPosterUi,
												CJaicons& aJaicons,
												CThemeColors& aThemeColors,
												CTimePeriodFormatter& aPeriodFormatter,
												CPresenceHolder& aPresenceHolder,
												CJuikGfxStore& aFeedGraphics,
												CStreamStatsCacher& aStreamStats,
												MCommonMenus& aCommonMenus,
												CProgressBarModel& aProgressBarModel)
{
	CALLSTACKITEM_N(_CL("CJaikuViewBase"), _CL("SetDependenciesL"));
	iActiveState = &aActiveState;
	iUserPics    = &aUserPics;
	iJabberData  = &aJabberData;
	iPhonebook   = &aPhonebook;
	iFeedStorage = &aFeedStorage;
	iViewNavigation = &aViewNavigation;
	iPosterUi = &aPosterUi;
	iJaicons = &aJaicons;
	iThemeColors = &aThemeColors;
	iPeriodFormatter = &aPeriodFormatter;
	iPresenceHolder = &aPresenceHolder;
	iFeedGraphics = &aFeedGraphics;
	iStreamStats = &aStreamStats;
	iCommonMenus = &aCommonMenus;
	iProgressBarModel = &aProgressBarModel;
}


EXPORT_C void CJaikuViewBase::SetParentViewL(const TVwsViewId& aParentViewId)
{
	CALLSTACKITEM_N(_CL("CJaikuViewBase"), _CL("SetParentViewL"));
	iParentViewId = aParentViewId;
}

EXPORT_C void CJaikuViewBase::ActivateParentViewL()
{
	CALLSTACKITEM_N(_CL("CJaikuViewBase"), _CL("ActivateParentViewL"));
	ActivateViewL( iParentViewId );
}


EXPORT_C CJaikuViewBase::CJaikuViewBase() 
{
}

EXPORT_C CJaikuViewBase::~CJaikuViewBase()
{
	if ( iErrorUi )
		{
			AppUi()->RemoveFromStack( iErrorUi );
			delete iErrorUi;
			iErrorUi = NULL;
		}
	delete iLastErrorInfo;
}

EXPORT_C void CJaikuViewBase::StoreAndReportLastErrorL(TInt aCode)
{
	delete iLastErrorInfo; iLastErrorInfo = NULL;
	const MErrorInfo* origEi = ErrorInfoMgr().GetLastErrorInfo( MakeErrorCode( CONTEXT_UID_CONTEXTCONTACTSUI, aCode ) );
	if ( origEi )
		iLastErrorInfo = origEi->CreateCopyL();
	_LIT(KSource, "CJaikuViewBase"); // This isn't use in new error handling mechanism
	_LIT(KReason, "Leave"); // This isn't use in new error handling mechanism 
	ReportActiveError( KSource, KReason, aCode);
}


EXPORT_C void CJaikuViewBase::ShowLastErrorL()
{
	if ( ! iErrorUi ) {
		iErrorUi= CErrorContainer::NewL(this, ClientRect(), ThemeColors(), iLastErrorInfo);
	}	
	AppUi()->AddToStackL( *this, iErrorUi );
}


EXPORT_C void CJaikuViewBase::DoActivateL(const TVwsViewId& aPrevViewId,
								 TUid aCustomMessageId,
								 const TDesC8& aCustomMessage) 
{
	MActiveErrorReporter* rep=GetContext()->GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);
	CALLSTACKITEM_N(_CL("CJaikuViewBase"), _CL("DoActivateL"));

	TUid prev = ViewNavigation().CurrentView();
	CC_TRAPD(err, ViewNavigation().SetCurrentViewL( Id() ) );
	if ( err == KErrNone )
		CC_TRAP(err, CommonDoActivateL( aPrevViewId, aCustomMessageId, aCustomMessage ) );
	
	// propagate memory errors and return view state
	if ( ( err == KErrNoMemory || err == KErrDiskFull )
		 && prev != TUid::Null() )
		{
			ViewNavigation().SetCurrentViewL( prev ); // let it just leave if something happens here
			User::Leave( err );
		}
	// else keep view state and show error 
	else if ( err != KErrNone )
		{
			StoreAndReportLastErrorL(err);
			ShowLastErrorL();
		}
}


EXPORT_C void CJaikuViewBase::CommonDoActivateL(const TVwsViewId& aPrevViewId,
												TUid aCustomMessageId,
												const TDesC8& aCustomMessage)
{
	CALLSTACKITEM_N(_CL("CJaikuViewBase"), _CL("CommonDoActivateL"));
	// ViewNavigation related calls moved to DoActivateL, so nothing extra to do here. 
	// Keeping CommonDoActivateL method for future and debugging purposes.
	RealDoActivateL(aPrevViewId, aCustomMessageId, aCustomMessage);
}


EXPORT_C void CJaikuViewBase::DoDeactivate()
{
	CC_TRAPD(err, RealDoDeactivateL());
	if ( iErrorUi )
		{
			AppUi()->RemoveFromStack( iErrorUi );
			delete iErrorUi;
			iErrorUi = NULL;
		}
}

EXPORT_C TUiDelegates CJaikuViewBase::UiDelegates()
{
	TUiDelegates d;
	d.iThemeColors = iThemeColors;
	d.iUserPics = iUserPics;
	d.iJaicons = iJaicons;
	d.iPeriodFormatter = iPeriodFormatter;
	d.iJabberData = iJabberData;
	d.iPresenceHolder = iPresenceHolder;
	d.iFeedGraphics = iFeedGraphics;
	d.iFeedStorage = iFeedStorage;
	d.iActiveState = iActiveState;
	return d;
}


EXPORT_C TBool CJaikuViewBase::PreHandleCommandL(TInt aCommand)
{
	if ( iErrorUi )
		{
			AppUi()->HandleCommandL( aCommand );
			return ETrue;
		}
	else
		return EFalse;
}
