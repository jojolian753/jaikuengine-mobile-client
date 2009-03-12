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

#include "jsui_button.h"

#include "ccu_feedfoundation.h"
#include "ccu_streamrenderers.h"

#include "juik_fonts.h"
#include "juik_label.h"
#include "jaiku_layout.h"

class CJsuiButtonImpl : public CJsuiButton, public MFeedFoundation
{
public:
	static CJsuiButtonImpl* NewL(const TDesC& aText,
								 const TUiDelegates& aDelegates,
								 TInt aFlags)
	{
		CALLSTACKITEMSTATIC_N(_CL("CJsuiButtonImpl"), _CL("NewL"));
		auto_ptr<CJsuiButtonImpl> self( new (ELeave) CJsuiButtonImpl(aDelegates, aFlags) );
		self->ConstructL(aText);
		return self.release();
	}
	
	CJsuiButtonImpl(const TUiDelegates& aDelegates,TInt aFlags) 
		: MFeedFoundation(aDelegates)
	{
	}

	/**
	 * This is utterly stupid to have own MakeHidden, but it seems that 
	 * calling MakeVisible causes KERN-EXEC 3 in platform code
	 */ 
	virtual void SetHidden(TBool aHide)
	{
		iHidden = aHide;
	}

	virtual TBool IsHidden() const
	{
		return iHidden;
	}
	
	TSize MinimumSize() 
	{
		if ( iHidden )
			return TSize(0,0);
		else
			return CJsuiButton::MinimumSize();
	}

	
	void SizeChanged()
	{
		TRect r = iMargin.InnerRect(Rect());
		iButtonRenderer->SetWidth( r.Width() );
		CJsuiButton::SizeChanged();
	}
	
	void ConstructL(const TDesC& aText) 
	{
		CALLSTACKITEM_N(_CL("CJsuiButtonImpl"), _CL("ConstructL"));		
		BaseConstructL();
		iMargin = Layout().GetLayoutItemL(LG_feed_controls_margins, LI_feed_controls_margins__button_out).Margins();
		iButtonRenderer = CButtonRenderer::NewL( iDelegates );

		TInt runningId = 1000;
		{			
			{
				MJuikSizer* sizer = Juik::CreateBoxSizerL( Juik::EVertical );				 
				RootContainer().SetRootSizerL( sizer );
			}
			{
				iButtonLabel = CJuikLabel::NewL( PrimaryTextColor(), PrimaryHighlightTextColor(),
												 JuikFonts::Normal(), RootContainer() );
				RootContainer().AddControlL( iButtonLabel, runningId++ );
				iButtonLabel->UpdateTextL( aText );
				iButtonLabel->iMargin = Layout().GetLayoutItemL(LG_feed_controls_margins, 
																LI_feed_controls_margins__button_in).Margins();
				RootContainer().GetRootSizerL()->AddL( *iButtonLabel, 0, Juik::EAlignCenterHorizontal);
			}
			
		}
	}
	
	~CJsuiButtonImpl()
	{
		delete iButtonRenderer;
	}


	void Draw(const TRect& /*aRect*/) const
	{
		CALLSTACKITEM_N(_CL("CJsuiButtonImpl"), _CL("Draw"));
		CWindowGc& gc = SystemGc();
		TRect r = iMargin.InnerRect( Rect() );
		iButtonRenderer->DrawButton(r, gc, IsFocused());
	}

	void SetTextL(const TDesC& aText) 
	{
		CALLSTACKITEM_N(_CL("CJsuiButtonImpl"), _CL("SetTextL"));
		iButtonLabel->UpdateTextL(aText);
	}

public: // from MJuikControl
 
	virtual const CCoeControl* CoeControl() const { return this; }
	virtual CCoeControl* CoeControl() { return this; }

	
private:	
	CJuikLabel* iButtonLabel;
	CButtonRenderer* iButtonRenderer;


	TBool iHidden;	
};

	
CJsuiButton* CJsuiButton::NewL(const TDesC& aText,
							   const TUiDelegates& aDelegates,
							   TInt aFlags)
{
	CALLSTACKITEMSTATIC_N(_CL("CJsuiButton"), _CL("NewL"));
	return CJsuiButtonImpl::NewL( aText, aDelegates, aFlags );
}
