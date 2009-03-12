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

#ifndef LOG_BASE_IMPL_H_INCLUDED

#define LOG_BASE_IMPL_H_INCLUDED

#include "i_log_source.h"
#include <e32base.h>
#include <f32file.h>
#include "i_logger.h"
#include "app_context.h"

#include "csd_event.h"
#include "cbbsession.h"

class Mlog_base_impl : public i_log_source, public MContextBase {
public:
	IMPORT_C void add_sinkL(Mlogger* sink);
	IMPORT_C virtual void ConstructL();
	IMPORT_C Mlog_base_impl(MApp_context& Context, const TDesC& aName, const TTupleName& aTupleName,
		TInt aLeaseInSeconds);
	IMPORT_C virtual ~Mlog_base_impl();
	IMPORT_C virtual const CBBSensorEvent& get_value();
	IMPORT_C void post_new_value(const CBBSensorEvent& aEvent);
	IMPORT_C void post_new_value(CBBSensorEvent& aEvent);
	IMPORT_C void post_error(const TDesC& aMsg, TInt aCode, const TTime& time);
	IMPORT_C void post_error(const TDesC& aMsg, TInt aCode);
	IMPORT_C void post_new_value(MBBData* aData);
	IMPORT_C void post_new_value(MBBData* aData, const TTime& time);
	IMPORT_C void post_unchanged_value(MBBData* aData);
	IMPORT_C TTime GetLeaseExpires();
protected:
	//CArrayFixFlat<Mlogger*>* loggers;
	CBBSensorEvent	iEvent;
	CBBSubSession	*iBBSubSession;
	TInt		iLeaseTimeInSeconds;
};

class Clog_base_impl : public CBase, public Mlog_base_impl {
public:
	IMPORT_C Clog_base_impl(MApp_context& Context, const TDesC& aName, 
		const TTupleName& aTupleName, TInt aLeaseTime);
	IMPORT_C static Clog_base_impl* NewL(MApp_context& Context, const TDesC& aName, 
		const TTupleName& aTupleName, TInt aLeaseTime);
};

#endif
