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
#include "tuplestore.h"
#include "symbian_auto_ptr.h"
#include "bbtypes.h"
#include "timeout.h"

static const TInt KVersionBeforeExpires=1;
static const TInt KVersionBeforeSubnameSplit=2;
static const TInt KVersionBeforeNewUids=3;
static const TInt KVersionBeforeNewUids2=4;
static const TInt KVersion=5;

const TTupleName KCLSettingsTuple = { { CONTEXT_UID_CONTEXTCOMMON2 }, 1 };

class CTupleStoreImpl : public CTupleStore, public MContextBase, public MDBStore, public MTimeOut {

	CTupleStoreImpl(CDb& Db, MApp_context& Context, MNotifyDeleted& aDeleteNotifications);
	void ConstructL();

	virtual void GetCurrentL(TTupleName& aNameInto, TDes& aSubNameInto, 
		TUint& aIdInto, TTupleType& aTupleTypeInto, 
		TComponentName& aComponentInto, RADbColReadStream& aDataInto,
		TUint& aSizeInto, TTime& aExpiresInto);
	virtual TUint PutL(TTupleType aTupleType, const TTupleName& aTupleName, const TDesC& aSubName, 
		const TComponentName& aComponent,
		const TDesC8& aSerializedData, TBBPriority aPriority, TBool aReplace,
		const TTime& aLeaseExpires, TBool aKeepExisting=EFalse);
	virtual void DeleteL(TTupleType aTupleType, const TTupleName& aName, const TDesC& aSubName);
	virtual void DeleteL(const TComponentName& aName);
	virtual void DeleteL(TUint aId, TTupleType& aTupleTypeInto, TTupleName& aNameInto, TDes& aSubNameInto);

	virtual void SeekL(TUint aId);
	virtual TBool FirstL(TTupleType aTupleType, const TTupleName& aTupleName, const TDesC& aSubName, 
			TBool aExact=ETrue);
	virtual TBool FirstL(TTupleType aTupleType, const TComponentName& aComponentName);
	virtual TBool NextL();
	virtual TUint GetCurrentIdL();
	TInt TextLength(TInt aColumn);

	void CleanOldTuplesL();

	enum TPartialMatch {
		EMatchNone,
		EMatchPartial,
		EMatchWildCard,
		EMatchFull
	};
	TPartialMatch MatchesL(TTupleType aTupleType, const TTupleName& aTupleName, const TDesC& aSubName);
	TPartialMatch MatchesPartialL(TTupleType aTupleType, const TTupleName& aTupleName, const TDesC& aSubName);
	TBool MatchesL(TTupleType aTupleType, const TComponentName& aComponentName);

	TBool SeekNameL(TTupleType aTupleType, const TTupleName& aName, const TDesC& aSubName);
	TBool SeekNameL(TTupleType aTupleType, const TTupleName& aName, const TComponentName& aComponentName);
	TBool SeekIdL(TUint aId);
	void GetLA(RADbColReadStream& aDataInto, TUint& aSizeInto);

	//CDb* SupportIncremental() { return &iDb; }
	
	~CTupleStoreImpl();

	void expired(CBase* aSource);
	friend class CTupleStore;
	friend class auto_ptr<CTupleStoreImpl>;

	enum TColumns {
		ETupleId=1,
		ETupleType,
		ENameModule,
		ENameId,
		ENameSubName1,
		EPriority,
		EComponentModule,
		EComponentId,
		EData,
		ELeaseExpires,
		ENameSubName2,
		ETries
	};
	enum TIndices {
		EIndexTuple = 0,
		EIndexId,
		EIndexComponent,
		EIndexLeaseExpires
	};
	TTupleName	iCurrentSearchTuple;
	TBuf<KMaxTupleSubNameLength> iCurrentSearchSubName;
	TTupleType	iCurrentSearchType;
	TComponentName	iCurrentSearchComponent;
	enum TCurrentSearch { ETuple, EComponent };
	TCurrentSearch	iCurrentSearchMode;
	TInt	iSubNameIndexLength;

	TTime		iNextExpiry;
	CTimeOut*	iTimer;
	MNotifyDeleted& iDeleteNotification;
	CDb&		iDb;
};

CTupleStore* CTupleStore::NewL(CDb& Db, MApp_context& Context, MNotifyDeleted& aDeleteNotification)
{
	CALLSTACKITEM_N(_CL("CTupleStore"), _CL("NewL"));

	auto_ptr<CTupleStoreImpl> ret(new (ELeave) CTupleStoreImpl(Db, Context, aDeleteNotification));
	ret->ConstructL();
	return ret.release();
}

CTupleStore::~CTupleStore()
{
	CALLSTACKITEM_N(_CL("CTupleStore"), _CL("~CTupleStore"));

}

CTupleStoreImpl::CTupleStoreImpl(CDb& Db, MApp_context& Context, MNotifyDeleted& aDeleteNotifications) :
	MContextBase(Context), MDBStore(Db.Db()), iDeleteNotification(aDeleteNotifications), iDb(Db) { }

TInt CTupleStoreImpl::TextLength(TInt aColumn)
{
	if (aColumn==ENameSubName1) 
		return KMaxTupleSubNameLength;
	else 
		return iSubNameIndexLength;
}

void CTupleStoreImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("ConstructL"));

	iSubNameIndexLength=40;

	// name:      id,           tupletype,   
	TInt cols[]={ EDbColUint32, EDbColInt32, 
	//{     Uid,         Id,          subname1 }, 
		EDbColInt32, EDbColInt32, EDbColText, 
	//      priority,   
		EDbColInt8,
	// {	Uid,         Id},
		EDbColInt32, EDbColInt32, 
	//      data              LeaseExpires
		EDbColLongBinary, EDbColDateTime, 
	//		subname2
		EDbColText, -1 };

	TInt col_flags[]={ TDbCol::EAutoIncrement, 0, 0, 0, 0, 0,
		0, 0, 
		0, 0,
		0, 0 };
	// the tuple id is added to indices so that data is read in
	// the order it arrives
	TInt idxs[]={ 
		ETupleType, ENameModule, ENameId, ENameSubName2, ETupleId, -2, 
		ETupleId, -2,
		ETupleType, EComponentModule, EComponentId, 
			EPriority, ETupleId, -2,
		ELeaseExpires, 
		-1 };

	MDBStore::ConstructL(cols, idxs, false, 
		_L("TUPLES"), EFalse, col_flags);

	iTimer=CTimeOut::NewL(*this, CActive::EPriorityIdle);
	if (SeekIdL(0)) {
		TInt version=0;
		iTable.GetL();
		// check version and migrate if necessary
		version=iTable.ColInt32(ENameId);

		if (version<KVersion) {
			SwitchIndexL(-1); // no index
			TBool found=iTable.FirstL();
			while (found) {
				iTable.GetL();
				TUint uid_in_table, new_uid;
				new_uid=uid_in_table=iTable.ColInt(ENameModule);
				GetBackwardsCompatibleUid(new_uid);
				if (new_uid != uid_in_table) {
					UpdateL();
					iTable.SetColL(ENameModule, new_uid);
					MDBStore::PutL();
				}
				found=iTable.NextL();
			}
		}
		if (version<=KVersionBeforeExpires) {
			SwitchIndexL(-1); // no index
			TBool found=iTable.FirstL();
			while (found) {
				iTable.GetL();
				TInt tupletype;
				TTupleName tn;

				tn.iModule.iUid=iTable.ColInt(ENameModule);
				tn.iId=iTable.ColInt(ENameId);
				tupletype=iTable.ColInt(ETupleType);

				if ( (!(tn == KCLSettingsTuple)) && tupletype!=ETupleSpaceInternal ) {
					MDBStore::DeleteL();
				}
				found=iTable.NextL();
			}
		}

		if (version<KVersionBeforeNewUids) {
			TInt no_idxs[]={ -1 };
			iTable.Close();
			DeleteIndices(_L("TUPLES"));
			CC_TRAPD(err, MDBStore::ConstructL(cols, no_idxs, 
				false, _L("TUPLES"), ETrue, col_flags));
			iDb.BeginL();
			TTransactionHolder th(*this);
			TBool found=iTable.FirstL();
			while (found) {
				iTable.GetL();
				if ( iTable.ColUint(ETupleId) == 0 ) {
					UpdateL();
					iTable.SetColL(ENameId, KVersion);
					MDBStore::PutL();
				} else if (! iTable.IsColNull(ENameSubName1)) {
					UpdateL();
					iTable.SetColL(ENameSubName2,
						iTable.ColDes(ENameSubName1).Left(iSubNameIndexLength));
					MDBStore::PutL();
				}
				found=iTable.NextL();
			}
			iDb.CommitL();
			MDBStore::ConstructL(cols, idxs, 
				false, _L("TUPLES"), ETrue, col_flags);
		}

		if (version>=KVersionBeforeNewUids) {
			CleanOldTuplesL();
		}
		SeekIdL(0);
		UpdateL();
		iTable.SetColL(ENameId, KVersion);
		MDBStore::PutL();
		
	} else {
		InsertL();
		iTable.SetColL(ETupleType, ETupleSpaceInternal);
		iTable.SetColL(ENameId, KVersion);
		iTable.SetColL( ELeaseExpires, Time::MaxTTime() );
		MDBStore::PutL();
	}
}

void CTupleStoreImpl::expired(CBase* aSource)
{
	CleanOldTuplesL();
}

void CTupleStoreImpl::GetCurrentL(TTupleName& aNameInto, TDes& aSubNameInto, 
	TUint& aIdInto, TTupleType& aTupleTypeInto, TComponentName& aComponentInto, RADbColReadStream& aDataInto,
	TUint& aSizeInto, TTime& aExpiresInto)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("GetCurrentL"));

	GetLA(aDataInto, aSizeInto);
	aNameInto.iModule.iUid=iTable.ColInt(ENameModule);
	aNameInto.iId=iTable.ColInt(ENameId);
	aSubNameInto=iTable.ColDes16(ENameSubName1);
	aComponentInto.iModule.iUid=iTable.ColInt(EComponentModule);
	aComponentInto.iId=iTable.ColInt(EComponentId);
	aIdInto=iTable.ColUint(ETupleId);
	aTupleTypeInto=(TTupleType)iTable.ColInt(ETupleType);
	aExpiresInto=iTable.ColTime(ELeaseExpires);
}

void CTupleStoreImpl::GetLA(RADbColReadStream& aDataInto, TUint& aSizeInto)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("GetLA"));

	iTable.GetL();
	if (iTable.IsColNull(EData)) {
		aSizeInto=0;
	} else {
		aDataInto.OpenLA(iTable, EData);
		aSizeInto=aDataInto.ReadUint32L();
	}
}

void CTupleStoreImpl::CleanOldTuplesL()
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("CleanOldTuplesL"));

	SwitchIndexL(EIndexLeaseExpires);
	iTable.FirstL();
	iTable.GetL();

	TTupleName name; TBuf<KMaxTupleSubNameLength> subname;
	TTime expires;
	
	if (iTable.IsColNull(ELeaseExpires)) {
		expires=TTime(0);
	} else {
		expires=iTable.ColTime(ELeaseExpires);
	}
	TTime now=GetTime();
	TInt tupletype=-1;
	while (now > expires) {
		if ( iTable.IsColNull(ELeaseExpires) ) {
			UpdateL();
			iTable.SetColL( ELeaseExpires, Time::MaxTTime() );
			MDBStore::PutL();
		} else {
			tupletype=iTable.ColInt(ETupleType);
			if (tupletype == ETupleDataOrRequest) {
				name.iModule.iUid=iTable.ColInt(ENameModule);
				name.iId=iTable.ColInt(ENameId);
				subname=iTable.ColDes16(ENameSubName1);
			}
			MDBStore::DeleteL();
			if (tupletype == ETupleDataOrRequest) {
				iDeleteNotification.NotifyDeleted(name, subname);
			}
		}
		if (! iTable.NextL() ) return;
		iTable.GetL();
		if (iTable.IsColNull(ELeaseExpires)) {
			expires=TTime(0);
		} else {
			expires=iTable.ColTime(ELeaseExpires);
		}
	}
	if ( expires == TTime(0) || expires == Time::MaxTTime() ) return;

	TTimeIntervalSeconds s;
	TInt err=expires.SecondsFrom(now, s);
	if (err==KErrOverflow) return;

	iNextExpiry=expires;
	TInt wait=s.Int();
	iTimer->Wait(wait);
}

TUint CTupleStoreImpl::PutL(TTupleType aTupleType, const TTupleName& aTupleName, const TDesC& aSubName, 
		const TComponentName& aComponent,
		const TDesC8& aSerializedData, TBBPriority aPriority, TBool aReplace,
		const TTime& aLeaseExpires, TBool aKeepExisting)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("PutL"));

	if (aTupleName.iModule.iUid == KBBAnyUidValue ||
		aTupleName.iId == KBBAnyId ||
		aComponent.iModule.iUid == KBBAnyUidValue ||
		aComponent.iId == KBBAnyId) User::Leave(KErrArgument);

	TUint ret;
	{
		TAutomaticTransactionHolder ath(*this);

		TBool exists=SeekNameL(aTupleType, aTupleName, aSubName);
		if (exists && aKeepExisting) {
			UpdateL();
			iTable.SetColL(ELeaseExpires, aLeaseExpires);
			MDBStore::PutL();
			return 0;
		} else if (exists && aReplace) {
			UpdateL();
		} else {
			InsertL();
			iTable.SetColL(ETupleType, aTupleType);
			iTable.SetColL(ENameModule, aTupleName.iModule.iUid);
			iTable.SetColL(ENameId, aTupleName.iId);
			iTable.SetColL(ENameSubName1, aSubName);
			iTable.SetColL(ENameSubName2, aSubName.Left(iSubNameIndexLength));
			iTable.SetColL(EPriority, aPriority);
			iTable.SetColL(EComponentModule, aComponent.iModule.iUid);
			iTable.SetColL(EComponentId, aComponent.iId);
		}
		ret=iTable.ColUint(ETupleId);

		if (aSerializedData.Length() > 0) {
			RADbColWriteStream w; w.OpenLA(iTable, EData);
			w.WriteUint32L(aSerializedData.Length());
			w.WriteL(aSerializedData);
			w.CommitL();
		} else {
			iTable.SetColNullL(EData);
		}
		iTable.SetColL(ELeaseExpires, aLeaseExpires);
		MDBStore::PutL();
	}

	if (aLeaseExpires < iNextExpiry) {
		TTime now=GetTime();
		TTimeIntervalSeconds s;
		aLeaseExpires.SecondsFrom(now, s);
		TInt wait=s.Int();
		iTimer->Wait(wait);
	}
	return ret;
}
	
void CTupleStoreImpl::DeleteL(const TComponentName& aName)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("DeleteL"));

	TBool found=EFalse;
	TComponentName name;
	while (FirstL(ETupleDataOrRequest, aName)) {
		MDBStore::DeleteL();
		found=ETrue;
	}
	while (FirstL(ETuplePermanentSubscriptionEvent, aName)) {
		MDBStore::DeleteL();
		found=ETrue;
	}
	while (FirstL(ETupleReply, aName)) {
		MDBStore::DeleteL();
		found=ETrue;
	}
	if (!found) User::Leave(KErrNotFound);
};

void CTupleStoreImpl::DeleteL(TTupleType aTupleType, const TTupleName& aName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("DeleteL"));

	TBool found=EFalse;
	TTupleName name; auto_ptr<HBufC> subname(HBufC::NewL(KMaxTupleSubNameLength));
	while (SeekNameL(aTupleType, aName, aSubName)) {
		MDBStore::DeleteL();
		if (aTupleType == ETupleDataOrRequest) {
			name.iModule.iUid=iTable.ColInt(ENameModule);
			name.iId=iTable.ColInt(ENameId);
			{
				TPtr p=subname->Des();
				p=iTable.ColDes16(ENameSubName1);
			}
			iDeleteNotification.NotifyDeleted(name, *subname);
		}
		found=ETrue;
	}
	if (!found) User::Leave(KErrNotFound);
}

void CTupleStoreImpl::DeleteL(TUint aId, TTupleType& aTupleTypeInto, TTupleName& aNameInto, TDes& aSubNameInto)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("DeleteL"));

	if (SeekIdL(aId)) {
		iTable.GetL();
		aNameInto.iModule.iUid=iTable.ColInt(ENameModule);
		aNameInto.iId=iTable.ColInt(ENameId);
		aSubNameInto=iTable.ColDes16(ENameSubName1);
		aTupleTypeInto=(TTupleType)iTable.ColInt(ETupleType);
		iDeleteNotification.NotifyDeleted(aNameInto, aSubNameInto);
		MDBStore::DeleteL();
	} else {
		User::Leave(KErrNotFound);
	}
}

CTupleStoreImpl::~CTupleStoreImpl()
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("~CTupleStoreImpl"));

	delete iTimer;
}

TBool CTupleStoreImpl::SeekNameL(TTupleType aTupleType, const TTupleName& aName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("SeekNameL"));

	TDbSeekMultiKey<4> rk;
	rk.Add(aTupleType);
	rk.Add((TInt)aName.iModule.iUid);
	rk.Add(aName.iId);
	if (aSubName.Length()>0)
		rk.Add(aSubName.Left(iSubNameIndexLength));
	SwitchIndexL(EIndexTuple);
	TBool ret=EFalse;

	ret=iTable.SeekL(rk);
	if (!ret) return EFalse;
	TPartialMatch m=MatchesL(aTupleType, aName, aSubName);
	while (m==EMatchPartial) {
		if (!iTable.NextL()) return EFalse;
		m=MatchesL(aTupleType, aName, aSubName);
	}
	if (m>EMatchPartial) return ETrue;
	return EFalse;
}

TBool CTupleStoreImpl::SeekIdL(TUint aId)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("SeekIdL"));

	TDbSeekKey rk(aId);
	SwitchIndexL(EIndexId);
	TBool ret=EFalse;

	
	ret=iTable.SeekL(rk);

	return ret;
}


CTupleStoreImpl::TPartialMatch CTupleStoreImpl::MatchesL(TTupleType aTupleType, const TTupleName& aTupleName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("TPartialMatch"));

	TPartialMatch m=MatchesPartialL(aTupleType, aTupleName, aSubName);
	if (m == EMatchPartial ) {
		if (
			aSubName.Left(iSubNameIndexLength).CompareF(
				iTable.ColDes(ENameSubName1))==0 ) return EMatchFull;
	}
	return m;
}

CTupleStoreImpl::TPartialMatch CTupleStoreImpl::MatchesPartialL(TTupleType aTupleType, 
								const TTupleName& aTupleName, const TDesC& aSubName)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("TPartialMatch"));

	iTable.GetL();
	if (iTable.ColInt32(ETupleType) != aTupleType) return EMatchNone;

	if (aTupleName.iModule==KBBAnyUid) {
		return EMatchWildCard;
	}
	if (iTable.ColInt32(ENameModule) != aTupleName.iModule.iUid) return EMatchNone;
	if (aTupleName.iId==KBBAnyId) return EMatchWildCard;
	if (iTable.ColInt32(ENameId) != aTupleName.iId) return EMatchNone;
	if (aSubName.Length()==0) return EMatchWildCard;
	if (aSubName.Left(iSubNameIndexLength).CompareF(iTable.ColDes(ENameSubName1))) 
		return EMatchNone;
	return EMatchPartial;
}


TBool CTupleStoreImpl::MatchesL(TTupleType aTupleType, const TComponentName& aComponentName)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("MatchesL"));

	iTable.GetL();
	if (iTable.ColInt32(ETupleType) != aTupleType) return EFalse;

	if (aComponentName.iModule==KBBAnyUid) {
		return ETrue;
	}
	if (iTable.ColInt32(EComponentModule) != aComponentName.iModule.iUid) return EFalse;
	if (aComponentName.iId==KBBAnyId) return ETrue;
	if (iTable.ColInt32(EComponentId) != aComponentName.iId) return EFalse;
	return ETrue;
}

void CTupleStoreImpl::SeekL(TUint aId)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("SeekL"));

	if (!SeekIdL(aId)) User::Leave(KErrNotFound);
}

TBool CTupleStoreImpl::FirstL(TTupleType aTupleType, const TTupleName& aTupleName, const TDesC& aSubName, 
		TBool aExact)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("FirstL"));

	iCurrentSearchMode=ETuple;
	iCurrentSearchType=aTupleType;
	iCurrentSearchTuple=aTupleName;
	iCurrentSearchSubName=aSubName;
	TDbSeekMultiKey<4> rk;
	rk.Add(aTupleType);
	if (aTupleName.iModule!=KBBAnyUid) {
		rk.Add((TInt)aTupleName.iModule.iUid);
		if (aTupleName.iId != KBBAnyId) {
			rk.Add(aTupleName.iId);
			if (aSubName.Length()>0 || aExact) {
				rk.Add(aSubName.Left(iSubNameIndexLength));
			}
		}
	}
	SwitchIndexL(EIndexTuple);
	TBool ret=EFalse;
	ret=iTable.SeekL(rk);
	if (!ret) return EFalse;
	TPartialMatch m=MatchesL(aTupleType, aTupleName, aSubName);
	while (m==EMatchPartial) {
		if (!iTable.NextL()) return EFalse;
		m=MatchesL(aTupleType, aTupleName, aSubName);
	}
	if (m>EMatchPartial) return ETrue;
	return EFalse;
}

TBool CTupleStoreImpl::FirstL(TTupleType aTupleType, const TComponentName& aComponentName)
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("FirstL"));

	iCurrentSearchMode=EComponent;
	iCurrentSearchType=aTupleType;
	iCurrentSearchComponent=aComponentName;
	TDbSeekMultiKey<3> rk;
	rk.Add(aTupleType);
	if (aComponentName.iModule.iUid!=KBBAnyUidValue) {
		rk.Add((TInt)aComponentName.iModule.iUid);
		if (aComponentName.iId != KBBAnyId) {
			rk.Add(aComponentName.iId);
		}
	}
	SwitchIndexL(EIndexComponent);
	TBool ret=EFalse;
	ret=iTable.SeekL(rk);
	if (ret && MatchesL(aTupleType, aComponentName)) return ETrue;
	return EFalse;
}


TBool CTupleStoreImpl::NextL()
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("NextL"));

	if (! iTable.NextL()) return EFalse;
	if (iCurrentSearchMode==ETuple) {
		if (MatchesL(iCurrentSearchType, iCurrentSearchTuple, iCurrentSearchSubName)>EMatchPartial)
			return ETrue;
		return EFalse;
	} else {
		return MatchesL(iCurrentSearchType, iCurrentSearchComponent);
	}
}
TUint CTupleStoreImpl::GetCurrentIdL()
{
	CALLSTACKITEM_N(_CL("CTupleStoreImpl"), _CL("GetCurrentIdL"));

	iTable.GetL();
	return iTable.ColUint32(ETupleId);
}
