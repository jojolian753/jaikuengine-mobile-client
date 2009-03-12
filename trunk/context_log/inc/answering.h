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

#ifndef CL_Canswering_H_INCLUDED
#define CL_Canswering_H_INCLUDED 1

#include <etel.h>
#include <cpbkcontactengine.h> 
#include <badesca.h>

#ifndef NO_CFLDRINGINGTONEPLAYER_H
#include <CFLDRingingTonePlayer.h>
#endif

#include <MdaAudioSamplePlayer.h>
#include <mtclbase.h>
#include <msvapi.h>
#include <mtclreg.h>
#include <sendas.h>

#include "call_listener.h"
#include "i_logger.h"
#include "status_notif.h"
#ifndef NO_PROFILEAPI_H
#include <profileapi.h>
#endif

#include "app_context.h"

class Canswering : public Ccall_listener, public Mlogger, public MMdaAudioPlayerCallback, public MMsvSessionObserver,
	MSendAsObserver {
public:
	void at_place(const TDesC& name);
	virtual void register_source(const TDesC& name, const TDesC& initial_value, const TTime& time);
	virtual void new_value(log_priority priority, const TDesC& name, const TDesC& value, const TTime& time);
	virtual void unregister_source(const TDesC& /*name*/, const TTime& /*time*/) { }
	virtual const TDesC& name() const;
	~Canswering();

	static Canswering* NewL(MApp_context& Context, i_status_notif* i_cb);
private:
	Canswering(MApp_context& Context);
	void ConstructL(i_status_notif* i_cb);
	enum call_status { ANSWERED, MISSED, REFUSED };
	struct call_item {
		TTime		call_time;
		TContactItemId	contact;
		call_status	status;
		bool		reply_sent;
	};

	void store_call_status(call_status status);

	virtual void handle_incoming();
	virtual void handle_disconnected();
	virtual void handle_answered();
	virtual void handle_refused();

	void increase_volume();
	void send_reply();
	bool is_repeated_call();
	bool sent_reply();
	bool is_long_call();

	virtual void MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration);
	virtual void MapcPlayComplete(TInt aError);

	// MMsvSessionObserver
	virtual void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3);

	// MSendAsObserver
	virtual TBool CapabilityOK(TUid /*aCapabilty*/, TInt /*aResponse*/);


	TInt		really_silent_profile;
	CDesCArrayFlat*	mobile_prefixes;

	CArrayFixFlat<call_item>* previous_calls;

	TBuf<100> prev_place;
	TTime	  prev_time;
	TBuf<100> last_place;
	TTime	  last_place_time;
	TInt	  profile;
	bool      reply_sent_for_current;

#ifndef NO_CFLDRINGINGTONEPLAYER_H
	CFLDRingingTonePlayer* ringp;
#endif
#ifndef NO_PROFILEAPI_H
	CProfileAPI* profileapi;
#endif
	CArrayFixFlat<TInt>* profile_table_ids;
	CMdaAudioPlayerUtility* player;

};

#endif
