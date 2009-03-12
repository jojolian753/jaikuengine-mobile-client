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

#include "stringarg.h"

_LIT(KTruncated, "[trunc]...");

EXPORT_C void TStringArg::AppendToString(TDes& aString) const
{
	if (aString.MaxLength()-aString.Length() >= Length() ) {
		DoAppendToString(aString);
	} else {
		aString.Append(KTruncated().Right(aString.MaxLength()-aString.Length()));
	}
}

_LIT(KSpace, " ");

EXPORT_C void TStringArg::DoAppendToString(TDes& aString) const
{
	switch (iType) {
	case EInt:
		aString.AppendNum(iVal.iInt);
		break;
	case EUint:
		aString.AppendNum(iVal.iUint);
		break;
	case EDes:
		aString.Append(*(iVal.iDes));
		break;
	case EChar:
		aString.Append(TPtrC(iVal.iChar));
		break;
	case ECharArr:
		{
			const TText16* c=iVal.iCharArr[0];
			while (c) {
				aString.Append(TPtrC(c));
				aString.Append(KSpace);
				c++;
			}
		}
		break;
	case ECustom:
		(*iCustomAppend)(iVal.iCustom, aString);
	}
}

EXPORT_C TUint TStringArg::Length() const
{
	switch (iType) {
	case EInt:
	case EUint:
		return 12;
	case EDes:
		return iVal.iDes->Length();
	case EChar:
		return TPtrC(iVal.iChar).Length();
	case ECharArr:
		{
			TInt len=0;
			const TText16* c=iVal.iCharArr[0];
			while (c) {
				len+=TPtrC(c).Length();
				len+=1;
				c++;
			}
			return len;
		}
	case ECustom:
		return iCustomLen;
	}
	return 0;
}

EXPORT_C TStringArg::TStringArg(TInt aArg) : iType(EInt) { iVal.iInt=aArg; }

EXPORT_C TStringArg::TStringArg(TUint aArg) : iType(EUint) { iVal.iUint=aArg; }

EXPORT_C TStringArg::TStringArg(const TDesC& aArg) : iType(EDes)  { iVal.iDes=&aArg; }

EXPORT_C TStringArg::TStringArg() : iType(ECustom) { }

EXPORT_C TStringArg::TStringArg(const XML_Char* aChar) : iType(EChar) { iVal.iChar=(TText16*)aChar; }
EXPORT_C TStringArg::TStringArg(const XML_Char** aCharArr) : iType(ECharArr) { iVal.iCharArr=(const TText16**)aCharArr; }

EXPORT_C TStringArg::TStringArg(const void* aCustomData, TInt aLength, 
				void(*aCustomAppend)(const void* aData, TDes& aString)) : iType(ECustom), 
				iCustomLen(aLength), iCustomAppend(aCustomAppend)
{
	iVal.iCustom=aCustomData;
}
