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

#include <coemain.h>
#include <UiLoft.rsg>

#include "UiLoftAppView.h"
#include "MyPicture.h"

#include "symbian_auto_ptr.h"

#include <contextcontactsui.mbg>

#include <eikrted.h>
#include <TXTRICH.H>

#include <GULICON.H>
#include <AknsControlContext.h>
#include <AknsBasicBackgroundControlContext.h>
#include <AknsDrawUtils.h>
#include <AknUtils.h>
// Standard construction sequence
CUiLoftAppView* CUiLoftAppView::NewL(const TRect& aRect)
    {
    CUiLoftAppView* self = CUiLoftAppView::NewLC(aRect);
    CleanupStack::Pop(self);
    return self;
    }

CUiLoftAppView* CUiLoftAppView::NewLC(const TRect& aRect)
    {
    CUiLoftAppView* self = new (ELeave) CUiLoftAppView;
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    return self;
    }

CUiLoftAppView::CUiLoftAppView()
    {
	// no implementation required
    }

#include <eikedwin.h>

CUiLoftAppView::~CUiLoftAppView()
    {
	delete iEditor;
	delete iBackground;
	if ( iIcons )
		{
			iIcons->ResetAndDestroy();
		}
	delete iIcons;
    }

CCoeControl* CUiLoftAppView::ComponentControl(TInt aIndex) const
{
	return iEditor;
}

TInt CUiLoftAppView::CountComponentControls() const
{
	//return 0;
	return 1;
}

#include <akndef.h>
#include <eikenv.h>
#include <eikappui.h>

#include "juik_icons.h"
const TInt KMyIconCount(1);
static const TIconID KMyIconIds[ KMyIconCount ]= {
	_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
					EMbmContextcontactsuiLight_green,  EMbmContextcontactsuiLight_green_mask )
};


void CUiLoftAppView::HandleResourceChange( TInt aType )
{
	CCoeControl::HandleResourceChange(aType);
	if ( aType == KEikDynamicLayoutVariantSwitch ) {
		CEikAppUi* appui=(CEikAppUi*)iEikonEnv->AppUi();
		TRect r = appui->ClientRect();
		SetRect( r );
	}
}

_LIT(KText, "Select Options to repair, disable or report bugs about Jaiku");
void CUiLoftAppView::ConstructL(const TRect& aRect)
    {
    // Create a window for this application view
    CreateWindowL();
	
	const CFont* sysfont = AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont); 
	TFontSpec sysfontspec = sysfont->FontSpecInTwips();
	iIcons = new (ELeave) CArrayPtrFlat<CGulIcon>(10);
	JuikIcons::LoadIconsL( iIcons, KMyIconIds, KMyIconCount);
	AknIconUtils::SetSize(iIcons->At(0)->Bitmap(), TSize(70,70));
	iEditor = new (ELeave) CEikRichTextEditor();
	iEditor->SetContainerWindowL(*this);
	iEditor->ConstructL(this, 1000,1000,CEikEdwin::EAllowPictures | CEikEdwin::EReadOnly ,EGulFontControlAll,EGulAllFonts);
	


	//iEditor->SetRect(TRect(TSize(Rect().Width() - 10,Rect().Height())));
// 	iEditor-> CreateScrollBarFrameL()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EOn );

	
	
//  	CRichText* richtext = iEditor->RichText();
	
// 	InsertMyPictureL(0);
// 	InsertMyPictureL(0);
// 	richtext->AppendParagraphL();

// 	_LIT(KBy,"Repsu:");
// 	_LIT(KPost, "They are planning to cut my balls off...");
// 	_LIT(KTimestamp, "3 weeks, 4 days ago. ");
// 	_LIT(KCommentCount, "6 Comments");


// 	auto_ptr<CParaFormat> paraFormat( CParaFormat::NewL() );
// 	paraFormat->iLeftMarginInTwips = 400;
// 	TParaFormatMask paraMask;
// 	paraMask.SetAttrib( EAttLeftMargin );

// 	TCharFormat charfmt;
// 	charfmt.iFontSpec = sysfontspec;
// 	TCharFormatMask charmask;
// 	charmask.SetAll();

// 	richtext->InsertL( richtext->DocumentLength(), KBy);
// 	richtext->ApplyCharFormatL( charfmt, charmask, 3, 5);
// 	richtext->AppendParagraphL();
// 	richtext->InsertL( richtext->DocumentLength(), KPost);
// 	TInt last = richtext->DocumentLength() - 1;
//  	richtext->ApplyParaFormatL( paraFormat.get(), paraMask, last, 1);
// 	richtext->AppendParagraphL();
// 	richtext->InsertL( richtext->DocumentLength(), KTimestamp);
// 	richtext->AppendParagraphL();
// 	richtext->InsertL( richtext->DocumentLength(), KCommentCount);
// 	last = richtext->DocumentLength() - 1;
//  	richtext->ApplyParaFormatL( paraFormat.get(), paraMask, last, 1);
	
// 	{
		
// // 		TFontSpec fontspec = LatinBold19()->FontSpecInTwips();
//  		TCharFormat charFormat;
// 			//fontspec.iTypeface.iName,
// 			//				fontspec.iHeight );
//  		TCharFormatMask charFormatMask;
		
// 		// 	   // Set text color
//  		charFormat.iFontPresentation.iTextColor = KRgbBlue;
		
// // 		// Activate the attributes
//  		charFormatMask.SetAttrib(EAttColor);
// //   	charFormatMask.SetAttrib(EAttFontTypeface);
// // 		charFormatMask.SetAttrib(EAttFontHeight);
		
// 		// Apply font to the whole text
// // 		iEditor->SelectAllL();
// 		iEditor->SelectAllL();
// 		iEditor->ApplyCharFormatL(charFormat, charFormatMask);
// 		iEditor->ClearSelectionL();
		
// 	}

	
	
	{
		CGlobalText *text=iEditor->GlobalText();
		
		TCharFormat cf;
		TCharFormatMask cfm;
		text->GetCharFormat(cf,cfm,0,0);

		cf.iFontPresentation.iTextColor=TRgb(0);
		cfm.SetAttrib(EAttColor);
		
		CGraphicsDevice *dev=SystemGc().Device();
		auto_ptr<CDesCArrayFlat> fontnames(new(ELeave)CDesCArrayFlat(5));

		FontUtils::GetAvailableFontsL(*dev,*fontnames,EGulAllFonts);
		
		for(TInt i=0;i<fontnames->Count();i++)
			{
				cf.iFontSpec.iTypeface.iName=_L("LatinPlain12");
				cfm.SetAttrib(EAttFontTypeface);
				text->ApplyCharFormatL(cf,cfm,text->DocumentLength(),0);
				
				text->InsertL(text->DocumentLength(),(*fontnames)[i]);
				text->InsertL(text->DocumentLength(),CPlainText::EParagraphDelimiter);
				
				cf.iFontSpec.iTypeface.iName=(*fontnames)[i];
				cfm.SetAttrib(EAttFontTypeface);
				text->ApplyCharFormatL(cf,cfm,text->DocumentLength(),0);
				
				text->InsertL(text->DocumentLength(),(*fontnames)[i]);
				text->InsertL(text->DocumentLength(),CPlainText::EParagraphDelimiter);
			}
	}

// 		TCharFormatMask charFormatMask;
// 		_LIT(KFontArial,"Arial");
// 		TFontSpec MyeFontSpec (KFontArial, 8*10);
// 		MyeFontSpec.iTypeface.SetIsProportional(ETrue);
// 		TCharFormat charFormat(MyeFontSpec.iTypeface.iName, MyeFontSpec.iHeight);
// 		charFormat.iFontPresentation.iTextColor=KRgbRed;
// 		charFormatMask.SetAttrib(EAttColor );
// 		CCharFormatLayer* iCharform = CEikonEnv::NewDefaultCharFormatLayerL();
// 		iCharform->Sense( charFormat, charFormatMask );
// 		iCharform->SetL( charFormat, charFormatMask );
// 		iEditor->SetCharFormatLayer(iCharform);	
// 	}
 // //    TFontSpec fontSpec(_L("Roman"),2);   
// //    TCharFormat charFormat(fontSpec.iTypeface.iName, fontSpec.iHeight);
// //    richtext->ApplyCharFormatL(charFormat, charFormatMask, 0, 5);

//    {
// 	   const CFont* font = CEikonEnv::Static()->NormalFont();
// 	   TFontSpec fontspec = font->FontSpecInTwips();
	   
// 	   TCharFormat cformat( fontspec.iTypeface.iName, fontspec.iHeight );
// 	   TCharFormatMask cmask;
// 	   cmask.SetAttrib(EAttFontTypeface); 
// 	   cmask.SetAttrib(EAttFontHeight);
// 	   SetSelectionL(0, 5);
// 	   iEditor->ApplyCharFormatL(cformat, cmask);
//    }

   
//    {
// 	   TInt len = richtext->DocumentLength();
// 	   SetSelectionL(len - 32, len);
	   
// // 	   const CFont* font = CEikonEnv::Static()->NormalFont();
// // 	   TFontSpec fontspec = font->FontSpecInTwips();
// // 	   fontspec.iFontStyle.SetBitmapType(EAntiAliasedGlyphBitmap);

// // 	   CFont* myFont = NULL;

// // 	   User::LeaveIfError( iEikonEnv->ScreenDevice()->GetNearestFontInPixels(myFont,fontspec) );
// 	   const CFont* font = AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont); 

// 	   TFontSpec myFontSpec = font->FontSpecInTwips();	   
// 	   TCharFormat cformat( myFontSpec.iTypeface.iName, myFontSpec.iHeight );

// 	   cformat.iFontPresentation.iTextColor = KRgbBlack;
// 	   TCharFormatMask cmask;
// 	   cmask.SetAll();
// 	   iEditor->ApplyCharFormatL(cformat, cmask);
//    }


//    SetSelectionL(0,5);
//    SetColorL(KRgbBlue);
	
	iEditor->ActivateL();
    SetRect(aRect);

	
    // Activate the window, which makes it ready to be drawn
    ActivateL();
    }




void CUiLoftAppView::InsertMyPictureL(TInt aPos)
	{
	CMyPicture* picture;
	// Create a CPicture derived class which will draw our image, depending this Size
	CGulIcon* icon = iIcons->At(iIcons->Count()-1);
	CFbsBitmap* bmp = icon->Bitmap();
	CFbsBitmap* mask = icon->Mask();
	picture = new( ELeave )CMyPicture(TSize(400, 400),*bmp, mask);
						
	CleanupStack::PushL(picture);
	// Prepare the Picture header, which will be instered into the Richtext
	TPictureHeader header;
	header.iPicture =TSwizzle<CPicture>(picture);
	iEditor->RichText()->InsertL( aPos,header); 
	CleanupStack::Pop(); // picture - Richtext take the ownership 
	}

void CUiLoftAppView::SetSelectionL(TInt aCursorPos,TInt aAnchorPos)
{
	iEditor->SetSelectionL(aCursorPos,aAnchorPos);
}

void CUiLoftAppView::SetColorL(TRgb aColor)
{
	TCharFormat charFormat;
	TCharFormatMask charFormatMask;
	
	charFormat.iFontPresentation.iTextColor = aColor;
	charFormatMask.SetAttrib(EAttColor);
	iEditor->ApplyCharFormatL(charFormat, charFormatMask);
	
//	iRtEd->SetBackgroundColorL(aColor); 
//  It is possible to change the background color - if foreground and background 
//  color is same, the text became invisible.
	}


void CUiLoftAppView::SetTypefaceL(const TDesC& aTypeFace)
{
	TCharFormat charFormat( aTypeFace, 3);
	TCharFormatMask charFormatMask;
	charFormatMask.SetAll();
	iEditor->ApplyCharFormatL(charFormat, charFormatMask);	
	}

void CUiLoftAppView::SizeChanged()
{


	TRect r=Rect();
	r.Move( 0, -r.iTl.iY );
	CAknsBasicBackgroundControlContext* prev=iBackground;
	TRAPD(err, iBackground=CAknsBasicBackgroundControlContext::NewL(KAknsIIDQsnBgAreaMain,
															 r, EFalse ));
	if (iBackground) delete prev;
	else iBackground=prev;
	iEditor->SetSkinBackgroundControlContextL(iBackground);
	//TBool skins = iEditor->SkinEnabled();
	TRect t=Rect();
	t.Move(10, 10);
	t.Resize(-20, -20);
	iEditor->SetRect(t);
}

// Draw this application's view to the screen
void CUiLoftAppView::Draw(const TRect& aRect) const
    {
    // Get the standard graphics context 
    CWindowGc& gc = SystemGc();
    
    // Gets the control's extent
    TRect rect = Rect();
    
    // Clears the screen
    //gc.Clear(rect);
	AknsDrawUtils::Background( AknsUtils::SkinInstance(), iBackground, gc, aRect );

    }


