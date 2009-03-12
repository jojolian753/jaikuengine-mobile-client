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
#include  <aknviewappui.h>
#include  <avkon.hrh>
#include  <ContextMediaui.rsg>
#include  "ContextMediaAppView2.h"
#include  "view_ids.h"
#include  "contextmediaui.hrh"
#include  <eikdialg.h>
#include  "cl_settings.h"
#include  <flogger.h>
#include  "reporting.h"
#include  <eikmenup.h>
#include  <aknmessagequerydialog.h> 
#include  <sendui.h>
#include  "cm_sendui.h"
#ifndef __S60V3__
#include  <SENDNORM.RSG>
#endif
#include "cm_tags.h"
#include "app_context_impl.h"
#include "cu_errorui.h"
#include "bberrorinfo.h"
#include "raii_apgcli.h"
#include "cbbsession.h"

EXPORT_C void CContextMediaEditorView::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("ConstructL"));

	iOrder=CPostStorage::EDescending;
	iSort=CPostStorage::EByDate;
	iNode=CPostStorage::MediaPoolId();
#ifndef __S60V3__
	iSendUi = CSendAppUi::NewL(EcontextmediaappCmdSendAppUi, 0);
#else
	iSendUi = CSendUi::NewL();
#endif
	iTimer=CTimeOut::NewL(*this);

	if (iStandAlone) {
		iTagDb=CDb::NewL(AppContext(), _L("TAGS"), EFileRead|EFileWrite|EFileShareAny);
		iTagStorage=CTagStorage::NewL(*iTagDb);
	}
	if (!iStandAlone) {
		BaseConstructL( R_CONTEXTMEDIAAPP_EDITORVIEW );
	} else {
		BaseConstructL( R_CONTEXTMEDIAAPP_EDITORVIEW_STANDALONE );
	}
	ResetThreadTarget();
	{
		auto_ptr<CBBSubSession> bbs(BBSession()->CreateSubSessionL(0));
		const TTupleName KMediaContainerHintTuple = { { CONTEXT_UID_CONTEXTMEDIAUI }, 1 };
		TRAPD(err, bbs->DeleteL(KMediaContainerHintTuple, KNullDesC));
	}
}

EXPORT_C CContextMediaEditorView::CContextMediaEditorView(TUid aViewId, TVwsViewId* NextViewId, MApp_context& Context, CCMNetwork &aCMNetwork, 
	CPostStorage &aStorage, MBBDataFactory * aFactory, CHttpTransfer2* aTransferer, TBool aStandAlone,
	class CAknIconArray * aTagIcons, MAskForNames* aAskForNames) 
	: MContextBase(Context), iViewId(aViewId), iCMNetwork(aCMNetwork), 
	  iStorage(aStorage), iFactory(aFactory), iNextViewId(NextViewId), iTransferer(aTransferer),
	iStandAlone(aStandAlone), iTagIcons(aTagIcons), iAskForNames(aAskForNames) {}

CContextMediaEditorView::~CContextMediaEditorView() {
	CC_TRAPD(err, ReleaseCContextMediaEditorView());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}
void CContextMediaEditorView::ReleaseCContextMediaEditorView()
{
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("ReleaseCContextMediaEditorView"));

	if (iContainer) AppUi()->RemoveFromViewStack( *this, iContainer );
	delete iTimer;
	delete iContainer;
	delete iSendUi;
	delete iTagStorage;
	delete iTagDb;
}

TUid CContextMediaEditorView::Id() const {
	return iViewId;
}

void CContextMediaEditorView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("HandleCommandL"));

	if (iContainer->IsLoadingMedia()) return;

	MUploadCallBack* cb=iCallBack; iCallBack=0; 
	TFileName f =iFileName; iFileName=KNullDesC;
	CC_TRAPD(err, HandleCommandInnerL(aCommand, f, cb));

	if (err!=KErrNone) {
		iFileName=f; iCallBack=cb;
		User::Leave(err);
	}
}

void CContextMediaEditorView::HandleCommandInnerL(TInt aCommand, TFileName& f, MUploadCallBack* cb)
{
	if (cb) {
		/* prompt mode */
		switch(aCommand) {
			case EcontextmediaappCmdPublish: {
				iContainer->save_contribution_to_post(ETrue);
				CCMPost * post= iContainer->get_current_post();
				if (post) {
					if (!iStandAlone && iThreadTarget!=CPostStorage::MediaPoolId()) {
						post->iParentId() = iThreadTarget;
						ResetThreadTarget();
					} else if (iStandAlone) {
						//FIXME: magic number
						post->iParentId() = TInt64(3);
						iStorage.UpdatePostL(post); 
					}
				}

				TBool del=ETrue;
				Settings().GetSettingL(SETTING_DELETE_UPLOADED, del);
				Reporting().DebugLog(_L("editor:call_back"));
				
				cb->Back(true, del, post);
			}
			break;

			case EcontextmediaappCmdPlay: {
				CContextMediaAppPostContainer * c = (CContextMediaAppPostContainer*)iContainer;
				c->Play();
				// no real action, so we do:
				iCallBack=cb; 
				iFileName=f;
			}
			break;

			case EcontextmediaappCmdPause: {
				CContextMediaAppPostContainer * c = (CContextMediaAppPostContainer*)iContainer;
				c->Pause();
				// no real action, so we do:
				iCallBack=cb; 
				iFileName=f;
			}
			break;

			case EcontextmediaappCmdOpenInGallery: {
				iCallBack=cb; 
				iFileName=f;
				CCMPost * aPost = iContainer->get_current_post();
				const TDesC& fn=aPost->iMediaFileName();
				RAApaLsSession session; session.ConnectLA();
				TThreadId threadId;
				User::LeaveIfError(session.StartDocument(fn, threadId));
				return;
				}
				break;
			case EcontextmediaappCmdStop: {
				CContextMediaAppPostContainer * c = (CContextMediaAppPostContainer*)iContainer;
				c->Stop();
				// no real action, so we do:
				iCallBack=cb; 
				iFileName=f;
			}
			break;

			case EcontextmediaappCmdContinue: {
				CContextMediaAppPostContainer * c = (CContextMediaAppPostContainer*)iContainer;
				c->Resume();
				// no real action, so we do:
				iCallBack=cb; 
				iFileName=f;
			}
			break;
		
			case EAknSoftkeyBack: 
			case EcontextmediaappCmdNewImage:
			case EcontextmediaappCmdDontpublish:
			case EcontextmediaappCmdDelete:
			case EcontextmediaappCmdPostpone: {
				ResetThreadTarget();
				iContainer->save_contribution_to_post(EFalse); // save fields to post
				CCMPost * aPost = iContainer->get_current_post();
				if (aPost && iStandAlone && aCommand==EcontextmediaappCmdDontpublish) {
					//FIXME: magic number
					aPost->iParentId()=TInt64(7);
				}
				if (aPost && iStandAlone && aCommand==EcontextmediaappCmdDelete) {
					//FIXME: magic number
					aPost->iParentId()=TInt64(9);
				}
				if (aPost) {
					iStorage.UpdatePostL(aPost); // save post to storage
					Reporting().DebugLog(_L("editor:call_cancel"));
					cb->Back(false, false, aPost);
				}
			}
			break;
			/*
			case EcontextmediaappCmdDelete: {
				ResetThreadTarget();
				HBufC * message = CEikonEnv::Static()->AllocReadResourceLC(R_DELETE_MESSAGE);
				CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(*message);
				dlg->SetHeaderText(_L("Delete media"));
				CleanupStack::PushL(dlg);
				dlg->PrepareLC(R_CONFIRMATION_QUERY);
				CleanupStack::Pop(dlg);
				if ( dlg->RunLD() ) {
					CleanupStack::PopAndDestroy(); //message
					CCMPost * aPost = iContainer->get_current_post();
					if (aPost) cb->Back(false, true, aPost);
				} else {
					CleanupStack::PopAndDestroy(); //message
					// no action taken -> back to initial state before command
					iCallBack=cb; 
					iFileName=f;
					return;
				}
			}
			break;
			*/

			case EAknSoftkeyCancel: {
				// deletes completely the captured media
				CCMPost * aPost = iContainer->get_current_post();
				if (aPost) cb->Back(false, true, aPost);
			}
			break;
			
			default:
				// no action taken, or outside command : restore callback and filename;
				iCallBack=cb; 
				iFileName=f;
				if ((aCommand >= EcontextmediaappCmdSendAppUi) && (aCommand < EcontextmediaappCmdSendAppUi+5)) {
					iContainer->save_contribution_to_post(EFalse);
					CCMPost * aPost = iContainer->get_current_post();
					if (! aPost) return;
					if (! iContainer->CloseCurrentMedia()) return;
					OpenPostInMessageEditorL(AppContext(),
						*iSendUi, *aPost,
						aCommand);
				} 
				return;
				break;
		}
		{
			TBuf<50> msg=_L("previous view: 0x");
			msg.AppendNum(iPrevViewId.iAppUid.iUid, EHex);
			msg.Append(_L(" ")); msg.AppendNum( int(iPrevViewId.iViewUid.iUid) );
			Reporting().UserErrorLog(msg);
		}
#ifndef __WINS__
		if (iNextViewId) {*iNextViewId=iPrevViewId;}
#endif
	} else {
		/*normal view mode*/
		switch(aCommand) {
			case EAknSoftkeySelect:
			case EcontextmediaappCmdPublish:
				{
					CCMPost * post = iContainer->get_current_post();
					if (post) {
						if (!iStandAlone) {
							post->iParentId() = iThreadTarget;
						} else {
						}
						ResetThreadTarget();
						iContainer->save_contribution_to_post(ETrue);
						iStorage.UpdatePostL(post);
						TBool del=ETrue;
						Settings().GetSettingL(SETTING_DELETE_UPLOADED, del);
						TFileName fn;
						CC_TRAPD(err, fn=iTransferer->AddFileToQueueL(post->iMediaUrl(), 
							SETTING_PUBLISH_URLBASE, 
							SETTING_PUBLISH_SCRIPT, del, _L("dummy"), post));
						if (err!=KErrNone) User::Leave(err);
						
						if (iStandAlone) {
							//FIXME: magic number
							post->iParentId() = TInt64(3);
						}
						post->iMediaUrl() = fn;
						post->iMediaFileName() = fn;

						iStorage.UpdatePostL(post);
						Reporting().ShowGlobalNote(3,
							_L("Queued for Publishing"));
						
					}
				}
				break;
			case EAknSoftkeyBack:
			case EcontextmediaappCmdPostpone:
			case EAknSoftkeyCancel:
			case EcontextmediaappCmdDontpublish:
				{
					iContainer->save_contribution_to_post(EFalse);
					CCMPost * aPost = iContainer->get_current_post();
					if (aPost && iStandAlone && aCommand==EcontextmediaappCmdDontpublish) {
						//FIXME: magic number
						aPost->iParentId()=TInt64(7);
					}
					if (aPost) iStorage.UpdatePostL(aPost);// save post to storage
					ResetThreadTarget();
				}
				break;
			case EcontextmediaappCmdPlay:
				{
					CContextMediaAppPostContainer * c = (CContextMediaAppPostContainer*)iContainer;
					c->Play();
				}
				break;
			case EcontextmediaappCmdDelete: {
				if (!iStandAlone) {
					ResetThreadTarget();
					HBufC * message = CEikonEnv::Static()->AllocReadResourceLC(R_DELETE_MESSAGE);
					CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(*message);
					dlg->SetHeaderText(_L("Delete media"));
					CleanupStack::PushL(dlg);
					dlg->PrepareLC(R_CONFIRMATION_QUERY);
					CleanupStack::Pop(dlg);
					if ( dlg->RunLD() ) {
						CCMPost * aPost = iContainer->get_current_post();
						if (aPost) iStorage.RemovePostL(aPost, ETrue);
					} else {
						return;
					}
					CleanupStack::PopAndDestroy(); //message
				} else {
					CCMPost * aPost = iContainer->get_current_post();
					if (!aPost) return;
					aPost->iParentId()=TInt(CPostStorage::ETrash);
					iStorage.UpdatePostL(aPost);
					iStorage.UpdateErrorInfo(aPost->iPostId(), 0);
				}
			}
			break;
			case EcontextmediaappCmdShowErrors: {
				CErrorInfoView* view=(CErrorInfoView*)AppUi()->View(KErrorInfoView);
				if (!view) User::Leave(KErrNotSupported);
				CCMPost * aPost = iContainer->get_current_post();
				if (!aPost || aPost->iErrorInfo) User::Leave(KErrArgument);
				view->ShowWithData(*aPost->iErrorInfo);
				return;
				}
				break;
			case EcontextmediaappCmdOpenInGallery: {
				CCMPost * aPost = iContainer->get_current_post();
				const TDesC& fn=aPost->iMediaFileName();
				RAApaLsSession session; session.ConnectLA();
				TThreadId threadId;
				User::LeaveIfError(session.StartDocument(fn, threadId));
				}
				return;
				break;
			default:
				if (aCommand >= EcontextmediaappCmdSendAppUi) {
					iContainer->save_contribution_to_post(EFalse);
					CCMPost * aPost = iContainer->get_current_post();
					if (! aPost) return;
					OpenPostInMessageEditorL(AppContext(),
						*iSendUi, *aPost,
						aCommand);
				} 
				return;
				break;
		}
		if (iPrevViewId.iAppUid==TUid::Uid(0)) {
			AppUi()->ActivateLocalViewL(iPrevViewId.iViewUid);
		} else {
			AppUi()->ActivateViewL(iPrevViewId);
		}
	}
}

void CContextMediaEditorView::HandleClientRectChange()
{
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("HandleClientRectChange"));

	//if ( iContainer ) iContainer->SetRect( TRect(TPoint(0,0),TSize(176,188)) );
}


EXPORT_C void CContextMediaEditorView::SetPreviousView(const TVwsViewId& aPrevViewId)
{
	iPrevViewId = aPrevViewId;	
}

void CContextMediaEditorView::DoActivateL(const TVwsViewId& aPrevViewId, TUid, const TDesC8&)
{
	iIsActivated=ETrue;
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("DoActivateL"));

	iEikonEnv->AddForegroundObserverL(*this);
	MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);

	if (!iStandAlone) {
		StatusPane()->MakeVisible(EFalse);
	}

	iMediaBrowsingOn=(iCallBack!=0);
	if (!iStandAlone || iCallBack!=0) iPrevViewId = aPrevViewId;
	if (!iContainer) {
		iContainer = new (ELeave) CContextMediaAppPostContainer(iCMNetwork, iStorage, 
			iThreadTarget, iSort, iOrder, this, iStandAlone, iTagStorage, iFileName,
			0, iTagIcons, iAskForNames);
		iContainer->SetPresence(iPresence); iPresence=0;
		iContainer->SetMopParent(this);
		TRect r=AppUi()->ApplicationRect();
		Cba()->ReduceRect(r);
		CC_TRAPD(err, iContainer->ConstructL( r ) );
		if (err==KErrNone) {
			AppUi()->AddToStackL(*this, iContainer );
		} else {
			delete iContainer; iContainer=0;
			User::Leave(err);
		}
        }

	if (iCallBack!=0 && !iStandAlone) {
		if ((iPrevViewId.iAppUid == KCameraUid) 
			|| (iPrevViewId.iAppUid == KCamera2Uid) 
			|| (iPrevViewId.iAppUid == KCamera3Uid) 
			|| (iPrevViewId.iAppUid == KVideoUid) 
			|| (iPrevViewId.iAppUid == KRecorderUid) ) {
                
			RWsSession ws;
			User::LeaveIfError( ws.Connect() );
			TApaTaskList taskList( ws );
			TApaTask task = taskList.FindApp(iPrevViewId.iAppUid);
			if(task.Exists()) task.KillTask();
			ws.Close();
		}
	}
}

void CContextMediaEditorView::DoDeactivate()
{
	if (!iIsActivated) {
		iIsActivated=EFalse;
		MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
		if (rep) rep->SetInHandlableEvent(ETrue);
	} else {
		iEikonEnv->RemoveForegroundObserver(*this);
	}
	iIsActivated=EFalse;
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("DoDeactivate"));

	if (iContainer) AppUi()->RemoveFromViewStack( *this, iContainer );
	CC_TRAPD(err, delete iContainer);
	if (err!=KErrNone) {
		User::Panic(_L("ContextMediaUi1"), err);
	}
	iContainer = NULL;
	if (iCallBack) iCallBack->Back(false, false, 0);
	iCallBack=0;
}

EXPORT_C void CContextMediaEditorView::SetThreadTarget(TInt64 aCodeId)
{
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("SetThreadTarget"));

	iThreadTarget = aCodeId;
}

EXPORT_C void CContextMediaEditorView::ResetThreadTarget()
{
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("ResetThreadTarget"));

	iThreadTarget = CPostStorage::MediaPoolId();;
}

void CContextMediaEditorView::Prompt(const TDesC& FileName, MUploadCallBack* CallBack)
{
	Reporting().DebugLog(_L("editor:prompt"));

	iCallBack = CallBack;
	iFileName=FileName;

	if (iContainer) {
		DoDeactivate();
		TUid dummy={0};
		DoActivateL(iPrevViewId, dummy, _L8(""));
	} else {
		AppUi()->ActivateLocalViewL(Id());
	}
}

void CContextMediaEditorView::SetPresence(CBBPresence* aPresence)
{
	delete iPresence;
	iPresence=aPresence;
}

void CContextMediaEditorView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CContextMediaEditorView"), _CL("DynInitMenuPaneL"));

#ifndef __S60V3__
	if (aResourceId == R_SENDUI_MENU) {
		iSendUi->DisplaySendCascadeMenuL(*aMenuPane);
#else
	if (0) {
#endif
	} else if (aResourceId == R_CONTEXTMEDIA_MENUBAR_EDITOR_MENUBAR_MENU) {
		
		CCMPost * aPost = iContainer->get_current_post();
		TInt size=0;
#ifndef __S60V3__
	//FIXME3RD
		TInt flags=TSendingCapabilities::ESupportsAttachmentsOrBodyText;
		if (aPost->iMediaFileName().Length()>0) {
			size=32*1024;
			flags|=TSendingCapabilities::ESupportsAttachments;
			//flags|=TSendingCapabilities::ESupportsAttachmentsOrBodyText;
		}
		TSendingCapabilities c( 0, 
			(*(aPost->iBodyText))().Length()+2+aPost->iSender.iName().Length()+size, flags);
		
#endif
		aMenuPane->SetItemDimmed(EcontextmediaappCmdShowErrors, ETrue);
		if (iCallBack==0) {
			if (aPost->iErrorInfo) aMenuPane->SetItemDimmed(EcontextmediaappCmdShowErrors, EFalse);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdPlay, !iContainer->IsCurrentPlayableMedia());
			aMenuPane->SetItemDimmed(EAknSoftkeySelect, iStandAlone);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdPublish, !iStandAlone);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdDelete, !iStandAlone); //FIXME: check this
			aMenuPane->SetItemDimmed(EcontextmediaappCmdNewImage, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdPostpone, !iStandAlone);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdDontpublish, !iStandAlone);
#ifndef __S60V3__
			iSendUi->DisplaySendMenuItemL(*aMenuPane, 1, c);
#else
			iSendUi->AddSendMenuItemL(*aMenuPane, 1, EcontextmediaappCmdSendAppUi);
#endif
			
		} else {
			aMenuPane->SetItemDimmed(EcontextmediaappCmdPlay, !iContainer->IsCurrentPlayableMedia());
			aMenuPane->SetItemDimmed(EAknSoftkeySelect, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdPublish, EFalse);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdDelete, EFalse);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdNewImage, iStandAlone);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdPostpone, !iStandAlone);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdDontpublish, !iStandAlone);
#ifndef __S60V3__
	//FIXME3RD
			iSendUi->DisplaySendMenuItemL(*aMenuPane, 4, c);
#endif
		}
	}
}

void CContextMediaEditorView::HandleGainingForeground()
{
	iTimer->Cancel();
}

void CContextMediaEditorView::HandleLosingForeground()
{
	if (iCallBack==0) return;

	if (iTimerWait>2) return;

	if (iTimerWait==0) iTimerWait=1;

	iTimer->Wait(iTimerWait);
	iTimerWait*=2;
}

void CContextMediaEditorView::expired(CBase*)
{
	AppUi()->ActivateLocalViewL(Id());
}
