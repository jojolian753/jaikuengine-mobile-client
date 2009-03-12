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

#include "symbian_tree.h"
#include "pointer.h"

class CGenericIntMapIterator : public MGenericIntMapIterator {
public:
	static CGenericIntMapIterator* NewL(TBtreeFix<CGenericIntMap::Entry, uint32>& Tree);
	virtual bool NextL(uint32& Key, void*& data);
	virtual ~CGenericIntMapIterator();
private:
	CGenericIntMapIterator(TBtreeFix<CGenericIntMap::Entry, uint32>& Tree);
	void ConstructL();
	TBtreeFix<CGenericIntMap::Entry, uint32>& iTree;
	TBtreeMark i;
	bool	first;
};


CGenericIntMapIterator* CGenericIntMapIterator::NewL(TBtreeFix<CGenericIntMap::Entry, uint32>& Tree)
{
	auto_ptr<CGenericIntMapIterator> ret(new (ELeave) CGenericIntMapIterator(Tree));
	ret->ConstructL();
	return ret.release();
}

bool CGenericIntMapIterator::NextL(uint32& Key, void*& data)
{
	TBool ret;
	if (first) {
		ret=iTree.ResetL(i);
		first=false;
	} else {
		ret=iTree.NextL(i);
	}
	if (ret) {
		CGenericIntMap::Entry e;
		iTree.ExtractAtL(i, e);
		data=e.data;
		Key=e.key;
	}
	return ret;
}

CGenericIntMapIterator::~CGenericIntMapIterator()
{
}

CGenericIntMapIterator::CGenericIntMapIterator(TBtreeFix<CGenericIntMap::Entry, uint32>& Tree) : iTree(Tree), first(true)
{
}

void CGenericIntMapIterator::ConstructL()
{
}

EXPORT_C void CGenericIntMap::AddDataL(uint32 Key, void* data, bool overwrite)
{
	if (overwrite) {
		DeleteL(Key);
	}
	TBtreePos pos;
	Entry e;
	e.key=Key; e.data=data;
	TBool no_dup=iTree->InsertL(pos, e);
	if (no_dup) iCount++;
	if (! no_dup && iDeletor) {
		(*iDeletor)(data);
	}
}

EXPORT_C void* CGenericIntMap::GetData(uint32 Key)
{
	TBtreePos pos;
	TBool found=iTree->FindL(pos, Key);
	if (found) {
		Entry e;
		iTree->ExtractAtL(pos, e);
		return e.data;
	} else {
		return 0;
	}
}

EXPORT_C void CGenericIntMap::DeleteL(uint32 Key)
{
	TBtreePos pos;
	TBool found=iTree->FindL(pos, Key);
	if (found) {
		if (iDeletor) {
			Entry e;
			iTree->ExtractAtL(pos, e);
			(*iDeletor)(e.data);
		}
		iTree->DeleteAtL(pos);
		iCount--;
	}
}

EXPORT_C CGenericIntMap::~CGenericIntMap()
{
	if (iDeletor) {
		TBtreeMark i;
		TBool more=iTree->ResetL(i);
		Entry e;
		while (more) {
			iTree->ExtractAtL(i, e);
			(*iDeletor)(e.data);
			more=iTree->NextL(i);
		}
	}

	delete iTree;
	delete iPool;
}

EXPORT_C void CGenericIntMap::Reset()
{
	if (iDeletor) {
		TBtreeMark i;
		TBool more=iTree->ResetL(i);
		Entry e;
		while (more) {
			iTree->ExtractAtL(i, e);
			(*iDeletor)(e.data);
			more=iTree->NextL(i);
		}
	}
	iTree->ClearL();
	iCount=0;
}

EXPORT_C int CGenericIntMap::Count() const
{
	return iCount;
}

EXPORT_C void CGenericIntMap::SetDeletor( void(*delete_func)(void* data) )
{
	iDeletor=delete_func;
}

EXPORT_C CGenericIntMap* CGenericIntMap::NewL()
{
	auto_ptr<CGenericIntMap> ret(new (ELeave) CGenericIntMap);
	ret->ConstructL();
	return ret.release();
}

MGenericIntMapIterator* CGenericIntMap::CreateIterator()
{
	return CGenericIntMapIterator::NewL(*iTree);
}

CGenericIntMap::CGenericIntMap()
{
}

void CGenericIntMap::ConstructL()
{
	iTree=new (ELeave) TBtreeFix<Entry, uint32>(EBtreeFast);
	iPool=CMemPagePool::NewL();
	iTree->Connect(iPool, &iKey);
}

void CGenericIntMap::GenKey::Between(const TAny* aLeft,const TAny* aRight,TBtreePivot& aPivot) const
{
	uint32 left=*(uint32*)aLeft;
	uint32 right=*(uint32*)aRight;

	uint32 mid=left+(right-left)/2;
	aPivot.Copy((TUint8*)&mid, sizeof(mid));
}


TInt CGenericIntMap::GenKey::Compare(const TAny* aLeft,const TAny* aRight) const
{
	uint32 left=*(uint32*)aLeft;
	uint32 right=*(uint32*)aRight;

	if (left<right) return -1;
	if (left>right) return 1;
	return 0;
}

const TAny* CGenericIntMap::GenKey::Key(const TAny* anEntry) const
{
	Entry* e=(Entry*)anEntry;
	return (TAny*)&(e->key);
}
