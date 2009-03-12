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

#ifndef CONTEXT_BBL_PROTOCOL_H_INCLUDED
#define CONTEXT_BBL_PROTOCOL_H_INCLUDED 1

#include "enginenotifier.h"
#include "app_context.h"
#include "checkedactive.h"
#include "csd_event.h"
#include "socketslistener.h"
#include "status_notif.h"

class MBBLocalNotifier {
public:
	virtual void Acked(TUint id) = 0;
	virtual void IncomingTuple(const CBBTuple* aTuple) = 0;
	virtual void Error(TInt aError, TInt aOrigError, const TDesC& aDescr) = 0;
	virtual void ReadyToWrite(TBool aReady) = 0;
	virtual void Disconnected() = 0;
};

class MBBLocalProtocolOwner {
public:
	virtual void ProtocolDeleted(class CBBLocalProtocol* aProtocol) = 0;
};

class CBBLocalProtocol : public CBase, public MSocketAcceptor {
public:
	IMPORT_C static CBBLocalProtocol* NewL(MApp_context& aContext, MBBLocalProtocolOwner& aOwner);
	CBBLocalProtocol();

	virtual void AddObserverL(MBBLocalNotifier* aObserver) = 0;

	virtual void Disconnect(TBool closeConnection) = 0;

	virtual void WriteL(const TDesC16& aData) = 0;

	virtual void SendXMLStreamHeaderL() = 0;

	virtual void SendDisconnectionL() = 0;
	virtual void SendXmlPacketL(const TDesC& aPacket) = 0; // data has to live until acked

	virtual void Read() = 0;
	virtual TBool Connected() const = 0;
};

#endif
