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

#ifndef CC_STRINGARG_H_INCLUDED
#define CC_STRINGARG_H_INCLUDED 1

#include <e32std.h>
#include "expat.h"

/*
 * This class is used to represent generic parameters
 * to error messages. The parameters are just concatenated.
 * 'Normal' users do not need to know about this class, it should
 * just be an implementation detail of MErrorInfoManager::(User|Tech)Message()
 *
 * Note that we have to make this variant-like for the 
 * built-in classes (instead of subclassing) so that something like
 *
 *	void foo(const TStringArg& a);
 * can be called like 
 *	foo(1);
 * while respecting the C++ only-one-user-defined-conversion rule
 *
 * for our own classes we can define an 'operator TStringArg' on the
 * class, which produces an object that uses the custom data and functions
 *
 */

class TStringArg {
public:
	IMPORT_C void AppendToString(TDes& aString) const;
	IMPORT_C virtual TUint Length() const;
	IMPORT_C explicit TStringArg(TUint aArg); 
	// explicit because otherwise conversion from int would be ambiguous
	IMPORT_C TStringArg(TInt aArg);
	IMPORT_C TStringArg(const TDesC& aArg);
	IMPORT_C TStringArg(const XML_Char* aChar);
	IMPORT_C TStringArg(const XML_Char** aCharArr);
	IMPORT_C TStringArg(const void* aCustomData, TInt aLength, 
		void(*aCustomAppend)(const void* aData, TDes& aString));
	template<int _Len> TStringArg(const TLitC<_Len>& aArg) {
		iVal.iDes=&aArg;
		iType=EDes;
	}
	template<int _Len> TStringArg(const TBuf<_Len>& aArg) {
		iVal.iDes=&aArg;
		iType=EDes;
	}
private:
	IMPORT_C virtual void DoAppendToString(TDes& aString) const;
	IMPORT_C TStringArg();
	enum TType { EInt, EUint, EDes, ECustom, EChar, ECharArr } iType;
	union {
		TUint iUint;
		TInt iInt;
		const TDesC *iDes;
		const void *iCustom;
		const TText16* iChar;
		const TText16** iCharArr;
	} iVal;
	TInt	iCustomLen;
	void(*iCustomAppend)(const void* aData, TDes& aString);
};

#endif
