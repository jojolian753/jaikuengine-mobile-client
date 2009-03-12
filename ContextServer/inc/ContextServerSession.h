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

#ifndef __CCONTEXTSERVERSESSION__
#define __CCONTEXTSERVERSESSION__

#include <e32base.h>
#include "ContextServer.h"
#include "lookup_notif.h"
#include "compat_server.h"

#ifndef __IPCV2__
class CContextServerSession : public CSession
#else
class CContextServerSession : public SESSION_CLASS
#endif
{

public: 
#ifndef __IPCV2__
	static CContextServerSession* NewL(RThread& aClient, CContextServer& aServer);
#else
	static CContextServerSession* NewL(CContextServer& aServer);
#endif
	~CContextServerSession();
	
	void ServiceL(const MESSAGE_CLASS& aMessage);

	void SendPresenceInfo(TDesC & presenceInfo);
	void Exiting();

private:
#ifndef __IPCV2__
	CContextServerSession(RThread& aClient, CContextServer& aServer);
#else
	CContextServerSession(CContextServer& aServer);
#endif
	void ConstructL() ;
	void PanicClient(const MESSAGE_CLASS& aMessage, TInt aPanic) const;

	void ConnectToPresenceServer(const MESSAGE_CLASS & aMessage);
	void DisconnectFromPresenceServer(const MESSAGE_CLASS & aMessage);
	void TerminateContextServer(const MESSAGE_CLASS & aMessage);
	void HandleRequestPresenceInfo(const MESSAGE_CLASS& aMessage);
	void HandleRequestPresenceNotification(const MESSAGE_CLASS & aMessage);
	void UpdateUserPresence(const MESSAGE_CLASS & aMessage);
	void SuspendConnection(const MESSAGE_CLASS & aMessage);
	void ResumeConnection(const MESSAGE_CLASS & aMessage);
	void HandleRequestMessageNotification(const MESSAGE_CLASS& aMessage);

	void CompleteMessage(TInt Code);
	void SendNewMessage(TMessage aJabberMessage);

public: // from MContextServerNotifier
	void NotifyEvent(CContextServer::TEvent aEvent);
	void ReportError(TContextServRqstComplete aErrorType, const TDesC & aErrorCode, const TDesC & aErrorValue);
private: 
	
	MESSAGE_CLASS       iMessage; TInt iMessageThreadId;
	void SetMessage(const MESSAGE_CLASS & aMsg);
	CContextServer&     iServer;
	RArray<TInt>	    iFlags;
	CLookupHandler* iLookupHandler;
	TInt				iLastEvent;
	TBool				iExiting;
};

#endif

