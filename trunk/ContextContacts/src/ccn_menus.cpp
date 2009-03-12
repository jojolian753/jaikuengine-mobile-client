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

#include "ccn_menus.h"

#include "ccn_connectioninfo.h"

#include "contextcontacts.hrh"
#include <contextcontacts.rsg>

#include "app_context.h"
#include "ccu_servermessage.h"
#include "cc_processmanagement.h"
#include "cl_settings.h"
#include "cn_networkerror.h"
#include "cu_common.h"
#include "connectioninit.h"
#include "ContextCommon.h"
#include "settings.h"
#include "raii_f32file.h"

#include <aknnotewrappers.h>
#include <bautils.h>

class CJaikuCommonMenusImpl : public CJaikuCommonMenus, public MContextBase
{
public:
	void ConstructL()
	{
		iServerMessage=CServerMessageListener::NewL();
	}

	~CJaikuCommonMenusImpl()
	{
		delete iServerMessage;
	}

	TBool HandleCommandL(TInt aCommand)
	{
		switch ( aCommand )
			{
			case EContextContactsCmdServerMessage:
				if (iServerMessage) iServerMessage->ShowMessage();
				return ETrue;

			case EContextContactsCmdStartWelcome:
#ifdef __WINS__
				if (0) {
					if (BaflUtils::FileExists(Fs(), _L("c:\\data\\full.txt"))) {
						Fs().Delete(_L("c:\\full.txt"));
					} else {
						RAFile f; f.ReplaceLA(Fs(), _L("c:\\data\\full.txt"), EFileWrite);
					}
					return ETrue;
				}
#endif
				ProcessManagement::StartApplicationL(KUidContextWelcome);
				return ETrue;

			case EcontextContactsCmdAppSuspendPresence:
				{
					CNetworkError::ConnectionRequestedL();
					Settings().WriteSettingL(SETTING_PRESENCE_ENABLE, 0);
					CConnectionOpener::ResetPermissionL(Settings());
					CNetworkError::ResetRequestedL();
					_LIT(message,"Disconnected.\nData is not transferred until you connect again.");
					CAknInformationNote* informationNote = new (ELeave) CAknInformationNote;
					informationNote->ExecuteLD(message);
				}
				return ETrue;
			case EcontextContactsCmdAppResumePresence:
				{
					TInt roaming=CConnectionOpener::ERoamingUnset;
					Settings().GetSettingL(SETTING_ALLOW_ROAMING, roaming);
					if (roaming==CConnectionOpener::ERoamingNo) {
						Settings().WriteSettingL(SETTING_ALLOW_ROAMING, CConnectionOpener::ERoamingUnset);
					}
					Settings().WriteSettingL(SETTING_PRESENCE_ENABLE, 1);
					CNetworkError::ConnectionRequestedL();
					_LIT(message,"Connecting to Jaiku.");
					CAknInformationNote* informationNote = new (ELeave) CAknInformationNote;
					informationNote->ExecuteLD(message);
				}
				return ETrue;
			case EContextContactsCmdDisableAutostart:
				ProcessManagement::SetAutoStartEnabledL(Fs(), DataDir()[0], EFalse);
				return ETrue;
			case EContextContactsCmdEnableAutostart:
				ProcessManagement::SetAutoStartEnabledL(Fs(), DataDir()[0], ETrue);
				return ETrue;
			}
		return EFalse;
	}

	TBool IsCommonMenu(TInt aResourceId)
	{
		switch ( aResourceId )
			{
			case R_JAIKU_CONNECTION_MENU:
			case R_JAIKU_GO_ONLINE_MENU:
			case R_JAIKU_SYSTEM_MESSAGES_MENU:
			case R_JAIKU_APPLICATION_MENU:
				return ETrue;
			default:
				return EFalse;
			}
	}
	
	void HideIfNotCommonMenuL( TInt aResourceId, CEikMenuPane* aMenuPane )
	{
		if ( IsCommonMenu(aResourceId) )
			return;
		
		if ( aMenuPane)
			{
				const TInt count = aMenuPane->NumberOfItemsInPane();
				for ( TInt i = 0; i < count; i++ )
					{
						CEikMenuPaneItem::SData& item = aMenuPane->ItemDataByIndexL(i);
						TInt cmd = item.iCommandId;
						SetItemDimmedIfExists(aMenuPane, cmd, ETrue);
					}
			}
	}
				

	void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane, TBool aShowOnlyCommonMenus)
	{
		CALLSTACKITEM_N(_CL("ConnectionMenu"), _CL("DynInitMenuPaneL"));
		
		if ( aShowOnlyCommonMenus )
			{
				HideIfNotCommonMenuL( aResourceId, aMenuPane );
			}

		if ( ! IsCommonMenu(aResourceId) )
			return;

		if ( aResourceId == R_JAIKU_CONNECTION_MENU ) 
			{
				TBool enabled=ETrue;
				Settings().GetSettingL(SETTING_PRESENCE_ENABLE, enabled);

				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdShowJabberError, ! ShouldShowConnectionErrorMenuItemL() );
				SetItemDimmedIfExists(aMenuPane, EcontextContactsCmdAppSuspendPresence, ! enabled);	
				//SetItemDimmedIfExists(aMenuPane, EcontextContactsCmdAppResumePresence, enabled);		
			}

		if ( aResourceId == R_JAIKU_GO_ONLINE_MENU ) 
			{
				TBool enabled=ETrue;
				Settings().GetSettingL(SETTING_PRESENCE_ENABLE, enabled);
				SetItemDimmedIfExists(aMenuPane, EcontextContactsCmdAppResumePresence, enabled);		
			}
				
		if ( aResourceId == R_JAIKU_SYSTEM_MESSAGES_MENU )
			{
				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdServerMessage, ! ( iServerMessage && iServerMessage->HasMessage() ) );
			}
		
		if ( aResourceId == R_JAIKU_APPLICATION_MENU )
			{
				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdSettings, EFalse);
				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdShutdown, EFalse);
			}
	}

	TBool ShouldShowConnectionErrorMenuItemL()
	{
		// Show connection error menu item 
		// 1)    a) Connection status is an explicit error
		//    or b) Connection status is disconnected and Jaiku is Online mode
		// and 
		// 2) Connection error exists
		TBool enabled = EFalse;
		Settings().GetSettingL(SETTING_PRESENCE_ENABLE, enabled);
		if ( enabled )
			{
				ContextServer::TContextServerStatus status = ConnectionInfo::GetStatusL( *(BBSession()) );
				
				if ( status == ContextServer::EConnected )
					return EFalse;
				
				auto_ptr<CBBErrorInfo> error( ConnectionInfo::GetErrorL( *(BBSession()) ) );
				if ( error.get() )
					return ETrue;
				else
					return EFalse;
			}
		return EFalse;
	}

private:
	CServerMessageListener* iServerMessage;

};


CJaikuCommonMenus* CJaikuCommonMenus::NewL()
{
	auto_ptr<CJaikuCommonMenusImpl> self( new (ELeave) CJaikuCommonMenusImpl );
	self->ConstructL();
	return self.release();
}
