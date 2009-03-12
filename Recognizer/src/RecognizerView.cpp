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

#include "RecognizerView.h"
#include <recognizer.rsg>
#include "symbian_auto_ptr.h"
#include "app_context.h"
#include <aknquerydialog.h> 
#include <avkon.hrh>
#include <aknviewappui.h>
#include "VisualCodeSystem.h"
#include <bautils.h>
#include "recognizerview.hrh"
#include <aknnavi.h> 
#include <akntitle.h>
#include "timeout.h"
#include "reporting.h"
#include "Contextnotifyclientsession.h"
#include <eiklabel.h>

enum KEYCODES {
	JOY_LEFT = 0xF807,
	JOY_RIGHT = 0xF808,
	JOY_UP = 0xF809,
	JOY_DOWN = 0xF80A,
	JOY_CLICK = 0xF845,
	KEY_CALL = 0xF862,
	KEY_CANCEL = 0xF863
};

const TUint KCameraKey = 0xF887;

class CRecognizerContainer : public CCoeControl, MVisualCodeObserver, MContextBase {
public: // Constructors and destructor
	CRecognizerContainer(MApp_context& Context);
	void ConstructL(const TRect& aRect, CAknView* View, TBool navi);
	~CRecognizerContainer();

public: // New functions
	CCodeInfo* GetCode();
public:
	void StartDisplay();
	void StopDisplay();

private:
	void DrawImage(const CFbsBitmap* aImage) const;
	inline void DrawCross(CWindowGc& gc, const TPoint& center) const;
	void ShowCodes(const TRect& aDrawRect, const TSize& aBmpSize) const;

private:
	void SizeChanged();
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void Draw(const TRect& aRect) const;
	void ViewFinderStartStop();
        virtual TKeyResponse OfferKeyEventL(
		const TKeyEvent& aKeyEvent,TEventCode aType);
private:
    	CVisualCodeSystem* iVCS;
	CFbsBitmap* iBitmap; bool owns_bitmap;
	TBool iTakingPicture;
	TInt iCurrentCode;
	CCodeInfo* iCurrentCodeInfo;
	bool	iInvalid;
	RPointerArray<CCodeInfo>* iCurrentCodeArray;
	TPoint iTranslation;
	TPoint iTarget;
	TRealPoint iTilting;
	TInt iMagnification;
	bool	started;
	CAknView*	iView;
	TBool iNavi;
	HBufC * iScreenMsgBuf;
	CEikLabel * iStandByLabel;

private: // MVisualCodeObserver

	virtual void PictureUpdateL(
		CFbsBitmap& aBitmap, 
		RPointerArray<CCodeInfo>* aCodes, 
		const TPoint& aTranslation,
			TInt aMinDiffTranslation,
			TInt aRotation,
			TInt aMinDiffRotation);

	virtual void PictureTaken(
		CFbsBitmap* aBitmap, 
		RPointerArray<CCodeInfo>* aCodes);

};

CRecognizerContainer::CRecognizerContainer(MApp_context& Context) : MContextBase(Context) { }

class CRecognizerViewImpl : public CRecognizerView, public MContextBase, MTimeOut {
public: // // Constructors and destructor
	
	CRecognizerViewImpl(MApp_context& Context, MRecognizerCallback* aCallback, 
		TUid aId, TUid LocalDefaultView, TVwsViewId* NextViewId, TInt aResourceId);
        void ConstructL();
        virtual ~CRecognizerViewImpl();
public:
	void StartDisplay();
	void StopDisplay();
protected:
	virtual void HandleForegroundEventL(TBool aForeground);
private:
	void expired(CBase * source);
private:
        void DoActivateL(const TVwsViewId& aPrevViewId,
		TUid aCustomMessageId,
		const TDesC8& aCustomMessage);
        void DoDeactivate();
	
        void HandleCommandL(TInt aCommand);
	
	TUid Id() const;

private: //Data
        CRecognizerContainer* iContainer; 
	MRecognizerCallback* iCallback;
	TVwsViewId	iPrevView;
	TUid		iId;
	TInt		iResource;
	TUid iDefaultView;
	TVwsViewId* iNextViewId;
	TInt iResourceId;
	CTimeOut * iStandByTimeOut;
	CTimeOut * iActivityTimeOut;
	TInt32 iCaptureHandle;
};


EXPORT_C CRecognizerView* CRecognizerView::NewL(MApp_context& Context, MRecognizerCallback* aCallback, TUid aId, 
						TUid LocalDefaultView, TVwsViewId* NextViewId, TInt aResourceId)
{
	CALLSTACKITEM_N(_CL("CRecognizerView"), _CL("NewL"));

	auto_ptr<CRecognizerViewImpl> ret(new (ELeave) CRecognizerViewImpl(Context, 
		aCallback, aId, LocalDefaultView, NextViewId, aResourceId));
	ret->ConstructL();
	return ret.release();
}

EXPORT_C CRecognizerView::~CRecognizerView()
{
	CALLSTACKITEM_N(_CL("CRecognizerView"), _CL("~CRecognizerView"));

}

CRecognizerViewImpl::CRecognizerViewImpl(MApp_context& Context, 
					 MRecognizerCallback* aCallback, TUid aId, 
					 TUid LocalDefaultView, TVwsViewId* NextViewId, 
					 TInt aResourceId) : 
	MContextBase(Context), iCallback(aCallback), iId(aId), 
	iDefaultView(LocalDefaultView), iNextViewId(NextViewId), iResourceId(aResourceId)
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("CRecognizerViewImpl"));

}

TUid CRecognizerViewImpl::Id() const
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("Id"));

	return iId;
}

void CRecognizerContainer::ConstructL(const TRect& aRect, CAknView* View, TBool navi)
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("ConstructL"));

	iView=View;
	iNavi=navi;
	iCurrentCodeArray=new RPointerArray<CCodeInfo>;
	iScreenMsgBuf = CEikonEnv::Static()->AllocReadResourceL(R_CODE_SCREEN_MSG);

	if (iNavi) {
		CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
		CAknNavigationControlContainer *np = (CAknNavigationControlContainer *)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi)); 
		CLocalNotifyWindow* tp=CLocalNotifyWindow::Global();
		np->PushDefaultL(ETrue);	
		HBufC * t = iEikonEnv->AllocReadResourceL(R_CHOOSE_CODE);
		tp->SetTitleText(t);
	} else {
//		CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
		CLocalNotifyWindow* tp=CLocalNotifyWindow::Global();
		tp->SetTitleTextToDefaultL();
	}

	CreateWindowL();

	iStandByLabel = new (ELeave) CEikLabel;
	iStandByLabel->SetContainerWindowL(*this);
	iStandByLabel->SetTextL(_L("Camera Standby\nClick to resume"));
	iStandByLabel->SetAlignment(TGulAlignment(EHCenterVCenter));
	iStandByLabel->SetExtent(TPoint(0,0), TSize(176,144));
	
	SetRect(aRect);
	ActivateL();
}

// Destructor
CRecognizerContainer::~CRecognizerContainer()
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("~CRecognizerContainer"));

	delete iStandByLabel;
	delete iScreenMsgBuf;

	if (iNavi) {
		CEikStatusPane* sp=iEikonEnv->AppUiFactory()->StatusPane();
		CAknNavigationControlContainer *np = (CAknNavigationControlContainer *)sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi)); 
		np->Pop(NULL);	
	}
	if (owns_bitmap) delete iBitmap;
	delete iVCS;
	delete iCurrentCodeInfo;
	if (iCurrentCodeArray) iCurrentCodeArray->Close();
	delete iCurrentCodeArray;
}

void CRecognizerContainer::SizeChanged()
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("SizeChanged"));

}


TInt CRecognizerContainer::CountComponentControls() const
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("CountComponentControls"));

	return 1;
}

CCoeControl* CRecognizerContainer::ComponentControl(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("ComponentControl"));

	if (aIndex==0) return iStandByLabel;		
	return NULL;
}

void CRecognizerContainer::DrawImage(const CFbsBitmap* aImage) const
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("DrawImage"));

	CWindowGc& gc = SystemGc();
	gc.Activate(Window());
	gc.Clear();

	TSize si = aImage->SizeInPixels();
	TInt wi = si.iWidth;
	TInt hi = si.iHeight;

	TInt wr = Rect().iBr.iX;
	TInt hr = Rect().iBr.iY;
	TPoint tl;
	tl.iX = (wr - wi) >> 1;
	tl.iY = (hr - hi) >> 1;
	gc.BitBlt(tl, aImage);

	if (iTranslation.iX == 0 && iTranslation.iY == 0) {
		ShowCodes(TRect(tl.iX, tl.iY, tl.iX + wi, tl.iY + hi), si);
	}

	// draw cross
	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	gc.SetPenColor(KRgbWhite);
	TPoint center = Rect().Center();
	DrawCross(gc, center);

	gc.Deactivate();
	iCoeEnv->WsSession().Flush();
}



inline void CRecognizerContainer::DrawCross(CWindowGc& gc, const TPoint& center) const 
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("DrawCross"));

	gc.DrawLine(TPoint(center.iX-5, center.iY), TPoint(center.iX+6, center.iY));
	gc.DrawLine(TPoint(center.iX, center.iY-5), TPoint(center.iX, center.iY+6));
}



// ---------------------------------------------------------
// CRecognizerContainer::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CRecognizerContainer::Draw(const TRect& aRect) const
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("Draw"));

	CWindowGc& gc = SystemGc();

	if (iBitmap != NULL) {
		TRect drawRect(0,1,176,133); // keep aspect ratio 160/120 = 640/480 = 4/3
		gc.DrawBitmap(drawRect, iBitmap);
		ShowCodes(drawRect, iBitmap->SizeInPixels());
	} else {
		CWindowGc& gc = SystemGc();
		gc.SetPenStyle(CGraphicsContext::ENullPen);
		if (!started) {
			gc.SetBrushColor(KRgbGray);
		} else {
			gc.SetBrushColor(KRgbWhite);
		}
		gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
		gc.DrawRect(aRect);
	}

}

void CRecognizerContainer::ShowCodes(const TRect& aDrawRect, const TSize& aBmpSize) const
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("ShowCodes"));

	RPointerArray<CCodeInfo>* Codes=0;
	TInt CurrentCode; bool draw_rect=true;
	if (!iVCS || iVCS->GetCodes() == NULL || iVCS->GetCodes()->Count()==0 || iInvalid) {
		if (!iCurrentCodeInfo) return;
		iCurrentCodeArray->Reset();
		User::LeaveIfError(iCurrentCodeArray->Append(iCurrentCodeInfo));
		Codes=iCurrentCodeArray;
		CurrentCode=0;
		if (iVCS || iInvalid) draw_rect=false;
	} else {
		Codes=iVCS->GetCodes();
		CurrentCode=iCurrentCode;
	}

	TInt xd = aDrawRect.iTl.iX;
	TInt yd = aDrawRect.iTl.iY;
	TInt hd = aDrawRect.Height();
	TInt wd = aDrawRect.Width();

	TInt hi = aBmpSize.iHeight;
	TInt wi = aBmpSize.iWidth;

	CWindowGc& gc = SystemGc();
	gc.SetPenStyle(CGraphicsContext::ESolidPen);

	TInt n = Codes->Count();

	// draw frames around the codes

	if (draw_rect) {
		for (TInt i = 0; i < n; i++) {
			CCodeInfo* ci = (*Codes)[i];

			gc.SetPenColor(ci->IsCodeValid() ? KRgbYellow : KRgbRed);
			gc.SetPenSize(i != CurrentCode ? TSize(1,1) : TSize(2,2));

			TPoint rb = ci->GetImageCoordinates(TPoint(10,10));

			gc.DrawLine(TPoint(xd + rb.iX*wd/wi, yd + rb.iY*hd/hi), 
						TPoint(xd + ci->x2*wd/wi, yd + ci->y2*hd/hi));
			gc.DrawLine(TPoint(xd + rb.iX*wd/wi, yd + rb.iY*hd/hi), 
						TPoint(xd + ci->x4*wd/wi, yd + ci->y4*hd/hi));
			gc.DrawLine(TPoint(xd + ci->x1*wd/wi, yd + ci->y1*hd/hi), 
						TPoint(xd + ci->x2*wd/wi, yd + ci->y2*hd/hi));
			gc.DrawLine(TPoint(xd + ci->x4*wd/wi, yd + ci->y4*hd/hi), 
						TPoint(xd + ci->x1*wd/wi, yd + ci->y1*hd/hi));
		}
	}

	gc.SetPenSize(TSize(1,1));

	// display value of current code

	if (CurrentCode >= 0 && CurrentCode < Codes->Count()) {
		CCodeInfo* ci = (*Codes)[CurrentCode];
		TBuf<64> info;

		// code value

		ci->code->AppendToString(info);
		gc.SetPenColor(KRgbYellow);
		gc.SetBrushStyle(CGraphicsContext::ENullBrush);
		const CFont* fontUsed = iEikonEnv->DenseFont();
		gc.UseFont(fontUsed);
		TInt baseline = aDrawRect.Height() - 2 * fontUsed->AscentInPixels();
		info.Copy(*iScreenMsgBuf);
		gc.DrawText(info, aDrawRect, baseline, CGraphicsContext::ECenter);

	}
}

void CRecognizerContainer::ViewFinderStartStop()
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("ViewFinderStartStop"));
	Reporting().DebugLog(_L("ViewFinderStartStop"));

	started=true;

	if (!iVCS || !iTakingPicture) {
		delete iVCS; iVCS=0; iBitmap=0;
		iVCS=new (ELeave) CVisualCodeSystem(AppContext());
		iVCS->ConstructL();
		iVCS->AddObserver(this);
	}
	if (iTakingPicture) {
		if (iBitmap) {
			CFbsBitmap* temp=iBitmap;
			iBitmap=new (ELeave) CFbsBitmap;
			owns_bitmap=true;
			User::LeaveIfError(iBitmap->Duplicate(temp->Handle()));
		}
		delete iVCS; iVCS=0;
		iTakingPicture=FALSE;
	} else {
		iTakingPicture = TRUE;
		// start with: updates on, translation off, rotation off, low-quality recognition on
		iVCS->Start(TRUE, FALSE, FALSE, TRUE);
	}
}

void CRecognizerContainer::StartDisplay()
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("StartDisplay"));

	iTakingPicture = FALSE;
	iCurrentCode = -1;
	iBitmap = NULL;
	iTranslation.SetXY(0, 0);
	iTarget.SetXY(320, 240);
	iTilting.SetXY(0.0, 0.0);
	iMagnification = 2;

	iStandByLabel->MakeVisible(EFalse);

	ViewFinderStartStop();
}

void CRecognizerContainer::StopDisplay()
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("StopDisplay"));

	if (owns_bitmap) delete iBitmap; iBitmap=0;
	delete iVCS; iVCS=0;
	delete iCurrentCodeInfo; iCurrentCodeInfo=0;
	iCurrentCodeArray->Reset();
	iStandByLabel->MakeVisible(ETrue);
	DrawDeferred();
}

void CRecognizerContainer::PictureUpdateL(
	CFbsBitmap& aBitmap, 
	RPointerArray<CCodeInfo>* aCodes, 
	const TPoint& aTranslation,
	TInt /*aMinDiffTranslation*/,
	TInt /*aRotation*/,
	TInt /*aMinDiffRotation*/)
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("PictureUpdateL"));

	iTranslation.SetXY(aTranslation.iX, aTranslation.iY);
	if (aCodes != NULL) {
		iCurrentCode = iVCS->FindClosestCode(iTarget);
		if (iCurrentCode >= 0) {
			CCodeInfo* ci = (*aCodes)[iCurrentCode];
			TPoint p = ci->GetTilting();
			iTilting.SetXY(p.iX, p.iY);

			if (ci->IsCodeValid()) {
				iInvalid=false;
				delete iCurrentCodeInfo; iCurrentCodeInfo=0;
				iCurrentCodeInfo=new (ELeave) CCodeInfo;
				iCurrentCodeInfo->ConstructL(*ci);
			} else {
				iInvalid=true;
			}
		}
	}

	iBitmap=&aBitmap;
	DrawImage(&aBitmap);
}

CCodeInfo* CRecognizerContainer::GetCode()
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("GetCode"));

	return iCurrentCodeInfo;
}

void CRecognizerContainer::PictureTaken(
	CFbsBitmap* aBitmap, 
	RPointerArray<CCodeInfo>* /*aCodes*/)
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("PictureTaken"));

	iTakingPicture = FALSE;
	iBitmap = aBitmap;
	iCurrentCode = iVCS->FindClosestCode(iTarget);
	DrawNow();
}


void CRecognizerViewImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("ConstructL"));

#ifndef __WINS__
	TFileName resfile=_L("c:\\System\\data\\recognizer.rsc");
	if (! BaflUtils::FileExists(Fs(), resfile) ) {
		resfile.Replace(0, 1, _L("e"));
	}
#else
	TFileName resfile=_L("z:\\System\\data\\recognizer.rsc");
	BaflUtils::NearestLanguageFile(iEikonEnv->FsSession(), resfile); //for localization
#endif
	User::LeaveIfError(iResource=iEikonEnv->AddResourceFileL(resfile));

	if (iResourceId==-1) iResourceId=R_RECOGNIZER_VIEW;
	
	BaseConstructL(iResourceId);
	iActivityTimeOut = CTimeOut::NewL(*this);
	iStandByTimeOut = CTimeOut::NewL(*this);
}

void CRecognizerViewImpl::DoActivateL(const TVwsViewId& aPrevViewId, TUid /*aCustomMessageId*/,
		const TDesC8& /*aCustomMessage*/)
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("DoActivateL"));

	iPrevView=aPrevViewId;
	if (!iContainer) {
		iContainer = new (ELeave) CRecognizerContainer(AppContext());
		iContainer->SetMopParent( this );
		iContainer->ConstructL( ClientRect(), this, iResourceId==R_RECOGNIZER_VIEW );
		AppUi()->AddToStackL( *this, iContainer );
	}

	
}

void CRecognizerViewImpl::DoDeactivate()
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("DoDeactivate"));
	
	if ( iContainer )
        {
		AppUi()->RemoveFromViewStack( *this, iContainer );
        }
	
	delete iContainer;
	iContainer = 0;
}

// ----------------------------------------------------
// CRecognizerViewImpl::~CRecognizerViewImpl()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CRecognizerViewImpl::~CRecognizerViewImpl()
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("~CRecognizerViewImpl"));
	
	if (iActivityTimeOut) iActivityTimeOut->Reset();
	delete iActivityTimeOut;

	if (iStandByTimeOut) iStandByTimeOut->Reset();
	delete iStandByTimeOut;

	if (iContainer) {
		AppUi()->RemoveFromViewStack( *this, iContainer );
		delete iContainer;
        }
	if (iResource) iEikonEnv->DeleteResourceFile(iResource);
}

TKeyResponse CRecognizerContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
	CALLSTACKITEM_N(_CL("CRecognizerContainer"), _CL("OfferKeyEventL"));

	if (aKeyEvent.iCode==JOY_CLICK && aType==EEventKey) {
		Reporting().DebugLog(_L("CRecognizerContainer::OfferKeyEventL::joy"));

		if (iStandByLabel->IsVisible()) {
			((CRecognizerViewImpl*)iView)->StartDisplay();
		} else {
#ifdef __WINS__
			iView->HandleCommandL(ERecognizerCaptureCode);
#else
			if (GetCode()) {
				iView->HandleCommandL(ERecognizerCaptureCode);
			}
#endif
		}
		return EKeyWasConsumed;
	} else {
		if (iNavi) return EKeyWasConsumed;
		return EKeyWasNotConsumed;
	}
}

void CRecognizerViewImpl::HandleCommandL(TInt aCommand)
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("HandleCommandL"));
	Reporting().DebugLog(_L("CRecognizerViewImpl::HandleCommandL"));

	switch ( aCommand )
        {
	case ERecognizerCaptureCode:
		{
#ifdef __WINS__
		TInt value(100);

		CAknNumberQueryDialog * dlg = CAknNumberQueryDialog::NewL( value, CAknQueryDialog::ENoTone );
		CleanupStack::PushL(dlg);
		dlg->PrepareLC(R_WINS_CODE_SELECTION);
		CleanupStack::Pop(dlg);
		if ( dlg->RunLD() ) {
			auto_ptr<CBigInteger> i(new (ELeave) CBigInteger);
			i->ConstructL(32);
			i->Set(value);
			CCodeInfo* c=new CCodeInfo;
			if (!c) {
				User::Leave(KErrNoMemory);
			}
			c->code=i.release();
			auto_ptr<CCodeInfo> cp(c);

			iCallback->CodeSelected(*c);
		} else {
			iCallback->Cancelled();
		}
		
#else
		CCodeInfo* code=iContainer->GetCode();
		if (!code) return;
		iCallback->CodeSelected(*code);
#endif
		}
		break;
	case ERecognizerCancel:
		{
		iCallback->Cancelled();
		}
		break;
        default:
		AppUi()->HandleCommandL(aCommand);
		break;      
        }
	if (iNextViewId) {
		//AppUi()->ActivateLocalViewL(iDefaultView);
		*iNextViewId=iPrevView;
	}
}

void CRecognizerViewImpl::HandleForegroundEventL(TBool aForeground)
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("HandleForegroundEventL"));
	Reporting().DebugLog(_L("CRecognizerViewImpl::HandleForegroundEventL"));

	if (aForeground) {
		StartDisplay();
	} else {
		StopDisplay();
	}
}

void CRecognizerViewImpl::StartDisplay()
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("StartDisplay"));

	iContainer->StartDisplay();
	iActivityTimeOut->Wait(4);
	iStandByTimeOut->Wait(30);
#ifdef __S60V2__
	TInt captureTopPriority(1000000000);
	RWindowGroup& groupWin = iCoeEnv->RootWin();
	iCaptureHandle = groupWin.CaptureKey(KCameraKey, 0, 0, captureTopPriority);
#endif
}

void CRecognizerViewImpl::StopDisplay()
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("StopDisplay"));

	iActivityTimeOut->Reset();
	iStandByTimeOut->Reset();
#ifdef __S60V2__
	if (iCaptureHandle) {
		RWindowGroup& groupWin = iCoeEnv->RootWin();
		groupWin.CancelCaptureKey(iCaptureHandle);
		iCaptureHandle=0;
	}
#endif
	iContainer->StopDisplay();
}

void CRecognizerViewImpl::expired(CBase * source)
{
	CALLSTACKITEM_N(_CL("CRecognizerViewImpl"), _CL("expired"));

        	if (source==iStandByTimeOut) {
		iActivityTimeOut->Reset();
		iContainer->StopDisplay();
	} else if (source==iActivityTimeOut) {
		User::ResetInactivityTime();
		iActivityTimeOut->Wait(4);
	}
}


