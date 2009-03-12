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

#ifndef CL_LOG_SMS_H_INCLUDED
#define CL_LOG_SMS_H_INCLUDED 1

#include <e32base.h>
#include "log_base_impl.h"
#include "sms.h"
#include "call_listener.h"

class CLog_sms : public Mlog_base_impl , public i_handle_received_sms, public CBase
{
public:
	IMPORT_C static CLog_sms* NewL(MApp_context& Context, const TDesC& name);
	IMPORT_C ~CLog_sms();

private:
	void SaveMMSFiles(const TMsvId& entry_id, class CMmsClientMtm* mmsMtm);
	//from i_handle_received_sms
	virtual bool handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, 
		TUid aMtmUid, CBaseMtm* aMtm);
	virtual void handle_sending(const TMsvId& entry_id, 
		TUid aMtmUid, CBaseMtm* aMtm);

	virtual bool handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, const TDesC& sender, const TDesC& body); // return true if message is to be deleted
	virtual void handle_change(const TMsvId& msg_id, const TDesC& sender);
	virtual void handle_delete(const TMsvId& msg_id, const TMsvId& parent_folder, const TDesC& sender);
	virtual void handle_move(const TMsvId& msg_id, const TMsvId& from_folder, const TMsvId& to_folder, const TDesC& sender);
	virtual void handle_error(const TDesC& descr);
	virtual void handle_sending(const TMsvId& entry_id, const TDesC& sender, const TDesC& body);
	virtual void handle_read(const TMsvId& msg_id, const TDesC& sender, 
		TUid aMtmUid, CBaseMtm* aMtm);
	
private:
	CLog_sms (MApp_context& Context);
	void MapEntry(const TMsvId& entry_id, CBBString* aInto);
	void ConstructL(const TDesC& name);
	TBool IsFromToBuddy(const TDesC& sender);

private:
	TFileName	iAttchPath, iCopyToPath;
	CBBString*	iValue;
	CSpecialGroups*	iSpecialGroups;	

};

#endif
