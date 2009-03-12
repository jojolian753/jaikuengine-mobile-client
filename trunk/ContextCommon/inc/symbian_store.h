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

#ifndef SYMBIAN_STORE_H_INCLUDED
#define SYMBIAN_STORE_H_INCLUDED 1

#include "store.h"

#include <d32dbms.h>
#include <f32file.h>
#include <s32file.h>

class MDBReclaimableStore : public MReclaimable {
public:
	virtual void SetCurrentStamp(uint16 Stamp);
	virtual void ReclaimL(uint16 upto);
	virtual void ReadFromDbL() { }
	virtual void BeforeDelete() { }
protected:
	MDBReclaimableStore();
	virtual ~MDBReclaimableStore();
	void ConstructL(int* columns, int* idx_columns, bool unique_idx, const TDesC& name, RFs& fs);
	CPermanentFileStore*	iStore;
	RDbStoreDatabase	iDb;
	RDbTable		iTable;
	void PutL();
	uint16			GetStamp() const { return iStamp; }
private:
	int			iUpdateCount; // for compacting
	int			iTimeStampCol; 
	bool			iDbOpen;
	bool			iTableOpen;
	uint16			iStamp;
};

class CCellMap : public CBase, public MCellMap, public MDBReclaimableStore {
public:
	virtual void AddMappingL(uint16 Cell, uint32 Sequence);
	virtual void FindL(uint16 Cell);
	virtual uint32 NextL(); // return 0 when no more matches
	virtual ~CCellMap();
	virtual int Count() const;

	static CCellMap* NewL(RFs& fs);
private:
	CCellMap();
	void ConstructL(RFs& fs);
	bool	iMoreResults;
	uint16	iCurrentCell;

};

class CSequenceMap : public CBase, public MSequenceMap, public MDBReclaimableStore {
public:
	virtual void AddMappingL(uint32 Sequence, uint16 Base);
	virtual void FindL(uint32 Sequence);
	virtual TBaseCount NextL(); // Base==0 wheno no more matches
	virtual ~CSequenceMap();
	virtual int Count() const;
	static CSequenceMap* NewL(RFs& fs);
private:
	CSequenceMap();
	void ConstructL(RFs& fs);
	bool	iMoreResults;
	uint32	iCurrentSequence;
};

class CSequenceIdMap : public CBase, public MSequenceIdMap, public MDBReclaimableStore {
public:
	virtual uint32 AddSequenceL(uint16* Sequence);
	virtual uint32 GetSequenceIdL(uint16* Sequence);
	virtual const uint16* GetSequenceL(uint32 Id);
	virtual ~CSequenceIdMap();
	virtual int Count() const;
	static CSequenceIdMap* NewL(RFs& fs, MGenericIntMap* intmap, CReclaimManager* manager);
	virtual void ReadFromDbL();
	virtual void BeforeDelete();
private:
	CSequenceIdMap();
	void ConstructL(RFs& fs, MGenericIntMap* intmap, CReclaimManager* manager);

	MGenericIntMap*	iRevMap;
	uint32		iNextId;
	CReclaimManager* iManager;

	uint32		iStampCount, iStampGranularity;
	uint16		iLimit;
};


class CMapFactory : public CBase, public MMapFactory {
public:
	virtual MCellMap*	CreateCellMapL();
	virtual MSequenceMap*	CreateSequenceMapL();
	virtual MSequenceIdMap* CreateSequenceIdMapL(int SequenceLen);
	virtual MGenericIntMap*	CreateGenericIntMapL();
	static CMapFactory* NewL(RFs& fs);
private:
	CMapFactory();
	void ConstructL(RFs& fs);
	RFs*	iFs;
	CReclaimManager* iManager;
};


#endif // SYMBIAN_STORE_H_INCLUDED
