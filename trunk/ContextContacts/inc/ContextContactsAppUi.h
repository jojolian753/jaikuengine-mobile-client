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

#ifndef CONTEXTCONTACTSAPPUI_H
#define CONTEXTCONTACTSAPPUI_H

// INCLUDES
#include <eikapp.h>
#include <eikdoc.h>
#include <e32std.h>
#include <coeccntx.h>
#include <aknviewappui.h>
#include <akntabgrp.h>
#include <aknnavide.h>
#include <akniconarray.h>

#include "ContextContactsDetailView.h"
#include "file_output_base.h"
#include "app_context.h"
#include "phonebook.h"
#include "jabberdata.h"
#include "cb_presence.h"
#include "contextcontactsview.h"
#include "cbbsession.h"
#include "contextappui.h"
#include "ccn_message.h"

#include "contextvariant.hrh"

#ifndef __JAIKU__ 
#include "presencedetailview.h"
#include "contextcontactsview2.h"
#endif

#ifdef __S60V3__
#include <rpbkviewresourcefile.h> 
#endif

class CContextContactsContainer;



class CContextContactsAppUi : public CAknViewAppUi, MCoeForegroundObserver, 
	public MContextBase, public MContextAppUi
{
 public: 
	CContextContactsAppUi (MApp_context& Context);
	void ConstructL();
	void InnerConstructL();
	~CContextContactsAppUi();
        
 public: // New functions
	void DisplayPresenceDetailsL(const TDesC& Name, const TDesC& aNick, const CBBPresence* Data);
	void DisplayRichPresenceL();
	void DisplayPresenceDescriptionL(const TDesC& Name, const CBBPresence* Data);
	void DisplayContactDetailsL( TInt contact_id );
	void DisplayPersonalStreamL();

	void ShowAboutDialogL();
	
	CDb* JabberDb();
	void SetCurrentContactIdL(TInt aId );

	TBool DummyContactBehaviorL(TInt /*aCommand*/);
	
 private: // Own 
	void ActivateJaikuViewL(TUid aUid);

	void PostJaikuL();
	void HandleOpenContactL();
	void HandleDisplayRichPresenceL();
	void HandleDisplayContactDetailsL();

	void HandleCallL();
	void HandleInfoMemL();

	void EditNickL(TBool aEditable);
	void SendInvitationSMSL();
	void ShowConnectionErrorL();
	void ShowLastJaikuViewErrorL();

	void ReleaseCContextContactsAppUi();


	CActiveContact& ActiveContact() const;

 private: // Construction related 

	void WriteVersionToLogL();

	void N70InitL();
	void ConstructNotifyWindowL();
	void ConstructBackendL();	
	void ConstructPhonebookAndPresenceL();
	void ConstructLayoutL();
	void ConstructIconsL();
	void ConstructUiHelpersL();
	void ConstructViewsL();

	void SetViewDependenciesL( CJaikuViewBase& aView ) const;
	void LoadLegacyPresenceIconsL();
	void LoadPhonebookViewResourceL();
	void LoadContextContactsUiResourceL();

	void PruneFeedItemsL();


 private: // From framework 

	virtual void HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination);
	virtual void HandleGainingForeground();
	virtual void HandleLosingForeground();
	virtual void HandleResourceChangeL( TInt aType );
	virtual TErrorHandlerResponse HandleError
	    (TInt aError,
	     const SExtendedError& aExtErr,
	     TDes& aErrorText,
	     TDes& aContextText);
     
	void HandleCommandL(TInt aCommand);
	virtual TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

	TBool ProcessCommandParametersL(TApaCommand aCommand, 
									TFileName& aDocumentName,const TDesC8& aTail);
 	void ShowHideNoteIfNecessaryL();
 private:

	phonebook*	iPhonebook;
	phonehelper_ui *iPhoneHelper;
	CDb*		iJabberDb;
	CJabberData*	iJabberData;
	class CMessaging*	iMessaging;

	CContextContactsView* iContactsView; // OWNED BY FRAMEWORK
	class CRichPresenceView* iRichPresenceView; // OWNED BY FRAMEWORK
	CContextContactsDetailView * iContactDetailView; // OWNED BY FRAMEWORK
	class CEverybodysFeedView* iEverybodysFeedView; // OWNED BY FRAMEWORK
	class CFeedGroupView* iFeedGroupView; // OWNED BY FRAMEWORK
	class CAuthorFeedView* iAuthorFeedView; // OWNED BY FRAMEWORK
	class CCommentsView* iCommentsView; // OWNED BY FRAMEWORK
#ifndef __JAIKU__
 	class CPresenceDetailView	*iPresenceDetailView; // OWNED BY FRAMEWORK
	class CPresenceDescriptionView	*iPresenceDescriptionView; // OWNED BY FRAMEWORK
	CContextContactsView2* iGroupsView; // OWNED BY FRAMEWORK
#endif 

	CPresenceUpdater* iPresenceUpdater;
	CPresenceHolder* iPresenceHolder;

	CArrayFixFlat<TInt>* iResource_files;
	class CUninstallSupport*	iUninstallSupport;

	static const TInt KJaikuSettings[];
	class CSettingsView* iSettingsView;
	class CDynamicSettingsView* iSettingsView2;


#ifdef __S60V3__
	RPbkViewResourceFile	iPbkResource;
#endif

	class CJuikLayout* iLayout;
	class CJuikIconManager* iIconManager;

	class CJabberPics* iJabberPics;
	class CUserPics* iUserPics;

	class CBTDeviceList		*iBuddyBTs, *iLaptopBTs, *iDesktopBTs, *iPDABTs;
	class CBTDevApView		*iBuddyBTView, *iLaptopBTView, *iDesktopBTView, *iPDABTView;

	class CErrorInfoView* iErrorInfoView;
	
	class CAboutUi* iAboutUi;

	class CFeedEngine* iFeedEngine;
	
	class CActiveState* iActiveState;

	class CPoster* iPoster;

	class CPosterUi* iPosterUi;

	class CViewNavigation* iViewNavigation; // ownership transfered to Context
	
	class CJaicons* iJaicons;

	class CThemeColors* iThemeColors;
	
	class CTimePeriodFormatter* iPeriodFormatter;

	class CJuikGfxStore* iFeedGraphics;

	class CStreamStatsCacher* iStreamStatsCacher;

	class CJaikuCommonMenus* iCommonMenus;

	class CProgressBarModel* iProgressBarModel;

	class CSettingsUi* iSettingsUi;
	
	TUid	iNextViewUid;
	TBool	iDidBack;
};

#endif


