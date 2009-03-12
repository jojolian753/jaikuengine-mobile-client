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

#include "xml.h"
#include "symbian_auto_ptr.h"

/*
 * Concepts:
 * !Using expat!
 */



EXPORT_C CXmlParser* CXmlParser::NewL(MXmlHandler& Handler, TBool aDoNamespaces)
{
	auto_ptr<CXmlParser> ret(new (ELeave) CXmlParser(Handler));
	ret->ConstructL(aDoNamespaces);
	return ret.release();
}

EXPORT_C void CXmlParser::Parse(const char* buf, int len, int isFinal)
{
	if (!XML_Parse(iParser, buf, len, isFinal)) {
		XML_Error code=XML_GetErrorCode(iParser);
		if (code==XML_ERROR_NO_MEMORY) User::Leave(KErrNoMemory);

		iHandlerPtr->Error(code, XML_ErrorString(code), XML_GetCurrentByteIndex(iParser));
	}
}

EXPORT_C void CXmlParser::SetHandler(MXmlHandler* aHandler)
{
	XML_SetUserData(iParser, aHandler);
	iHandlerPtr=aHandler;
}

CXmlParser::~CXmlParser()
{
	if (iParser) XML_ParserFree(iParser);
}

void CXmlParser::StartElement(void *userData,
			   const XML_Char *name,
			   const XML_Char **atts)
{
	MXmlHandler* h=(MXmlHandler*)userData;
	h->StartElement(name, atts);
}

void CXmlParser::EndElement(void *userData,
			 const XML_Char *name)
{
	MXmlHandler* h=(MXmlHandler*)userData;
	h->EndElement(name);
}

void CXmlParser::CharacterData(void *userData,
			    const XML_Char *s,
			    int len)
{
	MXmlHandler* h=(MXmlHandler*)userData;
	h->CharacterData(s, len);
}

CXmlParser::CXmlParser(MXmlHandler& Handler) : iHandler(Handler), iHandlerPtr(&iHandler)
{
}


void* own_malloc(size_t size)
{
	void* p=User::Alloc(size);
#ifdef __WINS__
	if (!p) {
		TInt x;
		x=0;
	}
#endif
	return p;
}

void* own_realloc(void *ptr, size_t size)
{
	void* p=User::ReAlloc(ptr, size);
#ifdef __WINS__
	if (!p) {
		TInt x;
		x=0;
	}
#endif
	return p;
}

void own_free(void *ptr)
{
	User::Free(ptr);
}

void CXmlParser::ConstructL(TBool aDoNamespaces)
{
	iMemSuite.malloc_fcn=own_malloc;
	iMemSuite.realloc_fcn=own_realloc;
	iMemSuite.free_fcn = own_free;

	if (aDoNamespaces) {
		iParser=XML_ParserCreate_MM(0, &iMemSuite, 0);
	} else {
		iSep.Append(KNamespaceSeparator);
		iParser=XML_ParserCreate_MM(0, &iMemSuite, (XML_Char*)iSep.Ptr());
	}
	if (!iParser) User::Leave(KErrNoMemory);

	XML_SetElementHandler(iParser, StartElement, EndElement);
	XML_SetCharacterDataHandler(iParser, CharacterData);
	XML_SetUserData(iParser, &iHandler);
}

EXPORT_C void SeparateNamespace(const XML_Char* name, TPtrC& local, TPtrC& uri)
{
	TPtrC namep( (const TUint16*)name);
	SeparateNamespace(namep, local, uri);
}

EXPORT_C void SeparateNamespace(const TDesC& namep, TPtrC& local, TPtrC& uri)
{
	TInt sep=namep.Locate(KNamespaceSeparator);
	if (sep==KErrNotFound) {
		uri.Set(0, 0);
		local.Set(namep);
	} else {
		uri.Set( namep.Left(sep) );
		local.Set( namep.Mid(sep+1) );
	}
}
