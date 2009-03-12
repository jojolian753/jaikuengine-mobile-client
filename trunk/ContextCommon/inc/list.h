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

#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED 1

#include "pointer.h"

template<typename Data>
struct PlainNode {
	Data	Item;
	PlainNode<Data>*	Next;
	PlainNode(): Next(0) { }
	void Release() { }
};

template<typename Data>
struct PtrNode {
	Data*	Item;
	PtrNode<Data>*	Next;
	PtrNode(): Next(0), Item(0) { }
	~PtrNode() { delete Item; }
	void Release() { Item=0; }
};

template<typename Data, typename NodeType=PlainNode<Data> > class CList : public CBase {
public:
	typedef NodeType Node;

	Node	*iFirst, *iCurrent;
	TInt	iCount;
	Node* AppendL(Data Item) {
		Node* n=new (ELeave) Node;
		n->Item=Item;
		if (iCurrent) {
			iCurrent->Next=n;
			iCurrent=iCurrent->Next;
		} else {
			iCurrent=iFirst=n;
		}
		iCount++;
		return iCurrent;
		
	}
	void DeleteNode(Node* ListNode, bool Destroy)
	{
		if (!ListNode) return;
		Node* n=iFirst, *prev=0;
		while (n!=ListNode) {
			prev=n;
			n=n->Next;
		}
		if (n) {
			if (prev) {
				prev->Next=n->Next;
				if (n==iCurrent) iCurrent=prev;
			} else {
				if (iCurrent==iFirst) iCurrent=0;
				iFirst=iFirst->Next;
			}
		}
		if (Destroy) delete n;
		iCount--;
	}

	void MoveToTop(Node* ListNode)
	{
		if (ListNode==iFirst) return;
		DeleteNode(ListNode, false);
		ListNode->Next=iFirst;
		iFirst=ListNode;
		iCount++;
	}

	static CList<Data, NodeType>* NewL() {
		auto_ptr<CList <Data, NodeType> > self(new (ELeave) CList);
		self->ConstructL();
		return self.release();
	}

	~CList() {
		reset();
	}
	void reset() {
		while (iFirst!=0) {
			Node* tmp=iFirst->Next;
			delete iFirst;
			iFirst=tmp;
		}
		iCurrent=0;
		iCount=0;
	}
	void DeleteFirst() {
		if (!iFirst) return;
		Node* tmp=iFirst;
		if (iCurrent==iFirst) {
			iCurrent=tmp->Next;
		}
		iFirst=tmp->Next;
		delete tmp;
		iCount--;
	}
	void DeleteLast()
	{
		if (!iCurrent) {
			User::Leave(-1005);
		}
		DeleteNode(iCurrent, true);
	}
	Data Top() {
		if (!iFirst) return Data();
		return iFirst->Item;
	}

	Data Pop() {
		if (!iFirst) {
			return Data();
		}

		Data ret=iFirst->Item;
		iFirst->Release();
		DeleteFirst();
		return ret;
	}
	void Push(Data Item) {
		Node* n=new (ELeave) Node;
		n->Item=Item; // may not leave
		n->Next=iFirst;
		iFirst=n;
		if (!iCurrent) iCurrent=iFirst;
		iCount++;
	}
protected:
	CList() : iFirst(0), iCurrent(0), iCount(0) { }
	void ConstructL() { }
};

template<typename Data> class CPtrList : public CList< Data*, PtrNode<Data> >
{
public:
	static CPtrList<Data>* NewL() {
		auto_ptr<CPtrList <Data> > self(new (ELeave) CPtrList);
		self->ConstructL();
		return self.release();
	}
private:
	CPtrList() : CList<Data*, PtrNode<Data> >() { }
};

#endif
