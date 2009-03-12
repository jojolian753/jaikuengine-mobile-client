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

#include "uploadview.h"

#include "contextlog_resource.h"
#include <aknviewappui.h>
#include <bautils.h>
#include <eikedwin.h>
#include <eiklabel.h>
#include "symbian_auto_ptr.h"
#include "context_log.hrh"
#include "cl_settings.h"
#include "MdaImageConverter.h"
#include "cm_post.h"
#include "app_context_impl.h"
#include "reporting.h"
#include "timeout.h"

enum KEYCODES {
	JOY_LEFT = 0xF807,
	JOY_RIGHT = 0xF808,
	JOY_UP = 0xF809,
	JOY_DOWN = 0xF80A,
	JOY_CLICK = 0xF845,
	KEY_CALL = 0xF862,
	KEY_CANCEL = 0xF863
}; 

_LIT(KTag, "Tag");
_LIT(KDescription, "Description");

#define IMG_HEIGHT	40

class CUploadContainer : public CCoeControl, public MEikEdwinObserver, public MContextBase,
	public MMdaImageUtilObserver {
public:
	CUploadContainer(MApp_context& Context, const TDesC& FileName, bool TagOnly);
	~CUploadContainer();
	void ConstructL(const TRect& aRect);

	void GetFieldsLC(HBufC*& Tag, HBufC*& Description);
	void CloseFile();
private:
        void SizeChanged();
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
	void HandleEdwinEventL(CEikEdwin* aEdwin,TEdwinEvent aEventType);

	virtual void MiuoConvertComplete(TInt aError);
	virtual void MiuoCreateComplete(TInt aError);
	virtual void MiuoOpenComplete(TInt aError);

	void SetEditSelected(CEikEdwin *Edit, bool IsFocused);
	void SetEditActive(CEikEdwin *Edit, bool IsActive);
	void ShowImageError(const TDesC& Descr, TInt Code);

	CArrayPtrFlat< CCoeControl > *iControls;
	CEikEdwin *iTagEdit, *iDescriptionEdit;
	//CEikLabel *iTagLabel, *iDescriptionLabel;

	CEikLabel *iImgPlaceHolder;

	CEikEdwin *iSelected, *iActive;
	const TDesC&	iFileName;
#ifndef __S60V2__
	CMdaServer*	iMdaServer;
#endif
	CMdaImageFileToBitmapUtility*	iFileUtil;
	CMdaBitmapScaler*		iScaler;

	CFbsBitmap	*iOrigBitmap, *iScaledBitmap; bool scaled;

	bool	iTagOnly;
};

class CUploadViewImpl : public CUploadView, public MContextBase, public MCoeForegroundObserver,
	public MTimeOut {
private:

	struct TCallBackItem {
		TFileName		iFileName;
		MUploadCallBack*	iCallBack;
		TCallBackItem() : iCallBack(0) { }
		TCallBackItem(const TDesC& aFileName, MUploadCallBack* aCallBack) :
			iFileName(aFileName), iCallBack(aCallBack) { }
	};

	CUploadViewImpl(MApp_context& Context, const CBBPresence* Presence, 
		TVwsViewId* NextViewId, TUid Id, bool TagOnly, CDiscover* aDiscover);
	void ConstructL();

        TUid Id() const;
	
        void HandleCommandL(TInt aCommand);
        void DoActivateL(const TVwsViewId& aPrevViewId,
		TUid aCustomMessageId,
		const TDesC8& aCustomMessage);
        void DoDeactivate();

	//MCoeForegroundObserver
	void HandleGainingForeground();
	void HandleLosingForeground();

	void expired(CBase*);

	friend class CUploadView;
	CUploadContainer* iContainer;
	TVwsViewId	iPrevView;
	virtual void Prompt(const TDesC& FileName, MUploadCallBack* CallBack);
	MBBData* MakePacketLC();

	TFileName	iFileName;
	MUploadCallBack* iCallBack;

	CList<TCallBackItem> *iCallBacks;
	CTimeOut*	iTimer;

	const CBBPresence	*iPresence;
	TVwsViewId*	iNextViewId;
	bool		iNext;
	TUid		iId;
	bool		iTagOnly;
	CDiscover*	iDiscover;
	TInt		iTimerWait;
public:
	virtual ~CUploadViewImpl();
};

CUploadView* CUploadView::NewL(MApp_context& Context, const CBBPresence* Presence, 
		TVwsViewId* NextViewId, TUid Id, bool TagOnly, CDiscover* aDiscover)
{
	CALLSTACKITEM_N(_CL("CUploadView"), _CL("NewL"));

	auto_ptr<CUploadViewImpl> ret(new (ELeave) CUploadViewImpl(Context, Presence, 
		NextViewId, Id, TagOnly, aDiscover));
	ret->ConstructL();
	return ret.release();
}

CUploadContainer::CUploadContainer(MApp_context& Context, const TDesC& FileName, bool TagOnly) : 
	MContextBase(Context), iFileName(FileName), iTagOnly(TagOnly)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("CUploadContainer"));

}

void CUploadContainer::GetFieldsLC(HBufC*& Tag, HBufC*& Description)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("GetFieldsLC"));

	const TDesC& tag=iTagEdit->Text()->Read(0);
	delete Tag; Tag=0;
	if (tag.Length()>0 && tag.Left(tag.Length()-1).CompareF(KTag)) Tag=tag.Left(tag.Length()-1).AllocLC();
	else Tag=HBufC::NewLC(0);

	delete Description; Description=0;
	if (iDescriptionEdit) {
		const TDesC& desc=iDescriptionEdit->Text()->Read(0);
	
		if (desc.Length()>0 && desc.Left(desc.Length()-1).CompareF(KDescription)) 
			Description=desc.Left(desc.Length()-1).AllocLC();
		else
			Description=HBufC::NewLC(0);
	} else {
		Description=HBufC::NewLC(0);
	}
}

CUploadContainer::~CUploadContainer()
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("~CUploadContainer"));

	if (iTagEdit) {
		const TDesC& tag=iTagEdit->Text()->Read(0);
		if (tag.Length()>0) {
			Settings().WriteSettingL(SETTING_UPLOAD_TAG, tag.Left(tag.Length()-1));
		} else {
			Settings().WriteSettingL(SETTING_UPLOAD_TAG, _L(""));
		}
	}
	if (iFileUtil)
		iFileUtil->Close();
	delete iFileUtil;
	if (iControls) iControls->ResetAndDestroy();
	delete iControls;

	delete iScaler;
	delete iOrigBitmap; 
#ifndef __S60V2__
	delete iMdaServer;
#endif
}

void CUploadContainer::HandleEdwinEventL(CEikEdwin* /*aEdwin*/, TEdwinEvent /*aEventType*/)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("HandleEdwinEventL"));

}

void CUploadContainer::ShowImageError(const TDesC& Descr, TInt Code)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("ShowImageError"));

	if (!iImgPlaceHolder) return;

	iImgPlaceHolder->MakeVisible(ETrue);

	TBuf<30> msg;
	msg.Append(Descr); msg.Append(_L(": "));
	msg.AppendNum(Code);
	iImgPlaceHolder->SetTextL(msg);
}

void CUploadContainer::MiuoConvertComplete(TInt aError)
{
	// FIXME: error handling

	if (!iScaledBitmap) {
		iScaler=CMdaBitmapScaler::NewL();
		iScaledBitmap=iOrigBitmap;
		iScaler->ScaleL(*this, *iScaledBitmap,
			TSize(100, IMG_HEIGHT));
	} else {
		scaled=true;
		iImgPlaceHolder->MakeVisible(EFalse);
		DrawNow();
	}
}

void CUploadContainer::MiuoCreateComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("MiuoCreateComplete"));

}

void CUploadContainer::MiuoOpenComplete(TInt aError)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("MiuoOpenComplete"));

	if (aError!=KErrNone) {
		TBuf<40> msg;
		msg.Format(_L("error opening pic %d"), aError);
		iImgPlaceHolder->SetTextL(msg);
	} else {
		TFrameInfo frameInfo;
		iFileUtil->FrameInfo(0, frameInfo);
		iOrigBitmap=new (ELeave) CFbsBitmap;
		iOrigBitmap->Create(frameInfo.iOverallSizeInPixels, EColor4K);
		iFileUtil->ConvertL(*iOrigBitmap);
	}
}

TKeyResponse CUploadContainer::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("OfferKeyEventL"));

	// lessen chance of accidental dismissal
	if (aKeyEvent.iCode == KEY_CANCEL) return EKeyWasConsumed;

	if (! iDescriptionEdit) {
		if (iActive) {
			return iActive->OfferKeyEventL(aKeyEvent, aType);
		} else {
			return EKeyWasNotConsumed;
		}
	}

	if (aKeyEvent.iCode==JOY_CLICK) {
		SetEditActive(iSelected, iActive==0);
		return EKeyWasConsumed;
	}
	else if(iActive) 
	{
		TKeyResponse ret;
		ret=iActive->OfferKeyEventL(aKeyEvent, aType);
		return ret;
	} else if (aKeyEvent.iCode==JOY_UP || aKeyEvent.iCode==JOY_DOWN ||
		aKeyEvent.iCode==JOY_LEFT || aKeyEvent.iCode==JOY_RIGHT  ) {
		CEikEdwin *prev=iSelected;
		SetEditSelected(iSelected, false);
		if (prev==iTagEdit) {
			SetEditSelected(iDescriptionEdit, true);
		} else {
			SetEditSelected(iTagEdit, true);
		}
		return EKeyWasConsumed;
	} else {
		return EKeyWasNotConsumed;
	}
}

void CUploadContainer::ConstructL(const TRect& aRect)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("ConstructL"));

	iControls=new (ELeave) CArrayPtrFlat< CCoeControl >(10);
	CreateWindowL();

	TRect r(TPoint(5, 5), TSize(aRect.Width()-10, 1));

	if (iFileName.Right(3).CompareF(_L("jpg"))==0) {
#ifndef __S60V2__
		iMdaServer=CMdaServer::NewL();
		iFileUtil=CMdaImageFileToBitmapUtility::NewL(*this, iMdaServer);
#else
		iFileUtil=CMdaImageFileToBitmapUtility::NewL(*this, 0);
#endif
		iFileUtil->OpenL(iFileName);
		scaled=false;
		iImgPlaceHolder=new (ELeave) CEikLabel;
		iControls->AppendL(iImgPlaceHolder);
		iImgPlaceHolder->SetContainerWindowL( *this );
		iImgPlaceHolder->SetFont(iEikonEnv->DenseFont());
		iImgPlaceHolder->SetTextL( _L("loading image...") ); 
		r.SetHeight(IMG_HEIGHT);
		iImgPlaceHolder->SetRect(r);
		r.Move(0, r.Height()+4);
	} else {
		r.SetHeight(IMG_HEIGHT);
		r.Move(0, r.Height()+4);
	}

	TInt tagw=0;
	/*
	iTagLabel=new (ELeave) CEikLabel;
	iControls->AppendL(iTagLabel);
	iTagLabel->SetContainerWindowL( *this );
	iTagLabel->SetTextL( _L("Tag:") );
	r.SetHeight(12);
	iTagLabel->SetRect(r);
	tagw=iTagLabel->MinimumSize().iWidth+4;
	r.Move( tagw , 0); r.SetWidth(r.Width()-tagw);
	*/

	TBuf<100> tag;
	Settings().GetSettingL(SETTING_UPLOAD_TAG, tag);
	if (tag.Length() == 0) tag=KTag();

	iTagEdit=new (ELeave) CEikEdwin;
	iControls->AppendL(iTagEdit);
	iTagEdit->SetContainerWindowL( *this );
	iTagEdit->ConstructL();
	iTagEdit->SetTextL(&tag);
	iTagEdit->AddEdwinObserverL(this);
	r.SetHeight(16);
	iTagEdit->SetRect(r);
	iTagEdit->ActivateL();
	r.Move(-tagw, r.Height()+4); r.SetWidth(r.Width()+tagw);


	TBool no_descr;
	if (iTagOnly) 
		no_descr=true;
	else
		no_descr=false;
	
	if (!no_descr) {
		TInt height=56;
		/*
		iDescriptionLabel=new (ELeave) CEikLabel;
		iControls->AppendL(iDescriptionLabel);
		iDescriptionLabel->SetContainerWindowL( *this );
		iDescriptionLabel->SetTextL( _L("Description:") );
		r.SetHeight(12);
		iDescriptionLabel->SetRect(r);
		r.Move(0, r.Height()+4);
		height-=16;
		*/
		r.Move(0, 2);

		iDescriptionEdit=new (ELeave) CEikEdwin;
		iControls->AppendL(iDescriptionEdit);
		iDescriptionEdit->SetContainerWindowL( *this );
		iDescriptionEdit->ConstructL();
		iDescriptionEdit->AddEdwinObserverL(this);
		iDescriptionEdit->SetTextL(&(KDescription()));
		r.SetHeight(height);
		iDescriptionEdit->SetRect(r);
		r.Move(0, r.Height()+4);

		SetEditSelected(iDescriptionEdit, true); SetEditActive(iDescriptionEdit, true);
	} else {
		SetEditSelected(iTagEdit, true); SetEditActive(iTagEdit, true);
	}

	SetRect(aRect);
	ActivateL();	
}

void CUploadContainer::SetEditSelected(CEikEdwin *Edit, bool IsSelected)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("SetEditSelected"));

	if (IsSelected) 
		iSelected=Edit;
	else
		iSelected=0;
	DrawNow();
}

void CUploadContainer::SetEditActive(CEikEdwin *Edit, bool IsActive)
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("SetEditActive"));

	TBuf<20> label;
	if (Edit==iDescriptionEdit) label=KDescription();
	else label=KTag();

	const TDesC& text=Edit->Text()->Read(0).Left(Edit->Text()->Read(0).Length()-1);
	if (IsActive) {
		iActive=Edit;
		iActive->SetFocus(ETrue);
		if (! text.CompareF(label) ) {
			Edit->SetTextL(&KNullDesC);
		}
	} else {
		if (iActive) iActive->SetFocus(EFalse);
		if (text.Length()==0) Edit->SetTextL(&label);

		iActive=0;
	}
	DrawNow();
}

void CUploadContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("SizeChanged"));

}

TInt CUploadContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("CountComponentControls"));

	return iControls->Count();
}

CCoeControl* CUploadContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("ComponentControl"));

	return iControls->At(aIndex);
}

void CUploadContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CUploadContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc.DrawRect(aRect);

	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	gc.SetBrushStyle(CGraphicsContext::ENullBrush);

	TGulBorder border(TGulBorder::ESingleGray);
	TRect edit_rect=iTagEdit->Rect();
	edit_rect.Resize(4, 4);
	edit_rect.Move(-2, -2);
	border.Draw(gc, edit_rect);

	if (iDescriptionEdit) {
		edit_rect=iDescriptionEdit->Rect();
		edit_rect.Resize(4, 4);
		edit_rect.Move(-2, -2);
		border.Draw(gc, edit_rect);
	}

	if (iSelected) {
		TGulBorder border(TGulBorder::ESingleBlack);
		//gc.SetPenColor(KRgbBlack);
		TRect edit_rect=iSelected->Rect();
		edit_rect.Resize(4, 4);
		edit_rect.Move(-2, -2);
		border.Draw(gc, edit_rect);
	}
	if (iActive) {
		TGulBorder border(TGulBorder::EFocusedSunkenControl);
		//gc.SetPenColor(KRgbBlack);
		TRect edit_rect=iActive->Rect();
		edit_rect.Resize(4, 4);
		edit_rect.Move(-2, -2);
		border.Draw(gc, edit_rect);
	}

	if (scaled) {
		TSize s=iScaledBitmap->SizeInPixels();
		TPoint lt=TPoint( (Rect().Width()-s.iWidth)/2, 4);
		TRect r( lt, s);
		gc.DrawBitmap(r, iScaledBitmap);
	}
}

CUploadViewImpl::CUploadViewImpl(MApp_context& Context, const CBBPresence* Presence, 
		TVwsViewId* NextViewId, TUid Id, bool TagOnly, CDiscover* aDiscover) : MContextBase(Context),
		iPresence(Presence), iNextViewId(NextViewId), iId(Id), iTagOnly(TagOnly), iDiscover(aDiscover)
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("CUploadViewImpl"));

}

void CUploadViewImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("ConstructL"));

	iTimer=CTimeOut::NewL(*this);
	iTimerWait=1;
	iCallBacks=CList<TCallBackItem>::NewL();

	BaseConstructL( R_UPLOAD_VIEW );
}

TUid CUploadViewImpl::Id() const
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("Id"));

	return iId;
}

void GetImei(TDes& aInto)
{
#ifndef __WINS__
	TPlpVariantMachineId machineId;
	PlpVariant::GetMachineIdL(machineId);
	aInto=machineId;
#else
	// Return a fake IMEI when working on emulator
	_LIT(KEmulatorImsi, "244050000000000");
	aInto=KEmulatorImsi;
#endif
}

MBBData* CUploadView::MakePacketL(const TDesC& tag, const TDesC& description,
	MApp_context& Context, const CBBPresence* Presence, CDiscover* Discover)
{
	auto_ptr<CCMPost> buf(CCMPost::NewL(0));

	buf->iBodyText->Append(description);
	buf->iTag->Append(tag);
	TTime t=GetTime();
	buf->iTimeStamp()=t;
	buf->iParentId()=9000;

	TBuf<50> name;
	Context.Settings().GetSettingL(SETTING_PROJECT_NAME, name);
	buf->iProject()=name;
	name.Zero();
	Context.Settings().GetSettingL(SETTING_JABBER_NICK, name);
	buf->iSender.iJabberNick()=name;
	name.Zero();
	Context.Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, name);
	buf->iSender.iName()=name;
	name.Zero();
	Context.Settings().GetSettingL(SETTING_PHONENO, name);
	buf->iSender.iPhoneNo()=name;
	GetImei(buf->iSender.iImei());
	if (Discover) {
		buf->iSender.iBt=TBBBtDeviceInfo(Discover->GetOwnAddress().Des(), _L(""),  0, 0, 0);
	}
	buf->iPresence()=bb_cast<CBBPresence>(Presence->CloneL(KNullDesC));

	return buf.release();
}

MBBData* CUploadViewImpl::MakePacketLC()
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("MakePacketLC"));

	HBufC *tag=0, *desc=0;
	iContainer->GetFieldsLC(tag, desc);

	MBBData* buf=CUploadView::MakePacketL(*tag, *desc, AppContext(),
		iPresence, iDiscover);
	CleanupStack::PopAndDestroy(2);

	CleanupPushBBDataL(buf);
	return buf;
}

void CUploadContainer::CloseFile()
{
	if (iFileUtil) {
		iFileUtil->Close();
	}
	delete iFileUtil; iFileUtil=0;
}

void CUploadViewImpl::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("HandleCommandL"));

	iNext=false; 
	MUploadCallBack* cb=iCallBack; iCallBack=0;
	if (cb) {
		switch(aCommand) {
		case Econtext_logCmdSoftkeyUpload:
			{
			TBool del=ETrue;
			Settings().GetSettingL(SETTING_DELETE_UPLOADED, del);
			MBBData* buf=MakePacketLC();
			iContainer->CloseFile();
			cb->Back(true, del, buf);
			Reporting().ShowGlobalNote(EAknGlobalConfirmationNote, _L("Queued for Upload"));
			CleanupStack::PopAndDestroy();
			}
			break;
		case Econtext_logCmdSoftkeyCancel:
			iContainer->CloseFile();
			cb->Back(false, false, 0);
			Reporting().ShowGlobalNote(EAknGlobalConfirmationNote, _L("Moved to NotShared"));
			break;
		default:
			return;
			break;
		}
	}
	if (!iNext && iCallBacks->iCount > 0) {
		iCallBack=0;
		TCallBackItem i=iCallBacks->Pop();
		Prompt(i.iFileName, i.iCallBack);
	}
	if (!iNext) {
		// if no next, just display previous view
		iCallBack=0;
		TUid statusv={1};
		AppUi()->ActivateLocalViewL(statusv);
		//CSwitchBack::NewL(iPrevView);
		*iNextViewId=iPrevView;
	} else {
		//if iNext -> save callback, remove view, reload it, faking previous view id 
		MUploadCallBack* cb=iCallBack; iCallBack=0;
		DoDeactivate();
		TUid dummy={0};
		iCallBack=cb;
		DoActivateL(iPrevView, dummy, _L8(""));
	}
}

void CUploadViewImpl::Prompt(const TDesC& FileName, MUploadCallBack* CallBack)
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("Prompt"));
	*iNextViewId=TVwsViewId();

	if (!iCallBack) {
		iFileName=FileName;
		iCallBack=CallBack;
		if (iContainer) iNext=true;
		else AppUi()->ActivateLocalViewL(iId);
	} else {
		iCallBacks->AppendL(TCallBackItem(FileName, CallBack));
	}
}

void CUploadViewImpl::DoActivateL(const TVwsViewId& aPrevViewId,
	TUid /*aCustomMessageId*/,
	const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("DoActivateL"));

	iEikonEnv->AddForegroundObserverL(*this);
	iTimerWait=1;
	iTimer->Cancel();
	iPrevView=aPrevViewId;
	if (!iContainer) {
		iContainer=new (ELeave) CUploadContainer(AppContext(), iFileName, iTagOnly);
		iContainer->SetMopParent(this);
		iContainer->ConstructL(ClientRect());
		AppUi()->AddToStackL( *this, iContainer );
        } 
}

void CUploadViewImpl::DoDeactivate()
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("DoDeactivate"));

	iTimer->Cancel();
	iEikonEnv->RemoveForegroundObserver(*this);
	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
	iContainer = 0;
	if (iCallBack) iCallBack->Back(false, false, 0);
	iCallBack=0;
}

void CUploadViewImpl::HandleGainingForeground()
{
	iTimer->Cancel();
}
void CUploadViewImpl::HandleLosingForeground()
{
	iTimer->Wait(iTimerWait);
	iTimerWait*=2;
}
void CUploadViewImpl::expired(CBase*)
{
	AppUi()->ActivateLocalViewL(iId);
}

CUploadViewImpl::~CUploadViewImpl()
{
	CALLSTACKITEM_N(_CL("CUploadViewImpl"), _CL("~CUploadViewImpl"));

	delete iTimer;
	delete iCallBacks;
	delete iContainer;
}
