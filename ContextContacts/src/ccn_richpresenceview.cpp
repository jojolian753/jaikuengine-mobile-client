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

#include "ccu_activestate.h"
#include "ccu_contactdataproviders.h"
#include "ccu_contact.h"
#include "ccu_contactview_base.h"
#include "ccu_mainbgcontainer.h"
#include "ccu_richpresencelistbox.h" 
#include "ccu_userpics.h"
#include "ccu_utils.h"

#include <contextcontacts.rsg>
#include "contextcontacts.hrh"

#include <contextcontactsui.mbg>


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


class CRichPresenceContainer : public CCoeControl, public MEikListBoxObserver, public MContextBase
{
public:
	static CRichPresenceContainer* NewL(CRichPresenceView *aView, CCoeControl* aParent, CActiveContact& aActiveContact, CJabberData& aJabberData, CUserPics& aUserPics)
	{
		CALLSTACKITEMSTATIC_N(_CL("CRichPresenceContainer"), _CL(""));
		auto_ptr<CRichPresenceContainer> self( new (ELeave) CRichPresenceContainer(aView, aActiveContact, aJabberData, aUserPics) );
		self->ConstructL(aParent);
		return self.release();
	}


	CRichPresenceContainer(CRichPresenceView *aView, CActiveContact& aActiveContact, CJabberData& aJabberData, CUserPics& aUserPics) : 
		iView( aView), iActiveContact(aActiveContact), iJabberData(aJabberData), iUserPics(aUserPics) {}



	void ConstructL(CCoeControl* aParent)
	{
		CALLSTACKITEM_N(_CL("CRichPresenceContainer"), _CL(""));
		SetContainerWindowL( *aParent );

		// Providers 
		iListController = CRichPresenceListController::NewL(this, iActiveContact);
	    iListController->GetListBoxL()->SetListBoxObserver(this);

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
		return iListController->GetListBoxL();
	}
	

	void HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType)
	{
		CALLSTACKITEM_N(_CL("CRichPresenceContainer"), _CL("HandleListBoxEventL"));
		if(aListBox == iListController->GetListBoxL())
			{
				switch(aEventType)
					{
					case EEventEnterKeyPressed:
						{
							contact* c = iActiveContact.GetL();
							TBool isMe = c && c->is_myself;
							switch ( iListController->GetListBoxL()->CurrentItemIndex() ) 
								{
								case CRichPresenceListController::EPresenceLineRow:
									if ( isMe ) 
											iView->HandleCommandL(EContextContactsCmdSetUserDesc);
									else
											ShowPresenceLineDetailsL();
									break;
								case CRichPresenceListController::EUserActivityRow:
									ShowUserActivityDetailsL();
									break;
								case CRichPresenceListController::ELocationRow:
									if ( isMe ) 
										iView->HandleCommandL(EContextContactsCmdAppNameLocation);
									else
										ShowLocationDetailsL();
										
									break;
								case CRichPresenceListController::ENearbyRow:
									ShowNearbyDetailsL();
									break;
								case CRichPresenceListController::ECalendarRow:
									ShowCalendarDetailsL();
									break;
								default:
									break;
								}
						}
					}
			}
	}
	
	~CRichPresenceContainer() 
	{
		delete iListController;
	}

	void SizeChanged()
	{
		TRect rect = Rect();
		TJuikLayoutItem l = TJuikLayoutItem(rect).Combine( Layout().GetLayoutItemL(LG_richpresence_list, 
																				   LI_richpresence_list__listbox));
		iListController->GetListBoxL()->AdjustAndSetRect( l.Rect() );
	}


	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{
		if ( aType == EEventKey )
			{   
				// FIXME: View swiching (CViewNavigation) framework should handle this, but 
				// first contact details view has to be able to cope with non-existent contacts
// 				if ( aKeyEvent.iCode==EKeyLeftArrow )
// 					{
// 						return EKeyWasNotConsumed;
// 					}
// 				if ( aKeyEvent.iCode==EKeyRightArrow )
// 					{
// 						iView->HandleCommandL(EContextContactsCmdDisplayContactDetails);
// 						return EKeyWasConsumed;
// 					}
				
				if (aKeyEvent.iCode==KEY_CALL) 
					{
						iView->HandleCommandL(EContextContactsCmdCall);
						return EKeyWasConsumed;
					}
			}
		
		return iListController->GetListBoxL()->OfferKeyEventL(aKeyEvent, aType);
	}


	void ShowPresenceLineDetailsL()
	{
		auto_ptr< CArrayPtr<MContactDataProvider> > providers( new (ELeave) CArrayPtrFlat<MContactDataProvider>(2) );		
		providers->AppendL( ContactDataProviders::PresenceLineL() );
		providers->AppendL( ContactDataProviders::PresenceLineTStampL() );
		
		_LIT(KHeader, "Presence line");
		ShowDetailNoteL(KHeader, *providers);
		providers->ResetAndDestroy(); // FIXME, unsafe		
		
	}


	void ShowUserActivityDetailsL()
	{
		auto_ptr< CArrayPtr<MContactDataProvider> > providers( new (ELeave) CArrayPtrFlat<MContactDataProvider>(2) );		
		providers->AppendL( ContactDataProviders::ProfileNameL() );
		providers->AppendL( ContactDataProviders::UserActivityL() );
		_LIT(KHeader, "User activity");
		ShowDetailNoteL(KHeader, *providers);
		providers->ResetAndDestroy(); // FIXME, unsafe				
	}


	void ShowLocationDetailsL()
	{
		auto_ptr< CArrayPtr<MContactDataProvider> > providers( new (ELeave) CArrayPtrFlat<MContactDataProvider>(2) );		
		providers->AppendL( ContactDataProviders::LocationL() );
		providers->AppendL( ContactDataProviders::LocationTStampL() );
		
		_LIT(KHeader, "Location");
		ShowDetailNoteL(KHeader, *providers);
		providers->ResetAndDestroy(); // FIXME, unsafe				
	}

	void ShowNearbyDetailsL()
	{
		auto_ptr< CArrayPtr<MContactDataProvider> > providers( new (ELeave) CArrayPtrFlat<MContactDataProvider>(2) );		
		providers->AppendL( ContactDataProviders::NearbyPeopleL() );		
		_LIT(KHeader, "Nearby");
		ShowDetailNoteL(KHeader, *providers);
		providers->ResetAndDestroy(); // FIXME, unsafe				
	}


	void ShowCalendarDetailsL()
	{
		auto_ptr< CArrayPtr<MContactDataProvider> > providers( new (ELeave) CArrayPtrFlat<MContactDataProvider>(2) );		
		providers->AppendL( ContactDataProviders::CalendarTitleL() );
		providers->AppendL( ContactDataProviders::CalendarDateTimeL() );
		
		_LIT(KHeader, "Calendar");
		ShowDetailNoteL(KHeader, *providers);
		providers->ResetAndDestroy(); // FIXME, unsafe		
	}
	
	
	void ShowDetailNoteL(const TDesC& aHeader, CArrayPtr<MContactDataProvider>& aProviders)
	{
		iHeader = aHeader;
		iBody.Zero();
		contact* c = iActiveContact.GetL();		
		if ( c )
			{
				for (TInt i=0; i < aProviders.Count(); i++)
					{				
						aProviders[i]->SubcellDataL( c, iBody );
						iBody.Append( _L("\n") );				
					}
			}
		ShowDetailNoteL( iHeader, iBody );
	}
	
	void ShowDetailNoteL(TDesC& aHeader, TDesC& aBody)
	{
		auto_ptr<CAknMessageQueryDialog> note( CAknMessageQueryDialog::NewL(aBody) );
		note->SetHeaderTextL(aHeader);
		note->ExecuteLD(R_FULLSTRING_DIALOG);
		note.release();
	}


	
private:
	CRichPresenceView* iView;
	CActiveContact& iActiveContact;	
	CJabberData& iJabberData;
	CUserPics& iUserPics;

	// owned
	CRichPresenceListController* iListController; 

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

		if ( aResourceId == R_RICH_PRESENCE_VIEW_MENUPANE )
			{

				contact* c = ActiveState().ActiveContact().GetL();
				TBool isDummy = JabberData().IsDummyContactId( c->id );
				TBool editableJabber =  (! isDummy) && (! c->is_myself);
				
				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdShowJabber, editableJabber );
				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdEditJabber, ! editableJabber );
						
				TBool enabled=ETrue;
				Settings().GetSettingL(SETTING_PRESENCE_ENABLE, enabled);
				SetItemDimmedIfExists(aMenuPane, EcontextContactsCmdAppSuspendPresence, ! enabled);	
				SetItemDimmedIfExists(aMenuPane, EcontextContactsCmdAppResumePresence, enabled);
			}
		else if ( aResourceId == R_AUTHORVIEWS_SUBMENU )
			{
				SetItemDimmedIfExists(aMenuPane, EContextContactsCmdDisplayRichPresence, ETrue);
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
		iBgContainer = CMainBgContainer::NewL( this, ClientRect(), ThemeColors() );
		SetBaseContainer( iBgContainer );
		iPresenceContainer = CRichPresenceContainer::NewL( this, iBgContainer, ActiveState().ActiveContact(), JabberData(), UserPics() );
		iBgContainer->SetContentL( iPresenceContainer );

		iBgContainer->MakeVisible(ETrue);
		iBgContainer->ActivateL();
		AppUi()->AddToStackL( *this, iBgContainer );
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
