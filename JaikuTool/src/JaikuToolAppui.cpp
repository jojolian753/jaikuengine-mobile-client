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

#include <avkon.hrh>
#include <aknnotewrappers.h> 

#include "JaikuTool.pan"
#include "JaikuToolAppUi.h"
#include "JaikuToolAppView.h"
#include "JaikuTool.hrh"

#include <sendui.h>
#ifdef __S60V3__
#include <cmessagedata.h>
#endif
#include "context_uids.h"
#include "contextvariant.hrh"

enum TSWStartupReason {
    ESWNone = 100,
};

class SysStartup {
public:
	IMPORT_C static TInt ShutdownAndRestart( const class TUid& aSource, TSWStartupReason aReason);
};

void DEXCL();
void RunConnectorsL();

class CGPS;
CGPS* StartGpsL();
void StopGps(CGPS* gps);

// ConstructL is called by the application framework
void CJaikuToolAppUi::ConstructL()
    {
    BaseConstructL(EAknEnableSkin);

    iAppView = CJaikuToolAppView::NewL(ClientRect());    
#ifndef __WINS__
	RProcess me;
	iDrive=TChar(me.FileName()[0]);
#else
	iDrive=TChar('c');
#endif

#ifndef __S60V3__
	iSendUi = CSendAppUi::NewL(100);
#else
	iSendUi = CSendUi::NewL();
#endif

    AddToStackL(iAppView);
    }

CJaikuToolAppUi::CJaikuToolAppUi()                              
    {
	// no implementation required
    }

#include <errorui.h>
#include <aknglobalnote.h>

TErrorHandlerResponse CJaikuToolAppUi::HandleError(TInt aError,
     const SExtendedError& aExtErr,
     TDes& aErrorText,
     TDes& aContextText) {
     
     	CErrorUI* eui=CErrorUI::NewLC();
		CAknGlobalNote *note=CAknGlobalNote::NewLC();
		const TDesC& msg=eui->TextResolver().ResolveErrorString(aError);
		if (msg.Length()>0) {
			note->ShowNoteL(EAknGlobalErrorNote, msg);
		} else {
			TBuf<30> msg=_L("Error: ");
			msg.AppendNum(aError);
			note->ShowNoteL(EAknGlobalErrorNote, msg);
		}
		CleanupStack::PopAndDestroy(2);
	return ENoDisplay;

     }

CJaikuToolAppUi::~CJaikuToolAppUi()
{
    if (iAppView)
        {
        RemoveFromStack(iAppView);
        delete iAppView;
        iAppView = NULL;
        }
        
	delete iSendUi;
	StopGps(iGPS);
}

#include "context_uids.h"
#include "cc_processmanagement.h"
#include <bautils.h>

void CJaikuToolAppUi::DataDir(TDes& datadir)
{
	datadir.Zero();
#ifndef __S60V3__
	datadir.Append(_L("c:\\system"));
#else
	datadir.Append(iDrive);
	datadir.Append(_L(":"));
#endif
	datadir.Append(_L("\\data\\context\\"));
}

void CJaikuToolAppUi::ResetAllDataL()
{
	StopJaikuL();
	TFileName datadir;
	DataDir(datadir);
	TInt pushed=0;
	CFileMan* fm=CFileMan::NewL(iEikonEnv->FsSession());
	pushed++; CleanupStack::PushL(fm);
	fm->RmDir(datadir);
	CleanupStack::PopAndDestroy(pushed);
}

void CJaikuToolAppUi::StopJaikuL()
{
	ProcessManagement::KillApplicationL(iEikonEnv->WsSession(), KUidContextContacts);	
	ProcessManagement::KillApplicationL(iEikonEnv->WsSession(), KUidcontext_log);
}

void CJaikuToolAppUi::SendLogsL()
{
	StopJaikuL();
	TInt pushed=0;
#ifndef __S60V3__
    CDesCArrayFlat* attach=new (ELeave) CDesCArrayFlat(4);
    pushed++; CleanupStack::PushL(attach);
#else
	CMessageData* data=CMessageData::NewL();
    pushed++; CleanupStack::PushL(data);
#endif

    TInt size=0;
    TBuf<20> names=_L("*.txt");
    TFileName logs;
    for (int j=0; j<2; j++) {
		TInt pushed=0;
		if (j==0) {
        	DataDir(logs);
        } else {
        	DebugLogDir(logs);
        }
    	logs.Append(names);
        CDir *d=0;
        TInt err=iEikonEnv->FsSession().GetDir(logs, KEntryAttNormal, ESortByName, d);
        if (err!=KErrNone && err!=KErrNotFound && err!=KErrPathNotFound) User::Leave(err);
        if (d) {
        	pushed++; CleanupStack::PushL(d);
        }
        if (err==KErrNone && d) {
            for (int i=0; i<d->Count(); i++) {
                const TEntry& e=(*d)[i];
                if (e.iSize>0) {
		    		if (j==0) {
		    			DataDir(logs);
		    		} else {
		    			logs=_L("c:\\logs\\context\\");
		    		}
                    logs.Append(e.iName);
                    RFile f;
                    TInt err=f.Open(iEikonEnv->FsSession(), logs, EFileRead);
                    if (err==KErrNone) {
                        f.Close();
#ifndef __S60V3__
                        attach->AppendL(logs);
#else
						data->AppendAttachmentL(logs);
#endif
                        size+=e.iSize;
                    }
                }
            }
        }
	    CleanupStack::PopAndDestroy(pushed);
	    names=_L("*.*");
    }
    
    TSendingCapabilities c( 0,
            size, TSendingCapabilities::ESupportsAttachments);
#ifndef __S60V3__
    iSendUi->CreateAndSendMessagePopupQueryL(_L("Send log files"), c, 0, attach.get(),
            KNullUid, 0, 0, 0, EFalse);
#else
	iSendUi->ShowQueryAndSendL(data, c, 0, KNullUid, EFalse, _L("Send log files"));
#endif
    CleanupStack::PopAndDestroy(pushed);
}

void CJaikuToolAppUi::DebugLogDir(TDes& aInto)
{
	aInto=_L("c:\\Logs\\Context\\");
}

void CJaikuToolAppUi::SetDebugLogsEnabled(TBool aEnabled)
{
	TFileName dir; DebugLogDir(dir);
	TInt pushed=0;
	if (!aEnabled) {
		StopJaikuL();
		CFileMan* fm=CFileMan::NewL(iEikonEnv->FsSession());
		pushed++; CleanupStack::PushL(fm);
		fm->RmDir(dir);
	} else {
		BaflUtils::EnsurePathExistsL(iEikonEnv->FsSession(), dir);
	}
	CleanupStack::PopAndDestroy(pushed);
}

TBool CJaikuToolAppUi::DebugLogsEnabled()
{
	TFileName dir; DebugLogDir(dir);
	return BaflUtils::FolderExists(iEikonEnv->FsSession(), dir);
}

TBool CJaikuToolAppUi::JaikuDataExists()
{
	TFileName dir; DataDir(dir);
	return BaflUtils::FolderExists(iEikonEnv->FsSession(), dir);
}

#include <es_enum.h>

void CloseConnectionsL()
{
	RSocketServ ss;
	TInt pushed=0;
	User::LeaveIfError(ss.Connect());
	CleanupClosePushL(ss); pushed++;
	RConnection c; 
	User::LeaveIfError(c.Open(ss));
	CleanupClosePushL(c); pushed++;
	TUint connectionCount;
	User::LeaveIfError(c.EnumerateConnections(connectionCount));
	TPckgBuf<TConnectionInfo> connectionInfo;
	for (TUint i = 1; i <= connectionCount; ++i) {
		TInt pushed=0;
		c.GetConnectionInfo(i, connectionInfo);
		RConnection tmp;
		User::LeaveIfError(tmp.Open(ss));
		CleanupClosePushL(tmp); pushed++;
		User::LeaveIfError(tmp.Attach(connectionInfo, RConnection::EAttachTypeNormal));
		User::LeaveIfError(tmp.Stop());
		CleanupStack::PopAndDestroy(pushed);
	}
	CleanupStack::PopAndDestroy(pushed);
}

#include "cu_common.h"
void CJaikuToolAppUi::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane *aMenuPane)
{
	TBool autostart_enabled=ProcessManagement::IsAutoStartEnabled(iEikonEnv->FsSession(), iDrive);
	TBool debuglogs_enabled=DebugLogsEnabled();
	
	TApaTaskList tl(iEikonEnv->WsSession());
	TApaTask task1=tl.FindApp(KUidcontext_log);
	TBool context_log_running=task1.Exists();
	TApaTask task2=tl.FindApp(KUidContextContacts);
	TBool contacts_running=task2.Exists();
	
	SetItemDimmedIfExists(aMenuPane, EJaikuToolCmdDisableAutostart, !autostart_enabled);
	SetItemDimmedIfExists(aMenuPane, EJaikuToolCmdEnableAutostart, autostart_enabled);
	
	SetItemDimmedIfExists(aMenuPane, EJaikuToolDisableDebugLogs, !debuglogs_enabled);
	SetItemDimmedIfExists(aMenuPane, EJaikuToolEnableDebugLogs, debuglogs_enabled);
	
	SetItemDimmedIfExists(aMenuPane, EJaikuToolCmdStopJaikuSettings, !context_log_running);
	SetItemDimmedIfExists(aMenuPane, EJaikuToolCmdStopJaikuContacts, !contacts_running);
	
	SetItemDimmedIfExists(aMenuPane, EJaikuToolCmdResetData, !JaikuDataExists());
}

void TestQosL();
void ShowCellSizeL();

void AllocLots1L() {
	while(true) {
		User::AllocL(10*1024);
	}
}

void AllocLots2L() {
	TBuf<80> name;
	TInt i=0;
	while(true) {
		name=_L("jaikutestalloc");
		name.AppendNum(i);
		RChunk c;
		TInt size=8000;
		TInt err;
		do {
			err=c.CreateGlobal(name, size*1024, size*1024);
			if (err==KErrNoMemory) size/=2;
		} while(err==KErrNoMemory && size>10);
		User::LeaveIfError(err);
		char* p=(char*)c.Base();
		p+=(size-1)*1024;
		*p='a';
		i++;
	}
}

// handle any menu commands
void CJaikuToolAppUi::HandleCommandL(TInt aCommand)
    {
    switch(aCommand)
        {
        case EEikCmdExit:
        case EAknSoftkeyExit:
            Exit();
            break;

		case EJaikuToolCmdStopJaikuSettings:
			ProcessManagement::KillApplicationL(iEikonEnv->WsSession(), KUidcontext_log);
			break;
		case EJaikuToolCmdStopJaikuContacts:
			ProcessManagement::KillApplicationL(iEikonEnv->WsSession(), KUidContextContacts);
			break;
		case EJaikuToolCmdDisableAutostart:
			ProcessManagement::SetAutoStartEnabledL(iEikonEnv->FsSession(), iDrive, EFalse);
			break;
		case EJaikuToolCmdEnableAutostart:
			ProcessManagement::SetAutoStartEnabledL(iEikonEnv->FsSession(), iDrive, ETrue);
			break;
		case EJaikuToolCmdResetData:
			ResetAllDataL();
			break;
		case EJaikuToolCmdSendLogs:
			SendLogsL();
			break;
		case EJaikuToolEnableDebugLogs:
			SetDebugLogsEnabled(ETrue);
			break;
		case EJaikuToolDisableDebugLogs:
			SetDebugLogsEnabled(EFalse);
			break;
		case EJaikuToolReboot:
			User::LeaveIfError(SysStartup::ShutdownAndRestart( KUidJaikuTool, ESWNone ));
			break;
		case EJaikuToolCellSize:
			ShowCellSizeL();
			break;
		case EJaikuToolTestQoS:
			TestQosL();
			break;
		case EJaikuToolDEXC:
			DEXCL();
			break;
		case EJaikuToolCloseConnections:	
			CloseConnectionsL();
			break;
		case EJaikuToolAlloc1:
			AllocLots1L();
			break;
		case EJaikuToolAlloc2:
			AllocLots2L();
			break;
		case EJaikuToolConnectors:
			RunConnectorsL();
			break;
		case EJaikuToolGPS:
			StopGps(iGPS);
			iGPS = StartGpsL();
			break;
        default:
            Panic(EJaikuToolBasicUi);
            break;
        }
    }

void DEXCL()
{
	RProcess p;
	User::LeaveIfError(p.Create(_L("D_EXC.exe"), KNullDesC));
	p.Resume();
	p.Close();
	return;
}

#ifdef __DEV__

class RHeap2 : public RHeap {
public:
	TInt MinCell() { return iMinCell; }
	void SetMinCell(TInt aNewValue) { iMinCell=aNewValue; }
};

_LIT(KHeap, "jaikutoolheap");
void ShowCellSizeL()
{
	RHeap *h=&(User::Heap());
	RHeap2 *h2=(RHeap2*)h;
	TBuf<100> msg=_L("Original minimun heap cell size: ");
	msg.AppendNum(h2->MinCell());
	
	
	CAknGlobalNote* note=CAknGlobalNote::NewL();
	CleanupStack::PushL(note);
	note->ShowNoteL(EAknGlobalInformationNote, msg);
	CleanupStack::PopAndDestroy();
	//h2->SetMinCell(36);
}

#else
void ShowCellSizeL() { }
#endif

#ifdef QOS_TEST
#include <ES_SOCK.H>
#include <cs_subconparams.h>
#include <etel.h>
#include <etelpckt.h>
#include <etelQoS.h>

class CQoSTest : public CActive {
public:
	CQoSTest() : CActive(CActive::EPriorityStandard) { }
	RSocketServ iSS;
	RConnection iConn;
	RSubConnection iSubConn;
	RPhone iPhone;
	RTelServer iTelServer;
	RTelServer::TPhoneInfo iPhoneInfo;
	RPacketService iPacketService;
	void ConstructL() {
		CActiveScheduler::Add(this);
		User::LeaveIfError(iSS.Connect());
		iConn.Open(iSS, KAfInet);
		User::LeaveIfError(iTimer.CreateLocal());
		tries=0;
		User::LeaveIfError(iTelServer.Connect());
		User::LeaveIfError(iTelServer.GetPhoneInfo(0, iPhoneInfo));
		User::LeaveIfError(iPhone.Open(iTelServer, iPhoneInfo.iName));
		TInt err=iPacketService.Open(iPhone);
		User::LeaveIfError(err);
		iConn.Start(iStatus);
		SetActive();
	}
	void ShowStatusL(const TDesC& aMsg) {
		CAknGlobalNote *note=CAknGlobalNote::NewLC();
		
		note->ShowNoteL(EAknGlobalInformationNote, aMsg);
		CleanupStack::PopAndDestroy();
	}
	RPacketService::TContextInfo iContextInfo;
	RPacketContext iPacketContext;
	RPacketQoS iQoS;
	RTimer iTimer;
	TBuf<100> state;
	TInt tries;
	RSocket sock;
	enum TState { EStarting, EGettingInfo,  };
	TState iState;
	TName iQosProfile;
	void RunL() {
		if (iStatus.Int()!=KErrNone) {
			TBuf<30> msg=_L("Error ");
			msg.AppendNum(iStatus.Int());
			msg.Append(_L(" at "));
			msg.AppendNum(iState);
			ShowStatusL(msg);
			return;
			
		}
		switch (iState) {
		case EStarting:
			{
			state=_L("packetservice::getinfo");
			iPacketService.GetContextInfo(iStatus, 0, iContextInfo);
			iState=EGettingInfo;
			SetActive();
			}
			break;
		case EGettingInfo:
			{
			state=_L("openexisting");
			TInt err=iPacketContext.OpenExistingContext(iPacketService, iContextInfo.iName);
			User::LeaveIfError(err);
			state=_L("getprofilename");
			err=iPacketContext.GetProfileName(iQosProfile);
			User::LeaveIfError(err);
			err=iQoS.OpenExistingQoS(iPacketContext, iQosProfile);
			User::LeaveIfError(err);
			}
			break;
		}
	}
	TInt RunError(TInt aError) {
		TBuf<100> msg=_L("Error ");
		msg.AppendNum(aError);
		msg.Append(_L(" at "));
		msg.Append(state);
		TRAPD(ignored, ShowStatusL(msg));
		return KErrNone;
	}
	void SetParametersL() {
	}
	void PrintParametersL() {
	}
	void DoCancel() {
		iConn.Stop();
	}
	~CQoSTest() {
		Cancel();
		iSubConn.Close();
		iConn.Close();
		iSS.Close();
	}
};

void TestQosL()
{
	CQoSTest* o=new (ELeave) CQoSTest;
	o->ConstructL();
}
#else
void TestQosL()
{
}
#endif

#include <ES_SOCK.H>
#include <e32math.h>
#include <CommDbConnPref.h>

class CConnector : public CActive {
public:
	CConnector(TInt64& aSeed, TBool aStart, TInt aId, RFile& aFile) : CActive(EPriorityStandard), 
		iSeed(aSeed), iStart(aStart), iId(aId), iFile(aFile) { }
	RSocketServ iServ;
	RConnection iConnection;
	RTimer	iTimer;
	TInt64& iSeed;
	TInt	iId;
	RFile&	iFile;
	TBuf8<100> iMsgBuf;
	TCommDbConnPref iConnPref;
	enum TState { EIdle, EWaiting, EConnecting, EWaitingToClose };
	TState iState;
	TInt iCount;
	TBool iConnectionOpen;
	TBool iStart;
	
	void Log(const TDesC8& aMsg, TInt aCode=KErrNone) {
		iMsgBuf.Zero();
		iMsgBuf.AppendNumFixedWidth(iId, EDecimal, 4);
		iMsgBuf.Append(_L8(": "));
		iMsgBuf.Append(aMsg.Left(85));
		if (aCode!=0) {
			iMsgBuf.Append(_L8(" "));
			iMsgBuf.AppendNum(aCode);
		}
		iMsgBuf.Append(_L8("\n"));
		iFile.Write(iMsgBuf);
		iFile.Flush();
	}
	void ConstructL(TInt aAp) {
		User::LeaveIfError(iServ.Connect());
		User::LeaveIfError(iTimer.CreateLocal());
		CActiveScheduler::Add(this);
		iConnPref.SetIapId(aAp);
		iConnPref.SetDirection(ECommDbConnectionDirectionOutgoing);
		iConnPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
		iConnPref.SetBearerSet(ECommDbBearerUnknown);
		StartL();
	}
	
	~CConnector() {
		Log(_L8("~CConnector"));
		Cancel();
		CloseConnection();
		iServ.Close();
		iTimer.Close();
	}
	void StartL() {
		Log(_L8("StartL"));
		CloseConnection();
		if (iStart) {
			TReal r=Math::FRand(iSeed);
			TReal waitf=r*1000.0*1000.0;
			TInt wait=waitf;
			iTimer.After( iStatus, wait*10 );
		} else {
			iTimer.After( iStatus, 150*1000 );
		}
		iState=EWaiting;
		SetActive();
	}
	void DoCancel() {
		Log(_L8("DoCancel"));
		switch (iState) {
		case EWaiting:
		case EWaitingToClose:
			Log(_L8("iTimer.Cancel"));
			iTimer.Cancel();
			break;
		case EConnecting:
			CloseConnection();
			break;
		}
	}
	
	TInt RunError(TInt aError) {
		Log(_L8("RunError"), aError);
		TRAPD(err, StartL());
		if (err!=KErrNone) delete this;
		return KErrNone;
	}
	void GetProgress() {
		TNifProgress pr;
		TInt err=iConnection.Progress(pr);
		Log(_L8("Progress"), err);
		if (err==KErrNone) {
			Log(_L8("Progress: iState"), pr.iStage);
			Log(_L8("Progress: iError"), pr.iError);
		}
	}
	void RunL() {
		Log(_L8("RunL"), iStatus.Int());
		if ( iState==EWaiting ) {
			User::LeaveIfError(iConnection.Open(iServ));
			iConnectionOpen=ETrue;
			if (iStart) {
				Log(_L8("iConnection.Start"));
				iConnection.Start(iConnPref, iStatus);
			} else {
				TInt pushed=0;
				RConnection tmp;
				User::LeaveIfError(tmp.Open(iServ));
				CleanupClosePushL(tmp); pushed++;
				TUint connectionCount=0;
				User::LeaveIfError(tmp.EnumerateConnections(connectionCount));
				if (connectionCount==0) {
					Log(_L8("No connections"));
					CleanupStack::PopAndDestroy(pushed);
					StartL();
					return;
				}
				TPckgBuf<TConnectionInfo> connectionInfo;
				User::LeaveIfError(tmp.GetConnectionInfo(1, connectionInfo));
				TInt err=iConnection.Attach(connectionInfo, RConnection::EAttachTypeNormal);
				Log(_L8("Attach"), err);
				if (err==KErrNone) {
					GetProgress();
				}
				CleanupStack::PopAndDestroy(pushed);
				TRequestStatus *s=&iStatus;
				User::RequestComplete(s, err);
			}
			iState=EConnecting;
			SetActive();
		} else if (iState==EConnecting) {
			GetProgress();
			iTimer.After( iStatus, 1*100 );
			iState=EWaitingToClose;
			SetActive();
		} else {
			CloseConnection();
			if (iCount>1000) {
				delete this;
			} else {
				iCount++;
				StartL();
			}
		}
	}
	void CloseConnection() {
		if (! iConnectionOpen) return;
		Log(_L8("iConnection.Close1"));
		User::After(TTimeIntervalMicroSeconds32(100));
		iConnection.Close();
		Log(_L8("iConnection.Close2"));
		iConnectionOpen=EFalse;
	}
};

#include <apsettingshandlerui.h>
#include <commdb.h>

TInt RunConnectorsThread(TAny* aPtr);

void RunConnectorsL()
{
	CApSettingsHandler* settingsHandler = CApSettingsHandler::NewLC(
	                                     ETrue,
	                                     EApSettingsSelListIsPopUp,
	                                     EApSettingsSelMenuSelectOnly,
	                                     KEApIspTypeAll,
	                                     EApBearerTypeAll,
	                                     KEApSortNameAscending);
	TUint32 ap;
	settingsHandler->RunSettingsL(0, ap);
	CleanupStack::PopAndDestroy();
	
	RThread thread;
	User::LeaveIfError(thread.Create(_L("jaikutool2"), 
		&RunConnectorsThread, // thread's main function
		20*1024, /* stack */
		20*1024, /* min heap */
		1024*1024, /* max heap */
		(TAny*)ap,
		EOwnerProcess));
	thread.SetPriority(EPriorityNormal);
	thread.Resume();
	thread.Close();
}

void RunConnectorsThreadL(TInt ap);

TInt RunConnectorsThread(TAny* aPtr)
{
	CTrapCleanup *cl=0;
	cl=CTrapCleanup::New();	
	TRAPD(err, RunConnectorsThreadL((TInt)aPtr));
	delete cl;
	return err;
}

void RunConnectorsThreadL(TInt ap)
{
	CBaActiveScheduler *s=new (ELeave) CBaActiveScheduler;
	CActiveScheduler::Install(s);
	
	TInt64* seed=new TInt64;
	*seed=0;
	RFs* fs=new RFs;
	User::LeaveIfError(fs->Connect());
	RFile* f=new RFile;
	User::LeaveIfError(f->Replace(*fs, _L("c:\\conns.txt"), EFileWrite));
	
	for (int i=0; i<5; i++) {
		CConnector* c=new (ELeave) CConnector(*seed, ETrue, i, *f);
		c->ConstructL(ap);
	}
	/*
	for (int i=0; i<5; i++) {
		CConnector* c=new (ELeave) CConnector(*seed, EFalse, 1000+i, *f);
		c->ConstructL(ap);
	}
	*/
	CActiveScheduler::Start();
	return;
}

#include <lbs.h>

class CGPS : public CActive {
 public:
  static CGPS* NewL() {
    CGPS* ret = new (ELeave) CGPS;
    CleanupStack::PushL(ret);
    ret->ConstructL();
    CleanupStack::Pop();
    return ret;
  }
  ~CGPS() {
    Cancel();
    Close();
    timer_.Close();
    log_.Close();
    fs_.Close();
  }
 private:
  RPositioner positioner_; TBool positioner_open_;
  RPositionServer posserver_; TBool posserver_open_;
  RTimer timer_;
  TPositionInfo position_;
  RFs fs_;
  RFile log_;
  CGPS() : CActive(CActive::EPriorityStandard) { }
  enum State { EIdle, EWaitingForPosition, EWaitingForTimer };
  State state_;
  void DoCancel() {
    if (EWaitingForPosition == state_) {
      positioner_.CancelRequest(EPositionerNotifyPositionUpdate);
    } else {
      timer_.Cancel();
    }
  }
  void ConstructL() {
    CActiveScheduler::Add(this);
    User::LeaveIfError(timer_.CreateLocal());
    User::LeaveIfError(fs_.Connect());
    User::LeaveIfError(log_.Replace(fs_, _L("c:\\data\\gpslog.txt"), EFileWrite));
    StartListeningL();
  }
  void LogMessage(const TDesC8& msg) {
    TBuf8<20> ts;
    TTime nowt; nowt.HomeTime();
    TDateTime now = nowt.DateTime();
    ts.AppendNumFixedWidth(now.Hour(), EDecimal, 2);
    ts.Append(_L8(":"));
    ts.AppendNumFixedWidth(now.Minute(), EDecimal, 2);
    ts.Append(_L8(":"));
    ts.AppendNumFixedWidth(now.Second(), EDecimal, 2);
    ts.Append(_L8(" "));
    log_.Write(ts);
    log_.Write(msg);
    log_.Write(_L8("\n"));
  }
  void StartListeningL() {
    Close();
    User::LeaveIfError(posserver_.Connect()); posserver_open_ = ETrue;
#ifdef __WINS__
    TPositionModuleId module;
    User::LeaveIfError(posserver_.GetDefaultModuleId(module));
#else
    const TPositionModuleId module = TUid::Uid(270526860); // A-GPS
#endif
    User::LeaveIfError(positioner_.Open(posserver_, module)); positioner_open_ = ETrue;
    User::LeaveIfError(positioner_.SetRequestor(
        CRequestor::ERequestorService,
        CRequestor::EFormatApplication,
        _L("testapp")));
    
    positioner_.SetUpdateOptions(
        TPositionUpdateOptions(
            TTimeIntervalMicroSeconds(60 * 1000 * 1000),
            TTimeIntervalMicroSeconds(45 * 1000 * 1000),
            TTimeIntervalMicroSeconds(0),
            EFalse));
            
    positioner_.NotifyPositionUpdate(position_, iStatus);
    SetActive();
    state_ = EWaitingForPosition;
    LogMessage(_L8("Started request "));
  }
  void RunL() {
    if (EWaitingForPosition == state_) {
      LogMessage(_L8("Completed request "));
      Close();
#ifdef __WINS__
      timer_.After(iStatus, TTimeIntervalMicroSeconds32(2 * 1000 * 1000));
#else
      timer_.After(iStatus, TTimeIntervalMicroSeconds32(120 * 1000 * 1000));
#endif
      SetActive();
      state_ = EWaitingForTimer;
    } else {
      StartListeningL();
    }
  }
  TInt RunError(TInt error) {
    return error;
  }
  void Close() {
    if (positioner_open_) {
      positioner_.Close();
      positioner_open_ = EFalse;
    }
    if (posserver_open_) {
      posserver_.Close();
      posserver_open_ = EFalse;
    }
  }
};

CGPS* StartGpsL() {
  return CGPS::NewL();
}

void StopGps(CGPS* gps) {
  delete gps;
}
