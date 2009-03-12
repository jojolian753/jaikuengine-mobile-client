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

#include "ccu_trafficlight.h"

#include "cb_presence.h"
#include "ccu_presencestatus.h"
#include "ccu_staticicons.h"
//#include "ccu_streamrenderers.h"

#include <contextcontactsui.mbg>

#include "app_context.h"
#include "juik_iconmanager.h"
#include "juik_image.h"
#include "juik_sizer.h"
#include "juik_sizercontainer.h"
#include "juik_layoutitem.h"
#include "juik_layout.h"
#include "juik_gfxstore.h"
#include "jaiku_layoutids.hrh"

class CTrafficLightMgrImpl : public CTrafficLightMgr, public MContextBase, public MPresenceListener
{	
public:	
	~CTrafficLightMgrImpl()
	{
		CALLSTACKITEM_N(_CL("CTrafficLightMgrImpl"), _CL("~CTrafficLightMgrImpl"));
		if ( iOwnsImage )
			delete iTrafficLight;

		if ( iIcon ) 
			{
				iDelegates.iFeedGraphics->ReleaseIcon( iIcon );
				iIcon = NULL;				
			}
		iDelegates.iPresenceHolder->RemoveListener( this );
	}
	

	CTrafficLightMgrImpl(const TDesC& aNick, const TUiDelegates& aDelegates) : iNick(aNick), iDelegates( aDelegates ), iOwnsImage(ETrue), iCurrentStatus(EPresenceNone) {}
	
	CJuikImage* GetControl() 
	{ 
		iOwnsImage = EFalse; 
		return iTrafficLight; 
	}

	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CTrafficLightMgrImpl"), _CL("ConstructL"));
		TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls, LI_feed_controls__traffic_light);
		TSize sz = lay.Size();
		iTrafficLight = CJuikImage::NewL( NULL, sz);
// 		iTrafficLight->iMargin = Layout().GetLayoutItemL(LG_feed_controls_margins, 
// 														 LI_feed_controls_margins__author_header_buddy).Margins();
		
		TInt id = iDelegates.iJabberData->GetContactIdL( iNick );
		CBBPresence* p = iDelegates.iPresenceHolder->GetPresence( id );
		UpdateL( p );

		iDelegates.iPresenceHolder->AddListener( this );
	}

	TIconID2 PresenceIconName(TPresenceStatus aStatus)
	{
		switch ( aStatus )
			{
			case EPresenceNone:  
				{
				TIconID2 id = { KErrNotFound, KErrNotFound };
				return id;
				}
			case EPresenceGray:
				{
					TIconID2 id = {  EMbmContextcontactsuiLight_gray, EMbmContextcontactsuiLight_gray_mask  };
					return id;
				}
			case EPresenceRed: 
				{
					TIconID2 id = { EMbmContextcontactsuiLight_red, EMbmContextcontactsuiLight_red_mask  };
					return id;
				}
				
			case EPresenceYellow:
				{
					TIconID2 id = { EMbmContextcontactsuiLight_yellow, EMbmContextcontactsuiLight_yellow_mask };
					return id;
				}
				
				break;
				
			case EPresenceGreen: 
				{
					TIconID2 id = { EMbmContextcontactsuiLight_green, EMbmContextcontactsuiLight_green_mask };
					return id;
				}
				break;

			default: 
				{
					TIconID2 id = { KErrNotFound, KErrNotFound };
					return id;
				}
			}
	}

	void UpdateIconL( TIconID2 aIconId )
	{
		CALLSTACKITEM_N(_CL("CTrafficLightMgrImpl"), _CL("UpdateIconL"));

		if ( iIcon )
			{
				iDelegates.iFeedGraphics->ReleaseIcon( iIcon );
				iIcon = NULL;
			}
		TJuikLayoutItem lay = Layout().GetLayoutItemL(LG_feed_controls, LI_feed_controls__traffic_light);
		TSize sz = lay.Size();
		
		TComponentName name = { { CONTEXT_UID_CONTEXTCONTACTSUI }, 11000 + aIconId.iBitmap };		
		iIcon = iDelegates.iFeedGraphics->GetIcon( name, sz );
		if ( ! iIcon  )
			{
				iIcon = JuikIcons::LoadSingleIconL( StaticIcons::ContextContactsUiIconFile(), aIconId ); 
				JuikIcons::SetIconSizeL( *iIcon, sz, EAspectRatioPreserved );
				iIcon = iDelegates.iFeedGraphics->SetIconL( name, iIcon );
			}		
		iTrafficLight->UpdateL( *iIcon );
	}

		
	void UpdateL( CBBPresence* aP ) 
	{
		CALLSTACKITEM_N(_CL("CTrafficLightMgrImpl"), _CL("UpdateL"));
		if ( aP )
			{
				TPresenceStatus status = PresenceStatusL( aP, ETrue, EFalse );
				if ( status != iCurrentStatus )
					{
						iCurrentStatus = status;
						TIconID2 iconName = PresenceIconName( status );
						if (iconName.iBitmap != KErrNotFound )
								UpdateIconL( iconName );
						else
							iTrafficLight->ClearL();
					}
			}
		else
			{						
				iTrafficLight->ClearL();
			}
	}
	
	virtual void PresenceChangedL(TInt aContactId, CBBPresence* Info) 
	{
		CALLSTACKITEM_N(_CL("CTrafficLightMgrImpl"), _CL("PresenceChanged"));
		TInt id = iDelegates.iJabberData->GetContactIdL( iNick );
		if ( id == aContactId )
			{
				CBBPresence* p = iDelegates.iPresenceHolder->GetPresence( id );
				UpdateL( p );
				//iTrafficLight->DrawDeferred();
			}
	}
	
	virtual void Notify(const TDesC & aMessage) 
	{
		
	}

	
private:
	TUiDelegates iDelegates;
	TBool iOwnsImage;
	CJuikImage* iTrafficLight;
	CJabberData::TNick iNick;

	TPresenceStatus iCurrentStatus;

	CGulIcon* iIcon;
};

EXPORT_C CTrafficLightMgr* CTrafficLightMgr::NewL(const TDesC& aNick, const TUiDelegates& aDelegates)
{
	CALLSTACKITEMSTATIC_N(_CL("CTrafficLightMgr"), _CL("NewL"));
	auto_ptr<CTrafficLightMgrImpl> self( new (ELeave) CTrafficLightMgrImpl(aNick, aDelegates) );
	self->ConstructL();
 	return self.release();
}
