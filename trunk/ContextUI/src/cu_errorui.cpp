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

#include "cu_errorui.h"

#include "cu_common.h"
#include "contextui.hrh"
#include <contextui.rsg>

#include "app_context.h"
#include "break.h"
#include "errorinfo.h"
#include "raii_f32file.h"
#include "symbian_auto_ptr.h"

#ifndef __S80__
#include <aknutils.h>
#include <aknviewappui.h>
#endif
#include <bautils.h>
#include <eikrted.h>
#include <eikenv.h>
#include <sendui.h>
#ifndef __S60V3__
#include  <SENDNORM.RSG>
#else
#include <cmessagedata.h>
#endif
#include <txtrich.h>


void AppendFormatted(CRichText& aInto, const TDesC& aText, const TRgb& aColor)
{
	TInt pos=aInto.DocumentLength();
#ifndef __S80__
	TFontSpec fontspec = LatinPlain12()->FontSpecInTwips();
#else
	TFontSpec fontspec = CEikonEnv::Static()->DenseFont()->FontSpecInTwips();
#endif
	TCharFormat charFormat( fontspec.iTypeface.iName, fontspec.iHeight );
	TCharFormatMask charFormatMask;
	charFormat.iFontPresentation.iTextColor=aColor;
	charFormatMask.SetAttrib(EAttColor);

	aInto.InsertL(pos, aText);
	aInto.ApplyCharFormatL(charFormat, charFormatMask, pos, aText.Length());

	CParaFormat paraFormat;
	TParaFormatMask paraFormatMask;
	aInto.GetParaFormatL(&paraFormat, paraFormatMask, pos, aText.Length());
	paraFormatMask.SetAttrib(EAttLineSpacingControl);
	paraFormatMask.SetAttrib(EAttLineSpacing);
	// paraFormatMask.SetAttrib(EAttAlignment);
	paraFormat.iLineSpacingControl = CParaFormat::ELineSpacingExactlyInTwips;
	paraFormat.iLineSpacingInTwips = 115;
	// paraFormat.iHorizontalAlignment = aAlignment;
	aInto.ApplyParaFormatL(&paraFormat, paraFormatMask, pos, aText.Length());

}

void UserFriendlyIntoRichText(CRichText& aInto, const MErrorInfo& aFrom)
{
	TBuf<14> pre;
	TRgb color;
	switch (aFrom.Severity()) {
	case EInfo:
		pre=_L("[Info] ");
		color=KRgbBlue;
		break;
	case EWarning:
		pre=_L("[Warning] ");
		color=KRgbYellow;
		break;
	case EError:
		pre=_L("[Error] ");
		color=KRgbRed;
		break;
	case ECorrupt:
		pre=_L("[Critical] ");
		color=KRgbDarkRed;
		break;
	}

	AppendFormatted(aInto, pre, color);
	AppendFormatted(aInto, aFrom.UserMessage(), color);
	aInto.InsertL(aInto.DocumentLength(), CEditableText::EParagraphDelimiter);

	switch (aFrom.ErrorType()) {
	case EBug:
		AppendFormatted(aInto, _L("Contact support."), color);
		break;
	case EInputData:
		AppendFormatted(aInto, _L("Please correct the input."), color);
		break;
	case ETemporary:
		AppendFormatted(aInto, _L("Please try again."), color);
		break;
	case ELocalEnvironment:
		AppendFormatted(aInto, _L("There is a problem with the setup on the phone."), color);
		break;
	case ERemote:
		AppendFormatted(aInto, _L("There is a problem with the remote system."), color);
		break;
	}
	aInto.InsertL(aInto.DocumentLength(), CEditableText::EParagraphDelimiter);

}

void TechnicalIntoRichText(CRichText& aInto, const MErrorInfo& aFrom)
{
	AppendFormatted(aInto, _L("Technical information:"), KRgbBlack);
	aInto.InsertL(aInto.DocumentLength(), CEditableText::EParagraphDelimiter);
	AppendFormatted(aInto, aFrom.TechMessage(), KRgbBlack);
	aInto.InsertL(aInto.DocumentLength(), CEditableText::EParagraphDelimiter);

	AppendFormatted(aInto, _L("Error code: "), KRgbBlack);
	TBuf<24> code; code.AppendNum( aFrom.ErrorCode().iUid, EHex );
	code.Append(_L(" "));
	code.AppendNum( aFrom.ErrorCode().iCode );
	AppendFormatted(aInto, code, KRgbBlack);
	aInto.InsertL(aInto.DocumentLength(), CEditableText::EParagraphDelimiter);

	AppendFormatted(aInto, _L("Stack trace: "), KRgbBlack);
	aInto.InsertL(aInto.DocumentLength(), CEditableText::EParagraphDelimiter);
	AppendFormatted(aInto, aFrom.StackTrace(), KRgbBlack);
	aInto.InsertL(aInto.DocumentLength(), CEditableText::EParagraphDelimiter);
}

EXPORT_C void ErrorIntoRichTextL(CRichText& aInto, const MErrorInfo& aFrom)
{
	TInt	nodecount=0;

	const MErrorInfo* list_i=&aFrom;
	while (list_i) {
		const MErrorInfo* nest_i=list_i;
		while (nest_i) {
			nodecount++;
			UserFriendlyIntoRichText(aInto, *nest_i);
			nest_i=nest_i->InnerError();
		}
		list_i=list_i->NextError();
	}
	if (nodecount==1) {
		TechnicalIntoRichText(aInto, aFrom);
	} else {
		list_i=&aFrom;
		while (list_i) {
			const MErrorInfo* nest_i=list_i;
			while (nest_i) {
				nodecount++;
				UserFriendlyIntoRichText(aInto, *nest_i);
				TechnicalIntoRichText(aInto, *nest_i);
				nest_i=nest_i->InnerError();
			}
			list_i=list_i->NextError();
		}
	}
}

class CErrorInfoContainerImpl: public CErrorInfoContainer, public MContextBase {
public:
	void ConstructL(const TRect& aRect) {
		CreateWindowL();

		iText=new (ELeave) CEikRichTextEditor();
#ifndef __S80__
		iText->SetMopParent(this);
#endif
		const TInt edwinFlags = EEikEdwinInclusiveSizeFixed|
			EEikEdwinNoAutoSelection|
			EEikEdwinDisplayOnly|
			EEikEdwinReadOnly|
			EEikEdwinLineCursor|
			EEikEdwinNoHorizScrolling
#ifndef __S80__
		| EEikEdwinAvkonDisableCursor;
#else
			;
#endif
		iText->ConstructL( this, 10, 2000, edwinFlags );
#ifndef __S80__
		iText->SetAknEditorFlags(edwinFlags);
#endif
		iText->SetContainerWindowL(*this);

		TRect r=aRect; r.iTl.iX=0; r.iTl.iY=0;
		iText->SetRect( r );
		iText->CreatePreAllocatedScrollBarFrameL();
		iText->SetReadOnly(ETrue);
		iText->SetFocusing(ETrue);
		iText->SetFocus(ETrue);
		iText->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, 
			CEikScrollBarFrame::EAuto);

		SetRect(aRect);
		ActivateL();
	}
	TInt CountComponentControls() const {
		return 1;
	}
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType )
	{
		switch(aKeyEvent.iCode) {
		case EKeyUpArrow:{
			TInt pos=iText->CursorPos();
			iText->MoveCursorL(TCursorPosition::EFPageUp, EFalse);
			iText->MoveCursorL(TCursorPosition::EFLineBeg, EFalse);
			if (pos!=iText->CursorPos()) {
				iText->MoveDisplayL(TCursorPosition::EFPageUp);
				iText->DrawNow();
			}
			iText->UpdateScrollBarsL();
			iText->ScrollBarFrame()->DrawScrollBarsNow();
			return EKeyWasConsumed;
		}
			
		case EKeyDownArrow:{
			TInt pos=iText->CursorPos();
			iText->MoveCursorL(TCursorPosition::EFPageDown, EFalse);
			iText->MoveCursorL(TCursorPosition::EFLineBeg, EFalse);
			if (pos!=iText->CursorPos()) {
				iText->MoveDisplayL(TCursorPosition::EFPageDown);
				iText->DrawNow();
			}
			iText->UpdateScrollBarsL();
			iText->ScrollBarFrame()->DrawScrollBarsNow();
			return EKeyWasConsumed;
		}
			
		default:
			break;
		}
		
		return EKeyWasNotConsumed;
	}   
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		return iText;
	}

	void HandleTextChangedL() {
		iText->HandleTextChangedL();
		iText->UpdateScrollBarsL();
		iText->ScrollBarFrame()->DrawScrollBarsNow();
	}
	CRichText* RichText() {
		return iText->RichText();
	}
	TInt TextLength() {
		return iText->TextLength();
	}
	~CErrorInfoContainerImpl() {
		delete iText;
	}
private:
	CEikRichTextEditor* iText;
};

EXPORT_C CErrorInfoContainer* CErrorInfoContainer::NewL(MObjectProvider* aMopParent, const TRect& aRect)
{
	auto_ptr<CErrorInfoContainerImpl> self( new (ELeave) CErrorInfoContainerImpl );
#ifndef __S80__
	self->SetMopParent(aMopParent);
#endif
	self->ConstructL( aRect );
	return self.release();
}

class CErrorInfoViewImpl : public CErrorInfoView, public MContextBase {
private:
	CErrorInfoViewImpl();
	void ConstructL();

	virtual void ShowWithData(const MErrorInfo& aInfo);
	
	TUid Id() const;

        void HandleCommandL(TInt aCommand);
        void DoActivateL(const TVwsViewId& aPrevViewId,
		TUid aCustomMessageId,
		const TDesC8& aCustomMessage);
	virtual void HandleResourceChange( TInt aType );
        void DoDeactivate();
	void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
	void OpenInSendL(TInt aCommand);

	void TmpFileNameL(TFileName& aFileName);
	void WriteErrorInfoToFileL(const TDesC& aFileName);


	friend class CErrorInfoView;


	MErrorInfo*		iErrorInfo;
	TVwsViewId	iPrevView;
	TInt		iResource;
	CErrorInfoContainer* iContainer;
#ifndef __S60V3__
	CSendAppUi*	iSendUi;
#else
	CSendUi* iSendUi;
#endif

	void ReleaseCErrorInfoViewImpl(void);
public:
	virtual ~CErrorInfoViewImpl();
};

EXPORT_C CErrorInfoView* CErrorInfoView::NewL()
{
	CALLSTACKITEM_N(_CL("CErrorInfoView"), _CL("NewL"));

	auto_ptr<CErrorInfoViewImpl> ret(new (ELeave) CErrorInfoViewImpl);
	ret->ConstructL();
	return ret.release();
}
CErrorInfoViewImpl::CErrorInfoViewImpl() { }

void CErrorInfoViewImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CErrorInfoViewImpl"), _CL("ConstructL"));
	iResource=LoadSystemResourceL(iEikonEnv, _L("contextui"));
	BaseConstructL( R_ERRORINFO_VIEW );

}

TUid CErrorInfoViewImpl::Id() const {
	return KErrorInfoView;
}

void CErrorInfoViewImpl::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CErrorInfoViewImpl"), _CL("HandleCommandL"));
	if (aCommand==EAknSoftkeyBack || aCommand==EAknSoftkeyClose) {
		ActivateViewL(iPrevView);
	} else {
		OpenInSendL(aCommand);
	}
}

void CErrorInfoViewImpl::OpenInSendL(TInt aCommandId)
{
#ifndef __S60V3__
	TUid mtm_uid=iSendUi->MtmForCommand(aCommandId) ;
	TSendingCapabilities cap=iSendUi->MtmCapabilitiesL(mtm_uid);
	CRichText* body=0;

	auto_ptr<CDesCArrayFlat> attach(new (ELeave) CDesCArrayFlat(1));
	TFileName body_filen;

	if (cap.iFlags & TSendingCapabilities::ESupportsBodyText) {
		body=iContainer->RichText();
	} else {
		TFileName body_filen;
		TmpFileNameL( body_filen );
		WriteErrorInfoToFileL( body_filen );
		attach->AppendL(body_filen);
	}

#ifndef __S80__
	iSendUi->CreateAndSendMessageL(aCommandId, body, attach.get(),
		KNullUid, 0, 0, EFalse);
#else
	iSendUi->CreateAndSendMessageL(aCommandId, body, attach.get(),
		KNullUid, 0, 0);
#endif

#else // 3rd ed 
	TUid mtm=KNullUid;
	
	TInt flags = TSendingCapabilities::EAllServices;
	mtm = iSendUi->ShowSendQueryL(0, TSendingCapabilities(0, 0, flags), 0, _L("Send logs"));
	
	TSendingCapabilities caps; 
	iSendUi->ServiceCapabilitiesL( mtm, caps );
	
	auto_ptr<CMessageData> data( CMessageData::NewL() );
	
	if ( caps.iFlags & TSendingCapabilities::ESupportsAttachments )
		{
			TFileName filename;
			TmpFileNameL( filename );
			WriteErrorInfoToFileL( filename );
			data->AppendAttachmentL( filename );
		}
	else
		{
			CRichText* body=iContainer->RichText();
			data->SetBodyTextL( body );
		}
	iSendUi->CreateAndSendMessageL(mtm, data.get(), KNullUid, EFalse);
#endif // 3rd
}


void CErrorInfoViewImpl::TmpFileNameL(TFileName& aFileName)
{
	aFileName=DataDir();
	aFileName.Append( _L("errorinfo") );
	aFileName.Append(_L(".txt"));
}

void CErrorInfoViewImpl::WriteErrorInfoToFileL(const TDesC& aFileName)
{
	Fs().Delete( aFileName);
	RAFile f;
	f.ReplaceLA(Fs(), aFileName, EFileWrite|EFileShareAny);
	const TDesC& errortext = iContainer->RichText()->Read(0, iContainer->TextLength());
	
	TPtrC8 body( (TText8*)errortext.Ptr(), errortext.Length()*2);
	User::LeaveIfError(f.Write(body));
}

void CErrorInfoViewImpl::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane)
{
	if (aResourceId == R_ERRORUI_MENU) 
		{
#ifndef __S60V3__
		TInt flags=TSendingCapabilities::ESupportsAttachmentsOrBodyText;
		TSendingCapabilities c( 0, 0, flags);
		iSendUi->DisplaySendMenuItemL(*aMenuPane, 0, c);
#else
		TInt position = 0;
		iSendUi->AddSendMenuItemL(*aMenuPane, position, EContextUICmdSendUi, 
								  TSendingCapabilities(0, 0, TSendingCapabilities::EAllServices));
		
#endif
	}

#ifndef __S60V3__
	if (aResourceId == R_SENDUI_MENU) 
		{
		iSendUi->DisplaySendCascadeMenuL(*aMenuPane);
		}
#endif
}

void CErrorInfoViewImpl::DoActivateL(const TVwsViewId& aPrevViewId,
	TUid /*aCustomMessageId*/,
	const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CErrorInfoViewImpl"), _CL("DoActivateL"));

	if (! iSendUi ) {
#ifndef __S60V3__
		iSendUi = CSendAppUi::NewL( CSendAppUi::NewL(EContextUICmdSendUi, 0) );
#else
		iSendUi = CSendUi::NewL();
#endif
	}
	
	iPrevView=aPrevViewId;
	if (!iContainer) {
		iContainer= CErrorInfoContainer::NewL( this, ClientRect() );
		
		if (iErrorInfo) {
			ErrorIntoRichTextL( *iContainer->RichText(), *iErrorInfo );
			iContainer->HandleTextChangedL();
			delete iErrorInfo; iErrorInfo = 0;
		}
		AppUi()->AddToStackL( *this, iContainer );
        } 
}

void CErrorInfoViewImpl::HandleResourceChange( TInt aType ) {
  if ( aType == KEikDynamicLayoutVariantSwitch ) {
    if ( iContainer ) {
      TRect r = ClientRect();
      iContainer->SetRect( r );
    }
  }
}

void CErrorInfoViewImpl::DoDeactivate()
{
	CALLSTACKITEM_N(_CL("CErrorInfoViewImpl"), _CL("DoDeactivate"));

	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer; iContainer = 0;
	delete iErrorInfo; iErrorInfo = 0;
}

CErrorInfoViewImpl::~CErrorInfoViewImpl() {
	CC_TRAPD(err, ReleaseCErrorInfoViewImpl());
	if (err!=KErrNone) {
		User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}
}
void CErrorInfoViewImpl::ReleaseCErrorInfoViewImpl(void)
{
	CALLSTACKITEM_N(_CL("CErrorInfoViewImpl"), _CL("ReleaseCErrorInfoViewImpl"));

	if (iResource) iEikonEnv->DeleteResourceFile(iResource);
	delete iContainer;
	delete iErrorInfo;
	delete iSendUi;
}

void CErrorInfoViewImpl::ShowWithData(const MErrorInfo& aInfo)
{
	delete iErrorInfo; iErrorInfo=0;
	iErrorInfo=aInfo.CreateCopyL();
	AppUi()->ActivateLocalViewL(KErrorInfoView);
}
