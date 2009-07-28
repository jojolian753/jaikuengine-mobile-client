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

#include "db.h"

#include "break.h"
#include <bautils.h>
#include <e32std.h>
#include "symbian_auto_ptr.h"
#include "app_context_fileutil.h"
#include "reporting.h"

class CCompactor: public CActive, public MContextBase {
public:
	static CCompactor* NewL(RDbDatabase& aDatabase) {
		CALLSTACKITEMSTATIC_N(_CL("CCompactor"), _CL("NewL"));
		auto_ptr<CCompactor> ret(new (ELeave) CCompactor(aDatabase));
		ret->ConstructL();
		return ret.release();
	}
	~CCompactor() {
		CALLSTACKITEM_N(_CL("CCompactor"), _CL("~CCompactor"));
		Cancel();
		iTimer.Close();
	}
	void Trigger() {
		CALLSTACKITEM_N(_CL("CCompactor"), _CL("Trigger"));
		if (IsActive()) return;
		iTimer.After(iStatus, TTimeIntervalMicroSeconds32(5*1000*1000));
		iState=EStarting;
		SetActive();
	}
	void Redo() {
		CALLSTACKITEM_N(_CL("CCompactor"), _CL("Redo"));
		if (!IsActive()) return;
		Cancel();
		Trigger();
	}
	void Finish() {
		CALLSTACKITEM_N(_CL("CCompactor"), _CL("Finish"));
		if (!IsActive()) return;
		if (iState==EStarting) return;
		
		iFinishing=ETrue;
		Cancel();
		iFinishing=EFalse;
		
		iStep=iStepBuf();
		TInt err=iStatus.Int();
		while (iStep!=0 && err==KErrNone) {
			err=iIncremental.Next(iStep);
		}
	}
private:
	RDbDatabase& iDb;
	RDbIncremental iIncremental;
	RTimer iTimer;
	
	enum TState { EIdle, EStarting, ECompacting };
	TState iState;
	
	CCompactor(RDbDatabase& aDatabase) : CActive(CActive::EPriorityStandard), iDb(aDatabase) { }
	void ConstructL() {
		CALLSTACKITEM_N(_CL("CCompactor"), _CL("ConstructL"));
		CActiveScheduler::Add(this);
		iTimer.CreateLocal();
	}
	TInt iStep;
	TPckgBuf<TInt> iStepBuf;
	TBool iFinishing;
	void DoCancel() {
		CALLSTACKITEM_N(_CL("CCompactor"), _CL("DoCancel"));
		if (iState==EStarting) {
			iTimer.Cancel();
			return;
		}
		if (iFinishing) return;
		Reporting().UserErrorLog(_L("Canceling compact"));
		iIncremental.Close();
	}
	void RunL() {
		CALLSTACKITEM_N(_CL("CCompactor"), _CL("RunL"));
		if (iStatus.Int()!=KErrNone) User::Leave(iStatus.Int());
		switch(iState) {
		case EIdle:
		case EStarting:
			User::LeaveIfError(iIncremental.Compact(iDb, iStep));
			iStepBuf()=iStep;
			//fallthru
		case ECompacting:
			if (iStepBuf()==0) {
				iErrorCount=0;
				iIncremental.Close();
				iState=EIdle;
				return;
			}
			iIncremental.Next(iStepBuf, iStatus);
			iState=ECompacting;
			SetActive();
		}
	}
	TInt iErrorCount;
	TInt RunError(TInt aError) {
		CALLSTACKITEM_N(_CL("CCompactor"), _CL("RunError"));
		TBuf<50> msg=_L("Error in compact: ");
		msg.AppendNum(aError);
		Reporting().UserErrorLog(msg);
		if (iState==ECompacting) iIncremental.Close();
		iState=EIdle;
		iErrorCount++;
		if (iErrorCount>10) return aError;
		return KErrNone;
	}
};

const int COMPACT_BETWEEN_UPDATES=10;
const int COMPACT_BETWEEN_DELETES=10;

EXPORT_C CDb* CDb::NewL(MApp_context& Context, const TDesC& dbname, TInt aFileMode, bool shared)
{
	CALLSTACKITEM2_N(_CL("CDb"), _CL("NewL"), &Context);
	auto_ptr<CDb> ret(new (ELeave) CDb(Context, shared));
	ret->ConstructL(dbname, aFileMode);
	return ret.release();
}

EXPORT_C CDb::~CDb()
{
	CALLSTACKITEM_N(_CL("CDb"), _CL("~CDb"));
	TInt err;
	if (iCompactor) iCompactor->Finish();
	delete iCompactor;
	if (iDbOpen) { 
		CC_TRAP(err, iDb.Compact());
		CC_TRAP(err, iDb.Close());
	}
	delete iStore;
}

EXPORT_C RDbDatabase& CDb::Db()
{
	CALLSTACKITEM_N(_CL("CDb"), _CL("Db"));
	return iDb;
}

CDb::CDb(MApp_context& Context, bool shared) : MContextBase(Context), iShared(shared) { }

void CDb::ConstructL(const TDesC& dbname, TInt aFileMode)
{
	CALLSTACKITEM_N(_CL("CDb"), _CL("ConstructL"));
	iFileMode=aFileMode;

	TFileName nameonly;
	nameonly.Format(_L("%S.db"), &dbname);
	MoveIntoDataDirL(AppContext(), nameonly);

	TFileName database_file_install;
	TFileName database_file;

	database_file_install.Format(_L("%S%S_inst.db"), &AppDir(), &dbname);
	database_file.Format(_L("%S%S.db"), &DataDir(), &dbname);

	if (BaflUtils::FileExists(Fs(), database_file_install)) {
		User::LeaveIfError(BaflUtils::CopyFile(Fs(), database_file_install, database_file));
		User::LeaveIfError(BaflUtils::DeleteFile(Fs(), database_file_install));
	}

	bool retry=true;
	while(retry) {
		retry=false;
		TInt store_exists;
		delete iStore; iStore=0;
		if(iDbOpen) iDb.Close(); iDbOpen=false;
		TRAP(store_exists,
			iStore = CPermanentFileStore::OpenL(Fs(), database_file, aFileMode)
			);
	
		//TRAP (store_exists, iStore = ::OpenL(Fs(), database_file, EFileRead|EFileWrite));

		if (store_exists==KErrNone) { 
			CC_TRAPD(err, iDb.OpenL(iStore, iStore->Root()));
			if (err==KErrNotFound) {
				// store not constructed properly
				delete iStore; iStore=0;
				User::LeaveIfError(Fs().Delete(database_file));
				retry=true;
			} else if(err!=KErrNone) {
				User::Leave(err);
			} else {
				iDbOpen=true;
				TInt err=iDb.Recover();
				if (err==KErrNotFound) User::Leave(KErrCorrupt);
				User::LeaveIfError(err);
			}
		} else {
			// construct database
			CC_TRAPD(err, {
				iStore = CPermanentFileStore::ReplaceL(Fs(), database_file,aFileMode);
				iStore->SetTypeL(iStore->Layout());
				TStreamId id=iDb.CreateL(iStore);
				iDbOpen=true;
				iStore->SetRootL(id);
				iStore->CommitL();
			});
			if (err!=KErrNone) {
				// roll back
				Fs().Delete(database_file);
				User::Leave(err);
			}
		}
	}
	/*
	 * named databases not supported?
	 *
	if (BaflUtils::FileExists(Fs(), database_file)) {
		TInt err=iDb.Open(Fs(), database_file);
		User::LeaveIfError(err);
	} else {
		TInt err=iDb.Create(Fs(), database_file);
		User::LeaveIfError(err);
	}
	*/
}

EXPORT_C void CDb::BeginL()
{
	CALLSTACKITEM_N(_CL("CDb"), _CL("BeginL"));
	User::LeaveIfError(iDb.Begin());
}

EXPORT_C void CDb::CommitL()
{
	CALLSTACKITEM_N(_CL("CDb"), _CL("CommitL"));
	TInt err=iDb.Commit();
	if (err!=KErrNone) {
		iDb.Rollback();
		if (iDb.IsDamaged()) iDb.Recover();
	}
	++iTransactionCount;
	if (iTransactionCount>4) iDb.Compact();
	User::LeaveIfError(err);
}

EXPORT_C void CDb::RollBack()
{
	CALLSTACKITEM_N(_CL("CDb"), _CL("RollBack"));
	iDb.Rollback();
	if (iDb.IsDamaged()) iDb.Recover();
	++iTransactionCount;
	if (iTransactionCount>4) iDb.Compact();
}

EXPORT_C void CDb::RedoCompactL()
{
	if (!iCompactor) iCompactor=CCompactor::NewL(iDb);
	iCompactor->Redo();
}

EXPORT_C void CDb::TriggerCompactL()
{
	if (!iCompactor) iCompactor=CCompactor::NewL(iDb);
	iCompactor->Trigger();
}

EXPORT_C void CDb::MarkCorrupt()
{
	iIsCorrupt=true;
}

EXPORT_C bool CDb::IsCorrupt()
{
	return iIsCorrupt;
}

EXPORT_C void CDb::ResetCorruptedDatabaseL(MApp_context_access& aContext, const TDesC& aDbName)
{
	CALLSTACKITEMSTATIC_N(_CL("CDb"), _CL("ResetCorruptedDatabaseL"));

	TTime now = GetTime();
	TBuf<20> timeBuf;
	now.FormatL(timeBuf, _L("%F%Y%M%D%H%T%S"));
	
	RFs& fs = aContext.Fs();

	TFileName dbFile; 
	TFileName backupFile;
	dbFile.Format(_L("%S%S.db"), &aContext.DataDir(), &aDbName);
	backupFile.Format(_L("%Scorrupt_%S_%S.txt"), &aContext.DataDir(), &aDbName, &timeBuf);
	
	if (BaflUtils::FileExists( fs, dbFile)) {
		User::LeaveIfError( BaflUtils::CopyFile( fs, dbFile, backupFile ) );
		User::LeaveIfError( BaflUtils::DeleteFile( fs, dbFile ) );
	}
	
}



EXPORT_C MDBStore::MDBStore(RDbDatabase& Db) : iDb(Db), iTextComparison(EDbCompareNormal)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("MDBStore"));
}

EXPORT_C void MDBStore::CloseTable()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("CloseTable"));
	if (iTableOpen) { CC_TRAPD(err, iTable.Close()); }
	iTableOpen=false;
}
EXPORT_C MDBStore::~MDBStore()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("~MDBStore"));
	CloseTable();
}

EXPORT_C TInt MDBStore::TextLength(TInt aColumn)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("TextLength"));
	return iTextLen;
}

EXPORT_C TInt MDBStore::KeyTextLength()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("KeyTextLength"));
	return 50;
}

EXPORT_C void MDBStore::CreateIndices(int* columns, int* idx_columns, bool unique_idx, const TDesC& name)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("CreateIndices"));
	TBuf<10> colname;
	if (idx_columns[0]!=-1) {
		CDbKey* idx_key=CDbKey::NewLC();
		TInt idx_count=0;

		for (int* idx_i=idx_columns; ; idx_i++) {
			if (*idx_i<0) {
				if (unique_idx && idx_count==0) 
					idx_key->MakeUnique();
				TBuf<30> idx_name;
				if (idx_count>0) idx_name.Format(_L("IDX%d"), idx_count);
				else idx_name=_L("IDX");
				TInt err=iDb.CreateIndex(idx_name, name, *idx_key);
				if (err)
					User::Leave(err);
				idx_key->Clear();
				++idx_count;

			} else {
				colname.Format(_L("C%d"), *idx_i);

				/*
				 * we must limit the key size, e.g. 255 characters
				 * is too long, so truncate the last text column in the idx
				 * (but we can't say 50 if the column is actually _shorter_
				 */
				if (*(idx_i+1)>=0) {
					idx_key->AddL(TDbKeyCol(colname));
				} else {
					if ( columns[*idx_i -1] == EDbColText)  {
						if (TextLength(*idx_i)!=0 && TextLength(*idx_i)<KeyTextLength()) {
							idx_key->AddL(TDbKeyCol(colname));
						} else {
							idx_key->AddL(TDbKeyCol(colname, KeyTextLength()));
						}
					} else {
						idx_key->AddL(TDbKeyCol(colname));
					}
					idx_key->SetComparison(iTextComparison);
				}
			}
			if (*idx_i==-1) break;
		}

		CleanupStack::PopAndDestroy(); // idx_key
	}
}

EXPORT_C void MDBStore::DeleteIndices(const TDesC& name)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("DeleteIndices"));
	CDbIndexNames* idxs=iDb.IndexNamesL(name);
	CleanupStack::PushL(idxs);
	TInt err;
	TBuf<20> idxname;
	for (int i=0; i< idxs->Count(); i++) {
		idxname=(*idxs)[i];
		err=iDb.DropIndex( idxname, name);
	}
	CleanupStack::PopAndDestroy();
}

EXPORT_C void MDBStore::ConstructL(int* columns, 
								   int* idx_columns, bool unique_idx,
			  const TDesC& name, TBool Alter, int* col_flags)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("ConstructL"));
	if (name.Length()>KMaxTableNameLen) User::Leave(KErrTooBig);
	iTableName=name;

	iCurrentIdx=-1;

	if (iTableOpen) iTable.Close();

	TInt table_exists=iTable.Open(iDb, name);
	iTable.Close();

	CDbColSet* cols=CDbColSet::NewLC();

	TBuf<10> colname; int colcount=1;
	for (int* col_i=columns; *col_i!=-1; col_i++) {
		colname.Format(_L("C%d"), colcount);
		colcount++;
		TDbCol n(colname, (TDbColType)*col_i);
		if (col_flags) {
			TInt flags=col_flags[colcount-2];

			/* the Database layer automatically sets ENotNull on
			 * EAutoIncrement columns. If we don't do the same,
			 * trying later to alter the table definition will fail
			 * with KErrArgument (-6)
			 */
			if (flags & TDbCol::EAutoIncrement) flags|=TDbCol::ENotNull;
			n.iAttributes=flags;
		}
		if (n.iType==EDbColText && TextLength(colcount-1)!=0) n.iMaxLength=TextLength(colcount-1);
		if (n.iType==EDbColBinary && TextLength(colcount-1)!=0) n.iMaxLength=TextLength(colcount-1);
		cols->AddL(n);
	}
#ifdef DEBUG_DBCOLS
	{
		TBuf<100> msg;
		RDebug::Print(_L("table has columns:"));
		auto_ptr<CDbColSet> cols(0);
		TRAPD(err, cols.reset(iDb.ColSetL(name))));
		for (int i=1; cols.get() && i <= cols->Count(); i++) {
			msg=_L("Column name: ");
			const TDbCol& col=(*cols)[i];
			msg.Append(col.iName);
			msg.Append(_L(" type: "));
			msg.AppendNum(col.iType);
			msg.Append(_L(" attr: "));
			msg.AppendNum(col.iAttributes);
			msg.Append(_L(" maxlength: "));
			msg.AppendNum(col.iMaxLength);
			RDebug::Print(msg);
		}
	}
#endif

#ifdef DEBUG_DBCOLS
	{
		TBuf<100> msg;
		RDebug::Print(_L("Asked to create table with columns:"));
		for (int i=1; i <= cols->Count(); i++) {
			msg=_L("Column name: ");
			const TDbCol& col=(*cols)[i];
			msg.Append(col.iName);
			msg.Append(_L(" type: "));
			msg.AppendNum(col.iType);
			msg.Append(_L(" attr: "));
			msg.AppendNum(col.iAttributes);
			msg.Append(_L(" maxlength: "));
			msg.AppendNum(col.iMaxLength);
			RDebug::Print(msg);
		}
	}
#endif

	TInt indexcount=0;
	{
		int* idx=idx_columns;
		if (*idx >= 0) {
			while (*idx != -1) {
				if (*idx == -2) indexcount++;
				idx++;
			}
			indexcount++;
		}
	}
	if (table_exists!=KErrNone) {
		TInt err_create=iDb.CreateTable(name, *cols);
		if (err_create==KErrAlreadyExists) 
			User::Leave(table_exists);
		User::LeaveIfError(err_create);

		CleanupStack::PopAndDestroy(); // cols

		CC_TRAPD(err, CreateIndices(columns, idx_columns, unique_idx, name));
		if (err!=KErrNone) {
			DeleteIndices(name);
			iDb.DropTable(name);
			User::Leave(err);
		}

		User::LeaveIfError(iTable.Open(iDb, name));
		iTableOpen=true;
	} else {
		if (Alter) {
			TInt actual_indexcount=0;
			{
				auto_ptr<CDbIndexNames> indexnames(iDb.IndexNamesL(name));
				actual_indexcount=indexnames->Count();
			}
			if (actual_indexcount != indexcount) DeleteIndices(name);
			TInt err=iDb.AlterTable(name, *cols);
			if (err!=KErrNone) {
				if (err==KErrArgument) {
					// have to recreate indexes
					DeleteIndices(name);
					TInt err=iDb.AlterTable(name, *cols);
					if (err!=KErrNone) 
						User::Leave(err);
					CC_TRAP(err, CreateIndices(columns, idx_columns, unique_idx, name));
					if (err!=KErrNone) {
						DeleteIndices(name);
						User::Leave(err);
					}
					actual_indexcount=indexcount;

				} else {
					User::Leave(err);
				}
			}
			if (actual_indexcount != indexcount) {
				CC_TRAP(err, CreateIndices(columns, idx_columns, unique_idx, name));
				if (err!=KErrNone) {
					DeleteIndices(name);
					User::Leave(err);
				}
			}
		}
		CleanupStack::PopAndDestroy(); // cols
		User::LeaveIfError(iTable.Open(iDb, name));
		iTableOpen=true;
	}

	for (int idx_i=indexcount-1; idx_i>=0; idx_i--) {
		CC_TRAPD(err, SwitchIndexL(idx_i));
		if (err==KErrNotFound) {
			if (iTableOpen) iTable.Close();
			iTableOpen=false;
			DeleteIndices(name);
			CreateIndices(columns, idx_columns, unique_idx, name);
			User::LeaveIfError(iTable.Open(iDb, iTableName));
			iTableOpen=true;
			SwitchIndexL(idx_i);
		} else if (err==KErrCorrupt) {
			if (iTableOpen) iTable.Close();
			iTableOpen=false;
			iDb.Recover();
			DeleteIndices(name);
			CreateIndices(columns, idx_columns, unique_idx, name);
			err=iTable.Open(iDb, iTableName);
			if (err!=KErrNone) {
				User::Leave(err);
			}
			iTableOpen=true;
			SwitchIndexL(idx_i);
		} else {
			User::LeaveIfError(err);
		}
	}

#ifdef DEBUG_DBCOLS
	{
		TBuf<100> msg;
		RDebug::Print(_L("Created with columns:"));
		auto_ptr<CDbColSet> cols(iDb.ColSetL(name));
		for (int i=1; i <= cols->Count(); i++) {
			msg=_L("Column name: ");
			const TDbCol& col=(*cols)[i];
			msg.Append(col.iName);
			msg.Append(_L(" type: "));
			msg.AppendNum(col.iType);
			msg.Append(_L(" attr: "));
			msg.AppendNum(col.iAttributes);
			msg.Append(_L(" maxlength: "));
			msg.AppendNum(col.iMaxLength);
			RDebug::Print(msg);
		}
	}
#endif
}

EXPORT_C CDb* MDBStore::SupportIncremental()
{
	return 0;
}

EXPORT_C void MDBStore::AlterL(int* columns, int* idx_columns, bool unique_idx,
	const TDesC& name, int* col_flags)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("AlterL"));
	if (iTableOpen) { CC_TRAPD(err, iTable.Close()); }
	ConstructL(columns, idx_columns, unique_idx, name, ETrue, col_flags);
}

EXPORT_C void MDBStore::Compact()
{
	if (SupportIncremental())
		SupportIncremental()->TriggerCompactL();
	else
		iDb.Compact();
}

EXPORT_C void MDBStore::BeginL()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("BeginL"));
	CancelCompactor();
	if (!iTableOpen) {
		{
			User::LeaveIfError(iDb.Recover());
		} 
		{
			User::LeaveIfError(iTable.Open(iDb, iTableName));
		}
		iTableOpen=true;
	}
	User::LeaveIfError(iDb.Begin());
}

EXPORT_C void MDBStore::InsertL()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("InsertL"));
	CancelCompactor();
	if (!iTableOpen) {
		{
			User::LeaveIfError(iDb.Recover());
		} 
		{
			User::LeaveIfError(iTable.Open(iDb, iTableName));
		}
		iTableOpen=true;
	}
	iTable.InsertL();
}

void MDBStore::CancelCompactor()
{
	if (SupportIncremental()) SupportIncremental()->RedoCompactL();
}

EXPORT_C void MDBStore::UpdateL()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("InsertL"));
	CancelCompactor();
	iTable.UpdateL();
}

EXPORT_C void MDBStore::SwitchIndexL(TInt Idx)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("SwitchIndexL"));
	CancelCompactor();
	if (iCurrentIdx==Idx) return;
	if (!iTableOpen) {
		{
			User::LeaveIfError(iDb.Recover());
		} 
		{
			User::LeaveIfError(iTable.Open(iDb, iTableName));
		}
		iTableOpen=true;
	}

	TBuf<30> idx_name;
	if (Idx==-1) {
		User::LeaveIfError(iTable.SetNoIndex());
		iCurrentIdx=Idx;
		return;
	}
	if (Idx>0) idx_name.Format(_L("IDX%d"), Idx);
	else idx_name=_L("IDX");
	User::LeaveIfError(iTable.SetIndex(idx_name));
	iCurrentIdx=Idx;
}

EXPORT_C TInt MDBStore::GetCurrentIndex()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("GetCurrentIndex"));
	return iCurrentIdx;
}

EXPORT_C void MDBStore::Reset(TInt aError)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("Reset"));
	// FIXME!
	// there should be a better way...
	//
	CancelCompactor();
	if (iInReset) return;
	iTable.Cancel();
	iInReset=true;
	if (aError==KErrNotReady) {
		iTable.Reset();
	} else if (aError==KErrDisconnected) {
		if (iTableOpen) iTable.Close();
		iTableOpen=false;
		iDb.Recover();
		TInt err=iTable.Open(iDb, iTableName);
		if (err!=KErrNone) {
			iInReset=false;
			User::Leave(err);
		}
		iTableOpen=true;
	} else if (aError==KErrCorrupt) {
		if (iTableOpen) iTable.Close();
		iTableOpen=false;
		iDb.Recover();
		TInt err=iTable.Open(iDb, iTableName);
		if (err!=KErrNone) {
			iInReset=false;
			User::Leave(err);
		}
		iTableOpen=true;
	} else {
		iInReset=false;
		User::Leave(aError);
	}
	TInt prev_idx=iCurrentIdx;
	iCurrentIdx=-2;
	CC_TRAPD(err2, SwitchIndexL(prev_idx));
	iInReset=false;
	User::LeaveIfError(err2);
}

EXPORT_C void MDBStore::Rollback()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("Rollback"));
	iTable.Cancel();
	if (iDb.InTransaction()) iDb.Rollback();
	if (iTableOpen) iTable.Close();
	iCurrentIdx=-2;
	iTableOpen=false;
	if (iDb.Recover()==KErrNone) {
		if (iTable.Open(iDb, iTableName)==KErrNone) {
			iTableOpen=true;
			TInt prev_idx=iCurrentIdx;
			CC_TRAPD(err, SwitchIndexL(prev_idx));
		}
	}
}

EXPORT_C void MDBStore::PutL()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("PutL"));
#ifdef __WINS__
  TInt dummy;
	TBreakItem b(GetContext(), dummy);
#endif
	iTable.PutL();
	if (!iDb.InTransaction() && iUpdateCount>=COMPACT_BETWEEN_UPDATES) {
		CALLSTACKITEM_N(_CL("MDBStore"), _CL("PutL.compact"));
		iUpdateCount=0;
		Compact();
	}
	++iUpdateCount;
}

EXPORT_C TBool MDBStore::CompactIfCloseL()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("CompactIfCloseL"));
	
	if (SupportIncremental()) return EFalse;
	if (iUpdateCount>5) {
		User::LeaveIfError(iDb.Compact());
		iUpdateCount=0;
		return ETrue;
	}
	return EFalse;
}

EXPORT_C void MDBStore::DeleteL()
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("DeleteL"));
	CancelCompactor();
	if ( iDb.InTransaction() ) {
		iTable.DeleteL();
	}
	else {
		TAutomaticTransactionHolder th(*this);
		iTable.DeleteL();
		
		if ( iUpdateCount>=COMPACT_BETWEEN_UPDATES ) {
			CALLSTACKITEM_N(_CL("MDBStore"), _CL("DeleteL.compact"));
			iUpdateCount=0;
			Compact();
		}
		++iUpdateCount;
	}
}

EXPORT_C void MDBStore::SetTextLen(TInt Len)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("SetTextLen"));
	if (Len>255) 
		iTextLen=255;
	else
		iTextLen=Len;
}

EXPORT_C void MDBStore::SetTextComparison(TDbTextComparison aComparison)
{
	CALLSTACKITEM_N(_CL("MDBStore"), _CL("SetTextComparison"));
	iTextComparison=aComparison;
}

EXPORT_C CSingleColDbBase::CSingleColDbBase(MApp_context& Context, RDbDatabase& Db) : MDBStore(Db), MContextBase(Context)
{
	CALLSTACKITEM_N(_CL("CSingleColDbBase"), _CL("CSingleColDbBase"));
}

EXPORT_C CSingleColDbBase::~CSingleColDbBase()
{
	CALLSTACKITEM_N(_CL("CSingleColDbBase"), _CL("~CSingleColDbBase"));
}

EXPORT_C bool CSingleColDbBase::SeekInnerL(TInt Idx, bool Updatable, bool Add)
{
	CALLSTACKITEM_N(_CL("CSingleColDbBase"), _CL("SeekInnerL"));
	TDbSeekKey rk(Idx);
	if (iTable.SeekL(rk) ) {
		iTable.GetL();
		if (Updatable) iTable.UpdateL();
		return true;
	} else if (Add) {
		if (!Updatable) User::Leave(-1001);
		iTable.InsertL();
		iTable.SetColL(1, Idx);
		return true;
	}
	return false;
}

EXPORT_C bool CSingleColDbBase::SeekL(TInt Idx, bool Updatable, bool Add)
{
	CALLSTACKITEM_N(_CL("CSingleColDbBase"), _CL("SeekL"));
	bool ret=false;
	ret=SeekInnerL(Idx, Updatable, Add);
	return ret;
}


EXPORT_C void CSingleColDbBase::ConstructL(const TDesC& TableName,
	int ColType)
{
	CALLSTACKITEM_N(_CL("CSingleColDbBase"), _CL("ConstructL"));
	int columns[] = { EDbColUint32, 0, -1 };
	columns[1]=ColType;
	int id_cols[]= { 1, -1 };

	// fix me : is this correct??
	// MDBStore::SetTextLen(255);
	MDBStore::ConstructL(columns, id_cols, true, TableName);
}


EXPORT_C TBool CSingleColDbBase::FirstL()
{
	CALLSTACKITEM_N(_CL("CSingleColDbBase"), _CL("FirstL"));
	return iTable.FirstL();
}

EXPORT_C TBool CSingleColDbBase::NextL()
{
	CALLSTACKITEM_N(_CL("CSingleColDbBase"), _CL("NextL"));
	return iTable.NextL();
}
