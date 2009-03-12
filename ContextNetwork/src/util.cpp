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

#include "break.h"
#include <flogger.h>
#include <E32SVR.H>
#include "app_context.h"
#include "concretedata.h"
#include <apgcli.h>
#include <apmstd.h>

void ConvertTo16(const TDesC8& msg, TDes16& msg16)
{
	TBuf<2> hex;
	int i=0;
	for (i=0; (i<msg.Length() && msg16.Length()<msg16.MaxLength()-4); i++) {
		unsigned char c=msg[i];
		if ( (c>32 && c<128) || c==10 || c==13 || c==' ' || c=='\t') {
			msg16.Append(c);
		} else {
			hex.Zero();
			//if (c<16) hex.Append(_L("0"));
			hex.AppendNumFixedWidth(c, EHex, 2);
			msg16.Append(_L("\\x"));
			msg16.Append(hex);
		}
	}
}

EXPORT_C void LogL(const TDesC& msg)
{

	RFileLogger iLog;
	User::LeaveIfError(iLog.Connect());
	CleanupClosePushL(iLog);
	iLog.CreateLog(_L("Context"),_L("Network"),EFileLoggingModeAppend);
	TInt i=0;
#ifdef __WINS__
	while (i<msg.Length()) {
#else
	{
#endif
		TInt len;
		len=msg.Length()-i;
		if (len>128) len=128;
		RDebug::Print(msg.Mid(i, len));
		iLog.Write(msg.Mid(i, len));
		i+=128;
	}
	
	iLog.CloseLog();
	CleanupStack::PopAndDestroy();
}

EXPORT_C void Log(const TDesC& msg)
{

	CC_TRAPD(err, LogL(msg));
	// ignore error, not much else we can do
	// and not critical
}
EXPORT_C void LogL(const TDesC8& msg)
{

	RFileLogger iLog;
	User::LeaveIfError(iLog.Connect());
	CleanupClosePushL(iLog);
	iLog.CreateLog(_L("Context"),_L("Network"),EFileLoggingModeAppend);
	TInt i=0;
#ifdef __WINS__
	while (i<msg.Length()) {
#else
	{
#endif
		TInt len=msg.Length()-i;
		if (len > 32) len=32;

		#ifdef __WINS__
			TBuf<128> msg16;
			ConvertTo16(msg.Mid(i, len), msg16);
			//RDebug::Print(msg16.Left(128)); // some unidentified(yet!) char makes RDebug crash :(
		#endif

		iLog.Write(msg.Mid(i, len));
		i+=32;
	}
	
	// Close the log file and the connection to the server.
	iLog.CloseLog();
	CleanupStack::PopAndDestroy();
}

EXPORT_C void Log(const TDesC8& msg)
{

	CC_TRAPD(err, LogL(msg));
	// ignore error, not much else we can do
	// and not critical
}

EXPORT_C void LogL(const TDesC& msg, TInt i)
{

	RFileLogger iLog;
	User::LeaveIfError(iLog.Connect());
	CleanupClosePushL(iLog);
	iLog.CreateLog(_L("Context"),_L("Network"),EFileLoggingModeAppend);
	iLog.Write(msg);
	RDebug::Print(msg.Left(128));
	TBuf<10> iBuf;
	iBuf.Format(_L("%d"),i);
	iLog.Write(iBuf);
	RDebug::Print(iBuf);
	// Close the log file and the connection to the server.
	iLog.CloseLog();
	CleanupStack::PopAndDestroy();
}

EXPORT_C void Log(const TDesC& msg, TInt i)
{

	CC_TRAPD(err, LogL(msg, i));
	// ignore error, not much else we can do
	// and not critical
}

EXPORT_C void GetMimeTypeL(const TDesC& FileName, TDes& aBuf)
{
	CALLSTACKITEM_N(_CL("CHttpTransfer2Impl"), _CL("GetMimeTypeL"));

	aBuf.Zero();
	aBuf.Append(_L("application/octect-stream"));

	TUid appUid(KNullUid);
	RApaLsSession ls;
	User::LeaveIfError(ls.Connect());
	TDataType dataType;
	TInt err = ls.AppForDocument(FileName, appUid, dataType);
	if ((err==KErrNone) && (dataType.Des().Length()!=0)){
		aBuf.Zero();	
		aBuf.Append(dataType.Des());
	}
	ls.Close();
	RDebug::Print(_L("file: %S"), &FileName);
	RDebug::Print(_L("mime: %S"), &aBuf);
}

EXPORT_C void ToPacket(const TDesC& filen, TDes& packet)
{
	CALLSTACKITEM_N(_CL("RDebug"), _CL("Print"));

	TInt dirpos=filen.LocateReverse('\\');
	packet=filen.Left(dirpos);
	packet.Append(_L("\\context"));
	packet.Append(filen.Mid(dirpos));
	TInt extpos=packet.LocateReverse('.');
	if (extpos!=KErrNotFound) {
		packet[extpos]='_';
	}
	packet.Append(_L(".xml"));
}

EXPORT_C void ToDel(const TDesC& filen, TDes& del)
{
	CALLSTACKITEM_N(_CL("TDummyPrompt"), _CL("TDummyPrompt"));

	TInt dirpos=filen.LocateReverse('\\');
	del=filen.Left(dirpos);
	del.Append(_L("\\context"));
	del.Append(filen.Mid(dirpos));

	TInt extpos=del.LocateReverse('.');
	if (extpos!=KErrNotFound) {
		del[extpos]='_';
	}
	del.Append(_L(".del"));
}


