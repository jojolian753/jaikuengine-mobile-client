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

#include "ccu_mainbgcontainer.h"

#include "ccu_themes.h"
#include "ccu_progressbar.h"

#include "app_context.h"
#include "break.h"
#include "contextcontacts.hrh"
#include "juik_animation.h"
#include "juik_label.h"
#include "juik_layout.h"
#include "juik_fonts.h"
#include "juik_sizercontainer.h"
#include "juik_sizer.h"
#include "jaiku_layoutids.hrh"
#include "symbian_auto_ptr.h"

#include <aknappui.h>
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <eikenv.h>
#include <gulalign.h>

#include "cl_settings.h"
#include "settings.h"


//#define SHOW_CONNECTION_STATUS_DETAILS

//#define ANIMATED_HIDE
class CProgressBar : public CCoeControl, public MContextBase, public CProgressBarModel::MListener
#ifdef ANIMATED_HIDE
, public CJuikAnimation::MAnimated 
#endif
{
public:
	class MContainer
	{
	public:
		virtual void RequestHeight(TInt aWantedHeight) = 0;
		virtual void DrawProgressBarBg(CWindowGc& aGc, const TRect& aWindowRect) const = 0;
	}& iContainer;


	CProgressBar(CProgressBarModel& aProgBarModel, CThemeColors& aThemeColors, MContainer& aContainer) 
		: iModel( aProgBarModel ), iThemeColors(aThemeColors), iContainer(aContainer) {}

	CJuikLabel* CreateAndAddLabelL(TInt aId)
	{
		auto_ptr<CJuikLabel> labelP( CJuikLabel::NewL( iFgColor, iFgColor, JuikFonts::Small(), *iContent ) );
		CJuikLabel* label = labelP.get();
		iContent->AddControlL( labelP.release(), aId );

		label->SetDebugId( aId );
		label->SetAlignment( EHCenterVCenter );
		label->SetLabelAlignment( ELayoutAlignCenter );
		
		iContent->GetRootSizerL()->AddL( *label, 0, Juik::EExpand | Juik::EAlignCenterHorizontal );
		return label;
	}
	
	
	void ConstructL(CCoeControl* aParent, CAknsBasicBackgroundControlContext* aBackground) 
	{
		CALLSTACKITEM_N(_CL("CProgressBar"), _CL("ConstructL"));
		
		iAlphaBgColor = iThemeColors.GetColorL( CThemeColors::EProgressAlphaBackground );
		iFgColor = iThemeColors.GetColorL( CThemeColors::EProgressText );
		iBgColor = iThemeColors.GetColorL( CThemeColors::EProgressBackground );
		
		iBackground = aBackground;
		iBorderColor=KRgbBlack;
		iBorder=0;
		
		//SetContainerWindowL( *aParent );
		CreateWindowL();

#ifdef ANIMATED_HIDE
		iAnimation = CJuikAnimation::NewL();
#endif

		
		iContent = CJuikSizerContainer::NewL();
		MJuikSizer* sizer = Juik::CreateFixedWidthSizerL();
		iContent->SetRootSizerL( sizer );

		TInt runningId = 7000;

#ifdef SHOW_CONNECTION_STATUS_DETAILS
		iDevLabel = CreateAndAddLabelL(  runningId++ );
#else 
		iDevLabel = NULL;
#endif 
		iFetchLabel = CreateAndAddLabelL(  runningId++ );
		iStatusLabel = CreateAndAddLabelL(  runningId++ );
		
		UpdateAllL();
		
		iWantedHeight = iModel.Progress() == 100 ? 0 : MaxHeight();
		iModel.AddListenerL( *this );		
		
		SetRect(TRect());
		ActivateL();
	}
	

	void UpdateAllL( ) 
	{
		UpdateMessageL( iDevLabel, iModel.Message( CProgressBarModel::EDevMessage ) );
		UpdateMessageL( iFetchLabel, iModel.Message( CProgressBarModel::EFetchMessage ) );
		UpdateMessageL( iStatusLabel, iModel.Message( CProgressBarModel::EStatusMessage ) );
	}

	void UpdateMessageL(CJuikLabel* aLabel, const TDesC& aMessage)
	{
		if ( aLabel )
			{
				if ( aMessage.Length() == 0)
					aLabel->ZeroL();
				else
					{
						const CFont* font =  aMessage.Length() > 30 ? JuikFonts::Tiny() : JuikFonts::Small();
						if ( aLabel->Font() !=  font)
							aLabel->SetFontProperlyL( font );
						aLabel->UpdateTextL( aMessage );
					}
			}
	}

	TInt CountComponentControls() const
	{
		return 1;
	}

	CCoeControl *ComponentControl(TInt aIndex) const
	{
		return iContent;
	}

	void Draw(const	TRect& aRect) const 
	{
		CALLSTACKITEM_N(_CL("CProgressBar"), _CL("Draw"));
 		CWindowGc& gc =	SystemGc();
 		TRect r=Rect();
		
		MAknsSkinInstance* skin = AknsUtils::SkinInstance();
		
		{
			if ( OwnsWindow() )
				{
					RWindow& w = Window();
					TRect r( w.AbsPosition(), w.Size() );
					iContainer.DrawProgressBarBg( gc, r );
				}
			else
				{
					MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );
					AknsDrawUtils::Background(skin, cc, this, gc, r);
				}
		}
		
		TRgb alphaBg = iAlphaBgColor;
		gc.SetPenStyle(CGraphicsContext::ESolidPen);
		gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
		gc.SetBrushColor(alphaBg);			
		gc.SetPenColor(alphaBg);			
		gc.DrawRect(r);

		if ( iModel.Progress() !=100) {


			gc.SetPenStyle(CGraphicsContext::ESolidPen);
 			gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
			gc.SetBrushColor(iBgColor);			
			gc.SetPenColor(iFgColor);			
			TReal p= 100 - iModel.Progress();
			p/=100.0;
			p*=r.Width();
			TRect left = r;
			left.SetWidth( p );

			TInt h = 0;
			if ( iStatusLabel ) h = iStatusLabel->Rect().Height();
			if ( h > 0 )
				{
					left.SetHeight( h );
				}

			left.Move( r.Width() - left.Width(), r.Height() - left.Height() );

			gc.DrawRect(left);
		}
	}

	void MessageChanged(CProgressBarModel::TMessageId aId, const TDesC& aMessage) 
	{
		CALLSTACKITEM_N(_CL("CProgressBar"), _CL("MessageChanged"));
		switch (aId )
			{
			case CProgressBarModel::EDevMessage:         
				UpdateMessageL( iDevLabel, aMessage ); break;
			case CProgressBarModel::EFetchMessage:       
				UpdateMessageL( iFetchLabel, aMessage ); break;
			case CProgressBarModel::EStatusMessage:      
				UpdateMessageL( iStatusLabel, aMessage ); break;
			};
		
		if ( Rect().Height() != WantedHeight() )
			{
				iContainer.RequestHeight( WantedHeight() );
			}
		else
			{
				DrawDeferred();
			}
	}

	void ProgressChanged(TInt aProgress) 
	{
		CALLSTACKITEM_N(_CL("CProgressBar"), _CL("ProgressChanged"));
		if ( iModel.Progress() == 100 )
			{
#ifdef ANIMATED_HIDE
				iAnimation->StartL(*this, TTimeIntervalMicroSeconds(  700 * 1000 ) );
				DrawDeferred();
#else
				iWantedHeight = 0;
				iContainer.RequestHeight( WantedHeight() );
#endif
				
			}
		else
			{
#ifdef ANIMATED_HIDE
				iAnimation->Stop();
				ProgressL(0.0);
				DrawDeferred();
#else
				iWantedHeight = MaxHeight();
				iContainer.RequestHeight( WantedHeight() );
				DrawNow();
#endif
			}
	}
	
	~CProgressBar() {
		CALLSTACKITEM_N(_CL("CProgressBar"), _CL("~CProgressBar"));
#ifdef ANIMATED_HIDE
		delete iAnimation;
#endif
		iModel.RemoveListener( *this );
		delete iContent;
	}
	
	virtual void SizeChanged()
	{
		TRect r = Rect();
		iContent->SetRect( r );
	}
	
	
	TInt MaxHeight()
	{
		TInt H = Layout().GetLayoutItemL( LG_progressbar, LI_progressbar__message ).h;
		return H;
	}


#ifdef ANIMATED_HIDE
	virtual void ProgressL(TReal aProgress)
	{
		CALLSTACKITEM_N(_CL("CProgressBar"), _CL("ProgressL"));
		iWantedHeight = MaxHeight() * (1.0 - aProgress);
		if ( Rect().Height() != WantedHeight() )
			{
				iContainer.RequestHeight( WantedHeight() );
			}
	}
	
	virtual void FinishedL() 
	{
		CALLSTACKITEM_N(_CL("CProgressBar"), _CL("Finished"));
		ProgressL( 1.0 );
	}	
#endif	
	
	TInt WantedHeight() const
	{
// 		if ( iModel.Progress() == 100 )
// 			return 0;
		if ( iContent )
			return Min(100, iContent->MinimumSize().iHeight);
		return iWantedHeight;
	}
	
#ifdef ANIMATED_HIDE
	CJuikAnimation* iAnimation;
#endif 

	TInt iWantedHeight;

	TInt iBorder; 
	TRgb iBorderColor;
	TRgb iBgColor;
	TRgb iFgColor;
	TRgb iAlphaBgColor;


	CProgressBarModel& iModel;
	CThemeColors& iThemeColors;

	CAknsBasicBackgroundControlContext* iBackground;
	CJuikLabel* iStatusLabel; // not owned, owned by iContainer
	CJuikLabel* iFetchLabel; // not owned, owned by iContainer
	CJuikLabel* iDevLabel; // not owned, owned by iContainer
	CJuikSizerContainer* iContent;
};


class CMainBgContainerImpl : public CMainBgContainer, public MContextBase, public CProgressBar::MContainer
{
public:


	CMainBgContainerImpl(CThemeColors& aThemeColors, CProgressBarModel* aProgBarModel) 
		: iThemeColors(aThemeColors),
		  iProgressBarModel(aProgBarModel) 
	{}
	
	
	void ConstructL(MObjectProvider* aMopParent, const TRect& aRect)
	{
		CALLSTACKITEM_N(_CL("CMainBgContainerImpl"), _CL("ConstructL"));
		SetMopParent( aMopParent );
		CreateWindowL();				
		{ 
			TRect rect=aRect;
			rect.Move( 0, -rect.iTl.iY );
			iBackground=CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain,
																  rect, EFalse );
		}
		
		if ( iProgressBarModel )
			{
				iProgressBar = new (ELeave) CProgressBar( *iProgressBarModel, iThemeColors, *this);
				iProgressBar->ConstructL(this, iBackground);
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
		CALLSTACKITEM_N(_CL("CMainBgContainerImpl"), _CL("SetContentL"));
		iContent = aContent;
		SizeChanged();
	}

	void RequestHeight(TInt aWantedHeight)
	{
		if ( Rect().Height() != 0 && 
			 iProgressBar && iProgressBar->Rect().Height() != aWantedHeight )
			{
				SizeChanged();
				DrawDeferred();
			}
	}

	
	void SizeChangedImplL()
	{
		CALLSTACKITEM_N(_CL("CMainBgContainerImpl"), _CL("SizeChangedImpl"));
		TRect fullR = Rect();
		iBackground->SetRect(fullR);

		TBool progressBarAtTop = EFalse;

		TInt pbar = 0;
		if ( iProgressBar ) pbar = iProgressBar->WantedHeight();

		TInt contentH = 0;
		{
			TRect r = fullR;
			if ( progressBarAtTop )
				r.Move(0,pbar);
			r.SetHeight( r.Height() - pbar );
			if ( iContent )
				iContent->SetRect(r);
			contentH = r.Height();
		}
		
		if ( iProgressBar )
			{
				TRect r = fullR;
				
				if ( iProgressBar->OwnsWindow() ) 
					{
						TRect cr = iEikonEnv->EikAppUi()->ClientRect();
						r.Move(0, cr.iTl.iY); 
					}
				
				r.SetHeight( pbar );
				if ( ! progressBarAtTop )
					r.Move(0, contentH );
				iProgressBar->SetRect( r );
			}		
	}

	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CMainBgContainerImpl"), _CL("SizeChanged"));
		CC_TRAPD( err, SizeChangedImplL() );
		if ( err != KErrNone )
			{
				ReportActiveError( KNullDesC, KNullDesC, err );
				User::Leave(err);
			}
	}

	TInt CountComponentControls() const
	{
	    CALLSTACKITEM_N(_CL("CMainBgContainerImpl"), _CL("CountComponentControl"));
		if ( iContent && iProgressBar ) return 2;
		else if ( iContent || iProgressBar ) return 1;
		else return 0;
	}
	
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CMainBgContainerImpl"), _CL("ComponentControl"));
		switch ( aIndex ) 
			{
			case 0:
				if ( iProgressBar )
					return iProgressBar;
				else if ( iContent )
					return iContent;
				break;
			case 1:
				return iContent;
				break;
			}				
		return NULL;
	}
	

	~CMainBgContainerImpl() 
	{
		delete iBackground;
		delete iProgressBar;
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
		if ( aType == EEventKey )
			{   
				if ( aKeyEvent.iCode==EKeyLeftArrow )
					{
						CEikAppUi* appUi = static_cast<CEikAppUi*>( iEikonEnv->AppUi() );
						appUi->HandleCommandL(EContextContactsCmdLeftView);
						return EKeyWasConsumed;
					}
				if ( aKeyEvent.iCode==EKeyRightArrow )
					{
						CEikAppUi* appUi = static_cast<CEikAppUi*>( iEikonEnv->AppUi() );
						appUi->HandleCommandL(EContextContactsCmdRightView);
						return EKeyWasConsumed;
					}
			}
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
	
private:
	CThemeColors& iThemeColors;

	CProgressBarModel* iProgressBarModel; // not owned
	CAknsBasicBackgroundControlContext* iBackground;
	CCoeControl* iContent;
	CProgressBar* iProgressBar;

};


EXPORT_C CMainBgContainer* CMainBgContainer::NewL(MObjectProvider *aMopParent, 
												  const TRect& aRect, 
												  CThemeColors& aThemeColors,
												  CProgressBarModel& aProgBarModel)
{
	return NewL( aMopParent, aRect, aThemeColors, &aProgBarModel );
} 

EXPORT_C CMainBgContainer* CMainBgContainer::NewL(MObjectProvider *aMopParent, 
												  const TRect& aRect, 
												  CThemeColors& aThemeColors,
												  CProgressBarModel* aProgBarModel)
{
	CALLSTACKITEMSTATIC_N(_CL("CMainBgContainer"), _CL("NewL"));
	auto_ptr<CMainBgContainerImpl> self( new (ELeave) CMainBgContainerImpl(aThemeColors, aProgBarModel) );
	self->ConstructL(aMopParent, aRect);
	return self.release();
	
}
