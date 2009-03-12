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

#include "flickr_rsp.h"
#include "bberrors.h"

class CFlickrResponseImpl : public CFlickrResponse {

	TBool iOk, iDone;
	TInt iErrorCode, iPhotoId;
	HBufC* iErrorMessage;
	HBufC* iXmlErrorMessage;
	~CFlickrResponseImpl() {
		delete iErrorMessage;
		delete iXmlErrorMessage;
	}
	virtual TBool Ok() {
		return iOk;
	}
	virtual TInt ErrorCode() {
		return iErrorCode;
	}
	virtual const TDesC& ErrorMessage() {
		if (iErrorMessage) {
			return *iErrorMessage;
		} else {
			return KNullDesC;
		}
	}
	virtual const TDesC& XmlErrorMessage() {
		if (iXmlErrorMessage) {
			return *iXmlErrorMessage;
		} else {
			return KNullDesC;
		}
	}
	virtual TInt PhotoId() {
		return iPhotoId;
	}
	virtual TBool Done() {
		return iDone;
	}

	TInt iLevel;

	virtual void StartElement(const XML_Char *n,
		const XML_Char **atts) {

		iBuf.Zero();
		TPtrC name((const TUint16*) n);
		++iLevel;

		if (iLevel==1 && name.CompareF(_L("rsp"))==0) {
			const XML_Char *n, *v;
			for (;;) {
				n=*atts; ++atts;
				v=*atts; ++atts;
				if (!n || !v) break;
				TPtrC name((const TUint16*) n); TPtrC value((const TUint16*) v);
				if (name.CompareF(_L("stat"))==0) {
					if (value.CompareF(_L("ok"))==0) {
						iOk=ETrue;
					} else {
						iOk=EFalse;
					}
				}
			}
		}
		if (iLevel==2 && name.CompareF(_L("photoid"))==0) {
			iInPhotoId=ETrue;
		} else {
			iInPhotoId=EFalse;
		}
		if (iLevel==2 && name.CompareF(_L("err"))==0) {
			const XML_Char *n, *v;
			for (;;) {
				n=*atts; ++atts;
				v=*atts; ++atts;
				if (!n || !v) break;
				TPtrC name((const TUint16*) n); TPtrC value((const TUint16*) v);
				if (name.CompareF(_L("code"))==0) {
					TLex l(value);
					l.Val(iErrorCode);
				} else if (name.CompareF(_L("msg"))) {
					delete iErrorMessage; iErrorMessage=0;
					iErrorMessage=value.Alloc();
				}
			}
		}
	}

	TBool iInPhotoId;
	TBuf<30> iBuf;
	virtual void EndElement(const XML_Char *n) {
		TPtrC name((const TUint16*) n);
		--iLevel;
		if (iLevel==0) iDone=ETrue;
		if (iInPhotoId) {
			TLex l(iBuf);
			l.Val(iPhotoId);
		}
		iBuf.Zero();
	}
	virtual void CharacterData(const XML_Char *s,
		int len) {

		if (iInPhotoId) {
			if (len > iBuf.MaxLength()-iBuf.Length()) len=iBuf.MaxLength()-iBuf.Length();
			TPtrC data((const TUint16*) s, len);
			iBuf.Append(data);
		}
	}
	virtual void Error(XML_Error Code, const XML_LChar * String, long ByteIndex) {
		iDone=ETrue;
		iOk=EFalse;
		iErrorCode=KErrInvalidXml;
		delete iErrorMessage; iErrorMessage=0;
		delete iXmlErrorMessage; iXmlErrorMessage=0;
		iXmlErrorMessage=TPtrC( (const TUint16*) XML_ErrorString(Code)).Alloc();
		iErrorMessage=_L("Invalid response received from flickr").Alloc();
	}
	virtual TBool IsPermanent() {
		switch(iErrorCode) {
		case 6:
		case 96:
		case 97:
		case 98:
		case 99:
		case 100:
			return ETrue;
		}
		return EFalse;
	}

	friend class CFlickrResponse;
};

CFlickrResponse* CFlickrResponse::NewL()
{
	return new (ELeave) CFlickrResponseImpl;
}
