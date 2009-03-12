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

#ifndef CONTEXT_CBBSESSION_H_INCLUDED
#define CONTEXT_CBBSESSION_H_INCLUDED 1

#include "blackboardclientsession.h"
#include "bbtypes.h"

class MBBObserver {
public:
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData) = 0;
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName) = 0;
};

class MBBSession {
public:
	/* if aIgnoreNotFound=ETrue and the tuple doesn't exist aDataInto will be 0 */
	virtual void GetL(const TTupleName& aName, const TDesC& aSubName, 
			MBBData*& aDataInto, TBool aIgnoreNotFound=EFalse) = 0;
	virtual void PutL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName=KNoComponent) = 0;
	virtual TUint PutRequestL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName,
			TBool aKeepExisting=EFalse) = 0;
	virtual void PutReplyL(const TTupleName& aTupleName, const TDesC& aSubName, 
			const MBBData* aData, const TTime& aLeaseExpires,
			const TComponentName& aComponentName) = 0;
	virtual void DeleteL(TUint id, TBool aIgnoreNotFound=EFalse) = 0;
	virtual void DeleteL(const TTupleName& aTupleName, 
		const TDesC& aSubName, TBool aIgnoreNotFound=EFalse) = 0;
	virtual void DeleteL(const TComponentName& aComponentName, 
		TBool aIgnoreNotFound=EFalse) = 0;
};

class CBBSubSession : public CCheckedActive, public MContextBase, public MBBSession {
public:
	virtual void AddNotificationL(const TTupleName& aTupleName) = 0;
	virtual void AddNotificationL(const TTupleName& aTupleName, TBool aGetExisting) = 0;
	virtual void AddNotificationL(const TComponentName& aComponentName) = 0;
	virtual void DeleteNotifications() = 0;

protected:
	CBBSubSession(MApp_context& Context);
};

class CBBSession : public CCheckedActive, public MContextBase, public MBBSession {
public:
	IMPORT_C static CBBSession* NewL(MApp_context& Context, MBBDataFactory* aFactory);

	virtual CBBSubSession* CreateSubSessionL(MBBObserver* anObserver) = 0;
	virtual MBBDataFactory* GetBBDataFactory() = 0;
protected:
	CBBSession(MApp_context& Context);
};

#endif
