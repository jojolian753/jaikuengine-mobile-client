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
#include "cm_tags.h"
#include "db.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"
#include <bautils.h>
#include "concretedata.h"
#include "raii_f32file.h"

_LIT(KTagFile, "c:\\system\\data\\context\\tags.txt");

class CTagStorageImpl : public CTagStorage, public MDBStore, public MContextBase {
public:
	CTagStorageImpl(CDb& aDb) : MDBStore(aDb.Db()) { }

	enum TColumns { 
		ETag=1, 
		ELastUse, 
		EUseCount };

	void AddFromFileL() {
		if (BaflUtils::FileExists(Fs(), KTagFile)) {
			RAFile f; f.OpenLA(Fs(), KTagFile, EFileRead|EFileShareAny);
			auto_ptr<CBBString> buf(CBBString::NewL(KNullDesC));
			TBuf8<128> buf8;
			TInt err=f.Read(buf8);
			while (err==KErrNone && buf8.Length()>0) {
				for (int i=0; i<buf8.Length(); i++) {
					TUint8 c=buf8[i];
					if (c=='\r' || c=='\n') {
						AddSingleStringL(buf->Value());
						buf->Zero();
					} else {
						TBuf<1> s; 
						s.Append(c);
						buf->Append(s);
					}
				}
				err=f.Read(buf8);
			}
			if (err==KErrNone) {
				AddSingleStringL(buf->Value());
			} else {
				Reporting().UserErrorLog(_L("Failed to read tags.txt"), err);
			}
		}
		Fs().Delete(KTagFile);
	}
	void ConstructL() {
		TInt cols[]= { 
			EDbColText,	// tag
			EDbColDateTime, // last use
			EDbColInt32, // usecount
			-1
		};
		TInt col_flags[]={ 0, 0, 0 };
		TInt idxs[]= { ETag, -2, ELastUse, -2, EUseCount, -1 };

		_LIT(KTags, "TAGS");
		SetTextComparison(EDbCompareCollated);	
		MDBStore::ConstructL(cols, idxs, false, KTags, ETrue, col_flags);

		iPos=-1;

		AddFromFileL();
	}
	TInt iPos;

	TInt MdcaCountL() {
		return iTable.CountL();
	}
	virtual TInt MdcaCount() const {
		CTagStorageImpl* i=(CTagStorageImpl*) this;
		TInt count=0;
		CC_TRAPD(err, count=i->MdcaCountL()); 
		return count;
	}

	void GoToItemL(TInt aIndex) {
		if (aIndex<0) User::Leave(KErrArgument);
		if (iPos==-1) {
			if (!iTable.FirstL()) User::Leave(KErrNotFound);
			iPos=0;
		}
		while (iPos<aIndex) {
			if (!iTable.NextL()) {
				iPos=-1;
				User::Leave(KErrNotFound);
			}
			++iPos;
		}
		while (iPos>aIndex) {
			if (!iTable.PreviousL()) {
				iPos=-1;
				User::Leave(KErrNotFound);
			}
			--iPos;
		}
	}
	virtual TPtrC16 MdcaPointL(TInt aIndex) {
		GoToItemL(aIndex);
		iTable.GetL();
		return iTable.ColDes(ETag);
	}

	virtual void DeleteItemL(TInt aIndex) {
		GoToItemL(aIndex);
		iPos=-1;
		DeleteL();
	}

	TBuf<52> iBuf;
	virtual TPtrC16 MdcaPoint(TInt aIndex) const {
		CTagStorageImpl* i=(CTagStorageImpl*) this;
		i->iBuf=_L("0\t");
		TPtrC16 p(0, 0);
		CC_TRAPD(err, p.Set(i->MdcaPointL(aIndex))); 
		if (err!=KErrNone) i->iPos=-1;
		i->iBuf.Append(p);
		return iBuf.Mid(0);
	}

	virtual TBool TagExists(const TDesC& aString) {
		TBuf<50> tag;
		if (aString.Length()>50) tag=aString.Left(50);
		else tag=aString;
		tag.Trim();
		if (tag.Length()==0) return EFalse;

		iPos=-1;
		TDbSeekKey rk(tag);
		if ( iTable.SeekL(rk) ) {
			return ETrue;
		}
		return EFalse;
	}

	virtual void AddSingleStringL(const TDesC& aString) {
		TBuf<50> tag;
		if (aString.Length()>50) tag=aString.Left(50);
		else tag=aString;
		tag.Trim();
		if (tag.Length()==0) return;

		iPos=-1;
		TDbSeekKey rk(tag);
		if ( iTable.SeekL(rk) ) {
			iTable.UpdateL();
			iTable.SetColL(EUseCount, iTable.ColInt(EUseCount)+1);
		} else {
			iTable.InsertL();
			iTable.SetColL(EUseCount, 1);
		}
		iTable.SetColL(ETag, tag);
		iTable.SetColL(ELastUse, GetTime());
		PutL();
	}
	virtual void AddFromStringAsNecessaryL(const TDesC& aString) {
		CALLSTACKITEM_N(_CL("CTagStorageImpl"), _CL("AddFromStringAsNecessaryL"));
		if (aString.Length()==0) return;

		TInt start_pos=0, colon_pos, comma_pos;
		colon_pos=aString.Locate(':');
		comma_pos=aString.Locate(',');
		if (colon_pos==KErrNotFound || (comma_pos!=KErrNotFound && comma_pos < colon_pos)) colon_pos=comma_pos;
		while (colon_pos!=KErrNotFound) {
			if (start_pos<colon_pos-1) {
				AddSingleStringL(aString.Mid(start_pos, colon_pos-start_pos));
			}
			if (colon_pos==aString.Length()-1) return;
			start_pos=colon_pos+1;
			colon_pos=aString.Mid(start_pos).Locate(':');
			comma_pos=aString.Mid(start_pos).Locate(',');
			if (colon_pos!=KErrNotFound) colon_pos+=start_pos;
			if (colon_pos==KErrNotFound || (comma_pos!=KErrNotFound && comma_pos < colon_pos)) colon_pos=comma_pos;
		}
		AddSingleStringL(aString.Mid(start_pos, aString.Length()-start_pos));
	}

	friend class CTagStorage;
	friend class auto_ptr<CTagStorageImpl>;
};

EXPORT_C CTagStorage* CTagStorage::NewL(class CDb& aDb)
{
	auto_ptr<CTagStorageImpl> ret( new (ELeave) CTagStorageImpl(aDb) );
	ret->ConstructL();
	return ret.release();
}
