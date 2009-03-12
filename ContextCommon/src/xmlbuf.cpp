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

#include "xmlbuf.h"
#include "symbian_auto_ptr.h"
#include <e32svr.h>

EXPORT_C CXmlBuf16* CXmlBuf16::NewL(TInt initial_size)
{
	auto_ptr<CXmlBuf16> ret(new (ELeave) CXmlBuf16);
	ret->ConstructL(initial_size);
	return ret.release();
}

EXPORT_C void CXmlBuf16::BeginElement(const TDesC& name,
	const MDesCArray* const attributes)
{
	BeginOpenElement(name);
	if (attributes) {
		for (int i=0; i<attributes->MdcaCount(); i++) {
			AppendL(_L(" "));
			AppendNameL(attributes->MdcaPoint(i++), EFalse);
			AppendL(_L("=\""));
			Characters(attributes->MdcaPoint(i), true);
			AppendL(_L("\""));
		}
	}
	AppendL(_L(">"));
}

EXPORT_C void CXmlBuf16::BeginOpenElement(const TDesC& name)
{
	iDepth++;
	AppendL(_L("<"));
	AppendNameL(name, ETrue);
	if (iDefaultNs && iDefaultNs->Des().Length()>0 && !iSetDefault) {
		AppendL(_L(" xmlns=\""));
		Characters(*iDefaultNs);
		AppendL(_L("\""));
		iSetDefault=ETrue;
	}
}

EXPORT_C void CXmlBuf16::SetDefaultNameSpaceL(const TDesC& aNameSpace)
{
	if (!iDefaultNs || iDefaultNs->Des().MaxLength()<aNameSpace.Length()) {
		delete iDefaultNs; iDefaultNs=0;
		iDefaultNs=aNameSpace.AllocL();
	} else {
		iDefaultNs->Des()=aNameSpace;
	}
}

EXPORT_C void CXmlBuf16::Attribute(const TDesC& name, const TDesC& aValue)
{
	AppendL(_L(" "));
	AppendNameL(name, EFalse);
	AppendL(_L("=\""));
	Characters(aValue, true);
	AppendL(_L("\""));
}
EXPORT_C void CXmlBuf16::CloseOpenElement()
{
	AppendL(_L(">"));
}

void CXmlBuf16::RemoveNs()
{
	if (iPrefixes==0) return;
	for (TInt i=iPrefixes->Count()-1; i>=0; i--) {
		if (iPrefixes->At(i).iDepth>iDepth) {
			delete iPrefixes->At(i).iUrl;
			iPrefixes->Delete(i);
		} else {
			break;
		}
	}
}

EXPORT_C void CXmlBuf16::EndElement(const TDesC& name)
{
	AppendL(_L("</"));
	AppendNameL(name, EFalse);
	AppendL(_L(">"));
	iDepth--;
	RemoveNs();
}

EXPORT_C void CXmlBuf16::EndElement(const TDesC& name, const TDesC& aNamespace)
{
	AppendL(_L("</"));
	AppendNameL(name, aNamespace, EFalse);
	AppendL(_L(">"));
	iDepth--;
	RemoveNs();
}

EXPORT_C void CXmlBuf16::Characters(const TDesC& buf, bool escape_quote)
{
	for (int i=0; i<buf.Length(); i++) {
		if (buf[i]=='<') {
			AppendL(_L("&lt;"));
		} else if (buf[i]=='&') {
			AppendL(_L("&amp;"));
		} else if (buf[i]=='"' && escape_quote) {
			AppendL(_L("&quot;"));
		} else {
			AppendL( TPtrC( &(buf[i]), 1) );
		}
	}
}

EXPORT_C void CXmlBuf16::Leaf(const TDesC& name, const TDesC& buf, const MDesCArray* const attributes)
{
	BeginElement(name, attributes);
	Characters(buf);
	EndElement(name);
}

EXPORT_C CXmlBuf16::CXmlBuf16() 
{
}

EXPORT_C void CXmlBuf16::ConstructL(TInt initial_size)
{
	iBuf=HBufC::NewL(initial_size);
}

void CXmlBuf16::AppendNameL(const TDesC& n, const TDesC& uri,
	TBool aNameBeforeNs)
{
	if (iPrefixes==0) iPrefixes=new (ELeave) CArrayFixSeg<TNs>(4);
	TBuf<10> prefix;
	TBool found=EFalse;
	for (int i=0; i<iPrefixes->Count(); i++) {
		if (iPrefixes->At(i).iUrl->Des().Compare(uri)==0) {
			prefix=iPrefixes->At(i).iPrefix;
			found=ETrue;
			break;
		}
	}
	if (!found) {
		prefix=_L("ns");
		prefix.AppendNum(iPrefixes->Count());
		auto_ptr<HBufC> u(uri.AllocL());
		iPrefixes->AppendL( TNs(prefix, u.get(), iDepth) );
		u.release();
	} else {
		AppendL(prefix);
		AppendL(_L(":"));
		AppendL(n);
		return;
	}
	if (aNameBeforeNs) {
		AppendL(prefix);
		AppendL(_L(":"));
		AppendL(n);
		AppendL(_L(" xmlns:"));
		AppendL(prefix);
		AppendL(_L("=\""));
		Characters(uri, ETrue);
		AppendL(_L("\""));
	} else {
		AppendL(_L(" xmlns:"));
		AppendL(prefix);
		AppendL(_L("=\""));
		Characters(uri, ETrue);
		AppendL(_L("\" "));
		AppendL(prefix);
		AppendL(_L(":"));
		AppendL(n);
	}
}

void CXmlBuf16::AppendNameL(const TDesC& aName, TBool aNameBeforeNs)
{
	TPtrC n, uri;
	SeparateNamespace(aName, n, uri);
	if (uri.Length()==0) {
		AppendL(aName);
		return;
	}
#ifdef __WINS__
	TBuf<100> msg=_L("NAME: |");
	msg.Append(aName); 
	msg.Append(_L("|"));
	RDebug::Print(msg);
	SeparateNamespace(aName, n, uri);
#endif
	AppendNameL(n, uri, aNameBeforeNs);
}

void CXmlBuf16::AppendL(const TDesC& buf)
{
	while (iBuf->Des().Length()+buf.Length() > iBuf->Des().MaxLength()) {
		iBuf=iBuf->ReAllocL(iBuf->Des().MaxLength()*2);
	}
	iBuf->Des().Append(buf);
}

EXPORT_C const TDesC& CXmlBuf16::Buf() const
{
	return *iBuf;
}

EXPORT_C void CXmlBuf16::Zero()
{
	iSetDefault=EFalse;
	iBuf->Des().Zero();
}

EXPORT_C CXmlBuf16::~CXmlBuf16()
{
	delete iBuf;
	delete iDefaultNs;
	if (iPrefixes) {
		for (int i=0; i<iPrefixes->Count(); i++) {
			delete iPrefixes->At(i).iUrl;
		}
	}
	delete iPrefixes;
}

EXPORT_C CXmlBuf8* CXmlBuf8::NewL(TInt initial_size)
{
	auto_ptr<CXmlBuf8> ret(new (ELeave) CXmlBuf8);
	ret->ConstructL(initial_size);
	return ret.release();
}

EXPORT_C void CXmlBuf8::BeginElement(const TDesC8& name, const MDesC8Array* const attributes)
{
	AppendL(_L8("<"));
	AppendL(name);
	if (attributes) {
		for (int i=0; i<attributes->MdcaCount(); i++) {
			AppendL(_L8(" "));
			AppendL(attributes->MdcaPoint(i++));
			AppendL(_L8("=\""));
			Characters(attributes->MdcaPoint(i), true);
			AppendL(_L8("\""));
		}
	}
	AppendL(_L8(">"));
}


EXPORT_C void CXmlBuf8::EndElement(const TDesC8& name)
{
	AppendL(_L8("</"));
	AppendL(name);
	AppendL(_L8(">"));
}

EXPORT_C void CXmlBuf8::Characters(const TDesC8& buf, bool escape_quote)
{
	for (int i=0; i<buf.Length(); i++) {
		if (buf[i]=='<') {
			AppendL(_L8("&lt;"));
		} else if (buf[i]=='&') {
			AppendL(_L8("&amp;"));
		} else if (buf[i]=='"' && escape_quote) {
			AppendL(_L8("&quot;"));
		} else {
			AppendL( TPtrC8( &(buf[i]), 1) );
		}
	}
}

EXPORT_C void CXmlBuf8::Characters(const TDesC16& buf, bool escape_quote)
{
	for (int i=0; i<buf.Length(); i++) {
		if (buf[i]=='<') {
			AppendL(_L8("&lt;"));
		} else if (buf[i]=='&') {
			AppendL(_L8("&amp;"));
		} else if (buf[i]=='"' && escape_quote) {
			AppendL(_L8("&quot;"));
		} else {
			AppendL( buf.Mid(i, 1));
		}
	}
}

EXPORT_C void CXmlBuf8::Leaf(const TDesC8& name, const TDesC16& buf, const MDesC8Array* const attributes)
{
	BeginElement(name, attributes);
	Characters(buf);
	EndElement(name);
}

EXPORT_C void CXmlBuf8::Leaf(const TDesC8& name, const TDesC8& buf, const MDesC8Array* const attributes)
{
	BeginElement(name, attributes);
	Characters(buf);
	EndElement(name);
}

EXPORT_C CXmlBuf8::CXmlBuf8() 
{
}

EXPORT_C void CXmlBuf8::ConstructL(TInt initial_size)
{
	iBuf=HBufC8::NewL(initial_size);
}

void CXmlBuf8::AppendL(const TDesC8& buf)
{
	while (iBuf->Des().Length()+buf.Length() > iBuf->Des().MaxLength()) {
		iBuf=iBuf->ReAllocL(iBuf->Des().MaxLength()*2);
	}
	iBuf->Des().Append(buf);
}

void CXmlBuf8::AppendL(const TDesC16& buf)
{
	while (iBuf->Des().Length()+buf.Length() > iBuf->Des().MaxLength()) {
		iBuf=iBuf->ReAllocL(iBuf->Des().MaxLength()*2);
	}
	iBuf->Des().Append(buf);
}

EXPORT_C const TDesC8& CXmlBuf8::Buf() const
{
	return *iBuf;
}

EXPORT_C void CXmlBuf8::Zero()
{
	iBuf->Des().Zero();
}

EXPORT_C CXmlBuf8::~CXmlBuf8()
{
	delete iBuf;
}

EXPORT_C void CXmlBuf16::Declaration(const TDesC& aEncoding)
{
	TBuf<100> enc;
	enc=_L("<?xml version='1.0'");
	if (aEncoding.Length()>0) {
		enc.Append(_L(" encoding='"));
		enc.Append(aEncoding);
		enc.Append(_L("'"));
	}
	enc.Append(_L("?>"));
	AppendL(enc);
}

EXPORT_C void CXmlBuf8::Declaration(const TDesC8& aEncoding)
{
	TBuf8<100> enc;
	enc=_L8("<?xml version='1.0'");
	if (aEncoding.Length()>0) {
		enc.Append(_L8(" encoding='"));
		enc.Append(aEncoding);
		enc.Append(_L8("'"));
	}
	enc.Append(_L8("?>"));
	AppendL(enc);
}
