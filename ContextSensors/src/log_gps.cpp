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

#include "log_gps.h"
#include "symbian_auto_ptr.h"
#include <btmanclient.h>
#include <btextnotifiers.h>
#include <btsdp.h>
#include <bt_sock.h>

#include "timeout.h"
#include "cl_settings.h"

#include "csd_gps.h"
#include "csd_current_app.h"
#include "csd_bluetooth.h"
#include <charconv.h>
#include "reporting.h"
#include "mutexrequest.h"
_LIT(KBtAccessMutex, "ContextBtMutex");

#define KBTServiceSerial 0x1101
#define READ_INTERVAL 5

#ifdef __WINS__
//#define NO_BT
#endif

_LIT8(KStartCmd, "");
_LIT8(KStopCmd, "");

_LIT(KName, "CLogGpsImpl");


const TUid KCameraUid = { 0x1000593F };
const TUid KCamera2Uid = { 0x101f857a}; //6630, 6680
const TUid KCamera3Uid = { 0x101ffa86 }; // N70

/*
 * Concepts
 * !Bluetooth RFComm connection!
 * !Bluetooth service discovery!
 * !Bluetooth GPS!
 */

class CLogGpsImpl : public CLogGps, public MTimeOut, 
	public MSdpAgentNotifier, public MSdpAttributeValueVisitor, public MSettingListener {
private:
	virtual void SelectDevice();
	virtual void TestDevice();
	virtual void ResetDevice();
	CLogGpsImpl(MApp_context& Context);
	virtual ~CLogGpsImpl();

	enum comm_state { IDLE, SELECTING, GETTING_SERVICE, CONNECTING, SENDING_START, SENDING_STOP, CLOSING, READING, RETRY, WAITING_ON_MUTEX };
	comm_state current_state;
	RNotifier iNotifier; bool not_is_open;
	TBTDeviceSelectionParams selectionFilter;
	TUUID targetServiceClass;
	TBTDeviceSelectionParamsPckg pckg;
	TBTDeviceResponseParamsPckg result;

	CTimeOut*	iTimeOut;
	TInt		iLogTime;
	TTime		iLogStarted;
	bool		iFirst;

	CSdpAgent* agent;
	CSdpSearchPattern* list;

	TBTSockAddr	iBtAddr;
	bool		iAddrSet; 

	TUint port;
	bool seen_rfcomm;

	RSocket socket; bool socket_is_open; bool socket_is_connected;
	RSocketServ socket_server; bool socket_server_is_open;

	TBuf8<100> buf;
	TBuf8<100> line; TBuf<100> line16;
	TGpsLine whole_line;
	int read_count;

	// parsing
	bool nl; bool cmd; bool biomap;
	bool getting_line;
	TBuf8<20> cmdstr; TBuf<20> cmdstr16;

	Clog_base_impl *iLastKnownLogger;

	void CheckedRunL();
	void DoCancel();
	void ConstructL();

	void get_service();
	void connect_to_service();
	void release();
	void handle_buffer();
	void retry(TInt WaitTime=30);
	void calculate_time();

	//MSdpAgentNotifier
	virtual void AttributeRequestComplete(TSdpServRecordHandle aHandle, TInt aError);
	virtual void AttributeRequestResult(TSdpServRecordHandle aHandle, 
		TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue);
	virtual void NextRecordRequestComplete(TInt aError, 
		TSdpServRecordHandle aHandle, TInt aTotalRecordsCount);
	//MSdpAttributeValueVisitor
	virtual void VisitAttributeValueL(CSdpAttrValue &aValue, TSdpElementType aType);
	virtual void StartListL(CSdpAttrValueList &aList);
	virtual void EndListL();

	// MTimeOut
	virtual void expired(CBase* Source);

	// Mlogger
	virtual void NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, const CBBSensorEvent& aEvent);

	// MSettingListener
	virtual void SettingChanged(TInt Setting);

	CMutexRequest*	iMutex;
	void ReleaseMutex();
	TBool iGotMutex;

	friend class CLogGps;
	friend class auto_ptr<CLogGpsImpl>;
};

EXPORT_C CLogGps* CLogGps::NewL(MApp_context& Context)
{
	CALLSTACKITEM2_N(_CL("CLogGps"), _CL("NewL"),  &Context);

	auto_ptr<CLogGpsImpl> ret(new (ELeave) CLogGpsImpl(Context));
	ret->ConstructL();
	return ret.release();
}

CLogGps::CLogGps(MApp_context& Context) : CCheckedActive(EPriorityNormal, _L("CLogGps")), 
	Mlog_base_impl(Context, KGps, KGpsTuple, 0)
{

}

EXPORT_C CLogGps::~CLogGps() { }

CLogGpsImpl::CLogGpsImpl(MApp_context& Context) : CLogGps(Context), targetServiceClass(0x2345), iFirst(true)
{
}

void CLogGpsImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("ConstructL"));

	Mlogger::ConstructL(AppContextAccess());
	SubscribeL(KBluetoothTuple);
	SubscribeL(KCurrentAppTuple);

	iLastKnownLogger=Clog_base_impl::NewL(AppContext(), KLastKnownGps, KLastKnownGpsTuple, 
		7*24*60*60);


	Settings().NotifyOnChange(SETTING_GPS_LOG_TIME, this);
	Settings().NotifyOnChange(SETTING_BT_SCAN_INTERVAL, this);

	iTimeOut=CTimeOut::NewL(*this);
	Mlog_base_impl::ConstructL();

#ifdef NO_BT
	whole_line()=_L("$GPRMC,143041.940,A,6009.7988,N,02454.3641,E,0.00,,010206,,*10");
	TGeoLatLong ll;
	NmeaToLatLong(whole_line(), ll);
	if (ll.iLat.Length()>0 && ll.iLong.Length()>0)
		iLastKnownLogger->post_new_value(&whole_line);
#endif
	iLogTime=15;
	calculate_time();
	TBuf8<10> dabuf;
	iAddrSet=Settings().GetSettingL(SETTING_GPS_BT_ADDR, dabuf);
	if (iAddrSet && dabuf.Length()==6) {
		TBTDevAddr  da(dabuf);
		TInt p;
		iAddrSet=Settings().GetSettingL(SETTING_GPS_BT_PORT, p);
		port=p;
		iBtAddr.SetBTAddr(da);
		iBtAddr.SetPort(port);
	}

	CActiveScheduler::Add(this); // add to scheduler
}

void CLogGpsImpl::ResetDevice()
{
	Settings().WriteSettingL(SETTING_GPS_BT_ADDR, KNullDesC8);
	iAddrSet=EFalse;
}

void CLogGpsImpl::SelectDevice()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("SelectDevice"));

#ifdef NO_BT
	TBuf8<6> addr=_L8("123456");
	Settings().WriteSettingL(SETTING_GPS_BT_ADDR, addr);

	//post_error(_L("not supported"), KErrNotSupported);
	return;
#else
	release();

	TInt ret;
	if ((ret=iNotifier.Connect())!=KErrNone) {
		TBuf<30> msg=_L("Error connecting to notifier: ");
		msg.AppendNum(ret);

		post_error(msg, ret);
		return;
	}
	not_is_open=true;

	selectionFilter.SetUUID(targetServiceClass);
	iNotifier.StartNotifierAndGetResponse(iStatus, KDeviceSelectionNotifierUid, pckg, result);
	
	SetActive();	
	current_state=SELECTING;
#endif
}

CLogGpsImpl::~CLogGpsImpl()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("~CLogGpsImpl"));

	release();
	delete iTimeOut;
	delete iLastKnownLogger;

	Settings().CancelNotifyOnChange(SETTING_BT_SCAN_INTERVAL, this);
	Settings().CancelNotifyOnChange(SETTING_GPS_LOG_TIME, this);
}

void CLogGpsImpl::release()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("release"));

	if (iTimeOut) iTimeOut->Reset();

	Cancel();
	ReleaseMutex();

	if (not_is_open) iNotifier.Close(); not_is_open=false;

	delete agent; agent=0;
	delete list; list=0;
	
	if (socket_is_connected) {
		TRequestStatus s;
		socket.Shutdown(RSocket::EImmediate, s);
		User::WaitForRequest(s);
		socket_is_connected=false;
	}
	if (socket_is_open) socket.Close(); socket_is_open=false;
	if (socket_server_is_open) socket_server.Close(); socket_server_is_open=false;

	current_state=IDLE;

}

void CLogGpsImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("DoCancel"));

	switch(current_state) {
	case SELECTING:
		iNotifier.CancelNotifier(KDeviceSelectionNotifierUid);
		break;
	case CONNECTING:
		socket.CancelConnect();
		break;
	case READING:
		socket.CancelRead();
		break;
	case RETRY:
	case WAITING_ON_MUTEX:
		ReleaseMutex();
		break;
	default:
		break;
	}
	current_state=IDLE;
}

void CLogGpsImpl::get_service()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("get_service"));

	release();
	agent = CSdpAgent::NewL(*this, result().BDAddr());
	list = CSdpSearchPattern::NewL();
	list->AddL(KBTServiceSerial);
	agent->SetRecordFilterL(*list);
	agent->NextRecordRequestL();
}

void CLogGpsImpl::retry(TInt WaitTime)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("retry"));

	if (iFirst) {
		TBuf<20> addr;
		iBtAddr.BTAddr().GetReadable(addr);
		TBuf<100> msg=_L("using dev ");
		msg.Append(addr);
		msg.Append(_L(" port "));
		msg.AppendNum(iBtAddr.Port());
		post_error(msg, KErrNone);
	}
	iFirst=false;

#ifdef NO_BT
	return;
#endif
	TTime curr;
	curr=GetTime();
	if (curr + TTimeIntervalSeconds(WaitTime) > iLogStarted+TTimeIntervalSeconds(iLogTime) ) {
		release();
	} else {
		release();
		current_state=RETRY;

		TTimeIntervalMicroSeconds32 w(60*1000*1000);
		iTimeOut->Wait(WaitTime);
		iMutex=CMutexRequest::NewL(AppContext(), KBtAccessMutex, w, &iStatus);
		SetActive();
	}

	line.Zero(); cmdstr.Zero();
	cmd=nl=getting_line=false;
}

void CLogGpsImpl::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("expired"));

	if (current_state==RETRY) {
		if (iGotMutex) {
			current_state=IDLE;
			connect_to_service();
		}
	} else {
		Cancel();
		if (KStopCmd().Length()>0) {
			socket.Write(KStopCmd, iStatus);
			current_state=SENDING_STOP;
		} else {
			socket.Shutdown(RSocket::EImmediate, iStatus);
			current_state=CLOSING;
		}
		SetActive();
	}
}

void CLogGpsImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("CheckedRunL"));

	TBuf<30> msg;
	if (iStatus.Int()!=KErrNone) {
		_LIT(stat_err, "error: %d %d");
		msg.Format(stat_err, iStatus.Int(), TInt(current_state));
		post_error(msg, iStatus.Int());
		if (current_state==READING || current_state==CONNECTING)
			retry();
		else
			release();
		return;
	}
	switch(current_state) {
	case RETRY:
		iGotMutex=ETrue;
		if (! iTimeOut->IsActive() ) {
			expired(iTimeOut);
		}
		break;
	case SELECTING:
		if(!result().IsValidBDAddr()) {
			msg=_L("cancelled");
			post_error(msg, KErrNone);
			current_state=IDLE;
		} else {
			msg=_L("selected ");
			msg.Append(result().DeviceName());
			post_error(msg, KErrNone);
			port=0; seen_rfcomm=false;
			get_service();
		}
		if (not_is_open) iNotifier.Close(); not_is_open=false;
		break;
	case SENDING_START:
		socket.Read(buf, iStatus);
		current_state=READING;
		SetActive();
		break;
	case SENDING_STOP:
		socket.Shutdown(RSocket::EImmediate, iStatus);
		current_state=CLOSING;
		SetActive();
		break;
	case CLOSING:
		release();
		break;
	case CONNECTING:
		{
			socket_is_connected=true;
			post_error(_L("connected"), KErrNone);
			read_count=0;
			if ( KStartCmd().Length()>0) {
				socket.Write(KStartCmd, iStatus);
				current_state=SENDING_START;
			} else {
				socket.Read(buf, iStatus);
				current_state=READING;
			}
			SetActive();
		}
		break;
	case IDLE:
	case GETTING_SERVICE:
		msg.Format(_L("Unexpected state %d"), current_state);
		post_error(msg, KErrGeneral);
		break;
	case READING:
		{
		handle_buffer();
		buf.Zero();
		socket.Read(buf, iStatus);
		current_state=READING;
		SetActive();
		}
		break;
	}
}

void CLogGpsImpl::handle_buffer()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("handle_buffer"));

	int pos=0;
	int len=buf.Length();
	bool incremented=false;
	TText8 cur;
	while (pos<len) {
		if (pos<len && getting_line) {
			cur=buf[pos];
			if (! ( cur== '\r' || cur == '\n') )  {
				line.Append(cur);
				if (line.Length()==line.MaxLength()) {
					// overflow: garbled
					line.Zero();
					getting_line=false;
				}
				if (cur=='\r') {
					++pos; incremented=true;
				}
			} else {
				TInt s;
				if (!biomap) {
					whole_line()=_L("$");
					CC()->ConvertToUnicode(cmdstr16, cmdstr, s);
					whole_line().Append(cmdstr16);
					whole_line().Append(_L(","));
				} else {
					whole_line()=_L("$BIOMAP,");
				}
				CC()->ConvertToUnicode(line16, line, s);
				whole_line().Append(line16);
				{
					TGeoLatLong lat_long;
					NmeaToLatLong(whole_line(), lat_long);
					if (lat_long.iLat.Length()>0 && lat_long.iLong.Length()>0)
						iLastKnownLogger->post_new_value(&whole_line);
				}
				post_new_value(&whole_line);
			}
		}
		if (pos<len && (cur=buf[pos])=='\n') {
			nl=true; cmd=false; getting_line=false;
			++pos; incremented=true;
		}
		if (pos<len && nl) {
			if ((cur=buf[pos])=='$') {
				cmd=true;
				cmdstr.Zero();
			} else {
				// biomapping - NOT IN USE, buggy
				//biomap=true;
				//getting_line=true;
				line.Zero();
				cmdstr.Zero();
			}
			nl=false;
			++pos; incremented=true;
			//cb->status_change(_L("nl"));
		}
		if (pos<len && cmd) {
			if ((cur=buf[pos])==',') {
				cmd=false;
				if (! cmdstr.Compare(_L8("GPRMC") ) ) {
					if (read_count==0) {
						getting_line=true;
						line.Zero();
						post_error(_L("reading line"), KErrNone);
					}
					++read_count;
					if (read_count==READ_INTERVAL) {
						read_count=0;
					}
				}
			} else {
				cmdstr.Append(cur);
				//TBuf<6> msg;
				//cc->ConvertToUnicode(msg, cmdstr, state);
				//cb->status_change(msg);
				if (cmdstr.Length()==cmdstr.MaxLength()) {
					// overflow: garbled
					cmdstr.Zero();
					cmd=false;
				}
			}
			++pos; incremented=true;
		}
		if (!incremented) ++pos;
		incremented=false;
	}
}


void CLogGpsImpl::AttributeRequestComplete(TSdpServRecordHandle /*aHandle*/, TInt /*aError*/)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("AttributeRequestComplete"));

	if (port==0) {
		post_error(_L("didn't find service"), KErrNotFound);
		Reporting().ShowGlobalNote(4,
			_L("Could not connect to GPS device"));
	} else {
		TBuf<30> msg=_L("found port ");
		msg.AppendNum(port);
		post_error(msg, KErrNone);
		iBtAddr.SetBTAddr(result().BDAddr());
		iBtAddr.SetPort(port);
		iAddrSet=true;
		msg=_L("writing setting, len ");
		msg.AppendNum(iBtAddr.Length());
		post_error(msg, KErrNone);
		Settings().WriteSettingL(SETTING_GPS_BT_ADDR, iBtAddr.BTAddr().Des());
		Settings().WriteSettingL(SETTING_GPS_BT_PORT, iBtAddr.Port());
		Reporting().ShowGlobalNote(4,
			_L("GPS device set"));
	}
	current_state=IDLE;
}

void CLogGpsImpl::AttributeRequestResult(TSdpServRecordHandle /*aHandle*/, 
				      TSdpAttributeID /*aAttrID*/, CSdpAttrValue* aAttrValue)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("AttributeRequestResult"));

	
	if (aAttrValue) 
		aAttrValue->AcceptVisitorL(*this);	
}

void CLogGpsImpl::VisitAttributeValueL(CSdpAttrValue &aValue, TSdpElementType aType)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("VisitAttributeValueL"));

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

void CLogGpsImpl::StartListL(CSdpAttrValueList &/*aList*/)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("StartListL"));

}

void CLogGpsImpl::EndListL()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("EndListL"));

}

void CLogGpsImpl::connect_to_service()
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("connect_to_service"));

	TInt ret;
	TBuf<30> err;
	if ( (ret=socket_server.Connect()) != KErrNone) {
		_LIT(f, "RSocketServer.Connect: %d");
		err.Format(f, ret);
		post_error(err, KErrGeneral);
		retry();
		return;
	}
	socket_server_is_open=true;

	//if ( (ret=socket.Open(socket_server, KBTAddrFamily, KSockStream, KRFCOMM )) != KErrNone) {
	if ( (ret=socket.Open(socket_server, _L("RFCOMM"))) != KErrNone ) {
		_LIT(f, "RSocket.Open: %d");
		err.Format(f, ret);
		post_error(err, KErrGeneral);
		retry();
		return;
	}
	socket_is_open=true;

	// only log for set time
	TTime curr; curr=GetTime();
	TTimeIntervalSeconds secs;
	TTime stop_at;
	stop_at=iLogStarted+TTimeIntervalSeconds(iLogTime);
	if (stop_at > curr ) {
		stop_at.SecondsFrom(curr, secs);
		iTimeOut->Wait(secs.Int());

		socket.Connect(iBtAddr, iStatus);
   
		current_state=CONNECTING;
		SetActive();
	} else {
		release();
	}


}


void CLogGpsImpl::NextRecordRequestComplete(TInt aError, 
					 TSdpServRecordHandle aHandle, TInt aTotalRecordsCount)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("NextRecordRequestComplete"));

	TBuf<30> msg;
	if (aError!=KErrNone && aError!=KErrEof) {
		_LIT(err, "service error: %d");
		msg.Format(err, aError);
		post_error(msg, KErrGeneral);
	} else if (aError==KErrEof) {
		_LIT(err, "disconnected");
		post_error(err, KErrGeneral);
	} else if(aTotalRecordsCount==0) {
		_LIT(err, "no Serial service");
		post_error(err, KErrGeneral);
	} else {
		_LIT(st, "found service");
		post_error(st, KErrNone);
		agent->AttributeRequestL(aHandle, KSdpAttrIdProtocolDescriptorList);
		return;
	}
	Reporting().ShowGlobalNote(4,
		_L("Could not connect to GPS device"));
	current_state=IDLE;
}

void CLogGpsImpl::NewSensorEventL(const TTupleName& aName, const TDesC& /*aSubName*/, const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("NewSensorEventL"));


	if (aEvent.iPriority()!=CBBSensorEvent::VALUE && 
		aEvent.iPriority()!=CBBSensorEvent::UNCHANGED_VALUE) return;

	if (!iAddrSet || iLogTime<=0) return;
	if (aName==KBluetoothTuple) {
		const CBBBtDeviceList* devs=bb_cast<CBBBtDeviceList>(aEvent.iData());
		if (!devs) return;
		for (const TBBBtDeviceInfo* di=devs->First(); di; di=devs->Next()) {
			if (di->iMAC().Compare(iBtAddr.BTAddr().Des())==0) {
				iLogStarted=GetTime();
				retry(3);
				return;
			}
		}
	} else if (aName==KCurrentAppTuple) {
		const TBBCurrentApp* app=bb_cast<TBBCurrentApp>(aEvent.iData());
		if (!app) return;
		if ( (app->iUid()==KCameraUid.iUid || app->iUid()==KCamera2Uid.iUid
				||app->iUid()==KCamera3Uid.iUid) && current_state==IDLE) {
			iLogStarted=GetTime();
			retry(0);
		}
	}
}

void CLogGpsImpl::SettingChanged(TInt)
{
	CALLSTACKITEM_N(_CL("CLogGpsImpl"), _CL("SettingChanged"));

	calculate_time();
}

void CLogGpsImpl::calculate_time()
{
	TInt bt_int;
	Settings().GetSettingL(SETTING_GPS_LOG_TIME, iLogTime);
	Settings().GetSettingL(SETTING_BT_SCAN_INTERVAL, bt_int);
	iLeaseTimeInSeconds=bt_int+60;
	if (iLogTime > bt_int-5) iLogTime=bt_int-5;
	if (iLogTime < 10) iLogTime=10;
}

void CLogGpsImpl::TestDevice()
{
	iLogStarted=GetTime();
	retry(1);
}
void CLogGpsImpl::ReleaseMutex()
{
	iGotMutex=EFalse;
	if (!iMutex) return;

	delete iMutex; iMutex=0;
	Reporting().DebugLog(_L("gps::releasemutex"));
}
