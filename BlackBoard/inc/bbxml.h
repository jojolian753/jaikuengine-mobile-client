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

#ifndef BBXML_H_INCLUDED 
#define BBXML_H_INCLUDED 1

#include "bbdata.h"
#include "concretedata.h"

class CXmlBufExternalizer : public CXmlBuf16, public MBBExternalizer {
public:
	IMPORT_C static CXmlBufExternalizer* NewL(TInt aInitialSize);
};

class TIgnoreXml : public MNestedXmlHandler {
public:
	IMPORT_C TIgnoreXml(MNestedXmlHandler* aParent, CXmlParser* aParser);
	virtual void SetError(TInt aError);
	virtual void StartElement(const XML_Char *name,
				const XML_Char **atts);

	virtual void EndElement(const XML_Char *name);
	virtual void CharacterData(const XML_Char *s,
				    int len);
	virtual void Error(XML_Error Code, const XML_LChar * String, long ByteIndex);

private:
	TUint	iDepth;
	CXmlParser* iParser;
};

IMPORT_C TPtrC RemoveContextNamespace(const XML_Char* name);

class TCheckingXml : public MNestedXmlHandler {
private:
	IMPORT_C virtual void StartElement(const XML_Char *name,
				const XML_Char **atts);
	IMPORT_C virtual void EndElement(const XML_Char *name);

	IMPORT_C virtual void CharacterData(const XML_Char *s,
				    int len);
protected:
	IMPORT_C TCheckingXml(MNestedXmlHandler* aParent, CXmlParser* aParser);

	IMPORT_C virtual void ErrorInHandler(TInt aError);

	IMPORT_C virtual void StartElementL(const XML_Char *name,
				const XML_Char **atts) = 0;
	IMPORT_C virtual void EndElementL(const XML_Char *name) = 0;

	IMPORT_C virtual void CharacterDataL(const XML_Char *s,
				    int len) = 0;

	TIgnoreXml	iIgnore; TBool iIgnoreElement;
	CXmlParser*	iParser;
};

class TBBXml : public TCheckingXml {
protected:
	IMPORT_C TBBXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, MBBData& aValue, TBool aCheckType);

	HBufC*&		iBuf;
	MBBData&	iValue;
	TBool		iCheckType;
};

class TSingleXml : public TBBXml {
public:
	IMPORT_C TSingleXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, MBBData& aValue, TBool aCheckType);
protected:
	IMPORT_C virtual void StartElementL(const XML_Char *name,
				const XML_Char **atts);
	IMPORT_C virtual void EndElementL(const XML_Char *name);

	IMPORT_C virtual void CharacterDataL(const XML_Char *s,
				    int len);

	IMPORT_C virtual void Error(XML_Error Code, const XML_LChar * String, long ByteIndex);
	IMPORT_C virtual void SetError(TInt aError);

};

class CContainerXml : public TBBXml, public CBase
{
protected:
	IMPORT_C CContainerXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, MBBData& aValue, TBool aCheckType);
	IMPORT_C ~CContainerXml();
protected:
	IMPORT_C virtual void StartElementL(const XML_Char *name,
				const XML_Char **atts);

	IMPORT_C virtual void HandleAttributesL(const XML_Char *name, const XML_Char **atts);
	virtual void StartInnerElementL(const XML_Char *name, const XML_Char **atts) = 0;
	virtual void EndInnerElementL(const XML_Char *name) = 0;

	IMPORT_C virtual void CharacterDataL(const XML_Char *s,
				    int len);
	IMPORT_C virtual void EndElementL(const XML_Char *s);
	IMPORT_C void EndElement(const XML_Char * name);

	IMPORT_C void SetCurrentHandler(MNestedXmlHandler* aHandler); // takes ownership
	IMPORT_C MNestedXmlHandler* GetCurrentHandler();
private:
	TInt	iDepth;
	MNestedXmlHandler* iCurrentHandler;
};

class CPartContainerXml : public CContainerXml
{
public:
	IMPORT_C CPartContainerXml(MNestedXmlHandler* aParent, CXmlParser* aParser,
		HBufC*& aBuf, TBBCompoundData& aValue, TBool aCheckType);
private:
	IMPORT_C virtual void HandleAttributesL(const XML_Char *name, const XML_Char **atts);
	IMPORT_C virtual void StartInnerElementL(const XML_Char *name, const XML_Char **atts);
	IMPORT_C virtual void EndInnerElementL(const XML_Char *name);

	IMPORT_C virtual void Error(XML_Error Code, const XML_LChar * String, long ByteIndex);
	IMPORT_C virtual void SetError(TInt aError);

	TBBCompoundData& iCompoundValue;
	TInt	iCurrentPart; TBool iErrorsInCurrent;
};

class CSingleParser : public CBase {
public:
	// if you give this a null pointer, it will create 
	// an object based on the type of the root element
	// and keep ownership of it. in that case you need to give
	// it a factory instance
	IMPORT_C static CSingleParser* NewL(MBBData* aParseInto, TBool  aCheckType,
		TBool aIgnoreUnknown, MBBDataFactory* aFactory=0);
	virtual void ParseL(const TDesC8& aXml) = 0;
	// gives you either back your own object or the owned one
	virtual MBBData* Data() = 0;
};

#endif
