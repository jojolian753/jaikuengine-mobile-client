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
#include "ContextCallLogAppUi.h"
#include "ContextCallLogView.h"
#include <ContextCallLog.rsg>
#include "contextcalllog.hrh"
#include "contextcalllogapp.h"

#include <avkon.hrh>
#include "presence_icons.h"
#include <bautils.h>

#include "cl_settings.h"
#include "app_context_impl.h"
#include "contextcommon.h"
#include "file_logger.h"
#include <gulicon.h>
#include <sysutil.h>

const TInt KUidToBackgroundValue=0x2004;
const TUid KUidToBackground={KUidToBackgroundValue};

void CContextCallLogAppUi::ConstructL()
{
	BaseConstructL(0x1008);

	MContextAppUi::ConstructL(AppContext(), _L("calllog_log"));
	TBool N70=EFalse;
#ifdef __S60V2__
	{
		HBufC8* agent=SysUtil::UserAgentStringL();
		if ( (*agent).Left(8).Compare(_L8("NokiaN70"))==0) N70=ETrue;
		delete agent;
	}	
#endif
	if (N70) iEikonEnv->SetSystem(ETrue);

	TInt err;
	CC_TRAP(err, { iJabberDb=CDb::NewL(AppContext(), _L("JABBER"), EFileRead|EFileShareAny); } );
	if (err == KErrNone) {
		iJabberData=CJabberData::NewL(AppContext(), *iJabberDb, SETTING_JABBER_NICK);
		iPresenceHolder=CPresenceHolder::NewL(*iJabberData);
	} else {
		iLog->write_line(_L("error reading jabber db."));
	}

	CC_TRAP(err, { iLookupDb=CDb::NewL(AppContext(), _L("LOOKUPS"), EFileWrite|EFileShareAny); } );
	if (err == KErrNone) {
		iStorage = CLookupStorage::NewL(AppContext(), *iLookupDb, *iJabberData);
	} else if (iLog) {
		iLog->write_line(_L("error reading lookup db."));
	}

	iCallLog=new (ELeave) call_log(AppContext(), iJabberData, iPresenceHolder);
	iCallLog->ConstructL();

	iPhoneHelper = new (ELeave) phonehelper_ui(iLog);
	iPhoneHelper->ConstructL();

	if (iPresenceHolder) iPresenceHolder->AddListener(iCallLog);
	if (iPresenceHolder) iPresenceHolder->AddListener(this);

	CEikStatusPane* sp = StatusPane();
	iNaviPane = (CAknNavigationControlContainer*)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	iDecoratedTabGroup = iNaviPane->ResourceDecorator();
	if (iDecoratedTabGroup)
	{
		iTabGroup = (CAknTabGroup*) iDecoratedTabGroup->DecoratedControl();
	}

	iIconlist = new (ELeave) CAknIconArray(30);
	LoadIcons(iIconlist);

	// for phonebook dialogs
	iResource_files=new (ELeave) CArrayFixFlat<TInt>(5);
	TFileName resfile=_L("z:\\System\\data\\PBKVIEW.RSC");
	BaflUtils::NearestLanguageFile(iEikonEnv->FsSession(), resfile); 
	iResource_files->AppendL(iEikonEnv->AddResourceFileL(resfile));

	iMissedView = new (ELeave) CContextCallLogView(iJabberData, iLog, iCallLog, iIconlist, *iPhoneHelper);

	CleanupStack::PushL( iMissedView );
	iMissedView->ConstructL(KMissedViewId);
	AddViewL( iMissedView );      // transfer ownership to CAknViewAppUi
	CleanupStack::Pop();    

	iReceivedView = new (ELeave) CContextCallLogView(iJabberData, iLog, iCallLog, iIconlist, *iPhoneHelper);

	CleanupStack::PushL( iReceivedView );
	iReceivedView->ConstructL(KReceivedViewId);
	AddViewL( iReceivedView );      // transfer ownership to CAknViewAppUi
	CleanupStack::Pop();  

	iDialledView = new (ELeave) CContextCallLogView(iJabberData, iLog, iCallLog, iIconlist, *iPhoneHelper);

	CleanupStack::PushL( iDialledView );
	iDialledView->ConstructL(KDialledViewId);
	AddViewL( iDialledView );      // transfer ownership to CAknViewAppUi
	CleanupStack::Pop();

	iLookupView = new (ELeave) CContextCallLogView(iJabberData, iLog, iStorage, iIconlist, *iPhoneHelper);

	CleanupStack::PushL( iLookupView );
	iLookupView->ConstructL(KLookupViewId);
	AddViewL( iLookupView );      // transfer ownership to CAknViewAppUi
	CleanupStack::Pop();

	auto_ptr<CPresenceDetailView> detailview(CPresenceDetailView::NewL());
	AddViewL(detailview.get());
	iPresenceDetailView=detailview.release();

	SetDefaultViewL(*iMissedView);

	iEikonEnv->AddForegroundObserverL(*this);
}

CContextCallLogAppUi::~CContextCallLogAppUi()
{
	if(iDialledView) iDialledView->exiting();
	if(iReceivedView) iReceivedView->exiting();
	if(iMissedView) iMissedView->exiting();
	if(iLookupView) iLookupView->exiting();

	iEikonEnv->RemoveForegroundObserver(*this);
	if (iResource_files)
	{
		for (int i=0; i<iResource_files->Count(); i++) {
			iEikonEnv->DeleteResourceFile((*iResource_files)[i]);
		}
	}
	delete iResource_files;
	delete iPhoneHelper;

	delete iCallLog;
	delete iPresenceHolder;
	delete iJabberData;
	delete iJabberDb;
	delete iDecoratedTabGroup; 
	if (iIconlist) iIconlist->ResetAndDestroy();
	delete iIconlist;

	delete iStorage;
	delete iLookupDb;
}

TKeyResponse CContextCallLogAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode /*aType*/)
{
	if ( iTabGroup == NULL )
	{
		return EKeyWasNotConsumed;
	}

	TInt active = iTabGroup->ActiveTabIndex();
	TInt count = iTabGroup->TabCount();

	switch ( aKeyEvent.iCode )
	{
		case EKeyLeftArrow:
			if ( active > 0 )
			{
				active--;
				iTabGroup->SetActiveTabByIndex( active );
				ActivateLocalViewL(TUid::Uid(iTabGroup->TabIdFromIndex(active)));
			}
			break;
		case EKeyRightArrow:
			if( (active + 1) < count )
			{
				active++;
				iTabGroup->SetActiveTabByIndex( active );
				ActivateLocalViewL(TUid::Uid(iTabGroup->TabIdFromIndex(active)));
			}
			break;
		default:
			return EKeyWasNotConsumed;
			break;
	}
	return EKeyWasConsumed;
}

void CContextCallLogAppUi::HandleCommandL(TInt aCommand)
{
	switch ( aCommand )
	{
		case EContextCallLogCmdCrash:
		{
			//TBuf<2> b; b.Append(_L("xxx"));
		}
		case EEikCmdExit:
		{
			Exit();
			break;
		}
		case EAknSoftkeyBack:
		{
			hide();
			break;
		}
		default:
			break;      
	}
}

void CContextCallLogAppUi::HandleGainingForeground()
{
	CALLSTACKITEM_N(_CL("CContextCallLogAppUi"), _CL("HandleGainingForeground"));

	iCallLog->ReRead();

	if (iLog) {
		iLog->write_time();
		iLog->write_to_output(_L("To foreground"));
		iLog->write_nl();
	}
}

void CContextCallLogAppUi::HandleLosingForeground()
{
	CALLSTACKITEM_N(_CL("CContextCallLogAppUi"), _CL("HandleLosingForeground"));

	if (iLog) {
		iLog->write_time();
		iLog->write_to_output(_L("To background\n"));
		iLog->write_nl();
	}
}

void CContextCallLogAppUi::Notify(const TDesC& /*aMessage*/)
{
	CALLSTACKITEM_N(_CL("CContextCallLogAppUi"), _CL("Notify"));
	// no impl	
}

void CContextCallLogAppUi::PresenceChangedL(TInt /*ContactId*/, CBBPresence* /*info*/)
{
	CALLSTACKITEM_N(_CL("CContextCallLogAppUi"), _CL("PresenceChangedL"));
	// no implementation
}

void CContextCallLogAppUi::DisplayPresenceDetailsL(const TDesC& Name, const CBBPresence* PresenceData,
						   TTime aTime)
{
	CALLSTACKITEM_N(_CL("CContextCallLogAppUi"), _CL("DisplayPresenceDetailsL"));

	if (PresenceData == 0) return;
	iPresenceDetailView->SetData(Name, PresenceData, aTime);
	ActivateLocalViewL(KPresenceDetailView);
}


void CContextCallLogAppUi::SetTab(TInt tabId)
{
	CALLSTACKITEM_N(_CL("CContextCallLogAppUi"), _CL("SetTab"));

	iTabGroup->SetActiveTabById(tabId);

}

TBool CContextCallLogAppUi::ProcessCommandParametersL(TApaCommand aCommand, 
	TFileName& aDocumentName, const TDesC8& aTail)
{
	if (aDocumentName.CompareF(_L("hide"))==0) {
		hide();
	}
	aDocumentName.Zero();
	return EFalse;
}

