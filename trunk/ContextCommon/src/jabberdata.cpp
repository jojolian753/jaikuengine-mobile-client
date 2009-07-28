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

#include "jabberdata.h"
#include "symbian_auto_ptr.h"
#include "break.h"
#include "settings.h"

// Columns:   ContactId,   Nick,     , ShowDetailsInList 
enum 
	{
		EColContactId = 1,
		EColNick = 2,
		EColShowDetails = 3, 
		EColRelationReason = 4,
		EColFirstName = 5, // For dummy items
		EColLastName = 6
	};


enum 
	{
		EIndexContactId = 0,
		EIndexNick = 1
	};


// Static utility methods

const TInt KMaxDummyId(-5000);

EXPORT_C TBool CJabberData::EqualNicksL(const TDesC& aX, const TDesC& aY)
{
	TBuf<KNickMaxLen> x = aX;
	CanonizeNickL( x );
	
	TBuf<KNickMaxLen> y = aY;
	CanonizeNickL( y );
	
	// If nicks are empty, do not return true 
	if ( x.Length() == 0 && y.Length() == 0)
		{
			return EFalse;
		}
	return x == y;
}

EXPORT_C void CJabberData::CanonizeNickL( TDes& aNick )
{
	TInt at = aNick.Locate('@');
	aNick.LowerCase();
	if ( at == KErrNotFound && aNick.Length()<aNick.MaxLength()-12) 
		{
			aNick.Append(_L("@jaiku.com"));
		}
	// over come a bug, where canonize nick created nicks in format 
	// mikie@jaiku.com@jaiku.com.
	else if ( at >= 0 )
		{
			TInt rest = at + 1;
			TInt secondAt = aNick.Mid(rest).Locate('@');			
			if ( secondAt >= 0 )
				{
					aNick.Left( rest + secondAt );
				}
		}
}

/**
 * E.g. teemu@jaiku.com is converted to teemu.
 */ 
EXPORT_C void CJabberData::TransformToUiNickL( TDes& aNick )
{
	TInt at = aNick.Locate('@');
	if ( at >= 0 ) aNick = aNick.Left(at);
}


// Class methods 

EXPORT_C CJabberData* CJabberData::NewL(MApp_context& Context, CDb& Db, TInt aMyNickSettingId)
{
	auto_ptr<CJabberData> ret(new (ELeave) CJabberData(Context, Db, aMyNickSettingId));
	ret->ConstructL();
	return ret.release();
}

void CJabberData::ConstructL()
{
	// Columns:   ContactId,   Nick,     , ShowDetailsInList, Reason, First Name, Last Name 
	TInt cols[]={ EDbColInt32, EDbColText, EDbColInt32, EDbColInt32, EDbColText, EDbColText, -1 };
	// Indexed by: 
	TInt idx[]= { 1, -2, 2, -1 };

	TBool alter=EFalse;
	if (iJabberNickDb.iFileMode & EFileWrite) alter=ETrue;
	MDBStore::ConstructL(cols, idx, true, _L("NICK"), alter);
	LowerCaseNicksL();
	
	Settings().GetSettingL( iMyNickSettingId, iUserNick);
	if (iUserNick.Length()> 50) iUserNick.SetLength(50);
	Settings().NotifyOnChange( iMyNickSettingId, this );

	iObservers=CList<MJabberDataObserver*>::NewL();
}

void CJabberData::LowerCaseNicksL()
{
	TBool found=iTable.FirstL();
	TBuf<50> lc;
	while(found) {
		iTable.GetL();
		lc=iTable.ColDes(2);
		if (lc.Compare(iTable.ColDes(2))!=0) {
			iTable.UpdateL();
			iTable.SetColL(2, lc);
			CC_TRAPD(err, PutL());
			if (err!=KErrNone) {
				iTable.Cancel();
				if (err==KErrAlreadyExists) {
					DeleteL();
				}
			}
		}
		found=iTable.NextL();
	}
}

EXPORT_C void CJabberData::AddObserver(MJabberDataObserver* aObserver)
{
	if (!aObserver) return;
	iObservers->AppendL(aObserver);
}

EXPORT_C void CJabberData::RemoveObserver(MJabberDataObserver* aObserver)
{
	if (!aObserver) return;
	CList<MJabberDataObserver*>::Node *n=iObservers->iFirst;
	while (n) {
		if (n->Item == aObserver) {
			iObservers->DeleteNode(n, true);
			break;
		}
		n = n->Next;
	}
}



EXPORT_C TBool CJabberData::IsUserNickL( const TDesC& aNick )
{
	return EqualNicksL( iUserNick, aNick );
}
	

EXPORT_C const TDesC& CJabberData::UserNickL()
{
	return iUserNick;
}


EXPORT_C bool CJabberData::GetJabberNickL(TInt ContactId, TDes& Nick)
{
	SwitchIndexL( EIndexContactId );
	TDbSeekKey rk( ContactId );
	if (iTable.SeekL(rk) ) {
		iTable.GetL();
		Nick.CopyLC(iTable.ColDes(2));
		return true;
	} else {
		Nick.Zero();
		return false;
	}
}

EXPORT_C void CJabberData::SetJabberNickL(TInt aContactId,
										  const TDesC& aNick,
										  TRelationReason aReason)
{
	SetJabberNickImplL( aContactId, aNick, aReason, KNullDesC, KNullDesC );
}


EXPORT_C void CJabberData::MarkNickAsRemovedL(const TDesC& aNick)
{
	TContactItemId id = GetContactIdL(aNick);
	if ( ! IsDummyContactId( id ) )
		{
			id = GetNewDummyContactIdL();			
		}
	SetJabberNickImplL( id, aNick, ERemovedByUser, KNullDesC, KNullDesC );
}


EXPORT_C TInt CJabberData::CreateDummyNickL( const TDesC& aNick, 
										  const TDesC& aLastName, const TDesC& aFirstName )
{
	TInt id = GetNewDummyContactIdL();
	SetJabberNickImplL( id, aNick, CJabberData::EAutomaticDummy, aLastName, aFirstName );
	return id;
}


EXPORT_C TBool CJabberData::GetShowDetailsInListL(TInt ContactId)
{
	SwitchIndexL( EIndexContactId );
	TDbSeekKey rk( ContactId );
	if (iTable.SeekL(rk) ) {
		iTable.GetL();
		if (iTable.IsColNull(3)) return ETrue;
		return iTable.ColInt32(3);
	} else {
		return ETrue;
	}
}

EXPORT_C void CJabberData::SetShowDetailsInListL(TInt ContactId, TBool aShowDetails)
{
	SwitchIndexL( EIndexContactId );
	TDbSeekKey rk( ContactId);
	if (iTable.SeekL(rk) ) {
		iTable.UpdateL();
		iTable.SetColL(EColShowDetails, aShowDetails);
	} else {
		iTable.InsertL();
		iTable.SetColL(EColContactId, ContactId);
		iTable.SetColL(EColNick, KNullDesC);
		iTable.SetColL(EColShowDetails, aShowDetails);
	}
	PutL();
}

EXPORT_C CJabberData::~CJabberData()
{
	Settings().CancelNotifyOnChange( iMyNickSettingId, this );
	delete iObservers;
	
}

void CJabberData::NotifyObservers(const TDesC& aNick, TInt aOldId, TInt aNewId)
{
	CList<MJabberDataObserver*>::Node* i=iObservers->iFirst;
	
	while (i) {
		CC_TRAPD(err, i->Item->IdChanged(aNick, aOldId, aNewId));
		i=i->Next;
	}
}

EXPORT_C TInt CJabberData::GetContactIdL(const TDesC& Nick)
{
	// KErrNotFound if not found

	// 1) Try to match nick with @jaiku.com
	TBuf<50> n;
	n.CopyLC(Nick.Left(n.MaxLength()));
	if (n.Locate('@')==KErrNotFound && n.Length()<n.MaxLength()-12) n.Append(_L("@jaiku.com"));
	SwitchIndexL( EIndexNick );
	TDbSeekKey rk(n);
	if (iTable.SeekL(rk) ) {
		iTable.GetL();
		return iTable.ColInt32(1);
	} else {
		// 2) try to match nick without jaiku.com
		_LIT(KJaiku, "@jaiku.com");
		if (n.Right(KJaiku().Length()).CompareF(KJaiku)==0) {
			TInt at = n.Locate('@'); 
			if ( at >= 0) n = n.Left( at );

			TDbSeekKey rk(n);
			if (iTable.SeekL(rk) ) {
				iTable.GetL();
				return iTable.ColInt32(1);
			}
		}
	}
	return KErrNotFound;
}


EXPORT_C HBufC* CJabberData::GetFirstNameL(TInt aContactId)
{
	return GetNameImplL(aContactId, EColFirstName);
}

EXPORT_C HBufC* CJabberData::GetLastNameL(TInt aContactId)
{
	return GetNameImplL(aContactId, EColLastName);
}


EXPORT_C void CJabberData::UpdateFirstLastNameL(TInt aContactId, 
												const TDesC& aFirst, 
												const TDesC& aLast)
{
	SwitchIndexL(EIndexContactId);
	TDbSeekKey rk( aContactId );
	
	// Update names for existing contact id only 		
	if (iTable.SeekL(rk) ) {
		
		iTable.GetL();
		// Update only if data has changed
		if ( iTable.ColDes(EColFirstName) != aFirst ||
			 iTable.ColDes(EColLastName) != aLast )
			{
				iTable.UpdateL();
				iTable.SetColL(EColFirstName, aFirst.Left(50));
				iTable.SetColL(EColLastName,  aLast.Left(50));
				PutL();			
			}
	} 
}


CJabberData::CJabberData(MApp_context& Context, CDb& Db, TInt aMyNickSettingId) 
	: MDBStore(Db.Db()),
	  MContextBase(Context), 
	  iJabberNickDb(Db),
	  iMyNickSettingId(aMyNickSettingId)
{
}


void CJabberData::SettingChanged(TInt aSetting)
{
	if ( aSetting == iMyNickSettingId )
		{
			Settings().GetSettingL( iMyNickSettingId, iUserNick );	
			if (iUserNick.Length()> 50) iUserNick.SetLength(50);
		}
}


EXPORT_C TInt CJabberData::GetNewDummyContactIdL()
{
	SwitchIndexL( EIndexContactId );
	TInt potentialId = KMaxDummyId;
	
	TDbSeekKey rk(potentialId);	
	if ( iTable.SeekL(rk, RDbTable::ELessEqual ) )
		{
			do {
				iTable.GetL();		
				TInt reservedId = iTable.ColInt(1);
				if (reservedId < potentialId)
					{
						return potentialId;
					}
				else if (reservedId == potentialId)
					{
						potentialId--;
					}
				else 
					{
						// Coding error
						User::Leave(KErrGeneral);
					}
			} while ( iTable.PreviousL() ); 
		}
	
	return potentialId;
}

EXPORT_C TBool CJabberData::IsDummyContactId( TInt aId )
{
	return ( aId <= KMaxDummyId );
}

EXPORT_C CContactIdArray* CJabberData::GetDummyIdsL()
{
	auto_ptr<CContactIdArray> ids( CContactIdArray::NewL() );

	SwitchIndexL( EIndexContactId );
	TDbSeekKey rk(KMaxDummyId);
	if ( iTable.SeekL(rk, RDbTable::ELessEqual ) )
		{
			do {
				iTable.GetL();		
				TInt reason = iTable.ColInt( EColRelationReason );
				if ( static_cast<TRelationReason>(reason) != ERemovedByUser )
					{	
						ids->AddL( iTable.ColInt(1) );					
					}
			} while ( iTable.PreviousL() ); 
		}
	
	return ids.release();
}


EXPORT_C TBool CJabberData::GetRelationReasonL( TInt aId, TRelationReason& aResult )
{
	SwitchIndexL( EIndexContactId );
	TDbSeekKey rk( aId );
	if (iTable.SeekL(rk) ) {
		iTable.GetL();
		if ( iTable.IsColNull( EColRelationReason )  )
			{
				if ( CJabberData::IsDummyContactId( aId ) )		
					aResult = EAutomaticDummy;
				else
					aResult = EUnknown;
			}
		else 
			{
				TInt reason = iTable.ColInt( EColRelationReason );
				aResult = static_cast<CJabberData::TRelationReason>(reason);
			}
		return ETrue;
	} else {
		return EFalse;
	}
}



void CJabberData::SetJabberNickImplL(TInt ContactId, 
									 const TDesC& Nick, 
									 TRelationReason aReason,
									 const TDesC& aFirstName,
									 const TDesC& aLastName)
{

	TBuf<50> n;
	TInt at=Nick.Locate('@');
	if (at==KErrNotFound)  {
		n=Nick.Left(n.MaxLength());
	} else {
		if (at>50-12) at=50-12;
		n=Nick.Left(at);
	}
	TInt oldid=KErrNotFound;

	// We try to notify observers in reasonable 
	// way because they listen changes for 1) nick or for 3) contact id
	// but we don't know which way they are interested 

	TInt originalContact1 = KErrNotFound;
	TInt originalContact2 = KErrNotFound;
	TBool oldContactUpdated = EFalse;
	TBool newContactAdded = EFalse;
	// Delete old nick of form 'teemu'
	{
		SwitchIndexL( EIndexNick );
		TDbSeekKey rk(n);
		if (iTable.SeekL(rk) ) {
			iTable.GetL();
			while (iTable.ColDes(EColNick).CompareF(n)==0) {
				oldid=iTable.ColInt(1);
				DeleteL();
				if (!iTable.NextL()) break;
				iTable.GetL();
			}
		}
		if (oldid!=KErrNotFound && aReason!=ERemovedByUser) originalContact1 = oldid;
	}
	
	// Delete old nick of form 'teemu@jaiku.com'
	if (n.Length()<n.MaxLength()-12) n.Append(_L("@jaiku.com"));
	{
		SwitchIndexL( EIndexNick );
		TDbSeekKey rk(n);
		if (iTable.SeekL(rk) ) {
			iTable.GetL();
			while (iTable.ColDes(EColNick).CompareF(n)==0) {
				oldid=iTable.ColInt(1);
				DeleteL();
				if (!iTable.NextL()) break;
				iTable.GetL();
			}
		}
		if (oldid!=KErrNotFound && aReason!=ERemovedByUser) originalContact2 = oldid;
	}

	// Update or insert new nick 
	TInt newid=KErrNotFound;
	{
		SwitchIndexL( EIndexContactId );
		TDbSeekKey rk( ContactId);
		n.CopyLC(Nick.Left(n.MaxLength()));
		oldid=KErrNotFound;
		if (iTable.SeekL(rk) ) {
			// existing contact
			oldid=ContactId;
			iTable.GetL();
			iTable.UpdateL();
			oldContactUpdated = ETrue;
		} else {
			// new contact
			newid=ContactId;
			iTable.InsertL();
			iTable.SetColL(EColContactId, ContactId);
			newContactAdded = ETrue;
		}

		iTable.SetColL(EColNick, n);
		iTable.SetColL(EColRelationReason, aReason);
		if ( aFirstName.Length() ) iTable.SetColL(EColFirstName, aFirstName.Left(50));
		if ( aLastName.Length() )  iTable.SetColL(EColLastName,  aLastName.Left(50));

		PutL();
	}

	if (aReason!=ERemovedByUser) 
		{
			if ( originalContact1 != KErrNotFound )
				{
					NotifyObservers(Nick, originalContact1, ContactId);
				}

			if ( originalContact2 != KErrNotFound )
				{
					NotifyObservers(Nick, originalContact2, ContactId);
				}
			
			if ( newContactAdded || oldContactUpdated )
				{
					NotifyObservers(Nick, oldid, newid);
				}

		}	
}


HBufC* CJabberData::GetNameImplL(TInt aContactId, TInt aColumn)
{
	SwitchIndexL( EIndexContactId );
	TDbSeekKey rk( aContactId );
	if (iTable.SeekL(rk) ) {
		iTable.GetL();
		if ( iTable.IsColNull( aColumn ) )
			return NULL;
		else
			return iTable.ColDes( aColumn ).AllocL();
	}
	else
		{
			return NULL;
		}
}


