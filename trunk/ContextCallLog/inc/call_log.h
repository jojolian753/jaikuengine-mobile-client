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

#ifndef CB_CALL_LOG_H_INCLUDED
#define CB_CALL_LOG_H_INCLUDED

#include "phonebook.h"
#include <logview.h>
#include <logcli.h>
#include "symbian_tree.h"
#include "list.h"


class call_log_i: public phonebook_i
{
public: 
	call_log_i() { }
	virtual ~call_log_i() { }
	virtual void ConstructL() = 0;

public: //from phonebook_i
	enum TFilter {
		EMissed,
		EDialled,
		EReceived
	};
	virtual CPbkContactEngine* get_engine() = 0;
	virtual void ReRead() = 0;
	virtual TInt GetContactId(TInt Index) = 0;
	virtual TInt GetIndex(TInt ContactId) = 0;
	virtual contact * GetContact(TInt index) = 0;
	virtual TInt Count() = 0;

	virtual void AddObserverL(phonebook_observer* i_obs)=0;
	virtual void RemoveObserverL(phonebook_observer* i_obs)=0;
	virtual void NotifyContentsChanged(TInt aContactId, TBool aPresenceOnly)=0;
	virtual TTime GetAtTime(TInt aIndex) = 0;

	virtual void DeleteEvent(TInt index) = 0;
	virtual void ClearEventList() = 0;
	virtual TPtrC get_phone_no(TInt index) = 0;
	virtual const TDesC& PresenceSuffix() = 0;

	virtual void SetFilter(TFilter aFilter) = 0;
	virtual CLogEvent * get_event(TInt aIndex) = 0;

	bool filter(const TDesC& substr, bool force=false) {return false;}
};


class call_log: public call_log_i, public CCheckedActive, public MContextBase,
	public MPresenceListener {
public:
	call_log(MApp_context& Context, CJabberData * JabberData, CPresenceHolder * PresenceHolder);
	virtual ~call_log();
	void ConstructL();

// from call_log_i
public:
	CPbkContactEngine* get_engine() { return eng; }
	virtual TInt Count();
	virtual void ReRead();
	virtual TInt GetContactId(TInt Index); 
	virtual TInt GetIndex(TInt );
	virtual contact * GetContact(TInt contactId);
	virtual TTime GetAtTime(TInt aIndex);
		
	virtual void AddObserverL(phonebook_observer* i_obs);
	virtual void RemoveObserverL(phonebook_observer* i_obs);

	virtual void DeleteEvent(TInt index);
	virtual void ClearEventList();
	virtual TPtrC get_phone_no(TInt index);
	virtual void SetFilter(TFilter aFilter);

	virtual const TDesC& PresenceSuffix();
	virtual CLogEvent * get_event(TInt aIndex);

// from MPresenceListener
private:
	virtual void PresenceChangedL(TInt ContactId, CBBPresence* Info);
	void Notify(const TDesC& aMessage);
	void refresh();
	//bool filter(const TDesC& /*substr*/, bool /*force*/);
		
protected:
	void CheckedRunL();
	void DoCancel();
	//virtual TBuf<100> ExportVCardToFile(TInt /*ContactId*/) { return _L("");}

private:
	void reset_call_log();
	bool handle_event(const CLogEvent& ev);
	void copy_new_to_current();
	void NotifyContentsChanged(TInt aContactId, TBool aPresenceOnly);
	virtual void set_observer(phonebook_observer* ) { User::Leave(-1028); }

private:	
	enum state { IDLE, WAITING_SET, WAITING_NEXT, WAITING_DELETE, WAITING_CLEAR, WAITING_CHANGE };
	state current_state;

	CArrayFixFlat<contact*>* current_contacts;
	CArrayFixFlat<contact*>* all_contacts;
	CArrayFixFlat<CLogEvent*> * current_events;
	CArrayFixFlat<CLogEvent*> * all_events;

	CPtrCArray* current_nos;
	CDesCArray* all_nos;
	
	TInt new_from;

	CPbkContactEngine *eng;
	bool owns_engine;
	CList<phonebook_observer*> *iObservers;

	CLogViewRecent* recent;
	CLogClient* logclient;
	CLogFilter* logfilter;

	TLogId last_seen_id;
	bool getting_first;
	
	bool do_refresh; // redo refresh after current

	CJabberData*	iJabberData;
	CPresenceHolder* iPresenceHolder;
        TInt event_count;
};

#endif
