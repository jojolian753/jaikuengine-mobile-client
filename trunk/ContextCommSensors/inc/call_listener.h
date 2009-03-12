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

#if !defined(Ccall_listener_H_INCLUDED)
#define Ccall_listener_H_INCLUDED 1

#include <etel.h>


#include <cpbkcontactengine.h> 
#include <badesca.h>


#include "i_logger.h"
#include "status_notif.h"
#include "app_context.h"

#include <logview.h>
#include <logcli.h>
#include <logwrap.h>
#include "timeout.h"

class CSpecialGroups {
public:
	static CSpecialGroups* NewL(MApp_context& Context);
	virtual void read_contact_groups() = 0;
	virtual void store_contact(const TDesC& number) = 0;
	virtual bool is_special_contact() = 0;
	virtual ~CSpecialGroups() { }
	virtual void AddGroupL(const TDesC& GroupName) = 0;
	virtual CContactIdArray* current_contact_ids() = 0;
};

class MCallerIdMngrObserver{
public:
	virtual void LastCallerId(const TDesC& caller_id) = 0;
};


class CCallerIdMngr: public CCheckedActive{
public:
	~CCallerIdMngr();

	static CCallerIdMngr* NewLC(MCallerIdMngrObserver * obs);
	static CCallerIdMngr* NewL(MCallerIdMngrObserver *obs);
	
	void GetLatest();
	TBuf<20> GetLastNumber();
private:	
	CCallerIdMngr(MCallerIdMngrObserver * obs);
	void ConstructL();
	
	void CheckedRunL(); // from CActive
	void DoCancel(); // from CActive
	TInt RunError(TInt aError);// from CActive

	enum state { IDLE, WAITING };
	state current_state;

//private member variables
private:
	CLogClient* iLogClient;
	CLogViewRecent* iRecentLogView;
	
	TBuf<20>  iLastNumber;
	MCallerIdMngrObserver *iObs;
};




class Ccall_listener: public CCheckedActive, public MContextBase, public MTimeOut, MCallerIdMngrObserver{
public:
	enum TDirection { INCOMING, OUTGOING };
protected:
	Ccall_listener(MApp_context& Context);
	virtual void ConstructL(i_status_notif* i_cb, TDirection dir);
	virtual ~Ccall_listener();

	void CheckedRunL();
	void DoCancel();
	TInt CheckedRunError(TInt aError);
	enum state { LISTENING_INCOMING, TEST_ME, CALL_IN_PROGRESS };
	state current_state;

	void expired(CBase*);

	virtual void handle_incoming() { }
	virtual void handle_disconnected() { }
	virtual void handle_answered() { }
	virtual void handle_refused() { }

	virtual void LastCallerId(const TDesC& caller_id);

	void listen_for_call();
	void store_contact();
	void open_call(); bool call_is_open;
	void close_call();

	i_status_notif* cb;
	CTimeOut*	iTimer;

	TTime	  call_start_time;

	TDirection iDir;
	TName	  call_name;
	RLine line;

#ifdef __WINS__
	class RCall *call;
#else

#  ifndef __S60V2__
#    ifndef NO_ETELAGSM_H
	class RAdvGsmCall *call; 
#    else
	class RCall *call;
#    endif
#  else
	class RMobileCall *call;
#  endif
#endif

	TBuf<30>	iState;
	//MAdvGsmCallControl::TGsmStatus current_call_status;
	RCall::TStatus current_call_status;

	bool	got_info_for_this_call;

	CSpecialGroups*	iSpecialGroups;
	TBuf<20> last_caller;
};

#endif
