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
#include "presencedescriptionview.h"

#include "PresenceTextFormatter.h"

#include <aknviewappui.h>
#include <aknnavi.h>
#include <akntitle.h> 
#include "csd_presence.h"
#include <eikedwin.h>
#include <eiklabel.h>
#include <bautils.h>
#include <contextcontactsui.rsg>

#include <contextvariant.hrh>
#ifdef __JAIKU__
#include "JaikuContactsListBox.h"
#endif

#include <eikrted.h> 
#include <TXTGLOBL.H>
#include "reporting.h"
#include "app_context_impl.h"

const TInt KScreenWidth=176;
const TInt KLabelHeight=15;
const TInt KOffset = 4;

_LIT(KQuestion, "?");

typedef CList<CEikLabel*> CLabelGroup;

class CPresenceDescriptionContainer : public CCoeControl, public MEikEdwinObserver, 
	public MEikScrollBarObserver {
public:
	CPresenceDescriptionContainer(CAknView *aView);
	~CPresenceDescriptionContainer();
	void ConstructL(const TRect& aRect, const TDesC& Name, const CBBPresence* PresenceData);
private:
	void Draw(const TRect& aRect) const;
	void HandleScrollEventL(CEikScrollBar* aScrollBar, TEikScrollEvent aEventType);
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	void HandleEdwinEventL(CEikEdwin* aEdwin,TEdwinEvent aEventType) { }
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;

	void CreateFontTesterL(const TRect& aRect);

	TEikScrollBarModel iModel;
	CEikScrollBarFrame * iSBFrame; 

	CEikLabel	*iTitle;
	CEikEdwin	*iEdit;
	CAknView	*iView;

	CEikRichTextEditor* iEditor;

	TInt iMaxScrollPos; 
	TInt iCurrentScrollPos; 
};

class CPresenceDescriptionViewImpl : public CPresenceDescriptionView {
private:
	CPresenceDescriptionViewImpl();
	void ConstructL();

	virtual void SetData(const TDesC& Name, const CBBPresence* PresenceData);
	
	TUid Id() const;

	void ShowDescription();
	
        void HandleCommandL(TInt aCommand);
        void DoActivateL(const TVwsViewId& aPrevViewId,
		TUid aCustomMessageId,
		const TDesC8& aCustomMessage);
        void DoDeactivate();

	friend class CPresenceDescriptionView;

	TBuf<100>	iName;
	const CBBPresence*	iPresence;
	CPresenceDescriptionContainer* iContainer;
	TVwsViewId	iPrevView;
	TInt		iResource;
public:
	virtual ~CPresenceDescriptionViewImpl();
	void ReleaseCPresenceDescriptionViewImpl();
};

EXPORT_C CPresenceDescriptionView* CPresenceDescriptionView::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CPresenceDescriptionView"), _CL("NewL"));

	auto_ptr<CPresenceDescriptionViewImpl> ret(new (ELeave) CPresenceDescriptionViewImpl);
	ret->ConstructL();
	return ret.release();
}

CPresenceDescriptionContainer::CPresenceDescriptionContainer(CAknView *aView) : 
	iView(aView) { }

CPresenceDescriptionContainer::~CPresenceDescriptionContainer()
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionContainer"), _CL("~CPresenceDescriptionContainer"));
	delete iEditor;
	delete iEdit;
	delete iTitle;
	delete iSBFrame;
}


void CPresenceDescriptionContainer::ConstructL(const TRect& aRect, const TDesC& Name, 
					       const CBBPresence* PresenceData)
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionContainer"), _CL("ConstructL"));

	CreateWindowL(); 

	// Font tester 
	//CreateFontTesterL(aRect);

	iTitle=new (ELeave) CEikLabel;
	iTitle->SetContainerWindowL(*this);
	iTitle->SetFont(CEikonEnv::Static()->DenseFont());
	iTitle->SetUnderlining(ETrue);
	iTitle->SetTextL(Name);
	iTitle->SetAlignment(TGulAlignment(EHCenterVCenter));
	
	TRect t=aRect;
	t.Move(0, 4);
	t.Resize(0, -(t.Height()-15));
	iTitle->SetRect(t);


	iEdit=new (ELeave) CEikEdwin;
	iEdit->SetContainerWindowL(*this);
	iEdit->ConstructL(CEikEdwin::ENoAutoSelection|CEikEdwin::EDisplayOnly);
	iEdit->AddEdwinObserverL(this);
	iEdit->ActivateL();
	TRgb light_gray  = TRgb(210,210,210);
	iEdit->OverrideColorL(EColorControlBackground, light_gray);
	iEdit->OverrideColorL(EColorControlText, KRgbBlack);

#ifdef __JAIKU__
	// FIXME: teemu. we should handle buffer sizes and long texts 
	// gracefully
	auto_ptr<HBufC> presenceMsg( HBufC::NewL( 2000 ) );
	TPtr tmpPtr = presenceMsg->Des();
	
	auto_ptr<CPresenceTextFormatter> formatter( CPresenceTextFormatter::NewL() );
	formatter->LongTextL( PresenceData, tmpPtr );
	iEdit->SetTextL( presenceMsg.get() );  // ownership is not transferred 
#else
	if (PresenceData && PresenceData->iUserGiven.iDescription().Length()>0) {
		iEdit->SetTextL(& (PresenceData->iUserGiven.iDescription()));
	} else {
		iEdit->SetTextL(&(KQuestion()));
	}
#endif
	TRect e=aRect;
	e.Resize(0, -19);
	e.Move(0, 19);
	iEdit->SetRect(e);

	SetRect(aRect);
	ActivateL();

	iEdit->MoveCursorL(TCursorPosition::EFPageUp, EFalse);
	while (iEdit->CursorPos()!=iEdit->TextLength()) {
		iEdit->MoveCursorL(TCursorPosition::EFLineDown, EFalse);
		iMaxScrollPos++;
	}
	iEdit->MoveCursorL(TCursorPosition::EFPageUp, EFalse);

	iCurrentScrollPos = 0;

	iModel = TEikScrollBarModel(iMaxScrollPos, iCurrentScrollPos, 0);
	iSBFrame = new (ELeave) CEikScrollBarFrame(this, this, ETrue);
	iSBFrame->SetScrollBarVisibilityL(CEikScrollBarFrame::EOn, CEikScrollBarFrame::EAuto);
#ifndef __S60V3__
	// FIXME3RD: do we have to do something
	iSBFrame->SetScrollBarManagement(CEikScrollBar::EVertical, CEikScrollBarFrame::EFloating);
#endif
	iSBFrame->Tile(&iModel);
	iSBFrame->DrawScrollBarsNow();
}


// void CPresenceDescriptionContainer::SizeChanged()
// {
// 	iEikonEnvApplication
// }

TInt CPresenceDescriptionContainer::CountComponentControls() const {
	if (iEditor)
		{
			return 1;
		}
	else
		{
			if (iEdit)
				return 2;
			return 1;
		}
}

void CPresenceDescriptionContainer::HandleScrollEventL(CEikScrollBar* aScrollBar, 
						       TEikScrollEvent aEventType)
{
}

TKeyResponse CPresenceDescriptionContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent,
							   TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionContainer"), _CL("OfferKeyEventL"));

	if (aType!=EEventKey) return EKeyWasNotConsumed;
	
	if ( iEditor && (aKeyEvent.iCode == EKeyDownArrow || aKeyEvent.iCode == EKeyUpArrow ) )
		{
			return iEditor->OfferKeyEventL(aKeyEvent, aType);
		}

	if ( aKeyEvent.iCode==EKeyLeftArrow ) {
		iView->HandleCommandL(EAknCmdExit);
		return EKeyWasConsumed;
	} else if ( aKeyEvent.iCode==EKeyDownArrow ) {
		if (iCurrentScrollPos+1<iMaxScrollPos) {
			iSBFrame->MoveThumbsBy(0, 1);
			iEdit->MoveDisplayL(TCursorPosition::EFLineDown);
			iSBFrame->DrawScrollBarsNow();
			iCurrentScrollPos++;
		}
	} else if ( aKeyEvent.iCode==EKeyUpArrow ) {
		if (iCurrentScrollPos>0) {
			iSBFrame->MoveThumbsBy(0, -1);
			iEdit->MoveDisplayL(TCursorPosition::EFLineUp);
			iSBFrame->DrawScrollBarsNow();
			iCurrentScrollPos--;
		}
	}
	return EKeyWasNotConsumed;
}


CCoeControl* CPresenceDescriptionContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionContainer"), _CL("ComponentControl"));
	if (iEditor) return iEditor;
	if (aIndex==0) return iTitle;
	else if (aIndex==1) return iEdit;
	return 0;
}

void CPresenceDescriptionContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);

	gc.SetBrushColor(KRgbWhite);
	gc.DrawRect(aRect);
}

CPresenceDescriptionViewImpl::CPresenceDescriptionViewImpl() { }

#include "cu_common.h"

void CPresenceDescriptionViewImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionViewImpl"), _CL("ConstructL"));
	iResource=LoadSystemResourceL(iEikonEnv, _L("contextcontactsui"));

	BaseConstructL( R_PRESENCEDESCRIPTION_VIEW );
}

TUid CPresenceDescriptionViewImpl::Id() const {
	return KPresenceDescriptionView;
}

void CPresenceDescriptionViewImpl::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionViewImpl"), _CL("HandleCommandL"));

	switch (aCommand) 
		{
		case EAknSoftkeyBack:
		case EAknCmdExit:
			ActivateViewL(iPrevView);		
			break;
		default:
			break;
		}
}

void CPresenceDescriptionViewImpl::DoActivateL(const TVwsViewId& aPrevViewId,
	TUid /*aCustomMessageId*/,
	const TDesC8& /*aCustomMessage*/)
{
	MActiveErrorReporter* rep=GetContext()->GetActiveErrorReporter();
	if (rep) rep->SetInHandlableEvent(ETrue);
	CALLSTACKITEM_N(_CL("CPresenceDescriptionViewImpl"), _CL("DoActivateL"));

	iPrevView=aPrevViewId;
	if (!iContainer) {
		TRect r = AppUi()->ApplicationRect();
		iContainer=new (ELeave) CPresenceDescriptionContainer(this);
		iContainer->ConstructL( r, iName, iPresence);
		iContainer->SetMopParent(this);
	
		AppUi()->AddToStackL( *this, iContainer );
        }
}

void CPresenceDescriptionViewImpl::DoDeactivate()
{
	CC_TRAPD( err, {

		CALLSTACKITEM_N(_CL("CPresenceDescriptionViewImpl"), _CL("DoDeactivate"));

		if ( iContainer )
			{
			AppUi()->RemoveFromViewStack( *this, iContainer );
			}
	
		delete iContainer;
		iContainer = 0;
	});
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);		
}


CPresenceDescriptionViewImpl::~CPresenceDescriptionViewImpl()
{
	CC_TRAPD(err, ReleaseCPresenceDescriptionViewImpl());
	if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
}

void CPresenceDescriptionViewImpl::ReleaseCPresenceDescriptionViewImpl()
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionViewImpl"), _CL("~CPresenceDescriptionViewImpl"));

	if (iResource) iEikonEnv->DeleteResourceFile(iResource);
	delete iContainer;
}

void CPresenceDescriptionViewImpl::SetData(const TDesC& Name, const CBBPresence* PresenceData)
{
	CALLSTACKITEM_N(_CL("CPresenceDescriptionViewImpl"), _CL("SetData"));

	iName=Name;
	iPresence=PresenceData;
}

void CPresenceDescriptionContainer::CreateFontTesterL(const TRect& aRect)
{
    iEditor=new(ELeave)CEikRichTextEditor;
    iEditor->ConstructL(this,0,0,CEikEdwin::ENoAutoSelection,0,0);
    iEditor->SetContainerWindowL(*this);
    iEditor->SetSize(aRect.Size());
    iEditor->SetFocus(ETrue);
    CGlobalText *text=iEditor->GlobalText();

    TCharFormat cf;
    TCharFormatMask cfm;
    text->GetCharFormat(cf,cfm,0,0);

    cf.iFontPresentation.iTextColor=TRgb(0);
    cfm.SetAttrib(EAttColor);

    CGraphicsDevice *dev=SystemGc().Device();
    CDesCArrayFlat *fontnames=new(ELeave)CDesCArrayFlat(5);
    FontUtils::GetAvailableFontsL(*dev,*fontnames,EGulAllFonts);

    for(TInt i=0;i<fontnames->Count();i++)
    {
		_LIT(KText, " Mikko Hypponen");
//         cf.iFontSpec.iTypeface.iName=_L("LatinPlain12");
//         cfm.SetAttrib(EAttFontTypeface);
//         text->ApplyCharFormatL(cf,cfm,text->DocumentLength(),0);

//         text->InsertL(text->DocumentLength(),(*fontnames)[i]);
//         text->InsertL(text->DocumentLength(),CPlainText::EParagraphDelimiter);

        cf.iFontSpec.iTypeface.iName=(*fontnames)[i];
        cfm.SetAttrib(EAttFontTypeface);
        text->ApplyCharFormatL(cf,cfm,text->DocumentLength(),0);

		TBuf<10> num;
		num.Num(i);
        text->InsertL(text->DocumentLength(),num);

        text->InsertL(text->DocumentLength(),KText);
        text->InsertL(text->DocumentLength(),CPlainText::EParagraphDelimiter);
    }
}
