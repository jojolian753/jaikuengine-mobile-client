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

#include "basestack.h"
#include "db.h"
#include "symbian_auto_ptr.h"

class CBaseStackImpl : public CBaseStack, public MContextBase, public MDBStore {
private:
	CBaseStackImpl(MApp_context& Context, CDb* Db);
	void ConstructL();
	~CBaseStackImpl();

	virtual void DeleteLastL();
	virtual void DeleteFirstL();
	virtual TBool LastL(TBaseItem& anItem);
	virtual TBool PrevL(TBaseItem& anItem);
	virtual TBool FirstL(TBaseItem& anItem);
	virtual TBool NextL(TBaseItem& anItem);
	virtual void PushBackL(const TBaseItem& anItem);
	virtual TInt CountL();
	virtual void SetLastLeft(const TTime& aLeft);
	virtual void SetLastName(const TDesC& aName);
	
	void Read(TBaseItem& anItem);
	void Write(const TBaseItem& anItem);

	CDb*	iDb;

	friend class CBaseStack;
	friend class auto_ptr<CBaseStackImpl>;
};

EXPORT_C CBaseStack* CBaseStack::NewL(MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CBaseStack"), _CL("NewL"),  &Context);

	auto_ptr<CDb> db(CDb::NewL(Context, _L("BASESTACK"), EFileRead|EFileWrite|EFileShareAny));
	auto_ptr<CBaseStackImpl> s(new (ELeave) CBaseStackImpl(Context, db.get()));
	db.release();
	s->ConstructL();
	return s.release();
}

EXPORT_C CBaseStack::~CBaseStack()
{
	CALLSTACKITEM_N(_CL("CBaseStack"), _CL("~CBaseStack"));

}

EXPORT_C CBaseStack::TBaseItem::TBaseItem() : iBaseId(-1), iBaseName(), iEntered(0), iLeft(0)
{
	CALLSTACKITEM_N(_CL("CBaseStack"), _CL("TBaseItem"));

}

EXPORT_C CBaseStack::TBaseItem::TBaseItem(const TBaseItem& anItem)
{
	CALLSTACKITEM_N(_CL("CBaseStack"), _CL("TBaseItem"));

	iBaseId=anItem.iBaseId;
	iBaseName=anItem.iBaseName;
	iEntered=anItem.iEntered;
	iLeft=anItem.iLeft;
}

EXPORT_C CBaseStack::TBaseItem::TBaseItem(TInt aBaseId, const TDesC& aBaseName, const TTime& anEntered) : iLeft(0)
{
	CALLSTACKITEM_N(_CL("CBaseStack"), _CL("TBaseItem"));

	iBaseId=aBaseId;
	iBaseName=aBaseName;
	iEntered=anEntered;
}

CBaseStackImpl::CBaseStackImpl(MApp_context& Context, CDb* Db) : MContextBase(Context), MDBStore(Db->Db()), iDb(Db)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("CBaseStackImpl"));

}

void CBaseStackImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("ConstructL"));

	int columns[] = { EDbColUint32, EDbColText16, EDbColDateTime, EDbColDateTime, -1 };
	int id_cols[] = { -1 };

	MDBStore::ConstructL(columns, id_cols, false, _L("basestack"));
}

CBaseStackImpl::~CBaseStackImpl()
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("~CBaseStackImpl"));

	delete iDb;
}

void CBaseStackImpl::DeleteLastL()
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("DeleteLastL"));

	if (iTable.LastL()) {
		iTable.DeleteL();
		//PutL();
	}
}

void CBaseStackImpl::DeleteFirstL()
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("DeleteFirstL"));

	if (iTable.FirstL()) {
		iTable.DeleteL();
		//PutL();
	}
}

TBool CBaseStackImpl::LastL(TBaseItem& anItem)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("LastL"));

	if (iTable.LastL()) {
		iTable.GetL();
		Read(anItem);
		return ETrue;
	}
	return EFalse;
}

TBool CBaseStackImpl::PrevL(TBaseItem& anItem)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("PrevL"));

	if (iTable.PreviousL()) {
		iTable.GetL();
		Read(anItem);
		return ETrue;
	}
	return EFalse;
}

TBool CBaseStackImpl::FirstL(TBaseItem& anItem)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("FirstL"));

	if (iTable.FirstL()) {
		iTable.GetL();
		Read(anItem);
		return ETrue;
	}
	return EFalse;
}

TBool CBaseStackImpl::NextL(TBaseItem& anItem)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("NextL"));

	if (iTable.NextL()) {
		iTable.GetL();
		Read(anItem);
		return ETrue;
	}
	return EFalse;
}

void CBaseStackImpl::PushBackL(const TBaseItem& anItem)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("PushBackL"));

	iTable.LastL();
	iTable.InsertL();
	Write(anItem);
	PutL();
}

TInt CBaseStackImpl::CountL()
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("CountL"));

	return iTable.CountL();
}

void CBaseStackImpl::Read(TBaseItem& anItem)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("Read"));

	anItem.iBaseId=iTable.ColUint(1);
	anItem.iBaseName=iTable.ColDes(2);
	anItem.iEntered=iTable.ColTime(3);
	anItem.iLeft=iTable.ColTime(4);
}

void CBaseStackImpl::Write(const TBaseItem& anItem)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("Write"));

	iTable.SetColL(1, anItem.iBaseId);
	iTable.SetColL(2, anItem.iBaseName.Left(50));
	iTable.SetColL(3, anItem.iEntered);
	iTable.SetColL(4, anItem.iLeft);
}

void CBaseStackImpl::SetLastLeft(const TTime& aLeft)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("SetLastLeft"));

	if (iTable.LastL()) {
		iTable.UpdateL();
		iTable.SetColL(4, aLeft);
		PutL();
	}
}

void CBaseStackImpl::SetLastName(const TDesC& aName)
{
	CALLSTACKITEM_N(_CL("CBaseStackImpl"), _CL("SetLastName"));

	if (iTable.LastL()) {
		iTable.UpdateL();
		iTable.SetColL(2, aName.Left(50));
		PutL();
	}
}

EXPORT_C CBaseStack::TBaseItem& CBaseStack::TBaseItem::operator=(const TBaseItem& aItem)
{
	iBaseId=aItem.iBaseId;
	iBaseName=aItem.iBaseName;
	iEntered=aItem.iEntered;
	iLeft=aItem.iLeft;

	return *this;
}
EXPORT_C void CBaseStack::TBaseItem::Reset()
{
	iBaseId=0;
	iBaseName.Zero();
	iEntered=TTime(0);
	iLeft=TTime(0);
}
