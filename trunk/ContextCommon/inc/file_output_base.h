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

#if !defined(FILE_OUTPUT_BASE_H_INCLUDED)

#define FILE_OUTPUT_BASE_H_INCLUDED

#include <f32file.h>
#include <charconv.h>
#include <e32std.h>
#include "timeout.h"
#include "list.h"

#include "app_context.h"

IMPORT_C void format_datetime(TDes& buffer, const TTime& time);

/*
 * If MaxLogCount is 0, then new logs are created each time, with
 * a timestamp as the filename suffix. If aMaxLogCount>0, then
 * logs are rotated instead, and numbers 1..aMaxLogCount are used as
 * the suffix. If the make_filename() functions are called with aMaxLogCount>0,
 * then an app context must have been created for this thread.
 */

class Mfile_output_base : public MContextBase {
public:
	IMPORT_C Mfile_output_base(MApp_context& Context);
	IMPORT_C virtual ~Mfile_output_base();
	IMPORT_C virtual void ConstructL(const TDesC& prefix, bool do_open_file=true,
		TInt aMaxLogCount=0, TInt* aRecentLogCount=0);
	IMPORT_C void switch_file();
	IMPORT_C void write_time();
	IMPORT_C void write_time(const TTime& time);
	IMPORT_C virtual void write_to_output(const TDesC& str);
	IMPORT_C virtual void write_to_output(const TDesC8& str);
	IMPORT_C static void make_filename(TDes& into_buffer, const TDesC& dir, const TDesC& prefix,
		TInt aMaxLogCount=0, TInt* aRecentLogCount=0);
	IMPORT_C static void make_filename(TDes& into_buffer, const TDesC& dir, const TDesC& prefix, 
		const TDesC& extension,	TInt aMaxLogCount=0, TInt* aRecentLogCount=0);
	IMPORT_C void write_nl();
	TBool	paused;
protected:
	IMPORT_C void open_file(const TDesC& prefix, TInt* aRecentLogCount=0);
	IMPORT_C void close_file();
private:
	RFile file;
	TBuf8<512> buf;
	HBufC* file_prefix;
	TInt state;
	bool	file_is_open;
	bool	has_e_drive;
	TFileName current_file_name;
	TInt	iMaxLogCount;
};

class Cfile_output_base: public Mfile_output_base, public CBase, public MTimeOut
{
public:
	IMPORT_C Cfile_output_base(MApp_context& Context);
	IMPORT_C virtual ~Cfile_output_base();
	IMPORT_C static Cfile_output_base* NewL(MApp_context& Context, const TDesC& prefix, bool do_open_file=true, 
		bool buffered_writes=false, TInt aMaxLogCount=0, TInt* aRecentLogCount=0);
	IMPORT_C virtual void write_to_output(const TDesC8& str);
	IMPORT_C virtual void write_to_output(const TDesC& str);
	IMPORT_C void log_message(const TDesC8& str);
	IMPORT_C void log_message(const TDesC& str);
private:
	virtual void ConstructL(const TDesC& prefix, bool do_open_file=true, 
		bool buffered_writes=false, TInt aMaxLogCount=0, TInt* aRecentLogCount=0);
	virtual void expired(); // not called
	virtual void expired(CBase* source); // not called

	CList< HBufC8* > *iBuffers;
	bool	iBuffered;
	CTimeOut*	iTimer;
	bool	iDeleting;
};

#endif
