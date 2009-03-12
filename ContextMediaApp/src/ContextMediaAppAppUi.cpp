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

#include "break.h"
#include "ContextMediaAppAppUi.h"
#include "view_ids.h"
#include "symbian_auto_ptr.h"
#include "contextmediaui.hrh"
#include "cm_post.h"
#include <contextmediaapp.rsg>
#include "cl_settings.h"
#include "local_defaults.h"
#include <flogger.h>
#include "app_context_impl.h"
#include "reporting.h"
#include <bautils.h>
#include "contextnotifyclientsession.h"
#include <contextmediaui.rsg>

const TUint KCameraKey = 0xF887;

CContextMediaAppAppUi::CContextMediaAppAppUi(MApp_context& Context, 
		CPostStorage &aThreadStorage, CBBDataFactory * aFactory, 
		CCMNetwork &aCMNetwork) : 
	MContextBase(Context), iThreadStorage(aThreadStorage), iCMNetwork(aCMNetwork), iFactory(aFactory) { }

void CContextMediaAppAppUi::ConstructL()
{
	CC_TRAPD(err, InnerConstructL());
	if (err!=KErrNone) {
		AppContext().ReportActiveError(_L(""),
			_L("UI Construct"), err);
	}
	User::LeaveIfError(err);
}

void CContextMediaAppAppUi::InnerConstructL()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("InnerConstructL"));

	{
		BaseConstructL(0x1008);
	}

	MContextAppUi::ConstructL(AppContext(), _L("medialog"));

	CLocalNotifyWindow::CreateAndActivateL();


	iCmuiResourceFile = LoadCmuiResourceL();

	iBusyNotifier = new (ELeave) CPopupNotifier;
	iBusyNotifier->ConstructL(TRect(TPoint(0,0), TSize(10,10)));

	Reporting().AddBusyIndicator(this);

	iCallBacks=CList<TCallBackItem>::NewL();
	iOldPrompt = new (ELeave) TCMOldPrompt(iThreadStorage, iCMNetwork, CPostStorage::MediaPoolId());

	{
		iPhoneHelper = new (ELeave) phonehelper(0);
		iPhoneHelper->ConstructL();
	}

	{
		iTransferDir=CTransferDir::NewL(AppContext(), *this, _L("MEDIA_TRANSFER"));
	}

// TAB HANDLING
	{
		CEikStatusPane* sp = StatusPane();
		iNaviPane = (CAknNavigationControlContainer*)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
		iDecoratedTabGroup = iNaviPane->ResourceDecorator();
		if (iDecoratedTabGroup) {
			iTabGroup = (CAknTabGroup*) iDecoratedTabGroup->DecoratedControl();
		}
	}

// CREATING VIEWS
	{
		{
			iPostView = new (ELeave) CContextMediaAppThreadView(*this, KPostViewId, KNullUid,
						iThreadStorage, CPostStorage::EDescending, CPostStorage::EByDate,
						CPostStorage::RootId(), ETrue, ETrue, *iPhoneHelper, iCMNetwork, 0);

			iPostView->ConstructL();
			AddViewL( iPostView );		// transfer ownership to CAknViewAppUi
		}


		{
			iThreadView = new (ELeave) CContextMediaAppThreadView(*this, KThreadViewId, KPostViewId,
						iThreadStorage, CPostStorage::EDescending, CPostStorage::EByDate,
						CPostStorage::RootId(), ETrue, EFalse, *iPhoneHelper, iCMNetwork, 0);

			iThreadView->ConstructL();
			AddViewL( iThreadView );      // transfer ownership to CAknViewAppUi

		}

		{
			iThreadsByLastPostView = new (ELeave) CContextMediaAppThreadView(*this, KThreadsByLastPostViewId, KThreadViewId,
						iThreadStorage, CPostStorage::EDescending, CPostStorage::EByLastPost,
						CPostStorage::RootId(), EFalse, EFalse, *iPhoneHelper, iCMNetwork, 0);
			iThreadsByLastPostView->ConstructL();
			AddViewL( iThreadsByLastPostView );      // transfer ownership to CAknViewAppUi

			iThreadsByAuthorView = new (ELeave) CContextMediaAppThreadView(*this, KThreadsByAuthorViewId, KThreadViewId,
						iThreadStorage, CPostStorage::EAscending, CPostStorage::EByAuthor,
						CPostStorage::RootId(), EFalse, EFalse, *iPhoneHelper, iCMNetwork, 0);
			iThreadsByAuthorView->ConstructL();
			AddViewL( iThreadsByAuthorView );      // transfer ownership to CAknViewAppUi

			iThreadsByTitleView = new (ELeave) CContextMediaAppThreadView(*this, KThreadsByTitleViewId, KThreadViewId,
						iThreadStorage, CPostStorage::EAscending, CPostStorage::EByTitle,
						CPostStorage::RootId(), EFalse, EFalse, *iPhoneHelper, iCMNetwork, 0);
			iThreadsByTitleView->ConstructL();
			AddViewL( iThreadsByTitleView );      // transfer ownership to CAknViewAppUi
		}

		{
			iVisualCodeView = CRecognizerView::NewL(AppContext(),
				this, KVisualCodeViewId, 
				KThreadsByLastPostViewId, 0, R_VISUALCODE_VIEW);
			AddViewL(iVisualCodeView);
		}

		{
			iEditorView = new (ELeave) CContextMediaEditorView(KEditorViewId, &iNextView, AppContext(), iCMNetwork, iThreadStorage, iFactory, iTransferDir->Transferer());
			iEditorView->ConstructL();
			AddViewL( iEditorView );      // transfer ownership to CAknViewAppUi
		}

		{
			auto_ptr<CSettingsView> iSettingsView(CSettingsView::NewL(KCMSettingsViewId, AppContext(), TLocalAppSettings::GetEnabledSettingsArray()));
			AddViewL(iSettingsView.get());
			iSettingsView.release();
		}
			
		{
			SetDefaultViewL(*iVisualCodeView);
		}
	}

	{
		iVisualPrompt = CVisualCodePrompt::NewL(AppContext(),
			this, KVisualCodePromptId, 
			KThreadsByLastPostViewId, 0 /*&(iThreadsByLastPostView->iNextView)*/);

		iThreadsByDatePrompt = new (ELeave) CContextMediaAppThreadView(*this, 
					KThreadsByDatePromptId, KThreadViewId,
					iThreadStorage, CPostStorage::EDescending, CPostStorage::EByDate,
					CPostStorage::RootId(), EFalse, EFalse, *iPhoneHelper, iCMNetwork, iVisualPrompt);
		iThreadsByDatePrompt->ConstructL();
		AddViewL( iThreadsByDatePrompt );      // transfer ownership to CAknViewAppUi
	}

	{
		iMediaPublishers=new (ELeave) CArrayPtrFlat<CPicturePublisher>(10);

		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\nokia\\images"),
			_L("*jpg"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir));
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\images"),
			_L("*jpg"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir));

		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\nokia\\videos"),
			_L("*3gp"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir));
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\videos"),
			_L("*3gp"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir));

		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\nokia\\Sounds\\digital"),
			_L("*amr"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir, _L("*aac")));
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\Sounds\\digital"),
			_L("*amr"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir, _L("*aac")));

		// n-gage
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\Record"),
			_L("*amr"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir, _L("*aac")));
		
		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\Sounds\\digital"),
			_L("*amr"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir, _L("*aac")));	

		AddMediaPublisher(CPicturePublisher::NewL(AppContext(), *this, _L("?:\\system\\apps\\ContextNote"),
			_L("*txt"), SETTING_MEDIA_UPLOAD_ENABLE, MEDIA_PUBLISHER_CM, SETTING_VISUALCODES_URLBASE, SETTING_VISUALCODES_SCRIPT, *this, *iOldPrompt, iTransferDir));
	}
	PublishOldMedia();

	iLastViewBeforePrompt = KThreadsByLastPostViewId;
}

void CContextMediaAppAppUi::PublishOldMedia()
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

CContextMediaAppAppUi::~CContextMediaAppAppUi()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("~CContextMediaAppAppUi"));

	CLocalNotifyWindow::Destroy();

	Reporting().RemoveBusyIndicator(this);
	delete iBusyNotifier;
	delete iPacket;
	delete iDecoratedTabGroup;
	delete iPhoneHelper;
	if (iMediaPublishers) iMediaPublishers->ResetAndDestroy();
	delete iMediaPublishers;

	delete iOldPrompt;
	delete iVisualPrompt;
	delete iTransferDir;

	delete iCallBacks;

	iEikonEnv->DeleteResourceFile(iCmuiResourceFile);
}

TKeyResponse CContextMediaAppAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("HandleKeyEventL"));

	if (aKeyEvent.iCode == KCameraKey ) {
		return EKeyWasConsumed;
	}

	if ( iTabGroup == NULL || iView == NULL) {
		return EKeyWasNotConsumed;
	}
	if ( (iView->Id() == KPostViewId) || (iView->Id() == KThreadViewId) ) {
		return EKeyWasNotConsumed;
	}
        	
	TInt active = iTabGroup->ActiveTabIndex();
	TInt count = iTabGroup->TabCount();

	switch ( aKeyEvent.iCode ) {
		case EKeyLeftArrow:
			if ( active > 0 ) {
				active--;
				iTabGroup->SetActiveTabByIndex( active );
				TInt i = iTabGroup->TabIdFromIndex(active);
				ActivateLocalViewL(TUid::Uid(i));
			}
			break;
		case EKeyRightArrow:
			if( (active + 1) < count ) {
				active++;
				iTabGroup->SetActiveTabByIndex( active );
				TInt i = iTabGroup->TabIdFromIndex(active);
				ActivateLocalViewL(TUid::Uid(i));
			}
			break;
		default:
			return EKeyWasNotConsumed;
			break;
	}
	return EKeyWasConsumed;
}

void CContextMediaAppAppUi::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("HandleCommandL"));

	switch ( aCommand ) {
		case EAknSoftkeyBack:
			hide();
			break;
		case EAknSoftkeyExit:
		case EAknCmdExit:
		case EEikCmdExit:{
			Exit();
			break;
		}
		case EcontextmediaappCmdSettings:
			ActivateLocalViewL(KCMSettingsViewId);
			break;
		default:
			break;      
	}
}

void CContextMediaAppAppUi::SetTab(TInt tabId)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("SetTab"));


	TInt idx = iTabGroup->TabIndexFromId(tabId);

	if (idx != KErrNotFound) iTabGroup->SetActiveTabByIndex(idx);

}

void CContextMediaAppAppUi::CodeSelected(const CCodeInfo& aCode)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("CodeSelected"));

	TInt64 id = aCode.code->ToInt64();
	CodeSelected(id);
}

void CContextMediaAppAppUi::CodeSelected(TInt64 id)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("CodeSelected"));

	CCMPost * aPost = 0;
	TBool activate_by_last_view=EFalse;

	CC_TRAPD(err, aPost=iThreadStorage.GetByPostIdL(0, id));
        if (err!=KErrNotFound) User::LeaveIfError(err);

        if (!aPost ) {
		// we haven't seen this code before
		TBool ok = iCMNetwork.FetchThread(id);
		/*if (err==KErrCouldNotConnect) {
			_LIT(KErrorText, "Incorrect IAP, Check connection settings.");
			CAknErrorNote* note = new(ELeave) CAknErrorNote;
			note->ExecuteLD(KErrorText());
		} else {
			activate_by_last_view=ETrue;
		}*/
		if (ok) activate_by_last_view=ETrue;
	} else {
		// we have seen this code before, refresh
		TBool ok=iCMNetwork.FetchThread(id);
		if (!ok) return;
		iThreadStorage.SetThreadVisibleL(id, ETrue);
		TBool is_placeholder=EFalse;
		CC_TRAPD(err, is_placeholder=iThreadStorage.IsPlaceHolderL(aPost->iPostId()));
		if (is_placeholder) {
			// display all threads
			activate_by_last_view=ETrue;
		} else {
			//display the target thread
			iThreadView->SetNodeId(id);
			iThreadView->SetCurrentIndex(0);
			iThreadView->SetTopIndex(0);
			ActivateLocalViewL(KThreadViewId);
		}
		iThreadStorage.Release(aPost, 0);
	}

	if (!activate_by_last_view) return;

	// set current index in view to the item representing thread id
	TInt i=-1;
	TBool ok = iThreadStorage.FirstL(CPostStorage::RootId(), CPostStorage::EByLastPost, CPostStorage::EDescending, EFalse);
	while (ok) {
		i++;
		if (iThreadStorage.GetCurrentIdL() == id) {
			ok=EFalse;
		} else {
			ok=iThreadStorage.NextL();
		}
	}
        
	TInt top = i-3;
	if (top<0) top=0;
	iThreadsByLastPostView->SetTopIndex(top);
	iThreadsByLastPostView->SetCurrentIndex(i);
	ActivateLocalViewL(KThreadsByLastPostViewId);
}

void CContextMediaAppAppUi::Cancelled()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("Cancelled"));

	// from visual code, no implementation 
}

void CContextMediaAppAppUi::AddMediaPublisher(CPicturePublisher* aPublisher)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("AddMediaPublisher"));

	auto_ptr<CPicturePublisher> p(aPublisher);
	iMediaPublishers->AppendL(aPublisher);
	p.release();
}

void CContextMediaAppAppUi::success(CBase * /*source*/)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("success"));
}

void CContextMediaAppAppUi::error(CBase * /*source*/,TInt /*code*/,const TDesC& /*aReason*/)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("error"));
}

void CContextMediaAppAppUi::info(CBase * /*source*/, const TDesC &msg)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("info"));

	if (msg.Compare(_L("all files transfered"))==0) {
		iCMNetwork.FetchThreads();
	}
}

void CContextMediaAppAppUi::Prompt(const TDesC& FileName, MUploadCallBack* CallBack)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("Prompt"));

	TBuf<200> msg=_L("AppUi:Prompt ");
	msg.Append(FileName.Left(150));
	Reporting().DebugLog(msg);

	if (!CallBack) User::Leave(KErrArgument);
	
	if (!iCallBack) {
		iNextView=TVwsViewId();
		iFileName=FileName;
		iCallBack=CallBack;
		iCurrentPrompt = KEditorViewId;

		if (iFileName.Right(3).CompareF(_L("amr"))==0) {
			RTimer    timer;
			TInt err = timer.CreateLocal();
			if (err == KErrNone) {
				TRequestStatus    status;
				timer.After(status, 8 * 100 * 1000);    // 0.8 sec
				User::WaitForRequest(status);
				timer.Close();
			}
		}
		iEditorView->Prompt(iFileName, this);
	} else {
		iCallBacks->AppendL(TCallBackItem(FileName, CallBack));
	}
}

TFileOpStatus CContextMediaAppAppUi::Back(bool Upload, bool DeleteFromPhone, MBBData* Packet)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppAppUi"), _CL("Back"));
	Reporting().DebugLog(_L("AppUi:Back"));

	TFileOpStatus ret;
	iNext=false;
	TBool done = EFalse;

	if (Upload || DeleteFromPhone) {
		_LIT(KPacketPanic, "Can't upload/delete with empty packet!");
		__ASSERT_ALWAYS(Packet!=0, User::Panic(KPacketPanic,1));
	}

	if (iCurrentPrompt == KEditorViewId) {
		delete iPacket; iPacket=0;
		//save param for later 
		iUpload=Upload;
		iDelete=DeleteFromPhone;
		iPacket=bb_cast<CCMPost>(Packet->CloneL(KNullDesC));
		
		if (iUpload) {
			// we want to upload!
			if (iPacket->iParentId() == CPostStorage::MediaPoolId()) {
				/* parent not already known -> need vc prompt*/
				iCurrentPrompt=KThreadsByDatePromptId;
				iThreadsByDatePrompt->Prompt(KNullDesC, this);
				return ret;
			} 
		} 
		
		if (1 || iDelete) { 
			// Remove traces from strorage 
			iThreadStorage.RemovePostL(iPacket);
		}
		// give the current packet to the actual transferer
		ret = iCallBack->Back(iUpload, iDelete, iPacket);
		done = ETrue;

	} else if (iCurrentPrompt == KThreadsByDatePromptId) {
		if (Upload) {
			// a code was chosen with vc
			CCMPost * aPost = bb_cast<CCMPost>(Packet);
			if (iPacket) iPacket->iParentId()=aPost->iParentId();
			
			//trigger a fetch of the thread, in case there were replies added 
			//to the thread before the one we are about to upload
			iCMNetwork.FetchThread(aPost->iParentId());

			// handle deletion
			//if (1 || iDelete) iThreadStorage.MarkAsRead(iPacket);
			iThreadStorage.RemovePostL(iPacket);

			// forward the callback to actual tranferer
			ret = iCallBack->Back(iUpload, iDelete, iPacket);
		} else {
			// upload was cancelled on choice of code
			ret = iCallBack->Back(Upload, DeleteFromPhone, iPacket);
		}
		done = ETrue;
	} else {
		//Should NEVER arrive here
		_LIT(KViewPanic, "Wrong view id");
		User::Panic(KViewPanic,1);
	}

	// update iMediaUrl for current iPacket and also corresponding the post in Storage
	if (done && (ret.err == KErrNone) ) {
		if (iPacket) {
			// there was a packet
			iPacket->iMediaUrl() = ret.fn;
			iPacket->iMediaFileName() = ret.fn;
			CC_TRAPD(ignore, iThreadStorage.UpdatePostL(iPacket));
		}
	}

	if (done && Upload) {
		// information note
		Reporting().ShowGlobalNote(3, _L("Queued for Publishing"));
	}

	Reporting().DebugLog(_L("Back:reset"));
	iCallBack=0;
	iCurrentPrompt=KNullUid;

	if (iCallBacks->iCount > 0) {
		Reporting().DebugLog(_L("Back:more"));
		TCallBackItem i=iCallBacks->Pop();
		Prompt(i.iFileName, i.iCallBack);
		return ret;
	}
	Reporting().DebugLog(_L("Back:activatethreadview"));

	// Activate previous view
	ActivateLocalViewL(iLastViewBeforePrompt);
	return ret;
}

void CContextMediaAppAppUi::ShowBusy(TBool aIsBusy)
{
	RWsSession& wsSession=CEikonEnv::Static()->WsSession();
	TInt id = CEikonEnv::Static()->RootWin().Identifier();
	TApaTask task(wsSession);
	task.SetWgId(id);
	if ( wsSession.GetFocusWindowGroup() != id ) return;

	CEikStatusPane* sp=CEikonEnv::Static()->AppUiFactory()->StatusPane();
	if (sp->IsVisible()) {
		// non-full screen view
		iBusyNotifier->SetPosition(TPoint(38,7));
	} else {
		// full screen view
		iBusyNotifier->SetPosition(TPoint(165,1));
	}
	iBusyNotifier->MakeVisible(aIsBusy);
	iBusyNotifier->DrawNow();
}

TUid CContextMediaAppAppUi::LastViewBeforePrompt()
{
	return iLastViewBeforePrompt;
}

TVwsViewId CContextMediaAppAppUi::NextView()
{
	return iNextView;
}

void CContextMediaAppAppUi::SetNextView(TVwsViewId aViewId)
{
	iNextView=aViewId;
}
