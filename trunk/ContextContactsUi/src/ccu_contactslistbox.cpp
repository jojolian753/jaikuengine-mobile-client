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

#include "ccu_contactslistbox.h"

#include "ccu_contactdataproviders.h"
#include "ccu_userpics.h"
#include "phonebook.h"
#include "ccu_contact.h"
#include "ccu_namefilter.h"

#include "jaiku_layoutids.hrh"
#include "juik_subcellrenderer.h"
#include "juik_listbox.h"
#include "juik_iconmanager.h"
#include "juik_layout.h"
#include "symbian_auto_ptr.h"
#include "callstack.h"

#include "icons.h"
#include <contextcontactsui.mbg>

#include <AknIconArray.h>
#include <AknUtils.h>
#include <eikfrlbd.h>


class CContactListBoxArray : public CBase, public MContextBase, public MDesCArray
{
public:
	static CContactListBoxArray* NewL(phonebook_i& aPhonebook,
									  CArrayPtr<MContactDataProvider>& aProviders)
	{
		auto_ptr<CContactListBoxArray> self( new (ELeave) CContactListBoxArray(aPhonebook));
		self->ConstructL( aProviders );
		return self.release();
	}


	CContactListBoxArray( phonebook_i& aPhonebook ) : iPhonebook( aPhonebook ) {} 
		
	void ConstructL(CArrayPtr<MContactDataProvider>& aProviders)
	{
		CALLSTACKITEM_N(_CL("JaikuContactsArray"), _CL("ConstructL"));
		iBuf = HBufC::NewL( 1000 );
		
		iJaikuProviders = new (ELeave) CArrayPtrFlat<MContactDataProvider>(10);
		iNonJaikuProviders = new (ELeave) CArrayPtrFlat<MContactDataProvider>(10);

		const TInt KCount(8);
		// 2 first: non-jaiku: name, extra,
		// 4 next:  jaiku: name, presenceline, buddypic, status, 
		// 1 last: marker
		TBool jaikuCells[KCount] =    { 0, 0, 1, 1, 1, 1, 1, 1 };
		TBool nonJaikuCells[KCount] = { 1, 1, 0, 0, 0, 0, 0, 1 };
		for (TInt i = 0; i < KCount; i++ )
			{
				iJaikuProviders->AppendL( jaikuCells[i] ? aProviders[i] : NULL );
				iNonJaikuProviders->AppendL( nonJaikuCells[i] ? aProviders[i] : NULL );		
			}
	}
	
	
	~CContactListBoxArray()
	{
		delete iJaikuProviders;
		delete iNonJaikuProviders;
		delete iBuf;
	}
	
private: // from MDesCArray
	TPtrC MdcaPoint(TInt aIndex) const;
	TInt MdcaCount() const;
	CArrayPtr<MContactDataProvider>& GetProvidersL(contact* c) const;

private:
	/**
	 * Internal buffer to format contact text
	 * own. 
	 */ 
	mutable HBufC* iBuf;
	phonebook_i& iPhonebook;
	CArrayPtr<MContactDataProvider>* iNonJaikuProviders;
	CArrayPtr<MContactDataProvider>* iJaikuProviders;
};


TInt CContactListBoxArray::MdcaCount() const
{
	CALLSTACKITEM_N(_CL("CJaikuContactsArray"), _CL("MdcaCount"));
	return iPhonebook.Count();
}

_LIT( KTab, "\t" );

// wrapper function
void GetDataL(MContactDataProvider* aP, contact* aC,  HBufC* aBuf)
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


CArrayPtr<MContactDataProvider>& CContactListBoxArray::GetProvidersL(contact* c) const
{
	if ( c && c->has_nick ) return *iJaikuProviders;
	else                    return *iNonJaikuProviders;
}


TPtrC CContactListBoxArray::MdcaPoint(TInt aIndex) const
{
	iBuf->Des().Zero();
	// Get parameters 
	contact* c = iPhonebook.GetContact( aIndex );	

	CArrayPtr<MContactDataProvider>& providers = GetProvidersL(c);
	
	GetDataL( providers.At(0), c, iBuf );
	
	for (TInt i=1; i < providers.Count(); i++)
		{						
			iBuf->Des().Append(KTab);
			GetDataL( providers.At(i), c, iBuf );
		}
	return *iBuf;
}



EXPORT_C CJaikuContactsListController* CJaikuContactsListController::NewL(CCoeControl* aParent,
																		  TContactUiDelegates& aDelegates) 
{
	auto_ptr<CJaikuContactsListController> self( new (ELeave) CJaikuContactsListController(aDelegates) );
	self->ConstructL(aParent);
	return self.release();
}


CJaikuContactsListController::CJaikuContactsListController(TContactUiDelegates& aDelegates)  
	: iDelegates( aDelegates )
{	
}

void CJaikuContactsListController::ConstructL(CCoeControl* aParent)
{
	// ListBox 
	MJuikIconManager& mgr = IconManager();

	CJuikGenListBox::TItemLayoutIds listItemLayout = { LG_contacts_list, LI_contacts_list__item_full, LI_contacts_list__item_content_only, ETrue };
	iListBox = CJuikGenListBox::NewL( aParent, listItemLayout, mgr );

	// Data providers 

	iAllProviders = new (ELeave) CArrayPtrFlat<MContactDataProvider>(10);
	TBool lastNameFirst = Phonebook().ShowLastNameFirstL();
	MContactDataProvider *name1 = ContactDataProviders::NameL(lastNameFirst);
	iAllProviders->AppendL( name1 );
	MContactDataProvider *extraname = ContactDataProviders::ExtraNameL();
	iAllProviders->AppendL( extraname );
	MContactDataProvider *name2 = ContactDataProviders::NameL(lastNameFirst);
	iAllProviders->AppendL( name2 );
	MContactDataProvider *presenceline = ContactDataProviders::PresenceAndLocationL(*(iDelegates.iPeriodFormatter));
	iAllProviders->AppendL( presenceline );
	MContactDataProvider *buddy = ContactDataProviders::BuddyIconL(mgr, UserPics());
	iAllProviders->AppendL( buddy );
	MContactDataProvider *status = ContactDataProviders::StatusIconL(mgr);
	iAllProviders->AppendL( status );
	MContactDataProvider *unread = ContactDataProviders::UnreadStreamIconL(mgr, iDelegates);
	iAllProviders->AppendL( unread );
	MContactDataProvider *marker = ContactDataProviders::EmptyL();
	iAllProviders->AppendL( marker );

	
	// Renderers 
	
	auto_ptr< CArrayPtr<MSubcellRenderer> > renderers( new (ELeave) CArrayPtrFlat<MSubcellRenderer>(5) );
	
	CArrayFix<TJuikLayoutItem>* ls = Layout().GetLayoutGroupL( LG_contacts_list_item_content );
	for( TInt i=0; i < ls->Count(); i++)
		{
			TJuikLayoutId id = { LG_contacts_list_item_content, i };
			auto_ptr<CSubcellRenderer> r( new (ELeave) CSubcellRenderer( id ));
			r->AddProviderL( *(iAllProviders->At(i)) );
			renderers->AppendL( r.release() );
		}
	TInt lastColumn = renderers->Count() - 1;
	
	iListBox->SetRenderersL( renderers.release() );

	iListBox->EnableMarkerL( lastColumn );		

	MDesCArray* array = CContactListBoxArray::NewL(Phonebook(), *iAllProviders); //CJaikuContactsListBox::CreateLBArrayL(aPhonebook, iUserPics );
	iListBox->Model()->SetItemTextArray( array );
	iListBox->Model()->SetOwnershipType( ELbmOwnsItemArray );
	
	iNameArray=CNameArray::NewL( iDelegates.iPhonebook );
	iListBox->JuikListBoxModel()->SetFilterArray( iNameArray );

	UserPics().AddObserverL( *this ); // we don't need notifications for initial set of icons
 	iDelegates.iFeedStorage->SubscribeL( this );
}


void CJaikuContactsListController::FeedItemEvent(CBBFeedItem* aItem, TEvent aEvent)
{
	// do nothing
}

void CJaikuContactsListController::AuthorCountEvent(const TDesC& aAuthor,
															TInt aNewItemCount, 
															TInt aNewUnreadCount)
{
	//iDelegates.iJabberData->
}


EXPORT_C CJuikGenListBox* CJaikuContactsListController::GetListBoxL()
{
	return iListBox;
}


CJaikuContactsListController::~CJaikuContactsListController()
{	
 	iDelegates.iFeedStorage->UnSubscribeL( this );
	UserPics().RemoveObserverL( *this );
	delete iListBox;
	delete iNameArray;
	if (iAllProviders) iAllProviders->ResetAndDestroy();
	delete iAllProviders;
}


void CJaikuContactsListController::UserPicChangedL(const TDesC& aNick, TBool aIsNew)
{
	iListBox->DrawDeferred();
}

