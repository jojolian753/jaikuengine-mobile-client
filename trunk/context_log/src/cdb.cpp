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

#include "cdb.h"
#include "symbian_auto_ptr.h"

#ifndef __S60V2__
#include "cdb_v1.h"
#else
#include "cdb_v2.h"
#endif

CCommDbDump::CCommDbDump()
{
}

void CCommDbDump::ConstructL()
{
	iFs.Connect();
	iCC=CCnvCharacterSetConverter::NewL();
	iCC->PrepareToConvertToOrFromL(KCharacterSetIdentifierIso88591, iFs);
	state=CCnvCharacterSetConverter::KStateDefault;
}

CCommDbDump::~CCommDbDump()
{
	iFile.Close();
	iFs.Close();
}

bool CCommDbDump::IsRow(TDbType t)
{
	if (t==TABLE || t==TABLE_SAME_AS_PREV || t==END) return false;
	return true;
}

void CCommDbDump::HandleField(const TDbItem& field)
{
	TInt err;
	TUint32 UintVal; TBool BoolVal;
	HBufC* LongTextVal;
	TPtrC name(field.iName);
	TRAP(err,
	switch(field.iType) {
	case UINT:
		iView->ReadUintL(name, UintVal);
		iValue.Format(_L("%u"), UintVal);
		break;
	case BOOL:
		iView->ReadBoolL(name, BoolVal);
		if (BoolVal) {
			iValue=_L("TRUE");
		} else {
			iValue=_L("FALSE");
		}
		break;
	case DES:
		iView->ReadTextL(name, iValue);
		break;
	case DES8:
		iView->ReadTextL(name, iBuf);
		CC().ConvertToUnicode(iValue, iBuf, state);
		break;
	case LONGDES:
		LongTextVal=iView->ReadLongTextLC(name);
		iValue=LongTextVal->Des().Left(2048);
		CleanupStack::PopAndDestroy();
		break;
	}
	);
	if (err!=KErrNone) {
		if (err==KErrUnknown) {
			iValue=_L("NULL");
		} else {
			iValue.Format(_L("Error reading value %d"), err);
		}
	}
	if (err!=-1) FieldValue(name, iValue);
}

void CCommDbDump::OpenViewLC(const TDesC& name)
{
	iView=db->OpenTableLC(name);
	CleanupStack::Pop();
}

void CCommDbDump::DumpDBtoFileL(const TDesC& FileName)
{
	User::LeaveIfError(iFile.Replace(Fs(), FileName, EFileWrite));
	CleanupClosePushL(iFile);

	db=CCommsDatabase::NewL(EDatabaseTypeIAP);
	CleanupStack::PushL(db);
	db->ShowHiddenRecords();

	TDbItem	item;
	int	i;
	int	previous_i=0;

	TInt err;

	for(i=0; (item=IAP_DB[i]).iType!=END; i++ ) {
		if (item.iType==TABLE) {
			previous_i=i;
		}
		if (item.iType==TABLE || item.iType==TABLE_SAME_AS_PREV) {
			TPtrC name(item.iName);
			BeginTable(name);
			TRAP(err, OpenViewLC(name));
			if (err!=KErrNone) {
				iValue=_L("Error opening table ");
				iValue.Append(TPtrC(item.iName));
				iValue.Append(_L(": "));
				iValue.AppendNum(err);
				FieldValue(_L("Error"), iValue);
			} else {
				CleanupStack::PushL(iView);
				while( (err=iView->GotoNextRecord()) == KErrNone ) {
					BeginRow();
					TDbItem field;
					int f;
					for (f=0; IsRow( (field=DEFAULT_FIELDS[f]).iType ); f++) {
						HandleField(field);
					}
					for (f=previous_i+1; IsRow( (field=IAP_DB[f]).iType ); f++) {
						HandleField(field);
					}
					EndRow();
				}
				CleanupStack::PopAndDestroy(); // iView
			}
			EndTable();
		}
	}
	CleanupStack::PopAndDestroy(2); // iFile, db
}


void CCommDbDump::BeginTable(const TDesC& name)
{
	iFile.Write(_L8("\nTABLE "));
	CC().ConvertFromUnicode(iBuf, name);
	iFile.Write(iBuf);
	iFile.Write(_L8("\n"));
}

void CCommDbDump::BeginRow()
{
	iFile.Write(_L8("\nROW"));
}

void CCommDbDump::FieldValue(const TDesC& name, const TDesC& value)
{
	iFile.Write(_L8("\n"));
	CC().ConvertFromUnicode(iBuf, name);
	iFile.Write(iBuf);
	iFile.Write(_L8(" = "));
	CC().ConvertFromUnicode(iBuf, value);
	iFile.Write(iBuf);
}

void CCommDbDump::EndRow()
{
	iFile.Write(_L8("\n"));
}

void CCommDbDump::EndTable()
{
	iFile.Write(_L8("\n"));
}

CCommDbDump* CCommDbDump::NewL()
{
	auto_ptr<CCommDbDump> ret(new (ELeave) CCommDbDump);
	ret->ConstructL();
	return ret.release();
}
