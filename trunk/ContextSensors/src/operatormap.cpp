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
#include "operatormap.h"
#include "symbian_auto_ptr.h"
#include "raii_f32file.h"
#include <s32btree.h>
#include <bautils.h>

class COperatorMapImpl : public COperatorMap, public MContextBase {
	COperatorMapImpl(MApp_context& Context);
	void ConstructL();

	virtual TBool NameToMccMnc(const TDesC& aName, TUint& aMccInto, TUint& aMncInto);
	~COperatorMapImpl();

	virtual void AddRef();
	virtual void Release();

	struct TItem {
		TBuf<20>	iOperatorName;
		TUint		iMCC;
		TUint		iMNC;
	};

	class TOpKey : public MBtreeKey {
	public:
		virtual void Between(const TAny* aLeft,const TAny* aRight, TBtreePivot& aPivot) const;
		virtual TInt Compare(const TAny* aLeft,const TAny* aRight) const;
		virtual const TAny* Key(const TAny* anEntry) const;
	};
	void ProcessLineL(const TDesC& aLine);
	void DoConstructL();

	TBtreeFix< TItem, TBuf<20> > iOpTree; bool iOpTreeIsOpen;

	TOpKey iOpKey;
	CMemPagePool  *iOpPool;
	TUint	iRefCount;
	TBool	iConstructed;

	friend class COperatorMap;
	friend class auto_ptr<COperatorMapImpl>;
};

EXPORT_C COperatorMap* COperatorMap::NewL(MApp_context& Context)
{
	auto_ptr<COperatorMapImpl> ret(new (ELeave) COperatorMapImpl(Context));
	ret->ConstructL();
	ret->AddRef();
	return ret.release();
}

EXPORT_C COperatorMap::~COperatorMap() { }

COperatorMapImpl::COperatorMapImpl(MApp_context& Context) : MContextBase(Context), iOpTree(EBtreeFast) { }

void COperatorMapImpl::ProcessLineL(const TDesC& aLine)
{
	if (aLine.Length()==0) return;
	TItem i;

	TLex l(aLine);
	TChar c;
	
	User::LeaveIfError(l.Val(i.iMCC));
	while ( (c=l.Get()) && c!=',' );
	User::LeaveIfError(l.Val(i.iMNC));
	while ( (c=l.Get()) && c!=',' );
	i.iOperatorName=l.Remainder();
	if (i.iOperatorName.Length()==0) return;

	if (i.iOperatorName.Compare(_L("elisa"))==0) i.iOperatorName=_L("radiolinja");

	TBtreePos pos;
	if (!iOpTree.InsertL(pos, i, ENoDuplicates)) User::Leave(KErrAlreadyExists);

}

void COperatorMapImpl::ConstructL()
{
}

void COperatorMapImpl::DoConstructL()
{
	iOpPool=CMemPagePool::NewL();
	iOpTree.Connect(iOpPool, &iOpKey);
	iOpTreeIsOpen=true;

#ifndef __WINS__
	TFileName fn=_L("z:\\system\\bootdata\\operinfo.txt");
	if ( ! BaflUtils::FileExists(Fs(), fn) ) {
		fn=_L("z:\\system\\bootdata\\operinfo_west.txt");
	}
	RAFile f; f.OpenLA(Fs(), fn, EFileRead);
#else
	// return;
	RAFile f; f.OpenLA(Fs(), _L("z:\\system\\bootdata\\operinfo.txt"), EFileRead);
#endif
	TBuf<64> line; //maxlength known: mcc+mnc+operatorname=40
	TBuf8<128> readbuf;
	TBool ignoreline=EFalse, line_beg=ETrue;
	TInt err;
	
	for(f.Read(readbuf); readbuf.Length()>0; f.Read(readbuf)) {
		TPtrC readp((const TUint16*)readbuf.Ptr(), readbuf.Length()/2);
		for (int i=0; i<readp.Length(); i++) {
			TChar c=readp[i];
			if (c=='\n' || c=='\r') {
				CC_TRAP(err, ProcessLineL(line));
				ignoreline=EFalse;
				line_beg=ETrue;
				line.Zero();
				continue;
			}
			if (line_beg && c==';') {
				ignoreline=ETrue;
			} else if (! ignoreline) {
				if (line.Length()==line.MaxLength()) {
					// broken line, ignore
					ignoreline=ETrue;
					line.Zero();
				} else {
					line.Append(c.GetLowerCase());
				}
			}
			line_beg=EFalse;
		}
	}
	CC_TRAP(err, ProcessLineL(line));

	iConstructed=ETrue;
}

TBool COperatorMapImpl::NameToMccMnc(const TDesC& aName, TUint& aMccInto, TUint& aMncInto)
{
#ifndef __WINS__
	if (!iConstructed) DoConstructL();

	TBuf<20> name=aName.Left(20);
	name.LowerCase();
	if (name.Compare(_L("elisa"))==0) name=_L("radiolinja");

	TBtreePos	pos;
	TBool found;
	found=iOpTree.FindL(pos, name);
	if (!found) return EFalse;

	TItem e;
	iOpTree.ExtractAtL(pos, e);
	aMccInto=e.iMCC;
	aMncInto=e.iMNC;
	return ETrue;
#else
	RDebug::Print(_L("aName"));
	aMccInto=244;
	aMncInto=5;
	return ETrue;
#endif
}

COperatorMapImpl::~COperatorMapImpl()
{
	if (iOpTreeIsOpen) {
		CC_TRAPD(err, iOpTree.ClearL());
	}
	delete iOpPool;
}

void COperatorMapImpl::AddRef()
{
	++iRefCount;
}

void COperatorMapImpl::Release()
{
	--iRefCount;
	if (!iRefCount) delete this;
}

void COperatorMapImpl::TOpKey::Between(const TAny* aLeft,const TAny* , TBtreePivot& aPivot) const
{
	aPivot.Copy((const TUint8*)aLeft, sizeof(TBuf<20>));
}

TInt COperatorMapImpl::TOpKey::Compare(const TAny* aLeft,const TAny* aRight) const
{
	const TBuf<20>* left=(const TBuf<20>*)aLeft;
	const TBuf<20>* right=(const TBuf<20>*)aRight;

	return left->Compare(*right);
}

const TAny* COperatorMapImpl::TOpKey::Key(const TAny* anEntry) const
{
	const TItem* i=(const TItem*)anEntry;
	return &(i->iOperatorName);
}
