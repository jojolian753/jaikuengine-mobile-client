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

#ifndef CL_TRANSFER2_H_INCLUDED
#define CL_TRANSFER2_H_INCLUDED 1

#include <d32dbms.h>
#include "app_context.h"
#include "status_notif.h"
#include "connectioninit.h"
#include "bbdata.h"

typedef struct {
	TInt err;
	TFileName fn;
} TFileOpStatus;


class MPostUploadCallback {
public:
	// important: these numbers are used by mediaprompt/contextmediaui, don't change
	enum TState { EQueued=3, EFailed, EUploaded, EDeleted, ENotShared };
	virtual void StateChangedL(TInt64 aPostId, TState aState, const TDesC& aFileName,
		const class CBBErrorInfo* aError) = 0;
};

class CHttpTransfer2 : public CCheckedActive {
public:
	IMPORT_C static CHttpTransfer2* NewL(MApp_context& Context,
		RDbDatabase& Db, MSocketObserver* aStatusNotif);

	virtual TFileName AddFileToQueueL(const TDesC& aFileName, TInt UrlSetting,
		TInt ScriptSetting, TBool DeleteAfter, const TDesC& aMeta,
		MBBData* aPacket, TInt aWaitTime=0, TBool aDontMove=EFalse) = 0;
	virtual TInt DeleteFile(const TDesC& aFileName) = 0;

	virtual void DoNotTransfer(const TDesC& aFileName, TInt64 aPostId, TFileOpStatus& aStatus) = 0;
	virtual void MoveToOld(const TDesC& aFileName, TInt64 aPostId, TFileOpStatus& aStatus) = 0;
	virtual void DoNotTransfer(const TDesC& aFileName, TFileOpStatus& aStatus) = 0;
	virtual void MoveToOld(const TDesC& aFileName, TFileOpStatus& aStatus) = 0;

	virtual TBool FileIsQueued(const TDesC& aFileName) = 0;
	virtual void RemoveFromQueueL(const TDesC& aFileName) = 0;
	virtual void Trigger(TBool Force=EFalse) = 0;
	virtual void AddPostCallbackL(MPostUploadCallback* aCallback) = 0;
	virtual void RemovePostCallback(MPostUploadCallback* aCallback) = 0;

	CHttpTransfer2();
	IMPORT_C virtual ~CHttpTransfer2();
};

#endif
