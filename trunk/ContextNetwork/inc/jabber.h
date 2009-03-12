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

#ifndef __JABBER_H__
#define __JABBER_H__

#include "socketsengine.h"
#include "app_context.h"
#include "expat.h"
#include "list.h"

#ifdef __S60V2__
#include <zlib.h>
#else
// From Symbian FAQ
#include <ezlib.h>
#endif
#include "bbdata.h"

class MJabberObserver
{
public:
    enum TJabberStatus
        {
        EJabberConnected=0,
        EJabberDisconnected=1,
        EIdentificationFailed=10,
        EConnectionNotAllowed=11,
        EJabberPendingSuspend=12
    };

    virtual void NotifyJabberStatus(TInt st)=0;
    virtual void NotifyNewPresenceInfo(const TDesC & from, const TDesC & info, const TTime& stamp)= 0;
    virtual void NotifyNewMessage(const TDesC & from, const TDesC& subject, const TDesC & message) = 0;
    virtual void NotifyMessageSent() = 0;
    virtual void NotifyContextObject(MBBData* aObject, TInt aError, const TDesC& aName) = 0;
    virtual void NotifyCanWrite() = 0;
    virtual void NotifyError(const class MErrorInfo* aError) = 0;
};

class CJabber : public CBase, public MContextBase, public MTimeOut, public MEngineObserver, public MNestedXmlHandler
{
public:
    IMPORT_C static CJabber * NewL(MJabberObserver& aObserver, MApp_context& Context, const TDesC& aDeviceId);
    IMPORT_C ~CJabber();

    // Connect to a Jaiku mobile's jabber server.
    // Note: Server is not anymore used, all user accounts are for jaiku.com
    // domain and a host is hardcoded to xmpp.x.jaiku.com or read from
    // settings in development builds.
    IMPORT_C void ConnectL(const TDesC16 &u,
                           const TDesC16 &p,
                           const TDesC16& /*ignored*/,
                           const TUint32 iAccessPoint,
                           TBool aOnlyWhenActive);
    IMPORT_C void Disconnect(TBool close_connection);
    IMPORT_C TBool IsConnected();
    IMPORT_C void SendPresenceInfoL(const TDesC &presenceInfo, TBool aQuickRate=EFalse);
    IMPORT_C TBool SuspendL(); // only does on custom proxy protocol
    IMPORT_C TBool ResumeL();  // only does on custom proxy protocol
    IMPORT_C void SendMessageL(const TDesC &aTo, const TDesC& aSubject, const TDesC& aMessage);
    IMPORT_C void AckObjectL(TUint aId);
    IMPORT_C void SendContextObjectL(const class MBBData* aObject);
    IMPORT_C void ReadTimeout();
    IMPORT_C void NoReadTimeout();
private:
    void expired(CBase* Source);

    virtual void NotifyEngineStatus(TInt st, TInt aError);
    virtual void NotifyNewData(const TDesC8& aBuffer);
    virtual void NotifyCanWrite();

    void ConstructL();
    CJabber(MJabberObserver& aObserver,MApp_context& Context, const TDesC& aDeviceId);
    enum TJabberError {EIdentificationFailed, EStreamError, EXmlParseError, ENetworkError, EConnectionNotAllowed};
    void ReportError(TInt aErrorType, const TDesC& aExtra=KNullDesC, TBool aFromDisconnect=EFalse);
    void DoDisconnect(TBool close_connection);

    void StartElement (const XML_Char *el, const XML_Char **atts);
    void EndElement (const XML_Char *name);
    void CharacterData (const XML_Char *s, int len);
    virtual void Error(XML_Error Code, const XML_LChar * String, long ByteIndex);
    virtual void SetError(TInt aError);

private:
    void SendXMLStreamHeaderL();
    void IdentifyL();
    void SendDisconnectionL();
    void DoSendPresenceInfoL(TBool aKeepAlive=EFalse);
private:
    MJabberObserver& iObserver;

private:
    static const TInt KDefaultPortNumber;
    static const TInt KCloseSessionTimeOut;

        CSocketsEngine * iEngine;

    enum TJabberState { ENotConnected, EConnecting, EInitiating, EIdentifying, EConnected, EDisconnecting, EWaitingForRetry };

    TJabberState iJabberStatus;
    void ChangeStatus(TJabberState aNewStatus, TInt aError);

    TInt                          iPort;
    // Two different servername's, because header has to be "jaiku.com" although we connect to kaksi.org
    // iHost is used as an argument for iEngine
    TBuf16<KMaxServerNameLength>  iHost;
    // iServerName is used in xml header
    TBuf16<KMaxServerNameLength>  iServerName;

    TUint32               iAccessPoint;
    TBuf16<200> iUsername;  // size defined by Jabber specs
public:
    TBuf16<200> iFullNick;  // user@server/Resource
private:
    TBuf16<200> iPassword;  // size defined by Jabber specs
    TBuf16<100> iResource;
    TBuf16<100> iJabberSessionId;

    CXmlBuf16*      iXmlBuf;
    CXmlParser*     iParser;
    CList<TInt> * iStack;
    enum TParseState
    {   EStackUndefined,
        EStackIgnoreElement,
        EStackPresenceInfo,
        EStackMessage,
        EStackSubject,
        EStackBody,
        EStackOfflinePresenceInfo,
        EStackSession,
        EStackConnected,
        EStackIdentFailure,
        EStackStatus,
        EStackStreamError,
        EStackError,
        EStackContextXml
    };

    TBuf<20> iErrorCode;
    TBuf<50> iDeviceId;
    HBufC16 * iErrorValue;

    HBufC16 * iUserPresenceInfo;
    HBufC16 * iFrom;
    HBufC16 * iPresenceInfo;
    HBufC16 * iMessage;
    HBufC16 * iSubject;

    CTimeOut*       iSendTimer;
    CTimeOut*       iCloseOrConnectTimer;

    TTimeIntervalSeconds iPresenceInterval;
    TInt iFreshnessInterval;
    TTime iLastUpdateSent;  TBool iFirstAfterResume;

    TBool restart;
    TBool iCloseConnection;
    TBool iCustomProtocol;
    TBool iSuspended, iToSuspend, iNewPresence, iSendingMessage;

    friend class CContextServer;
    MNestedXmlHandler* iCurrentContextXml;
    MBBData* iCurrentContextObject;
    HBufC* iObjectBuf;
    TInt iContextError;
    class CXmlBufExternalizer* iOutgoingObjectBuf;
    TBool iOnlyWhenActive;
};

#endif
