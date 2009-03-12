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

#pragma warning(disable: 4706)

// INCLUDE FILES
#include "ContextLocaAppUi.h"
#include <contextloca.rsg>
#include <sendnorm.rsg>
#include "local_defaults.h"
#include "context_log.hrh"
#include "contextlocaapp.h"

#define CONTEXTNW 1

#include <avkon.hrh>
#include <aknquerydialog.h> 
#include <eikenv.h> // ieikonenv

#include <eikmenup.h>
#include <apgcli.h>

#include <bautils.h>

#include <apgwgnam.h>

#include "cl_settings.h"

#include "break.h"

#include "notifystate.h"
#include <contextloca.mbg>
#include "viewids.h"

#include "raii_apgcli.h"
#include "raii_f32file.h"

#include "loca_sender.h"
#include "sms_status.h"
#include "callstack.h"
#include "reporting.h"
#include "app_context_impl.h"

#include "contextnotifyclientsession.h"

#include "bbl_runner.h"

#include "cu_errorui.h"
#include "emptytext.h"
#include "file_logger.h"
#include "csd_battery.h"
#include "csd_system.h"
#include "transfer.h"
#include "context_logcontainer.h"
#include "call_recorder.h"
#include <eikdll.h>
#include "mediaprompt.h"
#include "ntpconnection.h"
#include "cellnaming.h"
#include "settingsview.h"
#include "sms_snapshot.h"
#include "loca_errorlog.h"
#include "dumper.h"
#include "csd_loca.h"
#include "cdb.h"
#include "ap_creation.h"
#include "symbian_auto_ptr.h"
#include "cc_imei.h"

_LIT(cellid_file, "cellid_names_v3.txt");
_LIT(transfer_cellid_file, "cellid_names_trans_v3.txt");

const TComponentName KListener = { { CONTEXT_UID_CONTEXTNETWORK}, 1 };
const TTupleName KListenerStop = { { CONTEXT_UID_CONTEXTNETWORK}, 2 };
#include "loca_logic.h"

//#include <syevdef.h>

enum JOYSTICK_EVENTS {
	JOY_LEFT = 0xF807,
	JOY_RIGHT = 0xF808,
	JOY_UP = 0xF809,
	JOY_DOWN = 0xF80A,
	JOY_CLICK = 0xF845
};

const TUid KUidPhone = { 0x100058b3 };
const TUid KUidMenu = {  0x101f4cd2 };

#ifndef __WINS__
_LIT(KIconFile, "c:\\system\\data\\contextloca.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\contextloca.mbm");
#endif

#ifdef __WINS__
class TDummyMsvObserver : public MMsvSessionObserver
{
	void HandleSessionEvent(TMsvSessionEvent, TAny*, TAny*, TAny*) { }
	void HandleSessionEventL(TMsvSessionEvent, TAny*, TAny*, TAny*) { }
};
#endif


typedef TInt32 TContactItemId;
class CProfileAPI : public CBase {
public:
	enum TProErrorCode
	    {
	    EPro0=0
	   ,EPro1
	    };
	IMPORT_C static CProfileAPI* NewL(TBool);
	IMPORT_C virtual ~CProfileAPI();
	IMPORT_C TProErrorCode SetProfileName( TInt aTableId );
};

void SetSilentL()
{
	auto_ptr<CProfileAPI> a(CProfileAPI::NewL(EFalse));
	a->SetProfileName(1);
}

void CContextLocaAppUi::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("ConstructL"));

#ifndef __WINS__
	bool wins=false;
	{
		TInt err;
		TRAP(err, SetSilentL());
	}
#else
	bool wins=true;
	TInt ignore;
	TBreakItem i(GetContext(), ignore);

#if 0
	BBSession()->DeleteL(KListener, ETrue);
	BBSession()->DeleteL(KLocaErrorTuple, 
		KNullDesC, ETrue);
	BBSession()->DeleteL(KListenerStop, 
		KNullDesC, ETrue);
	BBSession()->DeleteL(KLocaScriptTuple, 
		KNullDesC, ETrue);
	BBSession()->DeleteL(KLocaMessageStatusTuple, 
		KNullDesC, ETrue);

	//Settings().WriteSettingL(SETTING_CONTEXTNW_HOST, _L("10.1.0.1"));
	Settings().WriteSettingL(SETTING_CONTEXTNW_HOST, _L("loca.hiit.fi"));
	Settings().WriteSettingL(SETTING_CONTEXTNW_PORT, 5000);
	Settings().WriteSettingL(SETTING_CONTEXTNW_ENABLED, ETrue);
	Settings().WriteSettingL(SETTING_PUBLISH_AUTHOR,
		_L("Loca@South hall"));
	Settings().WriteSettingL(SETTING_PUBLISH_PASSWORD,
		_L("emulator"));
	Settings().WriteSettingL(SETTING_BT_SCAN_INTERVAL, 5);
	Settings().WriteSettingL(SETTING_ENABLE_LOCA_BLUEJACK, ETrue);
	Settings().WriteSettingL(SETTING_LOCA_BLUEJACK_MESSAGE_TIMEOUT, 90);
	Settings().WriteSettingL(SETTING_LOCA_BLUEJACK_CONNECT_COUNT, 7);
#endif

#endif

	CContextLogAppUiBase::ConstructL();
	state=_L("subscribeL");

#ifndef DONT_LOG_EVENTS_TO_FILE

	iLog->SubscribeL(KCellIdTuple);
#ifdef __WINS__
	iLog->SubscribeL(KBluetoothTuple);
	iLog->SubscribeL(KLocaMessageStatusTuple);
#endif
	iLog->SubscribeL(KOwnBluetoothTuple);
	iLog->SubscribeL(KBatteryTuple);
	iLog->SubscribeL(KNetworkTuple);
	iLog->SubscribeL(KChargerTuple);
	iLog->SubscribeL(KAppEventTuple);

#else
	// we always want errors to be logged
	iLog->SubscribeL(KAppEventTuple);
#endif

	state=_L("bbl");

	{
		TBool logging_enabled;
		Settings().GetSettingL(SETTING_LOGGING_ENABLE, logging_enabled);
		TInt notif_err;
		CC_TRAP(notif_err, iLoggingRunning=CNotifyState::NewL(AppContext(), KIconFile));
		if (logging_enabled) {
			if(iLoggingRunning) iLoggingRunning->SetCurrentState( EMbmContextlocaL, EMbmContextlocaL );
		} else {
			if(iLoggingRunning) iLoggingRunning->SetCurrentState( EMbmContextlocaL_not, EMbmContextlocaL_not );
		}
	}

	state=_L("LocaSender");
	iLocaSender=CLocaSender::NewL(AppContext());

	state=_L("notify");

	Settings().NotifyOnChange(SETTING_LOGGING_ENABLE, this);

	state=_L("sms status");
	if (smsh) iSmsStatus=CSmsStatusReplier::NewL(AppContext(), smsh);

	ConstructAfterPresenceMaintainerL();

	TInt ap;
	TRAPD(err, ap=CreateAPL(_L("cingular"),
		_L("WAP.CINGULAR"),
		_L("WAP@CINGULARGPRS.COM"),
		_L("CINGULAR1")));
#ifndef __WINS__
	if (err==KErrNone) 
		Settings().WriteSettingL(SETTING_IP_AP, ap);
#endif

#ifndef __WINS__
	TRAP(err, SyncTimeL(EFalse));
	if (err!=KErrNone) {
		TBuf<50> msg=_L("Failed to sync time: ");
		msg.AppendNum(err);
		LogAppEvent(msg);
	}
#else
	TimeSynced(ETrue);
#endif

	state=_L("create transferer");
	transferer=CSendUITransfer::NewL(AppContext(), this, 
		Econtext_logCmdSendAppUi, DataDir(), AppDir());
	transferer->add_filesL(_L("log*txt"), true);
	transferer->add_filesL(_L("starter*txt"), true);
	transferer->add_filesL(_L("book*txt"), true);
	transferer->add_filesL(_L("calllog*txt"), true);
	transferer->add_filesL(_L("comm*txt"), false);
	transferer->add_filesL(_L("rec*amr"), false);
	transferer->add_filesL(_L("cellid_names_trans.txt"), false);
	transferer->add_filesL(_L("mms*.*"), false);

	state=_L("create settings view");

	{
		auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KSettingsViewId, AppContext(), 
			TLocalAppSettings::GetEnabledSettingsArray()));
		AddViewL(iSettingsView.get());
		iSettingsView.release();
	}

	FinalConstructL();
/*
	TBBLocaMsgStatus msg;
	msg.iAtTime()=GetTime();
	msg.iMessageId()=15;
	msg.iRecipientAddress=TBBBluetoothAddress( TPtrC8((TUint8*)"\0\0\0\0\0\x01", 6),
		_L("myphone"));
	TTime exp; exp=GetTime(); exp+=TTimeIntervalHours(12);
	BBSession()->PutL(KLocaMessageStatusTuple, KNullDesC,
		&msg, exp);
	CErrorLogger* e=CErrorLogger::NewL();
	e->LogFormatted(_L("Example of a terrible error"));
	delete e;
	CBBDumper* d=CBBDumper::NewL();
*/
}

// ----------------------------------------------------
// CContextLocaAppUi::~CContextLocaAppUi()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CContextLocaAppUi::~CContextLocaAppUi()
{
	CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("~CContextLocaAppUi"));
	TInt err;

	{
		CLocalNotifyWindow::Destroy();

		TTime t2;
		t2=GetTime();
		if (iLog) 
			CC_TRAP(err, iLog->new_value(CBBSensorEvent::VALUE, _L("APP close"), state, t2));


		Settings().CancelNotifyOnChange(SETTING_LOGGING_ENABLE, this);
		Settings().CancelNotifyOnChange(SETTING_OWN_DESCRIPTION, this);

	}


	{
		TTime t; t=GetTime();
		if (iLog) CC_TRAP(err,
			iLog->new_value(CBBSensorEvent::VALUE, _L("APP"), _L("3"), t));
		delete transferer;
	}


	{
		CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("~CContextLocaAppUi_d1"));
		delete iSmsStatus;
	}
	{
		CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("~CContextLocaAppUi_d2"));
		delete iBBLocal;
	}
	{
		CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("~CContextLocaAppUi_d4"));
		delete bbl;
	}

	{
		CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("~CContextLocaAppUi_d6"));
		delete iContextRunning;
	}
	{
		CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("~CContextLocaAppUi_d5"));
		delete iLoggingRunning;
	}

	{
		CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("~CContextLocaAppUi_d7"));
		// near the end, since the cancelling doesn't work
		// right on s60v1 :-(
		delete iLocaSender;
	}

}


// ------------------------------------------------------------------------------
// CContextLocaAppUi::::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
//  This function is called by the EIKON framework just before it displays
//  a menu pane. Its default implementation is empty, and by overriding it,
//  the application can set the state of menu items dynamically according
//  to the state of application data.
// ------------------------------------------------------------------------------
//
void CContextLocaAppUi::DynInitMenuPaneL(
					 TInt aResourceId,CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("DynInitMenuPaneL"));

	switch(aResourceId) {
	case R_OTHERS_MENU:
#ifndef REALLY_ONLY_LOGGING
		if (iLog) {
			if (iLog->is_paused()) {
				aMenuPane->SetItemDimmed(Econtext_logCmdAppPauseLog, ETrue);	
				aMenuPane->SetItemDimmed(Econtext_logCmdAppUnPauseLog, EFalse);
			} else {
				aMenuPane->SetItemDimmed(Econtext_logCmdAppPauseLog, EFalse);	
				aMenuPane->SetItemDimmed(Econtext_logCmdAppUnPauseLog, ETrue);
			}
		} else {
			aMenuPane->SetItemDimmed(Econtext_logCmdAppPauseLog, ETrue);	
			aMenuPane->SetItemDimmed(Econtext_logCmdAppUnPauseLog, ETrue);
		}
#endif
		break;

	case R_USER_ACTIONS:

		aMenuPane->SetItemDimmed(Econtext_logCmdAppSuspendPresence, ETrue);	
		aMenuPane->SetItemDimmed(Econtext_logCmdAppResumePresence, ETrue);
		break;
	case R_SENDUI_TOPMENU:
		transferer->DisplaySendMenuL(*aMenuPane, 1);
		break;
	case R_SENDUI_MENU:
		// sendui cascade
		transferer->DisplayMenuL(*aMenuPane);
		break;
	default:
		break;
	}
}



void CContextLocaAppUi::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CContextLocaAppUi"), _CL("HandleCommandL"));

	SetInHandlableEvent(ETrue);
#ifdef __WINS__
	TInt err;
	TBreakItem b(GetContext(), err);
#endif
	if (BaseHandleCommandL(aCommand)) return;

	switch ( aCommand )
	{
	case Econtext_logCmdAppPauseLog:
		Settings().WriteSettingL(SETTING_LOGGING_ENABLE, EFalse);
		break;
	case Econtext_logCmdAppUnPauseLog:
		Settings().WriteSettingL(SETTING_LOGGING_ENABLE, ETrue);
		break;
	case Econtext_logCmdCancelSend:
		// do nothing
		break;
	case Econtext_logCmdAppSettings:
		ActivateLocalViewL(KSettingsViewId);
		break;
	case Econtext_logCmdDumpCommDb:
		{
		CCommDbDump* dump=CCommDbDump::NewL();
		dump->DumpDBtoFileL(_L("c:\\commdb.txt"));
		delete dump;
		}
		break;
	case Econtext_logCmdCreateAp:
		{
		CreateAPL(_L("cingular"),
			_L("WAP.CINGULAR"),
			_L("WAP@CINGULARGPRS.COM"),
			_L("CINGULAR1"));
		}
		break;
	case Econtext_logCmdStartSensors:
		{
			if  (!iSensorRunner)
				iSensorRunner=CSensorRunner::NewL(
					AppContext(), smsh, EFalse, *this);
		}
		break;

	case Econtext_logCmdAppTest:
		{
		//loc->test();
		//recorder->test();
		DialogTest();
		}
		break;
	case Econtext_logCmdAppImsi:
		{
			/*if (loc) status_change(loc->GetImsi());
			else */{
#ifndef __WINS__
				TBuf<20> machineId;
				GetImeiL(machineId);
				status_change(machineId);
#else
				// Return a fake IMEI when working on emulator
				_LIT(KEmulatorImsi, "244050000000000");
				status_change(KEmulatorImsi);
#endif
			}
		}
			
		break;
	default:
		if (aCommand>Econtext_logCmdSendAppUi || aCommand==Econtext_logCmdSendFtp) {
			
			if (aCommand==Econtext_logCmdSendFtp) {
				//iPeriodicTransfer->Transfer(false);
				//iPeriodicTransfer->Transfer(true);
			} else {
				status_change(_L("trying transfer"));

				/*
				cellid_name_file.Close();
				*/
				
				TFileName transfer_cellid_filen, cellid_filen;
				transfer_cellid_filen.Format(_L("%S%S"), &AppDir(), &transfer_cellid_file);
				cellid_filen.Format(_L("%S%S"), &DataDir(), &cellid_file);

				TInt ferr=BaflUtils::CopyFile(Fs(), cellid_filen, transfer_cellid_filen);
				if (ferr!=KErrNone) {
					TBuf<30> errmsg;
					errmsg.Format(_L("copy: %d"), ferr);
					error(errmsg);
					return;
				}
				/*
				ferr=cellid_name_file.Open(Fs(), cellid_filen, 
					EFileShareAny | EFileStreamText | EFileRead | EFileWrite);
				if (ferr!=KErrNone) {
					TBuf<30> errmsg;
					errmsg.Format(_L("reopen: %d, RESTART NOW"), ferr);
					error(errmsg);
					return;
				}
				*/
				iLog->switch_file();

				transferer->transfer_files(aCommand);
			}
			
		}
		break;
	}
}


void CContextLocaAppUi::DialogTest()
{
	return;
}

void CContextLocaAppUi::SettingChanged(TInt Setting)
{
	if (Setting==SETTING_LOGGING_ENABLE) {
		TApaTaskList tl(Ws());
		TApaTask booktask=tl.FindApp(KUidcontextbook);
		if (booktask.Exists()) {
			booktask.SendSystemEvent(EApaSystemEventShutdown);
		}
		TBool logging;
		Settings().GetSettingL(SETTING_LOGGING_ENABLE, logging);
		if (iLoggingRunning) {	
			if (!logging) {
				iLoggingRunning->SetCurrentState( EMbmContextlocaL_not, EMbmContextlocaL_not );
			} else {
				iLoggingRunning->SetCurrentState( EMbmContextlocaL, EMbmContextlocaL );
			}
		}

	}
}

void CContextLocaAppUi::TimeSynced(TBool aSuccess)
{
	TTime t; t=GetTime();
	if (aSuccess) {
		delete iNtpTimeOut; iNtpTimeOut=0;
		Settings().WriteSettingL(SETTING_LAST_TIMESYNC, t);
		if (!bbl) {
			LogAppEvent(_L("Starting 1.6"));
			bbl=CBBLogger::NewL(AppContext(),this);
		}
	}
	if (!iSensorRunner) {
		if (aSuccess) {
			iSensorRunner=CSensorRunner::NewL(
				AppContext(), smsh, EFalse, *this);
		} else {
			if (! iNtpTimeOut ) iNtpTimeOut=CTimeOut::NewL(*this);
			iNtpTimeOut->Wait(5);
		}
	}
}

#include "bb_listener.h"

void CContextLocaAppUi::expired(CBase* aSource)
{
	if (aSource==iNtpTimeOut) {
		iNtpCount++;
		if (iNtpCount>10) User::Leave(KContextErrTimeoutInBBProtocol);
		SyncTimeL(EFalse);
	} else {
		CContextLogAppUiBase::expired(aSource);
	}
}

// End of File  
