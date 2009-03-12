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
#include "ContextMediaAppView.h"
#include "ContextMediaAppView2.h"
#include <ContextMediaUi.rsg>
#include "ContextMediaUi.hrh"
#include <cntdb.h>
#include <aknpopup.h> 
#include <apgcli.h> 
#include <eikdll.h>
#include <eikmenup.h>
#include  <sendui.h>
#include  "cm_sendui.h"
#ifndef __S60V3__
#include  <SENDNORM.RSG>
#endif
#include  <aknmessagequerydialog.h> 
#include "cu_errorui.h"
#include "bberrorinfo.h"
#include "raii_apgcli.h"
#include <aknviewappui.h>
#include "app_context_impl.h"
#include "reporting.h"
#include "cu_busy.h"
#include <apgtask.h>
#include "reporting.h"
const TUid KUidPhone = { 0x100058b3 };

TBool AppExists(TUid aAppUid) 
{
	RApaLsSession appList;
	CleanupClosePushL(appList);

	User::LeaveIfError(appList.Connect());
	TApaAppInfo dummy;
	TInt ret = appList.GetAppInfo(dummy, aAppUid);
	CleanupStack::PopAndDestroy(); 
	return (ret==KErrNone);
}

//_-----------------------------------------------------

EXPORT_C CContextMediaAppThreadView::CContextMediaAppThreadView(MContextMediaAppUi& aAppUi,
																TUid aViewId, TUid aNextViewId, 
																CPostStorage &aStorage, 
																CPostStorage::TOrder aOrder, CPostStorage::TSortBy aSort,
																TInt64 aNode, TBool is_single_thread_view, TBool is_full_screen, 
																phonehelper &aPhoneHelper, CCMNetwork &aCMNetwork, 
																MUploadPrompt * aVisualPrompt, TBool aStandAlone,
																CAknIconArray * aTagIcons, class CHttpTransfer2* aTransferer,
																class MAskForNames* aAskForNames, TInt aResourceId
																
			   ) :  iViewId(aViewId), iNextViewId(aNextViewId), iStorage(aStorage), iCMNetwork(aCMNetwork), 
				iOrder(aOrder), iSort(aSort), 
				iNode(aNode), is_single_thread_view(is_single_thread_view), is_full_screen(is_full_screen), 
				iPhoneHelper(aPhoneHelper), iVisualPrompt(aVisualPrompt), iAppUi(aAppUi),
				iStandAlone(aStandAlone), iTagIcons(aTagIcons), iTransferer(aTransferer),
				iAskForNames(aAskForNames), iResourceId(aResourceId) { }


EXPORT_C void CContextMediaAppThreadView::ConstructL()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("ConstructL"));

	iEngine=CPbkContactEngine::Static();
	if (iEngine) {
		owns_engine=false;
	} else {
		iEngine=CPbkContactEngine::NewL();
		owns_engine=true;
	}

	if (iResourceId==0) {
		if (Id() == KThreadsByDatePromptId) {
			BaseConstructL( R_CONTEXTMEDIAAPP_THREADSPROMPT );
		} else if (Id() == KPostViewId) {
			BaseConstructL( R_CONTEXTMEDIAAPP_POSTVIEW);
		} else if (is_single_thread_view) {
			BaseConstructL( R_CONTEXTMEDIAAPP_THREADVIEW );
		} else {
			BaseConstructL( R_CONTEXTMEDIAAPP_THREADSVIEW );
		}
	} else {
		BaseConstructL( iResourceId );
	}
#ifndef __S60V3__
	iSendUi = CSendAppUi::NewL(EcontextmediaappCmdSendAppUi, 0);
#else
	iSendUi = CSendUi::NewL();
#endif

	iCameraAppFound=AppExists(KCameraUid);
	iCameraApp2Found=AppExists(KCamera2Uid);
	iVideoAppFound=AppExists(KVideoUid);
	iVideoApp2Found=AppExists(KVideo2Uid);
	iAudioAppFound=AppExists(KRecorderUid);
}


CContextMediaAppThreadView::~CContextMediaAppThreadView() {
	CC_TRAPD(err, ReleaseCContextMediaAppThreadView());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}

void CContextMediaAppThreadView::ReleaseCContextMediaAppThreadView()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("ReleaseCContextMediaAppThreadView"));

	if ( iContainer ) AppUi()->RemoveFromViewStack( *this, iContainer );
	delete iContainer;
	if (owns_engine) delete iEngine;
	delete iSendUi;
}

TUid CContextMediaAppThreadView::Id() const {
	return iViewId;
}

void ShowWaitLoad()
{
	auto_ptr<HBufC> text(CEikonEnv::Static()->AllocReadResourceL(R_WAIT_FOR_LOAD));
	GetContext()->Reporting().ShowGlobalNote(
		//EAknGlobalInformationNote, 
		1, 
		*text);
}

void CContextMediaAppThreadView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("HandleCommandL"));

	CAknViewAppUi * appui = (CAknViewAppUi *)AppUi();

	/* Normal use of view */
	if (this->iCallBack == 0) {
		switch ( aCommand )
		{
			case EAknSoftkeyBack:{
				if (is_single_thread_view || iStandAlone) {
					if (iPrevViewId.iAppUid==TUid::Uid(0)) {
						if (iPrevViewId.iViewUid!=TUid::Uid(0)) 
							AppUi()->ActivateLocalViewL(iPrevViewId.iViewUid);
						else
							AppUi()->HandleCommandL(aCommand);
					} else {
						AppUi()->ActivateViewL(iPrevViewId);
					}
				} else {
					AppUi()->HandleCommandL( aCommand );
				}
				break;
			}
			case EcontextmediaappCmdReFetch:{
				CCMPost * aPost = iContainer->get_current_post();
				if (aPost) {
					aPost->iMediaFileName().Zero();
					aPost->iErrorCode() = 0;
					aPost->iErrorDescr().Zero();
					CC_TRAPD(ignore, iStorage.UpdatePostL(aPost));// save post to storage
					iCMNetwork.FetchMedia(aPost, ETrue /*force refetch*/);
					((CContextMediaAppPostContainer*)iContainer)->display_current();
				}
				break;
			}
			case EcontextmediaappCmdHide:{
				CCMPost * aPost = iContainer->get_current_post();
				if (aPost) {
					TInt idx = iContainer->get_current_idx();
					TInt top = iContainer->get_top_idx();
					//TInt count = iContainer->get_item_count();
                                        iStorage.SetThreadVisibleL(aPost->iPostId(), EFalse);
					top--; if (top<0) top=0;
					idx--; if (idx<0) idx=0;
					iContainer->set_top_idx(top);
					iContainer->set_current_idx(idx);
				}
				break;
			}

			case EcontextmediaappCmdUnhideAll:{
				iStorage.SetAllThreadsVisibleL();
				break;
			}

			case EcontextmediaappCmdOpenItem:{
				CCMPost * aPost = iContainer->get_current_post();
				if (aPost && iNextViewId.iUid) {
					TInt64 postid=TInt64(0);
					if (iStandAlone) {
						if (aPost->iPostId() != iStorage.RootId()) {
							if (! iStorage.FirstL(aPost->iPostId(), CPostStorage::EByDate, 
								CPostStorage::EDescending, EFalse) ) return;
							postid=iStorage.GetCurrentIdL();
						} else {
							if (! iStorage.FirstAllL() ) return;
							postid=iStorage.RootId();
						}
						if (aPost->iPostId() == iStorage.MediaPoolId()) {
							CContextMediaEditorView * view = 
								(CContextMediaEditorView*)(appui->View(KEditorViewId));
							view->SetThreadTarget(postid);
							view->SetPreviousView( TVwsViewId( TUid::Uid(0), Id()) );
							AppUi()->ActivateLocalViewL(view->Id());
						} else {
							CContextMediaAppThreadView * view = 
								(CContextMediaAppThreadView *)(appui->View(iNextViewId));
							view->SetNodeId(postid);
							view->SetPreviousView( TVwsViewId(TUid::Uid(0), Id()) );

							AppUi()->ActivateLocalViewL(view->Id());
						}
					} else {
						postid=aPost->iPostId();
						CContextMediaAppThreadView * view = 
							(CContextMediaAppThreadView *)(appui->View(iNextViewId));
						view->SetNodeId(postid);
						AppUi()->ActivateLocalViewL(view->Id());
					}
				}
				break;
			}
			case EcontextmediaappCmdPurgeFrom:
			case EcontextmediaappCmdDeleteFrom: {
				CCMPost * aPost = iContainer->get_current_post();
				if (! aPost ) return;
				if (iContainer->IsLoadingMedia()) {
					ShowWaitLoad();
					return;
				}
				if (aPost->iParentId() == TInt64(CPostStorage::ETrash) ) {
					auto_ptr<CAutoBusyMessage> busy(CAutoBusyMessage::NewL(R_PURGE_WAITNOTE));
					iStorage.RemovePostsL(aPost->iParentId(), aPost->iTimeStamp(), 
						CPostStorage::EByDateTime);
				} else {
					auto_ptr<CAutoBusyMessage> busy(CAutoBusyMessage::NewL(R_DELETE_WAITNOTE));
					iStorage.MovePostsL(aPost->iParentId(), TInt64(CPostStorage::ETrash), aPost->iTimeStamp(), 
						CPostStorage::EByDateTime);
				}
							    }
				break;
			case EcontextmediaappCmdPurge:
			case EcontextmediaappCmdDelete: {
				CCMPost * aPost = iContainer->get_current_post();
				if (! aPost ) return;
				if (aPost->iParentId() == TInt64(CPostStorage::ETrash) ) {
					if (iContainer->IsLoadingMedia()) {
						ShowWaitLoad();
						return;
					}

					/*HBufC * message = CEikonEnv::Static()->AllocReadResourceLC(R_DELETE_MESSAGE);
					CAknMessageQueryDialog * dlg = CAknMessageQueryDialog::NewL(*message);
					CleanupStack::PushL(dlg);
					dlg->PrepareLC(R_CONFIRMATION_QUERY);
					CleanupStack::Pop(dlg);
					if ( dlg->RunLD() ) {
						if (aPost) iStorage.RemovePostL(aPost);
					} else {
					} 
					CleanupStack::PopAndDestroy(); //message
					*/
					iStorage.RemovePostL(aPost, ETrue);
				} else {
					iTransferer->RemoveFromQueueL(aPost->iMediaFileName());
					aPost->iParentId()=TInt(CPostStorage::ETrash);
					iStorage.UpdatePostL(aPost);
					iStorage.UpdateErrorInfo(aPost->iPostId(), 0);
				}
			}
			break;
			case EcontextmediaappCmdRepublish: {
				CCMPost * aPost = iContainer->get_current_post();
				if (!aPost) return;
				iTransferer->RemoveFromQueueL(aPost->iMediaFileName());
				aPost->iParentId()=CPostStorage::MediaPoolId();
				iStorage.UpdatePostL(aPost);
				iStorage.UpdateErrorInfo(aPost->iPostId(), 0);
				CContextMediaEditorView * view = 
					(CContextMediaEditorView*)(appui->View(KEditorViewId));
				view->SetThreadTarget(aPost->iPostId());
				view->SetPreviousView( TVwsViewId(TUid::Uid(0), KThreadsByLastPostViewId) );
				AppUi()->ActivateLocalViewL(view->Id());
				}
				break;
			case EcontextmediaappCmdRefresh:{
				if (iNode!=CPostStorage::RootId()) {
					iCMNetwork.FetchThread(iNode);
				} else {
					iCMNetwork.FetchThreads();
				}
				break;
			}

			case EcontextmediaappCmdMarkAsRead:{
				iContainer->mark_as_read();
				break;
			}

			case EcontextmediaappCmdFirstPost:
			case EcontextmediaappCmdReply:{
				auto_ptr<CEikTextListBox> list (new(ELeave) CAknSinglePopupMenuStyleListBox);
				CAknPopupList * popupList = CAknPopupList::NewL(list.get(), 
					R_AVKON_SOFTKEYS_OK_BACK, AknPopupLayouts::EMenuWindow);
				CleanupStack::PushL(popupList);

				list->ConstructL(popupList, CEikListBox::ELeftDownInViewRect);
				list->CreateScrollBarFrameL(ETrue);
				list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);

				auto_ptr<CArrayFixFlat<TInt> > commands (new (ELeave) CArrayFixFlat<TInt>(5));
				auto_ptr<CDesCArrayFlat> items (new (ELeave) CDesCArrayFlat(5));
				
				if (iStorage.FirstL(CPostStorage::MediaPoolId(), CPostStorage::EByDate, CPostStorage::EDescending, ETrue)) {
					TBuf<50> buf;
					iCoeEnv->ReadResource(buf, R_CAPTURED);
					items->AppendL(buf);
					commands->AppendL(EcontextmediaappCmdReplyWithCaptured);
				}
	                        
				if (iCameraAppFound) {
					TBuf<50> buf;
					iCoeEnv->ReadResource(buf, R_CAMERA);
					items->AppendL(buf);
					commands->AppendL(EcontextmediaappCmdCamera);
				}

				if (iCameraApp2Found) {
					TBuf<50> buf;
					iCoeEnv->ReadResource(buf, R_CAMERA);
					items->AppendL(buf);
					commands->AppendL(EcontextmediaappCmdCamera2);
				}

				if (iVideoAppFound) {
					TBuf<50> buf;
					iCoeEnv->ReadResource(buf, R_VIDEO);
					items->AppendL(buf);
					commands->AppendL(EcontextmediaappCmdVideo);
				}

				if (iVideoApp2Found) {
					TBuf<50> buf;
					iCoeEnv->ReadResource(buf, R_VIDEO);
					items->AppendL(buf);
					commands->AppendL(EcontextmediaappCmdVideo2);
				}

				if (iAudioAppFound) {
					TBuf<50> buf;
					iCoeEnv->ReadResource(buf, R_AUDIO);
					items->AppendL(buf);
					commands->AppendL(EcontextmediaappCmdAudio);
				}

				CTextListBoxModel* model = list->Model();
				model->SetItemTextArray(items.release());
				model->SetOwnershipType(ELbmOwnsItemArray);

				TBuf<50> title;
				iCoeEnv->ReadResource(title, R_REPLY);
				popupList->SetTitleL(title);

				
				TInt popupOk = popupList->ExecuteLD();
				CleanupStack::Pop();
				if (popupOk) {
					TInt index = list->CurrentItemIndex();
					HandleCommandL(commands->At(index));
				}
				break;
			}

			case EcontextmediaappCmdReplyWithCaptured: {
				CContextMediaEditorView * view = (CContextMediaEditorView*)(appui->View(KEditorViewId));
				if (Id() == KPostViewId) {
					CCMPost * aPost = iContainer->get_current_post();
					if (aPost) view->SetThreadTarget(aPost->iParentId());
				} else {
					view->SetThreadTarget(iNode);
				}
				AppUi()->ActivateLocalViewL(view->Id());
				break;
			}
			case EcontextmediaappCmdCamera:{
				CContextMediaEditorView * view = (CContextMediaEditorView*)(appui->View(KEditorViewId));
				if (Id() == KPostViewId) {
					CCMPost * aPost = iContainer->get_current_post();
					if (aPost) view->SetThreadTarget(aPost->iParentId());
				} else {
					view->SetThreadTarget(iNode);
				}
				ActivateViewL(TVwsViewId(KCameraUid, KCameraViewUid));
				break;
			}
		        case EcontextmediaappCmdCamera2:{
				CContextMediaEditorView * view = (CContextMediaEditorView*)(appui->View(KEditorViewId));
				if (Id() == KPostViewId) {
					CCMPost * aPost = iContainer->get_current_post();
					if (aPost) view->SetThreadTarget(aPost->iParentId());
				} else {
					view->SetThreadTarget(iNode);
				}
				ActivateViewL(TVwsViewId(KCamera2Uid, KCamera2ViewUid));
				break;
			}
			case EcontextmediaappCmdVideo:{
				CContextMediaEditorView * view = (CContextMediaEditorView*)(appui->View(KEditorViewId));
				if (Id() == KPostViewId) {
					CCMPost * aPost = iContainer->get_current_post();
					if (aPost) view->SetThreadTarget(aPost->iParentId());
				} else {
					view->SetThreadTarget(iNode);
				}
				ActivateViewL(TVwsViewId(KVideoUid, KVideoViewUid));
				break;
			}
		      case EcontextmediaappCmdVideo2:{
				CContextMediaEditorView * view = (CContextMediaEditorView*)(appui->View(KEditorViewId));
				if (Id() == KPostViewId) {
					CCMPost * aPost = iContainer->get_current_post();
					if (aPost) view->SetThreadTarget(aPost->iParentId());
				} else {
					view->SetThreadTarget(iNode);
				}
				ActivateViewL(TVwsViewId(KVideo2Uid, KVideo2ViewUid));
				break;
			}
			case EcontextmediaappCmdAudio:{
				CContextMediaEditorView * view = (CContextMediaEditorView*)(appui->View(KEditorViewId));
				if (Id() == KPostViewId) {
					CCMPost * aPost = iContainer->get_current_post();
					if (aPost) view->SetThreadTarget(aPost->iParentId());
				} else {
					view->SetThreadTarget(iNode);
				}
				//ActivateViewL(TVwsViewId(KRecorderUid, KRecorderViewUid), KRecorderMessageUid, KNullDesC8);
#ifndef __S60V3__
				//FIXME3RD
				auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\voicerecorder\\voicerecorder.app")));
				cmd->SetCommandL(EApaCommandRun);
				CC_TRAPD(err, EikDll::StartAppL(*cmd));
#endif
				break;
			}

			case EcontextmediaappCmdCall: {
				CCMPost * aPost = iContainer->get_current_post();
				if (aPost && aPost->iSender.iPhoneNo().Length()>0) {
					CC_TRAPD(err, iPhoneHelper.make_callL(aPost->iSender.iPhoneNo()));
				}
				break;
			}

			case EcontextmediaappCmdSms: {
				CCMPost * aPost = iContainer->get_current_post();
				if (aPost && aPost->iSender.iPhoneNo().Length()>0) {
					CDesCArrayFlat* recip=new CDesCArrayFlat(1);
					CleanupStack::PushL(recip);
					
					CDesCArrayFlat* alias=new CDesCArrayFlat(1);
					CleanupStack::PushL(alias);
					
					recip->AppendL(aPost->iSender.iPhoneNo());
					alias->AppendL(aPost->iSender.iName());
					
					iPhoneHelper.send_sms(recip, alias);

					CleanupStack::PopAndDestroy(2); // recip, alias, title
				}
				break;
			}
			case EcontextmediaappCmdPlay: 
				{
					CContextMediaAppPostContainer * c = (CContextMediaAppPostContainer*)iContainer;
					if ( ((c->iMediaFileType==CContextMediaAppPostContainer::EAudio)) || ((c->iMediaFileType==CContextMediaAppPostContainer::EVideo) ) ) {
						c->Play();
					} else {
						//FIX ME: Message here
					}
				}
				break;
			case EcontextmediaappCmdContinue: 
				((CContextMediaAppPostContainer*)iContainer)->Resume();
				break;
			case EcontextmediaappCmdStop: 
				((CContextMediaAppPostContainer*)iContainer)->Stop();
				break;
			case EcontextmediaappCmdPause: 
				((CContextMediaAppPostContainer*)iContainer)->Pause();
				break;
			case EcontextmediaappCmdShowErrors: {
				CErrorInfoView* view=(CErrorInfoView*)AppUi()->View(KErrorInfoView);
				if (!view) 
					User::Leave(KErrNotSupported);
				CCMPost * aPost = iContainer->get_current_post();
				if (!aPost || !aPost->iErrorInfo) 
					User::Leave(KErrArgument);
				view->ShowWithData(*aPost->iErrorInfo);
				
				}
				break;
			case EcontextmediaappCmdOpenInGallery: {
				CCMPost * aPost = iContainer->get_current_post();
				const TDesC& fn=aPost->iMediaFileName();
				RAApaLsSession session; session.ConnectLA();
				TThreadId threadId;
				User::LeaveIfError(session.StartDocument(fn, threadId));
				}
				break;
			default:  {
				if ((aCommand >= EcontextmediaappCmdSendAppUi) && (aCommand < EcontextmediaappCmdSendAppUi+5)) {
					CCMPost * aPost = iContainer->get_current_post();
					if (! aPost) return;
					if (! iContainer->CloseCurrentMedia()) return;
					OpenPostInMessageEditorL(AppContext(),
						*iSendUi, *aPost,
						aCommand);
				} else {
					AppUi()->HandleCommandL( aCommand );
				}
				break;
			}
		 }
	} else {
		MUploadCallBack* cb=iCallBack;
		iCallBack=0;
		/* Prompt use of the view */
		switch ( aCommand )
		{
			case EAknSoftkeyCancel:

				cb->Back(false, false, 0);
				break;
			case EcontextmediaappCmdOpenItem:
			case EAknSoftkeySelect: {
				CCMPost * aPost = iContainer->get_current_post();
				if (aPost) {
					auto_ptr<CCMPost> buf(CCMPost::NewL(0));
					buf->iParentId()=aPost->iPostId();
					cb->Back(true, true, buf.get());
				} else {
					if (iVisualPrompt) {
						iVisualPrompt->Prompt(KNullDesC, cb);
					} else {
						cb->Back(false, false, 0);
					}
				}
				break;
			}
			default:
				break;
		}
	}
}

void CContextMediaAppThreadView::HandleClientRectChange()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("HandleClientRectChange"));

	if ( iContainer ) iContainer->SetRect(ClientRect());
}

void CContextMediaAppThreadView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("DynInitMenuPaneL"));

	const CCMPost * aPost = iContainer->get_current_post();
	TBool is_placeholder=EFalse;
	if (aPost) {
		CC_TRAPD(ignore, is_placeholder=iStorage.IsPlaceHolderL(aPost->iPostId()));
	}

	//multi-thread view menu
#ifndef __S60V3__
	if (aResourceId == R_SENDUI_MENU) {
		iSendUi->DisplaySendCascadeMenuL(*aMenuPane);
#else
	if (0) {
#endif
	} else if (aResourceId == R_CONTEXTMEDIA_THREADS_PANE) {
		if (aPost && !iStandAlone) {
			aMenuPane->SetItemDimmed(EcontextmediaappCmdOpenItem, is_placeholder);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdRefresh, EFalse);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdHide, is_placeholder);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdMarkAsRead, !(aPost->iUnreadCounter()>0));
			aMenuPane->SetItemDimmed(EcontextmediaappCmdUnhideAll, !iStorage.HasHiddenThreads());
		} else {
			aMenuPane->SetItemDimmed(EcontextmediaappCmdOpenItem, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdMarkAsRead, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdRefresh, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdHide, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdUnhideAll, ETrue);
		}
	} else if (aResourceId == R_CONTEXTMEDIA_THREAD_PANE) {
		// 1-thread view menu
		
		if (!iStandAlone) {
			aMenuPane->SetItemDimmed(EcontextmediaappCmdRefresh, EFalse);
			if (iContainer->get_item_count() == 1) {
				aMenuPane->SetItemDimmed(EcontextmediaappCmdFirstPost, EFalse);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdReply, ETrue);
			} else {
				aMenuPane->SetItemDimmed(EcontextmediaappCmdFirstPost, ETrue);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdReply, EFalse);
			}

			if (aPost) {
				aMenuPane->SetItemDimmed(EcontextmediaappCmdOpenItem, EFalse);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdMarkAsRead, !(aPost->iUnreadCounter()>0));
				aMenuPane->SetItemDimmed(EcontextmediaappCmdContact, !IsKnownContact(aPost->iSender.iPhoneNo()));
			} else {
				aMenuPane->SetItemDimmed(EcontextmediaappCmdOpenItem, ETrue);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdMarkAsRead, ETrue);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdContact, ETrue);
			}
		} else {
			aMenuPane->SetItemDimmed(EcontextmediaappCmdRefresh, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdMarkAsRead, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdContact, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdFirstPost, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdReply, ETrue);
		}
	} else if (aResourceId == R_CONTEXTMEDIA_MENUBAR_POST_MENU) {
		aMenuPane->SetItemDimmed(EcontextmediaappCmdShowErrors, ETrue);
		if (!iStandAlone) {
			if (aPost) {
				if (aPost->iErrorInfo) aMenuPane->SetItemDimmed(EcontextmediaappCmdShowErrors, EFalse);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdReFetch, (aPost->iErrorCode()==KErrNone));
				aMenuPane->SetItemDimmed(EcontextmediaappCmdPlay, !iContainer->IsCurrentPlayableMedia());
				aMenuPane->SetItemDimmed(EcontextmediaappCmdContact, !IsKnownContact(aPost->iSender.iPhoneNo()));
				aMenuPane->SetItemDimmed(EcontextmediaappCmdMarkAsRead, !(aPost->iUnreadCounter()>0));
			} else {
				aMenuPane->SetItemDimmed(EcontextmediaappCmdReFetch, ETrue);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdPlay, ETrue);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdContact, ETrue);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdMarkAsRead, ETrue);
			}
			aMenuPane->SetItemDimmed(EcontextmediaappCmdRepublish, ETrue);
		} else {
			TInt size=0;
#ifndef __S60V3__
				//FIXME3RD
			TInt flags=TSendingCapabilities::ESupportsAttachmentsOrBodyText;
#endif
			if (aPost) {
				if (aPost->iErrorInfo) aMenuPane->SetItemDimmed(EcontextmediaappCmdShowErrors, EFalse);
				if (aPost->iMediaFileName().Length()>0) {
					size=32*1024;
#ifndef __S60V3__
				//FIXME3RD
					flags|=TSendingCapabilities::ESupportsAttachments;
					//flags|=TSendingCapabilities::ESupportsAttachmentsOrBodyText;
#endif
				}
				aMenuPane->SetItemDimmed(EcontextmediaappCmdRepublish, aPost->iParentId()==TInt64(CPostStorage::EDrafts));
			} else {
				aMenuPane->SetItemDimmed(EcontextmediaappCmdRepublish, ETrue);
				aMenuPane->SetItemDimmed(EcontextmediaappCmdShowErrors, ETrue);
			}
#ifndef __S60V3__
				//FIXME3RD
			TSendingCapabilities c( 0, 
				size, flags);
#endif
			TBool dim_delete=ETrue, dim_delete_from=ETrue;
			TBool dim_purge=ETrue, dim_purge_from=ETrue;

			if (aPost) {
				if (aPost->iParentId()==TInt64(CPostStorage::ETrash) ) {
					dim_purge=EFalse;
					if (iNode!=CPostStorage::RootId()) dim_purge_from=EFalse;
				} else {
					dim_delete=EFalse;
					if (iNode!=CPostStorage::RootId()) dim_delete_from=EFalse;
				}
				aMenuPane->SetItemDimmed(EcontextmediaappCmdOpenInGallery, EFalse );
			} else {
				aMenuPane->SetItemDimmed(EcontextmediaappCmdOpenInGallery, ETrue );
			}
			aMenuPane->SetItemDimmed(EcontextmediaappCmdDelete, dim_delete );
			aMenuPane->SetItemDimmed(EcontextmediaappCmdDeleteFrom, dim_delete_from );
			aMenuPane->SetItemDimmed(EcontextmediaappCmdPurge, dim_purge );
			aMenuPane->SetItemDimmed(EcontextmediaappCmdPurgeFrom, dim_purge_from );

			aMenuPane->SetItemDimmed(EcontextmediaappCmdPlay, !iContainer->IsCurrentPlayableMedia());
			aMenuPane->SetItemDimmed(EcontextmediaappCmdReply, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdReFetch, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdMarkAsRead, ETrue);
			aMenuPane->SetItemDimmed(EcontextmediaappCmdContact, ETrue);
			if (aPost)
#ifndef __S60V3__
				iSendUi->DisplaySendMenuItemL(*aMenuPane, 1, c);
#else
				iSendUi->AddSendMenuItemL(*aMenuPane, 1, EcontextmediaappCmdSendAppUi);
#endif
		}
	} else if (aResourceId == R_REPLY_MENU) {
		if (!iStorage.FirstL(CPostStorage::MediaPoolId(), 
			CPostStorage::EByDate, CPostStorage::EDescending, ETrue)) aMenuPane->SetItemDimmed(EcontextmediaappCmdReplyWithCaptured, ETrue);
		if (!iCameraAppFound) aMenuPane->SetItemDimmed(EcontextmediaappCmdCamera, ETrue);
		if (!iCameraApp2Found) aMenuPane->SetItemDimmed(EcontextmediaappCmdCamera2, ETrue);
		if (!iVideoAppFound) aMenuPane->SetItemDimmed(EcontextmediaappCmdVideo, ETrue);
		if (!iVideoApp2Found) aMenuPane->SetItemDimmed(EcontextmediaappCmdVideo2, ETrue);
		if (!iAudioAppFound) aMenuPane->SetItemDimmed(EcontextmediaappCmdAudio, ETrue);
	} else {
		AppUi()->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}


EXPORT_C void CContextMediaAppThreadView::SetPreviousView(const TVwsViewId& aPrevViewId)
{
	iPrevViewId = aPrevViewId;	
}


void CContextMediaAppThreadView::DoActivateL(
   const TVwsViewId& aPrevViewId,TUid aCustomMessageId,
   const TDesC8& aCustomMessage)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("DoActivateL"));

	MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);

      	if (Id() != KThreadsByDatePromptId){
		iAppUi.LastViewBeforePrompt()=Id();
		CContextMediaEditorView * view = (CContextMediaEditorView*)(AppUi()->View(KEditorViewId));
		view->ResetThreadTarget();
	}
		
	if (!iStandAlone)
		StatusPane()->MakeVisible(!is_full_screen);

	TRect r = ClientRect();

	// managing previous view
	// FIXME: this should be done more cleanly
	if ( !iStandAlone &&
		((aPrevViewId.iAppUid==KUidMeaningApp)  // remember previous view from same app only
		&& (aPrevViewId.iViewUid != KEditorViewId) // for prompt
		&& (aPrevViewId.iViewUid != KThreadsByDatePromptId)
		&& (aPrevViewId.iViewUid != KVisualCodePromptId)
		&& (aPrevViewId.iViewUid != KPostViewId)))
	{
		iPrevViewId = aPrevViewId;
	}
		
	// managing previous indices
	if (aPrevViewId.iViewUid == KThreadViewId) {
		if (Id()==KPostViewId) iCurrentIndex=iTopIndex=0;
	} else if (aPrevViewId.iViewUid == KPostViewId) {
	} else if (aPrevViewId.iViewUid == KVisualCodeViewId) {
	} else {
		iCurrentIndex=iTopIndex=0;
	}

	if (Id() != KThreadsByDatePromptId) {
		TVwsViewId v = iAppUi.NextView();
		if ( v != TVwsViewId()) {
			iAppUi.SetNextView(TVwsViewId());

			{
				/* it tends not to be safe to try to re-activate
				   the media capture apps if they've been shut down
				*/
				RWsSession& ws=CEikonEnv::Static()->WsSession();
				TApaTaskList tl(ws);
				TApaTask app_task=tl.FindApp(v.iAppUid);
				if (! app_task.Exists() ) {
					// so activate the phone app instead
					TApaTask app_task=tl.FindApp(KUidPhone);
					if (app_task.Exists()) app_task.BringToForeground();
					goto done_activate;
				} else {
					//Reporting().UserErrorLog(_L("previous app exists"));
				}
			}
			if (v.iAppUid == KCameraUid) {
				CC_TRAPD(err, ActivateViewL(TVwsViewId(KCameraUid, KCameraViewUid)));
			} else if (v.iAppUid == KCamera2Uid) {
				/* the camera on 6630 will be stuck in showing the photo on reactivation */
#ifndef __S60V3__
				//FIXME3RD
				auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\camcorder\\camcorder.app")));
				cmd->SetCommandL(EApaCommandRun);
				CC_TRAPD(err, EikDll::StartAppL(*cmd));
#endif
			} else if (v.iAppUid == KCamera3Uid) {
#ifndef __S60V3__
				//FIXME3RD
				/* the camera on N70 will hang if the slider's been closed and we
				   try to reactivate it */
				//Reporting().UserErrorLog(_L("starting camera3"));
				auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\cammojave\\cammojave.app")));
				cmd->SetCommandL(EApaCommandRun);
				CC_TRAPD(err, EikDll::StartAppL(*cmd));			
#endif
			} else if (v.iAppUid == KVideoUid) {
				CC_TRAPD(err, ActivateViewL(TVwsViewId(KVideoUid, KVideoViewUid)));
                        } else if (v.iAppUid == KRecorderUid) {
#ifndef __S60V3__
				//FIXME3RD
				auto_ptr<CApaCommandLine> cmd(CApaCommandLine::NewL(_L("z:\\system\\apps\\voicerecorder\\voicerecorder.app")));
				cmd->SetCommandL(EApaCommandRun);
				CC_TRAPD(err, EikDll::StartAppL(*cmd));			
#endif
			} else {
				//Reporting().UserErrorLog(_L("activating previous"));
				CC_TRAPD(err, ActivateViewL(v));
			}
		} 
	} 

done_activate:

	// Normal display        
	if (!iStandAlone) iAppUi.SetTab(Id().iUid);  // it's safe even if no tab

        if (Id() == KPostViewId) {
		if (!iContainer) {
			iContainer = new (ELeave) CContextMediaAppPostContainer(iCMNetwork, iStorage, 
				iNode, iSort, iOrder, this, iStandAlone, 0, KNullDesC, 0, iTagIcons,
				iAskForNames);
			iContainer->SetMopParent(this);

			TRect r=AppUi()->ApplicationRect();
			Cba()->ReduceRect(r);
			iContainer->ConstructL( r );
			AppUi()->AddToStackL( *this, iContainer);
		}
	} else {
		if (!iContainer) {
			iContainer = new (ELeave) CContextMediaAppListboxContainer(iCMNetwork, iStorage, 
				iNode, iSort, iOrder, this, iStandAlone);
			iContainer->SetMopParent(this);
			iContainer->ConstructL( r );
			iContainer->set_top_idx(iTopIndex);
			iContainer->set_current_idx(iCurrentIndex);
			AppUi()->AddToStackL( *this, iContainer);
		}
		iContainer->DrawNow();
	}
}

void CContextMediaAppThreadView::DoDeactivate()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("DoDeactivate"));

	if ( iContainer ) {
		iCurrentIndex = iContainer->get_current_idx();
		iTopIndex = iContainer->get_top_idx();
		AppUi()->RemoveFromViewStack( *this, iContainer );
		iContainer->MakeVisible(EFalse);
        }
    	CC_TRAPD(err, delete iContainer);
	if (err!=KErrNone) {
		User::Panic(_L("Error-in-delete"), 1);
	}
	iContainer = NULL;
}

EXPORT_C void CContextMediaAppThreadView::SetNodeId(TInt64 aNode) {
	iNode = aNode;
}

EXPORT_C void CContextMediaAppThreadView::SetTopIndex(TInt idx) {
	iTopIndex = idx;
}

EXPORT_C void CContextMediaAppThreadView::SetCurrentIndex(TInt idx) {
	iCurrentIndex = idx;
}

TBool CContextMediaAppThreadView::IsKnownContact(const TDesC& phone_number)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("IsKnownContact"));

	if (phone_number.Length() <=0) return EFalse;
	TInt nbDigits, errorCode = 0;
	
#ifndef __S60V3__
	TInt phoneno;
	phoneno = iEngine->Database().TextToPhoneMatchNumber(iContainer->get_current_post()->iSender.iPhoneNo(), nbDigits, 8);
	if (phoneno == 0) return EFalse;
	CContactIdArray * matching_contact_ids = 0;	
	CC_TRAP(errorCode, matching_contact_ids = iEngine->Database().MatchPhoneNumberL(phoneno) );
#else
	CContactIdArray * matching_contact_ids = 0;	
	const TDesC& phoneno=iContainer->get_current_post()->iSender.iPhoneNo();
	CC_TRAP(errorCode, matching_contact_ids = iEngine->Database().MatchPhoneNumberL(phoneno, 8) );
#endif
	
	TBool ret = ((errorCode!=KErrNotFound) && (matching_contact_ids->Count()>0));
	delete matching_contact_ids; matching_contact_ids=0; 
	return ret;
}

EXPORT_C TBool CContextMediaAppThreadView::IsSingleThreadView()
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("IsSingleThreadView"));

	return this->is_single_thread_view;
}

void CContextMediaAppThreadView::Prompt(const TDesC& /*FileName*/, MUploadCallBack* CallBack)
{
	CALLSTACKITEM_N(_CL("CContextMediaAppThreadView"), _CL("Prompt"));

	iCallBack = CallBack;
	
	AppUi()->ActivateLocalViewL(this->Id());
}
