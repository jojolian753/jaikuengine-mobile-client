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

#include   "SettingsView.h"

#include "cu_autosettings.h"
#include "cu_common.h"
#include "ccu_utils.h"
#include "contextui.hrh"
#include  <contextui.rsg>

#include "break.h"

#include "contextvariant.hrh"
#include "app_context_impl.h"
#include "reporting.h"

#include  <aknviewappui.h>
#include  <aknsettingitemlist.h>


class CCLSettingListImpl: public CCLSettingList, public MContextBase 
{
public:
	CCLSettingListImpl(const TInt  * EnabledSettings,
					   TBool aOnlyContextLogPublishing): CCLSettingList(), 
														 iEnabledSettings(EnabledSettings), 
														 iOnlyContextLogPublishing(aOnlyContextLogPublishing) {}
	~CCLSettingListImpl();

	virtual void UpdateSettingListL();
	void PreConstructL();
private:
	void SetListData();
	void UpdateSettings();
	

private: // from CAknSettingItemList	
	CAknSettingItem* CreateSettingItemL( TInt identifier );
	void SetCurrentItemIndex(TInt aIndex);
	virtual void EditItemL(TInt aIndex, TBool aCalledFromMenu);

	void MyLoadSettingsL();
	void StoreSettingsL();
	
private:
	const TInt  * iEnabledSettings;
	TBool	iOnlyContextLogPublishing;

	CAutoSettings* iAutoSettings;
};


void CCLSettingListImpl::PreConstructL() 
{
	iAutoSettings = CAutoSettings::NewL( iOnlyContextLogPublishing );
}

void CCLSettingListImpl::UpdateSettingListL() 
{
	iAutoSettings->UpdateSettingItemsL();
}

CCLSettingListImpl::~CCLSettingListImpl()
{
	delete iAutoSettings;
}

TBool Enabled(const TInt* aEnabledSettings, TInt aIdentifier)
{
	for (const TInt* sett=aEnabledSettings; (*sett)>=0; sett++) {
		if (aIdentifier==*sett) return ETrue;
	}
	return EFalse;
}


CAknSettingItem* CCLSettingListImpl::CreateSettingItemL( TInt identifier )
{
	CALLSTACKITEM_N(_CL("CCLSettingListImpl"), _CL("CreateSettingItemL"));
	CAknSettingItem * settingItem = iAutoSettings->CreateSettingItemL( identifier );
 	if (! Enabled(iEnabledSettings, identifier) ) settingItem->SetHidden(ETrue);
 	return settingItem;
}


/**
 * Password setting item is not updated after edits, this is bug in platform
 * See:KIS000542
 * http://forum.nokia.com/document/Forum_Nokia_Technical_Library_v1_35/contents/FNTL/Password_text_not_updated_automatically_in_setting_item_list.htm
 */
void CCLSettingListImpl::EditItemL(TInt aIndex, TBool aCalledFromMenu)
{
	CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);
	SettingItemArray()->At(aIndex)->UpdateListBoxTextL();
}



void CCLSettingListImpl::MyLoadSettingsL() 
{
	for (int i=0; i<SETTINGS_COUNT; i++) {
		iAutoSettings->LoadSettingL(i);
	}
	//CCLSettingList::LoadSettingsL();
}


void CCLSettingListImpl::StoreSettingsL() 
{
	CCLSettingList::StoreSettingsL();
	for (int i=0; i<SETTINGS_COUNT; i++) {
		iAutoSettings->StoreSettingL( i );
	}
}

CCLSettingList* CCLSettingList::NewL(const TInt * aEnabledSettings,
									 TBool aOnlyContextLogPublishing,
									 MObjectProvider *aParent,
									 TInt aResourceId)
{
	auto_ptr<CCLSettingListImpl> self( new (ELeave) CCLSettingListImpl(aEnabledSettings, aOnlyContextLogPublishing ) );
	self->SetMopParent(aParent);
	self->PreConstructL();
	self->ConstructFromResourceL( aResourceId );
	return self.release();
}

// ================= MEMBER FUNCTIONS =======================

EXPORT_C CSettingsView* CSettingsView::NewL(TUid aViewId, MApp_context& Context, const TInt * EnabledSettings,
					    TBool aOnlyContextLogPublishing)
{
	CALLSTACKITEMSTATIC_N(_CL("CSettingsView"), _CL("NewL"));

	auto_ptr<CSettingsView> ret(new (ELeave) CSettingsView(aViewId, Context, EnabledSettings,
		aOnlyContextLogPublishing));
	ret->ConstructL();
	return ret.release();
}

CSettingsView::CSettingsView(TUid aViewId, MApp_context& Context, 
			     const TInt * EnabledSettings,
			     TBool aOnlyContextLogPublishing) : MContextBase(Context), iViewId(aViewId), 
			     iEnabledSettings(EnabledSettings), iOnlyContextLogPublishing(aOnlyContextLogPublishing) { }

// ---------------------------------------------------------
// CSettingsView::ConstructL(const TRect& aRect)
// EPOC two-phased constructor
// ---------------------------------------------------------
//
void CSettingsView::ConstructL()
{
	CALLSTACKITEM_N(_CL("CSettingsView"), _CL("ConstructL"));
	iResource=LoadSystemResourceL(iEikonEnv, _L("contextui"));

	BaseConstructL( R_SETTINGS_VIEW );
}

EXPORT_C CSettingsView::~CSettingsView()
{
	//CALLSTACKITEM_N(_CL("CSettingsView"), _CL("~CSettingsView"));

	if ( iListBox ) {
		AppUi()->RemoveFromViewStack( *this, iListBox );
        }
	delete iListBox;
	if (iResource) iEikonEnv->DeleteResourceFile(iResource);
}

TUid CSettingsView::Id() const {
	return iViewId;
}

EXPORT_C void CSettingsView::SetPreviousLocalViewId(TUid aViewId) {
	iPreviousLocalViewId=aViewId;
}

void CSettingsView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM_N(_CL("CSettingsView"), _CL("HandleCommandL"));

	switch ( aCommand )
        {
		case EContextUICmdSettingsChange:
		{
			TInt visibleIx = iListBox->ListBox()->CurrentItemIndex();
			if (visibleIx >= 0)
				{
					TInt itemIx = iListBox->SettingItemArray()->ItemIndexFromVisibleIndex (visibleIx);
					iListBox->EditItemL( itemIx , ETrue );
				}
		}
		break;
        case EAknSoftkeyOk:
        case EAknSoftkeyDone:
		case EContextUICmdSettingsSave:
		{
			iListBox->StoreSettingsL();
		}
        default:
		if (iPreviousLocalViewId==TUid::Uid(0)) {
			AppUi()->ActivateViewL(iPreviousViewId);
		} else {
			AppUi()->ActivateLocalViewL(iPreviousLocalViewId);
		}
		break;
        }
}

void CSettingsView::HandleClientRectChange()
{
	CALLSTACKITEM_N(_CL("CSettingsView"), _CL("HandleClientRectChange"));
	if ( iListBox ) {
		iListBox->SetRect( ClientRect() );
        }
}

EXPORT_C void CSettingsView::HandleResourceChange( TInt aType )
{
	if ( aType == KEikDynamicLayoutVariantSwitch )
		{
		  HandleClientRectChange();
		}
}

void CSettingsView::DoActivateL(
				const TVwsViewId& aPrevViewId,TUid /*aCustomMessageId*/,
				const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CSettingsView"), _CL("DoActivateL"));
	iIsActivated=ETrue;
	MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);
#ifdef __WINS__
	TInt dummy;
	TBreakItem b(GetContext(), dummy);
#endif
	iPreviousViewId = aPrevViewId;
	if (!iListBox) {
		iListBox = CCLSettingList::NewL(iEnabledSettings, iOnlyContextLogPublishing, this, R_CL_SETTINGS_LIST);
		iListBox->MyLoadSettingsL();
		iListBox->UpdateSettingListL();
		
		AppUi()->AddToStackL( *this, iListBox );
		iListBox->MakeVisible(ETrue);
		iListBox->SetRect(ClientRect());

		CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
		CAknNavigationControlContainer *np = (CAknNavigationControlContainer *)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi)); 
		np->PushDefaultL();

		iListBox->ActivateL();
		iListBox->DrawNow();
        } 
        
   StatusPaneUtils::SetContextPaneIconToDefaultL();
   StatusPaneUtils::SetTitlePaneTextToDefaultL();
}


void CSettingsView::DoDeactivate()
{
	if (!iIsActivated) {
		iIsActivated=EFalse;
		MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
		if (rep) rep->SetInHandlableEvent(ETrue);
	}
	CALLSTACKITEM_N(_CL("CSettingsView"), _CL("DoDeactivate"));
	CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
	CAknNavigationControlContainer* np = (CAknNavigationControlContainer *)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	np->Pop(NULL);
	
	if ( iListBox )
        {
		AppUi()->RemoveFromViewStack( *this, iListBox );
        }
	
	delete iListBox;
	iListBox = NULL;
}



