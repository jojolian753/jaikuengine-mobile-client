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
#include "Context_logAppUi.h"

#include "contextlog_resource.h"

#ifndef __S60V3__
#include <sendnorm.rsg>
#endif

#include "local_defaults.h"
#include "context_log.hrh"
#include "context_logapp.h"

#include "contextvariant.hrh"
#define CONTEXTNW 1
//#undef CONTEXTNW
//#undef LOCA

#include <avkon.hrh>
#include <aknquerydialog.h> 
#include <eikenv.h> // ieikonenv

#include <eikmenup.h>
#include <apgcli.h>

#include <bautils.h>

#include <RPbkViewResourceFile.h>
#include <CPbkContactEditorDlg.h>
#include <CPbkContactEngine.h>
#include <CPbkSelectFieldDlg.h>
#include <CPbkContactItem.h>

#include <apgwgnam.h>

#include "userview.h"
#include "presencedetailview.h"
#include "presencedescriptionview.h"

#include "cl_settings.h"

#include "break.h"

#include "hideview.h"

#include "notifystate.h"
#include <context_log.mbg>
#include "viewids.h"

#include "raii_apgcli.h"
#include "raii_f32file.h"

#include "loca_sender.h"
#include "sms_status.h"
#include "callstack.h"
#include "reporting.h"
#include "app_context_impl.h"
#include "presence_ui_helper.h"

#include "contextnotifyclientsession.h"

#include "csd_sms.h"

#include "dumper.h"

#ifndef CONTEXTJAIKU
#include "bbl_runner.h"
#endif

#include "cu_errorui.h"
#include "emptytext.h"
#include "file_logger.h"
#include "csd_battery.h"
#include "csd_system.h"
#include "csd_current_app.h"
#include "bt_dev_ap_view.h"
#include "log_comm.h"
#include "transfer.h"
#include "context_logcontainer.h"
#ifndef __S60V3__
#ifndef __S60V2FP3__
#include "call_recorder.h"
#endif
#endif
#include <eikdll.h>
#include "mediaprompt.h"
#include "ntpconnection.h"
#include "settingsview.h"
#include "sms_snapshot.h"

#include "csd_presence.h"
#include "alerter.h"
#ifndef __S60V3__
#include "cdb.h"
#endif
#include "autoap.h"
#include "cu_sendmsg.h"
#include "cc_imei.h"
#include "statusview.h"
#include <aknmessagequerydialog.h> 
#include "sleep.h"
#include "csd_md5hash.h"
#include "cu_common.h"

#include "cc_processmanagement.h"
#ifdef __S60V3__
#include <sendui.h>
#include <cmessagedata.h>
#endif

_LIT(cellid_file, "cellid_names_v3.txt");
_LIT(transfer_cellid_file, "cellid_names_trans_v3.txt");

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
_LIT(KIconFile, "c:\\system\\data\\context_log.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\context_log.mbm");
#endif

#ifdef __WINS__
class TDummyMsvObserver : public MMsvSessionObserver
{
	void HandleSessionEvent(TMsvSessionEvent, TAny*, TAny*, TAny*) { }
	void HandleSessionEventL(TMsvSessionEvent, TAny*, TAny*, TAny*) { }
};
#include "contextcommon.h"
#endif

void CContext_logAppUi::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("ConstructL"));

#if defined(__WINS__) && defined(__S60V3__)
	User::__DbgMarkStart(RHeap::EUser);
	User::__DbgMarkEnd(RHeap::EUser, 0);
#endif

#ifndef __WINS__
	bool wins=false;
#else
	bool wins=true;
#endif

#ifdef __WINS__
	//Settings().WriteSettingL(SETTING_JABBER_PASS, KNullDesC);
	//Settings().WriteSettingL(SETTING_JABBER_NICK, _L("jyri"));
	//Settings().WriteSettingL(SETTING_JABBER_PASS_SHA1, _L("3654030b1feb7f371d8efdedeb345fd4db54c3d2"));
#endif

	CContextLogAppUiBase::ConstructL();


#ifndef DONT_LOG_EVENTS_TO_FILE

	iLog->SubscribeL(KCellIdTuple);
	iLog->SubscribeL(KCellNameTuple);
	iLog->SubscribeL(KCityNameTuple);
	iLog->SubscribeL(KCountryNameTuple);
	iLog->SubscribeL(KLocationTuple);
	iLog->SubscribeL(KBaseTuple);
	iLog->SubscribeL(KProfileTuple);
	iLog->SubscribeL(KBluetoothTuple);
	iLog->SubscribeL(KOwnBluetoothTuple);
	iLog->SubscribeL(KGpsTuple);
	iLog->SubscribeL(KAlarmTuple);
	iLog->SubscribeL(KUnreadTuple);
	iLog->SubscribeL(KCalendarTuple);
	iLog->SubscribeL(KIdleTuple);
	iLog->SubscribeL(KBatteryTuple);
	iLog->SubscribeL(KNetworkTuple);
	iLog->SubscribeL(KChargerTuple);
	iLog->SubscribeL(KCurrentAppTuple);
	iLog->SubscribeL(KAppEventTuple);
	iLog->SubscribeL(KSmsTuple);
	iLog->SubscribeL(KUserGivenContextTuple);

#else
	// we always want errors to be logged
	iLog->SubscribeL(KAppEventTuple);
#endif

#ifndef CONTEXTJAIKU
#if CONTEXTNW
	state=_L("bbl");
	bbl=CBBLogger::NewL(AppContext(),this);
#endif

	iBBLocal=CBBLocalRunner::NewL(AppContext(), this);
	//iSocketTestReader=CSocketTestReader::NewL(AppContext(), 2000, _L("c:\\bblocal.xml"));
#endif

#ifndef CONTEXTJAIKU
	{
		TBool logging_enabled;
		Settings().GetSettingL(SETTING_LOGGING_ENABLE, logging_enabled);
		TInt notif_err;
		CC_TRAP(notif_err, iLoggingRunning=CNotifyState::NewL(AppContext(), KIconFile));
		if (logging_enabled) {
			if(iLoggingRunning) iLoggingRunning->SetCurrentState( EMbmContext_logL, EMbmContext_logL );
		} else {
			if(iLoggingRunning) iLoggingRunning->SetCurrentState( EMbmContext_logL_not, EMbmContext_logL_not );
		}
	}
#endif

#ifdef LOCA
	state=_L("LocaSender");
	iLocaSender=CLocaSender::NewL(AppContext());
#endif

	ringing=false;

	iCaptionMyContext=CEikonEnv::Static()->AllocReadResourceL(R_ME_CAPTION);
	
	state=_L("notify");

	Settings().NotifyOnChange(SETTING_LOGGING_ENABLE, this);

	//__UHEAP_MARK;

#ifndef __JAIKU__
#if DO_SMS_STATUS
	state=_L("sms status");
	if (smsh) iSmsStatus=CSmsStatusReplier::NewL(AppContext(), smsh);
#endif
#endif


#ifndef NO_PRESENCE
	state=_L("iUserContextLog");

	iUserContextLog=CCircularLog::NewL(1,ETrue);
	
	CEikonEnv::Static()->ReadResourceAsDes16(prev, R_PREVIOUS);
	CEikonEnv::Static()->ReadResourceAsDes16(not_avail, R_JABBER_NOT_AVAIL);
	CEikonEnv::Static()->ReadResourceAsDes16(ago, R_AGO);

	iUserContext=HBufC::NewL(iCaptionMyContext->Length()+256);
	PresenceToListBoxL(0, iUserContext, iCaptionMyContext, 0, KNullDesC, GetTime(),
		prev, ago, not_avail, EFalse, ETrue);
	iUserContextLog->AddL(*iUserContext);

#endif


#ifndef NO_PRESENCE
	state=_L("create presence publisher");
	LogAppEvent(state);
	CallStackMgr().ResetCallStack();
	iPresenceMaintainer=CPresenceMaintainer::NewL(AppContext(),
        iBuddyBTs, iLaptopBTs, iDesktopBTs, iPDABTs);
        
	state=_L("created presence publisher");
	LogAppEvent(state);

	LogAppEvent(_L("Starting 2.2"));
#endif

	CallStackMgr().ResetCallStack();
	ConstructAfterPresenceMaintainerL();

	TBool enable_options=false;

#if 0
#ifndef NO_PRESENCE
	state=_L("create user view");
	auto_ptr<CUserView> userview(CUserView::NewL(iUserContextLog, iPresenceMaintainer));
	AddViewL(userview.get());
	CUserView* v=userview.release();
	iUserView=v;
#  ifdef REALLY_ONLY_LOGGING
	SetDefaultViewL(*v);
#  else
	if ( ! Settings().GetSettingL(SETTING_OPTIONS_ENABLE, enable_options) || !enable_options) {
		SetDefaultViewL(*v);
	}
#  endif
#endif

#endif // NO_PRESENCE

#ifdef NO_PRESENCE
	state=_L("create hide view");
	auto_ptr<CHideView> Hideview(CHideView::NewL());
	AddViewL(Hideview.get()); 
	if ( Settings().GetSettingL(SETTING_OPTIONS_ENABLE, enable_options) && enable_options) {
		Hideview.release();
	} else {
		SetDefaultViewL(*(Hideview.release()));
	}
#endif


#ifndef NO_PRESENCE
	state=_L("create presence detail");
	auto_ptr<CPresenceDetailView> detailview(CPresenceDetailView::NewL());
	AddViewL(detailview.get());
	iPresenceDetailView=detailview.release();

	state=_L("create presence description");
	auto_ptr<CPresenceDescriptionView> Descriptionview(CPresenceDescriptionView::NewL());
	AddViewL(Descriptionview.get());
	iPresenceDescriptionView=Descriptionview.release();

	{
		auto_ptr<CBTDevApView> btView(CBTDevApView::NewL(AppContext(), 
			iBuddyBTs, KBtDevSetBuddiesViewId, &(iStatusView->iNextViewId), EFalse));
		AddViewL(btView.get());
		btView.release();
	}
	{
		auto_ptr<CBTDevApView> btView(CBTDevApView::NewL(AppContext(), 
			iLaptopBTs, KBtDevSetLaptopsViewId, &(iStatusView->iNextViewId), EFalse));
		AddViewL(btView.get());
		btView.release();
	}
	{
		auto_ptr<CBTDevApView> btView(CBTDevApView::NewL(AppContext(), 
			iPDABTs, KBtDevSetPDAsViewId, &(iStatusView->iNextViewId), EFalse));
		AddViewL(btView.get());
		btView.release();
	}
	{
		auto_ptr<CBTDevApView> btView(CBTDevApView::NewL(AppContext(), 
			iDesktopBTs, KBtDevSetDesktopsViewId, 
			&(iStatusView->iNextViewId), EFalse));
		AddViewL(btView.get());
		btView.release();
	}

#endif //NO_PRESENCE

	state=_L("create commlog");
	comml=Clog_comm::NewL(AppContext(), this);

	if (wins) 
		iAppContainer->SubscribeL(KIdleTuple);

#ifdef __JAIKU__
#define NO_PERIODIC_TRANSFER 1
#endif

#ifndef __JAIKU__
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

#ifndef NO_PERIODIC_TRANSFER
	iPeriodicTransfer=CPeriodicTransfer::NewL(AppContext(), 24, this, iTransferDir);
#endif

	iSmsSnapshot=CSmsSnapshot::NewL(AppContext(), this, iPresenceMaintainer, 
		iTransferDir->Transferer());
	smsh->AddHandler(iSmsSnapshot);
	/*
	answerer=Canswering::NewL(AppContext(), this);
	lp->add_sinkL(answerer);
	iAppContainer->CurrentLoc()->add_sinkL(answerer);
	*/


#  ifndef NO_CALLRECORDING
#    ifndef __S60V3__
	state=_L("create call recorders");
	in_recorder=CCall_record::NewL(AppContext(), this, CCall_record::INCOMING);
	out_recorder=CCall_record::NewL(AppContext(), this, CCall_record::OUTGOING);
#    endif
#  endif
#else // !JAIKU
#  ifndef __S60V3__
	iSendUi = CSendAppUi::NewL(Econtext_logCmdSoftkeyUpload, 0);
#  else
	iSendUi = CSendUi::NewL();
#  endif
#endif // __JAIKU__

	state=_L("create settings view");

	{
		auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KSettingsViewId, AppContext(), 
			TLocalAppSettings::GetEnabledSettingsArray()));
		AddViewL(iSettingsView.get());
		iSettingsView.release();
	}

	// Set access point automatically 
	// JAIKU we don't want to set it, because it might pick WAP access point
	// SetAccessPointAutomaticallyL();

	FinalConstructL();

#ifdef __WINS__
	//DialogTest();
#endif
}

void CContext_logAppUi::ShowSettings()
{
	TVwsViewId v( KUidcontext_log, KSettingsViewId);
	ActivateViewL(v);
}

class CMediaTest : public CActive, MContextBase, MUploadCallBack {
public:
	TInt iCount;
	MUploadPrompt* iPrompt;
	CMediaTest(MApp_context& aContextBase, MUploadPrompt* pr) : CActive(CActive::EPriorityIdle),
		MContextBase(aContextBase), iPrompt(pr), iCount(392) { }
	void ConstructL() {
		CActiveScheduler::Add(this);
		Async();
		pic=_L("c:\\media\\pic.jpg");
	}
	~CMediaTest() {
		Cancel();
	}
	void Async() {
		if (IsActive()) return;
		TRequestStatus *s=&iStatus;
		User::RequestComplete(s, KErrNone);
		SetActive();
	}
	void RunPrompt() {
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::EDeterministic, iCount);
		iPrompt->Prompt(fn, this);
	}
	void RunL() {
		User::__DbgSetAllocFail(RHeap::EUser, RHeap::ENone, 1);
		TBuf<100> msg=_L("CMediaTest, round: ");
		iCount++;
		msg.AppendNum(iCount);
		if (iCount==66) {
			TInt x;
			x=0;
		}
		Reporting().DebugLog(msg);
		fn=_L("c:\\media\\pic"); fn.AppendNum(iCount); fn.Append(_L(".jpg"));
		User::LeaveIfError(BaflUtils::CopyFile(Fs(), pic, fn));
		CC_TRAPD(err, RunPrompt());
		msg=_L("CMediaTest, round: " );
		msg.AppendNum(iCount);
		if (err!=KErrNone) {
			msg.Append(_L("got error from prompt: "));
			msg.AppendNum(err);
			//Async();
		} else {
			msg.Append(_L("success in prompt"));
		}
		Reporting().DebugLog(msg);
	}
	TFileName fn, pic;
	TFileOpStatus st;
	virtual TFileOpStatus Back(bool Upload, bool DeleteFromPhone, MBBData* Packet) {
		st.fn=fn;
		st.err=KErrNone;
		TBuf<100> msg=_L("CMediaTest, round: ");
		iCount++;
		msg.Append(_L(" got to Back"));
		Reporting().DebugLog(msg);
		Async();
		return st;
	}
	void DoCancel() { }
};

CContext_logAppUi::~CContext_logAppUi()
{
	CC_TRAPD(err, ReleaseCContext_logAppUi());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}

void CContext_logAppUi::ReleaseCContext_logAppUi()
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("~CContext_logAppUi"));
	TInt err;

	{
		CLocalNotifyWindow::Destroy();

		delete iMediaTest;
		delete iDumper;
		delete iSendUi;

		TTime t2;
		t2=GetTime();
		if (iLog) {
			CC_TRAP(err, iLog->new_value(CBBSensorEvent::VALUE, _L("APP close"), state, t2));
		}


		Settings().CancelNotifyOnChange(SETTING_LOGGING_ENABLE, this);
		Settings().CancelNotifyOnChange(SETTING_OWN_DESCRIPTION, this);

		if (iTestFileOpen) iTestFile.Close();

		delete comml;

		delete iCaptionMyContext;
		
		//delete lt;
		if (t) {
			t->Kill(0);
			delete t;
		}

	}


	{
		TTime t; t=GetTime();
		if (iLog) CC_TRAP(err,
			iLog->new_value(CBBSensorEvent::VALUE, _L("APP"), _L("3"), t));
#ifndef NO_PERIODIC_TRANSFER
		delete iPeriodicTransfer;
#endif
		delete transferer;
		delete iSmsSnapshot;
	}

	{
		delete iPresenceMaintainer;
		delete iUserContextLog;
		delete iUserContext;
	}

	TTime t=GetTime();
	{

		if (iLog) CC_TRAP(err,
			iLog->new_value(CBBSensorEvent::VALUE, _L("APP"), state, t));
	}
	{
		//delete answerer;
#ifndef __S60V3__
		delete in_recorder;
		delete out_recorder;
#endif
	}


	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("~CContext_logAppUi_d1"));
		delete iSmsStatus;
	}
#ifndef CONTEXTJAIKU
	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("~CContext_logAppUi_d2"));
		delete iBBLocal;
	}
	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("~CContext_logAppUi_d3"));
		delete iSocketTestReader;
	}
	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("~CContext_logAppUi_d4"));
		delete bbl;
	}
#endif

	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("~CContext_logAppUi_d6"));
		delete iContextRunning;
	}
	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("~CContext_logAppUi_d5"));
		delete iLoggingRunning;
	}

	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("~CContext_logAppUi_d7"));
		// near the end, since the cancelling doesn't work
		// right on s60v1 :-(
		delete iLocaSender;
	}

}

void CContext_logAppUi::SendLogsL()
{
#ifdef __S60V3__
	auto_ptr<CMessageData> data(CMessageData::NewL());
#else
	auto_ptr<CDesCArrayFlat> attach(new (ELeave) CDesCArrayFlat(4));
#endif
	
	LogAppEvent(_L("Switching log file for sending"));
	iLog->switch_file();
	LogAppEvent(_L("Switched log file for sending"));

	TInt size=0;
	TBuf<20> names=_L("log*.txt");
	for (int j=0; j<3; j++) {
		TFileName logs=DataDir();
		if (logs.Right(1).Compare(_L("\\"))) {
			logs.Append(_L("\\"));
		}
		logs.Append(names);
		CDir *d=0;
		User::LeaveIfError(Fs().GetDir(logs, KEntryAttNormal, ESortByName, d));
		auto_ptr<CDir> dp(d);
		for (int i=0; i<d->Count(); i++) {
			const TEntry& e=(*d)[i];
			if (e.iSize>0) {
				logs=DataDir();
				if (logs.Right(1).Compare(_L("\\"))) {
					logs.Append(_L("\\"));
				}
				logs.Append(e.iName);
				RFile f;
				TInt err=f.Open(Fs(), logs, EFileRead);
				if (err==KErrNone) {
					f.Close();
#ifndef __S60V3__
					attach->AppendL(logs);
#else
					//FIXME3RD: we should use AppendAttachmentHandleL instead
					//but it's too undocumented for the moment
					data->AppendAttachmentL(logs);
#endif
					size+=e.iSize;
				}
			}
		}
		if (j==0) {
			names=_L("starter*.txt");
		} else if ( j==1 ) {
			names=_L("book_log*.txt");
		}
	}
#ifndef __S60V3__
	//FIXME3RD
	TSendingCapabilities c( 0, 
		size, TSendingCapabilities::ESupportsAttachments);
	iSendUi->CreateAndSendMessagePopupQueryL(_L("Send log files"), c, 0, attach.get(), 
		KNullUid, 0, 0, 0, EFalse);
#else
	iSendUi->ShowQueryAndSendL(data.get(), KCapabilitiesForAllServices,
		0, KNullUid, EFalse, _L("Send log files"));
#endif
}

// ------------------------------------------------------------------------------
// CContext_logAppUi::::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
//  This function is called by the EIKON framework just before it displays
//  a menu pane. Its default implementation is empty, and by overriding it,
//  the application can set the state of menu items dynamically according
//  to the state of application data.
// ------------------------------------------------------------------------------
//
void CContext_logAppUi::DynInitMenuPaneL(
					 TInt aResourceId,CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("DynInitMenuPaneL"));

	switch(aResourceId) {
	case R_OTHERS_MENU:
#ifndef __JAIKU__
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
#endif
		break;
#ifdef __JAIKU__
	case R_SETTINGS_MENU:
		if ( BaflUtils::FileExists(Fs(), _L("c:\\system\\data\\context\\disable_autostart.dat")) ) {
			aMenuPane->SetItemDimmed(Econtext_logCmdDisableAutostart, ETrue);
			aMenuPane->SetItemDimmed(Econtext_logCmdEnableAutostart, EFalse);
		} else {
			aMenuPane->SetItemDimmed(Econtext_logCmdDisableAutostart, EFalse);
			aMenuPane->SetItemDimmed(Econtext_logCmdEnableAutostart, ETrue);
		}
		{
			TBuf8<50> addr;
			if (Settings().GetSettingL(SETTING_GPS_BT_ADDR, addr) && addr.Length()==6) {
				aMenuPane->SetItemDimmed(Econtext_logCmdResetGPS, EFalse);
			} else {
				aMenuPane->SetItemDimmed(Econtext_logCmdResetGPS, ETrue);
			}
		}
		break;
#endif

	case R_CONTEXT_LOG_MENU:
		{
			SetItemDimmedIfExists(aMenuPane, Econtext_logCmdMediaPool, !iMediaRunner);
		}
		break;

	case R_USER_ACTIONS:

#if 0
#ifndef REALLY_ONLY_LOGGING
		if (iPresencePublisher) {
			if (iPresencePublisher->ConnectionSuspended()) {
				aMenuPane->SetItemDimmed(Econtext_logCmdAppSuspendPresence, ETrue);	
				aMenuPane->SetItemDimmed(Econtext_logCmdAppResumePresence, EFalse);
			} else {
				aMenuPane->SetItemDimmed(Econtext_logCmdAppSuspendPresence, EFalse);	
				aMenuPane->SetItemDimmed(Econtext_logCmdAppResumePresence, ETrue);
			}
		} else {
			aMenuPane->SetItemDimmed(Econtext_logCmdAppSuspendPresence, ETrue);	
			aMenuPane->SetItemDimmed(Econtext_logCmdAppResumePresence, ETrue);
		}
#endif
#endif
		break;
#ifndef __S60V3__
//FIXME3RD
	case R_SENDUI_TOPMENU:
		transferer->DisplaySendMenuL(*aMenuPane, 1);
		break;
	case R_SENDUI_MENU:
		// sendui cascade
		transferer->DisplayMenuL(*aMenuPane);
		break;
#endif
	default:
		break;
	}
}

void MoveProfileL(TBool aRestore)
{
#ifndef __S60V3__
	ProcessManagement::KillExeL(_L("c:\\system\\programs\\cl_starter.exe"));
#endif

	RFs fs; TInt err;
	if ( (err=fs.Connect()) != KErrNone) {
		User::Panic(_L("Cannot connect to filesystem"), err);
	}
	CleanupClosePushL(fs);
	CFileMan* fm=0;
	TRAP(err, fm=CFileMan::NewL(fs));
	if ( err!=KErrNone ) {
		User::Panic(_L("Cannot create file manager"), err);
	}
	CleanupStack::PushL(fm);

	if (! aRestore) {
		err=fm->RmDir(_L("c:\\system\\data\\context_old\\"));
		if (err!=KErrNone && err!=KErrNotFound && err!=KErrPathNotFound) {
			User::Panic(_L("Cannot delete previous old profile"), err);
		}
		TTime wait_until; wait_until.HomeTime(); wait_until+=TTimeIntervalSeconds(15);
		for(;;) {
			if ( (err=fs.Rename(_L("c:\\system\\data\\context"), _L("c:\\system\\data\\context_old"))) != KErrNone) {
				TTime now; now.HomeTime();
				if (now<wait_until) {
					User::After(TTimeIntervalMicroSeconds32(500*1000));
				} else {
					break;
				}
			}
		}
		if (err!=KErrNone && err!=KErrNotFound) {
			User::Panic(_L("Cannot rename profile"), err);
		}
	} else {
		TTime wait_until; wait_until.HomeTime(); wait_until+=TTimeIntervalSeconds(15);
		for(;;) {
			if ( (err=fm->RmDir(_L("c:\\system\\data\\context\\")))!= KErrNone) {
				TTime now; now.HomeTime();
				if (now<wait_until) {
					User::After(TTimeIntervalMicroSeconds32(500*1000));
				} else {
					break;
				}
			}
		}
		if (err!=KErrNone) {
			User::Panic(_L("Cannot delete profile"), err);
		}
		wait_until.HomeTime(); wait_until+=TTimeIntervalSeconds(15);
		for(;;) {
			err=fs.Rename(_L("c:\\system\\data\\context_old"), _L("c:\\system\\data\\context"));
			if (err!=KErrNone) {
				TTime now; now.HomeTime();
				if (now<wait_until) {
					User::After(TTimeIntervalMicroSeconds32(500*1000));
				} else {
					break;
				}
			}
		}
		if (err!=KErrNone && err!=KErrNotFound && err!=KErrPathNotFound) {
			User::Panic(_L("Cannot rename old profile"), err);
		}
	}
	CleanupStack::PopAndDestroy(2);
}


void MoveProfile()
{
	TRAPD(err, MoveProfileL(EFalse));
	if (err!=KErrNone) {
		User::Panic(_L("Failed to rename profile"), err);
	}
}
void RestoreProfile()
{
	TRAPD(err, MoveProfileL(ETrue));
	if (err!=KErrNone) {
		User::Panic(_L("Failed to rename profile"), err);
	}
}



void CContext_logAppUi::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("HandleCommandL"));
	
#ifdef __WINS__
	// set to false to debug error reporting
	SetInHandlableEvent(ETrue);
#else
	SetInHandlableEvent(ETrue);
#endif

#ifdef __WINS__
	TInt dummy;
	TBreakItem b(GetContext(), dummy);
#endif
	if (BaseHandleCommandL(aCommand)) return;

	switch ( aCommand )
	{
	case Econtext_logCmdContacts:
#ifndef __S60V3__
// FIXME3RD
		{
			auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\phonebook\\phonebook.app")));
			cmd->SetCommandL(EApaCommandRun);
			CC_TRAPD(err, EikDll::StartAppL(*cmd));
		}
#endif
		break;
	case Econtext_logCmdLogs:
#ifndef __S60V3__
// FIXME3RD
		{
			auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\logs\\logs.app")));
			cmd->SetCommandL(EApaCommandRun);
			CC_TRAPD(err, EikDll::StartAppL(*cmd));
		}
#endif
		break;
	case Econtext_logPresenceDetails:
		PresenceDetails();
		break;
	case Econtext_logPresenceDescription:
		PresenceDescription();
		break;

	case Econtext_logCmdAppPauseLog:
		Settings().WriteSettingL(SETTING_LOGGING_ENABLE, EFalse);
		break;
	case Econtext_logCmdAppUnPauseLog:
		Settings().WriteSettingL(SETTING_LOGGING_ENABLE, ETrue);
		break;
	case Econtext_logCmdAppGetCommLog:
		{
		if (comml->write_comm_log()) {
			_LIT(getting, "Getting comm log");
			iAppContainer->set_status(getting);
		} else {
			_LIT(no, "Nothing to do");
			iAppContainer->set_status(no);
		}
		break;
		}
	case Econtext_logCmdBTBuddies:
		ActivateLocalViewL(KBtDevSetBuddiesViewId);
		break;
	case Econtext_logCmdBTPDAs:
		ActivateLocalViewL(KBtDevSetPDAsViewId);
		break;
	case Econtext_logCmdBTLaptops:
		ActivateLocalViewL(KBtDevSetLaptopsViewId);
		break;
	case Econtext_logCmdBTDesktops:
		ActivateLocalViewL(KBtDevSetDesktopsViewId);
		break;
	case Econtext_logCmdBT_Dev_AP:
		User::Leave(KErrNotSupported);
		break;
	case Econtext_logCmdCancelSend:
		// do nothing
		break;
	case Econtext_logCmdAppSettings:
		ActivateLocalViewL(KSettingsViewId);
		break;
	case Econtext_logCmdAppTest:
		{
		//loc->test();
		//recorder->test();
		DialogTest();
		}
		break;
	case Econtext_logCmdAppCrash:
		{
		TBuf<2> buf; buf.Append(_L("xxxx"));
		}
		break;
	case Econtext_logCmdSoftkeyUpload:
		SendLogsL();
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
	case Econtext_logMyContext:
		ActivateLocalViewL(KUserViewId);
		break;
	case Econtext_logCmdAppSuspendPresence:
		{
			LogAppEvent(_L("User suspended presence"));
			Settings().WriteSettingL(SETTING_PRESENCE_ENABLE, 0);
		}
		break;
	case Econtext_logCmdAppResumePresence:
		{
			LogAppEvent(_L("User resumed presence"));
			Settings().WriteSettingL(SETTING_PRESENCE_ENABLE, 1);
		}
		break;
	case Econtext_logCmdSetUserGiven:
		{
			SetUserGiven();
		}
		break;
#if 0
//FIXME3RD
	case Econtext_logCmdFreezePresence:
		LogAppEvent(_L("FreezePresence"));
		if (iPresencePublisher) iPresencePublisher->FreezeL();
		if (iUserView) iUserView->SetFrozen(ETrue);
		break;
	case Econtext_logCmdUnFreezePresence:
		LogAppEvent(_L("UnFreezePresence"));
		if (iPresencePublisher) iPresencePublisher->UnFreezeL();
		if (iUserView) iUserView->SetFrozen(EFalse);
		break;
#endif
	case Econtext_logCmdInvite:
		{
		TBuf<100> url=_L("http://jaiku.com/invite?from=");
		TBuf<50> nick;
		Settings().GetSettingL(SETTING_JABBER_NICK, nick);
		url.Append(nick);
		SendMessageWithSendUiL(_L("Invite Buddy"), url);
		}
		break;
	case Econtext_logCmdResetAll:
		{
		HBufC * message = CEikonEnv::Static()->AllocReadResourceLC(R_SURE_TO_RESET);
		CAknQueryDialog * dlg = CAknQueryDialog::NewL(CAknQueryDialog::ENoTone);
		CleanupStack::PushL(dlg);
		dlg->SetPromptL(*message);
		CleanupStack::Pop(dlg);
		if ( dlg->ExecuteLD(R_CL_CONFIRMATION_QUERY_DIALOG) ) {
			TApaTaskList tl(iEikonEnv->WsSession());
			TApaTask app_task=tl.FindApp( KUidContextContacts );
			if (app_task.Exists()) app_task.EndTask();
			User::After(2);
			AppContext().RunOnShutdown(MoveProfile);
			Exit();
		} else {
		} 
		CleanupStack::PopAndDestroy(); //message
		}
		break;

	default:
		if (aCommand>Econtext_logCmdSendAppUi || aCommand==Econtext_logCmdSendFtp) {
			
			if (aCommand==Econtext_logCmdSendFtp) {
				//iPeriodicTransfer->Transfer(false);
#ifndef NO_PERIODIC_TRANSFER
				iPeriodicTransfer->Transfer(true);
#endif
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


_LIT(KUserGiven, "user_given");

void CContext_logAppUi::RestoreUserGiven()
{
}

void CContext_logAppUi::SetUserGiven()
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("SetUserGiven"));

	const TInt m_length = iPresenceMaintainer->Data()->iUserGiven.iDescription.iValue.MaxLength();

	TTime usergiventime;
	auto_ptr<HBufC> usergiven(HBufC::NewL( m_length) );
	TPtr16 p = usergiven->Des();
	Settings().GetSettingL(SETTING_OWN_DESCRIPTION, p);
	
	auto_ptr<HBufC> pr(CEikonEnv::Static()->AllocReadResourceL(R_CL_SET_USER_GIVEN));
	CAknTextQueryDialog* dlg = new(ELeave) CEmptyAllowingTextQuery(p, CAknQueryDialog::ENoTone);
	CleanupStack::PushL(dlg);
	dlg->SetPredictiveTextInputPermitted(ETrue);
	dlg->SetMaxLength( m_length );
	dlg->SetPromptL(*pr);
	
	CleanupStack::Pop();
	if (dlg->ExecuteLD(R_CONTEXT_LOG_SET_USER_GIVEN) ) {
		Settings().WriteSettingL(SETTING_OWN_DESCRIPTION_TIME, GetTime());
		Settings().WriteSettingL(SETTING_OWN_DESCRIPTION, p);
	}
}


class CLogEventTest : public CActive {
public:
	static CLogEventTest* NewL(RFs& Fs) {
		auto_ptr<CLogEventTest> ret(new (ELeave) CLogEventTest);
		ret->ConstructL(Fs);
		return ret.release();
	}
	CLogEventTest() : CActive(EPriorityNormal) { }
	void ConstructL(RFs& Fs) {
		lc=CLogClient::NewL(Fs);
		le=CLogEvent::NewL();
		le->SetEventType(KLogCallEventTypeUid);
		le->SetContact(1);
		le->SetDirection(_L("Missed call"));
		le->SetDuration(1);
		le->SetDurationType(KLogDurationValid);
		le->SetNumber(_L("0505536758"));
		TTime now; now.HomeTime();

		le->SetTime(now);
		le->SetRemoteParty(_L("mika"));
		CActiveScheduler::Add(this);
		lc->AddEvent(*le, iStatus);
		SetActive();
	}
	void RunL() {
		delete this;
	}
	void DoCancel() { }
	~CLogEventTest() {
		Cancel();
		delete lc;
		delete le;
	}
private:
	CLogClient*	lc;
	CLogEvent*	le;
};

void CContext_logAppUi::PresenceDetails()
{
	TBuf<50> nick;
	Settings().GetSettingL(SETTING_JABBER_NICK, nick);

	iPresenceDetailView->SetData(*iCaptionMyContext, nick, iPresenceMaintainer->Data(), GetTime());

	ActivateLocalViewL(KPresenceDetailView);

	return;
}

void CContext_logAppUi::PresenceDescription()
{
	iPresenceDescriptionView->SetData(*iCaptionMyContext, iPresenceMaintainer->Data());

	ActivateLocalViewL(KPresenceDescriptionView);

	return;
}


void CContext_logAppUi::DialogTest()
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("DialogTest"));
// _LIT(KFileScript, "def f(general, dev, msg) :\n"
// L"	f=open('c:/python-locatest.txt', 'a')\n"
// L"	for k in dev.keys():\n"
// L"		f.write(\"%s %d %s \" % (k, dev[k]['last_seen'], general['mac']) )\n"
// L"		f.write(\"%d \" % dev[k]['count'] )\n"
// L"		f.write(\"%d \" % dev[k]['avg'] )\n"
// L"		f.write(\"%f \" % dev[k]['var'] )\n"
// L"		f.write(\"%d \" % dev[k]['visitbegin'] )\n"
// L"		f.write(\"%d \" % dev[k]['prev_visitbegin'] )\n"
// L"		f.write(\"%d\\n\" % dev[k]['prev_visitend'] )\n"
// L"	f.close()\n"
// L"	return\n"
// 	 );

// 	const TTupleName KLocaScriptTuple = { { CONTEXT_UID_CONTEXT_LOG }, 1001 };
// 	auto_ptr<CBBString> s(CBBString::NewL(KNullDesC));
// 	s->Append(KFileScript);
// 	BBSession()->PutL(KLocaScriptTuple, _L("f"), s.get(), Time::MaxTTime());

// 	iDumper=CBBDumper::NewL();
// 	return;

	User::Leave(KErrGeneral);

#if 0
	{
		TBuf<200> msg;
		{
			auto_ptr<CAccessPointLister> lister(CAccessPointLister::NewL());
			TInt id; TBuf<50> name;
			while (lister->NextRecordL(id, name)) {
				msg.AppendNum(id);
				msg.Append(_L(": "));
				msg.Append(name);
				msg.Append(_L("\n"));
			}
		}
		CAknMessageQueryDialog *note=CAknMessageQueryDialog::NewL(msg);
		note->SetHeaderText(_L("All access points:"));
		note->ExecuteLD(R_LOGVIEW_EVENT_DIALOG);
	}
	{
	TInt id; TBuf<50> name;
	TestApL( id=CAutoAccessPoint::GetDefaultApL(name) );
	TBuf<100> msg=_L("testing access point ");
	msg.Append(name); msg.Append(_L(" ("));
	msg.AppendNum(id); msg.Append(_L(")"));
	LogAppEvent(msg);
	}
	return;
#endif

#ifdef __WINS__
	static onetwo=1;

	auto_ptr<CBBSubSession> bbs(BBSession()->CreateSubSessionL(0));

	if (onetwo==1) {
		TRAPD(err, bbs->DeleteL(KIncomingPresence, KNullDesC, ETrue));
		if (err!=KErrNone && err!=KErrNotFound) User::Leave(err);
		onetwo=2;
		return;
	}
	onetwo=1;

	refcounted_ptr<CBBPresence> pres(CBBPresence::NewL());

	pres->iBaseInfo.iCurrent.iBaseName()=_L("[Lookups logged]");
	pres->iBaseInfo.iCurrent.iEntered()=GetTime()-TTimeIntervalMinutes(10);
	pres->iBaseInfo.iPreviousStay.iBaseName=_L("Downtown Helsinki");
	pres->iBaseInfo.iPreviousStay.iEntered()=GetTime()-TTimeIntervalHours(4);
	pres->iBaseInfo.iPreviousStay.iLeft()=GetTime()-TTimeIntervalMinutes(23);
	pres->iNeighbourhoodInfo.iBuddies()=1;
	pres->iProfile.iProfileName=_L("General");
	pres->iProfile.iRingingVolume=7;
	pres->iProfile.iVibra=EFalse;
	pres->iUserActive.iActive=EFalse;
	pres->iUserActive.iSince=GetTime()-TTimeIntervalMinutes(30);
	pres->iUserGiven.iDescription()=_L("Day off");
	pres->iUserGiven.iSince()=GetTime()-TTimeIntervalHours(23);
	pres->iSentTimeStamp()=GetTime();
	TTime expires; expires=GetTime(); expires+=TTimeIntervalMinutes(30);
	bbs->PutL(KIncomingPresence, _L("demo1"), pres.get(), expires);

	pres->iBaseInfo.iCurrent.iBaseName()=_L("Work:MRL");
	pres->iBaseInfo.iCurrent.iEntered()=GetTime()-TTimeIntervalMinutes(5*60-4);
	pres->iBaseInfo.iPreviousStay.iBaseName=_L("Home:Broadgate Park");
	pres->iBaseInfo.iPreviousStay.iEntered()=GetTime()-TTimeIntervalHours(15);
	pres->iBaseInfo.iPreviousStay.iLeft()=GetTime()-TTimeIntervalMinutes(5*60+30);
	pres->iNeighbourhoodInfo.iBuddies()=2;
	pres->iNeighbourhoodInfo.iOtherPhones=5;
	pres->iNeighbourhoodInfo.iLaptops=1;
	pres->iProfile.iProfileName=_L("Meeting");
	pres->iProfile.iRingingVolume=0;
	pres->iProfile.iVibra=ETrue;
	pres->iUserActive.iActive=EFalse;
	pres->iUserActive.iSince=GetTime()-TTimeIntervalMinutes(30);
	pres->iUserGiven.iDescription()=KNullDesC;
	pres->iUserGiven.iSince()=GetTime()-TTimeIntervalHours(23);
	pres->iSentTimeStamp()=GetTime();
	pres->iCalendar.iCurrent.iDescription()=_L("MRL Lab meeting");
	pres->iCalendar.iCurrent.iStartTime()=TTime(_L("20060519:120000"));
	pres->iCalendar.iCurrent.iEndTime()=TTime(_L("20060519:123000"));
	pres->iCalendar.iNext.iDescription()=_L("Article review");
	pres->iCalendar.iNext.iStartTime()=TTime(_L("20060519:130000"));
	pres->iCalendar.iNext.iEndTime()=TTime(_L("20060519:143000"));
	bbs->PutL(KIncomingPresence, _L("demo2"), pres.get(), expires);

	pres->iBaseInfo.iCurrent.iBaseName()=_L("Work:HIIT");
	pres->iBaseInfo.iCurrent.iEntered()=GetTime()-TTimeIntervalMinutes(7*60-16);
	pres->iBaseInfo.iPreviousStay.iBaseName=_L("Home:Kallio");
	pres->iBaseInfo.iPreviousStay.iEntered()=GetTime()-TTimeIntervalHours(15);
	pres->iBaseInfo.iPreviousStay.iLeft()=GetTime()-TTimeIntervalMinutes(7*60+10);
	pres->iNeighbourhoodInfo.iBuddies()=2;
	pres->iNeighbourhoodInfo.iOtherPhones=1;
	pres->iNeighbourhoodInfo.iLaptops=0;
	pres->iNeighbourhoodInfo.iDesktops=1;
	pres->iProfile.iProfileName=_L("General");
	pres->iProfile.iRingingVolume=7;
	pres->iProfile.iVibra=ETrue;
	pres->iUserActive.iActive=ETrue;
	pres->iUserActive.iSince=GetTime()-TTimeIntervalMinutes(5);
	pres->iUserGiven.iDescription()=_L("Working hard");
	pres->iUserGiven.iSince()=GetTime()-TTimeIntervalHours(3);
	pres->iSentTimeStamp()=GetTime();
	pres->iCalendar.iCurrent.iDescription()=KNullDesC;
	pres->iCalendar.iCurrent.iStartTime()=TTime(0);
	pres->iCalendar.iCurrent.iEndTime()=TTime(0);
	pres->iCalendar.iNext.iDescription()=_L("Fussball");
	pres->iCalendar.iNext.iStartTime()=TTime(_L("20060519:170000"));
	pres->iCalendar.iNext.iEndTime()=TTime(_L("20060519:183000"));
	bbs->PutL(KIncomingPresence, _L("demo3"), pres.get(), expires);

	pres->iBaseInfo.iCurrent.iBaseName()=_L("Cafe Java");
	pres->iBaseInfo.iCurrent.iEntered()=GetTime()-TTimeIntervalMinutes(40);
	pres->iBaseInfo.iPreviousStay.iBaseName=_L("HIIT:Work");
	pres->iBaseInfo.iPreviousStay.iEntered()=GetTime()-TTimeIntervalHours(15);
	pres->iBaseInfo.iPreviousStay.iLeft()=GetTime()-TTimeIntervalMinutes(55);
	pres->iNeighbourhoodInfo.iBuddies()=1;
	pres->iNeighbourhoodInfo.iOtherPhones=0;
	pres->iNeighbourhoodInfo.iLaptops=0;
	pres->iNeighbourhoodInfo.iDesktops=0;
	pres->iProfile.iProfileName=_L("Silent");
	pres->iProfile.iRingingVolume=0;
	pres->iProfile.iVibra=EFalse;
	pres->iUserActive.iActive=ETrue;
	pres->iUserActive.iSince=GetTime()-TTimeIntervalMinutes(5);
	pres->iUserGiven.iDescription()=_L("Coffee with Eve");
	pres->iUserGiven.iSince()=GetTime()-TTimeIntervalHours(1);
	pres->iSentTimeStamp()=GetTime()-TTimeIntervalMinutes(15);
	pres->iCalendar.iCurrent.iDescription()=KNullDesC;
	pres->iCalendar.iCurrent.iStartTime()=TTime(0);
	pres->iCalendar.iCurrent.iEndTime()=TTime(0);
	pres->iCalendar.iNext.iDescription()=KNullDesC;
	pres->iCalendar.iNext.iStartTime()=TTime(0);
	pres->iCalendar.iNext.iEndTime()=TTime(0);
	bbs->PutL(KIncomingPresence, _L("demo4"), pres.get(), expires);
// #else
// 	CCommDbDump* d=CCommDbDump::NewL();
// 	d->DumpDBtoFileL(_L("c:\\commdb.txt"));
// 	return;
// #endif
	return;

	iSmsSnapshot->Test();
	return;

	iDumper=CBBDumper::NewL();
	return;

	/*
	{
		iSensorRunner->Stop();
		if (iLoggers) {
			iLoggers->ResetAndDestroy();
		}
		delete iLoggers; iLoggers=0;

		if (iMediaTest) delete iMediaTest;
		iMediaTest=0;
		iMediaTest=new (ELeave) CMediaTest(AppContext(), iMediaPrompt);
		iMediaTest->ConstructL();
	}
	*/
	return;

	//delete iSocketTestReader; iSocketTestReader=0;
	//iSocketTestReader=CSocketTestReader::NewL(AppContext(), 2000, _L("c:\\bblocal2.xml"));

	return;

	iDumper=CBBDumper::NewL();

	return;
	

	TInt ap; Settings().GetSettingL(SETTING_IP_AP, ap);
	iNTPConnection=CNTPConnection::NewL(*this);
	iNTPConnection->Sync(ap);
	return;

	/*
	_LIT(KS, "Liittymän 0407679267 sijainti: HELSINKI, Esplanadi, N60:10'06\", E24:56'57\" (ast:min'sek\" WGS84:ssä)");
	TMsvId dummy(1);
	i_handle_received_sms* s=loc;
	s->handle_reception(dummy, dummy, _L("15400"), KS);
	return;
	_LIT(KS, "Liittymän 0407679267 sijainti: HELSINKI, Esplanadi, N60:10'06\", E24:56'57\" (ast:min'sek\" WGS84:ssä)");
	TMsvId dummy(1);
	i_handle_received_sms* s=lsms;
	s->handle_sending(dummy, _L("0505536758"), KS);
	return;
	*/

	/*
	TMsvId dummy(1);
	iSmsSnapshot->handle_reception(dummy, dummy, _L("0505536758"), _L("WINS"));
	return;
	*/

	/*
	iSmsSnapshot->Test();
	return;
	*/

	CLogEventTest* t=CLogEventTest::NewL(Fs());
	
	return;

	/*
	TBuf<100> err;
	TInt ret=iMMS->SendMessage(_L("0505536758"), _L("body of msg"), _L("c:\\http.txt"), err, false);
	if (ret!=KErrNone) Error(ret, err);
	else status_change(_L("message sent"));
	return; */

	/**/

	//TBuf<2> buf; buf.Append(_L("xxxx"));

#endif
	if (iTestFileOpen) {
		iTestFile.Close();
		iTestFileOpen=false;
	} else {
		
		TFileName filen=_L("C:\\nokia\\images\\200609\\");
		BaflUtils::EnsurePathExistsL(Fs(), filen);
		filen.Append(_L("img"));
		TInt i=2; filen.AppendNum(i); filen.Append(_L(".jpg"));
		while (BaflUtils::FileExists(Fs(), filen)) {
			filen=_L("C:\\nokia\\images\\200609\\img");
			++i; filen.AppendNum(i); filen.Append(_L(".jpg"));
		}
		{
			TFileName from=_L("C:\\nokia\\images\\img.jpg");
			BaflUtils::CopyFile(Fs(), from, filen);
		}
		{
			TFileName from2=_L("C:\\nokia\\images\\old\\img.jpg");
			BaflUtils::CopyFile(Fs(), from2, filen);
		}
		/*
		if (iTestFile.Open(Fs(), filen, EFileWrite)==KErrNone) {
			iTestFileOpen=true;
		}
		*/
	}
	return;

	/*
	if (IsDisplayingMenuOrDialog()) return;


	CCoeEnv *env = CEikonEnv::Static();
	RPbkViewResourceFile pbkRes( *env);
	pbkRes.OpenL();
	CleanupClosePushL(pbkRes);

	CPbkContactEngine *ipPabEng=CPbkContactEngine::NewL();
	CleanupStack::PushL(ipPabEng);

	CPbkContactItem* aContactItem = ipPabEng->ReadContactL( 1 );
	CleanupStack::PushL(aContactItem);

	// launch the contacts dialog
	CPbkContactEditorDlg *ipPabDlg =
	CPbkContactEditorDlg::NewL(*ipPabEng, *aContactItem, EFalse);

	ipPabDlg->SetMopParent( this );

	TInt res = KErrNone;
	CC_TRAPD( err, res = ipPabDlg->ExecuteLD());

	
	//CPbkSelectFieldDlg *d=new (ELeave) CPbkSelectFieldDlg;
	//TPbkContactItemField* res=d->ExecuteLD(aContactItem->CardFields(), R_AVKON_SOFTKEYS_OPTIONS_BACK);
	

	CleanupStack::PopAndDestroy(3); //engine, pbkRes, aContactItem
	*/
}

void CContext_logAppUi::NotifyNewPresence(const CBBPresence* d)
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("NotifyNewPresence"));

	PresenceToListBoxL(d, iUserContext,
		iCaptionMyContext, 0, KNullDesC, GetTime(),
		prev, ago, not_avail, EFalse, ETrue);

	iUserContextLog->AddL(*iUserContext);
}

void CContext_logAppUi::SettingChanged(TInt Setting)
{
	CContextLogAppUiBase::SettingChanged(Setting);
#ifndef __JAIKU__
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
				iLoggingRunning->SetCurrentState( EMbmContext_logL_not, EMbmContext_logL_not );
			} else {
				iLoggingRunning->SetCurrentState( EMbmContext_logL, EMbmContext_logL );
			}
		}

	} else 
#endif
	if (Setting==SETTING_OWN_DESCRIPTION) {
	}
}


void CContext_logAppUi::SetAccessPointAutomaticallyL()
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("SetAccessPointAutomaticallyL"));

	TInt ap=-1;
	if (! Settings().GetSettingL(SETTING_IP_AP, ap) ) {
		TInt ignore;
		TBuf<50> name;
		CC_TRAP(ignore, ap=CAutoAccessPoint::GetOperaApL(name));
		/*if (ap==-1) {
			CC_TRAP(ignore, ap=CAutoAccessPoint::GetBrowserApL(name));
		}*/
		if (ap==-1) {
			CC_TRAP(ignore, ap=CAutoAccessPoint::GetInternetApL(name));
		}
		if (ap==-1) {
			QueueOp( static_cast<TCallBack>(&CContext_logAppUi::ShowSettings), 0 );
		} else {
			Settings().WriteSettingL(SETTING_IP_AP, ap);
		}
	}
}

// End of File  
