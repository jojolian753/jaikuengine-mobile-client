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

#ifndef BB_LIST_H_INCLUDED
#define BB_LIST_H_INCLUDED 1

#include "bbdata.h"
#include "list.h"

class CBBGenericList : public CBase, public MBBData {
public:
	IMPORT_C static CBBGenericList* NewL(const TDesC& aName, const TDesC& aChildName,
		const TDesC& aStringSep, MBBDataFactory* aFactory);
	IMPORT_C virtual ~CBBGenericList();

	IMPORT_C virtual const TDesC&    Name() const;

	IMPORT_C virtual void		IntoStringL(TDes& aString) const;
	IMPORT_C virtual void		IntoXmlL(MBBExternalizer* aBuf, TBool aIncludeType=ETrue) const;
	IMPORT_C virtual void		ExternalizeL(RWriteStream& aStream) const;

	IMPORT_C virtual void		FromStringL(const TDesC& aString);
	IMPORT_C virtual MNestedXmlHandler*	FromXmlL(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBool aCheckType=ETrue);
	IMPORT_C virtual void		InternalizeL(RReadStream& aStream);

	IMPORT_C virtual const TTypeName& Type() const;
	IMPORT_C static const TTypeName& StaticType();
	IMPORT_C virtual void	AddItemL(HBufC*	aName, MBBData* aData); // takes ownership
	IMPORT_C TInt		Count() const;
	IMPORT_C MBBData*	First();
	IMPORT_C MBBData*	Next();
	IMPORT_C const MBBData*	First() const;
	IMPORT_C const MBBData*	Next() const;
	IMPORT_C TBool Equals(const MBBData* aRhs) const;
	IMPORT_C void Reset();
	IMPORT_C virtual MBBData* Assign(const MBBData* aRhs);

	IMPORT_C virtual MBBData* CloneL(const TDesC& Name) const;
protected:
	IMPORT_C CBBGenericList& operator=(const CBBGenericList& aRhs);
	IMPORT_C CBBGenericList(const TDesC& aName, const TDesC& aChildName,
		const TDesC& aStringSep, MBBDataFactory* aFactory);
	IMPORT_C void	ConstructL();
	IMPORT_C virtual TBool	FixedType() const;
protected:
	IMPORT_C virtual void SetName(const TDesC* aName);
	struct TDataItem {
		HBufC*		iName;
		MBBData*	iData;

		TDataItem(HBufC* aName, MBBData* aData) : iName(aName), iData(aData) { }
		TDataItem() : iName(0), iData(0) { }
	};
	CList<TDataItem>*	iData;
	const TDesC			*iName;
	const TDesC&		iChildName;
	const TDesC&		iStringSep;
	MBBDataFactory*		iFactory;

	// used by CListXml to store intermediate results
	HBufC*			iCurrentName;
	MBBData*		iCurrentData;
	void			AddCurrentItemL();
	mutable CList<TDataItem>::Node  *iIterator;

	friend class CListXml;
};

#endif
