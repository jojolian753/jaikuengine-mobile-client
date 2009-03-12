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

#include "cu_dynamicsettings.h"

#include "cl_settings.h"
#include "cu_common.h"
#include "contextui.hrh"
#include <contextui.rsg>

#include "app_context_impl.h"
#include "break.h"
#include "errorhandling.h"
#include "reporting.h"
#include "symbian_auto_ptr.h"

#include <aknlists.h>
#include <aknsettingitemlist.h>
#include <aknviewappui.h>

#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknnavi.h>
#include <eikspane.h>

class CBgContainer : public CCoeControl, public MContextBase
{
public:
	

	CBgContainer() 
	{}
	
	
	void ConstructL(MObjectProvider* aMopParent, const TRect& aRect)
	{
		CALLSTACKITEM_N(_CL("CBgContainer"), _CL("ConstructL"));
		SetMopParent( aMopParent );
		CreateWindowL();				
		{ 
			TRect rect=aRect;
			rect.Move( 0, -rect.iTl.iY );
			iBackground=CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain,
																  rect, EFalse );
		}
		SetRect( aRect );		
	}
	
	void FocusChanged( TDrawNow aDrawNow )
    {
		CCoeControl::FocusChanged( aDrawNow );
        if( iContent )
			{
				iContent->SetFocus( IsFocused(), aDrawNow );
			}
    }
		
	void SetContentL(CCoeControl* aContent)
	{
		CALLSTACKITEM_N(_CL("CBgContainer"), _CL("SetContentL"));
		iContent = aContent;
		SizeChanged();
	}
		
	void SizeChangedImplL()
	{
		CALLSTACKITEM_N(_CL("CBgContainer"), _CL("SizeChangedImpl"));
		TRect fullR = Rect();
		iBackground->SetRect(fullR);
		
		if ( iContent )
			iContent->SetRect(fullR);
	}

	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CBgContainer"), _CL("SizeChanged"));
		CC_TRAPD( err, SizeChangedImplL() );
		if ( err != KErrNone )
			{
				ReportActiveError( KNullDesC, KNullDesC, err );
				User::Leave(err);
			}
	}

	TInt CountComponentControls() const
	{
	    CALLSTACKITEM_N(_CL("CBgContainer"), _CL("CountComponentControl"));
		return iContent ? 1 : 0;
	}
	
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CBgContainer"), _CL("ComponentControl"));
		switch ( aIndex ) 
			{
			case 0:
				return iContent;
				break;
			}				
		return NULL;
	}
	

	~CBgContainer() 
	{
		delete iBackground;
		iContent = NULL;
	}

	void Draw(const TRect& aRect) const
	{
		CWindowGc& gc = SystemGc();
		TRect r = Rect();
		AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, gc, r );
	}


	
	void DrawProgressBarBg(CWindowGc& aGc, const TRect& aWindowRect) const
	{
		MAknsSkinInstance* skin = AknsUtils::SkinInstance();

		TRect r = aWindowRect;
		//TRect r = iProgressBar->Rect();
		TRect cr = iEikonEnv->EikAppUi()->ClientRect();
		r.Move(0, - cr.iTl.iY); 
		
		TPoint dstPosInCanvas(0,0);			
		TRect partOfBackground( r );
		TBool skinBackgroundDrawn = 
			AknsDrawUtils::DrawBackground( skin, 
										   iBackground,
										   NULL, 
										   aGc, 
										   dstPosInCanvas,
										   partOfBackground,
										   KAknsDrawParamLimitToFirstLevel);
	}
	
	
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{
		if ( iContent )
			return iContent->OfferKeyEventL(aKeyEvent, aType);
		return EKeyWasNotConsumed;
	}
	

 	TTypeUid::Ptr MopSupplyObject(TTypeUid aId)
 	{
		if (aId.iUid == MAknsControlContext::ETypeId)
			{
				return MAknsControlContext::SupplyMopObject( aId, iBackground );
			}
		return CCoeControl::MopSupplyObject( aId );
	}

public:
	static CBgContainer* NewL(MObjectProvider *aMopParent, 
							  const TRect& aRect);
	
private:
	CAknsBasicBackgroundControlContext* iBackground;
	CCoeControl* iContent;

};

CBgContainer* CBgContainer::NewL(MObjectProvider *aMopParent, 
												  const TRect& aRect)
{
	CALLSTACKITEMSTATIC_N(_CL("CBgContainer"), _CL("NewL"));
	auto_ptr<CBgContainer> self( new (ELeave) CBgContainer() );
	self->ConstructL(aMopParent, aRect);
	return self.release();
}

class CDynamicSettingsViewImpl 
	: public CDynamicSettingsView, 
	  public MContextBase,
	  public MEikListBoxObserver
{
public:	
	CDynamicSettingsViewImpl(TUid aViewId) : iViewId(aViewId) { }
	
	void ConstructL(const TItemDef* aItemDefs, TInt aDefCount)
	{
		CALLSTACKITEM_N(_CL("CDynamicSettingsViewImpl"), _CL("ConstructL"));
		iResource=LoadSystemResourceL(iEikonEnv, _L("contextui"));
		
		iItemDefs.Reset();
		for (TInt i=0; i < aDefCount; i++)
			iItemDefs.Append( aItemDefs[i] );
		
		BaseConstructL( R_SETTINGS_VIEW );
	}
	
	~CDynamicSettingsViewImpl()
	{
		CALLSTACKITEM_N(_CL("CDynamicSettingsViewImpl"), _CL("~CDynamicSettingsViewImpl"));
		
		RemoveContainerL();
		iItemDefs.Close();
		if (iResource) iEikonEnv->DeleteResourceFile(iResource);
	}
	
	TUid Id() const {
		return iViewId;
	}
	
	void SetPreviousLocalViewId(TUid aViewId) {
		iPreviousLocalViewId=aViewId;
	}

    void HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType)
	{
		switch (aEventType)
			{
			case EEventEnterKeyPressed:
			case EEventItemDoubleClicked:
				ShowSettingPageL(EFalse);
            break;
			default:
				break;
			}
	}
	

	void ShowSettingPageL(TInt aCalledFromMenu)
    {
		TInt index = iListbox->CurrentItemIndex();
		if ( index >= 0 )
			{
				CAknSettingItem* item = iSettingItemArray->At(index);
				
				item->EditItemL(aCalledFromMenu);
				item->UpdateListBoxTextL();
				iListbox->DrawDeferred();
			}
		//iListBox-DrawNow();
    }

	
	void HandleCommandL(TInt aCommand)
	{   
		CALLSTACKITEM_N(_CL("CDynamicSettingsViewImpl"), _CL("HandleCommandL"));
		
		switch ( aCommand )
			{
			case EContextUICmdSettingsChange:
				{
					ShowSettingPageL( ETrue );				
				}
				break;
			case EContextUICmdResetUiState1:
				{
					Settings().WriteSettingL( SETTING_DONT_DOUBLE_CONFIRM_DELETES, 0 );
					Settings().WriteSettingL( SETTING_SHOW_WARNING_ON_BACK, 1 );
				}
				break;
			case EContextUICmdSettingsSave:
				StoreSettingsL();
				break;
			case EAknSoftkeyOk:
			case EAknSoftkeyDone:
			case EAknSoftkeyBack:
				StoreSettingsL();					
				if (iPreviousLocalViewId==TUid::Uid(0)) {
					AppUi()->ActivateViewL(iPreviousViewId);
				} else {
					AppUi()->ActivateLocalViewL(iPreviousLocalViewId);
				}
				break;
			default:
				AppUi()->HandleCommandL(aCommand);
			}
	}
	

	void StoreSettingsL()
    {
		CALLSTACKITEM_N(_CL("CDynamicSettingsViewImpl"), _CL("StoreSettingsL"));
		for ( TInt i=0; i < iSettingItemArray->Count(); i++)
			{
				iSettingItemArray->At(i)->StoreL();
			}
		
		for ( TInt i = 0; i < iItemDefs.Count(); i++)
			{
				TItemDef def = iItemDefs[ i ];
				def.iFactory->StoreSettingL(def.iIdentifier, i );
			}
	}


	void HandleClientRectChange()
	{
		CALLSTACKITEM_N(_CL("CDynamicSettingsViewImpl"), _CL("HandleClientRectChange"));
		if ( iContainer )
			iContainer->SetRect( ClientRect() );
	}

	virtual void HandleResourceChange( TInt aType ) {
		if ( aType == KEikDynamicLayoutVariantSwitch )
		{
		  HandleClientRectChange();
		}
	}

	
	void DoActivateL(
					 const TVwsViewId& aPrevViewId,TUid /*aCustomMessageId*/,
				const TDesC8& /*aCustomMessage*/)
	{
		CALLSTACKITEM_N(_CL("CDynamicSettingsViewImpl"), _CL("DoActivateL"));
		iIsActivated=ETrue;
		MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
		if (rep) rep->SetInHandlableEvent(ETrue);
#ifdef __WINS__
		TInt dummy;
		TBreakItem b(GetContext(), dummy);
#endif
		iPreviousViewId = aPrevViewId;

		{
			CEikStatusPane *sp = StatusPane();
			// Fetch pointer to the default navi pane control
			CAknNavigationControlContainer* naviPane = ( CAknNavigationControlContainer * )
				sp->ControlL( TUid::Uid( EEikStatusPaneUidNavi ) );
			naviPane->PushDefaultL(ETrue);
		}

		if (!iContainer)
			{
				iContainer = CBgContainer::NewL( this, ClientRect() );
				
				if (!iListbox) {
					iListbox = new (ELeave) CAknSettingStyleListBox();
					iListbox->ConstructL( iContainer, EAknListBoxSelectionList );
						//iListbox->SetMopParent( iContainer );
					iListbox->SetContainerWindowL( *iContainer );
					iListbox->CreateScrollBarFrameL( ETrue );
					iListbox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, 
																		CEikScrollBarFrame::EAuto );
					iListbox->SetListBoxObserver( this );
					iSettingItemArray = new (ELeave) CAknSettingItemArray( 10, EFalse, 0);
					CTextListBoxModel* model = iListbox->Model();
					model->SetItemTextArray( iSettingItemArray );
					model->SetOwnershipType( ELbmDoesNotOwnItemArray );
				} 
			}
		
		UpdateListBoxL();
		
		iContainer->SetContentL( iListbox );

		AppUi()->AddToStackL( *this, iContainer );
		iContainer->SetRect(ClientRect());
		
		// 	CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
		// 	CAknNavigationControlContainer *np = (CAknNavigationControlContainer *)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi)); 
		// 	np->PushDefaultL();
	
		iContainer->ActivateL();
	}
	
	
	void UpdateListBoxL()
    {
		iSettingItemArray->ResetAndDestroy();
		
		RPointerArray<MSettingFactory> activated;

		for (TInt ordinal = 0; ordinal < iItemDefs.Count(); ordinal++)
			{
				TItemDef def = iItemDefs[ ordinal ];

				MSettingFactory* factory = def.iFactory;
				
				if ( activated.Find( factory) == KErrNotFound )
					{
						factory->ActivateL();
						activated.AppendL( factory );
					}

				CAknSettingItem* item = factory->CreateItemL( def.iIdentifier, ordinal );
				iSettingItemArray->InsertL( ordinal, item );
				//item->SetParentListBox( iListbox );
			}
		
		activated.Close();

		iSettingItemArray->RecalculateVisibleIndicesL();
		// Calling HandleItemAdditional redraws whole list.
		iListbox->HandleItemAdditionL();
 		iListbox->UpdateScrollBarsL();
    }

	void RemoveContainerL()
	{
		if ( iContainer )
			{
				AppUi()->RemoveFromViewStack( *this, iContainer );
			}

		RPointerArray<MSettingFactory> deactivated;		
		for (TInt ordinal = 0; ordinal < iItemDefs.Count(); ordinal++)
			{
				TItemDef def = iItemDefs[ ordinal ];

				MSettingFactory* factory = def.iFactory;
				
				if ( deactivated.Find( factory) == KErrNotFound )
					{
						factory->DeactivateL();
						deactivated.AppendL( factory );
					}
			}
		deactivated.Close();


		delete iListbox;
		iListbox = NULL;
		
		if ( iSettingItemArray )
			iSettingItemArray->ResetAndDestroy();
		delete iSettingItemArray;
		iSettingItemArray = NULL;
		
		delete iContainer;
		iContainer = NULL;
	}
	
	void DoDeactivate()
	{
		if (!iIsActivated) {
			iIsActivated=EFalse;
			MActiveErrorReporter* rep=AppContext().GetActiveErrorReporter();
			if (rep) rep->SetInHandlableEvent(ETrue);
		}
		CALLSTACKITEM_N(_CL("CDynamicSettingsViewImpl"), _CL("DoDeactivate"));
		RemoveContainerL();

		{
			CEikStatusPane *sp = StatusPane();
			// Fetch pointer to the default navi pane control
			CAknNavigationControlContainer* naviPane = ( CAknNavigationControlContainer * )
				sp->ControlL( TUid::Uid( EEikStatusPaneUidNavi ) );
			naviPane->Pop();
		}

	}

	TBool iIsActivated;

	CAknSettingStyleListBox* iListbox; // own 
	CAknSettingItemArray* iSettingItemArray; // own 
	CBgContainer* iContainer;
	
	TUid iViewId;
	TUid iPreviousLocalViewId;	
 	TVwsViewId iPreviousViewId;
	TInt		iResource;

	RArray<TItemDef> iItemDefs;
};

EXPORT_C CDynamicSettingsView* CDynamicSettingsView::NewL(TUid aViewId, const TItemDef* aItemDefs, TInt aDefCount)
{
	CALLSTACKITEMSTATIC_N(_CL("CDynamicSettingsView"), _CL("NewL"));
	
	auto_ptr<CDynamicSettingsViewImpl> ret(new (ELeave) CDynamicSettingsViewImpl(aViewId));
	ret->ConstructL(aItemDefs, aDefCount);
	return ret.release();
}
