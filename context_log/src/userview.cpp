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

#include <aknnavi.h>
#include "presence_icons.h"
#include <aknIconArray.h>
#include "userview.h"
#include "symbian_auto_ptr.h"

#include "contextlog_resource.h"

#include "context_log.hrh"
#include <aknviewappui.h> 
#include <eiktxlbm.h>
#include <akntitle.h> 
#include "break.h"

#include <gulbordr.h>
#include <gulicon.h>
#include <eikmenup.h>

#include "app_context.h"
#include "cl_settings.h"

CUserContextContainer::CUserContextContainer(CAknViewAppUi* AppUi) : iAppUi(AppUi) { }

void CUserContextContainer::SetSizes(const TRect& aRect)
{
	TRect r(aRect);
	r.SetHeight(20);
	//r.Move(2, 2);
	r.SetWidth(r.Width());

	TRect c(r);
	//c.Move( (c.Width()-iTitle->MinimumSize().iWidth)/2, 0);
	//iTitle->SetRect(c);

	r.SetHeight(42);
	r.Move(0, 21);
	iListBox->SetRect(r);

	r.SetHeight(aRect.Height()-r.Height()-28);
	r.Move(0, 45);
	c=r;
	c.Move( (c.Width()-iDescription->MinimumSize().iWidth)/2, 0);
	c.Move( 0, (c.Height()-iDescription->MinimumSize().iHeight)/2 );
	iDescription->SetRect(c);

	r=aRect;
	r.SetHeight(21);
	c=r;
	c.Move( (c.Width()-iFrozenLabel->MinimumSize().iWidth)/2, 3);
	iFrozenLabel->SetRect(c);

}

void CUserContextContainer::SetFrozen(TBool aIsFrozen)
{
	iFrozenLabel->MakeVisible(aIsFrozen);
}
void CUserContextContainer::ConstructL(const TRect& aRect, CCircularLog* Log, MEikListBoxObserver* ListBoxObserver)
{
	CALLSTACKITEM_N(_CL("CUserContextContainer"), _CL("ConstructL"));

	CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
	CAknTitlePane* tp=(CAknTitlePane*)sp->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
	tp->SetText(CEikonEnv::Static()->AllocReadResourceL(R_MY_CONTEXT_CAPTION));

	iDescriptionText=CEikonEnv::Static()->AllocReadResourceL(R_MY_CONTEXT_DESCRIPTION);
	iFrozenText=CEikonEnv::Static()->AllocReadResourceL(R_PRESENCE_IS_FROZEN);

	iLog=Log;
	
	iLog->SetObserver(this);

	CreateWindowL(); 


#ifdef __JAIKU_ENABLED__
	User::Leave( KErrNotSupported );
	iListBox = NULL; //;CJaikuContactsListBox::CreateListboxL( this );
	
	auto_ptr<CDesCArray> array( new (ELeave) CDesCArrayFlat(10) );
	array->AppendL( _L("Myself\tKallio, Helsinki\t1") );
	iListBox->Model()->SetItemTextArray(array.get());
	iListBox->Model()->SetOwnershipType(ELbmOwnsItemArray);
	array.release();
#else
	iListBox = new (ELeave) doublelinebox(0, 0);
	iListBox->SetMopParent(this);
	iListBox->ConstructL(iLog, iLog,
		this, EAknListBoxSelectionList);

	iListBox->SetItemHeightL(40);
	iListBox->View()->SetMatcherCursor(EFalse);
	
	// icon array
	CAknIconArray * icons = new (ELeave) CAknIconArray(30);
	LoadIcons(icons);
	iListBox->ItemDrawer()->FormattedCellData()->SetIconArray(icons);

	iListBox->Model()->SetItemTextArray(iLog);
	iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, 
		CEikScrollBarFrame::EAuto);

#endif
	iListBox->MakeVisible(ETrue);

	iFrozenLabel=new (ELeave) CEikLabel;
	iFrozenLabel->SetContainerWindowL( *this );
	iFrozenLabel->SetTextL(*iFrozenText);
	iFrozenLabel->SetFont(CEikonEnv::Static()->DenseFont());

	iDescription=new (ELeave) CEikLabel;
	iDescription->SetContainerWindowL( *this );
	iDescription->SetTextL(*iDescriptionText);
	iDescription->SetFont(CEikonEnv::Static()->DenseFont());

	SetSizes(aRect);

	iListBox->ActivateL();
	iListBox->DrawNow();

	iListBox->SetListBoxObserver(ListBoxObserver);

	SetRect(aRect);
	ActivateL();
}

CUserContextContainer::~CUserContextContainer()
{
	CALLSTACKITEM_N(_CL("CUserContextContainer"), _CL("~CUserContextContainer"));

	iLog->SetObserver(0);
	delete iListBox;
	delete iDescription;
	delete iFrozenLabel;

	delete iDescriptionText;
	delete iFrozenText;
}

void CUserContextContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CUserContextContainer"), _CL("SizeChanged"));

	SetSizes(Rect());
}

TInt CUserContextContainer::CountComponentControls() const {
	return 3;
}

CCoeControl* CUserContextContainer::ComponentControl(TInt aIndex) const {

	switch(aIndex) {
	case 0:
		return iListBox;
	case 1:
		return iDescription;
	case 2:
		return iFrozenLabel;
	
	default:
		return 0;
	}
}

void CUserContextContainer::Draw(const TRect& aRect) const {

	CWindowGc& gc = SystemGc();
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
}

void CUserContextContainer::ContentsChanged()
{
	CALLSTACKITEM_N(_CL("CUserContextContainer"), _CL("ContentsChanged"));

	CC_TRAPD(err,
		if (iListBox) iListBox->HandleItemRemovalL();
		iListBox->DrawNow());
}

TKeyResponse CUserContextContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CUserContextContainer"), _CL("OfferKeyEventL"));

	if (aType==EEventKey) 
	{
		if (aKeyEvent.iCode=='5') {
			iPresses++;
			if (iPresses==3) {
				iAppUi->HandleCommandL(Econtext_logDetailedView);
			}
			return EKeyWasConsumed;
		} else {
			iPresses=0;
		}
		switch (aKeyEvent.iCode) {
		case EKeyOK:
			iAppUi->HandleCommandL(Econtext_logCmdSetUserGiven);
			return EKeyWasConsumed;
		case EKeyRightArrow:
			iAppUi->HandleCommandL(Econtext_logPresenceDescription);
			return EKeyWasConsumed;
		case EKeyLeftArrow:
			iAppUi->HandleCommandL(Econtext_logPresenceDetails);
			return EKeyWasConsumed;
		default:
			break;
		}
	}
	
	return EKeyWasNotConsumed;
}

CUserView* CUserView::NewL(CCircularLog* Log, CPresencePublisher*& PresencePublisher)
{
	CALLSTACKITEM2_N(_CL("CUserView"), _CL("NewL"), ::GetContext());

	auto_ptr<CUserView> ret(new (ELeave) CUserView(PresencePublisher));
	ret->ConstructL(Log);
	return ret.release();
}

CUserView::CUserView(CPresencePublisher*& PresencePublisher) : iPresencePublisher(PresencePublisher)
{
	CALLSTACKITEM_N(_CL("CUserView"), _CL("CUserView"));

}

// ---------------------------------------------------------
// CAppLogView::ConstructL(const TRect& aRect)
// EPOC two-phased constructor
// ---------------------------------------------------------
//
void CUserView::ConstructL(CCircularLog* Log)
{
	CALLSTACKITEM_N(_CL("CUserView"), _CL("ConstructL"));

	BaseConstructL( R_USERVIEW_VIEW );
	iLog=Log;
}


CUserView::~CUserView() {
	CC_TRAPD(err, ReleaseCUserView());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}

void CUserView::ReleaseCUserView()
{
	CALLSTACKITEM_N(_CL("CUserView"), _CL("~CUserView"));

	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
}

TUid CUserView::Id() const {
	return KUserViewId;
}

void CUserView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM_N(_CL("CUserView"), _CL("HandleCommandL"));

	AppUi()->HandleCommandL(aCommand);
}

void CUserView::HandleClientRectChange()
{
	CALLSTACKITEM_N(_CL("CUserView"), _CL("HandleClientRectChange"));

	if ( iContainer )
        {
		iContainer->SetRect( ClientRect() );
        }
}

void CUserView::DoActivateL(
				const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/,
				const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CUserView"), _CL("DoActivateL"));

	if (!iContainer) {
		iContainer=new (ELeave) CUserContextContainer(AppUi());
		iContainer->SetMopParent(this);
		iContainer->ConstructL( ClientRect(), iLog, this);
		AppUi()->AddToStackL( *this, iContainer );
        } 
	SetFrozen(iPresencePublisher->IsFrozen());
}

void CUserView::SetFrozen(TBool aIsFrozen)
{
	if (iContainer) iContainer->SetFrozen(aIsFrozen);
}

void CUserView::DoDeactivate()
{
	TRAPD(err, {
		
	CALLSTACKITEM_N(_CL("CUserView"), _CL("DoDeactivate"));

	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
	iContainer = 0;
	});
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
}

void CUserView::HandleListBoxEventL(CEikListBox* /*aListBox*/,TListBoxEvent /*aEventType*/) { }

void CUserView::DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
{
	CALLSTACKITEM_N(_CL("CUserView"), _CL("DynInitMenuPaneL"));

	switch(aResourceId) {
	case R_USERVIEW_CONTROL_MENU:
		{
		if (iPresencePublisher) {
			if (iPresencePublisher->ConnectionSuspended()) {
				aMenuPane->SetItemDimmed(Econtext_logCmdAppSuspendPresence, ETrue);	
				aMenuPane->SetItemDimmed(Econtext_logCmdAppResumePresence, EFalse);
			} else {
				aMenuPane->SetItemDimmed(Econtext_logCmdAppSuspendPresence, EFalse);	
				aMenuPane->SetItemDimmed(Econtext_logCmdAppResumePresence, ETrue);
			}
			if (iPresencePublisher->IsFrozen() ) {
				aMenuPane->SetItemDimmed(Econtext_logCmdFreezePresence, ETrue);	
				aMenuPane->SetItemDimmed(Econtext_logCmdUnFreezePresence, EFalse);
			} else {
				aMenuPane->SetItemDimmed(Econtext_logCmdFreezePresence, EFalse);	
				aMenuPane->SetItemDimmed(Econtext_logCmdUnFreezePresence, ETrue);
			}
		} else {
			aMenuPane->SetItemDimmed(Econtext_logCmdAppSuspendPresence, ETrue);	
			aMenuPane->SetItemDimmed(Econtext_logCmdAppResumePresence, ETrue);
			aMenuPane->SetItemDimmed(Econtext_logCmdFreezePresence, ETrue);	
			aMenuPane->SetItemDimmed(Econtext_logCmdUnFreezePresence, ETrue);
		}
		/*
		TBool logging;
		Settings().GetSettingL(SETTING_LOGGING_ENABLE, logging);
		if (logging) {
			aMenuPane->SetItemDimmed(Econtext_logCmdAppPauseLog, EFalse);
			aMenuPane->SetItemDimmed(Econtext_logCmdAppUnPauseLog, ETrue);
		} else {
			aMenuPane->SetItemDimmed(Econtext_logCmdAppPauseLog, ETrue);
			aMenuPane->SetItemDimmed(Econtext_logCmdAppUnPauseLog, EFalse);
		}
		*/
		}
		break;

	case R_USERVIEW_VIEW_MENU:
		{
		TBool enable_options=ETrue;
		Settings().GetSettingL(SETTING_OPTIONS_ENABLE, enable_options);
		aMenuPane->SetItemDimmed(Econtext_logCmdStatusView, ! enable_options );
		}
		break;
	default:
		break;
	}
}
