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

#include "drawer.h"

#include "symbian_auto_ptr.h"
#include "break.h"
#include "app_context.h"
#include "juik_layout_impl.h"
#include "jaiku_layoutids.hrh"


#include <e32std.h>
#include <w32std.h>
#include <coedef.h>
#include <apgwgnam.h>
#ifndef __S80__
#include <aknkeylock.h>
#else
class RAknKeyLock {
public:
	TInt Connect() { return 0; }
	void Close() { }
	TBool IsKeyLockEnabled() { return EFalse; }
};
#endif

#include <aknsbasicbackgroundcontrolcontext.h>
#include <aknsdrawutils.h>
#include <aknsutils.h>

/*
 * Concepts:
 * !Drawing on top of Phone screen!
 * !UI in EXE!
 */

const TUid KUidPhone = { 0x100058b3 };
const TUid KUidIdle = { 0x101fd64c };
const TUid KUidMenu = { 0x101f4cd2 };

const TInt KIconSep = 1;

class CDrawerImpl : public CDrawer {
public:
	virtual TInt	AddIconL(CFbsBitmap* data, CFbsBitmap* mask);
	virtual void	RemoveIcon(TInt Id);
	virtual void	ChangeIconL(CFbsBitmap* data, CFbsBitmap* mask, TInt Id);
	virtual void	GetIcons(TInt* aIds, TInt aSize);
	virtual void	SetBackgroundL(TInt aHandle);
	CDrawerImpl(RWsSession& aWsSession);
	virtual ~CDrawerImpl();

	virtual void ForegroundChanged(CApaWindowGroupName* gn);
	virtual void ScreenChanged();
        
	void ConstructL();
	void StartL();
	void Listen();
	void Release();
	void Draw(const TRect& aRect);
	void ReadLayoutL();

	void RunL();
	TInt RunError(TInt aError);
	void DoCancel();
	void ReadSize();

	TBool reception_bar_on_screen();

	void CopyBackground();

	RWsSession& iWsSession; 
        CWindowGc* gc;
	CWsScreenDevice* screen;
	RWindowGroup wg; bool wg_is_open;
	RWindow	w; bool w_is_open;

	RAknKeyLock iKeyLock; bool kl_is_open;
	TUid iCurrentAppUid, iPreviousAppUid, iSecondPreviousAppUid;

	RPointerArray<CFbsBitmap> iIcons;
	RPointerArray<CFbsBitmap> iMasks;
	bool	full_redraw;

	CFbsBitmap*	iBackground;
	//CFbsBitmap* iBackgroundCopy;
	
	CJuikLayout* iLayout;

	CAknsBasicBackgroundControlContext* iBackgroundControlContext;
	TSize iScreenSize; // cached screensize, because reading it from CWsScreenDevice causes flush 
};

CDrawer::CDrawer(CActive::TPriority aPriority) : CActive(aPriority)
{
}

CDrawer::~CDrawer()
{
	
}

void CDrawerImpl::GetIcons(TInt* aIds, TInt aSize)
{
	TInt i=0, j=0;
	while (i<iIcons.Count() && j<aSize) {
		if (iIcons[i] && iMasks[i]) {
			aIds[j]=iIcons[i]->Handle();
			j++;
			aIds[j]=iMasks[i]->Handle();
			j++;
		}
		i++;
	}
}
	
void CDrawerImpl::ReadSize()
{
	TSize s(0, 0);
	TInt i=0;
	while (i<iIcons.Count()) {
		if (iIcons[i]) {
			TSize is=iIcons[i]->SizeInPixels();
			s.iWidth+=is.iWidth+KIconSep;
			if (is.iHeight > s.iHeight) s.iHeight=is.iHeight;
		}
		i++;
	}
	w.SetSize(s);
}

CDrawer* CDrawer::NewL(RWsSession& aWsSession)
{
	auto_ptr<CDrawerImpl> ret(new (ELeave) CDrawerImpl(aWsSession));
	ret->ConstructL();
	return ret.release();
}

TInt	CDrawerImpl::AddIconL(CFbsBitmap* aIcon, CFbsBitmap* aMask)
{
	User::LeaveIfError(iIcons.Append(aIcon));
	TInt err=iMasks.Append(aMask);
	if (err!=KErrNone) {
		iIcons.Remove(iIcons.Count()-1);
		User::Leave(err);
	}
	ReadSize();	

	full_redraw=true; Draw(TRect());
	return iIcons.Count()-1;
}

void CDrawerImpl::SetBackgroundL(TInt aHandle)
{
	if (!iBackground) {
		iBackground=new (ELeave) CFbsBitmap;
	}	
	iBackground->Reset();
	User::LeaveIfError(iBackground->Duplicate(aHandle));
}

void	CDrawerImpl::RemoveIcon(TInt Id)
{
	if (Id<0 || Id >= iIcons.Count()) return;

	delete iIcons[Id];
	iIcons[Id]=0;
	delete iMasks[Id];
	iMasks[Id]=0;
	ReadSize();	
	full_redraw=true; Draw(TRect());
}

void	CDrawerImpl::ChangeIconL(CFbsBitmap* data, CFbsBitmap* aMask, TInt Id)
{
	if (Id<0 || Id >= iIcons.Count()) User::Leave(KErrNotFound);

	delete iIcons[Id];
	iIcons[Id]=data;
	delete iMasks[Id];
	iMasks[Id]=aMask;

	ReadSize();	
	full_redraw=true; Draw(TRect());
}

CDrawerImpl::CDrawerImpl(RWsSession& aWsSession) : CDrawer(EPriorityHigh), iWsSession(aWsSession)
{
}

void CDrawerImpl::ReadLayoutL()
{
	TInt mode=screen->CurrentScreenMode();
	TPixelsAndRotation sz_and_rot;
	screen->GetScreenModeSizeAndRotation(mode, sz_and_rot);
	iScreenSize=sz_and_rot.iPixelSize;
	iLayout->UpdateLayoutDataL(iScreenSize);
	TJuikLayoutItem l =
		iLayout->GetLayoutItemL( LG_idleview_connection_icon, LI_idleview_connection_icon__icon  );

	TSize ws=w.Size();
	w.SetExtent( l.Rect().iTl, ws );
}

void CDrawerImpl::ScreenChanged()
{
	ReadLayoutL();

	TRect screenRect( TPoint(0,0), iScreenSize );

	delete iBackgroundControlContext;
	iBackgroundControlContext = NULL;
	iBackgroundControlContext = CAknsBasicBackgroundControlContext::NewL(KAknsIIDQsnBgScreenIdle, 
																		 screenRect, EFalse);

	
// 	if ( iBackgroundCopy )
// 		{
// 			delete iBackgroundCopy;
// 			iBackgroundCopy = NULL;
// 		}
// 	iBackgroundCopy = new (ELeave) CFbsBitmap();
// 	User::LeaveIfError(iBackgroundCopy->Create(screenSize, displayMode));

	full_redraw=true; Draw(TRect());
}

void CDrawerImpl::ConstructL()
{
	iLayout = CJuikLayout::NewL(EFalse);
	AknsUtils::InitSkinSupportL();
	
	
	screen=new (ELeave) CWsScreenDevice(iWsSession);
	User::LeaveIfError(screen->Construct());
	
	User::LeaveIfError(screen->CreateContext(gc));
	
	wg=RWindowGroup(iWsSession);
	User::LeaveIfError(wg.Construct((TUint32)&wg, EFalse));
	wg_is_open=true;
	wg.SetOrdinalPosition(1, ECoeWinPriorityAlwaysAtFront+1);
	wg.EnableReceiptOfFocus(EFalse);

	CApaWindowGroupName* wn=CApaWindowGroupName::NewLC(iWsSession);
	wn->SetHidden(ETrue);
	wn->SetWindowGroupName(wg);
	CleanupStack::PopAndDestroy();

	w=RWindow(iWsSession);
	User::LeaveIfError(w.Construct(wg, (TUint32)&w));
	w_is_open=true;

	w.Activate();
	w.SetExtent( TPoint(0, 0), TSize(0, 0) );
	ReadLayoutL();
	
	TRect screenRect( TPoint(0,0), MJuikLayout::ScreenSize());
	iBackgroundControlContext = CAknsBasicBackgroundControlContext::NewL(KAknsIIDQsnBgScreenIdle, 
																		 screenRect, EFalse);
	
// 	TSize screenSize = screen->SizeInPixels();
// 	TDisplayMode displayMode = screen->DisplayMode();
// 	if ( iBackgroundCopy )
// 		{
// 			delete iBackgroundCopy;
// 			iBackgroundCopy = NULL;
// 		}
// 	iBackgroundCopy = new (ELeave) CFbsBitmap();
// 	User::LeaveIfError(iBackgroundCopy->Create(screenSize, displayMode));
// 	CopyBackground();


	w.SetOrdinalPosition(1, ECoeWinPriorityAlwaysAtFront+1);
	w.SetNonFading(EFalse);

	TInt wgid=iWsSession.GetFocusWindowGroup();
	CApaWindowGroupName* gn;
	gn=CApaWindowGroupName::NewLC(iWsSession, wgid);

    ForegroundChanged(gn);
	CleanupStack::PopAndDestroy();

	TInt err=iKeyLock.Connect();
	if (err==KErrNone) kl_is_open=true;

	CActiveScheduler::Add(this);

	Listen();
}

CDrawerImpl::~CDrawerImpl()
{
	Cancel();	

	int i;
	for (i=0; i<iIcons.Count(); i++) {
		delete iIcons[i];
		delete iMasks[i];
	}
	iIcons.Close(); iMasks.Close();

	delete gc;
	delete screen;
	if (w_is_open) w.Close();
	if (wg_is_open) wg.Close();
	if (kl_is_open) iKeyLock.Close();
	delete iBackground;
	delete iBackgroundControlContext;
// 	delete iBackgroundCopy;
	
	delete iLayout;
}

void CDrawerImpl::StartL()
{
}

void CDrawerImpl::Listen()
{
	iStatus=KRequestPending;
	iWsSession.RedrawReady(&iStatus);
	SetActive();
}

void CDrawerImpl::Release()
{
}

void CDrawerImpl::RunL()
{
	TWsRedrawEvent e;
	iWsSession.GetRedraw(e);
	CopyBackground();
	Draw(e.Rect());
	Listen();
}

void CDrawerImpl::CopyBackground()
{
// 	if ( iBackgroundCopy )
// 	{
// 		TInt err = screen->CopyScreenToBitmap( iBackgroundCopy );		
		
// 	}
}


void CDrawerImpl::Draw(const TRect& serverRect)
{


	TRect aRect=serverRect;

	gc->Activate(w);

	if (full_redraw) {
		aRect=TRect(TPoint(0,0), w.Size());
	}
	full_redraw=false;
	//w.HandleTransparencyUpdate();
	//TInt err = w.SetTransparencyAlphaChannel();
	w.Invalidate(aRect);
	w.BeginRedraw();
	

	// Draw background 
	

	if ( iBackground )
		{
			TRect sourceRect( aRect );
			gc->BitBlt(TPoint(0,0), iBackground, sourceRect); 
		}
	else if ( iBackgroundControlContext )
		{
			TPoint dstPosInCanvas(0,0);			
			TRect partOfBackground( w.AbsPosition(), w.Size() );

			TBool skinBackgroundDrawn = 
				AknsDrawUtils::DrawBackground( AknsUtils::SkinInstance(), 
											   iBackgroundControlContext,
											   NULL, 
											   *gc, 
											   dstPosInCanvas,
											   partOfBackground,
											   KAknsDrawParamLimitToFirstLevel);
			//											   KAknsDrawParamDefault);
		}
		
// 	else if ( iBackgroundCopy )
//  		{
// 			TPoint p = w.AbsPosition();
// 			TSize s = w.Size();
//  			TRect sourceRect( p, s );

//  			gc->BitBlt(TPoint(0,0), iBackgroundCopy, sourceRect); 			
//  		}
	else
		{
// 			gc->SetPenStyle(CGraphicsContext::ENullPen);
// 			gc->SetBrushColor(KRgbWhite);
// 			gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
// 			gc->DrawRect(aRect);
 			TRgb backgroundColour(100,100,100);
// 			backgroundColour.SetAlpha(0x50);
 			gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
 			gc->SetBrushColor(backgroundColour);
			gc->Clear();
		}

	gc->SetBrushStyle(CGraphicsContext::ENullBrush);
	int xpos=0, i;
	for (i=0; i<iIcons.Count(); i++) {
		if (iIcons[i]!=0 && iMasks[i]!=0) {
			xpos+=KIconSep;
			TRect r( TPoint(0, 0), iIcons[i]->SizeInPixels());
			gc->BitBltMasked(TPoint(xpos, 0), iIcons[i], r, iMasks[i], ETrue);
			xpos+=r.Width();
		}
	}

	w.EndRedraw();
	gc->Deactivate();
	iWsSession.Flush();
}

TInt CDrawerImpl::RunError(TInt aError)
{
	return aError;
}

void CDrawerImpl::DoCancel()
{
	iWsSession.RedrawReadyCancel();
}

void CDrawerImpl::ForegroundChanged(CApaWindowGroupName* gn)
{
	TUid AppUid=gn->AppUid();
	RDebug::Print(_L("%u"), AppUid.iUid);

	iSecondPreviousAppUid = iPreviousAppUid;
	iPreviousAppUid = iCurrentAppUid;
	iCurrentAppUid = AppUid;

#ifdef __WINS__
	if ( iCurrentAppUid==KUidMenu || 
		 iCurrentAppUid==KUidPhone ||
		 iCurrentAppUid==KUidIdle ) {
		w.SetVisible(ETrue);
	} else {
		w.SetVisible(EFalse);
	}
#else
	if (iCurrentAppUid==KUidPhone
#ifdef __S60V3__
		|| iCurrentAppUid==KUidIdle
#endif
	) {
		w.SetVisible(ETrue);
	} else if (iCurrentAppUid==KNullUid) {
		
		//First, getting keylock status
		if (!kl_is_open) { 
			TInt err=iKeyLock.Connect();
			if (err==KErrNone) kl_is_open=true;
		}
		
		// Then many cases:
		if ( kl_is_open && iKeyLock.IsKeyLockEnabled() ) { // Keyboard is closed
			if (iPreviousAppUid == KUidPhone) { 
				// we locked the screen on the phone app -> display indicators
				// even if the screensaver goes on, no new uid event is generated,
				// so looking at the first previous one should be enough.
				w.SetVisible(ETrue);
			} else if ( (iPreviousAppUid == KNullUid) && (iSecondPreviousAppUid == KUidPhone) ) {
				w.SetVisible(ETrue);
			} else {
				w.SetVisible(EFalse);
			}
		} else { // Keyboard is not closed
			w.SetVisible(EFalse); // we are displaying either the app menu, or the power menu, 
						// but both have KNullUid, so no difference possible -> no display
		}
	} else {
		w.SetVisible(EFalse);
	}
#endif
	
}
