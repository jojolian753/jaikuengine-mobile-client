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

#include "NotifyCommon.h"

#include "ContextNotifyClientSession.h"

#include "reporting.h"
#include <e32math.h>
#include <eikenv.h>
#include "list.h"
#include "app_context.h"
#include <eikspane.h>
#include <eikimage.h>
#ifndef __S80__
#include <avkon.hrh>
#include <avkon.rsg>
#endif
#include <aknutils.h>
#include <bautils.h>
#include "break.h"
#include "server_startup.h"
#include "compat_server.h"

#ifdef __S60V2__
#include <AknsControlContext.h>
#include <AknsBasicBackgroundControlContext.h>
#include <AknsDrawUtils.h>
#endif
#include "context_uids.h"


static const TUint KDefaultMessageSlots = 2;
static const TUid KServerUid3 = { CONTEXT_UID_CONTEXTNOTIFY }; // matches UID in server/group/ContextNotifyer.mbm file

_LIT(KContextNotifyFilename, "ContextNotify");


TAknsItemID StatusPaneBackgroundSkin() 
{
	TAknsItemID id = KAknsIIDQsnBgAreaStatus;
#if defined(__S60V3__)
	TSize legacy(176, 208);
	TSize s;
	
	if ( ! AknLayoutUtils::LayoutMetricsSize(AknLayoutUtils::EScreen, s) )
		s = legacy;
	
	TBool landscape = s.iWidth > s.iHeight;
	AknLayoutUtils::TAknCbaLocation  cba=AknLayoutUtils::CbaLocation();
	
	if ( landscape ) 
		{
			if (cba == AknLayoutUtils::EAknCbaLocationRight ) {
				id = KAknsIIDQsnBgAreaStaconRt;
			} else {
				id = KAknsIIDQsnBgAreaStaconLt;
			}
		}
	else
		{
			id = KAknsIIDQsnBgAreaStatus;
		}
#endif
	return id;
}


/*
 * Concepts:
 * !Customizing the status pane!
 */

class MObjectProviderEx
        {
public:
        template<class T>
        T* MopGetObject(T*& aPtr)
                { return (aPtr=static_cast<T*>(MopGetById(T::ETypeId))); }

private: // must be overridden
        virtual TTypeUid::Ptr MopSupplyObject(TTypeUid aId) = 0;

private: // may be overridden to continue chain of responsibility
        IMPORT_C virtual MObjectProvider* MopNext();

private: // internal implementation
        IMPORT_C TAny* MopGetById(TTypeUid aId);
	friend class CLocalNotifyWindow;
	friend class CLocalNotifyWindowImpl;
        };





#ifndef __S80__

#include "ccu_utils.h"

class CLocalNotifyWindowImpl : public CLocalNotifyWindow, 
		public MContextBase {
	CLocalNotifyWindowImpl() { }

	~CLocalNotifyWindowImpl() {
		CALLSTACKITEM_N(_CL("CLocalNotifyWindowImpl"), _CL("CLocalNotifyWindowImpl"));

#ifdef __S60V2__
		delete iBackground;
#endif
		if (iResource) iEikonEnv->DeleteResourceFile(iResource);

		CList<TSlot>::Node	*i;
		if (iSlots) {
			for (i=iSlots->iFirst; i; i=i->Next) {
				delete i->Item.iImage;
			}
		}
		delete iSlots;
		delete iToBeNotified;
		delete iIconChanges;
	}

	void SetPositions() {
		// in initialization
		if (!iSlots) return;
		iPrevIndex=0; iPrevNode=iSlots->iFirst;

		TInt ignored;
		if (iSp) {
			TRAP(ignored, iPaneRect=iSp->PaneRectL(TUid::Uid(EEikStatusPaneUidTitle)));
		}
		
		TInt width=iScale*9;
		
		if (iSlots->iFirst) {
			width=iSlots->iFirst->Item.iImage->Size().iWidth+iScale;
		}
		TInt xpos=iPaneRect.iTl.iX+iPaneRect.Width()-width;
		TInt ypos=0;
#ifdef __S60V3__	
		TSize legacy(176, 208);
		TSize s;
		
		if ( ! AknLayoutUtils::LayoutMetricsSize(AknLayoutUtils::EScreen, s) )
			s = legacy;
		TBool landscape = s.iWidth > s.iHeight;
		AknLayoutUtils::TAknCbaLocation  cba=AknLayoutUtils::CbaLocation();
		
		if ( landscape ) {
				if (cba == AknLayoutUtils::EAknCbaLocationLeft ) {
					xpos=iPaneRect.iTl.iX+iScale;
				} else if ( cba == AknLayoutUtils::EAknCbaLocationBottom ) {
				  xpos=iPaneRect.iTl.iX + iPaneRect.Width() + 1;
				}
		}

#endif
		CList<TSlot>::Node	*i;
		for (i=iSlots->iFirst; i; i=i->Next) {
			ypos += iScale;
			i->Item.iImage->SetPosition(TPoint(0, ypos));
			ypos += i->Item.iImage->Size().iHeight;
		}
#ifdef __WINS__
		// and why do you have such big eyes?
		// so that I can see your skin better
		ypos+=iScale*10;
#endif

		TPoint tl( xpos, iPaneRect.iTl.iY);
		TSize sz( width, ypos );
		
		SetRect( TRect(tl, sz) );
	}

	void DrawOnGc(CWindowGc& gc, TPoint& aFromPos) {
		TInt xpos=aFromPos.iX, ypos=aFromPos.iY;

                gc.SetBrushStyle(CGraphicsContext::ENullBrush);
		CList<TSlot>::Node	*i;
		for (i=iSlots->iFirst; i; i=i->Next) {
			ypos += 1;
			TRect r( TPoint(0, 0), i->Item.iImage->Bitmap()->SizeInPixels());
			gc.BitBltMasked(TPoint(xpos, ypos), 
				i->Item.iImage->Bitmap(), r, i->Item.iImage->Mask(), ETrue);

			ypos += i->Item.iImage->Size().iHeight;
		}
	}
	
#ifdef __S60V2__
	virtual void Draw(const TRect& aRect) const {
		CALLSTACKITEM_N(_CL("CLocalNotifyWindowImpl"), _CL("Draw"));
		CWindowGc& gc = SystemGc();

		if (iBackground) {
			//AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, 
				//this, gc, aRect );
			AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, gc, aRect );
		} else {
			gc.Clear();
		}
	}
	TRect	iPaneRect;
	MObjectProviderEx* iParent;
	CAknsBasicBackgroundControlContext* iBackground;
	virtual void HandleResourceChange(TInt aType) {
		if ( aType == KEikDynamicLayoutVariantSwitch ) {
			SetPositions();
			Redraw();
			DrawNow();
		}
	}
	virtual void SizeChanged() {
		
		if (iBackground) {
			delete iBackground;
			iBackground=0;
		}

		TAknsItemID id = StatusPaneBackgroundSkin();
		TInt ignored;
		TRAP(ignored, iBackground=CAknsBasicBackgroundControlContext::NewL(id,
																		   TRect(0, 0, 0, 0),
																		   EFalse ));
		TRect r(0, 0, Rect().Width(), Rect().Height());
		r.Move( - Window().Position().iX, 0 );
		r.Resize( Window().Position().iX, 0 );
#ifdef __WINS__
		TBuf<100> msg;
		msg=_L("Rect: ");
		msg.AppendNum( r.iTl.iX ); msg.Append(_L(" "));
		msg.AppendNum( r.iTl.iY ); msg.Append(_L(" "));
		msg.AppendNum( r.Width() ); msg.Append(_L(" "));
		msg.AppendNum( r.Height() ); msg.Append(_L(" "));
		RDebug::Print(msg);
#endif
		if (iBackground) iBackground->SetRect( r );
	}
#endif
	TInt iScale;
    class CIconChanges : public CActive, public MContextBase {
	    static CIconChanges* NewL(CLocalNotifyWindow* aLocalNotify)
	    {
		CALLSTACKITEMSTATIC_N(_CL("CIconChanges"), _CL("NewL"));
		auto_ptr<CIconChanges> self( new (ELeave) CIconChanges );
		self->ConstructL( aLocalNotify );
		return self.release();
	    }

		CIconChanges() : CActive(EPriorityIdle) { }
		RContextNotifyClientSession iSession;
		TInt	iIcons[10];
		CLocalNotifyWindow* iLocalNotify;
		RTimer iTimer;
		TBool iConnected;
		void ConstructL(CLocalNotifyWindow* aLocalNotify) {
		    CALLSTACKITEM_N(_CL("CIconChanges"), _CL("ConstructL"));
		    CActiveScheduler::Add(this);
		    iLocalNotify=aLocalNotify;
		    iInitial=ETrue;
		    User::LeaveIfError(iTimer.CreateLocal());
		    iTimer.After(iStatus, TTimeIntervalMicroSeconds32(300*1000));
		    SetActive();
		}
		TBool iInitial;
		void RunL() {
			if (!iConnected) {
			    User::LeaveIfError(iSession.Connect());
			    iTimer.Close();
			    iConnected=ETrue;
			    iSession.NotifyOnIconChange(iIcons, 10, iStatus);
			    SetActive();
			    return;
			}
			if (iInitial) {
				TRequestStatus* s=&iStatus;
				iInitial=EFalse;
				User::RequestComplete(s, KErrNone);
				SetActive();
				return;
			}
			iLocalNotify->IconsChanged(iIcons, 10);
			iSession.NotifyOnIconChange(iIcons, 10, iStatus);
			SetActive();
		}
		void DoCancel() {
			if (iConnected) {
				iSession.Cancel();
			} else {
				iTimer.Cancel();
			}
		}
		~CIconChanges() {
			Cancel();
			if (iConnected) {		
				iSession.Close();
			} else {
				iTimer.Close();
			}
		}
		friend class CLocalNotifyWindowImpl;
	};
	CIconChanges*	iIconChanges;
	void ConstructL() {
		CALLSTACKITEM_N(_CL("CLocalNotifyWindowImpl"), _CL("ConstructL"));
		Reporting().UserErrorLog( _L("E61 localnotify debug variant"));
		iScale=1;
		
		/* if we just CreateWindowL() we get a 'floating' window,
		   since each new window is created on top of the 
		   previous ones. No key events will be routed if
		   we don't add it to the control stack.*/
		CreateWindowL();

		if ( ! iEikonEnv )
		    {
			Bug( _L("Eikon Environment not found") ).Raise();
		    }
		
#ifdef __S60V2__
		CEikAppUi* appui = (CEikAppUi*) iEikonEnv->AppUi();
		if ( appui && appui->ApplicationRect().Width() > 300 )
		    {
			CALLSTACKITEM_N( _CL("CLocalNotifyWindowImpl"), _CL("1a") );
			iScale=2;
		    }
		else
		    {
			CALLSTACKITEM_N( _CL("CLocalNotifyWindowImpl"), _CL("1b") );
		    }
#endif
		
		if ( ! iEikonEnv->AppUiFactory() )
		    {
			Bug( _L("AppUiFactory not available")).Raise();
		    }
		iSp=iEikonEnv->AppUiFactory()->StatusPane();

		iSlots=CList<TSlot>::NewL();
		iToBeNotified=CList<CCoeControl*>::NewL();

		iIconChanges= CIconChanges::NewL( this );
		SetPositions();
		ActivateL();
	}

	void Redraw() {
		CALLSTACKITEM_N(_CL("CLocalNotifyWindowImpl"), _CL("Redraw"));
		DrawDeferred();
		CList<CCoeControl*>::Node *i;
		for (i=iToBeNotified->iFirst; i; i=i->Next) {
			i->Item->DrawDeferred();
		}
	}
	virtual void IconsChanged(TInt* aIconIds, TInt aCount) {
		{
			CList<TSlot>::Node	*i=0;
			for (i=iSlots->iFirst; i; i=i->Next) {
				delete i->Item.iImage;
			}
			iSlots->reset();
		}
		TInt i=0;
		while (i<aCount) {
			if (aIconIds[i]==0) break;
			CFbsBitmap* bm=new (ELeave) CFbsBitmap;
			CleanupStack::PushL(bm);
			User::LeaveIfError(bm->Duplicate(aIconIds[i]));
			i++;
			CFbsBitmap* mask=new (ELeave) CFbsBitmap;
			CleanupStack::PushL(mask);
			User::LeaveIfError(mask->Duplicate(aIconIds[i]));
			i++;
			CEikImage* im=new (ELeave) CEikImage();
			im->SetPicture(bm, mask);
			im->SetPictureOwnedExternally(EFalse);
			CleanupStack::Pop(2);
			CleanupStack::PushL(im);
			im->SetSize(bm->SizeInPixels());
			iSlots->AppendL(TSlot(0, im));
			CleanupStack::Pop(1);
		}
		SetPositions();
		Redraw();
	}

	TInt CountComponentControls() const {
		CALLSTACKITEM_N(_CL("CLocalNotifyWindowImpl"), _CL("CountComponentControls"));
		return iSlots->iCount;
	}
	CCoeControl* ComponentControl(TInt aIndex) const {
		CALLSTACKITEM_N(_CL("CLocalNotifyWindowImpl"), _CL("ComponentControl"));
		if (aIndex==0) {
			iPrevNode=iSlots->iFirst;
			iPrevIndex=0;
		} else {
			while (iPrevIndex < aIndex) {
				iPrevNode=iPrevNode->Next;
				iPrevIndex++;
			}
		}
		return iPrevNode->Item.iImage;
	}

	virtual void AddNotifiedControl(CCoeControl* aControl) {
		iToBeNotified->AppendL(aControl);
	}
	virtual void RemoveNotifiedControl(CCoeControl* aControl) {
		CList<CCoeControl*>::Node *i;
		for (i=iToBeNotified->iFirst; i; i=i->Next) {
			if (i->Item == aControl) {
				iToBeNotified->DeleteNode(i, ETrue);
				return;
			}
		}
	}

	TInt	iLastId;
	mutable TInt	iRefCount;

	struct TSlot {
		TInt		iId;
		CEikImage*	iImage;

		TSlot() : iId(-1), iImage(0) { }
		TSlot(TInt aId, CEikImage* aImage) :
		iId(aId), iImage(aImage) { }
	};
	CList<TSlot>*	iSlots;
	CList<CCoeControl*> *iToBeNotified;
	CEikStatusPane* iSp;

	mutable CList<TSlot>::Node *iPrevNode; 
	mutable TInt iPrevIndex;
	TInt iResource;

	friend class CLocalNotifyWindow;
};


EXPORT_C void CNotifyWindowControl::Draw(const TRect& ) const
{
	CWindowGc& gc = SystemGc();
	TPoint pos=Position();
	if (iLocal) iLocal->DrawOnGc(gc, pos);
}

EXPORT_C void CNotifyWindowControl::ConstructL(CCoeControl *aTopLevel)
{
	iTopLevel=aTopLevel;
	iLocal=CLocalNotifyWindow::Global();
	if (iLocal) iLocal->AddNotifiedControl(iTopLevel);
}

EXPORT_C CNotifyWindowControl::~CNotifyWindowControl()
{
	if (iLocal)  {
		iLocal->RemoveNotifiedControl(iTopLevel);
	}
}

CLocalNotifyWindow::~CLocalNotifyWindow() { }

EXPORT_C CLocalNotifyWindow* CLocalNotifyWindow::Global()
{
	if (! CEikonEnv::Static()) return 0;
	CLocalNotifyWindowImpl* w=(CLocalNotifyWindowImpl*)Dll::Tls();
	return w;
}

EXPORT_C void CLocalNotifyWindow::Destroy()
{
	if (! CEikonEnv::Static()) return;
	CLocalNotifyWindowImpl* w=(CLocalNotifyWindowImpl*)Dll::Tls();

	if (!w) return;
	User::LeaveIfError(Dll::SetTls(0));
	delete w;
}


EXPORT_C void CLocalNotifyWindow::CreateAndActivateL()
{
	if (! CEikonEnv::Static()) return;
	void* existing=Dll::Tls();
	CLocalNotifyWindowImpl* w=0;
	if (existing) {
		User::Leave(KErrAlreadyExists);
	} else {
		auto_ptr<CLocalNotifyWindowImpl> wp(new (ELeave) CLocalNotifyWindowImpl);
		w=wp.get();
		
		w->ConstructL();
		User::LeaveIfError( Dll::SetTls(w) );
		wp.release();
	}
}

#endif
EXPORT_C RContextNotifyClientSession::RContextNotifyClientSession() : RSessionBase(), iIdPckg(0), iNotifyBuffer(0, 0)
{
	
}

#if defined(__WINS__)
IMPORT_C TInt ContextNotifyThreadFunction(TAny* aParam);
#endif

EXPORT_C TInt RContextNotifyClientSession::Connect()
{
	TInt result;
	
#if defined(__WINS__)
	CC_TRAP(result, StartServerL(KContextNotifyName, ContextNotifyThreadFunction));
#else
	CC_TRAP(result, StartServerL(KContextNotifyName, KServerUid3, KContextNotifyFilename));
#endif
	if (result == KErrNone)
	{
		result = CreateSession(KContextNotifyName,
			Version(),
			KDefaultMessageSlots);	
	}
	return result;
}

EXPORT_C void RContextNotifyClientSession::Close()
{
	RSessionBase::Close();
	delete iIdPckg; iIdPckg=0;
}

EXPORT_C TVersion RContextNotifyClientSession::Version() const
{
	return(TVersion(KContextNotifyMajorVersionNumber,
		KContextNotifyMinorVersionNumber,
		KContextNotifyBuildVersionNumber));
}

//----------------------------------------------------------------------------------------
#ifndef __S60V2__
void RContextNotifyClientSession::SendReceive(TInt aFunction, TRequestStatus& aStatus)
{
	RSessionBase::SendReceive(aFunction, 0, aStatus);
}
void RContextNotifyClientSession::SendReceive(TInt aFunction)
{
	RSessionBase::SendReceive(aFunction, 0);
}
#endif


EXPORT_C void RContextNotifyClientSession::Cancel() const
{
	SendReceive(ECancel);
}

EXPORT_C void RContextNotifyClientSession::TerminateContextNotify(TRequestStatus& aStatus)
{
	SendReceive(ETerminateContextNotify, aStatus);
}

EXPORT_C void RContextNotifyClientSession::AddIcon(CFbsBitmap* aIcon, CFbsBitmap* aMask, TInt& aId, TRequestStatus& aStatus)
{
	delete iIdPckg; iIdPckg=0; iIdPckg=new (ELeave) TPckg<TInt>(aId);

#ifndef __IPCV2__
	TAny* messageParameters[KMaxMessageArguments];
	messageParameters[0] = (void*)aIcon->Handle();
	messageParameters[1] = (void*)aMask->Handle();
	messageParameters[2] = (void*)iIdPckg;
#else
	TIpcArgs messageParameters(aIcon->Handle(), aMask->Handle(), iIdPckg, 0);
#endif

	SendReceive(EAddIcon, messageParameters, aStatus);
}

EXPORT_C void RContextNotifyClientSession::RemoveIcon(TInt aId, TRequestStatus& aStatus)
{
#ifndef __IPCV2__
	TAny* messageParameters[KMaxMessageArguments];
	messageParameters[0] = (void*)aId;
#else
	TIpcArgs messageParameters(aId);
#endif
	SendReceive(ERemoveIcon, messageParameters, aStatus);
}

EXPORT_C void RContextNotifyClientSession::ChangeIcon(CFbsBitmap* aIcon, CFbsBitmap* aMask, TInt aId, TRequestStatus& aStatus)
{
#ifndef __IPCV2__
	TAny* messageParameters[KMaxMessageArguments];
	messageParameters[0] = (void*)aIcon->Handle();
	messageParameters[1] = (void*)aMask->Handle();
	messageParameters[2] = (void*)aId;
#else
	TIpcArgs messageParameters(aIcon->Handle(), aMask->Handle(), aId);
#endif

	SendReceive(EChangeIcon, messageParameters, aStatus);
}

EXPORT_C void RContextNotifyClientSession::NotifyOnIconChange(TInt* aIconIds, TInt aCount, TRequestStatus& aStatus)
{
	aStatus=KRequestPending;
	for (int i=0; i<aCount; i++) {
		aIconIds[i]=0;
	}
	iNotifyBuffer.Set( (TUint8*)aIconIds, aCount*sizeof(TInt), aCount*sizeof(TInt) );
	TIpcArgs args(&iNotifyBuffer);
	SendReceive(ENotifyOnIconChange, args, aStatus);
}

EXPORT_C void RContextNotifyClientSession::SetIdleBackground(CFbsBitmap* aBitmap, TRequestStatus& aStatus)
{
	aStatus=KRequestPending;
	TIpcArgs messageParameters(aBitmap->Handle());
	SendReceive(ESetBackground, messageParameters, aStatus);
}
