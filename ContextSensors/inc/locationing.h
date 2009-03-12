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

#if !defined(LOCATIONING_H_INCLUDED)

#define LOCATIONING_H_INCLUDED

#include "sms.h"
#include "status_notif.h"
#include "app_context.h"
#include "list.h"
#include "connectioninit.h"
#include "settings.h"

#include "csd_cell.h"

class MNaming {
public:
	virtual void add_cellid_name(const TBBCellId& cellid, const TDesC& name) = 0;
};

class Clocationing: public CBase, public i_handle_received_sms, public MContextBase,
public MTimeOut, public MSettingListener {
public:
	IMPORT_C void test();
	IMPORT_C void GetNameL(const TBBCellId* CellId, TBool aForce=EFalse);
	//IMPORT_C const TDesC& GetImsi() const;
	IMPORT_C bool LocationingAvailable() const;
	IMPORT_C bool LocationingAllowed() const;

	IMPORT_C static Clocationing* NewL(MApp_context& Context, i_status_notif* i_cb, MNaming* i_naming, sms* i_sms_handler, MNetworkConnection * aPublisher);
	IMPORT_C ~Clocationing();

	IMPORT_C virtual void now_at_location(const TBBCellId* Cell, TInt id, bool is_base, bool loc_changed, TTime time);
private:
	Clocationing(MApp_context& Context);
	void ConstructL(i_status_notif* i_cb, MNaming* i_naming,sms* i_sms_handler, MNetworkConnection * aPublisher);
	virtual bool handle_reception(const TMsvId& entry_id, const TMsvId& folder_id, const TDesC& sender, const TDesC& body); // return true if message is to be deleted
	virtual void handle_error(const TDesC& descr);
	virtual void handle_change(const TMsvId& msg_id, const TDesC& sender);
	virtual void handle_read(const TMsvId& msg_id, const TDesC& sender, TUid, CBaseMtm*);
	virtual void handle_delete(const TMsvId& msg_id, const TMsvId& parent_folder, const TDesC& sender);
	virtual void handle_move(const TMsvId& msg_id, const TMsvId& from_folder, const TMsvId& to_folder, const TDesC& sender);
	virtual void handle_sending(const TMsvId& entry_id, const TDesC& sender, const TDesC& body);
	// MTimeOut
	virtual void expired(CBase*);
	
	// MSettingListener
	virtual void SettingChanged(TInt Setting);

	MNaming*	iNaming;

	struct TNamingRec {
		TBBCellId	CellId;
		bool		SentRequest;
		TNamingRec(const TBBCellId i_CellId) : CellId(i_CellId.Name()), SentRequest(false) 
		{ 
			CellId=i_CellId;
		}
		TNamingRec() : CellId(KCell) { }
		void Reset() {
			SentRequest=false;
		}
	};

	TNamingRec	iToName;
	CTimeOut*	iTimer;
	int		iToDiscard;
	TBuf<20>	iName;
	bool		iLocationingAvailable; // RL network
	bool		iEnabled;
	enum TStyle { ENone, ERadiolinja, ESonera };
	TStyle		iStyle;

	TBuf<20>	imsi;


	sms* smsh;
	MNetworkConnection * iPublisher;
	//sms_reception* smsh2;
	i_status_notif* cb;
	TInt		iRetryCount;
};

#endif

