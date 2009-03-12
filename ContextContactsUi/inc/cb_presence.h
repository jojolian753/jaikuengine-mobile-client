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

#ifndef CB_PRESENCE_H_INCLUDED
#define CB_PRESENCE_H_INCLUDED 1

#include "symbian_tree.h"
#include "list.h"
#include "jabberdata.h"
#include "csd_presence.h"
#include "timeout.h"
#include "cbbsession.h"


class MPresenceListener {
public:
	virtual void PresenceChangedL(TInt ContactId, CBBPresence* Info) = 0;
	virtual void Notify(const TDesC & aMessage) = 0;
};

class MPresenceFetcher {
public:
	virtual void NickChanged(const TDesC& aPrevious, const TDesC& aNew) = 0;
};


class CPresenceHolder : public CBase, public MBBObserver, public MContextBase,
	public MPresenceFetcher, public MJabberDataObserver  {
public:
	IMPORT_C static CPresenceHolder* NewL(CJabberData& JabberData);
	IMPORT_C ~CPresenceHolder();

	IMPORT_C void AddListener(MPresenceListener* Listener);
	IMPORT_C void RemoveListener(MPresenceListener* Listener);

	IMPORT_C const CBBPresence* const GetPresence(TInt ContactId) const;

	IMPORT_C void CancelRequest();
	//IMPORT_C void NewPresence(const TDesC& Nick, const TDesC& Info, const TTime& send_timestamp);
	IMPORT_C CBBPresence* GetPresence(TInt ContactId);

	IMPORT_C  void SetMatcher(class CContactMatcher* aMatcher);

private:

	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData);
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName);
	virtual void NickChanged(const TDesC& aPrevious, const TDesC& aNew);
	virtual void IdChanged(const TDesC& aNick, TInt aOldId, TInt aNewId);

private:
	CPresenceHolder(CJabberData& JabberData);
	void ConstructL();

	static void CBBPresenceDeletor(void* p);

	TContactItemId GetContactForNickL( const TDesC& aNick, const CBBPresence& aPresence );
	TContactItemId MatchPresenceToContactL(const TDesC& aNick, 
										   const CBBPresence& aPresence);

	void NotifyListeners(TInt aContact, CBBPresence* aPresence);

	CJabberData&			iJabberData;
	CGenericIntMap*			  iContactsPresence;
	//CList<MPresenceListener*>*	iListeners;
	RPointerArray<MPresenceListener> iListeners;
	class CContactMatcher*	iMatcher;

private:
	TTime				iSendTimeStamp;
	static const TInt KDefaultBufferSize;

	class CBBSubSession*		iBBSession;
};

#endif
