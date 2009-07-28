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
#include "jabber.h"
#include "sha1.h"
#include <flogger.h>
#include "concretedata.h"
#include <e32svr.h>
#include "compat_int64.h"
#include "cl_settings.h"
#include "bbxml.h"
#include "errorinfo.h"
#include "errorhandling.h"
#include "cn_networkerror.h"

#include "contextvariant.hrh"

const TInt CJabber::KDefaultPortNumber = 5224;
const TInt CJabber::KCloseSessionTimeOut = 2;
_LIT(KDefaultJabberRessource, "Context");
_LIT(KDefaultServerName, "jaiku.com");
_LIT(KDefaultHost, "jaiku-fe.mikie.iki.fi");

_LIT(KContextNS, "http://www.cs.helsinki.fi/group/context");
_LIT(KStream,"http://etherx.jabber.org/streams stream");
_LIT(KStreamError, "http://etherx.jabber.org/streams error");
_LIT(KIq, "jabber:client iq");
_LIT(KPresence, "jabber:client presence");
_LIT(KMessage, "jabber:client message");
_LIT(KStatus, "jabber:client status");
_LIT(KBody, "jabber:client body");
_LIT(KSubject, "jabber:client subject");

_LIT(KServerTime, "http://www.cs.helsinki.fi/group/context servertime");

_LIT(KId, "id");
_LIT(KType, "type");
_LIT(KResult, "result");
_LIT(KFrom, "from");
_LIT(KError, "error");
_LIT(KCode, "code");
_LIT(KUnavailable, "unavailable");

#include "util.h"

// HACKHACK
EXPORT_C CJabber * CJabber::NewL(MJabberObserver& aObserver, MApp_context &Context, const TDesC& aDeviceId)
{
    CJabber* self = new (ELeave) CJabber(aObserver, Context, aDeviceId);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
}

CJabber::CJabber(MJabberObserver& aObserver, MApp_context& Context, const TDesC& aDeviceId):
        MContextBase(Context), MNestedXmlHandler(0), iObserver(aObserver), iPort(KDefaultPortNumber),
        iServerName(KDefaultServerName), iHost(KDefaultHost),
        iResource(KDefaultJabberRessource), restart(ETrue)
{
#ifdef __WINS__
    iPresenceInterval=5;
    iFreshnessInterval=60;
#else
    //iPresenceInterval=60;
    //iFreshnessInterval=4*60;
    //iPresenceInterval=120;
    //iFreshnessInterval=8*60;
    //iPresenceInterval=10*60;
    //iFreshnessInterval=20*60;
    iPresenceInterval=4*60;
    iFreshnessInterval=8*60;
#endif
    iDeviceId=aDeviceId;
}

void CJabber::ConstructL()
{
    iStack = CList<TInt>::NewL();
    iXmlBuf=CXmlBuf16::NewL(256);

    iErrorValue = HBufC::NewL(64);

    iFrom = HBufC::NewL(64);
    iPresenceInfo=HBufC::NewL(512);
    iMessage=HBufC::NewL(512);
    iSubject=HBufC::NewL(128);
    iUserPresenceInfo=HBufC::NewL(512);

    iSendTimer = CTimeOut::NewL(*this);
    iCloseOrConnectTimer = CTimeOut::NewL(*this);
    ChangeStatus(ENotConnected, KErrNone);
    iOutgoingObjectBuf=CXmlBufExternalizer::NewL(2048);
    iOutgoingObjectBuf->SetDefaultNameSpaceL(KContextNS);
}

void CJabber::ChangeStatus(TJabberState aNewStatus, TInt aError)
{
    switch (aNewStatus)
        {
        case ENotConnected:
        Log(_L("Jabber::Not connected"), aError);
        break;

        case EConnecting:
        Log(_L("Jabber::Connecting"), aError);
        break;

    case EConnected:
        if (iCustomProtocol) {
            TBuf<100> clientversion;
            clientversion=_L("<c:clientversion value='7'");
            if (iDeviceId.Length()) {
                clientversion.Append(_L(" device_id='"));
                clientversion.Append(iDeviceId);
                clientversion.Append(_L("'"));
            }
            clientversion.Append(_L("/>"));
            iEngine->WriteL(clientversion);
        }
        iFirstAfterResume=EFalse;
        iEngine->NoReadTimeout();
        CNetworkError::ConnectionSuccessL();

        Log(_L("Jabber::Connected"), aError);
        break;

    case EInitiating:
        Log(_L("Jabber::Initiating"), aError);
        break;

        case EIdentifying:
        Log(_L("Jabber::Identifying"), aError);
        break;

    case EDisconnecting:
        Log(_L("Jabber::Disconnecting"), aError);
        break;

    case EWaitingForRetry:
        Log(_L("Jabber::Waiting For Retry"), aError);
        break;

        default:
        Log(_L("Jabber::Unknown status"), aError);
        break;
        }
    iJabberStatus = aNewStatus;
}

EXPORT_C CJabber::~CJabber()
{
    delete iFrom;
    delete iPresenceInfo;
    delete iMessage;
    delete iSubject;
    delete iUserPresenceInfo;
    delete iErrorValue;
    delete iXmlBuf;
    delete iParser;
    delete iStack;
    delete iEngine;
    delete iSendTimer;
    delete iCloseOrConnectTimer;
    delete iCurrentContextXml;
    delete iCurrentContextObject;
    delete iObjectBuf;
    delete iOutgoingObjectBuf;
}

EXPORT_C void CJabber::ConnectL(const TDesC16 &u,
                                const TDesC16 &p,
                                const TDesC16 &/*ignore*/,
                                TUint32 iAP,
                                TBool aOnlyWhenActive) //, TInt aPort)
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("ConnectL"));
    const TPtrC16 serverName(KDefaultServerName);

    iOnlyWhenActive=EFalse;

    if (iJabberStatus == EConnected) {
        iObserver.NotifyJabberStatus(MJabberObserver::EJabberConnected);
        // start presence timer, and send presence??
    } else if (iJabberStatus == ENotConnected) {
        CNetworkError::TryingConnectionL();
        if (iSuspended) iToSuspend=ETrue;
        iSuspended = EFalse;

        iLastUpdateSent=TTime(0);
        delete iEngine; iEngine=0;
        TBool compress=EFalse;

        compress=ETrue;
        iCustomProtocol=ETrue;

        iEngine = CSocketsEngine::NewL(*this, *GetContext(), compress,
            _L("presence"));

        iCloseConnection = EFalse;
        iUsername.Copy(u);
        iFullNick.Zero();
        iFullNick.Append(u);
        iFullNick.Append(_L16("@"));
        iFullNick.Append(serverName);
        iFullNick.Append(_L("/"));
        iFullNick.Append(iResource);
        iPassword.Copy(p);

        iServerName.Copy(serverName);
#ifdef __DEV__
        // Read host and port only in development builds.
        Settings().GetSettingL(SETTING_JABBER_PORT, iPort);
        iHost.Zero();
        Settings().GetSettingL(SETTING_JABBER_ADDR, iHost);
        if ( iHost.Length() == 0 )
            iHost.Copy(KDefaultHost);
#else
        // Use hardcoded values (xmpp.x.jaiku.com:5224)
        // always in production build.
        iPort = KDefaultPortNumber;
        iHost.Copy(KDefaultHost);
#endif
        iAccessPoint = iAP;
        iJabberSessionId.Zero();

        delete iParser; iParser=0;
        iParser=CXmlParser::NewL(*this);
        iXmlBuf->Zero();
        iStack->reset();

        ChangeStatus(EConnecting, KErrNone);

#ifdef __WINS__
        //iEngine->ConnectL(iHost, 9222, iAccessPoint);
        //iEngine->ConnectL(_L("10.1.0.1"), 9223, iAccessPoint);
        iEngine->ConnectL( iHost, iPort, iAccessPoint);
#else
        iEngine->ConnectL(iHost, iPort, iAccessPoint);
#endif
    }
}

EXPORT_C void CJabber::Disconnect(TBool close_connection)
{
    if (iJabberStatus != ENotConnected) {
        restart = EFalse;
        DoDisconnect(close_connection);
    }
}

void CJabber::DoDisconnect(TBool close_connection)
{
    TJabberState prev_state=iJabberStatus;
    iCloseConnection = close_connection;
    iSendTimer->Reset();
    ChangeStatus(EDisconnecting, KErrNone);
    iEngine->StopRead();
    if (prev_state==EConnected) {
        CC_TRAPD(err, SendDisconnectionL());
        Log(_L("Error in SendDisconnectionL %d"), err);
    }
    iCloseOrConnectTimer->Wait(KCloseSessionTimeOut);
}

void CJabber::NotifyEngineStatus(TInt st, TInt aError)
{
    if (st == MEngineObserver::ESocketConnected) {
        switch (iJabberStatus)
        {
        case EConnecting:
            ChangeStatus(EInitiating, KErrNone);
            iEngine->ReadTimeout();
            SendXMLStreamHeaderL();
            iEngine->Read();
            break;
        default:
            break;
        }
    } else if (st == MEngineObserver::ESocketDisconnected) {
            TBuf<12> errorcode; errorcode.AppendNum(aError);
        ReportError(ENetworkError, errorcode, ETrue);
        switch (iJabberStatus)
        {
        case EDisconnecting:
            // should arrive here through a nice manual close
            ChangeStatus(ENotConnected, aError);
            iObserver.NotifyJabberStatus(MJabberObserver::EJabberDisconnected);
            // should we wait a bit??
            if (restart) {
                ChangeStatus(EWaitingForRetry, 0);
                iCloseOrConnectTimer->Wait(2);
            }
            break;
        default:
            // unexpected error from Socket engine - not critical
            // no need to tell Jabber observer, just reconnect
            ChangeStatus(ENotConnected, aError);
            iObserver.NotifyJabberStatus(MJabberObserver::EJabberDisconnected);
            //CC_TRAPD(err, ConnectL(iUsername, iPassword, iServerName, iAccessPoint));
            break;
        }
    } else if (st == MEngineObserver::EConnectionNotAllowed) {
        CNetworkError::ResetRequestedL();
        CNetworkError::ResetTryingL();
        ReportError(EConnectionNotAllowed, _L("User disallowed network connection"), ETrue);
        ChangeStatus(ENotConnected, aError);
        iObserver.NotifyJabberStatus(MJabberObserver::EConnectionNotAllowed);
    } else {
        TBuf<40> errorcode; errorcode=_L("EngineStatus: ");
        errorcode.AppendNum(st);
        errorcode.Append(_L(", error: "));
        errorcode.AppendNum(aError);
        ReportError(ENetworkError, errorcode);
    }
}

void CJabber::SendXMLStreamHeaderL()
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("SendXMLStreamHeaderL"));

    if ( (iJabberStatus == EInitiating) ) {
        TBuf16<250> header;
        header.Append(_L16("<?xml version='1.0' encoding='utf-8'?><stream:stream version='1.0'"));
        header.Append(_L16(" xmlns='jabber:client' xmlns:c='http://www.cs.helsinki.fi/group/context' "));
        header.Append(_L16("xmlns:stream='http://etherx.jabber.org/streams'  to='"));
        header.Append(iServerName);
        header.Append(_L16("'>"));

        iEngine->WriteL(header);
    }
}



void CJabber::SendDisconnectionL()
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("SendDisconnectionL"));

    if ( (iJabberStatus == EDisconnecting) ) {
        TBuf16<200> msg;
        msg.Append(_L16("</stream:stream>"));
        iEngine->WriteL(msg);
    }
}



void CJabber::IdentifyL()
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("IdentifyL"));

    if (iJabberStatus == EIdentifying ) {
        TBuf8<100> iDigest;
        iDigest.Copy(iJabberSessionId);
        iDigest.Append(iPassword);

        SHA1 * sha1 = new SHA1;
        sha1->Input((char*)(iDigest.Ptr()),iDigest.Size());
        unsigned int message_digest_array[5];

        sha1->Result(message_digest_array);

        TBuf<40> digest;
        for (int i=0;i<5;i++)
        {
            TBuf<8> h;
            h.Format(_L("%08x"),message_digest_array[i]);
            digest.Append(h);
        }
        delete sha1;

        TBuf16<400> ident;

        ident.Append(_L16("<iq type='set' id='ctxauth'><query xmlns='jabber:iq:auth'><username>"));
        ident.Append(iUsername);

        ident.Append(_L16("</username><resource>"));
        ident.Append(iResource);
        ident.Append(_L16("</resource><digest>"));
        ident.Append(digest);
        ident.Append(_L16("</digest></query></iq>"));

        iEngine->WriteL(ident);
    }
}

EXPORT_C TBool CJabber::IsConnected()
{
    return (iJabberStatus == EConnected);
}

EXPORT_C void CJabber::ReadTimeout()
{
    iEngine->ReadTimeout();
}

EXPORT_C void CJabber::NoReadTimeout()
{
    iEngine->NoReadTimeout();
}

EXPORT_C TBool CJabber::SuspendL()
{
    if (! iCustomProtocol ) return EFalse;
    if (iJabberStatus != EConnected) {
        iToSuspend=ETrue;
        return EFalse;
    }
    iToSuspend=EFalse;

    if (iSuspended) return EFalse;
    //iEngine->NoReadTimeout();

    if (iOnlyWhenActive && iNewPresence) DoSendPresenceInfoL();

    iEngine->WriteL(_L("<c:suspend/>"));
    iSuspended=ETrue;
    return ETrue;
}

EXPORT_C TBool CJabber::ResumeL()
{
    if (! iCustomProtocol ) return EFalse;
    iToSuspend=EFalse;

    if (iJabberStatus != EConnected) {
        return EFalse;
    }
    if (!iSuspended) return EFalse;
    //iEngine->ReadTimeout();
    iEngine->WriteL(_L("<c:resume/>"));
    iSuspended=EFalse;

    if (iOnlyWhenActive) {
        if (iNewPresence) DoSendPresenceInfoL();
    }
    iFirstAfterResume=ETrue;

    return ETrue;
}


EXPORT_C void CJabber::AckObjectL(TUint aId)
{
    if (iJabberStatus == EConnected) {
        TBuf<35> ack=_L("<c:ack>");
        ack.AppendNum(aId);
        ack.Append(_L("</c:ack>"));
        iEngine->WriteL(ack);
    } else {
        User::Leave(KErrNotReady);
    }
}

EXPORT_C void CJabber::SendContextObjectL(const class MBBData* aObject)
{
    if (iJabberStatus == EConnected) {
        iOutgoingObjectBuf->Zero();
        aObject->IntoXmlL(iOutgoingObjectBuf);
        iEngine->WriteL(iOutgoingObjectBuf->Buf());
    } else {
        User::Leave(KErrNotReady);
    }
}

EXPORT_C void CJabber::SendMessageL(const TDesC &aTo, const TDesC& aSubject, const TDesC& aMessage)
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("SendMessageL"));

    if (iJabberStatus == EConnected) {
        Log(_L("Jabber::Sending Message"));
        iXmlBuf->Zero();
        auto_ptr<CPtrCArray> attr(new (ELeave) CPtrC16Array(1));
        attr->AppendL(_L16("from"));
        attr->AppendL(iFullNick.Mid(0));
        attr->AppendL(_L("to"));
        attr->AppendL(aTo);
        attr->AppendL(_L("type"));
        attr->AppendL(_L("chat"));
        attr->AppendL(_L("subject"));
        attr->AppendL(aSubject);
        iXmlBuf->BeginElement(_L16("message"), attr.get());

        iXmlBuf->BeginElement(_L16("body"));

        iXmlBuf->Characters(aMessage);

        iXmlBuf->EndElement(_L16("body"));
        iXmlBuf->EndElement(_L16("message"));

        iEngine->WriteL(iXmlBuf->Buf());
        iSendingMessage=ETrue;
    } else {
        User::Leave(KErrNotReady);
    }
}

void CJabber::DoSendPresenceInfoL(TBool aKeepAlive)
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("DoSendPresenceInfoL"));

    if (iJabberStatus != EConnected) return;

    CNetworkError::ConnectionSuccessL();

    if (aKeepAlive && iCustomProtocol) {
        Log(_L("***KEEPALIVE"));
        iEngine->WriteL(_L("<c:keepalive/>"));
        iSendTimer->Wait(iFreshnessInterval);
        return;
    }
    iNewPresence=EFalse;
    if (iUserPresenceInfo && iUserPresenceInfo->Des().Length()>0)
    {
        Log(_L("Jabber::SendingPresence"));
        iXmlBuf->Zero();
        auto_ptr<CPtrCArray> attr(new (ELeave) CPtrC16Array(1));
        attr->AppendL(_L16("from"));
        attr->AppendL(iFullNick.Mid(0));
        attr->AppendL(_L16("type"));
        attr->AppendL(_L16("available"));
        iXmlBuf->BeginElement(_L16("presence"), attr.get());

        iXmlBuf->Leaf(_L16("show"), _L16("xa"));
        iXmlBuf->BeginElement(_L16("status"));

        TBuf<15> timestamp;

        TTime time; time=GetTime();
        time+=GetTimeOffset();

        TDateTime dt;
        dt=time.DateTime();
        _LIT(KFormatTxt,"%04d%02d%02dT%02d%02d%02d");
        timestamp.Format(KFormatTxt, dt.Year(), (TInt)dt.Month()+1, (TInt)dt.Day()+1,
            dt.Hour(), dt.Minute(), dt.Second());

        iXmlBuf->Characters(timestamp);
        iXmlBuf->Characters(*iUserPresenceInfo);

        iXmlBuf->EndElement(_L16("status"));
        iXmlBuf->EndElement(_L16("presence"));

        iEngine->WriteL(iXmlBuf->Buf());

        iLastUpdateSent=GetTime();

        iSendTimer->Wait(iFreshnessInterval);
    }
}

EXPORT_C void CJabber::SendPresenceInfoL(const TDesC &presenceInfo, TBool aQuickRate)
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("SendPresenceInfoL"));

    Log(_L("Jabber::QueueingPresence To send"));

    //local copy of presenceInfo, in case of disconnection or queuing
    while ( presenceInfo.Length()>iUserPresenceInfo->Des().MaxLength() ) {
        iUserPresenceInfo=iUserPresenceInfo->ReAllocL(iUserPresenceInfo->Des().MaxLength()*2);
    }
    *iUserPresenceInfo=presenceInfo;

#if defined(__WINS__) && 0
    if (! ( iLastUpdateSent==TTime(0) ) ) return;
#endif

    TTime now; now=GetTime();

    if (iOnlyWhenActive && iSuspended && !aQuickRate) {
        iSendTimer->Reset();
        iNewPresence=ETrue;
        return;
    }

    if (iJabberStatus == EConnected && (now-iPresenceInterval > iLastUpdateSent || aQuickRate || iFirstAfterResume)) {
        iSendTimer->Reset();
        DoSendPresenceInfoL();
    } else if (iJabberStatus == EConnected) {
        //TBuf<50> msg;
        TTime next_send=iLastUpdateSent+iPresenceInterval;
        TInt w= I64LOW((next_send.Int64() - now.Int64())/(1000*1000));
        if (w<1) w=1;
        //msg.Format(_L("Queuing presence sending after %d secs"), w);
        iSendTimer->Wait(w);
        iNewPresence=ETrue;
    }
    iFirstAfterResume=EFalse;
    if (iOnlyWhenActive && iSuspended) iSendTimer->Reset();
}

void CJabber::Error(XML_Error code, const XML_LChar * String, long ByteIndex)
{
    TPtrC descr((TUint16*)String);
    Log(_L("XML parse error"));
    Log(descr);
    ReportError(EXmlParseError, descr);
}

void CJabber::NotifyNewData(const TDesC8& aBuffer)
{
#ifdef __WINS__
    User::Check();
#endif
    iParser->Parse((char*)(aBuffer.Ptr()),aBuffer.Size(), EFalse);
#ifdef __WINS__
    User::Check();
#endif
}

_LIT(KObject, "contextobject");

void CJabber::SetError(TInt aError)
{
    iContextError=aError;
}

void  CJabber::StartElement(const XML_Char *el, const XML_Char **atts)
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("startElement"));

    TInt currentState = EStackUndefined;

    if ( iStack->iCurrent != NULL ) {
        currentState = iStack->iCurrent->Item;
    }

    // if we're already ignoring parent tag
    if ( currentState == EStackIgnoreElement || currentState == EStackError)
    {
        // then let's ignore all children
        iStack->AppendL(EStackIgnoreElement);
        return;
    }

    TPtrC element = TPtrC( (TUint16*)el );

    if (element.Left( KContextNS().Length() ).Compare( KContextNS ) == 0) {
        TTypeName tn=TTypeName::IdFromAttributes(atts);
        iContextError=0;
        delete iCurrentContextXml; iCurrentContextXml=0;
        delete iCurrentContextObject; iCurrentContextObject=0;
        CC_TRAPD(err, iCurrentContextObject=BBDataFactory()->CreateBBDataL(tn,
            element.Mid( KContextNS().Length() +1), BBDataFactory()));
        if (err!=KErrNone) {
            iObserver.NotifyContextObject(0, err, element);
            iStack->AppendL(EStackIgnoreElement);
        } else {
            iCurrentContextXml=iCurrentContextObject->FromXmlL(this, iParser, iObjectBuf, EFalse);
            iParser->SetHandler(iCurrentContextXml);
            iCurrentContextXml->StartElement(el, atts);
            iStack->AppendL(EStackContextXml);
        }
    } else if (element.Compare(KStream) == 0) {
        // Case: <stream:stream>
        iStack->AppendL(EStackSession);

        for (int i = 0; atts[i]; i += 2) {
            TPtrC attribute = TPtrC((TUint16*)atts[i]);
            if ( attribute.Compare(KId) == 0) {
                (iJabberSessionId).Copy(TPtrC((TUint16*)atts[i+1]));
            }
        }
        ChangeStatus(EIdentifying, KErrNone);
        IdentifyL();
    } else if ( element.Compare(KStreamError) == 0) {
        // Case: <stream:error>
        iStack->AppendL(EStackStreamError);
    } else if ( (element.Compare(KIq) == 0) && ( currentState == EStackSession ) ) {
        // Case: <iq>
        TBool found = EFalse;
        for (int i = 0; atts[i]; i += 2) {
            TPtrC attribute = TPtrC((TUint16*)atts[i]);
            TPtrC value = TPtrC((TUint16*)atts[i+1]);

            if ( attribute.Compare(KType) == 0 ) {
                if (value.Compare(KResult) == 0 ) {
                    iStack->AppendL(EStackConnected);
                    ChangeStatus(EConnected, KErrNone);
                    Settings().WriteSettingL(SETTING_IDENTIFICATION_ERROR, KNullDesC);
                    iObserver.NotifyJabberStatus(MJabberObserver::EJabberConnected);
                } else if (value.Compare(KError) == 0) {
                    iStack->AppendL(EStackIdentFailure);
                }
                found = ETrue;
            }
        }
        if (!found)
        {
            iStack->AppendL(EStackIgnoreElement);
        }
    } else if ( element.Compare(KError) == 0 ) {
        // Case: <error>
        iErrorCode.Zero();

        delete iErrorValue;
        iErrorValue = iErrorValue = HBufC::NewL(0);

        for (int i=0; atts[i]; i+=2)
        {
            TPtrC attribute = TPtrC((TUint16*)atts[i]);
            TPtrC value = TPtrC((TUint16*)atts[i+1]);
            if ( attribute.Compare(KCode) == 0 )
            {
                iErrorCode.Copy(value);
            }
        }
        iStack->AppendL(EStackError);
    } else if (element.Compare(KPresence) == 0) {
        // Case: <presence>
        TBool contextBuddyFound = EFalse;
        TBool offlinePresenceInfo = EFalse;

        for (int i = 0; atts[i]; i += 2)
        {
            TPtrC attribute = TPtrC((TUint16*)atts[i]);
            TPtrC value = TPtrC((TUint16*)atts[i+1]);

            if ( (attribute.Compare(KFrom) == 0) && ( value.Find(KDefaultJabberRessource) != KErrNotFound) )
            {
                contextBuddyFound = ETrue;

                TInt len=value.Length() - KDefaultJabberRessource().Length() -1;
                if (iFrom->Des().MaxLength() < len) {
                    iFrom = iFrom->ReAllocL(len);
                }
                *(iFrom) = value.Left(value.Length() - KDefaultJabberRessource().Length()-1);
            }
            else if ( (attribute.Compare(KType) == 0) && (value.Compare(KUnavailable)==0) )
            {
                offlinePresenceInfo = ETrue;
            }
        }
        if (contextBuddyFound)
        {
            if (!offlinePresenceInfo)
            {
                iStack->AppendL(EStackPresenceInfo);
            }
            else
            {
                iStack->AppendL(EStackOfflinePresenceInfo);
            }
        }
        else
        {
            iStack->AppendL(EStackIgnoreElement);
        }
    } else if (element.Compare(KMessage) == 0) {
        // Case: <message>
        for (int i = 0; atts[i]; i += 2)
        {
            TPtrC attribute = TPtrC((TUint16*)atts[i]);
            TPtrC value = TPtrC((TUint16*)atts[i+1]);

            if (attribute.Compare(KFrom) == 0)
            {
                TInt len=value.Length();
                if (iFrom->Des().MaxLength() < len) {
                    iFrom = iFrom->ReAllocL(len);
                }
                *iFrom=value;
            }
        }
        iStack->AppendL(EStackMessage);
    } else if (element.Compare(KSubject)==0 && currentState == EStackMessage) {
        iStack->AppendL(EStackSubject);
    } else if (element.Compare(KBody)==0 && currentState == EStackMessage) {
        iStack->AppendL(EStackBody);
    } else if (element.Compare(KStatus) == 0) {
        // Case: <status>
        iStack->AppendL(EStackStatus);
    } else {
        // Case: unhandled tag
        iStack->AppendL(EStackIgnoreElement);
    }
}



void  CJabber::EndElement(const XML_Char *name)
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("endElement"));

#ifdef __WINS__
    RDebug::Print(_L("-%s"),name);
#endif
#ifdef __WINS__
    User::Check();
#endif
    TParseState state=(TParseState)iStack->iCurrent->Item;
    iStack->DeleteLast();
#ifdef __WINS__
    User::Check();
#endif
    switch (state)
    {
    case EStackIgnoreElement:
    case EStackStatus:
    case EStackBody:
    case EStackSubject:
        break;

    case EStackIdentFailure:
        Settings().WriteSettingL(SETTING_IDENTIFICATION_ERROR,
            _L("Incorrect username or password"));
        ReportError(EIdentificationFailed);
        break;

    case EStackPresenceInfo:
        {
            TInt timestamp_len=15;
            if (iPresenceInfo->Des().Length() > 0)
            {
                _LIT(KTime, "t");
                RDebug::Print(iPresenceInfo->Des().Left(timestamp_len));
                TBBTime stamp(KTime);
                CC_TRAPD(err, stamp.FromStringL(iPresenceInfo->Des().Left(timestamp_len)));

                if (stamp() == TTime(0))
                {
                    iObserver.NotifyNewPresenceInfo(*iFrom,
                    *iPresenceInfo, stamp());
                }
                else
                {
                    iObserver.NotifyNewPresenceInfo(*iFrom,
                    iPresenceInfo->Mid(timestamp_len), stamp());
                }
            }

            // reset values

            iFrom->Des().Zero();

            iPresenceInfo->Des().Zero();
            break;
        }
    case EStackMessage:
        {
            if (iMessage->Des().Length() > 0 || iSubject->Des().Length()>0) {
                iObserver.NotifyNewMessage(*iFrom, *iSubject, *iMessage);
            }
            iMessage->Des().Zero();
            iFrom->Des().Zero();
            iSubject->Des().Zero();
        }
        break;
    case EStackOfflinePresenceInfo:
        //send something to context server to notify the offline status of contact

        //reset values

        delete iFrom;
        iFrom = iFrom = HBufC::NewL(0);

        iPresenceInfo->Des().Zero();

        break;

    case EStackConnected:
        if (iUserPresenceInfo->Length() >0) {
            SendPresenceInfoL(*iUserPresenceInfo);
        }
        if (iCustomProtocol && iToSuspend) {
            iObserver.NotifyJabberStatus(MJabberObserver::EJabberPendingSuspend);
        }
        break;

    case EStackSession:
        // disconnect
#ifdef __WINS__
    User::Check();
#endif
        iCloseOrConnectTimer->Reset();
        iEngine->Disconnect(ETrue);
#ifdef __WINS__
    User::Check();
#endif
        break;

    case EStackError:
        break;
    case EStackContextXml:
        if (KServerTime()==TPtrC((const TUint16*)name)) {
            const TBBTime* servertime=bb_cast<TBBTime>(iCurrentContextObject);
            if (servertime) {
                TTime clienttime=GetTime();
                TInt err=clienttime.SecondsFrom( (*servertime)(), iOffset );
                if (err!=KErrNone) iOffset=0;
                iOutgoingObjectBuf->iOffset=iOffset;
            }
        } else {
            iObserver.NotifyContextObject(iCurrentContextObject, iContextError, TPtrC((const TUint16*)name));
        }
        break;

    case EStackStreamError:
#ifdef __WINS__
    User::Check();
#endif
        ReportError(EStreamError);
#ifdef __WINS__
    User::Check();
#endif
        break;
    }
}

void CJabber::CharacterData(const XML_Char *s, int len)
{
    CALLSTACKITEM_N(_CL("CJabber"), _CL("charData"));

    if (!iStack->iCurrent) return;

    if (iStack->iCurrent->Item == EStackStatus) {
        TPtrC t = TPtrC((TUint16*)s,len);
        while ( iPresenceInfo->Length()+len > iPresenceInfo->Des().MaxLength()) {
            iPresenceInfo=iPresenceInfo->ReAllocL(iPresenceInfo->Des().MaxLength()*2);
        }
        iPresenceInfo->Des().Append(t);

        //Log(*iPresenceInfo);
    } else if (iStack->iCurrent->Item == EStackError)   {
        iErrorValue = iErrorValue->ReAllocL(iErrorValue->Length()+len);
        iErrorValue->Des().Append((TUint16*)s,len);
    } else if (iStack->iCurrent->Item == EStackBody) {
        TPtrC t = TPtrC((TUint16*)s,len);
        while ( iMessage->Length()+len > iMessage->Des().MaxLength()) {
            iMessage=iMessage->ReAllocL(iMessage->Des().MaxLength()*2);
        }
        iMessage->Des().Append(t);
    } else if (iStack->iCurrent->Item == EStackSubject) {
        TPtrC t = TPtrC((TUint16*)s,len);
        while ( iSubject->Length()+len > iSubject->Des().MaxLength()) {
            iSubject=iSubject->ReAllocL(iSubject->Des().MaxLength()*2);
        }
        iSubject->Des().Append(t);
    }
}

void CJabber::expired(CBase* Source)
{
    if (Source==iSendTimer) {
        if (! iNewPresence ) {
            Log(_L("timer: DoSendPresenceInfoL(ETrue)"));
            DoSendPresenceInfoL(ETrue);
        } else {
            Log(_L("timer: DoSendPresenceInfoL(EFalse)"));
            DoSendPresenceInfoL(EFalse);
        }
    } else if (Source== iCloseOrConnectTimer) {
        if (iJabberStatus==EWaitingForRetry) {
            ConnectL(iUsername, iPassword, iHost, iAccessPoint, iOnlyWhenActive);
        } else if (iJabberStatus==EDisconnecting) {
            iEngine->Disconnect(iCloseConnection);
        }
    }
}

void CJabber::ReportError(TInt aErrorType, const TDesC& aExtra, TBool aFromDisconnect)
{
    if (!aFromDisconnect) DoDisconnect(EFalse);

    switch (aErrorType)
    {
    case EIdentificationFailed:
        Log(_L("CJabber: Identification Failed") );
        Log(_L("ErrorDetail:"));
        Log(iErrorCode);
        Log(*iErrorValue);
        restart = EFalse;
        iObserver.NotifyError(
            RemoteErr(_L("CJabber: Identification Failed Code: %1 Msg %2")).
                TechMsg(iErrorCode, *iErrorValue).
                UserMsg(gettext("Failed to login to Jaiku (%1)"), *iErrorValue).
                Get());
        iObserver.NotifyJabberStatus(MJabberObserver::EIdentificationFailed);
        break;

    case EStreamError:
        iObserver.NotifyError(
            RemoteErr(_L("CJabber: StreamError Msg %1")).
                TechMsg(*iErrorValue).
                UserMsg(gettext("Connection to Jaiku closed (%1)"), *iErrorValue).
                Get());
        Log(_L("CJabber: StreamError"));
        restart = ETrue;
        break;

    case EXmlParseError:
        iObserver.NotifyError(
            RemoteErr(_L("CJabber: XML Parse Error %1")).
                TechMsg(aExtra).
                UserMsg(gettext("Connection to Jaiku closed (invalid data)")).
                Get());
        Log(_L("CJabber: XML Parse Error"));
        restart = ETrue;
        break;
    case EConnectionNotAllowed:
        iObserver.NotifyError(
            EnvErr(_L("CJabber: connection disallowed")).
                UserMsg(gettext("Connection to Jaiku disallowed by user")).
                Get());
        Log(_L("CJabber: Connection disallowed"));
        restart = EFalse;
        break;
    default:
        // critical errors from Sockets Engine
        iObserver.NotifyError(
            RemoteErr(_L("CJabber: NetworkError %1")).
                TechMsg(aExtra).
                UserMsg(gettext("Connection to Jaiku closed (network error)")).
                Get());
        restart = EFalse;
        iObserver.NotifyJabberStatus(MJabberObserver::EJabberDisconnected);
        break;
    }
}

void CJabber::NotifyCanWrite()
{
    if (iSendingMessage) {
        iObserver.NotifyMessageSent();
        iSendingMessage=EFalse;
    } else if (iJabberStatus==EConnected) {
        iObserver.NotifyCanWrite();
    }
}
