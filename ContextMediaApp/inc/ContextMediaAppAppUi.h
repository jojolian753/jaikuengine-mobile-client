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

#ifndef CONTEXTMEDIAAPPAPPUI_H
#define CONTEXTMEDIAAPPAPPUI_H

#include <eikapp.h>
#include <eikdoc.h>
#include <e32std.h>
#include <coeccntx.h>
#include <aknviewappui.h>
#include <akntabgrp.h>
#include <aknnavide.h>
#include <akniconarray.h>
#include "phonehelper.h"
#include "contextmediaappview.h"
#include "contextmediaappview2.h"
#include "app_context.h"
#include <settingsview.h>
#include <recognizerview.h>
#include "cm_storage.h"
#include "cm_network.h"
#include "transferdir2.h"
#include "picture_publisher.h"
#include "status_notif.h"
#include "codeprompt.h"
#include "cm_oldprompt.h"
#include "popup_notifier.h"
#include "contextappui.h"

class CContextMediaAppAppUi : public CAknViewAppUi, public MContextBase, public MRecognizerCallback, 
				public MSocketObserver, public i_status_notif,
				public MUploadPrompt, MUploadCallBack, public MBusyIndicator,
				public MContextMediaAppUi, public MContextAppUi
{
	struct TCallBackItem {
		TFileName		iFileName;
		MUploadCallBack*	iCallBack;
		TCallBackItem() : iCallBack(0) { }
		TCallBackItem(const TDesC& aFileName, MUploadCallBack* aCallBack) :
			iFileName(aFileName), iCallBack(aCallBack) { }
	};

public: 
	CContextMediaAppAppUi(MApp_context& Context, 
		CPostStorage &aThreadStorage, CBBDataFactory * aFactory, 
		CCMNetwork &iCMNetwork);
        void ConstructL();
        ~CContextMediaAppAppUi();
private:
	void InnerConstructL();

public:
	void CodeSelected(const CCodeInfo& aCode);
	void CodeSelected(TInt64 id);
	void Cancelled();

public:
	virtual void ShowBusy(TBool aIsBusy);

private:
	void Prompt(const TDesC& FileName, MUploadCallBack* CallBack);
	TFileOpStatus Back(bool Upload, bool DeleteFromPhone, MBBData* Packet);
	void AddMediaPublisher(CPicturePublisher* aPublisher);
	void PublishOldMedia();

public:
	virtual TUid LastViewBeforePrompt();
	virtual TVwsViewId NextView();
	virtual void SetNextView(TVwsViewId aViewId);
	virtual void SetTab(TInt tabId);

private:
	void success(CBase * source);
	void error(CBase *source,TInt code,const TDesC& aReason);
	void info(CBase * source, const TDesC &msg);

private:
	void finished() {}
	inline void error(const TDesC& ) {}
	inline void status_change(const TDesC& ) {}

private:
        void HandleCommandL(TInt aCommand);
	virtual TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

private:
        CAknNavigationControlContainer* iNaviPane;
        CAknNavigationDecorator*        iDecoratedTabGroup;
	CAknTabGroup*                   iTabGroup;

	CRecognizerView *		iVisualCodeView;
	CContextMediaAppThreadView *	iThreadsByLastPostView;
	CContextMediaAppThreadView *	iThreadsByAuthorView;
	CContextMediaAppThreadView *	iThreadsByTitleView;

	CContextMediaAppThreadView *    iThreadsByDatePrompt;

	CContextMediaAppThreadView *	iThreadView;
	CContextMediaAppThreadView *	iPostView;

	CContextMediaEditorView *	iEditorView;

	CSettingsView *			iSettingsView;

	CPostStorage &iThreadStorage;
	CCMNetwork &iCMNetwork;
	CBBDataFactory * iFactory;

	phonehelper * iPhoneHelper;

	CArrayPtrFlat<CPicturePublisher> *iMediaPublishers;
	CTransferDir * iTransferDir;

	CVisualCodePrompt *		iVisualPrompt;

	// -------
	CList<TCallBackItem> *iCallBacks;
	bool		iNext;       //??   
	MUploadCallBack* iCallBack;
	TFileName	iFileName;
	TBool		iUpload;
	TBool		iDelete;
	CCMPost		*iPacket;
	TUid		iCurrentPrompt;	

	TCMOldPrompt	*iOldPrompt;
	CPopupNotifier * iBusyNotifier;

	TInt		iCmuiResourceFile;
public:
	TUid		iLastViewBeforePrompt;
	TVwsViewId	iNextView;
};

#endif

