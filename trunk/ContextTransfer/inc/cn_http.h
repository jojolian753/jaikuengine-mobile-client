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

#ifndef CONTEXT_HTTP_H_INCLUDED
#define CONTEXT_HTTP_H_INCLUDED 1

#include "socketsengine.h"
#include "app_context.h"
#include "raii_f32file.h"
#include "list.h"

class CHttpHeader : public CBase
{
public:
	IMPORT_C static CHttpHeader * NewL();
	IMPORT_C static CHttpHeader * NewLC();
	IMPORT_C void Copy(const CHttpHeader &aHeader);
	IMPORT_C void Reset();

private:
	CHttpHeader () {}
	void ConstructL();

public:
	TReal iHttpVersion;
	TInt iHttpReplyCode;
	TBuf<KMaxServerNameLength> iServername;
    TTime iServerTime;
	TBuf<256> iFilename;
	TBuf<256> iLocation;
	TTime iLastMod;
	TBuf<128> iContentType;
	TInt iSize;
	TInt iChunkStart;
	TInt iChunkEnd;

	//TBuf<50> iETag;
	//TBuf<200> iServerDescription;
	//TBuf<200> iMime;
};

class CPostPart : public CBase
{
public:
	IMPORT_C CPostPart(const TDesC& aName, const TDesC& aMimeType);
	IMPORT_C void SetFileName(const TDesC& aFileName); // sets the filename used in the post
	virtual TInt	Size() = 0;
	TBuf<128>	iName;
	TBuf<128>	iMimeType;
	TBuf<128>	iFileName;
	IMPORT_C virtual const TDesC&  FileName();

	virtual void ReadChunkL(TDes8& aIntoBuffer) = 0; // zero length at EOF
};

class CFilePart : public CPostPart
{
public:
	IMPORT_C static CFilePart* NewL(RFs& aFs, const TDesC& aFileName, 
		const TDesC& aName, const TDesC& aMimeType);
	IMPORT_C ~CFilePart();
private:
	CFilePart(const TDesC& aName, const TDesC& aMimeType);
	void ConstructL(RFs& aFs, const TDesC& aFileName);
	RFile	iFile; TBool iFileIsOpen;
	TInt	iSize;
	TInt	iRead;
	virtual TInt	Size();
	virtual void ReadChunkL(TDes8& aIntoBuffer);
};

class CBufferPart : public CPostPart
{
public:
	IMPORT_C static CBufferPart* NewL(HBufC8* aBuffer, TBool aTakeOwnership,
		const TDesC& aName, const TDesC& aMimeType);
	IMPORT_C static CBufferPart* NewL(const TDesC8& aBuffer,
		const TDesC& aName, const TDesC& aMimeType);
	IMPORT_C ~CBufferPart();
private:
	CBufferPart(const TDesC& aName, const TDesC& aMimeType);
	void ConstructL(const TDesC8& aBuffer);
	void ConstructL(HBufC8* aBuffer, TBool aTakeOwnership);
	virtual TInt	Size();
	virtual void ReadChunkL(TDes8& aIntoBuffer);
	HBufC8*	iBuffer;
	TInt	iPos;
	TInt	iRead;
	TBool	iOwnsBuffer;
};

class MHttpObserver 
{
public:
	enum THttpStatus
        {
		EHttpConnected=0,
		EHttpDisconnected=1,
		EHttpError=2,
		EHttpSentAll=3
	};

	virtual void NotifyHttpStatus(THttpStatus st, TInt aError)=0;
	virtual void NotifyNewHeader(const CHttpHeader &aHeader)=0;
	virtual void NotifyNewBody(const TDesC8 &chunk)=0;
};


class CHttp : public CBase
{
public:
	IMPORT_C static CHttp * NewL(MHttpObserver& aObserver, MApp_context& Context, const TDesC& aConnectionName=KNullDesC);

public:
	virtual void GetL(const TUint& iAP, const TDesC &url, const TTime &modified_since = TTime(0), 
		const TUint &chunkStart=0, const TUint &chunkEnd=0) = 0;
	
	virtual void PostL(const TUint& iAP, const TDesC &url, CPtrList<CPostPart>* aBodyParts) = 0;
		// guarantees to take ownership of parts even if Leaves

	//void HeadL(const TDesC &url, const TTime &modified_since);
	virtual void ReleaseParts() = 0;
	virtual void Disconnect() = 0;

	IMPORT_C static void AppendUrlEncoded(TDes& into, const TDesC& str);
};

typedef CHttp* (*HttpFactory)(MHttpObserver& aObserver, MApp_context& Context, const TDesC& aConnectionName);

#endif
