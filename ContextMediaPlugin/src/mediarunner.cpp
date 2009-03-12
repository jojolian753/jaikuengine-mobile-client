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

#include "mediarunner.h"
#ifdef FLICKR
#  include "cm_oldprompt.h"
#endif
#include "transferdir2.h"
#include "viewids.h"
#include "picture_publisher.h"
#include "cm_storage.h"
#include "mediaprompt.h"
#include "symbian_auto_ptr.h"
#include "app_context.h"
#include "app_context_impl.h"
#include "cl_settings.h"
#include "cm_oldprompt.h"
#include "break.h"
#include "settingsview.h"
#include "context_log.hrh"

class CMediaRunnerImpl : public CMediaRunner, public MContextBase {
	void AddMediaPublisher(CPicturePublisher* aPublisher);
	CArrayPtrFlat<class CPicturePublisher> *iMediaPublishers;
	class CTransferDir		*iTransferDir;
	class MUploadPrompt	*iOldPrompt;
	class CPostStorage	*iMediaStorage;
	class CMediaPrompt	*iMediaPrompt;
    class CSettingsView *iSettingsView1, *iSettingsView2, *iSettingsView3;
	CDb			*iMediaDb;
	TBuf<40> state;

	void ConstructL(CAknViewAppUi *aAppUi, class MPresenceMaintainer* aPublisher,
				  TVwsViewId* NextViewId, i_status_notif& notif,
				  MSocketObserver& aSocketObserver);
	~CMediaRunnerImpl();
	void StepDone() { }

	virtual void PublishOldMedia();
	virtual TUid MediaPoolViewId() { return iMediaPrompt->MediaPoolViewId(); }
	virtual void ShowMediaPoolL() {
		iMediaPrompt->ShowMediaPoolL();
	}
	virtual void RereadUnreadL() {
		iMediaStorage->RereadUnreadL();
	}
	virtual class CAknView* MediaView() {
		return iMediaPrompt->MediaView();
	}
	virtual TBool HandleCommandL(TInt aCommand);
	virtual void CreateSettingsViewsL();
	friend class CMediaRunner;
	friend class auto_ptr<CMediaRunnerImpl>;

	static const TInt KNetworkSettings[];
	static const TInt KAdvancedSettings[];
	static const TInt KBasicSettings[];

	CAknViewAppUi *iAppUi;
};

EXPORT_C CMediaRunner* CMediaRunner::NewL(CAknViewAppUi *aAppUi, class MPresenceMaintainer* aPublisher,
					TVwsViewId* NextViewId,
					i_status_notif& notif, MSocketObserver& aSocketObserver)
{
	auto_ptr<CMediaRunnerImpl> ret(new (ELeave) CMediaRunnerImpl);
	CC_TRAPD(err, ret->ConstructL(aAppUi, aPublisher, NextViewId, notif, aSocketObserver));
	if (err==KErrNone) {
		return ret.release();
	} else {
		return 0;
	}
}

void CMediaRunnerImpl::ConstructL(CAknViewAppUi *aAppUi, class MPresenceMaintainer* aPublisher,
				  TVwsViewId* NextViewId, i_status_notif& notif,
				  MSocketObserver& aSocketObserver)
{
	iAppUi=aAppUi;
	iMediaPublishers=new (ELeave) CArrayPtrFlat<CPicturePublisher>(10);

	iTransferDir=CTransferDir::NewL(AppContext(), aSocketObserver, _L("TRANSFER"));
	StepDone();

	iMediaDb=CDb::NewL(AppContext(), _L("CL_MEDIA"), EFileRead|EFileWrite);
	iMediaStorage=CPostStorage::NewL(AppContext(), *iMediaDb, BBDataFactory());
#ifndef CONTEXTMEDIA_AS_PLUGIN
	AppContext().TakeOwnershipL(iMediaDb);
	AppContext().TakeOwnershipL(iMediaStorage);
#endif
	iMediaPrompt=CMediaPrompt::NewL(aAppUi, aPublisher, NextViewId, AppContext(),
		*iMediaStorage, iTransferDir->Transferer());
	MUploadPrompt *pr=iMediaPrompt;

	iOldPrompt = new (ELeave) TCMOldPrompt(*iMediaStorage, *iMediaPrompt->GetDummyNetwork(),
		TInt64(CPostStorage::ENotPublished) );

	TBool do_other_media=ETrue;
	{
		auto_ptr<HBufC> url(HBufC::NewL(256));
		{
			TPtr16 p=url->Des();
			Settings().GetSettingL(SETTING_PUBLISH_URLBASE, p);
		}
		if ( (*url).FindF(_L("flickr"))!=KErrNotFound) do_other_media=EFalse;
	}
	state=_L("create picture publisher");
#ifndef __S60V3__
	AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\nokia\\images"),
		_L("*jpg"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir));
	AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\images"),
		_L("*jpg"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir));

	if (do_other_media) {
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\nokia\\videos"),
			_L("*3gp"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir));
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\videos"),
			_L("*3gp"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir));

		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\nokia\\Sounds\\digital"),
			_L("*amr"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir, _L("*aac")));
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\Sounds\\digital"),
			_L("*amr"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir, _L("*aac")));

		// n-gage
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\Record"),
			_L("*amr"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir, _L("*aac")));

		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\Sounds\\digital"),
			_L("*amr"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir, _L("*aac")));	

		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\system\\apps\\ContextNote"),
			_L("*txt"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir));
	}
#else
	AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\Data\\Images"),
		_L("*jpg"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir));

	AddMediaPublisher(CPicturePublisher::NewL(AppContext(), notif, _L("?:\\Images"),
		_L("*jpg"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CL, SETTING_PUBLISH_URLBASE, SETTING_PUBLISH_SCRIPT, *pr, *iOldPrompt, iTransferDir));

#endif //3rd

	StepDone();

#ifdef FLICKR
	if (iMediaPublishers) {
		for (int i=0; i< iMediaPublishers->Count(); i++) {
			iMediaPublishers->At(i)->PublishOld();
		}
	}
#endif
	StepDone();
}

void CMediaRunnerImpl::CreateSettingsViewsL()
{
	{
		auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KBasicSettingsViewId, AppContext(), 
			KBasicSettings, ETrue));
		iAppUi->AddViewL(iSettingsView.get());
		iSettingsView->SetPreviousLocalViewId(MediaPoolViewId());
		iSettingsView1=iSettingsView.release();
	}
	{
		auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KAdvancedSettingsViewId, AppContext(), 
			KAdvancedSettings, ETrue));
		iAppUi->AddViewL(iSettingsView.get());
		iSettingsView->SetPreviousLocalViewId(MediaPoolViewId());
		iSettingsView2=iSettingsView.release();
	}
	{
		auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KNetworkSettingsViewId, AppContext(), 
			KNetworkSettings, ETrue));
		iAppUi->AddViewL(iSettingsView.get());
		iSettingsView->SetPreviousLocalViewId(MediaPoolViewId());
		iSettingsView3=iSettingsView.release();
	}
}

TBool CMediaRunnerImpl::HandleCommandL(TInt aCommand)
{
	switch ( aCommand )
	{
	case Econtext_logCmdBasicSettings:
		iAppUi->ActivateLocalViewL(KBasicSettingsViewId);
		break;
	case Econtext_logCmdNetworkSettings:
		iAppUi->ActivateLocalViewL(KNetworkSettingsViewId);
		break;
	case Econtext_logCmdAdvancedSettings:
		iAppUi->ActivateLocalViewL(KAdvancedSettingsViewId);
		break;
    default:
        return EFalse;
    }

    return ETrue;
}

void CMediaRunnerImpl::AddMediaPublisher(CPicturePublisher* aPublisher)
{
	auto_ptr<CPicturePublisher> p(aPublisher);
	iMediaPublishers->AppendL(aPublisher);
	p.release();
	aPublisher->SubscribeL(KBaseTuple);
}

void CMediaRunnerImpl::PublishOldMedia()
{
	if (iTransferDir && iTransferDir->Transferer()) {
		iTransferDir->Transferer()->Trigger();
	}
	if (iMediaPublishers) {
		for (int i=0; i< iMediaPublishers->Count(); i++) {
			iMediaPublishers->At(i)->PublishOld();
		}
	}
}

CMediaRunnerImpl::~CMediaRunnerImpl()
{
	if (iMediaPublishers) {
		iMediaPublishers->ResetAndDestroy();
	}
	delete iMediaPublishers;

	delete iMediaPrompt;
#ifndef CONTEXTMEDIA_AS_PLUGIN
	/*
	iMediaStorage, and iMediaDb are given to appcontext and deleted by it
	*/
#else
	delete iMediaStorage;
	delete iMediaDb;
#endif

	delete iTransferDir;
	delete iOldPrompt;
}

const TInt CMediaRunnerImpl::KNetworkSettings[] = {
	SETTING_IP_AP,	 
	SETTING_PROXY,
	SETTING_PROXY_PORT,

	-1
};

const TInt CMediaRunnerImpl::KAdvancedSettings[] = {
	SETTING_PUBLISH_AUTHOR,
	SETTING_LOCATIONSERVICE_ENABLE,
	SETTING_PUBLISH_URLBASE,
	SETTING_PUBLISH_SCRIPT,
	SETTING_PUBLISH_USERNAME,
	SETTING_PUBLISH_PASSWORD,
	SETTING_PROJECT_NAME,
	SETTING_SHOW_AUTOTAGS,
	SETTING_SHOW_TAGS,
	SETTING_FIXED_PRIVACY,
	-1
};

const TInt CMediaRunnerImpl::KBasicSettings[] = {
	SETTING_MEDIA_UPLOAD_ENABLE,
	SETTING_DELETE_UPLOADED,
	SETTING_BT_SCAN_INTERVAL,
	SETTING_PROMPT_FOR_NAMES,
	SETTING_VIBRATE_ONLY,
	-1
};

