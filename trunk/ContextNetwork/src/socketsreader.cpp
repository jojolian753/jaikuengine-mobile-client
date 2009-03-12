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

#include "Sockets.pan"
#include "SocketsReader.h"
#include "EngineNotifier.h"
#include <flogger.h>
#include "app_context.h"
#include "util.h"

const TInt CSocketsReader::KReadTimeOut = 90;

EXPORT_C CSocketsReader* CSocketsReader::NewL(MEngineNotifier& aEngineNotifier, RSocket& aSocket, MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CSocketsReader"), _CL("NewL"), &Context);
	
	CSocketsReader* self = new (ELeave) CSocketsReader(aEngineNotifier, aSocket, Context);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}


CSocketsReader::CSocketsReader(MEngineNotifier& aEngineNotifier, RSocket& aSocket, MApp_context& Context)
: CCheckedActive(EPriorityLow, _L("SocketReader")), MContextBase(Context),
iSocket(aSocket),
iEngineNotifier(aEngineNotifier),
issueRead(ETrue)
{
}

EXPORT_C CSocketsReader::~CSocketsReader()
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("~CSocketsReader"));

	Cancel();
	delete iReadTimer;
	if (iLive) *iLive=EFalse;
}


EXPORT_C void CSocketsReader::SetIssueRead(TBool issue)
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("SetIssueRead"));
	
	issueRead = issue;
}


void CSocketsReader::ConstructL()
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("ConstructL"));

	iReadTimeOut=KReadTimeOut;
	iReadTimer = CTimeOut::NewL(*this);
	CActiveScheduler::Add(this);
}

void CSocketsReader::DoCancel()
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("DoCancel"));

	iReadTimer->Reset();
	iSocket.CancelRead();
}

TInt CSocketsReader::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("CheckedRunError"));

	Log(_L("Error in CSocketsReader::RunL "), aError);
	iEngineNotifier.ReportError(MEngineNotifier::EUnknownError, aError);
	return KErrNone;
}

void CSocketsReader::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("expired"));
	
	Cancel();
	iEngineNotifier.ReportError(MEngineNotifier::ETimeOutOnRead, 0);
}

void CSocketsReader::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("CheckedRunL"));

	iReadTimer->Reset();
	switch (iStatus.Int())
        {
        case KErrNone:
		//Log(iBuffer);
		iHasRead=ETrue;
		iEngineNotifier.UpdateByteCount(iBuffer.Length(), 0);

		TBool live; iLive=&live; live=ETrue;
		iEngineNotifier.ResponseReceived(iBuffer);
		if (live && issueRead) { 
			IssueRead();
			iLive=0;
		}
		break;
		
        case KErrDisconnected:
		iEngineNotifier.ReportError(MEngineNotifier::EDisconnected,iStatus.Int());
		break;
		
        default:
		iEngineNotifier.ReportError(MEngineNotifier::EGeneralReadError,iStatus.Int());
		break;
        }	
}

void CSocketsReader::IssueRead()
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("IssueRead"));

	__ASSERT_ALWAYS(!IsActive(), User::Panic(KPanicSocketsEngineRead, ESocketsBadState));
	if (iReadTimeOut>0)
		iReadTimer->Wait(iReadTimeOut);
	iStatus=KRequestPending;
	SetActive();
	iSocket.RecvOneOrMore(iBuffer, 0, iStatus, iDummyLength);
}

EXPORT_C void CSocketsReader::NoTimeout()
{
	if (iHasRead)
		iReadTimer->Reset();
	iReadTimeOut=0;
}

void CSocketsReader::Timeout()
{
	iReadTimeOut=KReadTimeOut;
	if (IsActive() && ! iReadTimer->IsActive()) iReadTimer->Wait(iReadTimeOut);
	iHasRead=EFalse;
}

EXPORT_C void CSocketsReader::Start()
{
	CALLSTACKITEM_N(_CL("CSocketsReader"), _CL("Start"));
	iReadTimeOut=KReadTimeOut;
	if (!IsActive()) IssueRead();
}
