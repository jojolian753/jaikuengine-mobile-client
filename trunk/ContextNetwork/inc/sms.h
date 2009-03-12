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

#ifndef CL_SMS_H_INCLUDED
#define CL_SMS_H_INCLUDED

#include <mtclbase.h>
#include <msvapi.h>
#include <mtclreg.h>

#include <f32file.h>

#include "file_output_base.h"
#include "app_context.h"
#include "list.h"
#include "symbian_tree.h"

/*
 * now for handling both SMS and MMS
 * but compatible with old code that only handles SMS
 *
 */

class i_handle_received_sms {
public:
	virtual bool handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, 
		const TDesC& sender, const TDesC& body) = 0; // return true if message is to be deleted
	IMPORT_C virtual bool handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, 
		TUid aMtmUid, CBaseMtm* aMtm);
		// default implementation calls above handle_reception for sms, returns false for MMS
	virtual void handle_change(const TMsvId& msg_id, 
		const TDesC& sender) = 0;
	virtual void handle_delete(const TMsvId& msg_id, 
		const TMsvId& parent_folder, const TDesC& sender) = 0;
	virtual void handle_move(const TMsvId& msg_id, 
		const TMsvId& from_folder, const TMsvId& to_folder, const TDesC& sender) = 0;
	virtual void handle_error(const TDesC& descr) = 0;
	virtual void handle_sending(const TMsvId& entry_id, 
		const TDesC& sender, const TDesC& body) =0;
	IMPORT_C virtual void handle_sending(const TMsvId& entry_id, 
		TUid aMtmUid, CBaseMtm* aMtm);
		// default implementation calls above handle_sending for sms, does nothing for MMS
	virtual void handle_read(const TMsvId& msg_id, 
		const TDesC& sender, TUid aMtmUid, CBaseMtm* aMtm) = 0;
};

class dummyhandler;

class sms : public CCheckedActive, MMsvSessionObserver, public MTimeOut {
public:
	IMPORT_C sms();
	IMPORT_C ~sms();
	IMPORT_C void ConstructL();
	void CheckedRunL() { }
	void DoCancel() { }
	IMPORT_C TInt send_message(const TDesC& recipient, const TDesC& body, bool keep_sent=true);
	//void set_reception_handler(i_handle_received_sms* handler);
	IMPORT_C void AddHandler(i_handle_received_sms* handler);
protected:
	virtual void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3);
	void handle_received(const TMsvId& entry_id, const TMsvId& folder_id);
	void handle_moved(const TMsvId& entry_id,const TMsvId& from_entry_id,const TMsvId& to_entry_id);
	void handle_changed(const TMsvId& entry_id);
	void handle_deleted(const TMsvId& entry_id, const TMsvId& parent_id);
	void handle_error(const TDesC& descr);
	void handle_sent(const TMsvId& entry_id);
	void handle_read(const TMsvId& msg_id);
	
private:
	void send_messageL(const TDesC& recipient, const TDesC& body, bool keep_sent=true);
	TUid loadmessageL(const TMsvId& entry_id, TMsvEntry& entry);

	TBool DeleteSentEntry(TMsvId aEntryId);
	CMsvSession* iSession;
	CMsvSession* iReceiveSession;
	CClientMtmRegistry* iMtmReg;
	CClientMtmRegistry* iReceiveMtmReg;
	CBaseMtm*	iMtm;
	CBaseMtm	*iReceiveMtm, *iMMSMtm;
	//i_handle_received_sms*	reception_handler;
	class CSmsSettings* sendOptions;
	TBuf<50> state;
	CMsvOperation* op;
	TRequestStatus copy_rs;
	dummyhandler*	dummy;
		
	CList<i_handle_received_sms*>*	iHandlers;

#ifdef __WINS__
	CTimeOut*	iTimer;
	int		iCount;
#endif
	void expired(CBase*);
	CTimeOut*	iChangedTimer;
	CList<TMsvId>	*iChangedList;
};



#endif
