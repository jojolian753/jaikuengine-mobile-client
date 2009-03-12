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

#include "ccu_utils.h"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"

#include <akncontext.h>
#include <akntitle.h>
#include <avkon.hrh>
#include <eikenv.h>
#include <eikspane.h> 
#include <APGWGNAM.H>

#include <eikappui.h>
#include <eikapp.h>
#ifdef __S60V3__
#include <aknsutils.h>
#endif

namespace StatusPaneUtils
{	
	EXPORT_C void SetTitlePaneTextL(const TDesC& aText)
	{	
		CALLSTACKITEMSTATIC_N(_CL(""), _CL("SetTitlePaneTextL"));
		MTitlePaneReplacement* tp=GetTitlePaneReplacement();
		if (tp) 
			{
				tp->SetTitleTextL( aText );
			}
		else // set through built-in title pane
			{
				CEikStatusPane* sp = CEikonEnv::Static()->AppUiFactory()->StatusPane();
				CAknTitlePane* titlePane = ( CAknTitlePane*  )sp->ControlL( 
																		   TUid::Uid( EEikStatusPaneUidTitle ) );
				
				titlePane->SetTextL( aText );
			}
	}
	EXPORT_C void SetTitlePaneTextToDefaultL()
	{
		CALLSTACKITEMSTATIC_N(_CL(""), _CL("SetTitlePaneTextToDefaultL"));
		
		CApaWindowGroupName* gn;
		gn=CApaWindowGroupName::NewLC(CEikonEnv::Static()->WsSession(), 
			CEikonEnv::Static()->RootWin().Identifier());

		SetTitlePaneTextL(gn->Caption());
		
		CleanupStack::PopAndDestroy();
	}
	

	EXPORT_C void SetContextPaneIconToDefaultL()
	{
		CALLSTACKITEMSTATIC_N(_CL(""), _CL("SetContextPaneIconToDefaultL"));
		CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
		CAknContextPane* contextPane = (CAknContextPane *)statusPane->ControlL(TUid::Uid(EEikStatusPaneUidContext));
		
#ifdef __S60V3__
		CFbsBitmap *bmp=0, *mask=0;
		CEikonEnv* env=CEikonEnv::Static();
		CEikAppUi* appui=env->EikAppUi();
		TUid appuid=appui->Application()->AppDllUid();
		AknsUtils::CreateAppIconLC( AknsUtils::SkinInstance(), 
			appuid, EAknsAppIconTypeContext, bmp, mask );
		CleanupStack::Pop(2);
		SetContextPaneIconL(bmp, mask);
#else
		contextPane->SetPictureToDefaultL();
#endif
	}
	
	
	EXPORT_C void SetContextPaneIconL(CFbsBitmap* aBitmap, CFbsBitmap* aMask)
	{	
		CALLSTACKITEMSTATIC_N(_CL(""), _CL("SetContextPaneIconL"));	
		auto_ptr<CFbsBitmap> bmp( aBitmap );
		auto_ptr<CFbsBitmap> mask( aMask );
		
		CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
		CAknContextPane* contextPane = (CAknContextPane *)statusPane->ControlL(TUid::Uid(EEikStatusPaneUidContext));	
		contextPane->SetPicture( bmp.release(), mask.release() );
	}
	
	EXPORT_C void SetTitlePaneReplacement(MTitlePaneReplacement* aReplacement)
	{
		User::LeaveIfError( Dll::SetTls(aReplacement) );
	}
	
	EXPORT_C MTitlePaneReplacement* GetTitlePaneReplacement()
	{
		return (MTitlePaneReplacement*)Dll::Tls();
	}
}
