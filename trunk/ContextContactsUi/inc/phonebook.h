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

#ifndef CB_PHONEBOOK_H_INCLUDED
#define CB_PHONEBOOK_H_INCLUDED 1

#include <e32base.h>
#include <badesca.h>
#include <cntdef.h>
#include <cpbkcontactengine.h> 
#include <mpbkcontactdbobserver.h> 
#include <cpbkcontactchangenotifier.h> 
#include "db.h"
#include "symbian_tree.h"
#include "cb_presence.h"
#include "jabberdata.h"
#include "timeout.h"
#include "ccu_contact.h"
#include "errorhandling.h"

static const TErrorCode KSyncBrokeIterator = { CONTEXT_UID_CONTEXTCONTACTSUI, -1 };

class phonebook_observer {
public:
	virtual void before_change() = 0;
	virtual void contents_changed(TInt contact_id, TBool aPresenceOnly) = 0;
	virtual void exiting() = 0;
};

class phonebook_i {
public:
	phonebook_i() { }
	virtual ~phonebook_i() { }
	virtual void ConstructL() = 0;

	/**
	 * FIXME: check where this is needed and think if we can abstract those needs
	 */ 
	virtual CPbkContactEngine* get_engine() const = 0;

	/**
	 * FIXME: Avoid calling this.
	 */ 
	virtual void ReRead() = 0;

	virtual TInt GetContactId(TInt Index) = 0;
	virtual TInt GetIndex(TInt ContactId) = 0;
	
	// FIXME: below two are too similar. It would be better if their
	// type signature would be different
	virtual contact* GetContact(TInt index) = 0;
	virtual contact* GetContactById(TInt aContactId) = 0;

	virtual TInt Count() = 0;

	virtual bool filter(const TDesC& substr, bool force=false) = 0;

	virtual void AddObserverL(phonebook_observer* i_obs)=0;
	virtual void RemoveObserverL(phonebook_observer* i_obs)=0;
	// private:
	virtual void NotifyContentsChanged(TInt aContactId, TBool aPresenceOnly)=0;
	virtual class CContactMatcher* GetMatcher() { return 0; };

	/**
	 * Accessories to unrelated services
	 */
	virtual TBool IsPresenceEnabled() const = 0;
	virtual TBool ShowLastNameFirstL() const = 0;
};

class MCachedContactsObserver {
public:
	virtual void CachedContactsEvent(TInt aError, TBool aFailed) = 0;
};

class phonebook : public CBase, MPbkContactDbObserver, public phonebook_i, public MContextBase,
	public MPresenceListener, public MSettingListener, public MTimeOut, public MCachedContactsObserver {
public:
	IMPORT_C phonebook(MApp_context& Context, CJabberData * JabberData, CPresenceHolder * PresenceHolder);
	IMPORT_C virtual ~phonebook();
	IMPORT_C void ConstructL();

//own
	IMPORT_C CArrayFixFlat<contact*>* GetContacts();

//from phonebook_i
	IMPORT_C CPbkContactEngine* get_engine() const;
	IMPORT_C virtual void ReRead();
	IMPORT_C virtual TInt GetContactId(TInt Index);
	IMPORT_C virtual TInt GetIndex(TInt ContactId);
	IMPORT_C virtual contact* GetContact(TInt index);
	IMPORT_C virtual contact* GetContactById(TInt aContactId);
	IMPORT_C virtual TInt Count();
	IMPORT_C bool filter(const TDesC& substr, bool force=false);

	IMPORT_C void AddObserverL(phonebook_observer* i_obs);
	IMPORT_C void RemoveObserverL(phonebook_observer* i_obs);
	IMPORT_C void NotifyContentsChanged(TInt aContactId, TBool aPresenceOnly);
	IMPORT_C class CContactMatcher* GetMatcher();
		
	// from MPresenceListener
	IMPORT_C virtual void PresenceChangedL(TInt ContactId, CBBPresence* Info);
	IMPORT_C void Notify(const TDesC& aMessage);
	

	contact* CreateDummyContactL(TContactItemId aId, CBBPresence* aPresence);

	contact* CreateContactL(TContactItemId aId, 
							const TDesC& aFirstName,
							const TDesC& aLastName, 
							const TDesC& aExtraName,
							CBBPresence* aPresence = NULL);
	
	virtual TBool IsPresenceEnabled() const;
	virtual TBool ShowLastNameFirstL() const;
 protected: // From MSettingListener
	virtual void SettingChanged(TInt aSetting);
	
private:
	// MPbkContactDbObserver::
	void HandleDatabaseEventL(TContactDbObserverEvent aEvent); 
	// MCachedContactsObserver
	virtual void CachedContactsEvent(TInt aError, TBool aFailed);

	void NotifyBeforeChangeL();

	void read_data(CPbkContactIter * iter);
	void read_db();
	void reset_contacts();

	void InsertInOrderL(contact* aContact);
	
	void GetNamesForContactL(CPbkContactItem* item);
	void ReadDummiesL();
	
	void expired(CBase* source);

 private: // data
	/**
	 * current contacts, read to contact objects
	 * Sorted by Jaiku sorting rules. 
	 */ 
	CArrayFixFlat<contact*>* contacts;

	/**
	 * FIXME: this seems to unnecessary.
	 */
	CArrayFixFlat<TContactItemId>* current_ids;
	
	mutable CPbkContactEngine *iMyEngine;

	TBuf<20> previous_filter; //FIXME: remove, not used

	CList<phonebook_observer*> *iObservers;
	CPbkContactItem* contactitem; // FIXME: remove, not used, (was: used for getting phone no)

	CPbkContactChangeNotifier* chnot; //FIXME: rename

	/**
	 * Maps { contact_id -> (index in contacts array + 1) } 
	 */
	CGenericIntMap *iContactToIndex;

	CJabberData *	iJabberData;
	CPresenceHolder * iPresenceHolder;

	TBool iPresenceEnabled;

	// used when reading the data
	TBuf<100> first_name;
	TBuf<100> last_name;
	TBuf<100> extra_name;
	TBuf<100> jabber_nick;

	class CContactMatcher* iMatcher;

	HBufC* iComparatorWorkBuffer1;
	HBufC* iComparatorWorkBuffer2;
	TBool lastNameFirst;
	class contacts_key* iComparator;
	CTimeOut*	iTimer;
	class CCachedContacts *iCachedContacts;
	TBool		iUseCachedContacts;
};



class CPresenceUpdater : public MTimeOut, public CBase
{
public : 
	IMPORT_C static CPresenceUpdater * NewL(phonebook_i & book);
	IMPORT_C void ConstructL();
	IMPORT_C virtual ~CPresenceUpdater();
	IMPORT_C CPresenceUpdater(phonebook_i & book);


	IMPORT_C void Start();
	IMPORT_C void Stop();
	IMPORT_C void Refresh();

public : // MTimeOut
	void expired(CBase*);

private:
	phonebook_i & book;
	CTimeOut * iWait;
	TBool iWaiting;

	const static TInt KTimeOut;
};

#endif
