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



class call_log: public phonebook_i, public CCheckedActive {
public:
	call_log();
	virtual ~call_log();
	void ConstructL();
	
	MDesCArray* get_array();
	bool filter(const TDesC& substr, bool force=false);
	TPtrC get_phone_no(TInt index);
	virtual void set_observer(phonebook_observer* i_obs);
	virtual CPbkContactEngine* get_engine() { return eng; }
	virtual TInt GetContactId(TInt /*Index*/) { return KErrNotFound; }
	virtual TInt GetIndex(TInt /*ContactId*/) { return KErrNotFound; }
	virtual contact * GetContact(TInt /*contactId*/) { return NULL; }
	
	void CheckedRunL();
	void DoCancel();

	void refresh();
private:
	bool handle_event(const CLogEvent& ev);
	void copy_new_to_current();
	void ReRead();
private:	
	enum state { IDLE, WAITING_SET, WAITING_NEXT };
	state current_state;
	CPbkContactEngine *eng;
	bool owns_engine;
	phonebook_observer* obs;
	CPtrCArray* current_array;
	CPtrCArray* current_nos;

	CLogViewRecent* recent;
	CLogClient* logclient;
	CLogFilter* logfilter;

	CDesCArray* all_array;
	CDesCArray* all_nos;
	TLogId last_seen_id;
	bool getting_first;
	TInt new_from;
	CGenericIntMap* iContactToItems;

	bool do_refresh; // redo refresh after current

};

#endif
