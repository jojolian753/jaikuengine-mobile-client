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

#if !defined(FILE_LOGGER_H_INCLUDED)

#define FILE_LOGGER_H_INCLUDED

#include "i_logger.h"
#include "i_log_source.h"
#include "file_output_base.h"
#include "pausable.h"
#include "settings.h"

class Cfile_logger : public Mlogger, public Mfile_output_base, 
	public CBase, public MPausable, public MSettingListener {
public:
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, const MBBData* aData);
	virtual void NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, const CBBSensorEvent& aEvent);
	virtual void new_value(TInt priority, const TDesC& aSource, const TDesC& aValue, const TTime& time);

	virtual ~Cfile_logger();
	IMPORT_C static Cfile_logger* NewL(MApp_context& Context,
				 const TDesC& prefix, CBBSensorEvent::TPriority limit=CBBSensorEvent::VALUE,
				 TInt aMaxLogCount=0);
	IMPORT_C void pause();
	IMPORT_C void unpause();
	IMPORT_C bool is_paused();
	IMPORT_C void write_line(const TDesC& aLine);
	virtual void write_to_output(const TDesC& str);
private:
	Cfile_logger(MApp_context& Context, CBBSensorEvent::TPriority limit=CBBSensorEvent::VALUE);
	void ConstructL(const TDesC& prefix, TInt aMaxLogCount);
	virtual void SettingChanged(TInt Setting);
	void get_value(const CBBSensorEvent& aEvent);
	void get_value(const MBBData* aData);

	CBBSensorEvent::TPriority priority_limit;
	bool	enabled;
	HBufC*	iBuf;

};

#endif
