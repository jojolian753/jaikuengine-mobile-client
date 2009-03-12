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

#include "phonehelper.h"
#include "app_context.h"

void phonehelper::ConstructL()
{
	CALLSTACKITEM(_L("phonehelper::ConstructL"));

#ifndef __WINS__
	iStatus=KErrNone;
	
	User::LeaveIfError( server.Connect() );

	// load a phone profile
	_LIT(KGsmModuleName, "phonetsy.tsy");
	User::LeaveIfError( server.LoadPhoneModule( KGsmModuleName ) );

	// initialize the phone object
	RTelServer::TPhoneInfo info;
	User::LeaveIfError( server.GetPhoneInfo( 0, info ) );
	User::LeaveIfError( phone.Open( server, info.iName ) );

        RPhone::TLineInfo lineinfo;
        phone.GetLineInfo(0, lineinfo);
        line.Open(phone, lineinfo.iName);

#endif
	CActiveScheduler::Add(this); 
}

phonehelper::phonehelper() : CCheckedActive(EPriorityIdle, _L("phonehelper"))
{
	CALLSTACKITEM(_L("phonehelper::phonehelper"));

}

void phonehelper::CheckedRunL()
{
	CALLSTACKITEM(_L("phonehelper::CheckedRunL"));

	call.Close();
}

void phonehelper::DoCancel()
{
	CALLSTACKITEM(_L("phonehelper::DoCancel"));

	call.DialCancel();
}

phonehelper::~phonehelper()
{
	CALLSTACKITEM(_L("phonehelper::~phonehelper"));

	Cancel();

#ifndef __WINS__
	call.Close();

	line.Close();
	_LIT(KGsmModuleName, "phonetsy.tsy");
	phone.Close();
	server.UnloadPhoneModule( KGsmModuleName );

	server.Close();
#endif
}

RCall::TStatus phonehelper::line_status()
{
	CALLSTACKITEM(_L("RCall::TStatus"));

	RLine::TLineInfo lineinfo;
	#ifndef __WINS__
		line.GetInfo(lineinfo);
	#else
		lineinfo.iStatus=RCall::EStatusIdle;
	#endif
	return lineinfo.iStatus;
}

TInt phonehelper::make_call(const TDesC& number)
{
	CALLSTACKITEM(_L("phonehelper::make_call"));

	// The status isn't reliable if no calls
	// have been made yet after startup
	/*
	if (line_status()!=RCall::EStatusIdle) {
		return -1;
	}

	*/

#ifndef __WINS__
	if (IsActive()) return -2;

	TInt ret=call.OpenNewCall(line);
	if (ret==KErrNone) {
		call.Dial(iStatus, number);
		SetActive();
	}
	return ret;
#else
	return KErrNone;
#endif
}
