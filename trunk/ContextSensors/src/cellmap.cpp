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
#include "cellmap.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"

EXPORT_C CCellMap* CCellMap::NewL(MApp_context& Context, RDbDatabase& Db, COperatorMap* aOpMap)
{

	auto_ptr<CCellMap> ret(new (ELeave) CCellMap(Context, Db, aOpMap));
	ret->ConstructL();
	return ret.release();
}

EXPORT_C void CCellMap::Parse(const TDesC& Cell, TUint& cell, TUint& area, TDes& nw)
{

	TLex lex(Cell);

	// ignore errors, sometimes we have misformed
	// cellids
	lex.Val(area);
	lex.SkipCharacters(); lex.SkipSpace();
	lex.Val(cell);
	lex.SkipCharacters(); lex.SkipSpace();
	nw=lex.Remainder().Left(20);
	if (nw.Length()==0) nw=_L("UNKNOWN");
}

EXPORT_C TInt CCellMap::GetId(const TBBCellId& Cell)
{
	CALLSTACKITEM_N(_CL("CCellMap"), _CL("GetId"));

	TDbSeekMultiKey<4> rk;
	rk.Add(Cell.iCellId()); rk.Add(Cell.iLocationAreaCode()); rk.Add(Cell.iMCC()); rk.Add(Cell.iMNC());

	TInt id;
	SwitchIndexL(0);
	if (iTable.SeekL(rk)) {
		iTable.GetL();
		id=iTable.ColInt32(4);
		return id;
	} else {
		iTable.InsertL();
		iTable.SetColL(1, Cell.iCellId());
		iTable.SetColL(2, Cell.iLocationAreaCode());
		iTable.SetColL(5, Cell.iMCC());
		iTable.SetColL(6, Cell.iMNC());
		id=iNextId++;
		iTable.SetColL(4, id);
		PutL();
		return id;
	}

}

EXPORT_C void CCellMap::SetId(const TBBCellId& Cell, TInt id)
{
	CALLSTACKITEM_N(_CL("CCellMap"), _CL("SetId"));

	TDbSeekMultiKey<3> rk;
	rk.Add(Cell.iCellId()); rk.Add(Cell.iLocationAreaCode()); rk.Add(Cell.iShortName());

	TAutomaticTransactionHolder th(*this);
	SwitchIndexL(0);
	if (iTable.SeekL(rk)) {
		iTable.UpdateL();
		iTable.SetColL(4, id);
	} else {
		iTable.InsertL();
		iTable.SetColL(1, Cell.iCellId());
		iTable.SetColL(2, Cell.iLocationAreaCode());
		iTable.SetColL(5, Cell.iMCC());
		iTable.SetColL(6, Cell.iMNC());
		iTable.SetColL(4, id);
		if (id>=iNextId) iNextId=id+1;
	}
	PutL();
}

EXPORT_C CCellMap::~CCellMap()
{
	CALLSTACKITEM_N(_CL("CCellMap"), _CL("~CCellMap"));

	if (iOpMap) iOpMap->Release();

	if (iConstructed) {
		CC_TRAPD(err,
		iTable.Cancel();
		TDbSeekMultiKey<4> rk;
		rk.Add(0); rk.Add(0); rk.Add(0); rk.Add(0);
		SwitchIndexL(0);
		if (iTable.SeekL(rk)) {
			iTable.UpdateL();
			iTable.SetColL(4, iNextId);
			PutL();
			Settings().WriteSettingL(SETTING_CELLMAP_CLOSED, ETrue);
		}
		);
	}
}

CCellMap::CCellMap(MApp_context& Context, RDbDatabase& Db, COperatorMap* aOpMap) : 
	MDBStore(Db), MContextBase(Context), iOpMap(aOpMap)
{
	CALLSTACKITEM_N(_CL("CCellMap"), _CL("CCellMap"));

}

void CCellMap::ConstructL()
{
	iConstructed=EFalse;

	_LIT(KName, "CELLMAP");
	iOpMap->AddRef();

	TInt cols[]={ EDbColUint32, EDbColUint32, EDbColText, EDbColInt32, EDbColUint32, EDbColUint32, -1 };
	TInt idx[]={ 1, 2, 5, 6, -2, 4, -1 };
	SetTextLen(20);
	MDBStore::ConstructL(cols, idx, true, KName);

	if ( iTable.ColCount() == 4 ) {
		// old cell mapping
		CloseTable();
		DeleteIndices(KName);
		TInt no_idx[]={ -1 };
		AlterL(cols, no_idx, true, KName);
		TBool rows=iTable.FirstL();
		TUint MCC, MNC;
		TBuf<20> nw;
		while (rows) {
			iTable.GetL();
			nw=iTable.ColDes(3);
			if (iTable.ColUint32(1)!=0 || iTable.ColUint32(2)!=0 || nw.Length()!=0) {
				if (! iOpMap->NameToMccMnc(nw, MCC, MNC)) {
					iTable.DeleteL();
					rows=iTable.NextL();
					continue;
				}
			} else {
				MCC=0; MNC=0;
			}
			iTable.UpdateL();
			iTable.SetColL(5, MCC);
			iTable.SetColL(6, MNC);
			PutL();
			rows=iTable.NextL();
		}
		CloseTable();
		if (0) {
			// debugging
			CreateIndices(cols, idx, false, KName);
			MDBStore::ConstructL(cols, idx, true, KName);
			auto_ptr<Cfile_output_base> op(Cfile_output_base::NewL(AppContext(), _L("cellmap")));
			PrintMapping(*op);
		} else {
			CreateIndices(cols, idx, true, KName);
			MDBStore::ConstructL(cols, idx, true, KName);
		}
		
	}

	TDbSeekMultiKey<4> rk;
	rk.Add(0); rk.Add(0); rk.Add(0); rk.Add(0);
	SwitchIndexL(0);
	if (iTable.SeekL(rk)) {
		TBool is_closed;
		if (!Settings().GetSettingL(SETTING_CELLMAP_CLOSED, is_closed)) {
			is_closed=EFalse;
		}
		if (is_closed) {
			iTable.GetL();
			iNextId=iTable.ColInt32(4);
			iNextId++;
		} else {
			SwitchIndexL(-1);
			TInt idx;
			TBool rows=iTable.FirstL();
			while (rows) {
				iTable.GetL();
				idx=iTable.ColInt32(4);
				if (idx>=iNextId) iNextId=idx+1;
				rows=iTable.NextL();
			}
			SwitchIndexL(0);
		}
	} else {
		iNextId=1;
		iTable.InsertL();
		iTable.SetColL(1, (TUint32)0);
		iTable.SetColL(2, (TUint32)0);
		iTable.SetColL(5, (TUint32)0);
		iTable.SetColL(6, (TUint32)0);
		iTable.SetColL(4, iNextId);
		iNextId++;
		PutL();
	}
	Settings().WriteSettingL(SETTING_CELLMAP_CLOSED, EFalse);

	iOpMap->Release(); iOpMap=0;
	iConstructed=ETrue;
}

EXPORT_C void CCellMap::GetCellL(TInt Id, TBBCellId& Cell)
{
	SwitchIndexL(1);
	TDbSeekKey rk(Id);
	if (! iTable.SeekL(rk) ) 
		User::Leave(KErrNotFound);
	iTable.GetL();
	Cell.iCellId()=iTable.ColUint32(1);
	Cell.iLocationAreaCode()=iTable.ColUint32(2);
	Cell.iMCC()=iTable.ColUint32(5);
	Cell.iMNC()=iTable.ColUint32(6);
}

void CCellMap::PrintMapping(Mfile_output_base& To)
{
	CALLSTACKITEM_N(_CL("CCellMap"), _CL("PrintMapping"));

	//SwitchIndexL(-1);
	TInt idx, cell, area, mcc, mnc;
	TBool rows=iTable.FirstL();
	TBuf<20> nw;
	TBuf<80> line;
	while (rows) {
		iTable.GetL();
		cell=iTable.ColUint32(1);
		area=iTable.ColUint32(2);
		nw=iTable.ColDes(3);
		idx=iTable.ColInt32(4);
		mcc=iTable.ColUint32(5);
		mnc=iTable.ColUint32(6);
		line.Format(_L("%d\t%d, %d, %d, %d, %S\n"), idx, area, cell, mcc, mnc, &nw);
		To.write_to_output(line);
		rows=iTable.NextL();
	}
	SwitchIndexL(0);
}
