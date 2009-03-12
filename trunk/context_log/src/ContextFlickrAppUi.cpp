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

#include "contextflickrappui.h"
#include "notifystate.h"
#include "presencemaintainer.h"
#include "file_logger.h"
#include "statusview.h"
#include <contextflickr.mbg>
#include "break.h"
#include <contextflickr.rsg>
#include "cellnaming.h"
#include <eikmenup.h>
#include "context_log.hrh"
#include "contextnotifyclientsession.h"
#include <bautils.h>
#include "cl_settings.h"
#include "viewids.h"
#include "mediarunner.h"
#include "settingsview.h"
#include  <sendui.h>
#ifndef __S60V3__
#include  <SENDNORM.RSG>
#endif
#include "splash.h"
#include <aknmessagequerydialog.h> 
#include "app_context_impl.h"

#ifndef __WINS__
_LIT(KIconFile, "c:\\system\\data\\contextflickr.mbm");
#else
_LIT(KIconFile, "z:\\system\\data\\contextflickr.mbm");
#endif

TInt CContextFlickrAppUi::InitializationSteps()
{
	return CContextLogAppUiBase::InitializationSteps()+4;
}
void CContextFlickrAppUi::StepDone()
{
	if (iSplash) iSplash->StepDone();
}

void CContextFlickrAppUi::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextFlickrAppUi"), _CL("ConstructL"));
#ifdef __WINS__
	TInt err;
	TBreakItem b(GetContext(), err);
#endif

	CContextLogAppUiBase::ConstructL();

	iLog->SubscribeL(KAppEventTuple);
	state=_L("create presence publisher");
	iPresenceMaintainer=CPresenceMaintainer::NewL(AppContext(),  
		iBuddyBTs, iLaptopBTs, iDesktopBTs, iPDABTs, this);
	StepDone();

	CContextLogAppUiBase::ConstructAfterPresenceMaintainerL();
	//SetDefaultViewL(*iStatusView);

	iContextRunning=CNotifyState::NewL(AppContext(), KIconFile);
	iContextRunning->SetCurrentState( EMbmContextflickrM, EMbmContextflickrM );
	StepDone();
	FinalConstructL();

	SetDefaultViewL(*iMediaRunner->MediaView());

	{
		auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KBasicSettingsViewId, AppContext(), 
			KBasicSettings, ETrue));
		AddViewL(iSettingsView.get());
		iSettingsView->SetPreviousLocalViewId(iMediaRunner->MediaPoolViewId());
		iSettingsView1=iSettingsView.release();
	}
	{
		auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KAdvancedSettingsViewId, AppContext(), 
			KAdvancedSettings, ETrue));
		AddViewL(iSettingsView.get());
		iSettingsView->SetPreviousLocalViewId(iMediaRunner->MediaPoolViewId());
		iSettingsView2=iSettingsView.release();
	}
	{
		auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KNetworkSettingsViewId, AppContext(), 
			KNetworkSettings, ETrue));
		AddViewL(iSettingsView.get());
		iSettingsView->SetPreviousLocalViewId(iMediaRunner->MediaPoolViewId());
		iSettingsView3=iSettingsView.release();
	}
	StepDone();

#ifndef __S60V3__
	//FIXME3RD
	iSendUi = CSendAppUi::NewL(Econtext_logCmdSoftkeyUpload, 0);
#endif

	TInt ap;
	if (! Settings().GetSettingL(SETTING_IP_AP, ap) ) {
		QueueOp( static_cast<TCallBack>(&CContextFlickrAppUi::FlShowSettings), 0 );
	}
	StepDone();
	delete iSplash; iSplash=0;
}

void CContextFlickrAppUi::FlShowSettings()
{
	TVwsViewId v( KUidContextFlickr, KNetworkSettingsViewId);
	ActivateViewL(v);
}

void CContextFlickrAppUi::RunOp(TCallBack op)
{
	TFlCallBack fl=static_cast<TFlCallBack>(op);
	((*this).*fl)();
}

CContextFlickrAppUi::~CContextFlickrAppUi()
{
#ifdef __WINS__
	User::Check();
#endif
	CALLSTACKITEM_N(_CL("CContextFlickrAppUi"), _CL("~CContextFlickrAppUi"));

	CLocalNotifyWindow::Destroy();
	delete iPresenceMaintainer;
	delete iContextRunning;
#ifndef __S60V3__
	//FIXME3RD
	delete iSendUi;
#endif
	delete iSplash;
#ifdef __WINS__
	User::Check();
#endif
}

void CContextFlickrAppUi::SendLogsL()
{
	auto_ptr<CDesCArrayFlat> attach(new (ELeave) CDesCArrayFlat(4));
	
	LogAppEvent(_L("Switching log file for sending"));
	iLog->switch_file();
	LogAppEvent(_L("Switched log file for sending"));

	TInt size=0;
	TBuf<20> names=_L("log*.txt");
	for (int j=0; j<2; j++) {
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
					attach->AppendL(logs);
					size+=e.iSize;
				}
			}
		}
		names=_L("starter*.txt");
	}
#ifndef __S60V3__
	//FIXME3RD
	TSendingCapabilities c( 0, 
		size, TSendingCapabilities::ESupportsAttachments);
	iSendUi->CreateAndSendMessagePopupQueryL(_L("Send log files"), c, 0, attach.get(), 
		KNullUid, 0, 0, 0, EFalse);
#endif
}

void KillExeL(const TDesC& aProcessName)
{
	TFindProcess f(_L("*"));
	TFullName t;
	RProcess r;
	TInt len=aProcessName.Length();
	if (len==0) User::Leave(KErrArgument);
	TInt found=0;
	while (f.Next(t)==KErrNone) {
		r.Open(t);
		CleanupClosePushL(r);
		if (r.FileName().Length()>=len && 
			r.FileName().Left(1).CompareF(_L("z"))!=0 && 
			aProcessName.CompareF(r.FileName().Right(len))==0) {
			r.Kill(2003);
			found++;
		}
		CleanupStack::PopAndDestroy();
	}
	if (found==0)
		User::Leave(KErrNotFound);
}

void MoveProfileL(TBool aRestore)
{
	KillExeL(_L("\\system\\apps\\starter\\starter.exe"));

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
		if (err!=KErrNone) {
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

void CContextFlickrAppUi::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CContextFlickrAppUi"), _CL("HandleCommandL"));
	SetInHandlableEvent(ETrue);

	TVwsViewId id; 
	if ( GetActiveViewId(id)!=KErrNone) {
		id.iViewUid=iMediaRunner->MediaPoolViewId();
	}
	iSettingsView1->SetPreviousLocalViewId( id.iViewUid );
	iSettingsView2->SetPreviousLocalViewId( id.iViewUid );
	iSettingsView3->SetPreviousLocalViewId( id.iViewUid );
#ifdef __WINS__
	TInt err;
	TBreakItem b(GetContext(), err);
#endif
	if (BaseHandleCommandL(aCommand)) return;

	switch ( aCommand )
	{
	case Econtext_logCmdBasicSettings:
		ActivateLocalViewL(KBasicSettingsViewId);
		break;
	case Econtext_logCmdNetworkSettings:
		ActivateLocalViewL(KNetworkSettingsViewId);
		break;
	case Econtext_logCmdAdvancedSettings:
		ActivateLocalViewL(KAdvancedSettingsViewId);
		break;
	case Econtext_logCmdSoftkeyUpload:
		SendLogsL();
		break;
	case Econtext_logCmdResetAll:
		{
		HBufC * message = CEikonEnv::Static()->AllocReadResourceLC(R_SURE_TO_RESET);
		CAknQueryDialog * dlg = CAknQueryDialog::NewL(CAknQueryDialog::ENoTone);
		CleanupStack::PushL(dlg);
		dlg->SetPromptL(*message);
		CleanupStack::Pop(dlg);
		if ( dlg->ExecuteLD(R_CL_CONFIRMATION_QUERY_DIALOG) ) {
			AppContext().RunOnShutdown(MoveProfile);
			Exit();
		} else {
		} 
		CleanupStack::PopAndDestroy(); //message
		}
		break;
	case Econtext_logCmdRestoreAll:
		AppContext().RunOnShutdown(RestoreProfile);
		Exit();
		break;

	default:
		User::Leave(KErrNotFound);
	};
}

void CContextFlickrAppUi::DynInitMenuPaneL(
					 TInt aResourceId,CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CContext_logAppUi"), _CL("DynInitMenuPaneL"));

	switch(aResourceId) {
	case R_LOCATION_MENU:
		if (iCellNaming && iCellNaming->locationing_available()) {
			aMenuPane->SetItemDimmed(Econtext_logCmdLocateViaOperator, EFalse);
		} else {
			aMenuPane->SetItemDimmed(Econtext_logCmdLocateViaOperator, ETrue);
		}
		break;
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
	/*case R_TROUBLESHOOT_MENU:
		if ( BaflUtils::FolderExists(Fs(), _L("c:\\system\\data\\context_old")) ) {
			aMenuPane->SetItemDimmed(Econtext_logCmdRestoreAll, EFalse);
		} else {
			aMenuPane->SetItemDimmed(Econtext_logCmdRestoreAll, ETrue);
		}
		break;*/
	}
}

// End of File  
