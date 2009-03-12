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

#include "ContextMenuAppUi.h"
#include "contextmenu.hrh"
#include "contextmenuapp.h"
#include "app_context_impl.h"
#include "contextcommon.h"
#include <uikon.hrh>
#include <eiktxlbx.h>
#include <aknpopup.h>
#include <aknlists.h> 
#include <apgwgnam.h>
#include <avkon.mbg>
#include "cl_settings.h"
#include "contextmenucontainer.h"

const TUid KUserViewId = {4}; // context_log

void CContextMenuAppUi::ConstructL()
{
	iAppHidden=EFalse;
        BaseConstructL(0x1008);
	/*
	StatusPane()->MakeVisible(EFalse);
	StatusPane()->ReduceRect(TRect(TPoint(0,0), TSize(100,100)));
	StatusPane()->ApplyCurrentSettingsL();
	*/
	
	CContextMenuView2 * view = new (ELeave) CContextMenuView2();
	CleanupStack::PushL( view );
	view->ConstructL();
	AddViewL( view );      // transfer ownership 
	CleanupStack::Pop();
	SetDefaultViewL(*view);

	// create 1 unique icon array
	iIconArray = new (ELeave) CAknIconArray(3);
	iIconArray->AppendL(iEikonEnv->CreateIconL(
			_L("Z:\\system\\data\\avkon.mbm"), EMbmAvkonQgn_stat_bt_blank, EMbmAvkonQgn_stat_bt_blank+1 ));
	iIconArray->AppendL(iEikonEnv->CreateIconL(
			_L("Z:\\system\\data\\avkon.mbm"), EMbmAvkonQgn_indi_checkbox_on, EMbmAvkonQgn_indi_checkbox_on_mask ));
	iIconArray->AppendL(iEikonEnv->CreateIconL(
			_L("Z:\\system\\data\\avkon.mbm"), EMbmAvkonQgn_indi_checkbox_off, EMbmAvkonQgn_indi_checkbox_off_mask ));
	iIconArray->AppendL(iEikonEnv->CreateIconL(
			_L("Z:\\system\\data\\avkon.mbm"), EMbmAvkonQgn_indi_navi_arrow_right, EMbmAvkonQgn_indi_navi_arrow_right_mask ));
}

CContextMenuAppUi::~CContextMenuAppUi()
{
	delete iIconArray;
}

void CContextMenuAppUi::HandleForegroundEventL(TBool onForeground)
{
	if (!iAppHidden) {
		// Hide from tasklist
		// this code coes not work in constructL but works fine here :( ??
		TInt wgId = iEikonEnv->RootWin().Identifier();
		RWsSession session = iEikonEnv->WsSession();
		CApaWindowGroupName* wgName = CApaWindowGroupName::NewLC(session, wgId);
		wgName->SetHidden(ETrue);
		wgName->SetWindowGroupName(iEikonEnv->RootWin());
		CleanupStack::PopAndDestroy(); 
		iAppHidden=ETrue;
	}

	if (onForeground) {
		ShowMenuL(EContextMenuMain);
	} else {
		iPopupCount=0;
	}
}

void CContextMenuAppUi::HandleCommandL(TInt aCommand)
{
	switch ( aCommand ) {
		case EEikCmdExit: {
			PrepareToExit();
			Exit();
			break;
		}
		case EContextMenuMyContext: {
			Hide(); // necessary to handle the view switch neatly
			ActivateViewL(TVwsViewId(KUidcontext_log, KUserViewId), TUid::Null(), KNullDesC8);
			break;
		};
		case EContextMenuSetDescription: {
			User::InfoPrint(_L("EContextMenuSetDescription"));
			break;
		};
		case EContextMenuTurnPresenceOn: {
			User::InfoPrint(_L("EContextMenuTurnPresenceOn"));
			break;
		};
		case EContextMenuTurnPresenceOff: {
			User::InfoPrint(_L("EContextMenuTurnPresenceOff"));
			break;
		};
		case ETest1On: {
			User::InfoPrint(_L("ETest1On"));
			break;
		};
		case ETest1Off: {
			User::InfoPrint(_L("ETest1Off"));
			break;
		};
		case ETest2On: {
			User::InfoPrint(_L("ETest2On"));
			break;
		};
		case ETest2Off: {
			User::InfoPrint(_L("ETest2Off"));
			break;
		};
		case EContextMenuFriends: {
			ShowMenuL(EContextMenuFriends);
			break;
		};
		case EAknSoftkeyBack:
		case EAknSoftkeyCancel: {
			Hide();
/*
#ifdef __WINS__
			ActivateViewL(TVwsViewId(KUidMenu, TUid::Uid(1)), TUid::Null(), KNullDesC8);
#else 
			ActivateViewL(TVwsViewId(KUidPhone, TUid::Uid(1)), TUid::Null(), KNullDesC8);
#endif
*/
			break;
		}
		default:
			break;      
	}
}

void CContextMenuAppUi::Hide()
{
	RWsSession& wsSession=CEikonEnv::Static()->WsSession();
	TInt id = CEikonEnv::Static()->RootWin().Identifier();
	if ( wsSession.GetFocusWindowGroup() == id ) {
		TApaTask task(wsSession);
		task.SetWgId(id);
		task.SendToBackground();
	}
}

void CContextMenuAppUi::Show()
{
	RWsSession& wsSession=CEikonEnv::Static()->WsSession();
	TInt id = CEikonEnv::Static()->RootWin().Identifier();
	if ( wsSession.GetFocusWindowGroup() != id ) {
		TApaTask task(wsSession);
		task.SetWgId(id);
		task.BringToForeground();
	}
}

void CContextMenuAppUi::ShowMenuL(TInt aCommandId )
{
	// 1. Create the appropriate menu array
	auto_ptr<CContextMenuArray> aContextMenuArray (new (ELeave) CContextMenuArray(AppContext()));
	aContextMenuArray->ConstructL();
	BuildMenuL(aContextMenuArray.get(), aCommandId);
	
	// 2. Create a listbox 
	auto_ptr<CContextMenuPopupMenuStyleListBox> list (new (ELeave) CContextMenuPopupMenuStyleListBox(aContextMenuArray.get(), (iPopupCount!=0)));

	// 3. Create a popup 
	CAknPopupList * popup = CAknPopupList::NewL(list.get(), R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow) ;
	CleanupStack::PushL(popup);

	// 4. construct the listbox
	list->ConstructL(popup, CEikListBox::ELeftDownInViewRect);
	list->CreateScrollBarFrameL(ETrue);
	list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	list->ItemDrawer()->FormattedCellData()->SetIconArray(iIconArray); // CContextMenuPopupMenuStyleListBox never owns the array
	list->Model()->SetItemTextArray(aContextMenuArray.get());
	list->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);

	// 5 . Execute the popup
	iPopupCount++;
	popup->ExecuteLD();
	CleanupStack::Pop(); // popup
	iPopupCount--;

	// . Hide the view if we were on the main menu
	if ( iPopupCount==0 ) HandleCommandL(EAknSoftkeyCancel);
}

void CContextMenuAppUi::BuildMenuL(CContextMenuArray * aContextMenuArray, TInt aCommandId)
{
	switch (aCommandId) {
		case EContextMenuFriends: {
			aContextMenuArray->AppendL(0, _L("Test1"), EFalse, CContextMenuArray::ESelected, ETest1On, ETest1Off);
			aContextMenuArray->AppendL(0, _L("Test2"), EFalse, CContextMenuArray::ESelected, ETest2On, ETest2Off);
			break;
		}

		case EContextMenuMain:
		default: {
			aContextMenuArray->AppendL(EContextMenuMyContext, _L("My Context"), EFalse, CContextMenuArray::ENone);
			aContextMenuArray->AppendL(EContextMenuSetDescription, _L("Set Description"), EFalse, CContextMenuArray::ENone);

			TBool presence_connected = EFalse; Settings().GetSettingL(SETTING_PRESENCE_ENABLE, presence_connected);
			CContextMenuArray::TSelectionState s;
			if (presence_connected) {
				s=CContextMenuArray::ESelected;
			} else {
				s=CContextMenuArray::ENotSelected;
			}
			aContextMenuArray->AppendL(0, _L("Presence Enabled"), EFalse, 
				CContextMenuArray::ENotSelected, EContextMenuTurnPresenceOn,EContextMenuTurnPresenceOff);

			aContextMenuArray->AppendL(EContextMenuFriends, _L("Vis. to Friends"), ETrue, 
				CContextMenuArray::ENotSelected, EContextMenuVisibleToFriendsOn,EContextMenuVisibleToFriendsOff);

			break;
		}
	}
}


