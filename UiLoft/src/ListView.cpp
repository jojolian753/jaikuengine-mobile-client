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

#include "ListView.h"

#include "ccu_mainbgcontainer.h"
#include "ccu_themes.h"
#include "juik_sizercontainer.h"
#include "juik_label.h"
#include "juik_multilabel.h"
#include "juik_urlmultilabel.h"
#include "juik_fonts.h"
#include "ccu_jpegcontrol.h"

#include <uiloft.rsg> // FIXME: change 

#include "app_context.h"
#include "app_context_impl.h"
#include "break.h"
#include "reporting.h"
#include "symbian_auto_ptr.h"


#include <akndef.h>
#include <aknviewappui.h>

#include <eiklabel.h> 

class CListViewImpl :
	public CListView, 
	public MContextBase,
	public CJpegControl::MListener
{
public:
	CListViewImpl() //const TVwsViewId& aParentViewId) 
	{
		//iParentViewId = aParentViewId;
	}
	
	
	~CListViewImpl() 
	{
		CC_TRAPD(err, ReleaseCListViewImpl());
		if (err!=KErrNone) User::Panic(_L("UNEXPECTED_LEAVE"), err);
	}

	//TInt iResource;
	
	void ReleaseCListViewImpl()
	{
		CALLSTACKITEM_N(_CL("CListViewImpl"), _CL("ReleaseCListViewImpl"));
		// if (iResource) iEikonEnv->DeleteResourceFile(iResource);
		iJpegs.Close();

		if ( iContainer )
			{
				AppUi()->RemoveFromStack( iContainer );
			}
		delete iContent;
		delete iContainer;
		delete iThemeColors;

		
	}


	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CListViewImpl"), _CL("ConstructL"));
		//iResource = LoadSystemResourceL( iEikonEnv, _L("contextcontactsui") );
		BaseConstructL( R_LIST_VIEW );
	}

	TUid Id() const {
		return KListView;
	}



	void DoActivateL(const TVwsViewId& aPrevViewId,TUid /*aCustomMessageId*/,const TDesC8& /*aCustomMessage*/)
	{
		MActiveErrorReporter* rep=GetContext()->GetActiveErrorReporter();
		if (rep) rep->SetInHandlableEvent(ETrue);
		CALLSTACKITEM_N(_CL("CListViewImpl"), _CL("DoActivateL"));
		CreateContentL();
	}
		
	void DoDeactivate()
	{
		CC_TRAPD(err, RealDoDeactivateL());
	}
	
	void RealDoDeactivateL()
	{
// 		iActiveContact.RemoveListenerL( *this );
// 		//StatusPaneUtils::SetContextPaneIconToDefaultL();
		if ( iContainer )
			{
				AppUi()->RemoveFromStack( iContainer );
			}
		delete iContent;
		iContent = NULL;

		delete iContainer;
		iContainer = NULL;
	}


	void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane)
	{
		CALLSTACKITEM_N(_CL("CListViewImpl"), _CL("DynInitMenuPaneL"));
// 		SetItemDimmedIfExists(aMenuPane, EcontextContactsCmdAppResumePresence, enabled);
	}


	void HandleCommandL(TInt aCommand)
	{
		CALLSTACKITEM_N(_CL("CListViewImpl"), _CL("HandleCommandl"));
		switch ( aCommand )
			{
// 			case EAknSoftkeyBack:
// 				ActivateViewL(iParentViewId);
// 				break;
			default:
				AppUi()->HandleCommandL( aCommand );
				break;
			}
	}

	TInt iRunningId; 
	void AddLabelL(const TDesC& aText)
	{
		auto_ptr<CJuikLabel> label( CJuikLabel::NewL( KRgbBlack, KRgbWhite, 
													  JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall ), 
													  *iContent ) );
		label->SetDebugId( iRunningId );		
		iContent->AddControlL( label.get(), iRunningId++ );
		CJuikLabel* labelPtr = label.release();
		labelPtr->UpdateTextL( aText );
		iContent->GetRootSizerL()->AddL( *labelPtr, 0, Juik::EExpandNot ); 
	}
	

	
	CJpegControl* AddJpegL(TSize aSize)
	{
		auto_ptr<CJpegControl> jpeg( CJpegControl::NewL( iContent, aSize ) );
		
		iContent->AddControlL( jpeg.get(), iRunningId++ );
		CJpegControl* jpegPtr = jpeg.release();
		jpegPtr->AddListenerL( *this );
		jpegPtr->SetFileL( _L("c:\\foo.jpg") );
		iContent->GetRootSizerL()->AddL( *jpegPtr, 0, Juik::EExpandNot ); 
		return jpegPtr;
	}
	
	RPointerArray<CJpegControl> iJpegs;

	void MediaLoaded(TBool /*aaha*/ )
	{
		for (TInt i=0; i < iJpegs.Count(); i++ )
			{
				iJpegs[i]->DrawDeferred();
			}
	}

	void AddMultiLabelL(const TDesC& aText)
	{
		auto_ptr<CMultiLabel> label( CMultiLabel::NewL( NULL, KRgbBlack, KRgbWhite) );
		// 														JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall ), 
		// 														*iContent ) 
		
		label->SetDebugId( iRunningId );		
		iContent->AddControlL( label.get(), iRunningId++ );
		CMultiLabel* labelPtr = label.release();
		labelPtr->UpdateTextL( aText );
		iContent->GetRootSizerL()->AddL( *labelPtr, 0, Juik::EExpandNot ); 
	}
	

	void AddLabelInContainerL(const TDesC& aText, Juik::TBoxOrientation aOrient = Juik::EVertical)
	{
		auto_ptr<CJuikSizerContainer> container( CJuikSizerContainer::NewL() );
		container->SetDebugId( iRunningId );
		iContent->AddControlL( container.get(), iRunningId++ );
		CJuikSizerContainer* c = container.release();
		iContent->GetRootSizerL()->AddL( *c, 0, Juik::EExpandNot ); 

		MJuikSizer* sizer = Juik::CreateBoxSizerL( aOrient );
		c->SetRootSizerL( sizer );
		
		auto_ptr<CJuikLabel> label( CJuikLabel::NewL( KRgbBlack, KRgbWhite, 
													  JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall ), 
													  *c ) );
		label->SetDebugId( iRunningId );		
		c->AddControlL( label.get(), iRunningId++ );
		CJuikLabel* labelPtr = label.release();
		labelPtr->UpdateTextL( aText );
		
		TInt p = aOrient == Juik::EVertical ? 0 : 1;		
		c->GetRootSizerL()->AddL( *labelPtr, p, Juik::EExpand ); 
	}


	void AddLabelInContainerHierarchyL(const TDesC& aText, Juik::TBoxOrientation aOrient = Juik::EVertical)
	{

		auto_ptr<CJuikSizerContainer> container( CJuikSizerContainer::NewL() );
		container->SetDebugId( iRunningId );
		iContent->AddControlL( container.get(), iRunningId++ );
		CJuikSizerContainer* c = container.release();
		iContent->GetRootSizerL()->AddL( *c, 0, Juik::EExpand ); 

		Juik::TBoxOrientation outerOrient = aOrient;
		MJuikSizer* sizer1 = Juik::CreateBoxSizerL( aOrient );
		c->SetRootSizerL( sizer1 );

		Juik::TBoxOrientation innerOrient = 
			aOrient == Juik::EVertical ? 
			Juik::EHorizontal :
			Juik::EVertical;
		MJuikSizer* sizer2 = Juik::CreateBoxSizerL( innerOrient );
		c->AddSizerL( sizer2, iRunningId++);

		TInt p = outerOrient == Juik::EVertical ? 0 : 1;
		sizer1->AddL( *sizer2, p, Juik::EExpand );
	
		auto_ptr<CJuikLabel> label( CJuikLabel::NewL( KRgbBlack, KRgbWhite, 
													  JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall ), 
													  *c ) );
		label->SetDebugId( iRunningId );		
		c->AddControlL( label.get(), iRunningId++ );
		CJuikLabel* labelPtr = label.release();
		labelPtr->UpdateTextL( aText );

		p = innerOrient == Juik::EVertical ? 0 : 1;		
		sizer2->AddL( *labelPtr, p, Juik::EExpand ); 
	}


	void AddLabelsInContainerHierarchyL(const TDesC& aText, Juik::TBoxOrientation aOrient = Juik::EVertical)
	{

		auto_ptr<CJuikSizerContainer> container( CJuikSizerContainer::NewL() );
		container->SetDebugId( iRunningId );
		iContent->AddControlL( container.get(), iRunningId++ );
		CJuikSizerContainer* c = container.release();
		iContent->GetRootSizerL()->AddL( *c, 0, Juik::EExpand ); 
		

		Juik::TBoxOrientation outerOrient = aOrient;
		MJuikSizer* sizer1 = Juik::CreateBoxSizerL( aOrient );
		c->SetRootSizerL( sizer1 );

		{
			auto_ptr<CJuikLabel> label( CJuikLabel::NewL( KRgbBlack, KRgbWhite, 
														 JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall ), 
														 *c ) );
			label->SetDebugId( iRunningId );
			c->AddControlL( label.get(), iRunningId++ );
			CJuikLabel* labelPtr = label.release();
			labelPtr->UpdateTextL( _L("FIXED") );	  
			sizer1->AddL( *labelPtr, 0, Juik::EExpand ); 
		}



		Juik::TBoxOrientation innerOrient = 
			aOrient == Juik::EVertical ? 
			Juik::EHorizontal :
			Juik::EVertical;
		MJuikSizer* sizer2 = Juik::CreateBoxSizerL( innerOrient );
		c->AddSizerL( sizer2, iRunningId++);

		TInt p = outerOrient == Juik::EVertical ? 0 : 1;
		sizer1->AddL( *sizer2, p, Juik::EExpand );
	

		
		{
			auto_ptr<CJuikLabel> label( CJuikLabel::NewL( KRgbBlack, KRgbWhite, 
														  JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall ), 
														  *c ) );
			label->SetDebugId( iRunningId );
			c->AddControlL( label.get(), iRunningId++ );
			CJuikLabel* labelPtr = label.release();
			labelPtr->UpdateTextL( _L("FIXEDUP") );	  
			TInt p = innerOrient == Juik::EVertical ? 0 : 1;		
			sizer2->AddL( *labelPtr, 0, Juik::EExpand ); 
		}

		{
			auto_ptr<CJuikLabel> label( CJuikLabel::NewL( KRgbBlack, KRgbWhite, 
														  JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall ), 
														  *c ) );
			label->SetDebugId( iRunningId );		
			c->AddControlL( label.get(), iRunningId++ );
			CJuikLabel* labelPtr = label.release();
			labelPtr->UpdateTextL( aText );
			
			p = innerOrient == Juik::EVertical ? 0 : 1;		
			sizer2->AddL( *labelPtr, 0, Juik::EExpand ); 
		}
		
		{
			auto_ptr<CJuikLabel> label( CJuikLabel::NewL( KRgbBlack, KRgbWhite, 
														  JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall ), 
														  *c ) );
			label->SetDebugId( iRunningId );
			c->AddControlL( label.get(), iRunningId++ );
			CJuikLabel* labelPtr = label.release();
			labelPtr->UpdateTextL( _L("FIXEDDOWN") );	  
			TInt p = innerOrient == Juik::EVertical ? 0 : 1;		
			sizer2->AddL( *labelPtr, 0, Juik::EExpand ); 
		}

	}
	

	void AddUrlMultiLabelL(const TDesC& aText)
	{
		auto_ptr<CUrlMultiLabel> label( CUrlMultiLabel::NewL( NULL, KRgbBlack, KRgbWhite, ETrue, *iContent ) );
		label->SetDebugId( iRunningId );		
		iContent->AddControlL( label.get(), iRunningId++ );
		CUrlMultiLabel* labelPtr = label.release();
		labelPtr->UpdateTextL( aText );
		iContent->GetRootSizerL()->AddL( *labelPtr, 0, Juik::EExpandNot ); 
	}


	void AddUrlInContainerL(const TDesC& aText, Juik::TBoxOrientation aOrient = Juik::EVertical)
	{
		auto_ptr<CJuikSizerContainer> container( CJuikSizerContainer::NewL() );
		container->SetDebugId( iRunningId );
		iContent->AddControlL( container.get(), iRunningId++ );
		CJuikSizerContainer* c = container.release();
		iContent->GetRootSizerL()->AddL( *c, 0, Juik::EExpandNot ); 

		MJuikSizer* sizer = Juik::CreateBoxSizerL( aOrient );
		c->SetRootSizerL( sizer );
		
		auto_ptr<CUrlMultiLabel> label( CUrlMultiLabel::NewL( NULL, KRgbBlack, KRgbWhite, ETrue, *c ) );
		label->SetDebugId( iRunningId );		
		c->AddControlL( label.get(), iRunningId++ );
		CUrlMultiLabel* labelPtr = label.release();
		labelPtr->UpdateTextL( aText );
		
		TInt p = aOrient == Juik::EVertical ? 0 : 1;
		c->GetRootSizerL()->AddL( *labelPtr, p, Juik::EExpandNot ); 
	}
	


	void AddMultiInContainerL(const TDesC& aText, Juik::TBoxOrientation aOrient = Juik::EVertical)
	{
		auto_ptr<CJuikSizerContainer> container( CJuikSizerContainer::NewL() );
		container->SetDebugId( iRunningId );
		iContent->AddControlL( container.get(), iRunningId++ );
		CJuikSizerContainer* c = container.release();
		iContent->GetRootSizerL()->AddL( *c, 0, Juik::EExpandNot ); 

		MJuikSizer* sizer = Juik::CreateBoxSizerL( aOrient );
		c->SetRootSizerL( sizer );
		
		auto_ptr<CMultiLabel> label( CMultiLabel::NewL( NULL, KRgbBlack, KRgbWhite) );
		label->SetDebugId( iRunningId );		
		c->AddControlL( label.get(), iRunningId++ );
		CMultiLabel* labelPtr = label.release();
		labelPtr->UpdateTextL( aText );
		
		c->GetRootSizerL()->AddL( *labelPtr, 1, Juik::EExpandNot ); 
	}
	
	

	void CreateContentL()
	{
		CALLSTACKITEM_N(_CL("CListViewImpl"), _CL("CreateContentL"));
		if ( iContainer )
			{
				AppUi()->RemoveFromStack( iContainer );
				delete iContainer;
				iContainer = NULL;
			}
		iThemeColors = CThemeColors::NewL();
		TRect r = ClientRect();
		r.Shrink(30,30);
		iContainer = CMainBgContainer::NewL(this, r , *iThemeColors, NULL );
		iContent = CJuikSizerContainer::NewL();
		iContent->SetContainerWindowL( *iContainer );		
 		iContainer->SetContentL( iContent );

		{
			MJuikSizer* sizer = Juik::CreateFixedWidthSizerL();
			iContent->SetRootSizerL( sizer );
			TRect r = ClientRect();
			r.SetHeight(0);
			sizer->SetMinSize( r.Size() );
		}

// 		_LIT(KText0, "Hei!"); 
// 		AddLabelL( KText0 );

//  		_LIT(KText1, "This one is from a corporation instead of a team of hackers and will be released in a few weeks. It runs on a wide range of Java handsets and provides a robust Jaiku client (initially) for the basic operations such as posting and viewing the key Jaiku streams. Pretty exciting, I love to play with all these works of art!");
//  		AddLabelL( KText1 );

//  		_LIT(KText4, "Plain ulrmultilabel. http://jaiku.com. Yeah. Symbian Signed promotes best practice in designing applications to run on Symbian OS phones.");
//  		AddUrlMultiLabelL( KText4 );


//    		_LIT(KText15, "15 Labels in vertical-horizontal hierarchy. Symbian Signed promotes best practice.");
//  		AddLabelsInContainerHierarchyL( KText15, Juik::EVertical);

//    		_LIT(KText16, "16 Labels in horizontal-vertical hierarchy. Symbian Signed promotes best practice.");
//  		AddLabelsInContainerHierarchyL( KText16, Juik::EHorizontal);

		iJpegs.AppendL( AddJpegL(TSize(100,100) ) );
		iJpegs.AppendL( AddJpegL(TSize(50,20) ) );
		iJpegs.AppendL( AddJpegL(TSize(20,50) ) );

 //   		_LIT(KText9, "9Label in vertical-horizontal hierarchy. Symbian Signed promotes best practice in designing applications to run on Symbian OS phones. ");
//  		AddLabelInContainerHierarchyL( KText9, Juik::EVertical);

//    		_LIT(KText10, "10Label in horizontal-vertical hierarchy. Symbian Signed promotes best practice in designing applications to run on Symbian OS phones. ");
//  		AddLabelInContainerHierarchyL( KText10, Juik::EHorizontal);
		
//    		_LIT(KText2, "Urlmultilabel in vertical sizercontainer. www.google.fi. Yo!  Symbian Signed promotes best practice in designing applications");
//    		AddUrlInContainerL( KText2, Juik::EVertical );

//   		_LIT(KText6, "Urlmultilabel in horizontal sizercontainer. www.google.fi. Yo!  Symbian Signed promotes best practice in designing applications");
//   		AddUrlInContainerL( KText6, Juik::EHorizontal );

//    		_LIT(KText11, "MultiLabel in horizontal container. Symbian Signed promotes best practice in designing applications to run on Symbian OS phones.");
//  		AddMultiInContainerL( KText11, Juik::EHorizontal);

//    		_LIT(KText12, "MultiLabel in vertical container. Symbian Signed promotes best practice in designing applications to run on Symbian OS phones.");
//  		AddMultiInContainerL( KText12, Juik::EVertical);


//   		_LIT(KText7, "1Label in horizontal sizercontainer.  Symbian Signed promotes best practice in designing applications to run on Symbian OS phones.");
//   		AddLabelInContainerL( KText7, Juik::EHorizontal );

//   		_LIT(KText8, "2Label in vertical sizercontainer.  Symbian Signed promotes best practice in designing applications to run on Symbian OS phones.");
//   		AddLabelInContainerL( KText8, Juik::EVertical );
		

//  		_LIT(KText3, "dasdjas dkldjs alkdjasl dj klas jdkldjlskadjklasdj lasdklas dklas jdklaskld asjdklas dklasdj klas");
//  		AddLabelL( KText3 );

		
 		iContainer->SetRect( ClientRect() );
		iContainer->ActivateL();
// 		iContainer->SetRect( ClientRect() );

		AppUi()->AddToStackL( *this, iContainer );
	}


	
	void HandleResourceChange( TInt aType )
	{
		if ( aType == KEikDynamicLayoutVariantSwitch )
			{
				if ( iContainer )
					{
						TRect r = ClientRect();
						iContainer->SetRect( r );
					}
			}
	}
	


private:
	CMainBgContainer* iContainer;
	CJuikSizerContainer* iContent;
	CThemeColors* iThemeColors;


};



CListView* CListView::NewL() // const TVwsViewId& aParentViewId)
{
	auto_ptr<CListViewImpl> self(new (ELeave) CListViewImpl() ); //aParentViewId));
	self->ConstructL();
	return self.release();
}
