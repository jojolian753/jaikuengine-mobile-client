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

#include "ccu_errorcontainer.h"

#include "ccu_themes.h"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"
#include "contextcontacts.hrh"
#include "juik_label.h"
#include "juik_sizercontainer.h"
#include "juik_sizer.h"
#include "juik_fonts.h"
#include "juik_keycodes.h"

#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <eikenv.h>
#include <aknappui.h>

#include "cl_settings.h"
#include "settings.h"


static const CFont* PrimaryFont() 
{
	return JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall );
}
	
static const CFont* SecondaryFont()
{
	return JuikFonts::GetLogicalFont( JuikFonts::EVerySmall );
}

static const CFont* SmallPrimaryFont()
{
	return JuikFonts::GetLogicalFont( JuikFonts::ESecondary );
}



class CErrorContainerImpl : public CErrorContainer, public MContextBase
{
public:
	CErrorContainerImpl(CThemeColors& aThemeColors) : iThemeColors(aThemeColors) {}
	
	enum 
		{
			EMessageLabel,
			EErrorLabel,
			EDetailsLabel
		};
	
	
	CJuikSizerContainer& Content() const
	{
		return *iContainer;
	}
	

	CJuikLabel* GetLabelL(TInt aId) const
	{
		return static_cast<CJuikLabel*>(Content().GetControlL(aId));
	}

	void ConstructL(MObjectProvider* aMopParent, const TRect& aRect, MErrorInfo* aErrorInfo)
	{
		CALLSTACKITEM_N(_CL("CErrorContainerImpl"), _CL("ConstructL"));
		SetMopParent( aMopParent );
		CreateWindowL();				
		{ 
			TRect rect=aRect;
			rect.Move( 0, -rect.iTl.iY );
			iBackground=CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain,
																  rect, EFalse );
		}
		
		iContainer = CJuikSizerContainer::NewL();
		iContainer->SetContainerWindowL(*this);
		
		{
			MJuikSizer* sizer = Juik::CreateFixedWidthSizerL();
			Content().SetRootSizerL( sizer );
		}
		
		{			
			auto_ptr<CJuikLabel> label = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
														   PrimaryFont(), Content() );
			label->SetAllMarginsTo( 4 );
			Content().AddControlL( label.get(), EMessageLabel );
			Content().GetRootSizerL()->AddL( *(label.release()), 0, Juik::EExpand ); 
			
			_LIT(KMessage, "Jaiku Mobile couldn't show this view due an error. If problem persists, contact us at bugs@jaiku.com.");
			GetLabelL(EMessageLabel)->UpdateTextL( KMessage );
			
		}
		

		if ( aErrorInfo )
			{
				{			
					auto_ptr<CJuikLabel> label = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
																   SecondaryFont(), Content() );
					label->SetAllMarginsTo( 4 );
					Content().AddControlL( label.get(), EErrorLabel );
					Content().GetRootSizerL()->AddL( *(label.release()), 0, Juik::EExpandNot );

					TPtrC p( aErrorInfo->UserMessage().Length() ?
							 aErrorInfo->UserMessage() : 
							 aErrorInfo->TechMessage() );
					auto_ptr<HBufC> b( HBufC::NewL( p.Length() + 100 ) );
					
					TBuf<14> pre;
					switch (aErrorInfo->Severity()) {
					case EInfo:
						pre=_L("[Info] ");
						break;
					case EWarning:
						pre=_L("[Warning] ");
						break;
					case EError:
						pre=_L("[Error] ");
						break;
					case ECorrupt:
						pre=_L("[Critical] ");
						break;
					}

					b->Des().Append( pre );
					b->Des().Append( p ); 
					
					GetLabelL(EErrorLabel)->UpdateTextL( *b );
				}
				
				{			
					auto_ptr<CJuikLabel> label = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
																   PrimaryFont(), Content() );
					label->SetAllMarginsTo( 4 );
					Content().AddControlL( label.get(), EDetailsLabel );
					Content().GetRootSizerL()->AddL( *(label.release()), 0, Juik::EExpand ); 
					{
						CJuikLabel* label = GetLabelL(EDetailsLabel);
						label->SetLabelAlignment( ELayoutAlignCenter );
						_LIT( KButtonText, "Show details");
						label->UpdateTextL( KButtonText );
						label->SetFocus( ETrue );
					}
				}
			}
		
		SetRect( aRect );
		ActivateL();
	}
		
	
	void SizeChangedImplL()
	{
		CALLSTACKITEM_N(_CL("CErrorContainerImpl"), _CL("SizeChangedImpl"));
		TRect fullR = Rect();
		iBackground->SetRect(fullR);
		
		if ( iContainer )
			iContainer->SetRect(fullR);
	}

	void SizeChanged()
	{
		CALLSTACKITEM_N(_CL("CErrorContainerImpl"), _CL("SizeChanged"));
		CC_TRAPD( err, SizeChangedImplL() );
		if ( err != KErrNone )
			{
				ReportActiveError( KNullDesC, KNullDesC, err );
				User::Leave(err);
			}
	}

	TInt CountComponentControls() const
	{
	    CALLSTACKITEM_N(_CL("CErrorContainerImpl"), _CL("CountComponentControl"));
		if ( iContainer ) return 1;
		else            return 0;
	}
	
	
	CCoeControl* ComponentControl(TInt aIndex) const
	{
		CALLSTACKITEM_N(_CL("CErrorContainerImpl"), _CL("ComponentControl"));
		if ( aIndex == 0 )
			return iContainer;
		return NULL;
	}
	

	~CErrorContainerImpl() 
	{
		delete iBackground;
		delete iContainer;
	}

	void Draw(const TRect& aRect) const
	{
		CWindowGc& gc = SystemGc();
		TRect r = Rect();
		TBool skinDrawn = AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, gc, r );
		if ( !skinDrawn )
			{
				gc.SetBrushColor( KRgbWhite );
				gc.Clear( r );
			}
		
		// Non-skin focus
		CJuikLabel* button = GetLabelL( EDetailsLabel );
		if ( button )
			{
				TRect focusRect = button->Rect();
				TRect innerRect = focusRect;
				innerRect.Shrink(4,4); 
				
				MAknsSkinInstance* skin = AknsUtils::SkinInstance();
				TBool skinnedFocus = AknsDrawUtils::DrawFrame( skin, gc, focusRect, innerRect, 
															   KAknsIIDQsnFrList, KAknsIIDQsnFrListCenter);
				if (!skinnedFocus)
					{
						gc.SetPenStyle( CGraphicsContext::ENullPen );
						gc.SetBrushColor( TRgb(200, 200, 200, 100) );
						gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
						gc.DrawRect( focusRect );
					}
			}
	}

	
	TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
	{
		if ( aType == EEventKey )
			{   
				TInt command = KErrNotFound;
				switch ( aKeyEvent.iCode ) 
					{
					case JOY_LEFT: command = EContextContactsCmdLeftView; break;
					case JOY_RIGHT: command = EContextContactsCmdRightView; break;
					case JOY_CLICK: command = EContextContactsCmdShowLastJaikuViewError; break;
					}
				if ( command != KErrNotFound )
					{
						CEikAppUi* appUi = static_cast<CEikAppUi*>( iEikonEnv->AppUi() );
						appUi->HandleCommandL(command);
						return EKeyWasConsumed;
					}
			}
		if ( iContainer )
			return iContainer->OfferKeyEventL(aKeyEvent, aType);
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
	

	CThemeColors& ThemeColors() const { return iThemeColors; }
   

	TRgb PrimaryTextColor() const
	{
		return ThemeColors().GetColorL( CThemeColors::EPrimaryText );
	}
	
	TRgb SecondaryTextColor() const
	{
		return ThemeColors().GetColorL( CThemeColors::ESecondaryText );
	}
	
	TRgb PrimaryHighlightTextColor() const
	{
		return ThemeColors().GetColorL( CThemeColors::EPrimaryHighlightText );
	}
	
	TRgb SecondaryHighlightTextColor() const
	{
		return ThemeColors().GetColorL( CThemeColors::ESecondaryHighlightText );
	}


private:
	CAknsBasicBackgroundControlContext* iBackground;
	CJuikSizerContainer* iContainer;
	CThemeColors& iThemeColors;
};


EXPORT_C CErrorContainer* CErrorContainer::NewL(MObjectProvider *aMopParent, 
												const TRect& aRect, 
												CThemeColors& aThemeColors, 
												MErrorInfo* aErrorInfo)
{
	CALLSTACKITEMSTATIC_N(_CL("CErrorContainer"), _CL("NewL"));
	auto_ptr<CErrorContainerImpl> self( new (ELeave) CErrorContainerImpl(aThemeColors) );
	self->ConstructL(aMopParent, aRect, aErrorInfo);
	return self.release();
} 
