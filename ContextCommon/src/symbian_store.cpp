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
#include "symbian_store.h"
#include <bautils.h>
#include <e32std.h>
#include "app_context.h"

#include "symbian_tree.h"

const int COMPACT_BETWEEN_UPDATES=20;
const int COMPACT_BETWEEN_DELETES=20;

MDBReclaimableStore::MDBReclaimableStore()
{
}

void MDBReclaimableStore::SetCurrentStamp(uint16 Stamp)
{
	iStamp=Stamp;
}

void MDBReclaimableStore::ReclaimL(uint16 upto)
{
	User::LeaveIfError(iDb.Commit());
	_LIT(idx, "IDX");
	TInt err;
	uint32 count=0;
	CC_TRAP(err,
		User::LeaveIfError(iDb.Begin());
		iTable.SetNoIndex();
		while (iTable.NextL()) {
			iTable.GetL();
			if (iTable.ColUint16(iTimeStampCol)<=upto) {
				BeforeDelete();
				iTable.DeleteL();
				++count;
				if (count=COMPACT_BETWEEN_DELETES) {
					User::LeaveIfError(iDb.Commit());
					iDb.Compact();
					count=0;
					User::LeaveIfError(iDb.Begin());
				}
			}
		}
		User::LeaveIfError(iDb.Commit());
	);
	if (err!=KErrNone) {
		iDb.Rollback();
		iTable.Reset();
	}
	iDb.Compact();
	User::LeaveIfError(iTable.SetIndex(idx));
	User::LeaveIfError(iDb.Begin());
}

MDBReclaimableStore::~MDBReclaimableStore()
{
	TInt err;
	if (iTableOpen) { CC_TRAP(err, iTable.Close()); }
	if (iDbOpen) { 
		CC_TRAP(err, iDb.Commit());
		CC_TRAP(err, iDb.Compact());
		CC_TRAP(err, iDb.Close());
	}
	delete iStore;
}

void MDBReclaimableStore::ConstructL(int* columns, int* idx_columns, bool unique_idx, const TDesC& name, RFs& fs)
{
	//FIXME: get this from somewhere
	_LIT(dir, "c:\\system\\apps\\dbtest\\");
	_LIT(tablename, "MAP");
	_LIT(idx, "IDX");
	
	TFileName database_file_install;
	TFileName database_file;


	database_file_install.Format(_L("%S%S_inst.db"), &dir, &name);
	database_file.Format(_L("%S%S.db"), &dir, &name);

	BaflUtils file_util;
	if (file_util.FileExists(fs, database_file_install)) {
		User::LeaveIfError(file_util.CopyFile(fs, database_file_install, database_file));
		User::LeaveIfError(file_util.DeleteFile(fs, database_file_install));
	} else if (file_util.FileExists(fs, database_file)) {
		User::LeaveIfError(file_util.DeleteFile(fs, database_file));
	}

	TInt store_exists;
	CC_TRAP(store_exists, iStore = CPermanentFileStore::OpenL(fs, database_file, EFileRead|EFileWrite));
	if (store_exists==KErrNone) { 
		iDb.OpenL(iStore, iStore->Root());
		iDbOpen=true;
		User::LeaveIfError(iTable.Open(iDb, tablename));
		iTableOpen=true;
		iTimeStampCol=iTable.ColCount();
	} else {
		// construct database
		iStore = CPermanentFileStore::ReplaceL(fs, database_file,EFileRead|EFileWrite);
		iStore->SetTypeL(iStore->Layout());
		TStreamId id=iDb.CreateL(iStore);
		iDbOpen=true;
		iStore->SetRootL(id);
		iStore->CommitL();
		CDbColSet* cols=CDbColSet::NewLC();

		TBuf<10> colname; int colcount=1;
		for (int* col_i=columns; *col_i!=-1; col_i++) {
			colname.Format(_L("C%d"), colcount);
			colcount++;
			cols->AddL(TDbCol(colname, (TDbColType)*col_i));
		}
		iTimeStampCol=colcount;
		cols->AddL(TDbCol(_L("TS"), EDbColUint16));

		User::LeaveIfError(iDb.CreateTable(tablename, *cols));
		CleanupStack::PopAndDestroy(); // cols

		CDbKey* idx_key=CDbKey::NewLC();

		for (int* idx_i=idx_columns; *idx_i!=-1; idx_i++) {
			colname.Format(_L("C%d"), *idx_i);
			idx_key->AddL(TDbKeyCol(colname));
		}

		if (unique_idx) 
			idx_key->MakeUnique();

		User::LeaveIfError(iDb.CreateIndex(idx, tablename, *idx_key));
		CleanupStack::PopAndDestroy(); // idx_key

		User::LeaveIfError(iTable.Open(iDb, tablename));
		iTableOpen=true;
	}
	ReadFromDbL();
	User::LeaveIfError(iTable.SetIndex(idx));
	iDb.Begin();
}

void MDBReclaimableStore::PutL()
{
	iTable.SetColL(iTimeStampCol, iStamp);
	iTable.PutL();
	if (iUpdateCount==COMPACT_BETWEEN_UPDATES) {
		iUpdateCount=0;
		User::LeaveIfError(iDb.Commit());
		iDb.Compact();
		User::LeaveIfError(iDb.Begin());
	}
	++iUpdateCount;
}

void CCellMap::AddMappingL(uint16 Cell, uint32 Sequence)
{
	TDbSeekMultiKey<2> rk;
	rk.Add((TUint)Cell);
	rk.Add((TUint)Sequence);
	if (iTable.SeekL(rk) ) {
		return;
	}
	iTable.InsertL();
	iTable.SetColL(1, Cell);
	iTable.SetColL(2, Sequence);
	PutL();
}

	
void CCellMap::FindL(uint16 Cell)
{
	TDbSeekKey rk(Cell);
	iCurrentCell=Cell;
	if (iTable.SeekL(rk) ) {
		iMoreResults=true;
		iTable.GetL();
	} else {
		iMoreResults=false;
	}
	return;
}

uint32 CCellMap::NextL()
// return 0 when no more matches
{
	uint32 ret=0;
	if (iMoreResults) {
		ret=iTable.ColUint32(2);
		iMoreResults=false;
		if (iTable.NextL()) {
			iTable.GetL();
			if (iTable.ColUint16(1)==iCurrentCell) {
				iMoreResults=true;
			}
		}
	}
	return ret;
}

CCellMap::~CCellMap()
{
}

int CCellMap::Count() const
{
	return -1;
}

CCellMap* CCellMap::NewL(RFs& fs)
{
	auto_ptr<CCellMap> ret(new (ELeave) CCellMap);
	ret->ConstructL(fs);
	return ret.release();
}

CCellMap::CCellMap()
{
}
void CCellMap::ConstructL(RFs& fs)
{
	int cols[]={ EDbColUint16, EDbColUint32, -1 };
	int id_cols[]= { 1, 2, -1 };
	MDBReclaimableStore::ConstructL( cols, id_cols, true, _L("CellMap"), fs);
}


void CSequenceMap::AddMappingL(uint32 Sequence, uint16 Base)
{
	TDbSeekMultiKey<2> rk;
	rk.Add((TUint)Sequence);
	rk.Add((TUint)Base);
	if (iTable.SeekL(rk)) {
		iTable.GetL();
		uint16 count=iTable.ColUint16(3);
		iTable.UpdateL();
		iTable.SetColL(3, count+1);
	} else {
		iTable.InsertL();
		iTable.SetColL(1, Sequence);
		iTable.SetColL(2, Base);
		iTable.SetColL(3, 1);
	}
	PutL();
}

void CSequenceMap::FindL(uint32 Sequence)
{
	iMoreResults=false;
	iCurrentSequence=Sequence;

	TDbSeekKey rk((TUint)Sequence);
	if (iTable.SeekL(rk)) {
		iTable.GetL();
		iMoreResults=true;
	}
	return;
}

TBaseCount CSequenceMap::NextL()
// Base==0 wheno no more matches
{
	TBaseCount ret(0, 0);
	if (iMoreResults) {
		iMoreResults=false;
		ret.Base=iTable.ColUint16(2);
		ret.Count=iTable.ColUint16(3);
		if (iTable.NextL()) {
			iTable.GetL();
			if (iTable.ColUint32(1)==iCurrentSequence) {
				iMoreResults=true;
			}
		}
	}
	return ret;
}

CSequenceMap::~CSequenceMap()
{
}

int CSequenceMap::Count() const
{
	return -1;
}

CSequenceMap* CSequenceMap::NewL(RFs& fs)
{
	auto_ptr<CSequenceMap> ret(new (ELeave) CSequenceMap);
	ret->ConstructL(fs);
	return ret.release();
}

CSequenceMap::CSequenceMap()
{
}

void CSequenceMap::ConstructL(RFs& fs)
{
	int cols[]={ EDbColUint32, EDbColUint16, EDbColUint16, -1 };
	int idx_cols[] = { 1, 2, -1 };
	MDBReclaimableStore::ConstructL( cols, idx_cols, true, _L("SequenceMap"), fs);
}

uint32 CSequenceIdMap::AddSequenceL(uint16* Sequence)
{
	TDbSeekMultiKey<4> rk;
	rk.Add((TUint)Sequence[0]);
	rk.Add((TUint)Sequence[1]);
	rk.Add((TUint)Sequence[2]);
	rk.Add((TUint)Sequence[3]);
	
	if (iTable.SeekL(rk)) {
		iTable.GetL();
		return iTable.ColUint32(5);
	}

	uint16 *s=new (ELeave) uint16[4];
	memmove(s, Sequence, 4*sizeof(uint16));

	TInt err;
	CC_TRAP(err,
		iTable.InsertL();
		iTable.SetColL(1, s[0]);
		iTable.SetColL(2, s[1]);
		iTable.SetColL(3, s[2]);
		iTable.SetColL(4, s[3]);
		iTable.SetColL(5, iNextId);
		PutL();
	);
	if (err==KErrNone) {
		iRevMap->AddDataL(iNextId, s);
		++iStampCount;
		if (iStampCount==iStampGranularity) {
			iManager->SetTimeStamp(GetStamp()+1);
			iStampCount=0;
			if (GetStamp()>iLimit) {
				iManager->TriggerReclaimL(GetStamp()-iLimit);
			}
		}
		return iNextId++;
	} else {
		delete [] s;
		return 0;
	}
}

void CSequenceIdMap::ReadFromDbL()
{
	uint32 id;
	uint16 stamp=0, rowstamp;
	while (iTable.NextL()) {
		iTable.GetL();
		uint16* s=new (ELeave) uint16[4];
		s[0]=iTable.ColUint16(1);
		s[1]=iTable.ColUint16(2);
		s[2]=iTable.ColUint16(3);
		s[3]=iTable.ColUint16(4);
		id=iTable.ColUint32(5);
		iRevMap->AddDataL(id, s);
		if (id>(iNextId-1)) {
			iNextId=id+1;
		}
		++iStampCount;
		rowstamp=iTable.ColUint16(6);
		if (rowstamp>stamp) stamp=rowstamp;

	}
	iStampCount=iStampCount % iStampGranularity;
	iManager->SetTimeStamp(stamp);
}

void CSequenceIdMap::BeforeDelete()
{
	uint32 id=iTable.ColUint32(5);
	iRevMap->DeleteL(id);
}

uint32 CSequenceIdMap::GetSequenceIdL(uint16* Sequence)
{
	TDbSeekMultiKey<4> rk;
	rk.Add((TUint)*Sequence);
	rk.Add((TUint)Sequence[1]);
	rk.Add((TUint)Sequence[2]);
	rk.Add((TUint)Sequence[3]);

	if (iTable.SeekL(rk)) {
		iTable.GetL();
		return iTable.ColUint32(5);
	} else {
		return 0;
	}
}

const uint16* CSequenceIdMap::GetSequenceL(uint32 Id)
{
	return (uint16*)iRevMap->GetData(Id);
}

CSequenceIdMap::~CSequenceIdMap()
{
	delete iRevMap;
}

int CSequenceIdMap::Count() const
{
	return -1;
}

CSequenceIdMap* CSequenceIdMap::NewL(RFs& fs, MGenericIntMap* intmap, CReclaimManager* manager)
{
	auto_ptr<CSequenceIdMap> ret(new (ELeave) CSequenceIdMap);
	ret->ConstructL(fs, intmap, manager);
	return ret.release();
}

CSequenceIdMap::CSequenceIdMap()
{
	iNextId=1;
	iLimit=7;
	iStampGranularity=500;
	iStampCount=0;
}

void delete_uint16arr( void* arr )
{
	uint16* b=(uint16*)arr;
	delete[] b;
}

void CSequenceIdMap::ConstructL(RFs& fs, MGenericIntMap* intmap, CReclaimManager* manager)
{
	int cols[]={ EDbColUint16, EDbColUint16, EDbColUint16, EDbColUint16, EDbColUint32, -1 };
	int id_cols[] = { 1, 2, 3, 4, -1 };

	iRevMap=intmap;
	iRevMap->SetDeletor( delete_uint16arr );
	iManager=manager;
	MDBReclaimableStore::ConstructL(cols, id_cols, true, _L("SequenceIdMap"), fs);
}

MCellMap*	CMapFactory::CreateCellMapL()
{
	auto_ptr<CCellMap> ret(CCellMap::NewL(*iFs));
	iManager->AddManagedL(ret.get());
	return ret.release();
}

MSequenceMap*	CMapFactory::CreateSequenceMapL()
{
	auto_ptr<CSequenceMap> ret(CSequenceMap::NewL(*iFs));
	iManager->AddManagedL(ret.get());
	return ret.release();
}

MSequenceIdMap* CMapFactory::CreateSequenceIdMapL(int SequenceLen)
{
	if (SequenceLen!=4) User::Leave(-4);

	auto_ptr<CSequenceIdMap> ret(CSequenceIdMap::NewL(*iFs, CreateGenericIntMapL(), iManager));
	iManager->AddManagedL(ret.get());
	return ret.release();
}

MGenericIntMap*	CMapFactory::CreateGenericIntMapL()
{
	return CGenericIntMap::NewL();
}

CMapFactory* CMapFactory::NewL(RFs& fs)
{
	auto_ptr<CMapFactory> ret(new (ELeave) CMapFactory);
	ret->ConstructL(fs);
	return ret.release();
}

CMapFactory::CMapFactory()
{
}

void CMapFactory::ConstructL(RFs& fs)
{
	iFs=&fs;
	iManager=CReclaimManager::NewL();
}
