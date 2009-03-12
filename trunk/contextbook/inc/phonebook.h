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

class contact : public CBase 
{
public:
	TContactItemId	id;
	HBufC*		name;
	HBufC*		first_name;
	HBufC*		last_name;
	TInt		current_idx;
	bool		has_nick;
	MPresenceData* presence;
	contact() {
		id=0;
		name=0;
		first_name=0;
		last_name=0;
		presence=0;
		has_nick=false;
	}

	static contact* NewL(TContactItemId i_id, const TDesC& i_first_name, const TDesC& i_last_name);
	void PrintPresenceToListBox();
	void set_presence(MPresenceData* data);
	~contact();

private:

};

class phonebook_observer {
public:
	virtual void before_change() = 0;
	virtual void contents_changed() = 0;
	virtual void exiting() = 0;
};

class phonebook_i {
public:
	phonebook_i() { }
	virtual ~phonebook_i() { }
	virtual void ConstructL() = 0;
	virtual CPbkContactEngine* get_engine() = 0;
	
	virtual MDesCArray* get_array() = 0;
	virtual bool filter(const TDesC& substr, bool force=false) = 0;
	virtual TPtrC get_phone_no(TInt index) = 0;
	virtual void set_observer(phonebook_observer* i_obs) = 0;
	
	virtual TInt GetContactId(TInt Index) = 0;
	virtual TInt GetIndex(TInt ContactId) = 0;
	virtual contact * GetContact(TInt contactId) = 0;
	virtual void ReRead() = 0;
};

class phonebook : public CBase, MPbkContactDbObserver, public phonebook_i, public MContextBase,
	public MPresenceListener {
public:
	phonebook(MApp_context& Context, CJabberData& JabberData, CPresenceHolder& PresenceHolder);
	virtual ~phonebook();
	void ConstructL();
	
	MDesCArray* get_array();
	
	CPbkContactEngine* get_engine();
	bool filter(const TDesC& substr, bool force=false);
	virtual TPtrC get_phone_no(TInt index);
	virtual void set_observer(phonebook_observer* i_obs);
	phonebook_observer * get_observer();

	virtual TInt GetContactId(TInt Index);
	virtual TInt GetIndex(TInt ContactId);
	virtual void PresenceChangedL(TInt ContactId, MPresenceData& Info);
	void Notify(const TDesC& aMessage);
	virtual contact * GetContact(TInt contactId);
	CArrayFixFlat<contact*>* GetContacts();
		
	virtual void ReRead();
private:
	// MPbkContactDbObserver::
	void HandleDatabaseEventL(TContactDbObserverEvent aEvent); 

	void read_data(CPbkContactIter * iter);
	void read_db();
	void reset_contacts();
	
	CPtrCArray* current_array;
	CArrayFixFlat<contact*>* contacts;
	CArrayFixFlat<TContactItemId>* current_ids;
	
	CPbkContactEngine *eng;
	bool owns_engine;
	TBuf<20> previous_filter;
	phonebook_observer* obs;
	CPbkContactItem* contactitem; // used for getting phone no

	CPbkContactChangeNotifier* chnot;
	
	CGenericIntMap *iContactToIndex;
	
	CJabberData&	iJabberData;
	CPresenceHolder& iPresenceHolder;

};


class CPresenceUpdater : public MTimeOut
{
public : 
	static CPresenceUpdater * NewL(phonebook & book);
	void ConstructL();
	virtual ~CPresenceUpdater();
	CPresenceUpdater(phonebook & book);


	void Start();
	void Stop();
	void Refresh();

public : // MTimeOut
	void expired(CBase*);

private:
	phonebook & book;
	CTimeOut * iWait;
	TBool iWaiting;

	const static TInt KTimeOut;
};

#endif
