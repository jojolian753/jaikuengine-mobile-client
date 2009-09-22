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

#include "sensorrunner.h"
#include "app_context.h"

#include "log_cellid.h"
#include "file_logger.h"
#include "bb_logger.h"
#include "discover.h"
#include "bases.h"
#include "log_local.h"
#include "log_idle.h"
#include "log_appuse.h"
#include "log_gps.h"
#include "operatormap.h"
#include "independent.h"
#include "log_systemevent.h"
#include "log_shareddata.h"
#include "bb_recovery.h"
#include "cl_connectionindicator.h"
#include "cc_processmanagement.h"
#include "cs_music.h"

const TInt KUidBatteryBarsValue = 0x100052D3;
const TUid KUidBatteryBars ={KUidBatteryBarsValue};
const TInt KUidChargerCStatusValue = 0x100052D7;
const TUid KUidChargerCStatus = {KUidChargerCStatusValue};
const TInt KUidNetworkStatusValue=0x100052C7;
const TUid KUidNetworkStatus  ={KUidNetworkStatusValue};

#include "keycapture.h"
#include "csd_system.h"
#include "csd_event.h"
#include "csd_battery.h"
#include "contextvariant.hrh"
#include "sleep.h"

#if !defined(FLICKR) && !defined(CONTEXTLOCA) 
#if !defined(__S60V2FP3__)
#include "log_alarm.h"
#include "log_profile.h"
#include "log_unread.h"
#ifndef CONTEXTJAIKU
#include "log_sms.h"
#endif
#endif // FP3
#endif
#ifndef CONTEXTLOCA
#include "log_calendar.h"
#include "csd_idle.h"
#include "csd_sms.h"
#endif
_LIT(KCharger, "charger");
_LIT(KNetworkStatus, "network_status");
_LIT(KBattery, "battery");
#include <bautils.h>
#include "callstack.h"
#include "csd_bluetooth.h"
#include "break.h"
#include <w32std.h>
#include <eikenv.h>
#include "cl_settings.h"
#include "symbian_auto_ptr.h"
#include "indep_watcher.h"
#include "cellnaming.h"

#include "contextlog_resource.h"
#include "app_context_impl.h"
#include "cl_settings_impl.h"
#include "bb_settings.h"
#include "btlist.h"
#include "presence_publisher.h"
#include "reporting.h"
#include "csd_presence.h"
#include "timeout.h"
#include "raii_f32file.h"
#ifndef __WINS__
const TInt KWatchDogTimeOut=5*60;
#else
const TInt KWatchDogTimeOut=1*60;
#endif
#include "jaikucacherclientsession.h"

_LIT(KUserGiven, "user_given");

class CSensorRunnerImpl : public CSensorRunner, public MContextBase, public MCellMapping,
	public MPresencePublisherListener, public MActiveErrorReporter,
	public MBBObserver, public MAppEvents, public MSettingListener,
	public MTimeOut {
public:
	CSensorRunnerImpl(MApp_context& aContext) : MContextBase(aContext)
		 { }

	Clog_cellid* lc;
	//Clog_soundlevel* ls;
#ifndef CONTEXTLOCA
	CLog_local*	ll;
	CLog_idle*	li;
	CLog_AppUse*	lau;
	independent_worker keycapture, bases_worker;
	TKeycaptureArgs	 iKeycaptureArgs;
#endif


#if !defined(FLICKR) && !defined(CONTEXTLOCA)
#if !defined(__S60V2FP3__)
#ifndef CONTEXTJAIKU
	CLog_sms*	lsms;
#endif
	Clog_profile* lp;
#endif // FP3
#endif
	CDiscover* discoverer;
	bases* base_counter;
	CDb		*iCellDb;
	CCellMap	*iCellMap;
	COperatorMap		*iOpMap;

	CArrayPtrFlat<Mlog_base_impl>	*iLoggers;
	CLogGps*		iGps;
	TBuf<50>	state;
	CIndependentWatcher*	iBasesWatcher;
	CCellNaming*			iCellNaming;
	class CDb			*iBTDb;
	class CBTDeviceList		*iBuddyBTs, *iLaptopBTs, *iDesktopBTs, *iPDABTs;
	PresencePublisher::CPresencePublisher*		iPresencePublisher;
	CSleepProfile*			iSleepProfile;
	CBBSubSession*			iBBSubSession;
	CTimeOut*				iWatchDogTimer;
	CConnectionIndicator*   iConnectionIndicator;
	RJaikuCacher		iJaikuCacher;
#ifdef __S60V3__
	CCenRepMusic			*iMusic;
#endif

	virtual CLogGps*	Gps() { return iGps; }
	virtual CCellMap*	CellMap() { return iCellMap; }
	virtual COperatorMap* OpMap() { return iOpMap; }
	virtual void NotifyNewPresence(const class CBBPresence* ) { }
	virtual void Stop() {
#ifndef CONTEXTLOCA
		LogAppEvent(_L("Stop: keycapture"));
		keycapture.stop();
		LogAppEvent(_L("Stop: bases"));
		bases_worker.stop();
		delete iBasesWatcher; iBasesWatcher=0;
#endif
		LogAppEvent(_L("Stop: loggers"));
		if (iLoggers) {
			iLoggers->ResetAndDestroy();
		}
		LogAppEvent(_L("Stopped: loggers"));
		delete iConnectionIndicator;
		delete iLoggers; iLoggers=0;
		delete base_counter; base_counter=0;
		delete iCellNaming; iCellNaming=0;
		delete iSleepProfile;
		LogAppEvent(_L("Stop: delete presence publisher"));
		delete iPresencePublisher;
		LogAppEvent(_L("Stop: delete bt databases"));
		delete iBTDb;
		delete iBuddyBTs;
		delete iLaptopBTs;
		delete iDesktopBTs;
		delete iPDABTs;
		iJaikuCacher.Close();
	}
	class MScreenChanged* ScreenChangeObserver() {
		return iConnectionIndicator;
	}
	~CSensorRunnerImpl() {
		LogAppEvent(_L("~CSensorRunnerImpl::Stop"));
		Stop();
		LogAppEvent(_L("delete iCellMap"));
		delete iCellMap;
		delete iCellDb;
		if (iOpMap) {
			iOpMap->Release(); 
			iOpMap=0;
		}
		delete iBBSubSession;
		delete iWatchDogTimer;
		Reporting().SetActiveErrorReporter(0);
	}
	TBool iHandlable;
	TFileName	iWatchFileName;
	virtual void SetInHandlableEvent(TBool aHandlable) {
		iHandlable=aHandlable;
	}

	void ReportError(const TDesC& Source,
		const TDesC& Reason, TInt Code)
	{
		_LIT(err, "error");
		TBBLongString msg(err);
		msg()=_L("Unhandled error ");
		msg().AppendNum(Code);
		msg().Append(_L(" "));
		msg().Append(Reason.Left(120));
		if (iLog) {
			TInt err;
			CC_TRAP(err, iLog->new_value(CBBSensorEvent::ERR, _L("APP"), msg(), GetTime()));
		}
	}

	void LogFormatted(const TDesC& aMsg)
	{
		if (iLog) {
			CC_TRAPD(err, iLog->new_value(CBBSensorEvent::ERR, _L("APP"), aMsg, GetTime()));
		}
	}
	virtual void LogAppEvent(const TDesC& msg) {
		LogFormatted(msg);
	}
	virtual void error(CBase* aSource, TInt err, const TDesC& msg) {
		ReportError(KNullDesC, msg, err);
	}

	Cfile_logger* iLog;
	void ConstructL(TBool aDoKeycapture, Cfile_logger* aLog ) {
		CALLSTACKITEM_N(_CL("CSensorRunnerImpl"), _CL("ConstructL"));

		iJaikuCacher.Connect();
		
		iLog=aLog;
		Reporting().SetActiveErrorReporter(this);
		if (iLog) {
			iLog->SubscribeL(KCellIdTuple);
#ifdef __DEV__
			iLog->SubscribeL(KCellNameTuple);
			iLog->SubscribeL(KCityNameTuple);
			iLog->SubscribeL(KCountryNameTuple);
			iLog->SubscribeL(KLocationTuple);
			iLog->SubscribeL(KBaseTuple);
#endif
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
			iLog->SubscribeL(KMusicTuple);
		}
		iBBSubSession=BBSession()->CreateSubSessionL(this);
#ifdef __DEV__
		iBBSubSession->AddNotificationL(KComponentVersionTuple);
		iBBSubSession->AddNotificationL(KComponentFunctionalityTuple);
		iBBSubSession->AddNotificationL(KComponentNameTuple);
		iBBSubSession->AddNotificationL(KComponentStateTuple);
#endif
		iBBSubSession->AddNotificationL(KComponentErrorCountTuple);
		iBBSubSession->AddNotificationL(KComponentErrorInfoTuple);

		iLoggers=new (ELeave) CArrayPtrFlat<Mlog_base_impl>(10);
		{
			TInt errd;
			TBool invisible=EFalse;
#ifdef CONTEXTLOCA
			invisible=ETrue;
#endif
			TRAP(errd, discoverer=CDiscover::NewL(AppContext(), invisible));
			if (errd==KErrNone) {
				iLoggers->AppendL(discoverer);
				iBTDb=CDb::NewL(AppContext(), _L("BTDEV"), EFileRead|EFileWrite|EFileShareAny);

				iBuddyBTs = CBTDeviceList::NewL(AppContext(), KBtBuddyDeviceList, &(iBTDb->Db()), _L("BUDDIES"));
				iLaptopBTs = CBTDeviceList::NewL(AppContext(), KBtLaptopDeviceList, &(iBTDb->Db()), _L("LAPTOPS"));
				iDesktopBTs = CBTDeviceList::NewL(AppContext(), KBtDesktopDeviceList, &(iBTDb->Db()), _L("DESKTOPS"));
				iPDABTs = CBTDeviceList::NewL(AppContext(), KBtPDADeviceList, &(iBTDb->Db()), _L("PDAS"));

			} else {
				error(this, errd, _L("Bluetooth scans not available: "));
				auto_ptr<HBufC> stack(CallStackMgr().GetFormattedCallStack(_L("AppUi")));
				LogAppEvent(*stack);
			}
		}
#ifndef NO_PRESENCE
		iPresencePublisher=PresencePublisher::CPresencePublisher::NewL(AppContext(), 0, *this, 
			iBuddyBTs, iLaptopBTs, iDesktopBTs, iPDABTs);
#endif

		iConnectionIndicator = CConnectionIndicator::NewL();		

		LogAppEvent(_L("Starting 1.2"));
		state=_L("create cell map");
		iCellDb=CDb::NewL(AppContext(), _L("CELLS"), EFileRead|EFileWrite);
		iOpMap=COperatorMap::NewL(AppContext()); 

		{
#ifndef __S60V3__
			TBool transfer_old_cell_ids=EFalse;
			if (! BaflUtils::FileExists(Fs(), _L("c:\\system\\data\\context\\cells.db"))) {
				transfer_old_cell_ids=ETrue;
			}
#endif
			iCellMap=CCellMap::NewL(AppContext(), iCellDb->Db(), iOpMap);

#ifndef __S60V3__
			if (transfer_old_cell_ids) {
				// NOTE: we must construct base_counter directly after
				// cellmap so that the mappings get transfered correctly
				// from the base db to the cell map db after upgrading

				LogAppEvent(_L("Starting 2"));

				state=_L("create base counter");
				base_counter=new (ELeave) bases(AppContext());
				base_counter->ConstructL(true, iCellMap);
				// base_counter->test(iOpMap, iCellMap); // NOP on ARMI
				delete base_counter; base_counter=0;
			}
#endif
		}
		iCellNaming=CCellNaming::NewL(AppContext(), this);
		state=_L("create log cellid");
		lc=Clog_cellid::NewL(AppContext(), iOpMap, iCellMap);
		iLoggers->AppendL(lc);

#if defined(__S60V3__) && defined(__DO_MUSIC__)
		iMusic=CCenRepMusic::NewL();	
		iLoggers->AppendL(iMusic);
#endif

#if !defined(FLICKR) && !defined(CONTEXTLOCA) && !defined(__S60V2FP3__)
#ifndef CONTEXTJAIKU
		if (smsh) {
			state=_L("create log sms");
			lsms=CLog_sms::NewL(AppContext(), KSMS);
			iLoggers->AppendL(lsms);
			smsh->AddHandler(lsms);
		}
#endif

		state=_L("create log profile");
		lp=Clog_profile::NewL(AppContext());
		iLoggers->AppendL(lp);

#  ifndef __S60V3__ 
//FIXME3RD
		{
			state=_L("create log alarm");
			auto_ptr<CLogAlarm> l(CLogAlarm::NewL(AppContext()));
			iLoggers->AppendL(l.get());
			l.release();
		}
		{
			state=_L("create log unread");
			CLogUnread* lu=0;
			CallStackMgr().ResetCallStack();
			CC_TRAPD(err, lu=CLogUnread::NewL(AppContext()));
			if (lu) {
				auto_ptr<CLogUnread> l(lu);
				iLoggers->AppendL(l.get());
				l.release();
			}
			CallStackMgr().ResetCallStack();
		}
#  endif
#endif
#ifndef CONTEXTLOCA
		{
			state=_L("create log calendar");
			auto_ptr<CLogCalendar> l(0);
			HBufC* busytext=0;
			/*if (CEikonEnv::Static()) {
				CEikonEnv::Static()->AllocReadResourceL(R_BUSY);
			} else*/ {
				//FIXMELOC
				busytext=HBufC::NewL(4);
				*busytext=_L("Busy");
			}
			CC_TRAPD(err, l.reset(CLogCalendar::NewL(AppContext(), busytext)));
			if (l.get()) {
				iLoggers->AppendL(l.get());
				l.release();
			} else {
				error(this, err, _L("CLogCalendar creation failed"));
			}
		}
#endif

#ifndef CONTEXTLOCA
		state=_L("create log appuse");
		lau=CLog_AppUse::NewL(AppContext());
		iLoggers->AppendL(lau);

		if (aDoKeycapture) {
			ll=CLog_local::NewL(AppContext(), KIdle);
			iLoggers->AppendL(ll);
			iKeycaptureArgs.iKeyStatus=ll->RequestStatus();
		}
#endif

#ifndef __S60V3__
//FIXME3RD
		state=_L("create system event loggers");
		Mlog_base_impl* l;
		l=CLog_SystemEvent::NewL(AppContext(), KCharger, KUidChargerCStatus, KChargerTuple); 
		iLoggers->AppendL(l);

		l=CLog_SystemEvent::NewL(AppContext(), KBattery, KUidBatteryBars, KBatteryTuple);
		iLoggers->AppendL(l);

		l=CLog_SystemEvent::NewL(AppContext(), KNetworkStatus, KUidNetworkStatus, KNetworkTuple); 
		iLoggers->AppendL(l);
#endif

#ifndef CONTEXTLOCA 
#if !defined(__S60V2FP3__)
		iGps=CLogGps::NewL(AppContext());
		iLoggers->AppendL(iGps); 

		if (aDoKeycapture) {
			state=_L("start keycapture");
			/*if (CEikonEnv::Static()) {
				RWindowGroup wg=CEikonEnv::Static()->RootWin();
				iKeycaptureArgs.iParentWg=wg.Identifier();
			} else*/ {
				iKeycaptureArgs.iParentWg=-1;
			}
			Settings().GetSettingL(SETTING_RIGHT_SOFTKEY_CONTEXT, iKeycaptureArgs.right_softkey_mapped);
			keycapture.start(_L("keycapture"), start_keycapture, (TAny*)&iKeycaptureArgs, 
				EPriorityAbsoluteForeground);
		}
#endif 		
#endif
#ifndef CONTEXTLOCA
		{
		state=_L("start bases");
		TInt wait_for_bases=12*60;
		//TInt wait_for_bases=5;
		iBasesWatcher=CIndependentWatcher::NewL(bases_worker.info,
			_L("bases"), *this, KLocationTuple, wait_for_bases);
		bases_worker.info.set_has_stopped( iBasesWatcher->GetStatus() );
		bases_worker.start(_L("bases"), bases::RunBasesInThread, 0, EPriorityLess);
		}
#endif
		iOpMap->Release(); iOpMap=0;

#ifndef NO_PRESENCE
#ifndef __S60V2FP3__
		iSleepProfile=CSleepProfile::NewL(iPresencePublisher);
#endif
		LogAppEvent(_L("Starting 2.1"));
		RestoreUserGiven();
		Settings().NotifyOnChange(SETTING_OWN_DESCRIPTION, this);

#endif
		iWatchDogTimer=CTimeOut::NewL(*this);
		iWatchDogTimer->Wait(5);

		state.Zero();
		iWatchFileName=DataDir();
		iWatchFileName.Append(_L("watch-context_log.dat"));
		expired(iWatchDogTimer);
	}
	virtual const TDesC& State() {
		return state;
	}
	void expired(CBase* aSource)
	{
		CALLSTACKITEM_N(_CL("CContextLogAppUiBase"), _CL("expired"));

		RAFile f; f.ReplaceLA(Fs(), iWatchFileName,
			EFileWrite|EFileShareAny);
		f.Write(_L8("watch"));
		iWatchDogTimer->Wait(KWatchDogTimeOut);
	}

	TBuf<100> iComponentMessage;
	void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData)
	{
		if (!aData) return;
		
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

	void DeletedL(const TTupleName& , const TDesC& ) { }
	void RestoreUserGiven()
	{
		CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("RestoreUserGiven"));
		
		if (!iPresencePublisher) return;
		const TInt m_length = iPresencePublisher->Data()->iUserGiven.iDescription.iValue.MaxLength();

		TTime usergiventime;
		auto_ptr<HBufC> usergiven(HBufC::NewL( m_length) );
		TPtr16 p = usergiven->Des();
		if ( !Settings().GetSettingL(SETTING_OWN_DESCRIPTION, p) )  return;
		if ( !Settings().GetSettingL(SETTING_OWN_DESCRIPTION_TIME, usergiventime) ) return;

		auto_ptr<CBBSensorEvent> e(new CBBSensorEvent(KUserGiven, KUserGivenContextTuple, 0, GetTime()));
		TBBUserGiven * ug = new (ELeave) TBBUserGiven(KUserGiven);
		ug->iDescription.iValue = *usergiven;
		ug->iSince.iValue = usergiventime;
		e->iData.SetValue(ug);
		
		TTime expires; expires=Time::MaxTTime();
		CC_TRAPD(err, iBBSubSession->PutL(e->TupleName(), KNullDesC, e.get(), expires));
	}
	void SettingChanged(TInt Setting)
	{
		if (Setting==SETTING_OWN_DESCRIPTION) {
			RestoreUserGiven();
		}
	}

	friend class CSensorRunner;
	friend class auto_ptr<CSensorRunnerImpl>;
};

CSensorRunner* CSensorRunner::NewL(class MApp_context& aContext,
	TBool aDoKeycapture, Cfile_logger* aLog)
{
	auto_ptr<CSensorRunnerImpl> r(new (ELeave) CSensorRunnerImpl(aContext));
	r->ConstructL(aDoKeycapture, aLog);
	return r.release();
}

class COwnActiveScheduler : public CBaActiveScheduler {
public:
	void Error(TInt aError) const {
		User::Leave(aError);
	}
	MApp_context* iContext;
	MApp_context& AppContext() {
		return *iContext;
	}
	void WaitForAnyRequest() 
	{
		if (iContext) {
			MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
			if (rep) rep->SetInHandlableEvent(EFalse);
			AppContext().CallStackMgr().ResetCallStack();
			AppContext().ErrorInfoMgr().ResetLastErrorInfo();
		} else {
			iContext=GetContext();
		}
		CBaActiveScheduler::WaitForAnyRequest();
	}

};

class CStopActive : public CActive {
public:
	CStopActive() : CActive(EPriorityNormal) { }
	void ConstructL(worker_info *wi) { 
		CActiveScheduler::Add(this);
		wi->set_do_stop(&iStatus);
		SetActive();
	}
	void RunL() {
		CActiveScheduler::Stop();
	}
	void DoCancel() {
		TRequestStatus* s=&iStatus;
		User::RequestComplete(s, KErrNone);
	}
	~CStopActive() {
		Cancel();
	}
};

class TAppEvents: public MAppEvents {
public:
	virtual void LogAppEvent(const TDesC& msg) { }
	virtual void error(CBase* aSource, TInt err, const TDesC& msg) { }
};

#include <APGWGNAM.H>

#include <e32property.h>
#include <connect/sbdefs.h>

class CBackupListener: public CActive {
public:
	CBackupListener() : CActive(EPriorityHigh) { }
	void ConstructL() {
		User::LeaveIfError(iProperty.Attach(KUidSystemCategory, conn::KUidBackupRestoreKey));
		CActiveScheduler::Add(this);
		iStatus = KRequestPending;
		iProperty.Subscribe(iStatus);
		SetActive();
	}
	void RunL() {
		TInt backupStateValue = 0;
		iProperty.Get(backupStateValue);
		if (backupStateValue==0 || 
				backupStateValue==(conn::EBURNormal | conn::ENoBackup)) {
			iProperty.Subscribe(iStatus);
			SetActive();
		} else {
			CActiveScheduler::Stop();
		}
	}
	void DoCancel() {
		iProperty.Cancel();
	}
	~CBackupListener() {
		Cancel();
		iProperty.Close();
	}
private:
	RProperty iProperty;
};

#include <eikenv.h>      // CEikonEnv

// Concepts:
// !Hiding a window group!
// !Responding to phone shutdown / uninstall!
class CMinimalTask : public CActive {
public:
	CMinimalTask() : CActive(EPriorityNormal) { }
	MScreenChanged* iScreenChangeObserver;
	CEikonEnv *iEnv;
	TBool iCreatedEnv;
	//RWsSession iSession;
	RWindowGroup* iWg;
	CApaWindowGroupName* iWgName;
	void ConstructL() {
		iEnv=CEikonEnv::Static();
		/*User::LeaveIfError(iSession.Connect());
		iWg=new (ELeave) RWindowGroup(iSession);
		User::LeaveIfError(iWg->Construct( (TUint32)iWg, EFalse));*/
		iWg=&(iEnv->RootWin());
		
		iWg->SetOrdinalPosition(-1);
		iWg->EnableReceiptOfFocus(EFalse);
		User::LeaveIfError(iWg->EnableScreenChangeEvents());
		iWgName=CApaWindowGroupName::NewL(Session());
		iWgName->SetHidden(ETrue);
		iWgName->SetSystem(ETrue);
		iWgName->SetAppUid( KUidcontext_log );
		iWgName->SetRespondsToShutdownEvent(ETrue);
		iWgName->SetWindowGroupName(*iWg);
		
		Session().EventReady(&iStatus);
		//iSession.EventReady(&iStatus);
		CActiveScheduler::Add(this);
		SetActive();
	}
	RWsSession& Session() {
		return iEnv->WsSession();
	}
	void DoCancel() {
		Session().EventReadyCancel();
	}
	~CMinimalTask() {
		Cancel();
		delete iWgName;
		if (iWg) {
			//iWg->Close();
			//delete iWg;
		}
		//iSession.Close();
	}
	void RunL() {
		TWsEvent e;
		Session().GetEvent(e);
		
		if (e.Type()==EEventUser) {
			TApaSystemEvent se=*(TApaSystemEvent *)e.EventData();
			if (se==EApaSystemEventShutdown) {
				GetContext()->CallStackMgr().SetIsExiting(ETrue);
				CActiveScheduler::Stop();
				return;
			}
		} else if (e.Type()==EEventScreenDeviceChanged) {
			if (iScreenChangeObserver) iScreenChangeObserver->ScreenChanged();
		}

		Session().EventReady(&iStatus);
		SetActive();
	}
};

void do_run_sensors(TAny* aPtr)
{
	worker_info *wi=(worker_info*)aPtr;

	auto_ptr<COwnActiveScheduler> s(new (ELeave) COwnActiveScheduler);
	CActiveScheduler* prev_s=CActiveScheduler::Replace(s.get());
	
	auto_ptr<CStopActive> stop(0);
	auto_ptr<CMinimalTask> task(0);
	auto_ptr<CBackupListener> bl(0);
	if (wi) {
		CActiveScheduler::Install(s.get());
		stop.reset(new (ELeave) CStopActive);
		stop->ConstructL(wi);
	} else {
		task.reset(new (ELeave) CMinimalTask);
		task->ConstructL();
		bl.reset(new (ELeave) CBackupListener);
		bl->ConstructL();
	}

	auto_ptr<CApp_context> appc(0);
	if (wi) {
		appc.reset(CApp_context::NewL(true, wi->name));
	} else {
		appc.reset(CApp_context::NewL(true, _L("context_log")));
	}
#ifndef __S60V3__
	appc->SetDataDir(_L("c:\\system\\data\\context\\"), false);
#endif
	appc->SetDebugLog(_L("context"), _L("sensorrunner"));
#ifndef __WINS__
	CC_TRAPD(err_starter, StartStarterL(_L("context_log"), KUidcontext_log, ETrue));
#else
	CC_TRAPD(err_starter, StartStarterL(_L("context_log"), KUidcontext_log, ETrue,
		CEikonEnv::Static()->WsSession()));
#endif
	auto_ptr<CBBDataFactory> bbf(CBBDataFactory::NewL());
	auto_ptr<CBBSession> bbs(CBBSession::NewL(*appc, bbf.get()));
	appc->SetBBSession(bbs.get());
	appc->SetBBDataFactory(bbf.get());
	TClSettings t;
	CBlackBoardSettings* settings=
		CBlackBoardSettings::NewL(*appc, t, KCLSettingsTuple);
	appc->SetSettings(settings);
	CConnectionOpener::CheckBootFileAndResetPermissionL(appc->Fs(), appc->DataDir()[0], *settings);

	auto_ptr<Cfile_logger> iLog(Cfile_logger::NewL(*appc, _L("log"), 
		CBBSensorEvent::INFO, 3));
	appc->SetFileLog(iLog.get());
	CC_TRAPD(err, iLog->new_value(CBBSensorEvent::ERR, _L("APP"), _L("starting"), GetTime()));


	auto_ptr<CBBRecovery> rec(CBBRecovery::NewL());
	appc->SetRecovery(rec.get());

#ifdef CONTEXTJAIKU
	auto_ptr<CSensorRunner> r(CSensorRunner::NewL(*appc, ETrue, iLog.get()));
#else
	auto_ptr<CSensorRunner> r(CSensorRunner::NewL(*appc, EFalse, iLog.get()));
#endif

	if (task.get()) task->iScreenChangeObserver=r->ScreenChangeObserver();
	
#ifndef __WINS__
	TRAP(err, ProcessManagement::StartApplicationL(KUidMeaningApp, KNullDesC, 5, EApaCommandBackground));
#endif
	
	CC_TRAPD(err_active, s->Start());
	
	TRAP(err, ProcessManagement::KillApplicationL(task->Session(), KUidMeaningApp));
	
	if (err_active!=KErrNone) {
		TBuf<40> msg=_L("active scheduler left: ");
		msg.AppendNum(err_active);
		CC_TRAPD(err, iLog->new_value(CBBSensorEvent::ERR, _L("APP"), msg, GetTime()));
	}
	CActiveScheduler::Replace(prev_s);
	User::LeaveIfError(err_active);
}

#ifdef __WINS__
IMPORT_C void AllocateContextCommonExceptionData();
#endif

// Need an AppUi if we have a CEikonEnv as it accesses one
// unconditionally on some devices, at least the E71.
class OurAppUi : public CEikAppUi {
 public:
  void ConstructL() {
    BaseConstructL(ENoAppResourceFile | ENoScreenFurniture);
  }
};

_LIT(KName, "SensorRunner");

TInt CSensorRunner::RunSensorsInThread(TAny* aPtr)
{
	CEikonEnv* iEnv=CEikonEnv::Static();
	CTrapCleanup *cl=0;
	TBool iCreatedEnv;
	if (!iEnv) {
		iEnv=new (ELeave) CEikonEnv;
		TRAPD(err, iEnv->ConstructL(EFalse));
		if (err!=KErrNone) {
			delete iEnv;
			User::Leave(err);
		}
    CEikAppUi* app_ui = new OurAppUi;
  	TRAP(err, app_ui->ConstructL());
		iCreatedEnv=ETrue;
	} else {
		cl=CTrapCleanup::New();
	}

	RSemaphore process_semaphore;
	TInt err=process_semaphore.OpenGlobal(KName);
	if (err==KErrNotFound) {
		err=process_semaphore.CreateGlobal(KName, 1);
	}
	if (err!=KErrNone) goto failed;
	err=process_semaphore.Wait(30*1000*1000);
	if (err!=KErrNone) goto failed2;

	{
		RThread me; me.SetPriority(EPriorityLess);
		me.RenameMe(KName);
	}
#ifdef __WINS__
	TRAPD(dummy, User::Leave(1));
	AllocateContextCommonExceptionData();
#endif

	CC_TRAP2(err,
	    do_run_sensors(aPtr), 0);

failed2:
	process_semaphore.Close();
failed:
	delete cl;
	if (iCreatedEnv) {
		iEnv->DisableExitChecks(ETrue);
		iEnv->DestroyEnvironment();
		//delete iEnv;
	}

	TTimeIntervalMicroSeconds32 w(50*1000);
	//TTimeIntervalMicroSeconds32 w(5*60*1000*1000);
	User::After(w);
	if (aPtr) {
		worker_info* wi=(worker_info*)aPtr;
		wi->stopped(err);
	}
	return err;
}
