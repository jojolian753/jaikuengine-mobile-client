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

#ifndef CONTEXTMEDIAAPPVIEW2_H
#define CONTEXTMEDIAAPPVIEW2_H

#include <aknview.h>
#include "transferdir2.h"
#include "transfer2.h"
#include "contextmediaappcontainer.h"

class CContextMediaEditorView : public CAknView, public MContextBase, public MUploadPrompt,
	MCoeForegroundObserver, MTimeOut
{
	struct TCallBackItem {
		TFileName		iFileName;
		MUploadCallBack*	iCallBack;
		TCallBackItem() : iCallBack(0) { }
		TCallBackItem(const TDesC& aFileName, MUploadCallBack* aCallBack) :
			iFileName(aFileName), iCallBack(aCallBack) { }
	};

public: 
	IMPORT_C CContextMediaEditorView(
		TUid aViewId, TVwsViewId* NextViewId, MApp_context& Context, CCMNetwork &aCMNetwork, 
		CPostStorage &aStorage, MBBDataFactory * aFactory, CHttpTransfer2* aTransferer,
		TBool aStandAlone=EFalse, class CAknIconArray * iTagIcons=0,
		class MAskForNames* aAskForNames=0);
        IMPORT_C void ConstructL();
        virtual ~CContextMediaEditorView();

public:
	virtual void Prompt(const TDesC& FileName, MUploadCallBack* CallBack);

public: 
        virtual TUid Id() const;
        virtual void HandleCommandL(TInt aCommand);
        virtual void HandleCommandInnerL(TInt aCommand, TFileName& f, MUploadCallBack* cb);
        virtual void HandleClientRectChange();
	virtual void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane);

private:
        void DoActivateL(const TVwsViewId& aPrevViewId,TUid aCustomMessageId, const TDesC8& aCustomMessage);
        void DoDeactivate();

	//MCoeForegroundObserver
	void HandleGainingForeground();
	void HandleLosingForeground();
	void expired(CBase* );

	void ReleaseCContextMediaEditorView();
public:
	IMPORT_C void SetThreadTarget(TInt64 aCodeId);
	IMPORT_C void ResetThreadTarget();
	virtual void SetPresence(class CBBPresence* aPresence); //takes ownership
	IMPORT_C void SetPreviousView(const TVwsViewId& aPrevViewId); // for standalone mode

private: 
        CContextMediaAppPostContainer* iContainer;
	TUid iViewId;
	TVwsViewId iPrevViewId;
	TInt64 iThreadTarget;
	CCMNetwork &iCMNetwork;
	CPostStorage &iStorage;
	MBBDataFactory * iFactory;
	TBool iMediaBrowsingOn;

	// -------
	TVwsViewId*	iNextViewId;
	MUploadCallBack* iCallBack;
	TFileName	iFileName;
	//---------
	CHttpTransfer2* iTransferer;
	CPostStorage::TOrder iOrder;
	CPostStorage::TSortBy iSort;
	TInt64 iNode;
	TBool		iStandAlone;

#ifndef __S60V3__
	class CSendAppUi * iSendUi;
#else
	class CSendUi * iSendUi;
#endif
	class CDb*	iTagDb;
	class CTagStorage* iTagStorage;
	class CBBPresence* iPresence;
	class CAknIconArray * iTagIcons;
	class CTimeOut*	iTimer;
	TInt		iTimerWait;

	TBool	iIsActivated;
	MAskForNames* iAskForNames;
};

#endif
