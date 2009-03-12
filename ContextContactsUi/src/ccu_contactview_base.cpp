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

#include "ccu_contactview_base.h"


#include "ccu_activestate.h"
#include "ccu_contact.h"
#include "ccu_utils.h"
#include "ccu_userpics.h"
#include "ccu_jaikuview_base.h"
#include "phonebook.h"
#include <contextcontactsui.mbg>
#include "juik_icons.h"

#include <akncontext.h> 
#include <akniconarray.h>
#include <akniconutils.h>
#include <avkon.hrh>
#include <eikenv.h>
#include <eikspane.h>
#include <FBS.H>

EXPORT_C MContactViewBase::MContactViewBase(CJaikuViewBase& aJaikuView) : 
	iJaikuView( aJaikuView ) {}


CActiveContact& MContactViewBase::ActiveContact()
{
	return iJaikuView.ActiveState().ActiveContact();
}

EXPORT_C void MContactViewBase::SetBaseContainer(CCoeControl* aBaseContainer)
{
	iBaseContainer = aBaseContainer;
}


EXPORT_C void MContactViewBase::SetTitleL()
{
	TBuf<100> titleText;
	contact* c = ActiveContact().GetL();
	if (c) c->AppendName( titleText, iJaikuView.Phonebook().ShowLastNameFirstL() );
	StatusPaneUtils::SetTitlePaneTextL( titleText );
}		


EXPORT_C void MContactViewBase::SetContextIconL( )
{
	CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
    CAknContextPane* contextPane = (CAknContextPane *)statusPane->ControlL(TUid::Uid(EEikStatusPaneUidContext));

	CGulIcon* userIcon = NULL;

	contact* c = ActiveContact().GetL();
	if (c) userIcon = iJaikuView.UserPics().GetIconL( c->id );
	
	if ( userIcon )
		{
			auto_ptr<CFbsBitmap> bmp( new (ELeave) CFbsBitmap);
			bmp->Duplicate( userIcon->Bitmap()->Handle() );

			auto_ptr<CFbsBitmap> mask(NULL);
			if (userIcon->Mask())
				{
					mask.reset( new (ELeave) CFbsBitmap );
					mask->Duplicate( userIcon->Mask()->Handle() );
				}
			StatusPaneUtils::SetContextPaneIconL(bmp.release(), mask.release());
		}
	else
		{
			static const TInt KBuddyProviderIconCount(1);
			static const TIconID KBuddyProviderIconIds[1]= {
				_INIT_T_ICON_ID("C:\\system\\data\\contextcontactsui.mbm", 
								EMbmContextcontactsuiDummybuddy,
								EMbmContextcontactsuiDummybuddy_mask )
			};
			auto_ptr<CAknIconArray> icons( new (ELeave) CAknIconArray(1) );
			JuikIcons::LoadIconsL( icons.get(), KBuddyProviderIconIds, KBuddyProviderIconCount );
			
			if (icons->Count() == 0)
				return;
			
			CGulIcon* icon = icons->At(0);
			TSize iconSize;
			AknIconUtils::SetSize(icon->Bitmap(), iconSize, EAspectRatioPreserved);
			
			//this does not work
			icon->SetBitmapsOwnedExternally(ETrue);
			StatusPaneUtils::SetContextPaneIconL(icon->Bitmap(), icon->Mask());
		}
}

EXPORT_C void MContactViewBase::UpdateStatusPaneL()
{
			SetTitleL();
			SetContextIconL();
}

#include <aknappui.h>

EXPORT_C void MContactViewBase::ActiveContactChanged( MActiveContactListener::TChangeType aChangeType )
{
	switch ( aChangeType )
		{
		case ECleared:
			{
				// Close view
				CAknAppUi* aui=(CAknAppUi*)CEikonEnv::Static()->AppUi();
				iJaikuView.ActivateParentViewL();
			}
			break;
		case EChanged:
			UpdateStatusPaneL();
			iBaseContainer->DrawDeferred();
			break;
		case EDataUpdated:
			iBaseContainer->DrawDeferred();
			break;
		case EUserPicChanged:
			SetContextIconL();
			break;
		}
}
