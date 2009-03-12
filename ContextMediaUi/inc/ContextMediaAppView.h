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

#ifndef CONTEXTMEDIAVIEW_H
#define CONTEXTMEDIAVIEW_H

#include <aknview.h>
#include "cm_storage.h"
#include <cpbkcontactengine.h>
#include "view_ids.h"
#include "phonehelper.h"
#include "contextmediaappcontainer.h"
#include "cm_network.h"
#include "transferdir2.h"

class CContextMediaAppThreadView : public CAknView, public MUploadPrompt, public MContextBase
{
public: 
	IMPORT_C CContextMediaAppThreadView(MContextMediaAppUi& aAppUi,	TUid aViewId, TUid aNextViewId, CPostStorage &aStorage, 
					CPostStorage::TOrder aOrder, CPostStorage::TSortBy aSort,
					TInt64 aNode, TBool is_single_thread_view, TBool is_full_screen, 
					phonehelper &aPhoneHelper, CCMNetwork &aCMNetwork, 
					MUploadPrompt * aVisualPrompt, TBool aStandAlone=EFalse,
					class CAknIconArray * aTagIcons=0, class CHttpTransfer2* aTransferer=0,
					class MAskForNames* aAskForNames=0, TInt aResourceId=0
				   );
	IMPORT_C void ConstructL();
	virtual ~CContextMediaAppThreadView();
public:
	virtual void Prompt(const TDesC& FileName, MUploadCallBack* CallBack);
public:
	IMPORT_C void SetNodeId(TInt64 aNode);
	IMPORT_C TBool IsSingleThreadView();

public:
	IMPORT_C void SetTopIndex(TInt idx);
	IMPORT_C void SetCurrentIndex(TInt idx);

public: 
	virtual TUid Id() const;
	virtual void HandleCommandL(TInt aCommand);
	virtual void HandleClientRectChange();
	virtual void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
	IMPORT_C void SetPreviousView(const TVwsViewId& aPrevViewId); // for standalone mode

private:
	void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	void DoDeactivate();

private:
	TBool IsKnownContact(const TDesC& phone_number);
	void ReleaseCContextMediaAppThreadView();

private: 
	CContextMediaAppGeneralContainer* iContainer;
	TUid iViewId;
	TUid iNextViewId;
	TVwsViewId iPrevViewId;
		
	CPostStorage &iStorage;
	CCMNetwork &iCMNetwork;
	CPostStorage::TOrder iOrder;
	CPostStorage::TSortBy iSort;
	TInt64 iNode;

	TInt iTopIndex;
	TInt iCurrentIndex;

	CPbkContactEngine * iEngine;
	TBool owns_engine;

	TBool is_single_thread_view;
	TBool is_full_screen;

	phonehelper &iPhoneHelper;

	TBool iCameraAppFound;
	TBool iCameraApp2Found;
        TBool iVideoAppFound;
	TBool iVideoApp2Found;
	TBool iAudioAppFound;

	MUploadCallBack*	iCallBack;
	MUploadPrompt * iVisualPrompt;
	MContextMediaAppUi& iAppUi;
	TBool iStandAlone;
#ifndef __S60V3__
	class CSendAppUi * iSendUi;
#else
	class CSendUi * iSendUi;
#endif
	class CAknIconArray * iTagIcons;
	class CHttpTransfer2* iTransferer;
	class MAskForNames* iAskForNames;
	TInt iResourceId;
};

#endif


