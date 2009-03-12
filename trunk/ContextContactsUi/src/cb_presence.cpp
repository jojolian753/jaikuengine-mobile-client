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

#include "break.h"
#include "cb_presence.h"
#include "symbian_auto_ptr.h"
#include "contextCommon.h"
#include "bbxml.h"
#include "cbbsession.h"
#include "ccu_contactmatcher.h"
#include "stringmap.h"

const TInt CPresenceHolder::KDefaultBufferSize = 256;


EXPORT_C CPresenceHolder* CPresenceHolder::NewL(CJabberData& JabberData)
{
	CALLSTACKITEMSTATIC_N(_CL("CPresenceHolder"), _CL("NewL"));


	auto_ptr<CPresenceHolder> ret(new (ELeave) CPresenceHolder(JabberData));
	ret->ConstructL();
	return ret.release();
}

EXPORT_C void CPresenceHolder::SetMatcher(class CContactMatcher* aMatcher)
{
	iMatcher=aMatcher;
}

EXPORT_C CPresenceHolder::~CPresenceHolder()
{
	CALLSTACKITEM_N(_CL("CPresenceHolder"), _CL("~CPresenceHolder"));

	iJabberData.RemoveObserver(this);

	delete iBBSession;
	iListeners.Close();
	delete iContactsPresence;
}

EXPORT_C void CPresenceHolder::AddListener(MPresenceListener* Listener)
{
	CALLSTACKITEM_N(_CL("CPresenceHolder"), _CL("AddListener"));


	iListeners.AppendL(Listener);
}


EXPORT_C void CPresenceHolder::RemoveListener(MPresenceListener* aListener)
{
	CALLSTACKITEM_N(_CL("CPresenceHolder"), _CL("RemoveListener"));
	TInt ix = ix = iListeners.Find( aListener );
	if ( ix > 0 )
		iListeners.Remove(ix);
}

EXPORT_C const CBBPresence* const CPresenceHolder::GetPresence(TInt ContactId) const
{
	CALLSTACKITEM_N(_CL("CPresenceHolder"), _CL("GetPresence"));

	return (CBBPresence*) iContactsPresence->GetData(ContactId);
}

CPresenceHolder::CPresenceHolder(CJabberData& JabberData) : 
	iJabberData(JabberData) { }

void CPresenceHolder::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPresenceHolder"), _CL("ConstructL"));

	iContactsPresence=CGenericIntMap::NewL();
	iContactsPresence->SetDeletor(CBBPresenceDeletor);

	iSendTimeStamp = TTime();

	iBBSession=BBSession()->CreateSubSessionL(this);
	iBBSession->AddNotificationL(KIncomingPresence, ETrue);

	iJabberData.AddObserver(this);

}


void CPresenceHolder::NickChanged(const TDesC& aPrevious, const TDesC& aNew)
{
	// FIXME: I think we don't need this method anymore, because IdChanged method 
	// should handle this case just fine. But before removing, think through.
	// This is called "manually" after nick is changed from NickForm in ContextContacts 
	TBuf<CJabberData::KNickMaxLen> newNick = aNew;

	if (CJabberData::EqualNicksL(aPrevious, aNew)) return;

	if (aPrevious.Length()>0 && aNew.Length()==0) {
		DeletedL(KIncomingPresence, aPrevious);
		
		// If user nick was deleted from contact, we update
		// presence data for user nick so that it gets
		// mapped to dummy Me-item. 
		if ( iJabberData.IsUserNickL( aPrevious ) )
			{
				newNick = aPrevious;
			}
		else
			{
				return;
			}
	}

	
	if (newNick.Locate('@')==KErrNotFound) newNick.Append(_L("@jaiku.com"));
	
	MBBData* val=0;
	iBBSession->GetL(KIncomingPresence, newNick, val, ETrue);
	const CBBPresence* retrieved=bb_cast<CBBPresence>(val);
	if (! retrieved ) {
		delete val;
		DeletedL(KIncomingPresence, newNick);
		return;
	}
	bb_auto_ptr<MBBData> p(val);
	NewValueL(0, KIncomingPresence, newNick, KNoComponent, val);
}


void CPresenceHolder::NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
	const TComponentName& aComponentName, const MBBData* aData)
{
	const CBBPresence* retrieved=bb_cast<CBBPresence>(aData);
	if (!retrieved) return;

	TPtrC nick( aSubName );

	if (nick.Length() == 0) return;

	TInt contactId = GetContactForNickL( nick, *retrieved );
	if ( contactId != KNullContactId )
		{
			refcounted_ptr<CBBPresence> data(bb_cast<CBBPresence>(retrieved->CloneL(KNullDesC)));
			if (!data.get()) User::Leave(KErrGeneral);
			data->iSent=ETrue;
			
			iContactsPresence->AddDataL(contactId, data.get(), true);
			data->AddRef();

			NotifyListeners(contactId, data.get());
		}
}

void CPresenceHolder::NotifyListeners(TInt aContact, CBBPresence* aPresence)
{
	for (TInt i=0; i < iListeners.Count(); i++)
		{
			MPresenceListener* listener = iListeners[i];
			CC_TRAPD(err, listener->PresenceChangedL(aContact, aPresence));
			listener->Notify(_L(""));
		}
}

void CPresenceHolder::DeletedL(const TTupleName& aName, const TDesC& aSubName)
{
	TPtrC nick( aSubName );

	if (nick.Length() == 0) return;

	TInt contactId=iJabberData.GetContactIdL(nick);
 	if (contactId != KErrNotFound) 
		{
			iContactsPresence->DeleteL(contactId);
			NotifyListeners(contactId, NULL);			
		}
}

EXPORT_C CBBPresence* CPresenceHolder::GetPresence(TInt aId)
{
	CALLSTACKITEM_N(_CL("CPresenceHolder"), _CL("GetPresence"));
	return (CBBPresence*)iContactsPresence->GetData(aId);
}

void CPresenceHolder::CBBPresenceDeletor(void* p)
{
	CALLSTACKITEMSTATIC_N(_CL("CPresenceHolder"), _CL("CBBPresenceDeletor"));
	if ( p )
		{
			CBBPresence* b=(CBBPresence*)p;			
			b->Release();
		}
}

void CPresenceHolder::IdChanged(const TDesC& aNick, TInt aOldId, TInt aNewId)
{
	if (aNewId==KErrNotFound && aOldId!=KErrNotFound) {
		iContactsPresence->DeleteL(aOldId);
	}
	
	if ( aOldId == aNewId) 
		return; // all is well already

	CBBPresence* pres(NULL);
	if (aOldId!=KErrNotFound && aNewId!=KErrNotFound) {
		pres = (CBBPresence*)iContactsPresence->GetData(aOldId);
		if ( pres ) {
			iContactsPresence->AddDataL(aNewId, pres, true);
			pres->AddRef();
			iContactsPresence->DeleteL(aOldId);
		}
	}
	if (aOldId!=KErrNotFound) {
		NotifyListeners(aOldId, NULL);
	}

	if (aNewId!=KErrNotFound) {
		NotifyListeners(aNewId, pres);
	}
}

TContactItemId CPresenceHolder::GetContactForNickL( const TDesC& aNick, const CBBPresence& aPresence )
{
	TInt id=iJabberData.GetContactIdL( aNick );
	
	// Do matching, if there is no previous relation or it is dummy contact relation
	// If there is previous relation, do matching only if new presence is with verified number
	// and old presence is with unverified number
	CJabberData::TRelationReason oldReason = CJabberData::EUnknown;					
	iJabberData.GetRelationReasonL( id, oldReason );
	
	if ( oldReason == CJabberData::ERemovedByUser )
		{
			return KNullContactId;
		}
	
	TBool doMatching = EFalse; 
	if ( id == KNullContactId  )
		{		
			doMatching = ETrue;
		}
	else if ( iJabberData.IsDummyContactId( id ) )
		{
			doMatching = ETrue;
		}
	else if ( oldReason == CJabberData::EMatchedWithUnverifiedNumber  &&  aPresence.iPhoneNumberIsVerified() )
		{
			doMatching = ETrue;
		}
	
	// Do automatic matching 
	if ( iMatcher && doMatching )
		{
			TContactItemId matchId = MatchPresenceToContactL( aNick, aPresence );
			if ( matchId != KNullContactId )
				id = matchId;
		}
	
	// If still not found, create empty dummy 
	if ( id == KNullContactId )
		{
			id = iJabberData.CreateDummyNickL(aNick, aPresence.iFirstName(), aPresence.iLastName() );
		}
	else if ( iJabberData.IsDummyContactId( id ) )
		{
			iJabberData.UpdateFirstLastNameL( id, aPresence.iFirstName(), aPresence.iLastName() );
		}
	
	return id;
}


TContactItemId CPresenceHolder::MatchPresenceToContactL(const TDesC& aNick, 
														const CBBPresence& aPresence)
{
	// 1) If found contact is already matched
	//   2.a) and if new contact is verified
	//        and old one is unverified
	//      -> overwrite, create dummy contact for old one
	//   2.b) no matches
	// 3) else (found contact not matched)
	//      -> set 
	// 4) if no matches
	//    -> create dummy contact
	TPtrC nick(aNick);
	auto_ptr<CContactIdArray> matched( iMatcher->FindMatchesForHashedNumberL( aPresence.iPhoneNumberHash() ) );
	
	if ( ! matched.get() )
		return KNullContactId;
	

	CJabberData::TRelationReason reason = aPresence.iPhoneNumberIsVerified() ?
		CJabberData::EMatchedWithVerifiedNumber  : CJabberData::EMatchedWithUnverifiedNumber;
	
	
	for ( TInt i = 0; i < matched->Count(); i++ )
		{
			TContactItemId matchId=(*matched)[i];
			
			CJabberData::TNick matchNick;
			if ( iJabberData.GetJabberNickL( matchId, matchNick ) )
				{
					if ( CJabberData::EqualNicksL( matchNick, aNick ) )
						{ // already matched
							return matchId;
						}
					
					CJabberData::TRelationReason matchReason = CJabberData::EUnknown;
					iJabberData.GetRelationReasonL( matchId, matchReason );
					if ( aPresence.iPhoneNumberIsVerified() &&
						 matchReason == CJabberData::EMatchedWithUnverifiedNumber )
						{
							// create dummy id for old match 							
							CBBPresence* oldsPresence = GetPresence( matchId );
							if ( oldsPresence ) {
								iJabberData.CreateDummyNickL( matchNick, 
															  oldsPresence->iFirstName(), 
															  oldsPresence->iLastName() );
							} else {
								iJabberData.CreateDummyNickL( matchNick, KNullDesC, KNullDesC );
							}

							// match id with new nick
							iJabberData.SetJabberNickL( matchId, aNick, 
														CJabberData::EMatchedWithVerifiedNumber );
							
							return matchId;
						}
				}
			else 
				{
					iJabberData.SetJabberNickL( matchId, aNick, reason );
					return matchId;
				}
		}
	return KNullContactId;
}
