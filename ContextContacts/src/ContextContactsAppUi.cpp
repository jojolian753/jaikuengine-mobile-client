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

#include "ContextContactsAppUi.h"

#include "contextcontactsapp.h"
#include "ContextContactsContainer.h"
#include "contextcontactsdocument.h"
#include "ContextContactsTabGroups.h"
#include "ContextContactsView.h"
#include "ccn_about.h"
#include "ccn_commentsview.h"
#include "ccn_connectioninfo.h"
#include "ccn_feedengine.h"
#include "ccn_everybodysfeedview.h"
#include "ccn_authorfeedview.h"
#include "ccn_feedgroupview.h"
#include "ccn_viewnavigation.h"
#include "ccn_richpresenceview.h"
#include "ccn_menus.h"
#include "ccn_settingsui.h"

#include "nickform.h"

#include <ContextContacts.rsg>
#include "contextcontacts.hrh"

#ifndef __JAIKU__
#include "ContextContactsView2.h"
#include "presencedescriptionview.h"
#endif

#include "bberrorinfo.h"
#include "bb_recovery.h"
#include "cbbsession.h"
#include "ccu_activestate.h"
#include "ccu_buildinfo.h"
#include "ccu_constants.h"
//#include "ccu_feedfoundation.h"
#include "ccu_streamrenderers.h"
#include "ccu_userpics.h"
#include "ccu_contactmatcher.h"
#include "ccu_jaicons.h"
#include "ccu_phonebookui.h"
#include "ccu_poster.h"
#include "ccu_posterui.h"
#include "ccu_progressbar.h"
#include "ccu_staticicons.h"
#include "ccu_streamstatscacher.h"
#include "ccu_themes.h"
#include "ccu_timeperiod.h"
#include "ccu_utils.h"
#include "csd_sentinvite.h"
#include "csd_buildinfo.h"
#include "cu_cellnaming.h"
#include "cu_dynamicsettings.h"
#include "ContextCommon.h"
#include "jabberpics.h"
#include "juik_layout_impl.h"
#include "juik_iconmanager.h"
#include "juik_gfxstore.h"
#include "settingsview.h"
#include "reporting.h"

#include "app_context_impl.h"
#include "break.h"
#include "callstack.h"
#include "db.h"
#include "raii_array.h"
#include "settings.h"
#include "cl_settings.h"

#include "csd_presence.h"
#include "csd_event.h"
#include "emptytext.h"
#include "file_logger.h"
#include "cu_systemapp.h"
#include "..\..\context_log\inc\viewids.h"
#include "cu_common.h"
#include "raii_f32file.h"
#include "contextnotifyclientsession.h"
#include "btlist.h"
#include "cc_bt_dev_ap_view.h"
#include "sha1.h"
#include "cu_errorui.h"

#include "contextvariant.hrh"


#include <aknnotewrappers.h>
#include <avkon.hrh>
#include <CPbkContactEditorDlg.h>
#include <CPbkContactEngine.h>
#include <CPbkSelectFieldDlg.h>
#include <CPbkContactItem.h>
#include <cpbkphonenumberselect.h> 
#include <apgcli.h>
#include <eikmenup.h>
#include <CPbkViewState.h>
#include <bautils.h>
#include <aknmessagequerydialog.h> 
#include <flogger.h>
#include <EikDll.h>
#include <sysutil.h>
#include <StringLoader.h>
//#define COUNTING 1
#include "cc_processmanagement.h"
#include "cn_networkerror.h"
#include "ccu_platforminspection.h"
 
// Settings view

const TUid KJaikuSettingsViewId = {0x24};
const TInt CContextContactsAppUi::KJaikuSettings[] = {
	SETTING_CALENDAR_SHARING,
	SETTING_IP_AP,
	SETTING_JABBER_NICK,
	SETTING_JABBER_PASS,
#ifdef __DEV__ 
	SETTING_JABBER_ADDR,
	SETTING_JABBER_PORT,
#endif
	SETTING_ALLOW_NETWORK_ACCESS,
	SETTING_BT_SCAN_INTERVAL,
	-1
};


const TInt KUidToBackgroundValue=0x2004;
const TUid KUidToBackground={KUidToBackgroundValue};

_LIT(KUserGiven, "dummy");

#include "juik_fonts.h"
#include "reporting.h"

#include "connectioninit.h"

void CContextContactsAppUi::ConstructL()
{	
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ConstructL"));
	// Enabled for WINS now -MR 20061221
    //User::Leave(KErrNoMemory); // test code -MR
	//User::Leave(KErrDiskFull);
	{
		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("BaseConstructL"));
#ifndef __S60V2__
		BaseConstructL(0x1008);
#else
		BaseConstructL(EAknEnableSkin);
#endif
	}
	{
		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ConnectionRequestedL"));
		TInt enabled=1;
		Settings().GetSettingL(SETTING_PRESENCE_ENABLE, enabled);
		if (enabled) CNetworkError::ConnectionRequestedL();
	}

	{
		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("IsRunningL(KUidcontext_log"));
		if (! ProcessManagement::IsRunningL(iEikonEnv->WsSession(), KUidcontext_log) ) {
			Settings().WriteSettingL(SETTING_LAST_CONNECTION_ERROR, _L(""));
		}
	}
	
#ifndef __WINS__1
	{
		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("StartContextLog"));
		CC_TRAPD(ignore, StartContextLogL());
	}
#endif

#ifdef __WINS__
	Reporting().DebugLog( _L("In ConstructL") );
	TInt dummy;
	TBreakItem b(GetContext(), dummy);	
#endif

	{
		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("MContextAppUi::ConstructL"));
#ifndef __JAIKU__
	MContextAppUi::ConstructL(AppContext(), _L("book_log"));
#else
	MContextAppUi::ConstructL(AppContext(), _L("book_log"), 3);
#endif
	}

	Reporting().SetActiveErrorReporter(this);

	WrapInnerConstructL();	
}



void CContextContactsAppUi::InnerConstructL()
{	
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("InnerConstructL"));

	WriteVersionToLogL();

#ifdef __DEV__
	{
		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("PlatformInspection"));
		auto_ptr<CPlatformInspection> inspection( CPlatformInspection::NewL() );
		inspection->LogAllL();
	}
#endif
	
 	N70InitL();
	
	ConstructNotifyWindowL();
 	
	ConstructBackendL();
	
	ConstructPhonebookAndPresenceL();
	
	ConstructLayoutL();

	ConstructIconsL();
	
	ConstructUiHelpersL();
	
	ConstructViewsL();
	
	iEikonEnv->AddForegroundObserverL(*this);
	
	if (iLog) iLog->write_line(_L("Started 3"));
	((CContextContactsDocument*)(Document()))->AppUiConstructed();
}



void CContextContactsAppUi::WriteVersionToLogL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("WriteVersionToLogL"));
	TBBBuildInfo info(_L("buildinfo") );
	CC_TRAPD( err, BuildInfo::ReadL( info ) );

	auto_ptr<HBufC> str( HBufC::NewL(500) );	
	if ( err == KErrNone )	
		{
			TPtr ptr = str->Des();
			info.IntoStringL( ptr );
		}
	else if ( err == KErrNotFound )
		{
			str->Des().Append( _L("Version unknown: buildinfo.xml couldn't be found") );
		}
	else
		{
			User::Leave( err );
		}		
	if (iLog) iLog->write_line( *str );
}


void CContextContactsAppUi::N70InitL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("N70InitL"));
	if (iLog) iLog->write_line(_L("N70InitL"));
	
	// FIXME: is this really N70 only, or S60 2nd FP3 specific issue?

	TBool N70=EFalse;
	
#if defined(__S60V2__) && !defined(__S60V3__)
	{
		HBufC8* agent=SysUtil::UserAgentStringL();
		if ( (*agent).Left(8).Compare(_L8("NokiaN70"))==0) N70=ETrue;
		delete agent;
	}	
#endif
	if (N70) iEikonEnv->SetSystem(ETrue);
}

void CContextContactsAppUi::ConstructNotifyWindowL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ConstructNotifyWindowL"));
	if (iLog) iLog->write_line(_L("ConstructNotifyWindowL"));

	//iUninstallSupport=CUninstallSupport::NewL();
	TInt err = KErrNone;
	CC_TRAP(err, CLocalNotifyWindow::CreateAndActivateL());
	if (err!=KErrNone) {
		ReportError( _L("AppUi"), _L("ConstructNotifyWindowL"), err );
	}
}

void CContextContactsAppUi::ConstructBackendL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ConstructBackendL"));
	if (iLog) iLog->write_line(_L("ConstructBackendL"));

	TInt err = KErrNone; 
	CC_TRAP(err, { iJabberDb=CDb::NewL(AppContext(), _L("JABBER"), EFileWrite|EFileShareExclusive); } );
	if (err == KErrNone) {
		GetContext()->TakeOwnershipL(iJabberDb);

		iJabberData=CJabberData::NewL(AppContext(), *iJabberDb, SETTING_JABBER_NICK);
		GetContext()->TakeOwnershipL(iJabberData);

		iPresenceHolder=CPresenceHolder::NewL(*iJabberData);
		GetContext()->TakeOwnershipL(iPresenceHolder);
	} else {
		if (iLog) iLog->write_line(_L("error reading jabber db."));
		User::Leave( err );
	}
	
	iFeedEngine = CFeedEngine::NewL(*iJabberData);
	GetContext()->TakeOwnershipL( iFeedEngine ); 
	
}


void CContextContactsAppUi::ConstructPhonebookAndPresenceL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ConstructPhonebookAndPresenceL"));
	if (iLog) iLog->write_line(_L("ConstructPhonebookAndPresenceL"));
	
	iPhonebook=new (ELeave) phonebook(AppContext(), iJabberData, iPresenceHolder);
	GetContext()->TakeOwnershipL( iPhonebook );
	iPhonebook->ConstructL();	
	if (iPresenceHolder) iPresenceHolder->AddListener(iPhonebook);

	iPresenceUpdater = new (ELeave) CPresenceUpdater(*iPhonebook);
	GetContext()->TakeOwnershipL( iPresenceUpdater );
	iPresenceUpdater->ConstructL();
	
	iStreamStatsCacher = CStreamStatsCacher::NewL(*iJabberData, *iPhonebook, iFeedEngine->Storage() );
	GetContext()->TakeOwnershipL( iStreamStatsCacher );
}

void CContextContactsAppUi::ConstructLayoutL()
{
	if (iLog) iLog->write_line(_L("Starting 2"));
	iLayout = CJuikLayout::NewL();
	GetContext()->SetLayout( iLayout );
}


void CContextContactsAppUi::ConstructIconsL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ConstructIconsL"));
	if (iLog) iLog->write_line(_L("ConstructIconsL"));
	
	iIconManager = CJuikIconManager::NewL();
	GetContext()->TakeOwnershipL( iIconManager );
	GetContext()->SetIconManager( iIconManager );

	{
		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("LoadContextContactsUiIconsL"));
		StaticIcons::LoadContextContactsUiIconsL( *iIconManager );
	}

 	{
 		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("LoadAvkonIconsL"));
 		StaticIcons::LoadAvkonIconsL( *iIconManager );
 	}

	if (iLog) iLog->write_line(_L("Starting 2.1.2"));
	iJabberPics = CJabberPics::NewL(*iJabberDb);
	GetContext()->TakeOwnershipL( iJabberPics );

	if (iLog) iLog->write_line(_L("Starting 2.1.3"));
	iUserPics = CUserPics::NewL(*iJabberPics, *iJabberData, IconManager());
	GetContext()->TakeOwnershipL( iUserPics );

	if (iLog) iLog->write_line(_L("Starting Jaicons"));
	iJaicons = CJaicons::NewL( *iIconManager );
	GetContext()->TakeOwnershipL( iJaicons );
}


void CContextContactsAppUi::ConstructUiHelpersL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ConstructUiHelpersL"));
	if (iLog) iLog->write_line(_L("ConstructUiHelpersL"));

	// for phonebook dialogs
	iResource_files=new (ELeave) CArrayFixFlat<TInt>(5);
	
	LoadPhonebookViewResourceL();
	LoadContextContactsUiResourceL();

	iActiveState = CActiveState::NewL( *iPhonebook, *iJabberData, *iUserPics, iFeedEngine->Storage()  );
	GetContext()->TakeOwnershipL( iActiveState );

	iPeriodFormatter = CTimePeriodFormatter::NewL();
	GetContext()->TakeOwnershipL( iPeriodFormatter );

	iPhoneHelper = new (ELeave) phonehelper_ui(iLog);
	iPhoneHelper->ConstructL();

	iPoster = CPoster::NewL( *iJabberData, iFeedEngine->Storage(), *iPresenceHolder  );
	iPosterUi = CPosterUi::NewL( *iPoster );

	iMessaging=CMessaging::NewL(iPhonebook, iPhoneHelper, iJabberData);
	GetContext()->TakeOwnershipL( iMessaging );

 	iAboutUi = CAboutUi::NewL();
	GetContext()->TakeOwnershipL( iAboutUi );

	iViewNavigation = CViewNavigation::NewL();
	GetContext()->TakeOwnershipL( iViewNavigation );

	iThemeColors = CThemeColors::NewL();	
	GetContext()->TakeOwnershipL( iThemeColors );

	iFeedGraphics = CJuikGfxStore::NewL( StaticIcons::ContextContactsUiIconFile() );	
	GetContext()->TakeOwnershipL( iFeedGraphics );

	iCommonMenus = CJaikuCommonMenus::NewL();
	GetContext()->TakeOwnershipL( iCommonMenus );

	iProgressBarModel = CProgressBarModel::NewL( *iPeriodFormatter );
	GetContext()->TakeOwnershipL( iProgressBarModel );

	iSettingsUi = CSettingsUi::NewL(*iAboutUi);
	GetContext()->TakeOwnershipL( iSettingsUi );
}




void CContextContactsAppUi::ConstructViewsL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ConstructViewsL"));
	// Contact list view
	{ 
		iContactsView = new (ELeave) CContextContactsView(iLog,  
														  *iPhoneHelper, 
														  iPresenceHolder, 
														  iMessaging);
		SetViewDependenciesL( *iContactsView );
		iContactsView->SetParentViewL( iContactsView->ViewId() );
		iContactsView->ConstructL();
		AddViewL( iContactsView );      // transfer ownership to CAknViewAppUi
	}

	// Contact card view
	{
		iContactDetailView = new (ELeave) CContextContactsDetailView(*iPhoneHelper, iMessaging);
		SetViewDependenciesL( *iContactDetailView );
		iContactDetailView->SetParentViewL( iContactsView->ViewId() );
		iContactDetailView->ConstructL();
		AddViewL( iContactDetailView );      // transfer ownership to CAknViewAppUi
	}


#ifndef __JAIKU__
	// ContextPhone presence details view (shows data)
	{
		auto_ptr<CPresenceDetailView> detailview(CPresenceDetailView::NewL());
		AddViewL(detailview.get());
		iPresenceDetailView=detailview.release();	
	}
	
	// ContextPhone presence description view (shows presence line)
	{
		auto_ptr<CPresenceDescriptionView> Descriptionview(CPresenceDescriptionView::NewL());
		AddViewL(Descriptionview.get());
		iPresenceDescriptionView=Descriptionview.release();
	}
#endif


	// Single feed item view
	{ 
		iCommentsView = CCommentsView::NewL(*iPoster);
		SetViewDependenciesL( *iCommentsView );
		iCommentsView->SetParentViewL( iContactsView->ViewId() );
		iCommentsView->ConstructL();
		AddViewL( iCommentsView );
	}

	// Feed group view
	{
		iFeedGroupView = CFeedGroupView::NewL(*iCommentsView);
		SetViewDependenciesL( *iFeedGroupView );
		iFeedGroupView->SetParentViewL( iContactsView->ViewId() );
		iFeedGroupView->ConstructL();
		AddViewL( iFeedGroupView );
	}

	// Feed view for all your contacts
	{
		iEverybodysFeedView = CEverybodysFeedView::NewL(*iCommentsView);
		SetViewDependenciesL( *iEverybodysFeedView );
		iEverybodysFeedView->SetParentViewL( iContactsView->ViewId() );
		iEverybodysFeedView->ConstructL();
		AddViewL( iEverybodysFeedView );
	}

	// Feed from one author
	{
		iAuthorFeedView = CAuthorFeedView::NewL(*iCommentsView);
		SetViewDependenciesL( *iAuthorFeedView );
		iAuthorFeedView->SetParentViewL( iContactsView->ViewId() );
		iAuthorFeedView->ConstructL();
		AddViewL( iAuthorFeedView );
	}

	// Jaiku Presence view
	{
		CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("Creating RichPresenceView"));
		iRichPresenceView = CRichPresenceView::NewL();
		SetViewDependenciesL( *iRichPresenceView );
		iRichPresenceView->SetParentViewL( iAuthorFeedView->ViewId() );
		iRichPresenceView->ConstructL();
		AddViewL( iRichPresenceView );
	}


	// Settings view
	{ 
		auto_ptr<CSettingsView> settingsView(CSettingsView::NewL(KJaikuSettingsViewId, AppContext(), 
																  KJaikuSettings, ETrue));
		AddViewL(settingsView.get());
		settingsView->SetPreviousLocalViewId( KViewId );
		iSettingsView=settingsView.release();
	}
	
	// Settings view
	{ 
		iSettingsView2 = iSettingsUi->CreateSettingsViewL();
		AddViewL( iSettingsView2 );
		iSettingsView2->SetPreviousLocalViewId( KViewId );
	}
	
	// Bluetooth buddies view
	{
		iBuddyBTs = CBTDeviceList::NewL(AppContext(), KBtBuddyDeviceList, 0, _L("BUDDIES"));
		auto_ptr<CBTDevApView> btView(CBTDevApView::NewL(AppContext(), 
			iBuddyBTs, KBtDevSetBuddiesViewId, 0, EFalse));
		SetViewDependenciesL( *btView );
		btView->SetParentViewL( iRichPresenceView->ViewId() );
		AddViewL(btView.get());
		iBuddyBTView = btView.release();
	}

	// Bluetooth laptops view
	{
		iLaptopBTs = CBTDeviceList::NewL(AppContext(), KBtLaptopDeviceList, 0, _L("LAPTOPS"));
		auto_ptr<CBTDevApView> btView(CBTDevApView::NewL(AppContext(), 
														 iLaptopBTs, KBtDevSetLaptopsViewId, 0, EFalse));
		SetViewDependenciesL( *btView );
		btView->SetParentViewL( iRichPresenceView->ViewId() );
		AddViewL(btView.get());
		iLaptopBTView = btView.release();
	}

	// Bluetooth pdas view
	{
		iPDABTs = CBTDeviceList::NewL(AppContext(), KBtPDADeviceList, 0, _L("PDAS"));
		auto_ptr<CBTDevApView> btView(CBTDevApView::NewL(AppContext(), 
			iPDABTs, KBtDevSetPDAsViewId, 0, EFalse));
		SetViewDependenciesL( *btView );
		btView->SetParentViewL( iRichPresenceView->ViewId() );
		AddViewL(btView.get());
		iPDABTView = btView.release();
	}

	// Bluetooth desktops view
	{
		iDesktopBTs = CBTDeviceList::NewL(AppContext(), KBtDesktopDeviceList, 0, _L("DESKTOPS"));
		auto_ptr<CBTDevApView> btView(CBTDevApView::NewL(AppContext(), 
														 iDesktopBTs, KBtDevSetDesktopsViewId, 
														 0, EFalse));
		SetViewDependenciesL( *btView );
		btView->SetParentViewL( iRichPresenceView->ViewId() );
		AddViewL(btView.get());
		iDesktopBTView = btView.release();
	}
	
	// Connection error info view
	{
		iErrorInfoView = CErrorInfoView::NewL();
		AddViewL( iErrorInfoView );
	}

	SetDefaultViewL(*iContactsView);
	
}

void CContextContactsAppUi::LoadPhonebookViewResourceL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("LoadPhonebookViewResourceL"));
#ifndef __S60V3__
	TFileName resfile=_L("z:\\System\\data\\PBKVIEW.RSC");
	BaflUtils::NearestLanguageFile(iEikonEnv->FsSession(), resfile); //for localization
	iResource_files->AppendL(iEikonEnv->AddResourceFileL(resfile));
#else
	iPbkResource.OpenL();
#endif
}

void CContextContactsAppUi::LoadContextContactsUiResourceL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("LoadContextContactsUiResourceL"));
	TInt handle = LoadSystemResourceL( iEikonEnv, _L("contextcontactsui") );
	iResource_files->AppendL(handle);
}


void CContextContactsAppUi::SetViewDependenciesL( CJaikuViewBase& aView ) const
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("SetViewDependenciesL"));
	aView.SetDependenciesL( *iActiveState, 
							*iUserPics, 
							*iJabberData, 
							*iPhonebook, 
							iFeedEngine->Storage(), 
							*iViewNavigation,
							*iPosterUi,
							*iJaicons,
							*iThemeColors,
							*iPeriodFormatter,
							*iPresenceHolder,
							*iFeedGraphics,
							*iStreamStatsCacher,
							*iCommonMenus,
							*iProgressBarModel);	
}


CContextContactsAppUi::CContextContactsAppUi(MApp_context& Context) : MContextBase(Context)
#ifdef __S60V3__
, iPbkResource(*iEikonEnv)
#endif
{ }

CContextContactsAppUi::~CContextContactsAppUi() {
	CC_TRAPD(err, ReleaseCContextContactsAppUi());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}

void CContextContactsAppUi::ReleaseCContextContactsAppUi()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("~CContextContactsAppUi"));

	if (iLog) {
		iLog->write_line(_L("Closing Contacts"));
	}
	if (iContactsView) iContactsView->before_exit();


	iEikonEnv->RemoveForegroundObserver(*this);
	if (iResource_files) {
		for (int i=0; i<iResource_files->Count(); i++) {
			iEikonEnv->DeleteResourceFile((*iResource_files)[i]);
		}
		delete iResource_files;
	}
#ifdef __S60V3__
	iPbkResource.Close();
#endif
	delete iPhoneHelper;
	
	CLocalNotifyWindow::Destroy();
	
	delete iBuddyBTs;
	delete iLaptopBTs;
	delete iDesktopBTs;
	delete iPDABTs;

	delete iPosterUi;
	delete iPoster;
	
	delete iLayout;
	delete iUninstallSupport;
	Reporting().SetActiveErrorReporter(0);
	
}


#ifndef KAknUidValueEndKeyCloseEvent
#define KAknUidValueEndKeyCloseEvent 0x101F87F0
#endif

void CContextContactsAppUi::HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination)
{
	MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);
	
	if (iFeedEngine) iFeedEngine->Storage().StopBackgroundActivities();
	// FIXME: 
	// This is "a hack proper" to get red key handling work similarly to Back-key.
	// I don't even know for sure that is this code for Red Key or something else
	// but it seems to come only when red key is pressed. Investigate with better time.
	if ( aEvent.Type() == KAknUidValueEndKeyCloseEvent ||
		 (aEvent.Type() == EEventKey && aEvent.Key()->iScanCode==EStdKeyNo) )
		{	
			iContactsView->ResetListFocusL();
			iNextViewUid=iContactsView->Id();
			Reporting().DebugLog( _L("HandleWsEventL"), aEvent.Type() );
			// at least on 3rd ed, the red key puts the app in the background anyways
			//HandleCommandL(EAknSoftkeyBack);
			return;
		}
	CC_TRAPD(err, CAknViewAppUi::HandleWsEventL( aEvent, aDestination ));
	if (err==KErrCancel) 
		return;
		
	if (err==KErrNoMemory || err==KErrDiskFull)
		if (rep) rep->SetInHandlableEvent(EFalse);
		
	User::LeaveIfError(err);
}



TKeyResponse CContextContactsAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("HandleKeyEventL"));
	return EKeyWasNotConsumed;

// 	if ( aType != EEventKey )
// 		return EKeyWasNotConsumed;
	
// 	TUid uid = TUid::Null();
// 	switch ( aKeyEvent.iCode )
//         {
// 		case EKeyLeftArrow:
// 			uid = iViewNavigation->LeftViewL();
// 			break;
// 		case EKeyRightArrow:
// 			uid = iViewNavigation->RightViewL();
// 			break;
// 		default:
// 			return EKeyWasNotConsumed;
// 			break;
//         }
// 	if ( uid != TUid::Null() )
// 		{
// 			ActivateLocalViewL(uid);
// 			return EKeyWasConsumed;
// 		}
// 	else 
// 		return EKeyWasNotConsumed;
}

#include <errorui.h>

TErrorHandlerResponse CContextContactsAppUi::HandleError(TInt aError,
     const SExtendedError& aExtErr,
     TDes& aErrorText,
     TDes& aContextText)
{

	MErrorInfo* ei=ErrorInfoMgr().GetLastErrorInfo();
	if (1 && ei && ei->ErrorCode()==KSyncBrokeIterator) {
		auto_ptr<CAknGlobalNote> note(CAknGlobalNote::NewL());
		note->ShowNoteL(EAknGlobalInformationNote, _L("Closing Jaiku for Contacts Sync"));
		User::Leave(KLeaveExit);
	}
	
	auto_ptr<CErrorUI> eui(CErrorUI::NewL(*iEikonEnv));
	auto_ptr<CAknGlobalNote> note(CAknGlobalNote::NewL());
	const TDesC& msg=eui->TextResolver().ResolveErrorString(aError);
	if (msg.Length()>0) {
		note->ShowNoteL(EAknGlobalErrorNote, msg);
	} else {
		TBuf<30> msg=_L("Error: ");
		msg.AppendNum(aError);
		note->ShowNoteL(EAknGlobalErrorNote, msg);
	}
        TErrorHandlerResponse resp=MContextAppUi::HandleError(aError, aExtErr, aErrorText, aContextText);
        
        return ENoDisplay;
        
#ifndef __S60V3__
        return CAknAppUi::HandleError(aError, aExtErr, aErrorText, aContextText);
#else
	return resp;
#endif
}

#include "cc_processmanagement.h"
#include "connectioninit.h"

void CContextContactsAppUi::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("HandleCommandL"));
	
	Reporting().DebugLog( _L("CContextContactsAppUi::HandleCommandL"), aCommand);

	if ( iCommonMenus->HandleCommandL(aCommand) )
		return;
	
	TBuf<30> msg;
	switch ( aCommand )
	{
		case EContextContactsCmdShutdown:
			GetContext()->CallStackMgr().SetIsExiting(ETrue);
		{			
			CC_TRAPD(ignored, CConnectionOpener::ResetPermissionL(Settings()));
#ifdef __JAIKU__
			// 1 Show information note 
			
			CC_TRAP(ignored, CNetworkError::ResetRequestedL());

			CC_TRAP(ignored, {
				auto_ptr<HBufC> message( StringLoader::LoadL( R_TXT_DISCONNECTING_JAIKU ) );

				auto_ptr<CAknGlobalNote> note( CAknGlobalNote::NewL() );
				note->ShowNoteL(EAknGlobalInformationNote, *message);
			});
			
			// 2 Kill server process
			CC_TRAP(ignored, 
				ProcessManagement::KillApplicationL(iEikonEnv->WsSession(), KUidJaikuSettings, 200));
#endif
			msg=_L("EContextContactsCmdShutdown");
			// 3 Exit Application (fallthru)
		}
		// fallthru
	case EEikCmdExit:
		//PruneFeedItemsL();

		GetContext()->CallStackMgr().SetIsExiting(ETrue);
		if (msg.Length()==0) msg=_L("EEikCmdExit");
		if (iLog) {
				iLog->new_value(
								CBBSensorEvent::INFO, _L("app_event"), 
								msg, GetTime());
			}
		iContactsView->before_exit();
		Exit();
		break;
		case EContextContactsCmdAppNameLocation:
			AskUserForCurrentLocationL();
			break;
		case EcontextContactsCmdAppNameCell:
			AskUserForCurrentCellNameL();
			break;
		case EcontextContactsCmdAppNameCity:
			AskUserForCurrentCityNameL();
			break;
		case EContextContactsCmdCall:
			HandleCallL();
			break;
     	case EContextContactsCmdAddJabber:
	    case EContextContactsCmdEditJabber:
			EditNickL(ETrue);
			break;
    	case EContextContactsCmdShowJabber:
	    	EditNickL(EFalse);
			break;
		case EContextContactsCmdInvite:
			SendInvitationSMSL();
			break;
	    case EContextContactsCmdShowJabberError:
			ShowConnectionErrorL();
			break;

		case EContextContactsCmdCrash:
		{
			TBuf<2> b; b.Append(_L("xxx"));
		}
		case EAknSoftkeyBack:
		{
			hide();
			iDidBack=ETrue;
			break;
		}
		case EContextContactsCmdSetUserDesc:
			{
				PostJaikuL();
				break;
			}
		case EContextContactsCmdInfoMem:
		{
			HandleInfoMemL();
			break;
		}
		
		case EContextContactsCmdOpen:
		{
#if defined(__WINS__MIKA_TEST)
//#if 1
			{
				RAFile f; f.ReplaceLA(Fs(), _L("e:\\fill.txt"), EFileWrite);
				TInt err=KErrNone;
				auto_ptr<HBufC8> buf(HBufC8::NewL(128*1024));
				buf->Des().SetLength( buf->Des().MaxLength() );
				buf->Des().FillZ();
				TInt count=0;
				while (err==KErrNone && count<500) {
					err=f.Write(*buf);
					count++;
				}
			}
#endif
			HandleOpenContactL();
			break;
		}

	case EContextContactsCmdDisplayContactDetails:
		{
			HandleDisplayContactDetailsL();
			break;
		}
		
	case EContextContactsCmdDisplayRichPresence:
		{
			HandleDisplayRichPresenceL();
			break;
		}

		case EContextContactsCmdSimDir:
#ifndef __S60V3__
		{
#ifndef __WINS__
			auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\simdirectory\\simdirectory.app")));
#else
			auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\calcsoft\\calcsoft.app")));
#endif
			cmd->SetCommandL(EApaCommandRun);
			CC_TRAPD(err, EikDll::StartAppL(*cmd));
		}
#else
			ActivateViewL( TVwsViewId( TUid::Uid(0x101f4cf6), TUid::Uid(1)) );
#endif
			break;
	case EContextContactsCmdSettings:
			{
				TUid current = iViewNavigation->CurrentView();
				if ( current != TUid::Null() )
					{
						iSettingsView2->SetPreviousLocalViewId( current );
					}
				ActivateLocalViewL( iSettingsView2->Id() );
				break;
			}
#ifdef __JAIKU__
	case EContextContactsCmdAdvancedSettings:
		{
		    TVwsViewId viewId( KUidJaikuSettings,
				       KStatusView );
				       
		    ActivateViewL(viewId);
			break;
		}
	case EContextContactsCmdBtFriends:
		ActivateLocalViewL( KBtDevSetBuddiesViewId );
		break;
	case EContextContactsCmdBtLaptop:
		ActivateLocalViewL( KBtDevSetLaptopsViewId );
		break;
	case EContextContactsCmdBtDesktop:
		ActivateLocalViewL( KBtDevSetDesktopsViewId );
		break;
	case EContextContactsCmdAbout:
		ShowAboutDialogL();
		break;
	case EContextContactsCmdShowLastJaikuViewError:
		ShowLastJaikuViewErrorL();
		break;
	case EContextContactsCmdContactsStream:
		ActivateLocalViewL( KEverybodysFeedView );
		break;
	case EContextContactsCmdCommentsView:
		{
			CAknView* current = View( iViewNavigation->CurrentView() );
			if ( current )
				{
					iCommentsView->SetParentViewL( current->ViewId() );
				}
			ActivateLocalViewL( KCommentsView );
		}
		break;
	case EContextContactsCmdFeedGroupView:
		{
			CAknView* current = View( iViewNavigation->CurrentView() );
			if ( current )
				{
					if ( current->ViewId() == iCommentsView->ViewId() )
						; //iFeedGroupView->SetParentViewL( current->ViewId() );
					else
						iFeedGroupView->SetParentViewL( current->ViewId() );
				}
			ActivateLocalViewL( KFeedGroupView );
		}
		break;
	case EContextContactsCmdPersonStream:
		DisplayPersonalStreamL( );
		break;
	case EContextContactsCmdLeftView:
		{
			TUid uid = iViewNavigation->LeftViewL();
			ActivateJaikuViewL( uid );
		}
		break;
	case EContextContactsCmdRightView:
		{
			TUid uid = iViewNavigation->RightViewL();
			ActivateJaikuViewL( uid );
		}
		break;
#endif 

	default:
			break;      
	}
}

_LIT(KStandby, "Jaiku will remain in standby");
#include <aknnotifystd.h>

void CContextContactsAppUi::ShowHideNoteIfNecessaryL()
{
	if (!iDidBack) return;
	iDidBack=EFalse;
	TBool do_show=ETrue;
	Settings().GetSettingL(SETTING_SHOW_WARNING_ON_BACK, do_show);
	if (do_show) {
		// FIXME:loc
		auto_ptr<CAknGlobalNote> note(CAknGlobalNote::NewL());
		note->ShowNoteL(EAknGlobalInformationNote, KStandby);
		Settings().WriteSettingL(SETTING_SHOW_WARNING_ON_BACK, EFalse);
	}
}

void CContextContactsAppUi::ActivateJaikuViewL(TUid aUid)
{
	if ( aUid == TUid::Null() )
		return;
	
	if ( aUid == KDetailViewId )
		HandleDisplayContactDetailsL();
	else 
		ActivateLocalViewL( aUid );	
}

void CContextContactsAppUi::HandleGainingForeground()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("HandleGainingForeground"));

	if (iNextViewUid!=TUid::Uid(0)) {
		ActivateLocalViewL(iNextViewUid);
		iNextViewUid=TUid::Uid(0);
	}
	if (iLog) {
		iLog->write_time();
		iLog->write_to_output(_L("To foreground"));
		iLog->write_nl();
	}
	if (iPresenceUpdater) iPresenceUpdater->Start();
}

void CContextContactsAppUi::HandleLosingForeground()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("HandleLosingForeground"));

	TRAPD(err, ShowHideNoteIfNecessaryL());
	if (iLog) {
		iLog->write_time();
		iLog->write_to_output(_L("To background\n"));
		iLog->write_nl();
	}
	if (iPresenceUpdater) iPresenceUpdater->Stop();

}


void CContextContactsAppUi::DisplayPresenceDetailsL(const TDesC& Name, 
													const TDesC& aNick,
													const CBBPresence* PresenceData)
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("DisplayPresenceDetailsL"));
#ifndef __JAIKU__		
	if (PresenceData == 0) return;
	iPresenceDetailView->SetData(Name, aNick, PresenceData, GetTime());
	ActivateLocalViewL(KPresenceDetailView);
#endif
}


void CContextContactsAppUi::DisplayPersonalStreamL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("DisplayPersonalStreamL"));	
	ActivateLocalViewL(KAuthorFeedView);

}

void CContextContactsAppUi::DisplayRichPresenceL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("DisplayPresenceDetailsL"));	
	ActivateLocalViewL(KRichPresenceView);
}


void CContextContactsAppUi::DisplayPresenceDescriptionL(const TDesC& Name, 
						    const CBBPresence* PresenceData)
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("DisplayPresenceDescriptionL"));
#ifndef __JAIKU__	
	if (PresenceData == 0) return;
	iPresenceDescriptionView->SetData(Name, PresenceData);
	ActivateLocalViewL(KPresenceDescriptionView);
#endif
}


void CContextContactsAppUi::DisplayContactDetailsL(	TInt contact_id )
{
	if ( iActiveState->ActiveContact().GetId() != contact_id )
		iActiveState->ActiveContact().SetL( contact_id );
	
	ActivateLocalViewL(KDetailViewId);
}


TBool CContextContactsAppUi::ProcessCommandParametersL(TApaCommand /*aCommand*/, 
													   TFileName& aDocumentName,
													   const TDesC8& /*aTail*/)
{
	if (aDocumentName.CompareF(_L("hide"))==0) {
		hide();
	}
	aDocumentName.Zero();
	return EFalse;
}

void CContextContactsAppUi::PostJaikuL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("PostJaikuL"));
	iPosterUi->PostJaikuL(this);
}


void CContextContactsAppUi::ShowAboutDialogL()
{
	iAboutUi->ShowNoteL();
}


CDb* CContextContactsAppUi::JabberDb()
{
	return iJabberDb;
}


void CContextContactsAppUi::HandleResourceChangeL( TInt aType )
{

	CAknViewAppUi::HandleResourceChangeL( aType );
	if ( iViewNavigation ) iViewNavigation->HandleResourceChangeL( aType );

	if ( aType == KAknsMessageSkinChange )
		{
			iThemeColors->Reset();
		}
	else if ( aType == KEikDynamicLayoutVariantSwitch )
		{

			if ( iFeedGraphics )
				iFeedGraphics->ReleaseCachedOrphansL();

			if ( iLayout ) iLayout->UpdateLayoutDataL();
			if ( iContactsView ) iContactsView->HandleResourceChange( aType );
			if ( iRichPresenceView ) iRichPresenceView->HandleResourceChange( aType );
			if (iEverybodysFeedView ) iEverybodysFeedView->HandleResourceChange( aType );
			if (iFeedGroupView) iFeedGroupView->HandleResourceChange( aType );
			if (iAuthorFeedView) iAuthorFeedView->HandleResourceChange( aType );
			if (iCommentsView) iCommentsView->HandleResourceChange( aType );
			if (iSettingsView) iSettingsView->HandleResourceChange( aType );
			if (iSettingsView2) iSettingsView2->HandleResourceChange( aType );
			if (iErrorInfoView) iErrorInfoView->HandleResourceChange( aType );

			if (iBuddyBTView) iBuddyBTView->HandleResourceChange( aType );
			if (iLaptopBTView) iLaptopBTView->HandleResourceChange( aType );
			if (iDesktopBTView) iDesktopBTView->HandleResourceChange( aType );
			if (iPDABTView) iPDABTView->HandleResourceChange( aType );

			CCoeControl* notify=CLocalNotifyWindow::Global();
			if (notify) notify->HandleResourceChange( aType );
		}
}


void CContextContactsAppUi::HandleOpenContactL()
{
	contact* c = ActiveContact().GetL();
	if ( c )
		{
			if ( c->has_nick )
				{
					DisplayPersonalStreamL();
				}
			else
				{
					DisplayContactDetailsL(c->id);
				}
		}
	else
		{
			// no op 
		}
}


void CContextContactsAppUi::HandleDisplayRichPresenceL()
{	
	contact* c = ActiveContact().GetL();
	
	if ( c )
		{
			if ( c->presence )
				{
					DisplayRichPresenceL();
				}
		}
}


void CContextContactsAppUi::HandleDisplayContactDetailsL()
{
	TInt contact_id = ActiveContact().GetId();

	if ( ! DummyContactBehaviorL(EContextContactsCmdDisplayContactDetails) )
		{
			DisplayContactDetailsL(contact_id);
		}
}

void CContextContactsAppUi::SetCurrentContactIdL(TInt aId )
{
	ActiveContact().SetL( aId );
}


TBool CContextContactsAppUi::DummyContactBehaviorL(TInt /*aCommand*/)
{
	TInt id = ActiveContact().GetId();
	TBool doDummyBehavior = EFalse;
	if ( iJabberData->IsDummyContactId( id ) )
		{
			doDummyBehavior = ETrue;
		}
	else 
		{
			auto_ptr<CPbkContactItem> item(NULL);
			CC_TRAPD(err, item.reset( iPhonebook->get_engine()->ReadContactL(id) ) );			
			if ( item.get() )
				{
					doDummyBehavior = EFalse;
				}
			else 
				{			
					// FIXME we could transfer this to dummy item immediately
					doDummyBehavior = ETrue;
				}
		}		 
	
	if ( doDummyBehavior )
		{
			CJabberData::TNick nick;
			iJabberData->GetJabberNickL( id, nick );
			
			TPtrC firstName(KNullDesC);
			TPtrC lastName(KNullDesC);
			
			contact* c = iPhonebook->GetContactById(id);
			
			if ( c && c->presence )
				{
					if ( c->first_name) firstName.Set( *(c->first_name ) );
					if ( c->last_name)  lastName.Set( *(c->last_name ) );
				}			

			auto_ptr<CPhonebookUi> pbui( CPhonebookUi::NewL() );
			TContactItemId newId = pbui->QueryContactForNickL(nick, firstName, lastName);
			if ( newId != KNullContactId )
				{
					iJabberData->SetJabberNickL( newId, nick, CJabberData::ESetByUser );
					iPhonebook->ReRead();
				}
			StatusPaneUtils::SetContextPaneIconToDefaultL();
			return ETrue;
		}
	else
		{
			return EFalse;
		}
}

void CContextContactsAppUi::HandleInfoMemL()
{
	TInt err, size=0, nb_contacts = -1;
	TInt64 free = -1;
	TEntry entry;
	TVolumeInfo info;
	_LIT(KContactsDb, "c:\\System\\Data\\Contacts.cdb");
	
	nb_contacts = iPhonebook->Count();
	err = Fs().Entry(KContactsDb, entry);
	if (err == KErrNone) size = entry.iSize;
	err = Fs().Volume(info);
	if (err == KErrNone) free = info.iFree;
	
	TReal x = size/1024;
	TReal y = I64REAL(free/1024);

	HBufC * header = CEikonEnv::Static()->AllocReadResourceL(R_CONTACTS_INFO_BUF);
	CleanupStack::PushL(header);
	TBuf<200> message;
	TRealFormat realFormat = TRealFormat(30, 0);
	//FIX ME: use resource
	message.AppendFormat(_L("%d contacts\n"), nb_contacts);
	message.AppendNum(x, realFormat);
	message.Append(_L(" kB used\n"));
	message.AppendNum(y, realFormat);
	message.Append(_L(" kB free\n"));
	
	CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(message);
	CleanupStack::PushL(dlg);
	dlg->PrepareLC(R_CONTACTS_INFO);
	dlg->QueryHeading()->SetTextL(*header);
	CleanupStack::Pop(dlg);
	
	dlg->RunLD();
	CleanupStack::PopAndDestroy(1); //header
}

void CContextContactsAppUi::HandleCallL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("HandleCallL"));
	if ( ! DummyContactBehaviorL(EContextContactsCmdCall) )
		{
			TInt contact_id = ActiveContact().GetId();
			CPbkPhoneNumberSelect* sel=new (ELeave) CPbkPhoneNumberSelect();
			auto_ptr<CPbkContactItem> item(iPhonebook->get_engine()->ReadContactL(contact_id));
#ifndef __S60V3__
			TPtrC no(sel->ExecuteLD(*item, NULL, ETrue /*if there's a default number!*/));
#else
			TPtrC no(0, 0);
			CPbkPhoneNumberSelect::TParams p(*item);
			p.SetUseDefaultDirectly( ETrue );	
			TBool selected=sel->ExecuteLD(p);
			if (selected) {
				const TPbkContactItemField* f=p.SelectedField();
				if (f && f->StorageType()==KStorageTypeText) {
					no.Set(f->Text());
				}
			}
#endif
			User::LeaveIfError(iPhoneHelper->make_callL(no));
			//iView->ResetListFocusL();
		}
}


void CContextContactsAppUi::EditNickL(TBool aEditable)
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("EditNickL"));


	contact* c = ActiveContact().GetL();
	auto_ptr<HBufC> name(NULL);
	if ( c )
		{
			name.reset( c->NameL( iPhonebook->ShowLastNameFirstL() ) );	
			CNickForm* f=CNickForm::NewL(*iJabberData, c->id, *name, iPhonebook, iPresenceHolder,
										 aEditable);
			f->ExecuteLD();
		}
}



void CContextContactsAppUi::SendInvitationSMSL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("SendInvitationSMSL"));

	contact* c = ActiveContact().GetL();
	if ( !c )
		return;
	
	CPbkPhoneNumberSelect* sel=new (ELeave) CPbkPhoneNumberSelect();
	TInt contact_id = c->id;

	CPbkContactItem * item = iPhonebook->get_engine()->ReadContactL(contact_id);
	// FIXME: why we use cleanup stack here?
	CleanupStack::PushL(item);
#ifndef __S60V3__
	TPtrC no(sel->ExecuteLD(*item, NULL, ETrue /*if there's a default number!*/));
#else
	TPtrC no(0, 0);
	CPbkPhoneNumberSelect::TParams p(*item);
	TBool selected=sel->ExecuteLD(p);
	if (selected) {
		const TPbkContactItemField* f=p.SelectedField();
		if (f && f->StorageType()==KStorageTypeText) {
			no.Set(f->Text());
		}
	}
#endif
	if (no.Length()>0) {
			bb_auto_ptr<TBBSentInvite> invitep(new (ELeave) TBBSentInvite);
			TBBSentInvite& invite=*invitep;
			auto_ptr<CDesCArrayFlat> recip(new CDesCArrayFlat(1));
			auto_ptr<CDesCArrayFlat> alias(new CDesCArrayFlat(1));
			auto_ptr<HBufC> title(item->GetContactTitleL());
			
			recip->AppendL(no);

			alias->AppendL(*title);

			// Form of invite is following:
			// http://jaiku.com/invite/mobile/mikie/45094123219b928d0c2e

			TBuf<150> url=_L("http://jaiku.com/invite/mobile/");

			auto_ptr<HBufC8> url8(HBufC8::NewL( 110 ));
			auto_ptr<CCnvCharacterSetConverter> utf8CC(CCnvCharacterSetConverter::NewL());
			utf8CC->PrepareToConvertToOrFromL(KCharacterSetIdentifierUtf8, Fs());

			{
				// Get my nick and drop @jaiku.com
				TBuf< CJabberData::KNickMaxLen > mynick = iJabberData->UserNickL();
				CJabberData::CanonizeNickL( mynick );
				url.Append(mynick);
				invite.iFrom=mynick;
			}
			CContactMatcher::HashPhoneNumberL(no, invite.iToNumberHash());
			url.Append(_L("/"));

			auto_ptr<SHA1> sha1(new SHA1);

			TTime now; now=GetTime();
			invite.iStamp=now;
			TDateTime epoch; epoch.Set(1970, EJanuary, 0, 0, 0, 0, 0);
			TTime e(epoch);
			TInt unixtime;
			TTimeIntervalSeconds secs;
			User::LeaveIfError(now.SecondsFrom(e, secs));
			unixtime=secs.Int();
			url.AppendNumFixedWidth(unixtime, EHex, 8);
			{
				TBuf<8> ts; ts.AppendNumFixedWidth(unixtime, EHex, 8);
				url8->Des().Zero();
				TPtr8 into=url8->Des();
				utf8CC->ConvertFromUnicode(into, ts);
				sha1->Input((char*)(into.Ptr()), into.Size());
			}
			{
				TBuf<50> pass;
				if (! Settings().GetSettingL(SETTING_JABBER_PASS_SHA1, pass) || pass.Length()==0) {
					Settings().GetSettingL(SETTING_JABBER_PASS, pass);
					if (pass.Length()==0) User::Leave(KErrNotFound); // FIXME: error message
					DoSHA1(pass, pass);
				}
				url8->Des().Zero();
				TPtr8 into=url8->Des();
				utf8CC->ConvertFromUnicode(into, pass);
				sha1->Input((char*)(into.Ptr()), into.Size());
			}
			unsigned int message_digest_array[5];			
			sha1->Result(message_digest_array);
			// we only take first 8 bytes (64 bits), instead of 20 (160 bits)
			for (int i=0;i<2;i++)
			{
				url.AppendNumFixedWidth(message_digest_array[i], EHex, 8);
			}
#ifdef __WINS__
			RDebug::Print(url);
#endif
			invite.iUrl=url;
			TTime expires=GetTime(); expires+=TTimeIntervalDays(7);
			BBSession()->PutRequestL( KSentInviteTuple, KNullDesC, invitep.get(), expires, KOutgoingTuples );

			iPhoneHelper->send_sms(recip.get(), alias.get(), url);
		}
	CleanupStack::PopAndDestroy(item);
}



void CContextContactsAppUi::ShowLastJaikuViewErrorL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ShowLastJaikuViewErrorL"));
	CJaikuViewBase* jaikuView = NULL;
	jaikuView = static_cast<CJaikuViewBase*> ( iView );

	if ( jaikuView )
		{
			MErrorInfo* error = jaikuView->LastErrorInfo();
			if (error) 
				{
					iErrorInfoView->ShowWithData( *error );
				}
		}
}

void CContextContactsAppUi::ShowConnectionErrorL()
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("ShowConnectionErrorL"));
	auto_ptr<CBBErrorInfo> errorInfo( ConnectionInfo::GetErrorL( *(BBSession())) );
	if ( errorInfo.get() )
		{
			iErrorInfoView->ShowWithData( *errorInfo );
		}
	else
		{
			User::Leave( KErrNotFound );
		}
}

CActiveContact& CContextContactsAppUi::ActiveContact() const
{
	return iActiveState->ActiveContact();
}


void CContextContactsAppUi::PruneFeedItemsL() 
{
	CALLSTACKITEM_N(_CL("CContextContactsAppUi"), _CL("PruneFeedItemsL"));
	TTime limit = GetTime();
	limit -= TTimeIntervalDays(5);
	TInt personalLimit = 10;
	iFeedEngine->Storage().RemoveFeedItemsL(limit, CFeedItemStorage::EByDateTime, personalLimit);
}
