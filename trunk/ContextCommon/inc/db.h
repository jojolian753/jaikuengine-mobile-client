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

#ifndef DB_H_INCLUDED
#define DB_H_INCLUDED 1

#include <d32dbms.h>
#include <f32file.h>
#include <s32file.h>
#include "app_context.h"

class CDb : public MContextBase, public CBase {
public:
	IMPORT_C static CDb* NewL(MApp_context& Context, const TDesC& dbname, 
		TInt aFileMode, bool shared=false);
	IMPORT_C virtual ~CDb();
	IMPORT_C RDbDatabase& Db();
	IMPORT_C void BeginL();
	IMPORT_C void CommitL();
	IMPORT_C void RollBack();
	IMPORT_C void RedoCompactL();
	IMPORT_C void TriggerCompactL();
	IMPORT_C void MarkCorrupt();
	IMPORT_C bool IsCorrupt();
	static IMPORT_C void ResetCorruptedDatabaseL(MApp_context_access& Context, const TDesC& aDbName);

	TInt			iFileMode;
private:
	CDb(MApp_context& Context, bool shared=false);
	void ConstructL(const TDesC& dbname, TInt aFileMode);
	CPermanentFileStore*	iStore;
	RDbStoreDatabase 	iDb;
	//RDbNamedDatabase 	iDb;
	bool			iDbOpen;
	bool			iShared;
	TInt			iTransactionCount;
	class CCompactor*	iCompactor;
	bool			iIsCorrupt;
	TFileName		iFileName;
};

#define KMaxTableNameLen 50

class MDBStore {
protected:
	IMPORT_C MDBStore(RDbDatabase& Db);
	IMPORT_C virtual ~MDBStore();
	IMPORT_C void ConstructL(int* columns, int* idx_columns, bool unique_idx,
		const TDesC& name, TBool Alter=EFalse, int* col_flags=0);
	IMPORT_C void AlterL(int* columns, int* idx_columns, bool unique_idx,
		const TDesC& name, int* col_flags=0);
	RDbDatabase&	iDb;
	RDbTable		iTable;
	IMPORT_C void SwitchIndexL(TInt Idx); // from 0
	IMPORT_C TInt GetCurrentIndex();
	IMPORT_C void PutL();
	IMPORT_C void DeleteL();
	IMPORT_C void SetTextLen(TInt Len); // has to be called before ConstructL
	IMPORT_C void Reset(TInt aError);
	IMPORT_C void DeleteIndices(const TDesC& name);
	IMPORT_C void CreateIndices(int* columns, int* idx_columns, bool unique_idx, const TDesC& name);
	IMPORT_C void CloseTable();
	IMPORT_C void SetTextComparison(TDbTextComparison aComparison); // has to be called before ConstructL
	IMPORT_C void Rollback();
	IMPORT_C void BeginL();
	IMPORT_C void InsertL();
	IMPORT_C void UpdateL();
	TBool			iHasTransactionHolder;
	IMPORT_C virtual TInt TextLength(TInt aColumn); // one-based
	IMPORT_C virtual TInt KeyTextLength(); // one-based
	IMPORT_C virtual CDb* SupportIncremental(); // 0 aka false by default
	IMPORT_C virtual TBool CompactIfCloseL();
	IMPORT_C virtual void Compact(); // trigger incremental compact if enabled, otherwise compact
private:
	void CancelCompactor();
	int			iCurrentIdx;
	int			iUpdateCount; // for compacting
	int			iTextLen;
	bool			iTableOpen;
	bool			iInReset;
	TBuf<KMaxTableNameLen>	iTableName;
	TDbTextComparison	iTextComparison;
	friend class TTransactionHolder;
	friend class TAutomaticTransactionHolder;
};

class CSingleColDbBase  : public MDBStore, public MContextBase, public CBase {
public:
	IMPORT_C TBool FirstL();
	IMPORT_C TBool NextL();
protected:
	IMPORT_C CSingleColDbBase(MApp_context& Context, RDbDatabase& Db);
	IMPORT_C ~CSingleColDbBase();
	IMPORT_C bool	SeekL(TInt Idx, bool Updatable=false, bool Add=false);
	IMPORT_C bool	SeekInnerL(TInt Idx, bool Updatable, bool Add);
	IMPORT_C void ConstructL(const TDesC& TableName,
		int ColType);
};

template <typename T, typename TMod=T> class CSingleColDb : public CSingleColDbBase {
public:
	static CSingleColDb<T, TMod>* NewL(MApp_context& Context, RDbDatabase& Db, 
		const TDesC& TableName);
	~CSingleColDb();
	void GetL(TUint& Idx, TMod& Value); // get current row
	bool GetValueL(TInt Idx, TMod& Value);
	void SetValueL(TInt Idx, const T& Value);
	int  ColType() const;
private:
	CSingleColDb(MApp_context& Context, RDbDatabase& Db);
	void ConstructL(const TDesC& TableName);
	void ReadValueL(TMod& Value);
	void WriteValue(const T& Value);
};

class TTransactionHolder {
private:
	MDBStore& iStore;
public:
	TTransactionHolder(MDBStore& aStore) : iStore(aStore) { 
		if (iStore.iHasTransactionHolder) 
			User::Leave(KErrInUse);
		iStore.iHasTransactionHolder=ETrue;
		CleanupClosePushL(*this); }
	~TTransactionHolder() { 
#ifdef __LEAVE_EQUALS_THROW__
		if (! std::uncaught_exception()) 
#endif
		{
			iStore.iHasTransactionHolder=EFalse;
			CleanupStack::Pop(); 			
		}
	}
	void Close() { 
		iStore.iHasTransactionHolder=EFalse;
		iStore.Rollback();
	}
};

class TAutomaticTransactionHolder {
private:
	MDBStore& iStore;
public:
	TAutomaticTransactionHolder(MDBStore& aStore) : iStore(aStore) { 
		if (iStore.iHasTransactionHolder) 
			User::Leave(KErrInUse);
		iStore.iHasTransactionHolder=ETrue;
		CleanupClosePushL(*this); 
	}
	~TAutomaticTransactionHolder() { 
#ifdef __LEAVE_EQUALS_THROW__
		if (! std::uncaught_exception()) 
#endif
		{
			iStore.iHasTransactionHolder=EFalse;
			CleanupStack::Pop(); 			
		}
	}
	void Close() { 
		iStore.iHasTransactionHolder=EFalse;
		iStore.Rollback();
	}
};

#endif
