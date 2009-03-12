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

#ifndef CONTEXTCONTACTSVIEW_H
#define CONTEXTCONTACTSVIEW_H

#include "ccu_jaikuview_base.h"
#include "phonebook.h"
#include "phonehelper_ui.h"
#include "jabberdata.h"
#include "file_output_base.h"
#include "settings.h"
#include "ccn_message.h"
#include "cn_networkerror.h"
#include "jaiku_viewids.hrh"

#include <akniconarray.h> 
#ifndef __S60V3__
#include <sendui.h> 
#endif

const TUid KContactsViewId = { JAIKU_CONTACTS_VIEWID };
const TUid KViewId = { JAIKU_CONTACTS_VIEWID }; // remove
 
class CContextContactsContainer;
class CPbkContactItem;
class TPbkContactItemField;

class CContextContactsView;
class CUserPics;
class CContactsTabGroups;


class CContextContactsView : public CJaikuViewBase, public MSettingListener, public MNetworkErrorObserver
{
 public: 
	CContextContactsView(Mfile_output_base * log, 
						 phonehelper_ui& phoneHelper, 
						 class MPresenceFetcher* aPresenceFetcher,
						 class CMessaging *aMessaging) : 
		aLog(log), 
		aPhoneHelper(phoneHelper), 
		mailbox_defined(EFalse), 
		current_contact_idx(-1), 
		current_contact_top_idx(-1), 
		iPresenceFetcher(aPresenceFetcher), 
		iMessaging(aMessaging) { }

        void ConstructL();
        ~CContextContactsView();
 public: 
        TUid Id() const;
        void HandleClientRectChange();
		void HandleResourceChange( TInt aType );

	void HandleCommandL(TInt aCommand);
	void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane);
	TInt GetCurrentContactId();
	void before_exit();
	void ResetListFocusL();

	TBool DummyContactBehaviorL(TInt aCommand);

 private: // From CJaikuViewBase
	void ReleaseViewImpl();
	void RealDoActivateL(const TVwsViewId& aPrevViewId,TUid aCustomMessageId, const TDesC8& aCustomMessage);
	void RealDoDeactivateL();
	
 private: // own utility functions
	void CreateContainerL();
	void RemoveContainer();
		
	void SendInvitationSMSL();

	void ShowNotSupportedForDummyMsgL();

	void TryDeleteContactL();
	void DeleteMultipleContactsUiL( CArrayFix<TInt>& aSelection );
	void DeleteSingleContactUiL( TInt contact_id );
	void DeleteContactL( TInt aId );

	void SendVCardL(TInt aCommand);

	void CallL();

	void CreateMessageL(TInt aCommand);	
	void HandleCreateMessageL( TAddressSelectorF aSelector, 
							   TMessageSenderF aSender, 
							   TPbkFieldFilterF aFilter,
							   TInt aWarningResource,
							   TUid aMtm);
	void CreateEmailL();
	void CreateMmsL();
	void CreateSmsL();
	
	void SettingChanged(TInt aSetting);
	virtual void NetworkError(TErrorDisplay aMode, const TDesC& aMessage);
	virtual void NetworkSuccess();
	
	TBool ShouldShowConnectionErrorMenuItemL();

 private: 
	CContextContactsContainer* iContactContainer;
	class CMainBgContainer* iBgContainer;

	bool exiting;
	Mfile_output_base * aLog;
	HBufC *iCaptionUnmark;
#ifndef __S60V3__
	class CSendAppUi * iSendAppUi;
	enum TWhichSendUi { EVCard, ECreateMessage };
	TWhichSendUi iWhichSendUi;
#else
	class CSendUi * iSendAppUi;
#endif
	CArrayFix< TUid > *iBtAndIr;
	phonehelper_ui& aPhoneHelper;
	TBool mailbox_defined;
	TBool iAuthenticationError;
	TInt current_contact_idx;
	TInt current_contact_top_idx;

	class MPresenceFetcher* iPresenceFetcher;
	class CServerMessageListener* iServerMessage;

	class CMessaging *iMessaging;
	CNetworkError*	iNetworkError;
};
#endif

