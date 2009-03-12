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

#include "circular.h"
#include "symbian_auto_ptr.h"

#include "app_context.h"
#include <e32svr.h>

EXPORT_C CCircularLog* CCircularLog::NewL(int Size, bool Reverse)
{
	CALLSTACKITEM_N(_CL("CCircularLog"), _CL("NewL"));

	auto_ptr<CCircularLog> ret(new (ELeave) CCircularLog(Reverse));
	ret->ConstructL(Size);
	return ret.release();
}

EXPORT_C void CCircularLog::DeleteLast()
{
	if (iLast==-1) return; //empty
	
	if (iLast == iFirst) {
		iLast=-1; iFirst=0;
		return;
	}

	--iLast;
	if (iLast<0) iLast+=iSize;
	if (iObserver) iObserver->ContentsChanged();
}

EXPORT_C void CCircularLog::AddL(const TDesC& String)
{
	CALLSTACKITEM_N(_CL("CCircularLog"), _CL("AddL"));

	++iLast;
	if (iFirst!=0 && iLast==iFirst) iFirst=iLast+1;

	if (iLast>=iSize) {
		iLast=0;
		iFirst=1;
	}
	if (iArray->MdcaCount()==iSize) {
		iArray->Delete(iLast);
	}
	iArray->InsertL(iLast, String);

	RDebug::Print(_L("Log at %d %S"), iLast, &String);
	if (iObserver) iObserver->ContentsChanged();
}

EXPORT_C CCircularLog::~CCircularLog()
{
	CALLSTACKITEM_N(_CL("CCircularLog"), _CL("~CCircularLog"));

	delete iArray;
}

CCircularLog::CCircularLog(bool Reverse) : iLast(-1)
{
	CALLSTACKITEM_N(_CL("CCircularLog"), _CL("CCircularLog"));

	iReverse=Reverse;
}

void CCircularLog::ConstructL(int Size)
{
	CALLSTACKITEM_N(_CL("CCircularLog"), _CL("ConstructL"));

	iArray=new (ELeave) CDesCArrayFlat(Size);
	iSize=Size;
}

EXPORT_C TInt CCircularLog::MdcaCount() const
{
	CALLSTACKITEM_N(_CL("CCircularLog"), _CL("MdcaCount"));

	if (iLast==-1) return 0;

	if (iLast>=iFirst) return iLast-iFirst+1;
	else return iLast-iFirst+iSize+1;
}

EXPORT_C TPtrC16 CCircularLog::MdcaPoint(TInt aIndex) const
{
	CALLSTACKITEM_N(_CL("CCircularLog"), _CL("MdcaPoint"));

	int idx;
	if (!iReverse) {
		idx=(iFirst+aIndex) % iSize;
	} else {
		idx=iLast-aIndex;
		if (idx<0) idx+=iSize;
	}
	return iArray->MdcaPoint(idx);
}

EXPORT_C void CCircularLog::SetObserver(MListObserver* Observer)
{
	CALLSTACKITEM_N(_CL("CCircularLog"), _CL("SetObserver"));

	iObserver=Observer;
}
