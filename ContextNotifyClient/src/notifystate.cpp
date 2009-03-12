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

#include "notifystate.h"

#include "break.h"
#include "symbian_auto_ptr.h"
#include "contextnotifyclientsession.h"
#include "app_context.h"
#include "icons.h"
#include "juik_layout_impl.h"
#include "jaiku_layoutids.hrh"


#include <eikappui.h>
#include <gulicon.h>
#include <eikenv.h>
#include <bautils.h>

#include <aknsutils.h>

#ifdef __SCALABLEUI_VARIANT__
#include <akniconutils.h>
#endif
#include <aknsskininstance.h>
#include <aknsutils.h>

class CNotifyStateImpl: public CNotifyState, public MContextBase {
public:
	virtual ~CNotifyStateImpl();
private:
	CNotifyStateImpl(MApp_context& Context, const TDesC& aBitmapFilename);
	void ConstructL(const TDesC& aBitmapFilename);

	void CheckedRunL();
	TInt CheckedRunError(TInt aError);
	void DoCancel();
	void SetCurrentState(TInt aIconId, TInt aMaskId);
	void StartL();
	void SetIcon();
	void LoadIconL();
	void Release();

	void UpdateLayoutDataL();

	TFileName	iFilename;
	TInt		iNotifyId, iLocalId;

	enum TRunState { EIdle, EChanging };
	TRunState iRunState;
	TInt	iNextIconId, iNextMaskId;
	CGulIcon	*iIcon;
	CFbsBitmap* iIdleBackground;

	RContextNotifyClientSession iNotifyClient;
	RWsSession	*iWs;
	CWsScreenDevice* iScreen;
#ifndef __S80__
	CLocalNotifyWindow* iLocalWindow;
#endif

	TJuikLayoutItem iLayoutItem;

	friend class CNotifyState;
};

EXPORT_C CNotifyState* CNotifyState::NewL(MApp_context& Context, const TDesC& aBitmapFilename)
{
	CALLSTACKITEM2_N(_CL("CNotifyState"), _CL("NewL"), &Context);

	auto_ptr<CNotifyStateImpl> ret(new (ELeave) CNotifyStateImpl(Context, aBitmapFilename));
	ret->ConstructL(aBitmapFilename);
	return ret.release();
}

CNotifyState::CNotifyState() : CCheckedActive(EPriorityNormal, _L("CNotifyStateImpl"))
{
}

CNotifyStateImpl::CNotifyStateImpl(MApp_context& Context, const TDesC& aBitmapFilename) : 
	MContextBase(Context), iFilename(aBitmapFilename), iNotifyId(-1), iNextIconId(-1),
		iLocalId(-1)
{
}

void CNotifyStateImpl::Release()
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("Release"));

	Cancel();
	iNotifyClient.Close();
#ifndef __S80__
	//iLocalWindow=CLocalNotifyWindow::Global();
	//if (iLocalWindow && iLocalId>=0) iLocalWindow->RemoveIcon(iLocalId);
#endif
	iLocalId=-1;
	iNotifyId=-1;
	delete iIcon; iIcon=0;
	delete iIdleBackground; iIdleBackground = NULL;
	delete iScreen; iScreen=0;
	if (iWs) {
		iWs->Close();
		delete iWs; iWs=0;
	}
}

CNotifyStateImpl::~CNotifyStateImpl()
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("~CNotifyStateImpl"));

	Release();
}

void CNotifyStateImpl::ConstructL(const TDesC& aBitmapFilename)
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("ConstructL"));

	#ifdef __S60V3__
	TParse p; p.Set(aBitmapFilename, 0, 0);
#  ifdef __WINS__
	iFilename=_L("z:");
#  else
	iFilename=_L("c:");
#  endif
	iFilename.Append(_L("\\resource\\"));
	iFilename.Append(p.NameAndExt());
#endif

	if (! BaflUtils::FileExists(Fs(), iFilename) ) {
		iFilename.Replace(0, 1, _L("e"));
		if (! BaflUtils::FileExists(Fs(), iFilename) ) User::Leave(KErrNotFound);
	}
	CActiveScheduler::Add(this);
	StartL();
}

void CNotifyStateImpl::StartL()
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("StartL"));
	
#ifndef __S80__
	iLocalWindow=CLocalNotifyWindow::Global();

	RWsSession* wsSession = NULL;
	if (! CEikonEnv::Static()) {
		if ( !iWs ) 
			{
				iWs=new (ELeave) RWsSession;
				User::LeaveIfError(iWs->Connect());
			}

		wsSession = iWs;
	} else {
 		AknsUtils::InitSkinSupportL();
 		MAknsSkinInstance* skinInstance=AknsUtils::SkinInstance();
		wsSession = &(CEikonEnv::Static()->WsSession());
 	}
	
	if ( ! iScreen )
		{
			iScreen=new (ELeave) CWsScreenDevice(*wsSession);
			User::LeaveIfError(iScreen->Construct());
		}
	UpdateLayoutDataL();		
#endif
	
	User::LeaveIfError( iNotifyClient.Connect() );
	if ( iIdleBackground )
		{
			TRequestStatus syncStatus;
			iNotifyClient.SetIdleBackground( iIdleBackground, syncStatus );
			User::WaitForRequest( syncStatus );
		}

}

void CNotifyStateImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("CheckedRunL"));
	iRunState=EIdle;
	if (iStatus!=KErrNone) 
		User::Leave(iStatus.Int());
}

TInt CNotifyStateImpl::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("CheckedRunError"));
	TInt e = aError;
	Release();
	CC_TRAPD(err, StartL());
	if (err==KErrNone) {
		SetIcon();
	}
	return err;
}

void CNotifyStateImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("DoCancel"));

	iNotifyClient.Cancel();
}

void CNotifyStateImpl::SetCurrentState(TInt aIconId, TInt aMaskId)
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("SetCurrentState"));

	iNextIconId=aIconId; iNextMaskId=aMaskId;
	if (iRunState==EIdle) {
		SetIcon();
	}
}


void CNotifyStateImpl::LoadIconL()
{
	delete iIcon; iIcon=0;

#ifdef __SCALABLEUI_VARIANT__
	
	CFbsBitmap* bitmapP =NULL;
	CFbsBitmap* maskP = NULL;
	AknIconUtils::CreateIconL(  bitmapP, maskP,
								iFilename, iNextIconId, iNextMaskId );
	auto_ptr<CFbsBitmap> bitmap( bitmapP );
	auto_ptr<CFbsBitmap> mask( maskP );
	iIcon=CGulIcon::NewL(bitmap.get(), mask.get());
	bitmap.release(); mask.release();

	MAknsSkinInstance* skinInstance=AknsUtils::SkinInstance();
	if ( skinInstance )
		{
			TRgb color = KRgbWhite;
			TInt err = AknsUtils::GetCachedColor(skinInstance, 
												 color,
												 KAknsIIDQsnIconColors,
												 EAknsCIQsnIconColorsCG1);
			if ( KErrNone == err )
				{
					AknIconUtils::SetIconColor( iIcon->Bitmap(), color );
				}
		}

	UpdateLayoutDataL(); // FIXME: should really listen for changes in screen
	AknIconUtils::SetSize(iIcon->Bitmap(), iLayoutItem.Rect().Size());
#else

	TInt scale=1;
	CEikonEnv* env=CEikonEnv::Static();
	RWsSession* ws=0;
	CWsScreenDevice* screen=0;
	if (env) {
		if ( ((CEikAppUi*)env->AppUi())->ApplicationRect().Width()>300) scale=2;
		ws=&(env->WsSession());
		screen=env->ScreenDevice();
	} else {
		ws=iWs;
		screen=iScreen;
	} 

	auto_ptr<CWsBitmap> bitmap(new (ELeave) CWsBitmap(*ws));
 	User::LeaveIfError(bitmap->Load(iFilename, iNextIconId));
	bitmap->SetSizeInTwips(screen);
#ifdef __S60V2__
	if (scale>1) {
		auto_ptr<CWsBitmap> scaled(new (ELeave) CWsBitmap(*ws));
		ScaleFbsBitmapL(bitmap.get(), scaled.get(), scale);
		bitmap=scaled;
	}
#endif
	auto_ptr<CWsBitmap> mask(new (ELeave) CWsBitmap(*ws));
	User::LeaveIfError(mask->Load(iFilename, iNextMaskId));
	mask->SetSizeInTwips(screen);
#ifdef __S60V2__
	if (scale>1) {
		auto_ptr<CWsBitmap> scaled(new (ELeave) CWsBitmap(*ws));
		ScaleFbsBitmapL(mask.get(), scaled.get(), scale);
		mask=scaled;
	}
#endif
	iIcon=CGulIcon::NewL(bitmap.get(), mask.get());
	bitmap.release(); mask.release();

#endif // __SCALABLEUI_VARIANT__
}

void CNotifyStateImpl::SetIcon()
{
	CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("SetIcon"));
	
	LoadIconL();
	
	iStatus=KRequestPending;
#ifndef __S80__
	/*iLocalWindow=CLocalNotifyWindow::Global();
	if (iLocalId>=0) {
		if (iLocalWindow) iLocalWindow->ChangeIconL(iIcon->Bitmap(), iIcon->Mask(), iLocalId);
	} else {
		if (iLocalWindow) iLocalId=iLocalWindow->AddIconL(iIcon->Bitmap(), iIcon->Mask());
	}*/
#endif
	
	SetActive();
	if (iNotifyId>=0) {
		iNotifyClient.ChangeIcon(iIcon->Bitmap(), iIcon->Mask(), iNotifyId, iStatus);
	} else {
		iNotifyClient.AddIcon(iIcon->Bitmap(), iIcon->Mask(), iNotifyId, iStatus);
	}
	iRunState=EChanging;
}

void CNotifyStateImpl::UpdateLayoutDataL()
{
       CALLSTACKITEM_N(_CL("CNotifyStateImpl"), _CL("UpdateLayoutDataL"));

	auto_ptr<CJuikLayout> layout( CJuikLayout::NewL(EFalse));
	TInt mode=iScreen->CurrentScreenMode();
	TPixelsAndRotation sz_and_rot;
	iScreen->GetScreenModeSizeAndRotation(mode, sz_and_rot);
	layout->UpdateLayoutDataL( sz_and_rot.iPixelSize );
	iLayoutItem =
		layout->GetLayoutItemL( LG_idleview_connection_icon, 
								LI_idleview_connection_icon__icon  );
}
