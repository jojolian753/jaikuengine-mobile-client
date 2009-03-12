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

#include "ContextbookAppUi.h"
#include "ContextbookView.h"
#include "ContextbookView2.h"
#include <contextbook.rsg>
#include "contextbook.hrh"
#include <eikmenup.h>
#include <avkon.hrh>
#include "db.h"
#include <RPbkViewResourceFile.h>
#include <CPbkContactEditorDlg.h>
#include <CPbkContactEngine.h>
#include <CPbkSelectFieldDlg.h>
#include <CPbkContactItem.h>

#include <eikspane.h> 
#include <eiklabel.h>
#include <aknindicatorcontainer.h> 


#include <logwrap.h>
#include <logcli.h>

#include <aknenv.h> 

#include "icons.h"
_LIT(KTestPresence, "<presence><event><datetime>20040428T195102 </datetime><location><location.value><location.network>RADIOLINJA</location.network><location.lac>9006</location.lac><location.cellid>18</location.cellid></location.value></location></event><event><datetime>20040428T195102 </datetime><base><base.current>ESTI1</base.current></base></event><event><datetime>20040428T195057 </datetime><useractivity><useractivity.value>active</useractivity.value></useractivity></event><event><datetime>20040428T195057 </datetime><profile><profile.value><profile.id>0</profile.id><profile.name>General</profile.name></profile.value></profile></event></presence>");

// ================= MEMBER FUNCTIONS =======================
//
// ----------------------------------------------------------
// CContextbookAppUi::ConstructL()
// ?implementation_description
// ----------------------------------------------------------
//
void CContextbookAppUi::ConstructL()
{
	CALLSTACKITEM(_L("CContextbookAppUi::ConstructL"));

	RThread me;
	me.Open(me.Id());
	me.SetExceptionHandler(exceptionhandler, 
		KExceptionAbort|KExceptionFault|KExceptionFpe|KExceptionInteger|
		KExceptionKill|KExceptionUserInterrupt);
	me.Close();

	/*CLogEvent * ev = CLogEvent::NewL();
	
	ev->SetRemoteParty(_L("test renaud"));
	ev->SetNumber(_L("1234567890"));
	ev->SetDirection(_L("Outgoing"));
	ev->SetEventType(KLogCallEventTypeUid);

	CLogClient * logclient = CLogClient::NewL(CEikonEnv::Static()->FsSession());
	TRequestStatus aStatus;
	logclient->AddEvent(*ev,aStatus);
	*/
	
	BaseConstructL();

	iconlist = new (ELeave) CAknIconArray(30);
	LoadIcons(iconlist);

	TBool logging=ETrue;
	Settings().GetSettingL(18, logging);
	// FIXME
	iLog=Cfile_output_base::NewL(AppContext(), _L("book_log"), logging, true);

	if (!logging && iLog) iLog->paused=true;

	iCaptionBook=CEikonEnv::Static()->AllocReadResourceL(R_CB_BOOK_CAPTION);
	iCaptionLog=CEikonEnv::Static()->AllocReadResourceL(R_CB_LOG_CAPTION);

	iJabberDb=CDb::NewL(AppContext(), _L("JABBER"), EFileWrite|EFileShareAny);
	iJabberData=CJabberData::NewL(AppContext(), *iJabberDb);

	iPresenceHolder=CPresenceHolder::NewL(*iJabberData);

	book=new (ELeave) phonebook(AppContext(), *iJabberData, *iPresenceHolder);
	book->ConstructL();
	if (iPresenceHolder) iPresenceHolder->AddListener(book);
	
	if (iPresenceHolder) iPresenceHolder->AddListener(this);

	iPresenceUpdater = new (ELeave) CPresenceUpdater(*book);
	iPresenceUpdater->ConstructL();

	log=new (ELeave) call_log;
	log->ConstructL();
	
	view1 = new (ELeave) CContextbookView(*iJabberData, iLog);
	
	CleanupStack::PushL( view1 );
	view1->ConstructL(KViewId, book, true, *iCaptionBook, iconlist);
	AddViewL( view1 );      // transfer ownership to CAknViewAppUi
	CleanupStack::Pop();    // view1
	
	view2 = new (ELeave) CContextbookView(*iJabberData, iLog);
	
	CleanupStack::PushL( view2 );
	view2->ConstructL(KView2Id, log, false, *iCaptionLog, iconlist);
	AddViewL( view2 );      // transfer ownership to CAknViewAppUi
	CleanupStack::Pop();    // view2
	
	auto_ptr<CPresenceDetailView> detailview(CPresenceDetailView::NewL());
	AddViewL(detailview.get());
	iPresenceDetailView=detailview.release();

	SetDefaultViewL(*view1);
	//SetDefaultViewL(*view2);

	iEikonEnv->AddForegroundObserverL(*this);

}

// ----------------------------------------------------
// CContextbookAppUi::~CContextbookAppUi()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CContextbookAppUi::~CContextbookAppUi()
{
	CALLSTACKITEM(_L("CContextbookAppUi::~CContextbookAppUi"));

	iEikonEnv->RemoveForegroundObserver(*this);

	delete iLog;

	delete iconlist;

	delete book;
	delete log;
	delete iPresenceHolder;
	delete iPresenceUpdater;
	delete iJabberData;
	delete iJabberDb;

	delete iCaptionBook;
	delete iCaptionLog;
}

// ------------------------------------------------------------------------------
// CContextbookAppUi::::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
//  This function is called by the EIKON framework just before it displays
//  a menu pane. Its default implementation is empty, and by overriding it,
//  the application can set the state of menu items dynamically according
//  to the state of application data.
// ------------------------------------------------------------------------------
//
void CContextbookAppUi::DynInitMenuPaneL( TInt /*aResourceId*/,
					  CEikMenuPane* /*aMenuPane*/)
{
	CALLSTACKITEM(_L("CContextbookAppUi::DynInitMenuPaneL"));

	

}

// ----------------------------------------------------
// CContextbookAppUi::HandleKeyEventL(
//     const TKeyEvent& aKeyEvent,TEventCode /*aType*/)
// ?implementation_description
// ----------------------------------------------------
//
TKeyResponse CContextbookAppUi::HandleKeyEventL(
						const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
{
	CALLSTACKITEM(_L("CContextbookAppUi::HandleKeyEventL"));

	return EKeyWasNotConsumed;
}

// ----------------------------------------------------
// CContextbookAppUi::HandleCommandL(TInt aCommand)
// ?implementation_description
// ----------------------------------------------------
//
void CContextbookAppUi::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM(_L("CContextbookAppUi::HandleCommandL"));

	switch ( aCommand )
        {
        case EEikCmdExit:
		view1->before_exit();
		view2->before_exit();
		Exit();
		break;
        
		case EAknSoftkeyBack:
		hide();
		break;
	case EcontextbookCmdLog:
		log->refresh();
		ActivateLocalViewL(KView2Id);
		break;

	case EcontextbookCmdBook:
		ActivateLocalViewL(KViewId);
		break;

	case EcontextbookCmdPresenceTest:
		{
			RThread me; me.Open(me.Id());
			me.Panic(_L("BOOK"), 100);

			/*
			TTime now;
			now.HomeTime();
			
			iPresenceHolder->NewPresence(_L("mikie@jabber.org"), KTestPresence, now);
			*/
		break;
		}
	case EcontextbookCmdEditor:
		{
			ShowEditor(true);
			break;
		}
	case EcontextbookCmdCreate:
		{
			ShowEditor(false);
			break;
		}
        default:
		break;      
        }
}

void CContextbookAppUi::ShowEditor(bool existing)
{
	CALLSTACKITEM(_L("CContextbookAppUi::ShowEditor"));

	CCoeEnv *env = CEikonEnv::Static();
	RPbkViewResourceFile pbkRes( *env);
	pbkRes.OpenL();
	CleanupClosePushL(pbkRes);

	CPbkContactEngine *ipPabEng=book->get_engine();

	CPbkContactItem* aContactItem=0;
	TBool create_new;
	if (existing) {
		aContactItem= ipPabEng->OpenContactLCX ( book->GetContactId( view1->GetCurrentIdx() ) );
		create_new=EFalse;
	} else {
		aContactItem=ipPabEng->CreateEmptyContactL();
		CleanupStack::PushL(aContactItem);
		create_new=ETrue;
	}

	// launch the contacts dialog
	CPbkContactEditorDlg *ipPabDlg =
	CPbkContactEditorDlg::NewL(*ipPabEng, *aContactItem, create_new);

	ipPabDlg->SetMopParent( this );

	TInt res = KErrNone;
	
	TRAPD( err, res = ipPabDlg->ExecuteLD());

	if (existing) {
		CleanupStack::PopAndDestroy(3); //pbkRes, aContactItem, contact lock
	} else {
		CleanupStack::PopAndDestroy(2); //pbkRes, aContactItem
	}
}

void CContextbookAppUi::hide()
{
	CALLSTACKITEM(_L("CContextbookAppUi::hide"));

	RWsSession& wsSession=CEikonEnv::Static()->WsSession();
	
	TApaTask task(wsSession);
	task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
	task.SendToBackground();
}

void CContextbookAppUi::HandleGainingForeground()
{
	CALLSTACKITEM(_L("CContextbookAppUi::HandleGainingForeground"));

	if (iLog) {
		iLog->write_time();
		iLog->write_to_output(_L("To foreground"));
		iLog->write_nl();
	}
	
	log->refresh();
	
	if (iPresenceUpdater) iPresenceUpdater->Start();
}

void CContextbookAppUi::HandleLosingForeground()
{
	CALLSTACKITEM(_L("CContextbookAppUi::HandleLosingForeground"));

	if (iLog) {
		iLog->write_time();
		iLog->write_to_output(_L("To background\n"));
		iLog->write_nl();
	}
	view1->ResetSearchField();

	if (iPresenceUpdater) iPresenceUpdater->Stop();

}

void CContextbookAppUi::Notify(const TDesC& aMessage)
{
	CALLSTACKITEM(_L("CContextbookAppUi::Notify"));

#ifdef __WINS__
	iEikonEnv->BusyMsgCancel();
	
	if (aMessage.Compare(_L("")) != 0 )
	{
		iEikonEnv->BusyMsgL(aMessage,TGulAlignment(EHRightVTop));
	}
#endif
		
}

void CContextbookAppUi::PresenceChangedL(TInt /*ContactId*/, MPresenceData &/*info*/)
{
	CALLSTACKITEM(_L("CContextbookAppUi::PresenceChangedL"));

	// no implementation
}

void CContextbookAppUi::DisplayPresenceDetailsL(const TDesC& Name, const MPresenceData* PresenceData)
{
	CALLSTACKITEM(_L("CContextbookAppUi::DisplayPresenceDetailsL"));

	if (PresenceData == 0) return;
	iPresenceDetailView->SetData(Name, PresenceData);
	ActivateLocalViewL(KPresenceDetailView);
}
// End of File  
