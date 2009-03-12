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

#include "bt.h"

_LIT(pfx, "comm");
_LIT(KServerTransportName,"RFCOMM");
_LIT(KDir, "c:\\system\\apps\\context_log\\log*.txt");

#define KBTServiceSerial 0x1101
#define KBTServiceOBEX 0x1105

Ctransfer::Ctransfer() : CActive(EPriorityIdle), file_output_base(),
current_state(IDLE), cb(0), targetServiceClass(0x2345),
agent(0), list(0), obex_client(0), dir(0), current_object(0)
{
}

void Ctransfer::ConstructL(i_discover_notif* callback) 
{
	file_output_base::ConstructL(pfx, false);
	
	CActiveScheduler::Add(this); // add to scheduler
	
	cb=callback;
}

bool Ctransfer::transfer_logs()
{
	if (current_state!=IDLE) return false;
	
	TInt ret;
	if ((ret=not.Connect())!=KErrNone) {
		TBuf<30> msg;
		_LIT(err_connect, "Error connecting to notifier %d");
		msg.Format(err_connect, ret);
		cb->error(msg);
		return false;
	}
	selectionFilter.SetUUID(targetServiceClass);
	not.StartNotifierAndGetResponse(iStatus, KDeviceSelectionNotifierUid, pckg, result);
	
	SetActive();
	
	current_state=SELECTING;
	return true;
}

void Ctransfer::release()
{
	not.Close();
	
	delete agent; agent=0;
	delete list; list=0;
	
	delete obex_client; obex_client=0;
	delete dir; dir=0;
	if (current_object) {
		current_object->Reset();
		delete current_object; current_object=0;
	}
	delete buf; buf=0;
}

Ctransfer::~Ctransfer()
{
	Cancel();
	
	release();
}

void Ctransfer::DoCancel()
{
	finished();
}

void Ctransfer::finished()
{
	if (current_state==IDLE) return;
	
	if (cb) cb->finished();
	
	current_state=IDLE;
}

void Ctransfer::get_service()
{
	agent = CSdpAgent::NewL(*this, result().BDAddr());
	list = CSdpSearchPattern::NewL();
	list->AddL(KBTServiceOBEX);
	agent->SetRecordFilterL(*list);
	agent->NextRecordRequestL();
}

bool Ctransfer::send_next_file()
{
	if (current_object) {
		cb->status_change(_L("got object"));
		return false;
	}

	TInt err_code;
	TBuf<100> msg;

	buf=CBufFlat::NewL(1024);

	TRAP(err_code,
		current_object=CObexBufObject::NewL(buf));
	
	if (err_code!=KErrNone) {
		_LIT(f, "NewL: %d");
		msg.Format(f, err_code);
		cb->error(msg);
		release();
		return false;
	}
	
	current_object->SetNameL(_L("telecom/pb.vcf"));

	TRAP(err_code, obex_client->Get(*current_object, iStatus));
	
	if (err_code!=KErrNone) {
		_LIT(f, "Put: %d");
		msg.Format(f, err_code);
		cb->error(msg);
		release();
		return false;
	}
		
	current_state=SENDING_FILE;
	SetActive();
	
	msg=(TText*)L"SetActive()";
	cb->status_change(msg);
	return true;
}

void Ctransfer::send_files()
{	
	TInt ret;
	send_next_file();
}

void Ctransfer::RunL()
{
	TBuf<30> msg;
	if (iStatus.Int()!=KErrNone) {
		_LIT(stat_err, "error: %d");
		msg.Format(stat_err, iStatus.Int());
		cb->error(msg);
		current_state=IDLE;
		release();
		return;
	}

	switch(current_state) {
	case SELECTING:
		if(!result().IsValidBDAddr()) {
			msg=(TText*)L"cancelled";
			cb->status_change(msg);
			//cb->finished();
			current_state=IDLE;
		} else {
			msg=(TText*)L"selected ";
			msg.Append(result().DeviceName());
			cb->status_change(msg);
			port=0; seen_rfcomm=false;
			get_service();
		}
		not.CancelNotifier(KDeviceSelectionNotifierUid);
		not.Close();
		break;
	case CONNECTING:
		msg=(TText*)L"connected, sending files";
		cb->status_change(msg);
		send_files();
		//socket.Close();
		break;
	case SENDING_FILE:
		if (!send_next_file()) {
			//cb->finished();
			release();
		}
		break;
	case IDLE:
	case GETTING_SERVICE:
		msg.Format(_L("Unexpected state %d"), current_state);
		//cb->error(msg);
		break;
	}
}

void Ctransfer::AttributeRequestComplete(TSdpServRecordHandle /*aHandle*/, TInt /*aError*/)
{
	if (port==0) {
		_LIT(err, "didn't find service");
		cb->error(err);
		current_state=IDLE;
	} else {
		_LIT(f, "found port %d");
		TBuf<30> msg;
		msg.Format(f, port);
		cb->status_change(msg);
		connect_to_service();
	}
}

void Ctransfer::AttributeRequestResult(TSdpServRecordHandle /*aHandle*/, 
				      TSdpAttributeID /*aAttrID*/, CSdpAttrValue* aAttrValue)
{
	
	if (aAttrValue) 
		aAttrValue->AcceptVisitorL(*this);	
}

void Ctransfer::VisitAttributeValueL(CSdpAttrValue &aValue, TSdpElementType aType)
{
	if (aType==ETypeUUID) {
		if (aValue.UUID()==KRFCOMM) {
			seen_rfcomm=true;
		} else {
			seen_rfcomm=false;
		}
	} else if (aType==ETypeUint && seen_rfcomm) {
		port=aValue.Uint();
	}
}

void Ctransfer::StartListL(CSdpAttrValueList &/*aList*/)
{
}

void Ctransfer::EndListL()
{
}

void Ctransfer::connect_to_service()
{
	/*
	TBTSockAddr address;
	
	address.SetBTAddr(result().BDAddr());
	address.SetPort(port);
	
	socket.Connect(address, iStatus);
	*/
	
	TObexBluetoothProtocolInfo protocolInfo;

	protocolInfo.iTransport.Copy(KServerTransportName);
	protocolInfo.iAddr.SetBTAddr(result().BDAddr());
	protocolInfo.iAddr.SetPort(port);
	
	delete obex_client;
	obex_client = CObexClient::NewL(protocolInfo);

	obex_client->Connect(iStatus);
	
	current_state=CONNECTING;
	SetActive();
}


void Ctransfer::NextRecordRequestComplete(TInt aError, 
					 TSdpServRecordHandle aHandle, TInt aTotalRecordsCount)
{
	TBuf<30> msg;
	if (aError!=KErrNone && aError!=KErrEof) {
		_LIT(err, "service error: %d");
		msg.Format(err, aError);
		cb->error(msg);
	} else if (aError==KErrEof) {
		_LIT(err, "disconnected");
		cb->error(err);
	} else if(aTotalRecordsCount==0) {
		_LIT(err, "no Serial service");
		cb->error(err);
	} else {
		_LIT(st, "found service");
		cb->status_change(st);
		agent->AttributeRequestL(aHandle, KSdpAttrIdProtocolDescriptorList);
		return;
	}
	current_state=IDLE;
}
