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

#include "ccn_settingsui.h"

#include "ccn_about.h"
#include "ccn_settingitemid.hrh"
#include <contextcontacts.rsg>

#include "app_context.h"
#include "break.h"
#include "cc_processmanagement.h"
#include "cl_settings.h"
#include "context_uids.h"
#include "contextvariant.hrh"
#include <contextui.rsg>
#include "cu_dynamicsettings.h"
#include "cu_launchersettingitem.h"
#include "cu_autosettings.h"
#include "cu_resourcereaders.h"
#include "jaiku_viewids.hrh"
#include "raii_array.h"
#include "symbian_auto_ptr.h"

#include <barsread.h>

const TUid KJaikuSettingsViewId2 = {JAIKU_SETTINGS_VIEWID};


class CSetupWizardSettingItem : public CLauncherSettingItem, public MContextBase
{
public:
	CSetupWizardSettingItem(TInt aId) : CLauncherSettingItem(aId, KNullDesC) {}
	
	void EditItemL( TBool /*aCalledFromMenu*/ )
    {
		ProcessManagement::StartApplicationL(KUidContextWelcome);
	}	
};

class CNetworkTrafficSettingItem : public CAknTextSettingItem, public MContextBase
{
public:
	CNetworkTrafficSettingItem(CAboutUi& aAboutUi, TInt aId) : 
		CAknTextSettingItem(aId, iNetworkTraffic), iAboutUi(aAboutUi) {}
	
	void LoadL() 
	{
		iNetworkTraffic.Zero();
		iAboutUi.AppendNetworkTraffic( iNetworkTraffic );
		CAknTextSettingItem::LoadL();
	}
	
	void EditItemL( TBool /*aCalledFromMenu*/ )
    {
		iAboutUi.ShowNoteL();
	}	
	
	TBuf<100> iNetworkTraffic;
	CAboutUi& iAboutUi;
};


class CAutoStartSettingItem : public CAknBinaryPopupSettingItem, public MContextBase
{
public:
	CAutoStartSettingItem(TInt aId) : iEnabled( EFalse ), CAknBinaryPopupSettingItem( aId,  iEnabled ) {}
	
	void LoadL() 
	{
		iEnabled = ProcessManagement::IsAutoStartEnabled(Fs(), DataDir()[0]);
		CAknBinaryPopupSettingItem::LoadL();
	}
	
	void StoreL()
	{
		CAknBinaryPopupSettingItem::StoreL();
		TBool prevValue = ProcessManagement::IsAutoStartEnabled(Fs(), DataDir()[0]);
		if (prevValue != iEnabled)
			{
				ProcessManagement::SetAutoStartEnabledL(Fs(), DataDir()[0], iEnabled);
			}		
	}


	TBool iEnabled;
};



// class CNetworkTrafficSettingItem : public CAknTextSettingItem, public MContextBase
// {
// public:
// 	CNetworkTrafficSettingItem(CAboutUi& aAboutUi, TInt aId) : 
// 		CAknTextSettingItem(aId, iNetworkTraffic), iAboutUi(aAboutUi) {}
	
// 	void LoadL() 
// 	{
// 		iNetworkTraffic.Zero();
// 		iAboutUi.AppendNetworkTraffic( iNetworkTraffic );
// 		CAknTextSettingItem::LoadL();
// 	}
	
// 	void EditItemL( TBool /*aCalledFromMenu*/ )
//     {
// 		iAboutUi.ShowNoteL();
// 	}	
	
// 	TBuf<100> iNetworkTraffic;
// 	CAboutUi& iAboutUi;
// };



class CSettingsUiImpl : 
	public CSettingsUi, 
	public MContextBase,
	public CDynamicSettingsView::MSettingFactory
{
public:
	CSettingsUiImpl(CAboutUi& aAboutUi) : iAboutUi(aAboutUi) {}
	
	virtual ~CSettingsUiImpl() 
	{
		delete iAutoSettings;
	}

	void ConstructL()
	{
		iAutoSettings = CAutoSettings::NewL( ETrue );
	}

#define ITEMCOUNT 9
	
	CDynamicSettingsView* CreateSettingsViewL()
	{
#ifdef __DEV__ 
		const TInt KItemCount(ITEMCOUNT + 2);
#else
		const TInt KItemCount(ITEMCOUNT + 0);
#endif 
		
		CDynamicSettingsView::TItemDef defs[KItemCount] = {
			{ this,    EJaikuSettingItemId_SetupWizard },
 			{ iAutoSettings, SETTING_JABBER_NICK },
 			{ iAutoSettings, SETTING_JABBER_PASS },
#ifdef __DEV__ 
 			{ iAutoSettings, SETTING_JABBER_ADDR },
 			{ iAutoSettings, SETTING_JABBER_PORT },
#endif
 			{ iAutoSettings, SETTING_IP_AP },
 			{ iAutoSettings, SETTING_CONNECTIVITY_MODEL },
  			{ iAutoSettings, SETTING_DOWNLOAD_IMAGES_YESNO },
 			{ iAutoSettings, SETTING_ALLOW_NETWORK_ACCESS },
  			{ this,    EJaikuSettingItemId_AutoStart },
  			{ this,    EJaikuSettingItemId_NetworkTraffic }
		};
			
		auto_ptr<CDynamicSettingsView> view( CDynamicSettingsView::NewL(KJaikuSettingsViewId2, defs, KItemCount) );
		return view.release();
	}
	
	virtual class CAknSettingItem* CreateItemL(TInt aId, TInt aOrdinal) 
	{
		TInt resource = 0;
		auto_ptr<CAknSettingItem> item(NULL);
		switch ( aId )
			{
			case EJaikuSettingItemId_SetupWizard: 
				item.reset( new (ELeave) CSetupWizardSettingItem( aId ) );
				resource = R_JAIKU_SETUPWIZARD_SETTING_ITEM;
				break;
			case EJaikuSettingItemId_AutoStart:
				item.reset( new (ELeave) CAutoStartSettingItem( aId ) );
				resource = R_JAIKU_AUTOSTART_SETTING_ITEM;
				break;
			case EJaikuSettingItemId_NetworkTraffic:   
				item.reset( new (ELeave) CNetworkTrafficSettingItem( iAboutUi, aId ) );
				resource = R_JAIKU_NETWORKTRAFFIC_SETTING_ITEM;
				break;
				
			default:
				Bug( _L("Support for setting item not implemented") ).Raise();
			}
		
		ConstructItemL( aOrdinal, *item, resource );
		return item.release();
	}

	void ConstructItemL(TInt aOrdinal, CAknSettingItem& aItem, TInt aResourceId )
	{
		auto_ptr<ResourceReaders::CSettingItem> r( new (ELeave) ResourceReaders::CSettingItem);
		r->ReadResourceL( aResourceId );
		
		aItem.SetEmptyItemTextL( r->EmptyItemText() );
		aItem.SetCompulsoryIndTextL( r->CompulsoryText() );
		aItem.ConstructL( EFalse, aOrdinal, r->Name(), NULL, r->iSettingPageResource, r->iType, r->iSettingEditorResource, r->iAssociatedResource );
	}

private:
	CAboutUi& iAboutUi;
	CAutoSettings* iAutoSettings;
};


CSettingsUi* CSettingsUi::NewL(CAboutUi& aAboutUi)
{
	auto_ptr<CSettingsUiImpl> self( new (ELeave) CSettingsUiImpl(aAboutUi) );
	self->ConstructL();
	return self.release();
}
	
