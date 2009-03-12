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
#include <e32base.h>
#include <flogger.h>
#include <e32svr.h>
#include "app_context.h"

void LogL(const TDesC& msg)
{
	RFileLogger iLog;
	User::LeaveIfError(iLog.Connect());
	CleanupClosePushL(iLog);
	iLog.CreateLog(_L("Context"),_L("Notify"),EFileLoggingModeAppend);
	RDebug::Print(msg.Left(128));
	
	iLog.CloseLog();
	CleanupStack::PopAndDestroy();
}

void Log(const TDesC& msg)
{
	CC_TRAPD(err, LogL(msg));
	// ignore error, not much else we can do
	// and not critical
}
void LogL(const TDesC8& msg)
{
	RFileLogger iLog;
	User::LeaveIfError(iLog.Connect());
	CleanupClosePushL(iLog);
	iLog.CreateLog(_L("Context"),_L("Notify"),EFileLoggingModeAppend);
	iLog.Write(msg);
	//RDebug::Print(msg);
	
	// Close the log file and the connection to the server.
	iLog.CloseLog();
	CleanupStack::PopAndDestroy();
}

void Log(const TDesC8& msg)
{
	CC_TRAPD(err, LogL(msg));
	// ignore error, not much else we can do
	// and not critical
}

void LogL(const TDesC& msg, TInt i)
{
	RFileLogger iLog;
	User::LeaveIfError(iLog.Connect());
	CleanupClosePushL(iLog);
	iLog.CreateLog(_L("Context"),_L("Notify"),EFileLoggingModeAppend);
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

void Log(const TDesC& msg, TInt /*i*/)
{
	CC_TRAPD(err, LogL(msg));
	// ignore error, not much else we can do
	// and not critical
}

