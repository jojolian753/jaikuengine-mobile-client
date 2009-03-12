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

#include "ccn_richpresenceview.h"

#include "ccu_presencelist2.h"

#include "ccu_activestate.h"
#include "ccu_contactdataproviders.h"
#include "ccu_contact.h"
#include "ccu_contactview_base.h"
#include "ccu_mainbgcontainer.h"
#include "ccu_richpresencelistbox.h" 
#include "ccu_userpics.h"
#include "ccu_uidelegates.h"

#include "ccu_utils.h"
#include <contextcontacts.rsg>
#include <contextcontactsui.mbg>

#include "contextcontacts.hrh"

#include "app_context.h"
#include "app_context_impl.h"
#include "break.h"
#include "cl_settings.h"
#include "cu_common.h"
#include "ContextNotifyClientSession.h"
#include "juik_icons.h"
#include "juik_layout.h"
#include "juik_listbox.h"
#include "juik_keycodes.h"
#include "juik_scrolllist.h"
#include "juik_subcellrenderer.h"
#include "jaiku_layoutids.hrh"
#include "reporting.h"
#include "settings.h"

#include <akncontext.h>
#include <akniconarray.h>
#include <aknmessagequerydialog.h> 
#include <akntitle.h>
#include <aknviewappui.h>
#include <gulicon.h>


enum 
	{
		ELocationIndex = 1
	};

class CRichPresenceContainer : public CCoeControl, public CSoftScrollList::MObserver, public MContextBase
{
public:
	static CRichPresenceContainer* NewL(CRichPresenceView *aView, CCoeControl* aParent, const TUiDelegates& aDelegates)
	{
		CALLSTACKITEMSTATIC_N(_CL("CRichPresenceContainer"), _CL(""));
		auto_ptr<CRichPresenceContainer> self( new (ELeave) CRichPresenceContainer(aView, aDelegates) );
		self->ConstructL(aParent);
		return self.release();
	}


	CRichPresenceContainer(CRichPresenceView *aView, const TUiDelegates& aDelegates) : 
		iView( aView), iDelegates(aDelegates) {}
	
	CActiveState& ActiveState() { return *(iDelegates.iActiveState); }
	CUserPics& UserPics() { return *(iDelegates.iUserPics); }
	CJabberData& JabberData() { return *(iDelegates.iJabberData); }
	CThemeColors& ThemeColors() { return *(iDelegates.iThemeColors); }
	
	void ConstructL(CCoeControl* aParent)
	{
		CALLSTACKITEM_N(_CL("CRichPresenceContainer"), _CL(""));
		SetContainerWindowL( *aParent );

		// Providers 
		iListController = CPresenceList::NewL( *this, iDelegates );
		iListController->List()->AddObserverL( *this );		
		ActivateL();
	}
		


	TInt CountComponentControls() const
	{
	    CALLSTACKITEM_N(_CL("CRichPresenceContainer"), _CL("CountComponentControl"));
		return 1;
	}
	
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CRichPresenceContainer"), _CL("ComponentControl"));
		return iListController->List(); //iListController->GetListBoxL();
	}
	
	
	~CRichPresenceContainer() 
	{
		if ( iListController ) 
			iListController->List()->RemoveObserver( *this );
		delete iListController;
	}

	void SizeChanged()
	{
		TRect rect = Rect();
		iListController->List()->SetRect( rect );
	}

	virtual void HandleListEventL(CSoftScrollList::TEvent aEvent, TInt aIndex) 
	{
		if ( aEvent == CSoftScrollList::EItemClicked )
			{
				contact* c = iDelegates.iActiveState->ActiveContact().GetL();
				TBool isMe = c && c->is_myself;
				if ( isMe ) 
					{
						CPresenceList::TItemId id = iListController->ItemId( aIndex );
						if ( id == CPresenceList::ELocationItem )
							iView->HandleCommandL(EContextContactsCmdAppNameLocation);
						else
							iView->HandleCommandL( EContextContactsCmdSharingSettings );
					}
			}
	}

	
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{
		if ( aType == EEventKey )
			{   				
				if (aKeyEvent.iCode==KEY_CALL) 
					{
						iView->HandleCommandL(EContextContactsCmdCall);
						return EKeyWasConsumed;
					}
			}
		
		return iListController->List()->OfferKeyEventL( aKeyEvent, aType ); 
	}

	CPresenceList::TItemId CurrentItemId() const
	{
		return iListController->CurrentItemId();
	}
	
private:
	CRichPresenceView* iView;
	TUiDelegates iDelegates;
	// owned
	CPresenceList* iListController;

	// Temporary buffers for message note
	TBuf<1000> iBody;
	TBuf<100> iHeader;
	
};


class CRichPresenceViewImpl :
	public CRichPresenceView, 
	public MContactViewBase
{
public:
	CRichPresenceViewImpl() : MContactViewBase(*static_cast<CJaikuViewBase*>(this))
	{
	}
	
	
	~CRichPresenceViewImpl() 
	{
		CC_TRAPD(err, ReleaseViewImpl());
		if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}

	
	void ReleaseViewImpl()
	{
		CALLSTACKITEM_N(_CL("CRichPresenceViewImpl"), _CL("ReleaseCRichPresenceViewImpl"));
		if (iResource) iEikonEnv->DeleteResourceFile(iResource);
		delete iBgContainer;
		delete iPresenceContainer;
	}


	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CRichPresenceViewImpl"), _CL("ConstructL"));
		iResource = LoadSystemResourceL( iEikonEnv, _L("contextcontactsui") );
		BaseConstructL( R_RICHPRESENCE_VIEW );
	}

	TUid Id() const {
		return KRichPresenceView;
	}


	void RealDoActivateL(const TVwsViewId& aPrevViewId,TUid /*aCustomMessageId*/,const TDesC8& /*aCustomMessage*/)
	{
 		UpdateStatusPaneL();
 		CreateListBoxL();
		ActiveState().ActiveContact().AddListenerL( *this );
	}
			
	void RealDoDeactivateL()
	{
		ActiveState().ActiveContact().RemoveListenerL( *this );
		RemoveContainer();
	}
	
	
	void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
	{
		CALLSTACKITEM_N(_CL("CRichPresenceViewImpl"), _CL("DynInitMenuPaneL"));
		
		TBool failedToConstruct = ! iPresenceContainer;
		CommonMenus().DynInitMenuPaneL(aResourceId, aMenuPane, failedToConstruct);
		if ( failedToConstruct )
			return;
		
		if ( aResourceId == R_RICH_PRESENCE_VIEW_MENUPANE )
			{
				
				contact* c = ActiveState().ActiveContact().GetL();
				TBool isDummy = JabberData().IsDummyContactId( c->id );
				TBool editableJabber =  (! isDummy) && (! c->is_myself);
				
				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdShowJabber, editableJabber );
				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdEditJabber, ! editableJabber );
				
			
				if ( c->is_myself )
					{
						CPresenceList::TItemId focus = iPresenceContainer->CurrentItemId();
						
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdAppNameLocation, 
											  focus != CPresenceList::ELocationItem );

						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdBtNeighbourhoodSubmenu, 
											  focus != CPresenceList::ENearbyItem );

						TBool sharing =
							focus == CPresenceList::ENearbyItem || 
							focus == CPresenceList::ECalendarItem;
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdSharingSettings, !sharing );
					}
				else
					{
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdAppNameLocation, ETrue );
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdBtNeighbourhoodSubmenu, ETrue );
						SetItemDimmedIfExists(aMenuPane, EContextContactsCmdSharingSettings, ETrue );
					}
			}
	}

	void HandleCalendarSharingL() 
	{
		TInt sharing = SHARE_CALENDAR_FULL;
		Settings().GetSettingL(SETTING_CALENDAR_SHARING, sharing);
		TInt oldSharing = sharing;

		CAknListQueryDialog* dlg = new( ELeave ) CAknListQueryDialog( &sharing );
		if ( dlg->ExecuteLD(R_JAIKU_CALENDAR_SHARING_QUERY) )
			{
				if ( sharing != oldSharing )
					Settings().WriteSettingL(SETTING_CALENDAR_SHARING, sharing);
			} 
 	}

	void HandleBluetoothSharingL() 
	{
		TInt interval = 0;
		Settings().GetSettingL(SETTING_BT_SCAN_INTERVAL, interval);

		TInt selection = interval == 0 ? 1 : 0;
		TInt old = selection;
		
		CAknListQueryDialog* dlg = new( ELeave ) CAknListQueryDialog( &selection );
		if ( dlg->ExecuteLD(R_JAIKU_BLUETOOTH_SHARING_QUERY) )
			{
				if ( old != selection )
					{
						interval = selection == 0 ? 300 : 0;
						Settings().WriteSettingL(SETTING_BT_SCAN_INTERVAL, interval);
					}
			} 
 	}

	void HandleCommandL(TInt aCommand)
	{
		CALLSTACKITEM_N(_CL("CRichPresenceViewImpl"), _CL("HandleCommandl"));
		switch ( aCommand )
			{
			case EAknSoftkeyBack:
				ActivateParentViewL();
				break;
			case EContextContactsCmdSharingSettings:
				{
					switch ( iPresenceContainer->CurrentItemId() )
						{
						case CPresenceList::ECalendarItem: HandleCalendarSharingL(); break;
						case CPresenceList::ENearbyItem:   HandleBluetoothSharingL(); break;
						}
					break;
				}
			default:
				AppUi()->HandleCommandL( aCommand );
				break;
			}
	}



	void RemoveContainer()
	{

		if ( iBgContainer )
			{
				AppUi()->RemoveFromStack( iBgContainer );
				delete iBgContainer;
				iBgContainer = NULL;
				SetBaseContainer( NULL );
			}
		if ( iPresenceContainer )
			{
				delete iPresenceContainer;
				iPresenceContainer = NULL;
			}
	}

	void CreateListBoxL()
	{
		CALLSTACKITEM_N(_CL("CRichPresenceViewImpl"), _CL("CreateListBoxL"));
		RemoveContainer();
		iBgContainer = CMainBgContainer::NewL( this, ClientRect(), ThemeColors(), ProgressBarModel() );
		SetBaseContainer( iBgContainer );

		TUiDelegates delegates = UiDelegates();
		iPresenceContainer = CRichPresenceContainer::NewL( this, iBgContainer, delegates );
		iBgContainer->SetContentL( iPresenceContainer );

		iBgContainer->MakeVisible(ETrue);
		iBgContainer->ActivateL();
		AppUi()->AddToStackL( *this, iBgContainer );

		iBgContainer->SetRect( ClientRect() );
	}

	
	void HandleResourceChange( TInt aType )
	{
	if ( aType == KEikDynamicLayoutVariantSwitch )
		{
			if ( iBgContainer )
				{
					TRect r = ClientRect();
					iBgContainer->SetRect( r );
				}
		}
	}



private:
	CMainBgContainer* iBgContainer;
	CRichPresenceContainer* iPresenceContainer;
	TInt iResource;
};


EXPORT_C CRichPresenceView* CRichPresenceView::NewL()
{
	auto_ptr<CRichPresenceViewImpl> self(new (ELeave) CRichPresenceViewImpl());
	//self->ConstructL();
	return self.release();
}
