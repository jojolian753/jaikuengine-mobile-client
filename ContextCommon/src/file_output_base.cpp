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

#include "file_output_base.h"

#include <e32std.h>
#include <bautils.h>
#include "app_context_impl.h"

#define MMC_ONLY_ON_TRANSFER 1

_LIT(nl, "\r\n");

EXPORT_C void Mfile_output_base::write_to_output(const TDesC8& str)
{
	if (NoSpaceLeft() || paused || !file_is_open) return;
	file.Write(str);
}

EXPORT_C void Mfile_output_base::write_nl()
{
	write_to_output(nl);
}

EXPORT_C void Mfile_output_base::write_to_output(const TDesC& str)
{
	TInt pos=0, len;
	len=buf.MaxLength();
	while (pos < str.Length()) {
		TInt real_len;
		if (pos+len > str.Length()) {
			real_len=str.Length()-pos;
		} else {
			real_len=len;
		}
		CC()->ConvertFromUnicode(buf, str.Mid(pos, real_len));
		pos+=len;
		write_to_output(buf);
	}
}

EXPORT_C void format_datetime(TDes& buffer, const TTime& time)
{
	TDateTime dt;
	dt=time.DateTime();
	_LIT(KFormatTxt,"%04d%02d%02dT%02d%02d%02d ");
	buffer.Format(KFormatTxt, dt.Year(), (TInt)dt.Month()+1, (TInt)dt.Day()+1,
		dt.Hour(), dt.Minute(), dt.Second());
}

EXPORT_C void Mfile_output_base::write_time(const TTime& time)
{
	TBuf<20> buffer;
	format_datetime(buffer, time);
	write_to_output(buffer);
}	

EXPORT_C void Mfile_output_base::write_time()
{
	TTime t=GetTime();
	write_time(t);
}

EXPORT_C void Mfile_output_base::open_file(const TDesC& prefix, TInt* aRecentLogCount)
{
	close_file();
	if (NoSpaceLeft()) return;
	
	make_filename(current_file_name, DataDir(), prefix, iMaxLogCount, aRecentLogCount);
	User::LeaveIfError(file.Replace(Fs(), current_file_name, EFileShareAny | EFileStreamText | EFileWrite));
	file_is_open=true;
}

EXPORT_C void Mfile_output_base::close_file()
{
	if (!file_is_open) return;

	file_is_open=false;
	file.Close();
#ifndef MMC_ONLY_ON_TRANSFER
	if (has_e_drive) {
		TFileName prev=current_file_name;
		current_file_name.Replace(0, 1, _L("e"));
		if (BaflUtils::CopyFile(Fs(), prev, current_file_name) == KErrNone) {
			BaflUtils::DeleteFile(Fs(), prev);
		}
	}	
#endif
}

EXPORT_C void Mfile_output_base::make_filename(TDes& into_buffer, const TDesC& dir, const TDesC& prefix,
					       TInt aMaxLogCount, TInt* aRecentLogCount)
{
	make_filename(into_buffer, dir, prefix, _L("txt"), aMaxLogCount, aRecentLogCount);
}

EXPORT_C void Mfile_output_base::make_filename(TDes& into_buffer, const TDesC& dir, 
					       const TDesC& prefix, const TDesC& extension,
					       TInt aMaxLogCount, TInt* aRecentLogCount)
{
	if (aRecentLogCount) (*aRecentLogCount)=0;

	if (aMaxLogCount==0) {
		TTime time;
		TDateTime dt;
		time=::GetTime();
		
		dt=time.DateTime();
		_LIT(KFormatTxt,"%S%S-%04d%02d%02dT%02d%02d%02d.%S");
		into_buffer.Format(KFormatTxt, &dir, 
			&prefix, dt.Year(), (TInt)dt.Month()+1, (TInt)dt.Day()+1,
			dt.Hour(), dt.Minute(), dt.Second(), &extension);
	} else {
		RFs& fs=::GetContext()->Fs();
		_LIT(KFormatTxt,"%S%S-%d.%S");
		TTime earliest=Time::MaxTTime();
		TInt earliest_i=0;
		TTime now; now.HomeTime(); now-=TTimeIntervalMinutes(5);
		for (int i=1; i<=aMaxLogCount; i++) {
			into_buffer.Format(KFormatTxt, &dir, 
				&prefix, i,
				&extension);
			TEntry e;
			TInt err=fs.Entry(into_buffer, e);
			if (err==KErrNotFound) return;
			if (err==KErrNone) {
				if (e.iModified > now && aRecentLogCount) (*aRecentLogCount)++;
				if (e.iModified < earliest) {
					earliest=e.iModified;
					earliest_i=i;
				}
			}
		}
		if (earliest_i==0) User::Leave(KErrInUse);
		into_buffer.Format(KFormatTxt, &dir, 
			&prefix, earliest_i,
			&extension);
	}
}

EXPORT_C void Mfile_output_base::switch_file()
{ 
	open_file(*file_prefix);
}

EXPORT_C void Mfile_output_base::ConstructL(const TDesC& prefix, bool do_open_file, TInt aMaxLogCount, TInt* aRecentLogCount)
{
	iMaxLogCount=aMaxLogCount;
	has_e_drive=false;
	if (file_prefix) delete file_prefix; file_prefix=0;
	file_prefix=HBufC::NewL(prefix.Length());
	*file_prefix=prefix;
	if (do_open_file) {
		open_file(prefix, aRecentLogCount);
	}
#ifndef __WINS__
	RFs fs;
	if (fs.Connect()==KErrNone) {
		TDriveInfo i;
		if (fs.Drive(i, EDriveE)==KErrNone) {
			if (i.iType!=EMediaNotPresent &&
				i.iType!=EMediaUnknown &&
				i.iType!=EMediaCdRom &&
				i.iType!=EMediaRom) {
				// memory card
				has_e_drive=true;
			}
		}
		fs.Close();
	}	
#endif

	state=CCnvCharacterSetConverter::KStateDefault;
}

void Cfile_output_base::ConstructL(const TDesC& prefix, bool do_open_file, bool buffered_writes,
					    TInt aMaxLogCount, TInt* aRecentLogCount)
{
	Mfile_output_base::ConstructL(prefix, do_open_file, aMaxLogCount, aRecentLogCount);
	if (buffered_writes) {
		iBuffered=true;
		iBuffers=CList<HBufC8*>::NewL();
		iTimer=CTimeOut::NewL(*this);
	}
}

EXPORT_C Mfile_output_base::Mfile_output_base(MApp_context& Context): MContextBase(Context), paused(EFalse), 
	file_prefix(0), file_is_open(false) { }

EXPORT_C Mfile_output_base::~Mfile_output_base() 
{
	file.Close();
	delete file_prefix;
}

EXPORT_C Cfile_output_base::Cfile_output_base(MApp_context& Context) : Mfile_output_base(Context)
{
}

EXPORT_C Cfile_output_base::~Cfile_output_base()
{
	iDeleting=true;

	if (iBuffered) expired(iTimer);
	delete iTimer;

	delete iBuffers;
}

EXPORT_C Cfile_output_base* Cfile_output_base::NewL(MApp_context& Context, const TDesC& prefix, bool do_open_file,
						    bool buffered_writes, TInt aMaxLogCount, TInt* aRecentLogCount)
{
	Cfile_output_base* ret;
	ret=new (ELeave) Cfile_output_base(Context);
	CleanupStack::PushL(ret);
	ret->ConstructL(prefix, do_open_file, buffered_writes, aMaxLogCount, aRecentLogCount);
	CleanupStack::Pop();
	return ret;
}

EXPORT_C void Cfile_output_base::write_to_output(const TDesC& str)
{
	Mfile_output_base::write_to_output(str);
}

inline int min(int x, int y)
{
	if (x<y) return x;
	return y;
}

EXPORT_C void Cfile_output_base::write_to_output(const TDesC8& str)
{
	if (iBuffered) {
		TInt left=0;
		if (iBuffers->iCurrent) {
			HBufC8* lastbuf=iBuffers->iCurrent->Item;
			left=lastbuf->Des().MaxLength()-lastbuf->Length();
			lastbuf->Des().Append( str.Left( min(left, str.Length()) ) );
		}
		while (left < str.Length() ) {
			HBufC8* newbuf=HBufC8::NewL(256);
			newbuf->Des().Append(str.Mid(left, min(256, str.Length()-left)  ));
			left+=256;
			iBuffers->AppendL(newbuf);
		}
		iTimer->Wait(5);
	} else {
		Mfile_output_base::write_to_output(str);
	}
}

void Cfile_output_base::expired(CBase* /*source*/)
{
	if (!iBuffers || !iBuffers->iFirst) return;

	HBufC8 *b=0, *prev=0;
	while ( (b=iBuffers->Pop()) ) {
		Mfile_output_base::write_to_output(b->Des());
		delete prev;
		prev=b;
	}
	if (!iDeleting && prev) {
		prev->Des().Zero();
		iBuffers->Push(prev);
	} else {
		delete prev;
	}
}

void Cfile_output_base::expired()
{ 
	User::Leave(-1026);
}

EXPORT_C void Cfile_output_base::log_message(const TDesC8& str)
{
	write_time();
	write_to_output(str);
	write_nl();

}

EXPORT_C void Cfile_output_base::log_message(const TDesC& str)
{
	write_time();
	write_to_output(str);
	write_nl();
}
