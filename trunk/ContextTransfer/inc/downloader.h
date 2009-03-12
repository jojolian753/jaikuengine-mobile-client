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

#ifndef CONTEXT_DOWNLOADER_H_INCLUDED
#define CONTEXT_DOWNLOADER_H_INCLUDED 1

#include "app_context.h"
#include "db.h"
#include "cn_http.h"


class MDownloadObserver {
public:
	virtual void DownloadFinished(TInt64 aRequestId, const TDesC& aFileName,
		const TDesC& aContentType) = 0;
	virtual void DownloadError(TInt64 aRequestId, TInt aCode, const TDesC& aDescr,
		TBool aWillRetry) = 0;
	virtual void DownloadStarted(TInt64 aRequestId) = 0;
	virtual void Dequeued(TInt64 aRequestId) = 0;
	virtual void ConnectivityChanged(TBool aConnecting, TBool aOfflineMode,
		TBool aLowSignal, TBool aCallInProgress) = 0;
};

/*
 * Switching levels will dequeue (eventually) all items with levels below.
 * Downloader starts with level=0 if nothing else has been set. 
 * Levels <0 are reserved for internal use.
 *
 */
 
class CDownloader : public CCheckedActive {
public:
	IMPORT_C static CDownloader* NewL(MApp_context& aContext, RDbDatabase& Db, 
		const TDesC& aDirectory, MDownloadObserver& aObserver, 
		const TDesC& aConnectionName, HttpFactory aHttpFactory=0);
	virtual void DownloadL(TInt64 aRequestId, 
		const TDesC& aUrl,
		TBool aForce=EFalse,
		TInt8 aLevel=-1) = 0;
	virtual void RemoveRequest(TInt64 aRequestId) = 0;
	virtual void SetFixedIap(TInt aIap) = 0;
	virtual void SetLevelL(TInt8 aLevel) = 0;
	virtual TInt8 GetLevelL() = 0;
	virtual void SetDownloadLimitLevelL(TInt8 aLevel) = 0;
	virtual void SetNoDownloadLimitLevelL() = 0;
	
	enum TDownloadable { ENotQueued, ENotDownloadable, EDownloadable };
	virtual TDownloadable IsDownloadable(TInt64 aRequestId) = 0;
	virtual class CConnectivityListener* ConnectivityListener() = 0; // for unittests
protected:
	CDownloader();
};

class MPurgeObserver {
public:
	virtual void FilePurged(const TDesC& aFilename) = 0;
};

class CPurger : public CCheckedActive {
public:
	CPurger();
	IMPORT_C static CPurger* NewL(MPurgeObserver &aObserver, const TDesC& aDirectory);
	virtual void TriggerL(TInt aLimitInKilobytes) = 0;
	virtual void Stop() = 0;
};

#endif
