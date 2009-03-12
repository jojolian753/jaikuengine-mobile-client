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

#include "jabber.h"
#include "symbian_auto_ptr.h"

CXmlParser* CXmlParser::NewL(MXmlHandler& Handler)
{
	auto_ptr<CXmlParser> ret(new (ELeave) CXmlParser(Handler));
	ret->ConstructL();
	return ret.release();
}

void CXmlParser::Parse(const char* buf, int len, int isFinal)
{
	if (!XML_Parse(iParser, buf, len, isFinal)) {
		XML_Error code=XML_GetErrorCode(iParser);
		iHandler.Error(code, XML_ErrorString(code), XML_GetCurrentByteIndex(iParser));
	}
}

CXmlParser::~CXmlParser()
{
	XML_ParserFree(iParser);
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

CXmlParser::CXmlParser(MXmlHandler& Handler) : iHandler(Handler)
{
}

void CXmlParser::ConstructL()
{
	iParser=XML_ParserCreate(0);
	XML_SetElementHandler(iParser, StartElement, EndElement);
	XML_SetCharacterDataHandler(iParser, CharacterData);
	XML_SetUserData(iParser, &iHandler);
}

CJabber* CJabber::NewL(MApp_context& Context, MJabberObserver& Observer)
{
	auto_ptr<CJabber> ret(new (ELeave) CJabber(Context, Observer));
	ret->ConstructL();
	return ret.release();
}

void CJabber::Connect(const TDesC& Host, const TDesC& UserId, const TDesC& Resource,
	const TDesC& Pass)
{
	DESTROY(iUserId); DESTROY(iResource); DESTROY(iPass);
	iUserId=UserId.AllocL(); iResource=Resource.AllocL(); iPass=Pass.AllocL();
	CSocketBase::Connect(Host, 5222, 120);
}

void CJabber::Send(const TDesC8& buf, int timeout, bool Internal)
{
	CancelRead();

	if (current_state!=IDLE) {
		User::Leave(7);
	}
	socket.Write(buf, iStatus);
	current_state=SENDING;
	timer->Wait(timeout);
	iSendIsInternal=Internal;

	SetActive();
}

auto_ptr<HBufC8> CJabber::ConvertAllocL(const TDesC& str)
{
	auto_ptr<HBufC8> desc8(HBufC8::NewL(str.Length()+1));
	TPtr8 ptr8(desc8->Des());
	CC()->ConvertFromUnicode( ptr8, str);
	return desc8;
}

void CJabber::SendPresence(const TDesC& Description)
{
	_LIT8(KFormat, "<presence type='available'><status>%S</status>");
	auto_ptr<HBufC8> desc8(ConvertAllocL(Description));
	
	auto_ptr<HBufC8> buf(HBufC8::NewL(Description.Length()+KFormat().Length()));
	buf->Des().Format(KFormat, &(desc8->Des()));
	Send(buf->Des(), 60, false);
}
	
void CJabber::Close()
{
	Send(_L8("</stream>"), 120, true);
}

CJabber::~CJabber()
{
	Cancel();
	DESTROY(iUserId); DESTROY(iResource); DESTROY(iPass);
	DESTROY(iParser);
	DESTROY(iParseState);
	if (iHost==iAlias) iAlias=0;
	DESTROY(iHost); DESTROY(iAlias);
}

CJabber::CJabber(MApp_context& Context, MJabberObserver& Observer) : 
	CSocketBase(Observer), MContextBase(Context), iObserver(Observer)
{
}

void CJabber::ConstructL()
{
	CSocketBase::ConstructL(256);
	iParser=CXmlParser::NewL(*this);
	iParseState=CList<EParseState>::NewL();
}

bool CJabber::DoRunL()
{
	if (iStatus!=KErrNone) {
		return CSocketBase::DoRunL();
	}

	switch(current_state) {
	case SENDING:
		if (!iSendIsInternal) iObserver.success(this);
		current_state=IDLE;
		read();
		break;
	case RECEIVING:
		iCallBack=0;
		iInParse=true;
		iParser->Parse((const char*)readbuf_data, read_len(), 0);
		iInParse=false;
		current_state=IDLE;
		if (iCallBack) ((*this).*iCallBack)();
		else read();
		break;
	case CONNECTING:
		OpenStream();
		break;
	default:
		return CSocketBase::DoRunL();
	}
	return true;
}

void CJabber::OpenStream()
{
	_LIT8(KFormat, 
		"<?xml version='1.0' encoding='iso-8859-1'?>"
		"<stream:stream xmlns:stream='http://etherx.jabber.org/streams' "
		"to='%S' xmlns='jabber:client'>");
	auto_ptr<HBufC8> to(ConvertAllocL(*iHost));
	auto_ptr<HBufC8> buf(HBufC8::NewL( KFormat().Length()+to->Length() ));
	buf->Des().Format(KFormat, &(to->Des()));
	Send(*buf, 60, true);
	iAlias=iHost;
}

TPtrC CJabber::GetAttributeValue(const XML_Char **atts, const TDesC& Name)
{
	int attr=0, value=1;
	while (atts[attr]) {
		TPtrC attrD(atts[attr]);
		TPtrC valueD(atts[value]);
		if (attrD.Find(Name)!=KErrNone) {
			return valueD;
		}
		attr+=2;
		value+=2;
	}
	return TPtrC(0, 0);
}

void CJabber::SendAuthentication()
{
	_LIT8(KFormat, 
		"<?xml version='1.0' encoding='iso-8859-1'?>"
		"<stream:stream xmlns:stream='http://etherx.jabber.org/streams' "
		"to='%S' xmlns='jabber:client'>");
}

void CJabber::GetAuthInfo()
{
	_LIT8(KFormat,
		"<iq id='m%d' type='get'>"
		"<query xmlns='jabber:iq:auth'>"
		"<username>%S</username>"
		"</query></iq>");
	auto_ptr<HBufC8> buf(HBufC8::NewL( KFormat().Length()+iUserId->Length()+6 ));
	buf->Des().Format(KFormat, iMsgId++, &(iUserId->Des()));
	Send(*buf, 60, true);
	iAuthMethod=0;
	iSessState=SESS_GETTING_AUTH;
}

void CJabber::StartElement(const XML_Char *name,
			const XML_Char **atts)
{
	TPtrC nameD(name);

	EParseState next_state=PARSE_IGNORE;

	switch(iSessState) {
	case SESS_IDLE:
		inconsistent(_L("Elements when idle"));
		break;
	case SESS_OPENING:
		switch(iParseState->Top()) {
		case PARSE_IDLE:
			if ( nameD.Find(_L("stream"))!=KErrNotFound ) {
				next_state=PARSE_STREAM;
				TPtrC fromD=GetAttributeValue(atts, _L("from"));
				if (fromD.Length()>0) {
					iAlias=fromD.AllocL();
				}
				iCallBack=GetAuthInfo;
			}
			break;
		default:
			inconsistent(_L("Toplevel element not stream"));
			break;
		}
	case SESS_GETTING_AUTH:
		switch (iParseState->Top()) {
		case PARSE_STREAM:
			if (nameD.Find(_L("iq"))!=KErrNotFound) {
				next_state=PARSE_IQ;
			} else {
				inconsistent(_L("Non-iq element after auth request"));
			}
			break;
		case PARSE_IQ:
			if (nameD.Find(_L("password"))!=KErrNone) {
				iAuthMethod=iAuthMethod | AUTH_PASSWORD;
			} else if (nameD.Find(_L("digest"))!=KErrNone) {
				iAuthMethod=iAuthMethod | AUTH_DIGEST;
			} else if (nameD.Find(_L("sequence"))!=KErrNone) {
				iAuthMethod=iAuthMethod | AUTH_DIGEST;
			} else if (nameD.Find(_L("token"))!=KErrNone) {
				next_state=PARSE_TOKEN;
			}
			break;
		}
	}
}

void CJabber::EndElement(const XML_Char *name)
{
	TCallBack	cb=0;

	switch(iParseState->Pop()) {
	case PARSE_IQ:
		switch(iSessState) {
		case SESS_GETTING_AUTH:
			cb=SendAuthentication;
			break;
		default:
			inconsistent(_L("unexpected iq end"));
			break;
		}
	default:
		break;
	}
	if (cb) {
		if (iCallBack) {
			inconsistent(_L("callback already set"));
		} else {
			iCallBack=cb;
		}
	}
}

void CJabber::AppendL(HBufC*& Into, const TDesC& String)
{
	if (!Into) {
		Into=String.AllocL();
	} else {
		TInt len=Into->Des().Length()+String.Length();
		if (Into->Des().MaxLength() < len ) {
			auto_ptr<HBufC> tmp(HBufC::NewL(len));
			tmp->Des().Append(*Into);
			tmp->Des().Append(String);
			delete Into;
			Into=tmp.release();
		} else {
			Into->Des().Append(String);
		}
	}
}

void CJabber::CharacterData(const XML_Char *s,
			    int len)
{
	switch (iParseState->Top()) {
	case PARSE_TOKEN:
		AppendL(iAuthToken, TPtrC(s, len));
		break;
	default:
		break;
	}
}

void CJabber::Error(XML_Error Code, const XML_LChar * String, long ByteIndex)
{
}
