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

#include "appuibase.h"

#include "Context_logContainer.h" 

#include "contextlog_resource.h"

#include "local_defaults.h"
#include "context_log.hrh"
#define CONTEXTNW 1
#include <avkon.hrh>
#include <aknquerydialog.h> 
#include <eikenv.h> // ieikonenv
#include <eikmenup.h>

#include <bautils.h>
#include "sms.h"
#include "cl_settings.h"
#include <eikdll.h>
#include "break.h"
#include "notifystate.h"
#include "btlist.h"
#include "viewids.h"
#include "callstack.h"
#include "reporting.h"
#include "app_context_impl.h"
#include "app_context_fileutil.h"
#include "contextnotifyclientsession.h"
#include "csd_sms.h"
#include "statusview.h"

#include "cu_errorui.h"
#include "emptytext.h"
#include "file_logger.h"
#include "sensorrunner.h"
#include "log_gps.h"
#include "applogview.h"
#include "raii_f32file.h"
#include "raii_apgcli.h"
#include "cellmap.h"
#include "presencemaintainer.h"
#include <eikdoc.h>
#include <hal.h>
#include <sysutil.h>
#include <aknwaitdialog.h> 
#include "autoap.h"
#include <aknmessagequerydialog.h> 
#include "file_logger.h"
#include "cn_datacounter.h"
#include "mediarunner.h"
#include "indep_watcher.h"
#include "bb_recovery.h"

#ifndef __WINS__
const TInt KWatchDogTimeOut=5*60;
#else
const TInt KWatchDogTimeOut=1*60;
#endif

#ifndef __WINS__
_LIT(KIconFile, "c:\\system\\data\\context_log.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\context_log.mbm");
#endif

_LIT(file_prefix, "context_log");

_LIT(KAppEvent, "app_event");
#include "ntpconnection.h"
#include "contextvariant.hrh"
#include "cu_cellnaming.h"
#include "cu_systemapp.h"
#include "ContextCommon.h"

/*
 * The whole structure of contextappui, appuibase, CContextXAppUi
 * is a bit hacky, and brittle. Should be rewritten.
 */

TErrorHandlerResponse CContextLogAppUiBase::HandleError(TInt aError,
     const SExtendedError& aExtErr,
     TDes& aErrorText,
     TDes& aContextText)
{

#ifdef __WINS__
	if (aError==-20) {
		TInt x=0;
	}
#endif
	MContextAppUi::HandleError(aError, aExtErr, aErrorText, aContextText);
	return CAknAppUi::HandleError(aError, aExtErr, aErrorText, aContextText);
}

void CContextLogAppUiBase::LogFormatted(const TDesC& aMsg)
{
	LogAppEvent(aMsg);
}

void CContextLogAppUiBase::LogAppEvent(const TDesC& msg)
{
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("LogAppEvent"));

	_LIT(KApp, "app");
	TBBLongString m(msg, KApp);
	if (iLastLog) iLastLog->AddL(msg);
	if (app_log) app_log->post_new_value(&m);
}

void CContextLogAppUiBase::StartMediaRunner()
{
	if (iMediaRunner) return;

	TInt err=iMediaPluginLibrary.Load(_L("ContextMediaPlugin.dll"));
	if ( err==KErrNone ) {
		TMediaRunnerNewLFunction newl=(TMediaRunnerNewLFunction)iMediaPluginLibrary.Lookup(1);
		iMediaRunner=newl(this, iPresenceMaintainer,
			&(iStatusView->iNextViewId), *this, *this);
	}
}

void CContextLogAppUiBase::SettingChanged(TInt Setting)
{
	if (Setting==SETTING_MEDIA_UPLOAD_ENABLE) {
		TInt enabled;
		Settings().GetSettingL(SETTING_MEDIA_UPLOAD_ENABLE, enabled);
		if (enabled==MEDIA_PUBLISHER_CL) {
			StartMediaRunner();
		}
	}
}

TInt CContextLogAppUiBase::InitializationSteps()
{
	// -2 for moving media things to the mediarunner. it should eventually call back for those
	return MContextAppUi::InitializationSteps()+15-2;
}

void CContextLogAppUiBase::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("ConstructL"));
#if !defined(FLICKR) && !defined(CONTEXTLOCA) && !defined(__JAIKU__)
	MContextAppUi::ConstructL(AppContext(), file_prefix);
#else
	MContextAppUi::ConstructL(AppContext(), file_prefix, 3);
#endif

	state=_L("BaseConstructL");

#ifdef __WINS__
	void* p=User::Alloc(20*1024);
	if (!p) User::LeaveNoMemory();
	User::Free(p);
#endif

#ifndef __S60V2__
		BaseConstructL(0x1008);
#else
		BaseConstructL(EAknEnableSkin);
#endif

	StepDone();

#ifdef __WINS__
	//BBSession()->DeleteL(KBaseTuple, KNullDesC, ETrue);
#endif
	//BBSession()->DeleteL(KSentDataCounterTuple, KNullDesC, ETrue);
	//BBSession()->DeleteL(KReceivedDataCounterTuple, KNullDesC, ETrue);

	CLocalNotifyWindow::CreateAndActivateL();
	StepDone();

	state=_L("create status view");
	iStatusView = CStatusView::NewL(AppContext());
	iAppContainer = iStatusView->Container();
	AddViewL( iStatusView );      // transfer ownership to CAknViewAppUi
	SetDefaultViewL(*iStatusView);
	StepDone();

	{
		auto_ptr<CAknView> errorview(CErrorInfoView::NewL());
		AddViewL( errorview.get() );
		errorview.release();
	}

	iBBSubSession=BBSession()->CreateSubSessionL(this);
	iBBSubSession->AddNotificationL(KComponentVersionTuple);
	iBBSubSession->AddNotificationL(KComponentFunctionalityTuple);
	iBBSubSession->AddNotificationL(KComponentNameTuple);
	iBBSubSession->AddNotificationL(KComponentStateTuple);
	iBBSubSession->AddNotificationL(KComponentErrorCountTuple);
	iBBSubSession->AddNotificationL(KComponentErrorInfoTuple);
	iBBSubSession->AddNotificationL(KContextServerStatusTuple);


	StepDone();
	//FIXME

	state=_L("create app event");
	app_log=Clog_base_impl::NewL(AppContext(), KAppEvent, KAppEventTuple, 0);

	Reporting().SetActiveErrorReporter(this);

	StepDone();

#ifndef NO_PRESENCE
	iBuddyBTs = CBTDeviceList::NewL(AppContext(), KBtBuddyDeviceList);
	iLaptopBTs = CBTDeviceList::NewL(AppContext(), KBtLaptopDeviceList);
	iDesktopBTs = CBTDeviceList::NewL(AppContext(), KBtDesktopDeviceList);
	iPDABTs = CBTDeviceList::NewL(AppContext(), KBtPDADeviceList);
#endif //NO_PRESENCE
	StepDone();

#ifndef __JAIKU__
	state=_L("smsh");

	//iSmsSocket=CSmsSocket::NewL(AppContext(), _L("itse"), this);

	{
		TInt err=0;
		smsh=new (ELeave) sms;
		CC_TRAP(err, smsh->ConstructL());
		if (err!=KErrNone) {
			error(this, err, _L("Cannot create sms handler"));
			delete smsh; smsh=0;
		}
	}
#endif
	StepDone();

	LogAppEvent(_L("Starting"));
	iLastLog=CCircularLog::NewL(20, true);

	LogAppEvent(_L("Starting 1.5"));

	iLoggers=new (ELeave) CArrayPtrFlat<Mlog_base_impl>(10);

	StepDone();
}

void CContextLogAppUiBase::ConstructAfterPresenceMaintainerL()
{
	// start the sensors after the presence maintainer, so that the presence
	// gets all the events. Should get them from the blackboard instead...
	//
	TInt wait_for_sensors=25*60*60;
	iSensorsWatcher=CIndependentWatcher::NewL(iSensorWorker.info,
		_L("sensors"), *this, KCellIdTuple, wait_for_sensors);
	iSensorWorker.info.set_has_stopped( iSensorsWatcher->GetStatus() );
	iSensorWorker.start(_L("sensors"), CSensorRunner::RunSensorsInThread, 0, EPriorityLess);

	StepDone();

#if !defined(CONTEXTLOCA)
	_LIT(KFlickrAuth, "c:\\system\\data\\context\\flickr_auth_token.txt");
	TInt enabled=0;
	Settings().GetSettingL(SETTING_MEDIA_UPLOAD_ENABLE, enabled);
#if !defined(__WINS__) && !defined(FLICKR)
	if (BaflUtils::FileExists(Fs(), KFlickrAuth) || enabled==MEDIA_PUBLISHER_CL) 
#endif
	{
		StartMediaRunner();
	}
	Settings().NotifyOnChange(SETTING_MEDIA_UPLOAD_ENABLE, this);

	StepDone();
#endif //CONTEXTLOCA


	LogAppEvent(_L("Starting 3"));

	state=_L("create log view");
	auto_ptr<CAppLogView> logview(0);
	if (iLog) iLog->write_line(_L("creating log view"));
	logview.reset(CAppLogView::NewL(iLastLog));
	state=_L("created log view");
	if (iLog) iLog->write_line(_L("created log view"));
	AddViewL(logview.get());
	iLogView=logview.release();
	StepDone();

	StepDone();

}

void CContextLogAppUiBase::NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
	const TComponentName& aComponentName, const MBBData* aData)
{
	if (!aData) return;
	
	if (aName==KContextServerStatusTuple) {
		const TBBInt* value=bb_cast<TBBInt>(aData);
		TBuf<30> msg;
		if (!value) return;
		switch((*value)()) {
			case ContextServer::EUnknown:
				msg=_L("Unknown presence status");
			break;
			case ContextServer::EStarting:
				msg=_L("Presence Starting");
			break;
			case ContextServer::EConnecting:
				msg=_L("Presence Connecting");
			break;
			case ContextServer::EConnected:
				msg=_L("Presence Connected");
			break;
			case ContextServer::EDisconnecting:
				msg=_L("Presence Disconnecting");
			break;
			case ContextServer::EDisconnected:
				msg=_L("Presence Disconnected");
			break;
			case ContextServer::EStopped:
				msg=_L("Presence Stopped");
			break;
		};
		LogAppEvent(msg);
		if (iAppContainer) iAppContainer->set_status(msg);
		return;
	}
	iComponentMessage=_L("Component ");
	if (aName==KComponentVersionTuple) {
		iComponentMessage.Append(_L("version"));
	} else if (aName==KComponentFunctionalityTuple) {
		iComponentMessage.Append(_L("functionality"));
	} else if (aName==KComponentNameTuple) {
		iComponentMessage.Append(_L("name"));
	} else if (aName==KComponentStateTuple) {
		iComponentMessage.Append(_L("state"));
	} else if (aName==KComponentErrorCountTuple) {
		iComponentMessage.Append(_L("errorcount"));
	} else if (aName==KComponentErrorInfoTuple) {
		iComponentMessage.Append(_L("errorinfo"));
	} else {
		return;
	}
	iComponentMessage.Append(_L(" for "));
	iComponentMessage.Append(aSubName);
	iComponentMessage.Append(_L(" : "));
	if (! (aName==KComponentErrorInfoTuple || aName==KComponentFunctionalityTuple)) {
		aData->IntoStringL(iComponentMessage);
		LogAppEvent(iComponentMessage);
	} else {
		LogAppEvent(iComponentMessage);
		auto_ptr<HBufC> buf( HBufC::NewL(4096) );
		TPtr p=buf->Des();
		TRAPD(ignored, aData->IntoStringL(p));
		LogAppEvent(p);
	}
}

void CContextLogAppUiBase::DeletedL(const TTupleName& , const TDesC& )
{
}

void CContextLogAppUiBase::FinalConstructL()
{
	iAppContainer->set_status(_L(""));

#ifndef __S60V3__
	iEikonEnv->SetSystem(ETrue);
#endif
	//iUninstallSupport=CUninstallSupport::NewL();

	LogAppEvent(_L("watchdog"));

	// note that we've started
	expired(iWatchDogTimer);
	_LIT(KContextLogStarted, "context_log_started");
	TBBTime t(KContextLogStarted);
	t()=GetTime();
	TTime expires; expires=Time::MaxTTime();
	CC_TRAPD(serr, iBBSubSession->PutL(KStatusTuple, KContextLogStarted, &t, expires));
	state=_L("");
	
	LogAppEvent(_L("Started"));
	initialising=false;
	Reporting().SetState(KNullDesC);
	StepDone();
}

CContextLogAppUiBase::~CContextLogAppUiBase()
{
	CC_TRAPD(err, ReleaseCContextLogAppUiBase());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}
void CContextLogAppUiBase::ReleaseCContextLogAppUiBase(void)
{
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("~CContextLogAppUiBase"));
	{
	TInt err;

	{
		if (iNTPWait) iNTPWait->ProcessFinishedL();
		delete iBBSubSession;
		delete iNTPConnection;
		delete iAccessPointTester;
		iSensorWorker.stop();
		delete iSensorsWatcher; iSensorsWatcher=0;
		delete iUninstallSupport;
	}

	{

		delete app_log;
		Reporting().SetActiveErrorReporter(0);
	}

	{
		TTime t; t=GetTime();
		if (iLog) 
			CC_TRAP(err, iLog->new_value(CBBSensorEvent::VALUE, _L("APP"), _L("4"), t));
		if (iLoggers) {
			iLoggers->ResetAndDestroy();
		}
		delete iLoggers;
		Settings().CancelNotifyOnChange(SETTING_MEDIA_UPLOAD_ENABLE, this);
		delete iMediaRunner; iMediaRunner=0;
		iMediaPluginLibrary.Close();
	}
#ifndef __JAIKU__
	delete smsh;
#endif
	delete iBuddyBTs;
	delete iLaptopBTs;
	delete iDesktopBTs;
	delete iPDABTs;
	delete iBTDb;

	delete iLastLog;

	delete iWatchDogTimer;

	CLocalNotifyWindow::Destroy();
#if defined(__S60V2__) && ! defined(__WINS__) && !defined(__S60V3__)
	// the cell change notification keeps the thread from
	// exiting otherwise
	{
		PrepareToExit();
		ReleaseContextAppUi();
		delete Document();
		User::Exit(iExitValue);
	}
#endif
	}
}

void CContextLogAppUiBase::HandleSystemEventL  (  const TWsEvent &    aEvent  )
{
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("HandleSystemEventL"));

	TApaSystemEvent event;
	event = *(TApaSystemEvent*)aEvent.EventData();

	switch ( event ) {
		case EApaSystemEventShutdown:
			if (iLog) {
				iLog->new_value(
					CBBSensorEvent::INFO, _L("app_event"), 
					_L("ext shutdown"), GetTime());
			}
			Exit();
			return;
			break;
		default:
			break;
	}
	CAknViewAppUi::HandleSystemEventL(aEvent);
}

void CContextLogAppUiBase::HandleWsEventL(const TWsEvent& aEvent, CCoeControl* aDestination)
{
	CallStackMgr().ResetCallStack();
	MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);

	CC_TRAPD(err, CAknAppUi::HandleWsEventL(aEvent, aDestination));
	if (err==KErrCancel) {
		CallStackMgr().ResetCallStack();
		return;
	}

	User::LeaveIfError(err);
}

void CContextLogAppUiBase::expired(CBase* aSource)
{
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("expired"));

	if (aSource==iWait) {
		MContextAppUi::expired(aSource);
	} else {
		/*
		RAFile f; f.ReplaceLA(Fs(), _L("c:\\system\\data\\context\\watch-context_log.dat"),
			EFileWrite|EFileShareAny);
		f.Write(_L8("watch"));
		iWatchDogTimer->Wait(KWatchDogTimeOut);
		*/
	}
}

TBool CContextLogAppUiBase::BaseHandleCommandL(TInt aCommand)
{
	TBuf<30> msg;
	switch ( aCommand )
	{
	case EAknSoftkeyBack:
		QueueOp(&CContextLogAppUiBase::hide);
		break;
	case Econtext_logCmdShutdown:
	{
#ifdef __JAIKU__
		TApaTaskList tl(iEikonEnv->WsSession());
		TApaTask app_task=tl.FindApp( KUidContextContacts );
		if (app_task.Exists()) app_task.EndTask();
#endif
		msg=_L("Econtext_logCmdShutdown");

	}
	// fallthru
	case EEikCmdExit:
		if (msg.Length()==0) msg=_L("EEikCmdExit");
		if (iLog) {
			iLog->new_value(
				CBBSensorEvent::INFO, _L("app_event"), 
				msg, GetTime());
		}
		Exit();
		break;
	case Econtext_logDetailedView:
	case Econtext_logCmdStatusView:
		ActivateLocalViewL(KStatusView);
		break;

	case Econtext_logCmdAppNameCell:
#if !defined(CONTEXTLOCA)
		AskUserForCurrentCellNameL();
#endif
		break;
	case Econtext_logCmdAppNameCity:
#if !defined(CONTEXTLOCA)
		AskUserForCurrentCityNameL();
#endif
		break;
	case Econtext_logCmdBaseLog:
		//FIXME3RD
		User::Leave(KErrNotSupported);
		break;
	case Econtext_logCmdAppLog:
		iLogView->SetLog( iLastLog );
		ActivateLocalViewL(KLogViewId);
		break;
	case Econtext_logCmdGPS:
		//FIXME3RD
		//if (iSensorRunner->Gps()) iSensorRunner->Gps()->SelectDevice();
		User::Leave(KErrNotSupported);
		break;
	case Econtext_logCmdResetGPS:
		//FIXME3RD
		//if (iSensorRunner->Gps()) iSensorRunner->Gps()->ResetDevice();
		User::Leave(KErrNotSupported);
		break;
	case Econtext_logCmdGPSTest:
		//FIXME3RD
		//if (iSensorRunner->Gps()) iSensorRunner->Gps()->TestDevice();
		User::Leave(KErrNotSupported);
		break;
	case Econtext_logCmdSettingsCancel:
	case Econtext_logCmdSettingsSave:
	case Econtext_logCmdlogviewClose:
		ActivateLocalViewL(KStatusView);
		break;
	case Econtext_logCmdPublishOld:
		if (iMediaRunner) iMediaRunner->PublishOldMedia();
		break;
	case Econtext_logCmdMediaPool:
		if (iMediaRunner) iMediaRunner->ShowMediaPoolL();
		break;
	case Econtext_logCmdLocateViaOperator:
		User::Leave(KErrNotSupported);
		break;
	case Econtext_logCmdDisableAutostart: {
		RAFile f; f.ReplaceLA(Fs(), _L("c:\\system\\data\\context\\disable_autostart.dat"),
			EFileWrite|EFileShareAny);
		f.Write(_L8("disable"));
					      }
		break;
	case Econtext_logCmdEnableAutostart: {
		User::LeaveIfError(Fs().Delete(_L("c:\\system\\data\\context\\disable_autostart.dat")));
					      }
		break;
	case Econtext_logCmdDeviceFamily: {
#ifndef __S60V3__
//FIXME3RD
#ifndef __S60V2__
		TInt revision = 0;
		HAL::Get( HALData::EDeviceFamilyRev, revision );
		TBuf<20> buf=_L("Rev: 0x");
		buf.AppendNum( (TUint)revision, EHex);
		info(0, buf);
#else
		auto_ptr<HBufC8> agent8(SysUtil::UserAgentStringL());
		auto_ptr<HBufC> agent( HBufC::NewL( (*agent8).Length() ) );
		TInt state;
		TPtr p=agent->Des();
		CC()->ConvertToUnicode(p, *agent8, state);
		info(0, *agent);
#endif
#endif
					  }
		break;
	case Econtext_logCmdRereadUnread: {
		if (iMediaRunner) iMediaRunner->RereadUnreadL();
					  }
		break;
	case Econtext_logCmdNTP: {
		SyncTimeL(ETrue);
		return ETrue;
	}

	case Econtext_logCmdAppAutoApList:
		{
			TBuf<160> msg;
			TInt ap, err;
			msg.Append(_L("Opera: "));
			TBuf <50> name;
			CC_TRAP(err, ap=CAutoAccessPoint::GetOperaApL(name) );
			if (err!=KErrNone) {
				msg.Append(_L("ERROR "));
				msg.AppendNum(err);
			} else {
				msg.AppendNum(ap);
				if (ap!=-1) {
					msg.Append(_L("\n("));
					msg.Append(name.Left(30)); msg.Append(_L(")"));
				}
			}
			msg.Append(_L("\nBrowser: "));
			CC_TRAP(err, ap=CAutoAccessPoint::GetBrowserApL(name) );
			if (err!=KErrNone) {
				msg.Append(_L("ERROR "));
				msg.AppendNum(err);
			} else {
				msg.AppendNum(ap);
				if (ap!=-1) {
					msg.Append(_L("\n("));
					msg.Append(name.Left(30)); msg.Append(_L(")"));
				}
			}
			msg.Append(_L("\n\"internet\": "));
			CC_TRAP(err, ap=CAutoAccessPoint::GetInternetApL(name) );
			if (err!=KErrNone) {
				msg.Append(_L("ERROR "));
				msg.AppendNum(err);
			} else {
				msg.AppendNum(ap); 
				if (ap!=-1) {
					msg.Append(_L("\n("));
					msg.Append(name.Left(30)); msg.Append(_L(")"));
				}
			}
			msg.Append(_L("\n\"default\": "));
			CC_TRAP(err, ap=CAutoAccessPoint::GetDefaultApL(name) );
			if (err!=KErrNone) {
				msg.Append(_L("ERROR "));
				msg.AppendNum(err);
			} else {
				msg.AppendNum(ap); 
				if (ap!=-1) {
					msg.Append(_L("\n("));
					msg.Append(name.Left(30)); msg.Append(_L(")"));
				}
			}
			CAknMessageQueryDialog *note=CAknMessageQueryDialog::NewL(msg);
			note->SetHeaderText(_L("System access points:"));
			note->ExecuteLD(R_LOGVIEW_EVENT_DIALOG);
		}
		break;
	default:
		return EFalse;
	}
	return ETrue;

}
void CContextLogAppUiBase::finished()
{
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("finished"));

	_LIT(got, "finished op");
	iAppContainer->set_status(got);
}

void CContextLogAppUiBase::error(CBase* source, TInt code, const TDesC& descr)
{
	iLastErrorFrom=source;
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("error"));

	if (descr.Length()>0 || code!=KErrNone) {
		TBuf<200> msg;
		msg.Append(_L("ERROR: "));
		msg.AppendNum(code);
		msg.Append(_L(" "));
		msg.Append(descr.Left(180));
		//globalNote->ShowNoteL(EAknGlobalInformationNote, descr	);
		LogAppEvent(msg);
#ifdef __WINS__
		RDebug::Print(msg);
#endif
		if (iAppContainer) iAppContainer->set_error(msg);
	} else {
		if (iAppContainer) iAppContainer->set_error(KNullDesC);
	}
	// FIXME: hack to get app closed if mmc is removed -MR 20061026
	if (descr.Length()>3 && descr.Right(3).CompareF(_L("-18"))==0) {
		if ( AppDir().Left(1).CompareF(_L("e"))==0 && !HasMMC(Fs()) ) {
			BaseHandleCommandL(Econtext_logCmdShutdown);
		}
	}
}

void CContextLogAppUiBase::error(const TDesC& descr)
{
	error(this, KErrUnknown, descr);
}

void CContextLogAppUiBase::status_change(const TDesC& status)
{
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("status_change"));

	//globalNote->ShowNoteL(EAknGlobalInformationNote, status);
	if (status.Length()>0) {
		TBuf<200> msg;
		msg.Append(_L("STATUS: "));
		msg.Append(status.Left(190));
		LogAppEvent(msg);
	}
	if (iAppContainer) iAppContainer->set_status(status);
}

void CContextLogAppUiBase::start_app(TUid app, TInt view)
{
	CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("start_app"));

	if (view==0) {
		RAApaLsSession ls; ls.ConnectLA();
		TThreadId tid;

		User::LeaveIfError( ls.StartDocument(_L(""), app, tid) );

	} else {
		CCoeAppUi::ActivateViewL(TVwsViewId(app, TUid::Uid(view)));
	}
}


TBool CContextLogAppUiBase::ProcessCommandParametersL(TApaCommand aCommand, 
	TFileName& aDocumentName,const TDesC8& aTail)
{
	if (aDocumentName.CompareF(_L("hide"))==0) {
		hide();
	}
	aDocumentName.Zero();
	return EFalse;
}


void CContextLogAppUiBase::NTPInfo(const TDesC& aMsg)
{
	info(iNTPConnection, aMsg);
}
void CContextLogAppUiBase::NTPError(const TDesC& aMsg, TInt aErrorCode)
{
	if (iNtpErrors==0) {
		iNtpErrors++;
		TInt ap;
		Settings().GetSettingL(SETTING_IP_AP, ap);
		delete iNTPConnection; iNTPConnection=0;
		iNTPConnection=CNTPConnection::NewL(*this);
		iNTPConnection->Sync(ap);
	} else {
		iNtpErrors++;
		if (iNTPWait) iNTPWait->ProcessFinishedL();
		error(iNTPConnection, aErrorCode, aMsg);
		if (iUserIniatedNtp)
			Reporting().ShowGlobalNote( EAknGlobalErrorNote, aMsg);
		delete iNTPConnection; iNTPConnection=0;
		TimeSynced(EFalse);
	}
}
void CContextLogAppUiBase::NTPSuccess(TTime aNewTime)
{
	if (iNTPWait) iNTPWait->ProcessFinishedL();

	TTime now; now.HomeTime();
#ifndef CONTEXTLOCA
	if ( aNewTime > now+TTimeIntervalMinutes(30) || aNewTime < now-TTimeIntervalMinutes(30) ) {
#else
	if (0) {
#endif
		TBuf<100> msg;
		msg=_L("Network time differs more than 30 minutes from current. Please check your timezone."); 
		Reporting().ShowGlobalNote( EAknGlobalErrorNote, msg);
	} else {
		_LIT(KTimeString,"%1/%2/%3 %-B%:0%J%:1%T%:2%S%.%*C4%:3%+B");
		TBuf<32> timeString1;
		aNewTime.FormatL(timeString1,KTimeString);
		TBuf<60> msg;
		msg=_L("Time set to "); msg.Append(timeString1);
		User::SetHomeTime(aNewTime);
		expired(iWatchDogTimer);
		if (iUserIniatedNtp)
			Reporting().ShowGlobalNote( EAknGlobalConfirmationNote, msg);
		TimeSynced(ETrue);
	}
	delete iNTPConnection; iNTPConnection=0;
}

void CContextLogAppUiBase::DialogDismissedL(TInt aButtonId)
{
	delete iNTPConnection; iNTPConnection=0;
}

void CContextLogAppUiBase::SyncTimeL(TBool aFromUser)
{
	iNtpErrors=0;
	iUserIniatedNtp=aFromUser;
	TInt ap;
	if (! Settings().GetSettingL(SETTING_IP_AP, ap) || ap==-1) return;
	if (iNTPWait) return;
	delete iNTPConnection; iNTPConnection=0;
	iNTPConnection=CNTPConnection::NewL(*this);
	iNTPConnection->Sync(ap);
	if (aFromUser) {
		iNTPWait = new (ELeave) CAknWaitDialog( (REINTERPRET_CAST(CEikDialog**,&iNTPWait))); 
		iNTPWait->SetTone( CAknNoteDialog::ENoTone ); 
		iNTPWait->SetCallback(this);
		iNTPWait->ExecuteLD(R_NTP_WAITNOTE);
	}
}
void CContextLogAppUiBase::TimeSynced(TBool )
{
}

void CContextLogAppUiBase::Done(TInt aAp, TInt aError)
{
	// TEEMU: add dismiss wait dialog here
	TBuf<100> msg=_L("Access point test result: ");
	msg.AppendNum(aError);
	msg.Append(_L(" for ap "));
	msg.AppendNum(aAp);
	LogAppEvent(msg);
	delete iAccessPointTester; iAccessPointTester=0;
}

void CContextLogAppUiBase::TestApL(TInt aAp)
{
	// TEEMU: add wait dialog here
	if (!iAccessPointTester) iAccessPointTester=CAccessPointTester::NewL(*this);
	iAccessPointTester->TestAp(aAp);
}
