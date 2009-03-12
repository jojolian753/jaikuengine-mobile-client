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

#ifndef CC_XMLBUF_H_INCLUDED
#define CC_XMLBUF_H_INCLUDED 1

#include <e32std.h>
#include <e32base.h>
#include <badesca.h>
#include "xml.h"

#if defined(_UNICODE)
    #define CXmlBuf	CXmlBuf16
#else
    #define CXmlBuf	CXmlBuf8
#endif

/* these classes follow the same conventions as xml.h and expat: namespaced
   names are given in the form uri . KNamespaceSeparator . local name */

class CXmlBuf16 : public CBase {
public:
	IMPORT_C static CXmlBuf16*	NewL(TInt initial_size);
	IMPORT_C void Zero();
	IMPORT_C void SetDefaultNameSpaceL(const TDesC& aNameSpace);
	IMPORT_C void BeginElement(const TDesC& name, 
		const MDesCArray* const attributes=0);
	IMPORT_C void BeginOpenElement(const TDesC& name);
	IMPORT_C void Attribute(const TDesC& name, const TDesC& aValue);
	IMPORT_C void Attribute(const TDesC& name, const TDesC& aNamespace,
		const TDesC& aValue);
	IMPORT_C void CloseOpenElement();
	IMPORT_C void EndElement(const TDesC& name);
	IMPORT_C void EndElement(const TDesC& name, const TDesC& aNamespace);
	IMPORT_C void Characters(const TDesC& buf, bool escape_quote=false);
	IMPORT_C void Leaf(const TDesC& name, const TDesC& buf, const MDesCArray* const attributes=0);
	IMPORT_C const TDesC& Buf() const;
	IMPORT_C ~CXmlBuf16();
	IMPORT_C void Declaration(const TDesC& aEncoding);
protected:
	IMPORT_C CXmlBuf16();
	IMPORT_C void ConstructL(TInt initial_size);

	void AppendL(const TDesC& buf);
	void AppendNameL(const TDesC& aName, TBool aNameBeforeNs);
	void AppendNameL(const TDesC& aName, const TDesC& aNamespace,
		TBool aNameBeforeNs);
	void RemoveNs();

	HBufC		*iBuf, *iDefaultNs;
	TBool		iSetDefault;
	struct TNs {
		TBuf<10>	iPrefix;
		HBufC*		iUrl;
		TInt		iDepth;

		TNs(const TDesC& aPrefix, HBufC* aUrl, TInt aDepth) :
		iPrefix(aPrefix), iUrl(aUrl), iDepth(aDepth) { }
		TNs() : iUrl(0), iDepth(-1) { }
	};
	CArrayFixSeg<TNs> *iPrefixes;
	TInt iDepth;
};

class CXmlBuf8 : public CBase {
public:
	IMPORT_C static CXmlBuf8*	NewL(TInt initial_size);
	IMPORT_C void Zero();
	IMPORT_C void BeginElement(const TDesC8& name, const MDesC8Array* const attributes=0);
	IMPORT_C void EndElement(const TDesC8& name);
	IMPORT_C void Characters(const TDesC8& buf, bool escape_quote=false);
	IMPORT_C void Characters(const TDesC16& buf, bool escape_quote=false);
	IMPORT_C void Leaf(const TDesC8& name, const TDesC8& buf, const MDesC8Array* const attributes=0);
	IMPORT_C void Leaf(const TDesC8& name, const TDesC16& buf, const MDesC8Array* const attributes=0);
	IMPORT_C const TDesC8& Buf() const;
	IMPORT_C ~CXmlBuf8();
	IMPORT_C void Declaration(const TDesC8& aEncoding);
protected:
	IMPORT_C CXmlBuf8();
	IMPORT_C void ConstructL(TInt initial_size);

	void AppendL(const TDesC8& buf);
	void AppendL(const TDesC16& buf);

	HBufC8*		iBuf;
};

#endif
