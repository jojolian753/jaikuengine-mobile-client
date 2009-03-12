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

#include "SocketsWriter.h"
#include "Sockets.pan"
#include "EngineNotifier.h"
#include <charconv.h>
#include <flogger.h>
#include "app_context.h"

#include "util.h"

const TInt CSocketsWriter::KTimeOut = 90; // 90 seconds time-out

EXPORT_C CSocketsWriter* CSocketsWriter::NewL(MEngineNotifier& aEngineNotifier, RSocket& aSocket, MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CSocketsWriter"), _CL("NewLC"), &Context);

	CSocketsWriter* self = new (ELeave) CSocketsWriter(aEngineNotifier, aSocket, Context);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CSocketsWriter::CSocketsWriter(MEngineNotifier& aEngineNotifier, RSocket& aSocket, MApp_context& Context)
: CCheckedActive(EPriorityLow, _L("SocketWriter")), MContextBase(Context),
iSocket(aSocket),
iEngineNotifier(aEngineNotifier) { }


CSocketsWriter::~CSocketsWriter()
{
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("~CSocketsWriter"));

	Cancel();
	delete iTimer;
	delete iTransferBuffer;
	delete iWriteBuffer;
}

void CSocketsWriter::DoCancel()
{	
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("DoCancel"));

	iSocket.CancelWrite();
	iTimer->Reset();
	iWriteStatus = EWaiting;
}

void CSocketsWriter::ConstructL()
{
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("ConstructL"));

	CActiveScheduler::Add(this);
	
	iTimeOut = KTimeOut; 
	iTimer = CTimeOut::NewL(*this);
	iWriteStatus = EWaiting;
	
	iTransferBuffer=HBufC8::NewL(1024);
	iWriteBuffer=HBufC8::NewL(1024);

}

void CSocketsWriter::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("expired"));

	Cancel();
	iWriteStatus = EWaiting;
	iEngineNotifier.ReportError(MEngineNotifier::ETimeOutOnWrite, KErrTimedOut);
}

TInt CSocketsWriter::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("CheckedRunError"));
	
	Log(_L("Error in CSocketsReader::RunL %d"), aError);
	return aError;
}

void CSocketsWriter::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("CheckedRunL"));

	iTimer->Cancel();

	if (iStatus == KErrNone)
	{
		switch(iWriteStatus)
		{
		case ESending:
			SendNextPacket();
			iEngineNotifier.CanWrite();
			break;
		default:
			User::Panic(KPanicSocketsEngineWrite, ESocketsBadStatus);
			break;
		};
	}
	else 
	{
		iWriteStatus = EWaiting;
		iEngineNotifier.ReportError(MEngineNotifier::EGeneralWriteError, iStatus.Int());
	}
}

EXPORT_C void CSocketsWriter::IssueWriteL(const TDesC8& aData)
{
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("IssueWriteL"));

	if ((iWriteStatus != EWaiting) && (iWriteStatus != ESending))
        {
		User::Leave(KErrNotReady);
        }
	while ((aData.Size() + iTransferBuffer->Des().Length()) > iTransferBuffer->Des().MaxLength())
        {
		iTransferBuffer=iTransferBuffer->ReAllocL(iTransferBuffer->Des().MaxLength()*2);
        }
	iTransferBuffer->Des().Append(aData);
	if (!IsActive()) SendNextPacket();
}

void CSocketsWriter::SendNextPacket()
{
	CALLSTACKITEM_N(_CL("CSocketsWriter"), _CL("SendNextPacket"));
	
	if (iTransferBuffer->Length() > 0)
        {
		if (!iWriteBuffer ||  iWriteBuffer->Des().MaxLength() < iTransferBuffer->Length() ) {
			delete iWriteBuffer; iWriteBuffer=iTransferBuffer->Des().AllocL();
		} else {
			*iWriteBuffer=*iTransferBuffer;
		}
		iTransferBuffer->Des().Zero();
		iEngineNotifier.UpdateByteCount(0, (*iWriteBuffer).Length());
		iStatus=KRequestPending;
		SetActive();
		iSocket.Write(*iWriteBuffer, iStatus); // Initiate actual write
		iTimer->Wait(iTimeOut);
		iWriteStatus = ESending;
        }
	else
        {
		iWriteStatus = EWaiting;
        }
}
