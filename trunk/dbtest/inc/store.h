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

#ifndef STORE_H_INCLUDED
#define STORE_H_INCLUDED 1

#include "types.h"
#include "pointer.h"
#include "list.h"

class MReclaimable {
public:
	virtual void SetCurrentStamp(uint16 Stamp) = 0;
	virtual void ReclaimL(uint16 upto) = 0;
};

class CReclaimManager : public CBase {
// NOTE: This doesn't take ownership
// of the managed objects
public:
	static CReclaimManager* NewL();
	void AddManagedL(MReclaimable* Managed);
	~CReclaimManager();
	void SetTimeStamp(uint16 Stamp);
	void TriggerReclaimL(uint16 Upto);
private:
	CReclaimManager();
	void ConstructL();
	CList<MReclaimable*>* iList;

	uint16 iCurrentStamp;
};

class MCellMap {
public:
	virtual void AddMappingL(uint16 Cell, uint32 Sequence) = 0;
	virtual void FindL(uint16 Cell) = 0;
	virtual uint32 NextL() = 0; // return 0 when no more matches
	virtual ~MCellMap() { }
	virtual int Count() const = 0;
};

struct TBaseCount {
	uint16	Base;
	uint16	Count;
	TBaseCount(uint16 b, uint16 c) : Base(b), Count(c) { }
	TBaseCount() { }
	// default copy constructor ok
};

class MSequenceMap {
public:
	virtual void AddMappingL(uint32 Sequence, uint16 Base) = 0;
	virtual void FindL(uint32 Sequence) = 0;
	virtual TBaseCount NextL() = 0; // Base==0 wheno no more matches
	virtual ~MSequenceMap() { }
	virtual int Count() const = 0;
};

class MSequenceIdMap {
public:
	virtual uint32 AddSequenceL(uint16* Sequence) = 0;
	virtual uint32 GetSequenceIdL(uint16* Sequence) = 0;
	virtual const uint16* GetSequenceL(uint32 Id) = 0;
	virtual ~MSequenceIdMap() { }
	virtual int Count() const = 0;
};

class MGenericIntMap {
public:
	virtual void AddDataL(uint32 Key, void* data) = 0;
	virtual void* GetData(uint32 Key) = 0;
	virtual void DeleteL(uint32 Key) = 0;
	virtual ~MGenericIntMap() { }
	virtual int Count() const = 0;
	virtual void SetDeletor( void(*delete_func)(void* data) ) = 0;
};

class MMapFactory {
public:
	virtual MCellMap*	CreateCellMapL() = 0;
	virtual MSequenceMap*	CreateSequenceMapL() = 0;
	virtual MSequenceIdMap* CreateSequenceIdMapL(int SequenceLen) = 0;
	virtual MGenericIntMap*	CreateGenericIntMapL() = 0;
};
#endif
