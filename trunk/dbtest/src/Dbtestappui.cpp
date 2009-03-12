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

#include "DbtestAppUi.h"
#include "DbtestContainer.h" 
#include <dbtest.rsg>
#include "dbtest.hrh"
#include "load.h"

#include <profileapi.h>
#include <etel.h>
#include <etelbgsm.h>
#include <avkon.hrh>
#include <CFLDRingingTonePlayer.h>
#include <D32RCHG.H>

#include <in_sock.h>
#include <es_sock.h>
#include <aknquerydialog.h> 

#include "symbian_tree.h"
#include "routes_test.h"
#include "cdb.h"

#include "symbian_auto_ptr.h"
#include "cert.h"

class GenKey : public MBtreeKey {
public:
	virtual void Between(const TAny* aLeft,const TAny* aRight,TBtreePivot& aPivot) const { }
	virtual TInt Compare(const TAny* aLeft,const TAny* aRight) const { return 0; }
	virtual const TAny* Key(const TAny* anEntry) const { return anEntry; }
};

class Entry {
public:
	uint32	key;
	void*	data;
};

// ================= MEMBER FUNCTIONS =======================
//
// ----------------------------------------------------------
// CDbtestAppUi::ConstructL()
// ?implementation_description
// ----------------------------------------------------------
//
void CDbtestAppUi::ConstructL()
{
	BaseConstructL();
	iAppContainer = new (ELeave) CDbtestContainer;
	iAppContainer->SetMopParent(this);
	iAppContainer->ConstructL( ClientRect() );

	/*
	transferer=new (ELeave) transfer(CEikonEnv::Static()->FsSession());
	transferer->ConstructL(this);

	bt=new (ELeave) Ctransfer;
	bt->ConstructL(this);

	discoverer=new (ELeave) discover(CEikonEnv::Static()->FsSession());
	discoverer->ConstructL(this);
	*/

	/*
	ftp=CFtp::NewL(*this);
	wap=CWap::NewL(AppContext(), *this);
	*/

	/*
	factory=CMapFactory::NewL(CEikonEnv::Static()->FsSession());
	routes=CRoutes::NewL(factory, 4);

	t=routes_test::NewL(routes, factory, iAppContainer);
	t->run_test();
	*/

	AddToStackL( iAppContainer );
}

// ----------------------------------------------------
// CDbtestAppUi::~CDbtestAppUi()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CDbtestAppUi::~CDbtestAppUi()
{
	if (iAppContainer)
        {
		RemoveFromStack( iAppContainer );
		delete iAppContainer;
        }
	delete t;
	if (is_open) {
		sock.CancelAll();
		sock.Close();
		sockserv.Close();
	}
	//delete ftp;
	//delete wap;
	delete routes;
	delete factory;
}

// ------------------------------------------------------------------------------
// CDbtestAppUi::::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
//  This function is called by the EIKON framework just before it displays
//  a menu pane. Its default implementation is empty, and by overriding it,
//  the application can set the state of menu items dynamically according
//  to the state of application data.
// ------------------------------------------------------------------------------
//
void CDbtestAppUi::DynInitMenuPaneL(
				    TInt /*aResourceId*/,CEikMenuPane* /*aMenuPane*/)
{
}

// ----------------------------------------------------
// CDbtestAppUi::HandleKeyEventL(
//     const TKeyEvent& aKeyEvent,TEventCode /*aType*/)
// ?implementation_description
// ----------------------------------------------------
//
TKeyResponse CDbtestAppUi::HandleKeyEventL(
					   const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
{
	return EKeyWasNotConsumed;
}

// ----------------------------------------------------
// CDbtestAppUi::HandleCommandL(TInt aCommand)
// ?implementation_description
// ----------------------------------------------------
//
void CDbtestAppUi::HandleCommandL(TInt aCommand)
{
	switch ( aCommand )
        {
        case EAknSoftkeyBack:
        case EEikCmdExit:
		{
			Exit();
			break;
		}
	case EdbtestCmdAppOBEX:
		//bt->transfer_logs();
		break;
        case EdbtestCmdAppTest:
		{
#if 0
			if (!is_open) {
				User::LeaveIfError(sockserv.Connect());
				User::LeaveIfError(sock.Open(sockserv, KAfInet, KSockStream, KUndefinedProtocol));
				TInetAddr a(INET_ADDR(128, 214, 48, 81) , 80);

				TRequestStatus s;
				sock.Connect(a, s);
				User::WaitForRequest(s);
				status_change(_L("opened"));
				is_open=true;
			} else {
				sock.CancelAll();
				sock.Close();
				sockserv.Close();
				status_change(_L("closed"));
				is_open=false;
			}
#else
#  if 0
			//transferer->log_gps();
			/*
			TInetAddr a(INET_ADDR(128, 214, 48, 81) , 21);
			ftp->Connect(a, _L8("tkt_cntx"), _L8("dKFJmqBi"));
			current_state=CONNECTING;
			*/
			/*
			run(this);
			*/
#  else
			//wap->Connect(1, _L("http://db.cs.helsinki.fi/~mraento/cgi-bin/put.pl"));
#  endif

#endif

		}
		break;
		
	case EdbtestCmdAppCommDb:
		{
			CCommDbDump* dump=CCommDbDump::NewL();
			CleanupStack::PushL(dump);
			dump->DumpDBtoFileL(_L("c:\\commdb.txt"));
			CleanupStack::PopAndDestroy();
		}
		break;
	case EdbtestCmdAppCert:
		{
			auto_ptr<CCertInstaller> i(CCertInstaller::NewL(AppContext()));
			i->InstallCertL(_L("c:\\hy.der"));
		}
	case EdbtestCmdAppDiscover:
		//discoverer->search();
		break;
	case EdbtestCmdAppCtmGSM:
		{
		TBuf<40> s;
		RDevRecharger c;
		TInt ret=0;
		TInt u=0;
		bool done=false;
		while (!done) {
			ret=c.Open(u);
			if (ret==KErrNone) {
				done=true;
			} else {
				++u;
				if (u==KNullUnit) done=true;
			}
		}
		if (ret!=KErrNone) {
			s.Format(_L("Open: %d"), ret);
		} else {
			TChargeInfoV1 i;
			i.iRawTemperature=i.iSmoothedTemperature=0;
			i.iChargeType=EChargeNone;
			TChargeInfoV1Buf b(i);
			c.ChargeInfo(b);
			s.Format(_L("%d r %d s %d t %d"), u, i.iRawTemperature, i.iSmoothedTemperature, i.iChargeType)	;
		}
		status_change(s);
		}
		break;

	case EdbtestCmdAppVibra:
		CFLDRingingTonePlayer* p;
		p=CFLDRingingTonePlayer::NewL(ETrue);
		p->SetVibra(ETrue);
		p->SetRingingType(0); 
		//((MFLDFileProcessor*)p)->ProcessFileL(_L("c:\\nokia\\sounds\\simple\\silent.rng"));
		((MFLDFileProcessor*)p)->ProcessFileL(_L("c:\\system\\apps\\context_log\\silent.rng"));
		break;
        default:
		break;      
        }
}

void CDbtestAppUi::success(CBase* source)
{ 
	TBuf<30> msg;
	msg.Format(_L("success %d"), current_state);
	if (current_state==CONNECTING) {
		//ftp->Retrieve(_L("README"));
		//ftp->Store(_L("s.txt"));
		//TInt size=40000;
		//CAknNumberQueryDialog* d=CAknNumberQueryDialog::NewL(size);
		//d->ExecuteLD(R_DBTEST_INPUT);
		//wap->Store(_L("s.txt"));
		current_state=RETRIEVING;
	} else if (current_state==RETRIEVING) {
		current_state=CLOSING;
		//ftp->Close();
		//wap->Close();
	}
	status_change(msg); 
}
void CDbtestAppUi::finished()
{
	_LIT(got, "Got comm log");
	iAppContainer->set_status(got);
}

void CDbtestAppUi::error(const TDesC& descr)
{	
	//globalNote->ShowNoteL(EAknGlobalInformationNote, descr	);
	iAppContainer->set_error(descr);
}


void CDbtestAppUi::status_change(const TDesC& status)
{
	//globalNote->ShowNoteL(EAknGlobalInformationNote, status);
	iAppContainer->set_status(status);
}

// End of File  
