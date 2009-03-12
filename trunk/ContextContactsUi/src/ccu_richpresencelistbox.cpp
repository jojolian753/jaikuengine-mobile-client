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

#include "ccu_richpresencelistbox.h"

#include "ccu_contactdataproviders.h"
#include "ccu_staticicons.h"

#include "jaiku_layoutids.hrh"
#include "juik_subcellrenderer.h"
#include "juik_listbox.h"
#include "juik_iconmanager.h"
#include "juik_layout.h"
#include "juik_icons.h"
#include "symbian_auto_ptr.h"
#include "callstack.h"

#include <contextcontactsui.mbg>
#include <akniconarray.h>
#include <aknutils.h>

_LIT( KTab, "\t" );


class CRichPresenceListBoxArray : public CBase, public MDesCArray
{
public:
	static CRichPresenceListBoxArray* NewL(CActiveContact& aActiveContact, 
										   CArrayPtr< CArrayPtr<MContactDataProvider> >* aProviders)
	{
		auto_ptr<CRichPresenceListBoxArray> self( new (ELeave) CRichPresenceListBoxArray(aActiveContact, aProviders));
		self->ConstructL();
		return self.release();
	}
	
	~CRichPresenceListBoxArray()
	{
		if ( iProvidersPerRow )
			{
				iProvidersPerRow->ResetAndDestroy();
			}
		delete iProvidersPerRow;
		delete iBuf;
	}
	
	CRichPresenceListBoxArray(CActiveContact& aActiveContact, CArrayPtr< CArrayPtr<MContactDataProvider> >* aProviders)
		: iActiveContact( aActiveContact ), iProvidersPerRow(aProviders) {}
	

	void GetDataL(MContactDataProvider* aP, contact* aC,  HBufC* aBuf) const
	{
		TPtr ptr = aBuf->Des();
		TInt pos = ptr.Length();
		if ( aP ) 
			{
				aP->SubcellDataL(aC, ptr);
				
				// Replace tabs from part that was just inserted!
				if ( pos < aBuf->Des().Length() )
					{
						TPtr newPart = aBuf->Des().MidTPtr(pos);
						AknTextUtils::ReplaceCharacters(newPart, KAknReplaceListControlChars, TChar(' ') );
					}
			}
	}
	
	
	CArrayPtr<MContactDataProvider>& GetProvidersL(TInt aIndex) const
	{
		return * ( iProvidersPerRow->At(aIndex) );
	}
	

	TPtrC MdcaPoint(TInt aIndex) const
	{
		CArrayPtr< MContactDataProvider >& p = GetProvidersL( aIndex );
		contact* c = iActiveContact.GetL();

		iBuf->Des().Zero();
		
		GetDataL( p.At(0), c, iBuf );
		
		for (TInt i=1; i < p.Count(); i++)
			{						
				iBuf->Des().Append(KTab);
				GetDataL( p.At(i), c, iBuf );
			}
		return *iBuf;
	}
	
	TInt MdcaCount() const
	{
		return iProvidersPerRow->Count();
	}
	
	void ConstructL()
	{
		iBuf = HBufC::NewL( 1000 );
	}
private:
	CActiveContact& iActiveContact;
	CArrayPtr< CArrayPtr<MContactDataProvider> >* iProvidersPerRow;
	HBufC* iBuf;
};

	


class CRichPresenceListControllerImpl : public CRichPresenceListController, public MContextBase
{
 public:
	virtual ~CRichPresenceListControllerImpl()
	{
		delete iListBox;
		if (iAllProviders) iAllProviders->ResetAndDestroy();
		delete iAllProviders;
	}

	CJuikGenListBox* GetListBoxL()
	{
		return iListBox;
	}

	
public:
	CRichPresenceListControllerImpl(CActiveContact& aActiveContact) : iActiveContact(aActiveContact) {}


	struct TSubcellDefinition
	{
		TInt iLayoutGroup;
		TInt iLayoutId;
		MContactDataProvider* iProviders[5]; 
	};





	void ConstructL(CCoeControl* aParent)
	{
		// ListBox 
		CJuikGenListBox::TItemLayoutIds layoutIds = { LG_richpresence_list, 
													  LI_richpresence_list__item_full, 
													  LI_richpresence_list__item_content_only,
													  ETrue };
		MJuikIconManager& mgr = IconManager();
		iListBox = CJuikGenListBox::NewL( aParent, layoutIds, mgr );	

		// Providers

		iAllProviders = new (ELeave) CArrayPtrFlat<MContactDataProvider>(10);

		MContactDataProvider *presenceline = ContactDataProviders::PresenceLineL();
		iAllProviders->AppendL( presenceline );
		MContactDataProvider *plineTStamp = ContactDataProviders::PresenceLineTStampL();
		iAllProviders->AppendL( plineTStamp );
		MContactDataProvider *useractivity = ContactDataProviders::UserActivityL();
		iAllProviders->AppendL( useractivity );

		MContactDataProvider *location = ContactDataProviders::LocationL();
		iAllProviders->AppendL( location );
		MContactDataProvider *locationTStamp = ContactDataProviders::LocationTStampL();
		iAllProviders->AppendL( locationTStamp );

		MContactDataProvider *calTitle = ContactDataProviders::CalendarTitleL();
		iAllProviders->AppendL( calTitle );
		MContactDataProvider *calDateTime = ContactDataProviders::CalendarDateTimeL();
		iAllProviders->AppendL( calDateTime );

		MContactDataProvider *nearbyHeader = ContactDataProviders::NearbyHeaderL();
		iAllProviders->AppendL( nearbyHeader );
		MContactDataProvider *nearbyPeople = ContactDataProviders::NearbyPeopleL();
		iAllProviders->AppendL( nearbyPeople );

		MContactDataProvider *statusIcon = ContactDataProviders::StatusIconL(mgr);
		iAllProviders->AppendL( statusIcon );

		const TDesC& iconFile = StaticIcons::ContextContactsUiIconFile();
		MContactDataProvider *locationIcon = ContactDataProviders::StaticIconL( mgr, iconFile, EMbmContextcontactsuiEarth );
		iAllProviders->AppendL( locationIcon );
		
		MContactDataProvider *nearbyIcon = ContactDataProviders::StaticIconL( mgr, iconFile, EMbmContextcontactsuiFriends);
		iAllProviders->AppendL( nearbyIcon );

		MContactDataProvider *calendarIcon = ContactDataProviders::StaticIconL( mgr, iconFile, EMbmContextcontactsuiCalendar );
		iAllProviders->AppendL( calendarIcon );
		
		MContactDataProvider *presenceIcon = ContactDataProviders::StaticIconL( mgr, iconFile, EMbmContextcontactsuiDummybuddy );
		iAllProviders->AppendL( presenceIcon );

		MContactDataProvider *profilename = ContactDataProviders::ProfileNameL();
		iAllProviders->AppendL( profilename );
		

		MContactDataProvider *empty = ContactDataProviders::EmptyL();
		iAllProviders->AppendL( empty );

		// Renderers

		TInt group = LG_richpresence_list_item_content;

		const TInt KSubcellsCount(3);
		const TInt KRows(5);
		TSubcellDefinition subcells[KSubcellsCount] =
			{ 
 				{group, LI_richpresence_list_item_content__maintext, {presenceline, profilename, location, nearbyHeader, calTitle} },
				{group, LI_richpresence_list_item_content__secondarytext, {plineTStamp, useractivity, locationTStamp, nearbyPeople, calDateTime} },
				{group, LI_richpresence_list_item_content__icon, { presenceIcon, statusIcon, locationIcon, nearbyIcon, calendarIcon} },
			};
				
			 
		auto_ptr< CArrayPtr< MSubcellRenderer > > subrs( new (ELeave) CArrayPtrFlat<MSubcellRenderer>(5) );
		auto_ptr< CArrayPtr< CArrayPtr<MContactDataProvider> > > providersPerRow( new (ELeave) CArrayPtrFlat< CArrayPtr<MContactDataProvider> >(5));
		
		for (TInt i=0; i < KRows; i++) 
			{
				providersPerRow->AppendL( new (ELeave) CArrayPtrFlat<MContactDataProvider>(5) );
			}

		for (TInt c=0; c < KSubcellsCount; c++)
			{
				TSubcellDefinition& s = subcells[c];
				TJuikLayoutId id = { s.iLayoutGroup, s.iLayoutId };
				

				auto_ptr< CArrayPtr<MContactDataProvider> > providers(new (ELeave) CArrayPtrFlat<MContactDataProvider>(10));
				auto_ptr<CSubcellRenderer> renderer( new (ELeave) CSubcellRenderer(id) );
				
				for(TInt r=0; r < KRows; r++)
					{
						renderer->AddProviderL( *(s.iProviders[r]) );
						providersPerRow->At(r)->AppendL( s.iProviders[r] );
					}
				
				subrs->AppendL( renderer.get() );
				renderer.release();
			}
		
		

		MDesCArray* array = CRichPresenceListBoxArray::NewL( iActiveContact, providersPerRow.release() ); 
		iListBox->Model()->SetItemTextArray( array );
		iListBox->Model()->SetOwnershipType( ELbmOwnsItemArray );

		iListBox->SetRenderersL( subrs.release() );		
	}
	

 private:
	CJuikGenListBox* iListBox; // own
	CArrayPtr<MContactDataProvider>* iAllProviders;
	CActiveContact& iActiveContact;
	TInt iProviderId;
};





EXPORT_C CRichPresenceListController* CRichPresenceListController::NewL(CCoeControl* aParent,
															   CActiveContact& aActiveContact)
{
	auto_ptr<CRichPresenceListControllerImpl> self( new (ELeave) CRichPresenceListControllerImpl(aActiveContact) );
	self->ConstructL(aParent);
	return self.release();
}

 



