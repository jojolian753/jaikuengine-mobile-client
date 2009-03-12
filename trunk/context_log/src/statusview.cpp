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

#include "break.h"
#include "statusview.h"
#include <akntitle.h>
#include <eikspane.h>
#include "contextlog_resource.h"

// INCLUDE FILES
#include  <aknviewappui.h>
#include  <avkon.hrh>
#include  "StatusView.h"
#include  "context_logContainer.h" 
#include  "Context_log.hrh"
#include  "symbian_auto_ptr.h"
#include "view_ids.h"
#include <eikdll.h>
const TUid KUidPhone = { 0x100058b3 };

// ---------------------------------------------------------
// CStatusView::ConstructL(const TRect& aRect)
// EPOC two-phased constructor
// ---------------------------------------------------------
//

CStatusView* CStatusView::NewL(MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CStatusView"), _CL("NewL"), &Context);

	auto_ptr<CStatusView> ret(new (ELeave) CStatusView(Context));
	ret->ConstructL();
	return ret.release();
}

void CStatusView::ConstructL()
{
	CALLSTACKITEM_N(_CL("CStatusView"), _CL("ConstructL"));

#ifndef FLICKR
	BaseConstructL( R_CL_STATUSVIEW );
#else
	BaseConstructL( R_CL_FLICKR_STATUSVIEW );
#endif
	iContainer = new (ELeave) CContext_logContainer(AppContext());
	iContainer->MakeVisible(EFalse);
	iContainer->SetMopParent(this);
	TRect r=ClientRect();
	iContainer->ConstructL( r );
}

// ---------------------------------------------------------
// CStatusView::~CStatusView()
// ?implementation_description
// ---------------------------------------------------------
//
CStatusView::~CStatusView() {
	CC_TRAPD(err, ReleaseCStatusView());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}

void CStatusView::ReleaseCStatusView()
{
	CALLSTACKITEM_N(_CL("CStatusView"), _CL("ReleaseCStatusView"));

	if ( iContainer && iActive )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
}

// ---------------------------------------------------------
// TUid CStatusView::Id()
// ?implementation_description
// ---------------------------------------------------------
//
TUid CStatusView::Id() const {
	return KStatusView;
}

// ---------------------------------------------------------
// CStatusView::HandleCommandL(TInt aCommand)
// ?implementation_description
// ---------------------------------------------------------
//
void CStatusView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM_N(_CL("CStatusView"), _CL("HandleCommandL"));

	AppUi()->HandleCommandL(aCommand);
}

// ---------------------------------------------------------
// CStatusView::HandleClientRectChange()
// ---------------------------------------------------------
//
void CStatusView::HandleClientRectChange()
{
	CALLSTACKITEM_N(_CL("CStatusView"), _CL("HandleClientRectChange"));

	if ( iContainer )
        {
		TRect r=ClientRect();
		iContainer->SetRect( r );
        }
}

// ---------------------------------------------------------
// CStatusView::DoActivateL(...)
// ?implementation_description
// ---------------------------------------------------------
//
void CStatusView::DoActivateL(
				   const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/,
				   const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CStatusView"), _CL("DoActivateL"));

	iActive=true;
	TRect r=ClientRect();
	iContainer->SetRect( r );
	iContainer->MakeVisible(ETrue);

	AppUi()->AddToStackL( *this, iContainer );
	CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
	CAknTitlePane* tp=(CAknTitlePane*)sp->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
	tp->SetText(CEikonEnv::Static()->AllocReadResourceL(R_CAPTION));
	if ( iNextViewId != TVwsViewId() ) {
		TVwsViewId v = iNextViewId;
		iNextViewId= TVwsViewId();

		{
			// it tends not to be safe to try to re-activate
			// the media capture apps if they've been shut down
			
			RWsSession& ws=CEikonEnv::Static()->WsSession();
			TApaTaskList tl(ws);
			TApaTask app_task=tl.FindApp(v.iAppUid);
			if (! app_task.Exists() ) {
				// so activate the phone app instead
				TApaTask app_task=tl.FindApp(KUidPhone);
				if (app_task.Exists()) app_task.BringToForeground();
				goto done_activate;
			}
		}
		if (v.iAppUid == KCameraUid) {
			CC_TRAPD(err, ActivateViewL(TVwsViewId(KCameraUid, KCameraViewUid)));
#ifndef __S60V3__
		//FIXME3RD
		} else if (v.iAppUid == KCamera2Uid) {
			/* the camera on 6630 will be stuck in showing the photo on reactivation */
			auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\camcorder\\camcorder.app")));
			cmd->SetCommandL(EApaCommandRun);
			CC_TRAPD(err, EikDll::StartAppL(*cmd));
		} else if (v.iAppUid == KCamera3Uid) {
			// the camera on N70 will hang if the slider's been closed and we
			//   try to reactivate it 
			//Reporting().UserErrorLog(_L("starting camera3"));
			auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\cammojave\\cammojave.app")));
			cmd->SetCommandL(EApaCommandRun);
			CC_TRAPD(err, EikDll::StartAppL(*cmd));			
		} else if (v.iAppUid == KVideoUid) {
			CC_TRAPD(err, ActivateViewL(TVwsViewId(KVideoUid, KVideoViewUid)));
                } else if (v.iAppUid == KRecorderUid) {
			auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\voicerecorder\\voicerecorder.app")));
			cmd->SetCommandL(EApaCommandRun);
			CC_TRAPD(err, EikDll::StartAppL(*cmd));			
#endif
		} else {
			//Reporting().UserErrorLog(_L("activating previous"));
			CC_TRAPD(err, ActivateViewL(v));
		}
done_activate:
		;

	}
}

// ---------------------------------------------------------
// CStatusView::HandleCommandL(TInt aCommand)
// ?implementation_description
// ---------------------------------------------------------
//
void CStatusView::DoDeactivate() {
	iActive=false;
	AppUi()->RemoveFromViewStack( *this, iContainer );
	iContainer->MakeVisible(EFalse);
}

void CStatusView::DynInitMenuPaneL(
		 TInt aResourceId,CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CStatusView"), _CL("DynInitMenuPaneL"));

	AppUi()->DynInitMenuPaneL(aResourceId, aMenuPane);
}

// End of File


