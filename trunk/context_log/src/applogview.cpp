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

#include "break.h"
#include "applogview.h"
#include "symbian_auto_ptr.h"

#include "contextlog_resource.h"

#include "context_log.hrh"
#include <aknviewappui.h> 
#include <eiktxlbm.h>
#include <aknmessagequerydialog.h>

#include "app_context.h"
#include <eiklbi.h>
#include "app_context_impl.h"
#include "reporting.h"

//------------------------------------------------------------------------------
class CCustomListItemDrawer : public CTextListItemDrawer
{
public:
        CCustomListItemDrawer( CTextListBoxModel* aModel, const CFont* aFont );
};

CCustomListItemDrawer::CCustomListItemDrawer( CTextListBoxModel* aModel, const CFont* aFont ) : CTextListItemDrawer( aModel, aFont )
{
}
class CCustomTextListBox : public CEikTextListBox
{
public:
	CCustomTextListBox();
	~CCustomTextListBox();
protected:
        void CreateItemDrawerL();
};

CCustomTextListBox::CCustomTextListBox() : CEikTextListBox() {}

CCustomTextListBox::~CCustomTextListBox() {}

void CCustomTextListBox::CreateItemDrawerL()
{
	iItemDrawer = new(ELeave) CTextListItemDrawer( Model(), iEikonEnv->DenseFont());
}




// ================= MEMBER FUNCTIONS =======================

CLogContainer::CLogContainer()
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("CLogContainer"));

}

void CLogContainer::ConstructL(const TRect& aRect, CCircularLog* Log, MEikListBoxObserver* ListBoxObserver)
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("ConstructL"));

	iLog=Log;
	iLog->SetObserver(this);

	CreateWindowL(); 

	iListBox = new (ELeave) CCustomTextListBox; //CEikTextListBox;
	iListBox->SetMopParent(this);
	iListBox->ConstructL(this, 0);

	iListBox->Model()->SetItemTextArray(iLog);
	iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, 
		CEikScrollBarFrame::EAuto);

	iListBox->MakeVisible(ETrue);
	iListBox->SetRect(aRect);
	iListBox->ActivateL();
	iListBox->DrawNow();

	iListBox->SetListBoxObserver(ListBoxObserver);

	SetRect(aRect);
	ActivateL();
}
CLogContainer::~CLogContainer()
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("~CLogContainer"));

	if (iLog) iLog->SetObserver(0);
	delete iListBox;
}
void CLogContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("SizeChanged"));

	iListBox->SetRect(Rect());
}

TInt CLogContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("CountComponentControls"));

	return 1;
}

CCoeControl* CLogContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("ComponentControl"));

	if (aIndex==0) return iListBox;
	return 0;
}

void CLogContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);
}

void CLogContainer::HandleControlEventL(
						CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("HandleControlEventL"));

	// TODO: Add your control event handler code here
}

void CLogContainer::ContentsChanged()
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("ContentsChanged"));

	CC_TRAPD(err,
		if (iListBox) iListBox->HandleItemRemovalL();
		iListBox->DrawNow());
}

TKeyResponse CLogContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CLogContainer"), _CL("OfferKeyEventL"));

	return iListBox->OfferKeyEventL(aKeyEvent, aType);
}

CAppLogView* CAppLogView::NewL(CCircularLog* Log)
{
	CALLSTACKITEM_N(_CL("CAppLogView"), _CL("NewL"));

	auto_ptr<CAppLogView> ret(new (ELeave) CAppLogView);
	ret->ConstructL(Log);
	return ret.release();
}

CAppLogView::CAppLogView()
{
	CALLSTACKITEM_N(_CL("CAppLogView"), _CL("CAppLogView"));

}

// ---------------------------------------------------------
// CAppLogView::ConstructL(const TRect& aRect)
// EPOC two-phased constructor
// ---------------------------------------------------------
//
void CAppLogView::ConstructL(CCircularLog* Log)
{
	CALLSTACKITEM_N(_CL("CAppLogView"), _CL("ConstructL"));

	BaseConstructL( R_LOGVIEW_VIEW );
	iLog=Log;
}


// ---------------------------------------------------------
// CAppLogView::~CAppLogView()
// ?implementation_description
// ---------------------------------------------------------
//
CAppLogView::~CAppLogView() {
	CC_TRAPD(err, ReleaseCAppLogView());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}
void CAppLogView::ReleaseCAppLogView()
{
	CALLSTACKITEM_N(_CL("CAppLogView"), _CL("~ReleaseCAppLogView"));

	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
}

// ---------------------------------------------------------
// TUid CAppLogView::Id()
// ?implementation_description
// ---------------------------------------------------------
//
TUid CAppLogView::Id() const {
	return KLogViewId;
}

// ---------------------------------------------------------
// CAppLogView::HandleCommandL(TInt aCommand)
// ?implementation_description
// ---------------------------------------------------------
//
void CAppLogView::HandleCommandL(TInt aCommand)
{   
	CALLSTACKITEM_N(_CL("CAppLogView"), _CL("HandleCommandL"));

	switch ( aCommand )
        {
        case EAknSoftkeyClose:
		case EAknSoftkeyBack:
			AppUi()->HandleCommandL( Econtext_logCmdlogviewClose );
		break;

        default:
		// TODO
		//AppUi()->HandleCommandL( Econtext_logCmdSettingsCancel );
		break;
        }
}

// ---------------------------------------------------------
// CAppLogView::HandleClientRectChange()
// ---------------------------------------------------------
//
void CAppLogView::HandleClientRectChange()
{
	CALLSTACKITEM_N(_CL("CAppLogView"), _CL("HandleClientRectChange"));

	if ( iContainer )
        {
		iContainer->SetRect( ClientRect() );
        }
}

void CAppLogView::SetLog(CCircularLog* Log)
{
	iLog=Log;
}

// ---------------------------------------------------------
// CAppLogView::DoActivateL(...)
// ?implementation_description
// ---------------------------------------------------------
//
void CAppLogView::DoActivateL(
				const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/,
				const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CAppLogView"), _CL("DoActivateL"));
	MActiveErrorReporter* rep=GetContext()->GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);

	if (!iContainer) {
		iContainer=new (ELeave) CLogContainer;
		iContainer->SetMopParent(this);
		iContainer->ConstructL( ClientRect(), iLog, this );
		AppUi()->AddToStackL( *this, iContainer );
        } 
}

// ---------------------------------------------------------
// CAppLogView::HandleCommandL(TInt aCommand)
// ?implementation_description
// ---------------------------------------------------------
//
void CAppLogView::DoDeactivate()
{
	CALLSTACKITEM_N(_CL("CAppLogView"), _CL("DoDeactivate"));

	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
	iContainer = 0;
}

void CAppLogView::HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType)
{
	if (aEventType== EEventEnterKeyPressed) {
		TInt idx=aListBox->CurrentItemIndex();
		TPtrC16 event=iLog->MdcaPoint(idx);
		auto_ptr<CAknMessageQueryDialog> note(CAknMessageQueryDialog::NewL(event));
		note.release()->ExecuteLD(R_LOGVIEW_EVENT_DIALOG);
	}
}

// End of File
