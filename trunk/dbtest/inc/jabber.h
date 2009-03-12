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

#ifndef JABBER_H_INCLUDED
#define JABBER_H_INCLUDED 1

#include "app_context.h"
#include "ftp.h"
#include <expat_config.h>
#include <expat.h>
#include "list.h"

class MXmlHandler {
public:
	virtual void StartElement(const XML_Char *name,
				const XML_Char **atts) = 0;

	virtual void EndElement(const XML_Char *name) = 0;
	virtual void CharacterData(const XML_Char *s,
				    int len) = 0;
	virtual void Error(XML_Error Code, const XML_LChar * String, long ByteIndex) = 0;
};

class CXmlParser : public CBase {
public:
	static CXmlParser* NewL(MXmlHandler& Handler);
	void Parse(const char*, int len, int isFinal);
	~CXmlParser();
private:
	static void StartElement(void *userData,
				   const XML_Char *name,
				   const XML_Char **atts);

	static void EndElement(void *userData,
				 const XML_Char *name);
	static void CharacterData(void *userData,
				    const XML_Char *s,
				    int len);

	CXmlParser(MXmlHandler& Handler);
	void ConstructL();

	MXmlHandler&	iHandler;
	XML_Parser	iParser;
};

class MJabberObserver : public MSocketObserver {
public:
	virtual void Presence(CBase* Source, const TDesC& User, 
		const TDesC& Resource, const TDesC& Description);
};

class CJabber : public MXmlHandler, public CSocketBase, public MContextBase {
public:
	static CJabber* NewL(MApp_context& Context, MJabberObserver& Observer);
	void Connect(const TDesC& Host, const TDesC& UserId, const TDesC& Resource,
		const TDesC& Pass);
	void SendPresence(const TDesC& Description);
	void Close();
	~CJabber();
private:
	CJabber(MApp_context& Context, MJabberObserver& Observer);
	void ConstructL();
	virtual bool DoRunL();
	void Send(const TDesC8& buf, int timeout, bool Internal);
	void OpenStream();

	auto_ptr<HBufC8> ConvertAllocL(const TDesC& str);
	TPtrC GetAttributeValue(const XML_Char **atts, const TDesC& Name);
	void AppendL(HBufC*& Into, const TDesC& String);
	
	enum EJabberAuth { AUTH_PASSWORD=0x1, AUTH_DIGEST=0x01, AUTH_SEQUENCE=0x001 };
	TInt		iAuthMethod;
	HBufC*		iAuthToken;
	void GetAuthInfo();
	void SendAuthentication();
	TInt		iMsgId;

	virtual void StartElement(const XML_Char *name,
				const XML_Char **atts);

	virtual void EndElement(const XML_Char *name);
	virtual void CharacterData(const XML_Char *s,
				    int len);
	virtual void Error(XML_Error Code, const XML_LChar * String, long ByteIndex);

	enum ESessState { SESS_IDLE, SESS_OPENING, SESS_GETTING_AUTH, SESS_LOGGING_IN, SESS_LOGGED_IN };
	ESessState iSessState;
	enum EParseState { PARSE_IDLE, PARSE_IGNORE, PARSE_STREAM, 
		PARSE_IQ, PARSE_TOKEN, PARSE_PRESENCE, 
		PARSE_PRESENCE_STATUS, PARSE_MESSAGE };
	bool iInParse;
	typedef void (CJabber::*TCallBack)(void);
	TCallBack	iCallBack;

	CList<EParseState> *iParseState;

	HBufC	*iAlias, *iUserId, *iResource, *iPass;
	MJabberObserver&	iObserver;
	bool		iSendIsInternal;
	CXmlParser	*iParser;
};

#endif
