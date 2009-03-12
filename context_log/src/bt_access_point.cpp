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
#include "bt_access_point.h"
#include "cl_settings.h"
#include <badesca.h>
#include <bttypes.h>

#ifdef __S60V20__
#include <btregistry.h>
#endif

#include <cdbcols.h>
#include <commdb.h>
#include <apdatahandler.h>
#include <eikenv.h>
#include "symbian_auto_ptr.h"

#include "raii_f32file.h"

_LIT(KContextAP, "ContextBT");
_LIT(KContextModem, "ContextBTModem");
_LIT(KClassName, "CBTAPManagerImpl");
_LIT(KDefaultServiceFile, "c:\\system\\apps\\context_log\\service.obj");
_LIT(KLoginScript, "CHARMAP [windows-1252]\nLOOP 10\n{\nSEND \"CLIENT\"+<0x0d>\nWAIT 3\n{\n\"SERVER\" OK\n}\n}\nEXIT KErrNoAnswer$\n\nOK:\nEXIT\n");

#define TEST_IF_NAME	_S("IfName")

class CBTAPManagerImpl : public CBTAPManager, public MContextBase  {
public:
	~CBTAPManagerImpl();
private:
	CBTAPManagerImpl(MApp_context& Context, CBTDeviceList* aList);
	void ConstructL();

	virtual void NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, const CBBSensorEvent& aEvent);

	void ParseDeviceNames(const TDesC& Names);

	void ConnectViaDevice();
	TInt SetTargetDevice(const TDesC8& addr);

	TInt CreateBTModem(CCommsDatabase * db);
	TInt CreateBTAccessPoint();

	TBool  iCharging;

	CDesC16Array *  iDiscovered;

	TInt BTAccessPointId;
	CBTDeviceList* iList;

	TBuf<12> current_gateway;

	

	friend class CBTAPManager;
};

//-----------------------------------------------------------------------------------
CBTAPManager* CBTAPManager::NewL(MApp_context& Context, CBTDeviceList* aList)
{
	CALLSTACKITEM_N(_CL("CBTAPManager"), _CL("NewL"));

	auto_ptr<CBTAPManagerImpl> ret(new (ELeave) CBTAPManagerImpl(Context, aList));
	ret->ConstructL();
	return ret.release();
}


CBTAPManagerImpl::CBTAPManagerImpl(MApp_context& Context, CBTDeviceList* aList) : 
		MContextBase(Context), BTAccessPointId(0), iList(aList)
{
	CALLSTACKITEM_N(_CL("CBTAPManagerImpl"), _CL("CBTAPManagerImpl"));
}

void CBTAPManagerImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBTAPManagerImpl"), _CL("ConstructL"));

	Mlogger::ConstructL(AppContextAccess());

	iDiscovered=new (ELeave) CDesC16ArrayFlat(16);

	// check if BT access point has been created
	bool context_ap_found = false;
	
	auto_ptr<CCommsDatabase> db(CCommsDatabase::NewL(EDatabaseTypeIAP));
	db->ShowHiddenRecords();
	CCommsDbTableView * iView;
	iView=db->OpenTableLC(TPtrC(IAP));

	TInt err;
	TUint32 id=0;
	
	while( (err=iView->GotoNextRecord()) == KErrNone ) 
	{
		iView->ReadUintL(TPtrC(COMMDB_ID), id);
		TBuf<100> iap_name;
		iView->ReadTextL(TPtrC(COMMDB_NAME), iap_name);
		if (iap_name.Compare(KContextAP) == 0)
		{
			context_ap_found = true;
			BTAccessPointId = id;
			err = -1; // to exit the while...
		}
	}
	CleanupStack::PopAndDestroy(iView);
	iView = NULL;

	db.reset();

	if ( !context_ap_found ) 
	{
		TInt ap_id = CreateBTAccessPoint();
		if (ap_id > 0)
		{
			BTAccessPointId = ap_id;
			
		}
	}
	Settings().WriteSettingL(SETTING_BT_AP,  BTAccessPointId);

}

CBTAPManagerImpl::~CBTAPManagerImpl()
{
	CALLSTACKITEM_N(_CL("CBTAPManagerImpl"), _CL("~CBTAPManagerImpl"));
	delete iDiscovered;
}

void CBTAPManagerImpl::NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, const CBBSensorEvent& aEvent)
{
	CALLSTACKITEM_N(_CL("CBTAPManagerImpl"), _CL("NewSensorEventL"));
	
	/*
	FIXME!
	if (name.Compare(_L("Charger"))==0)
	{
		if (priority==CBBSensorEvent::VALUE )
		{
			TInt err;
			TBool charging;
			TLex lex;
			lex = value.Mid(0, value.Length());
			err =  lex.Val(charging);
			iCharging = charging;
		}
		else
		{
			iCharging = EFalse;
		}
	}
	else if (name.Compare(_L("devices"))==0)
	{
		if (priority==CBBSensorEvent::VALUE ) {
			ParseDeviceNames(value);
			ConnectViaDevice();
		}
	}
	*/
}

void CBTAPManagerImpl::ParseDeviceNames(const TDesC& names)
{
	CALLSTACKITEM_N(_CL("CBTAPManagerImpl"), _CL("ParseDeviceNames"));
	
	//0020e04c71b8 [GLOMGOLD-25,1:1:2] 0002ee51c437 [Reno7650,2:3:4]
	iDiscovered->Reset();
	iDiscovered->Compress();
	if (names.Length() == 0) return;

	TInt ignore = 0;
	TBuf<12> temp_addr;

	for (int i=0; i< names.Length(); i++)
	{
		TBuf<1> c;
		c.Append(names.Mid(i,1));
		
		if (c.Compare(_L(" ")) == 0) {
		}
		else if (c.Compare(_L("[")) == 0) {
			ignore++;
		}
		else if (c.Compare(_L("]")) == 0) {
			ignore--;
		}
		else if (ignore == 0) {
			temp_addr.Append(c);
			if (temp_addr.Length() == 12)
			{
				iDiscovered->AppendL(temp_addr);
				temp_addr.Zero();
			}
		}
		
	}

}

void CBTAPManagerImpl::ConnectViaDevice()
{
	CALLSTACKITEM_N(_CL("CBTAPManagerImpl"), _CL("ConnectViaDevice"));

	TInt found = -1;
	int i =0;
	while ( (found == -1) && ( i<iDiscovered->Count()) )
	{
		TInt pos;
		if ( pos=iList->FindbyReadable( (*iDiscovered)[i] ) >= 0 )
		{
			found = pos;
		}
		else
		{
			i++;
		}
	}

	TInt iap;
	Settings().GetSettingL(SETTING_IP_AP, iap);

	TInt current_ap;
	Settings().GetSettingL(SETTING_CURRENT_AP, current_ap);

	TInt bt_ap;
	Settings().GetSettingL(SETTING_BT_AP, bt_ap);

	if (found == -1)
	{
		if (current_ap != iap)
		{
			Settings().WriteSettingL(SETTING_CURRENT_AP, iap);
		}
	}
	else
	{
		// Set Service in Registery
		SetTargetDevice( iList->AddrArray()->MdcaPoint(found) );
		Settings().WriteSettingL(SETTING_CURRENT_AP, bt_ap);
	}
}

TInt CBTAPManagerImpl::SetTargetDevice(const TDesC8& addr)
{
	CALLSTACKITEM_N(_CL("CBTAPManagerImpl"), _CL("SetTargetDevice"));
	
#ifdef __S60V20__
	auto_ptr<CBTRegistry> reg(CBTRegistry::NewL());

	// Use serialized object from file, cause we have problem creating one from scratch
	TBTCommPortSettingsPckg serial_settings = TBTCommPortSettingsPckg();

	TInt err;
	
	CC_TRAP(err, {
		RAFile file; file.OpenLA( Fs(), KDefaultServiceFile, EFileRead );
		User::LeaveIfError(file.Read(serial_settings));
	});
	if (err!=KErrNone) return err;

	// test: GLOMGOLD-25 address
	// 0020e04c71b8
	//TUint32 val1 = 32;
	//TUint32 val2 = 3763106232;
	//TInt64 add = TInt64(val1,val2);

	TBTDevAddr temp = TBTDevAddr(addr);

	serial_settings().SetBTAddr( temp );
	serial_settings().SetName(_L("ContextBT"));
	serial_settings().SetPort(1);

	reg->SetDefaultCommPort(serial_settings());
	return KErrNone;
#else
#  if 0
	TBTCommPortSettings settings(_L("BTCOMM::1"), 
#  else
	return KErrNotSupported;
#  endif
#endif
}


TInt CBTAPManagerImpl::CreateBTAccessPoint()
{
	CALLSTACKITEM_N(_CL("CBTAPManagerImpl"), _CL("CreateBTAccessPoint"));

	#ifndef __S60V2__
	return -1;

	// FIX ME:
	//investigate about 7650 comm db structure to have it working.
		
	return 0;
	#else
	
	
	
	CCommsDatabase * db = CCommsDatabase::NewL(EDatabaseTypeIAP);
	CleanupStack::PushL(db);
	db->ShowHiddenRecords();

	CCommsDbTableView * table;
	TInt err;
	TUint32 newModemId, networkId, dialOutIspId, iapId, wapAPId, wapIpBearerId;

	// BEARER TABLE -------------------------------------------------------------
	table=db->OpenTableLC(TPtrC(MODEM));
	err = table->InsertRecord(newModemId);

	table->WriteTextL(TPtrC(COMMDB_NAME), KContextModem);
	table->WriteTextL(TPtrC(MODEM_AGENT), _L("csd.agt"));
	table->WriteTextL(TPtrC(MODEM_PORT_NAME), _L("BTCOMM::1"));
	table->WriteTextL(TPtrC(MODEM_TSY_NAME), _L("PHONETSY"));
	table->WriteTextL(TPtrC(MODEM_NIF_NAME), _L("PPP"));
	table->WriteTextL(TPtrC(MODEM_CSY_NAME), _L("BTCOMM"));
	table->WriteUintL(TPtrC(MODEM_DATA_BITS), (TUint8)3);
	table->WriteUintL(TPtrC(MODEM_STOP_BITS), (TUint8)0);
	table->WriteUintL(TPtrC(MODEM_PARITY), (TUint8)0);
	table->WriteUintL(TPtrC(MODEM_RATE), (TUint32)15);
	table->WriteUintL(TPtrC(MODEM_HANDSHAKING), (TUint32)196);
	table->WriteUintL(TPtrC(MODEM_SPECIAL_RATE), (TUint32) 0);
	table->WriteUintL(TPtrC(MODEM_XON_CHAR), (TUint8) 17);
	table->WriteUintL(TPtrC(MODEM_XOFF_CHAR), (TUint8) 19);
	table->WriteUintL(TPtrC(MODEM_FAX_CLASS_PREF), (TUint8)0);
	table->WriteUintL(TPtrC(MODEM_SPEAKER_PREF), (TUint8)0);
	table->WriteUintL(TPtrC(MODEM_SPEAKER_VOL_PREF), (TUint8) 0);
	table->WriteTextL(TPtrC(MODEM_MODEM_INIT_STRING), _L8("ATZ"));
	table->WriteTextL(TPtrC(MODEM_DATA_INIT_STRING), _L8("AT"));
	table->WriteTextL(TPtrC(MODEM_FAX_INIT_STRING), _L8("AT"));
	table->WriteTextL(TPtrC(MODEM_ISP_INIT_STRING), _L8(""));
	table->WriteTextL(TPtrC(MODEM_DIAL_PAUSE_LENGTH), _L8("S8="));
	table->WriteTextL(TPtrC(MODEM_CARRIER_TIMEOUT), _L8(""));
	table->WriteTextL(TPtrC(MODEM_AUTO_ANSWER_RING_COUNT),_L8(""));
	table->WriteTextL(TPtrC(MODEM_SPEAKER_ALWAYS_OFF), _L8("M0"));
	table->WriteTextL(TPtrC(MODEM_SPEAKER_ALWAYS_ON),  _L8("M2"));
	table->WriteTextL(TPtrC(MODEM_SPEAKER_ON_AFTER_DIAL_UNTIL_CARRIER), _L8("M3"));
	table->WriteTextL(TPtrC(MODEM_SPEAKER_ON_UNTIL_CARRIER), _L8("M1"));
	table->WriteTextL(TPtrC(MODEM_SPEAKER_VOL_CONTROL_HIGH), _L8("L2"));
	table->WriteTextL(TPtrC(MODEM_SPEAKER_VOL_CONTROL_LOW), _L8("L0"));
	table->WriteTextL(TPtrC(MODEM_SPEAKER_VOL_CONTROL_MEDIUM), _L8("L1"));
	table->WriteTextL(TPtrC(MODEM_DIAL_TONE_WAIT_MODIFIER), _L8("W"));
	table->WriteTextL(TPtrC(MODEM_CALL_PROGRESS_1), _L8("X1"));
	table->WriteTextL(TPtrC(MODEM_CALL_PROGRESS_2), _L8("X2"));
	table->WriteTextL(TPtrC(MODEM_CALL_PROGRESS_3), _L8("X3"));
	table->WriteTextL(TPtrC(MODEM_CALL_PROGRESS_4), _L8("X4"));
	table->WriteTextL(TPtrC(MODEM_ECHO_OFF), _L8("E0"));
	table->WriteTextL(TPtrC(MODEM_VERBOSE_TEXT), _L8("V1"));
	table->WriteTextL(TPtrC(MODEM_QUIET_OFF), _L8("Q0"));
	table->WriteTextL(TPtrC(MODEM_QUIET_ON), _L8("Q1"));
	table->WriteTextL(TPtrC(MODEM_DIAL_COMMAND_STATE_MODIFIER), _L8(""));
	table->WriteTextL(TPtrC(MODEM_ON_LINE), _L8("O"));
	table->WriteTextL(TPtrC(MODEM_RESET_CONFIGURATION), _L8("Z"));
	table->WriteTextL(TPtrC(MODEM_RETURN_TO_FACTORY_DEFS), _L8("&F"));
	table->WriteTextL(TPtrC(MODEM_DCD_ON_DURING_LINK), _L8("&C1"));
	table->WriteTextL(TPtrC(MODEM_DTR_HANG_UP), _L8("&D2"));
	table->WriteTextL(TPtrC(MODEM_DSR_ALWAYS_ON), _L8("&S0"));
	table->WriteTextL(TPtrC(MODEM_RTS_CTS_HANDSHAKE), _L8("&K3"));
	table->WriteTextL(TPtrC(MODEM_XON_XOFF_HANDSHAKE), _L8("&K4"));
	table->WriteTextL(TPtrC(MODEM_ESCAPE_CHARACTER), _L8("+"));
	table->WriteTextL(TPtrC(MODEM_ESCAPE_GUARD_PERIOD), _L8("S12"));
	table->WriteTextL(TPtrC(MODEM_FAX_CLASS_INTERROGATE), _L8(""));
	table->WriteTextL(TPtrC(MODEM_FAX_CLASS), _L8(""));
	table->WriteTextL(TPtrC(MODEM_NO_ANSWER), _L8("NO ANSWER"));
	table->WriteTextL(TPtrC(MODEM_NO_DIAL_TONE), _L8("NO DIAL TONE"));
	table->WriteTextL(TPtrC(MODEM_BUSY), _L8("BUSY"));
	table->WriteTextL(TPtrC(MODEM_CARRIER), _L8("CARRIER"));
	table->WriteTextL(TPtrC(MODEM_CONNECT), _L8("CONNECT"));
	table->WriteTextL(TPtrC(MODEM_COMPRESSION_CLASS_5), _L8("COMPRESSION:CLASS 5"));
	table->WriteTextL(TPtrC(MODEM_COMPRESSION_NONE), _L8("COMPRESSION:NONE"));
	table->WriteTextL(TPtrC(MODEM_COMPRESSION_V42BIS), _L8("CONMPRESSION:V.42 bis"));
	table->WriteTextL(TPtrC(MODEM_PROTOCOL_ALT), _L8("PROTOCOL:ALT"));
	table->WriteTextL(TPtrC(MODEM_PROTOCOL_ALTCELLULAR), _L8("PROTOCOL:ART-CELLULAR"));
	table->WriteTextL(TPtrC(MODEM_PROTOCOL_LAPD), _L8("PROTOCOL:LAPD"));
	table->WriteTextL(TPtrC(MODEM_PROTOCOL_NONE), _L8("PROTOCOL:NONE"));
	//table->WriteUintL(TPtrC(MODEM_MESSAGE_CENTER_NUMBER), _
	//table->WriteTextL(TPtrC(MODEM_MESSAGE_DELIVERY_REPORT
	//table->WriteTextL(TPtrC(MODEM_MESSAGE_VALIDITY_PERIOD),
	//table->WriteTextL(TPtrC(MODEM_CHANNEL_PORT_NAME), 
	table->WriteUintL(TPtrC(MODEM_MIN_SIGNAL_LEVEL), (TUint)0);
	
	table->PutRecordChanges();
	CleanupStack::PopAndDestroy(table);
	table = NULL;


	// NETWORK TABLE -------------------------------------------------------------
	table=db->OpenTableLC(TPtrC(NETWORK));
	err = table->InsertRecord(networkId);

	table->WriteTextL(TPtrC(COMMDB_NAME), KContextAP);

	table->PutRecordChanges();
	CleanupStack::PopAndDestroy(table);
	table = NULL;


	// LOCATION TABLE ------------------------------------------------------------
	table=db->OpenTableLC(TPtrC(LOCATION));
	
	//Find line where name = MOBILE; get id in locationId

	CleanupStack::PopAndDestroy(table);
	table = NULL;


	// TABLE DIAL_OUT_ISP ---------------------------------------------------------
	table=db->OpenTableLC(TPtrC(DIAL_OUT_ISP));
	err = table->InsertRecord(dialOutIspId);

	table->WriteTextL(TPtrC(COMMDB_NAME), KContextAP);
	table->WriteTextL(TPtrC(ISP_DESCRIPTION), KContextAP);
	table->WriteUintL(TPtrC(ISP_TYPE), /*TCommsDbIspType::EIspTypeInternetAndWAP*/ 2);
	table->WriteTextL(TPtrC(ISP_DEFAULT_TEL_NUM), _L(""));
	table->WriteBoolL(TPtrC(ISP_DIAL_RESOLUTION), EFalse);
	table->WriteBoolL(TPtrC(ISP_USE_LOGIN_SCRIPT), ETrue);
	table->WriteLongTextL(TPtrC(ISP_LOGIN_SCRIPT), KLoginScript);
	table->WriteBoolL(TPtrC(ISP_PROMPT_FOR_LOGIN), EFalse);
	table->WriteTextL(TPtrC(ISP_LOGIN_NAME), _L("bt"));
	table->WriteTextL(TPtrC(ISP_LOGIN_PASS), _L("bt"));
	table->WriteBoolL(TPtrC(ISP_DISPLAY_PCT), EFalse);
	CC_TRAP(err, table->WriteTextL(TPtrC(TEST_IF_NAME), _L("PPP"));)		// FIXME : ??? ISP_IF_NAME
	table->WriteTextL(TPtrC(ISP_IF_NETWORKS), _L("ip"));
	table->WriteBoolL(TPtrC(ISP_IF_PROMPT_FOR_AUTH), EFalse);
	table->WriteTextL(TPtrC(ISP_IF_AUTH_NAME), _L("bt"));
	table->WriteTextL(TPtrC(ISP_IF_AUTH_PASS), _L("bt"));
	table->WriteUintL(TPtrC(ISP_IF_AUTH_RETRIES), 0 );  
	table->WriteBoolL(TPtrC(ISP_IF_CALLBACK_ENABLED), EFalse);
	table->WriteTextL(TPtrC(ISP_IF_CALLBACK_INFO), _L8(""));
	table->WriteUintL(TPtrC(ISP_IF_CALLBACK_TYPE), 1002);
	table->WriteUintL(TPtrC(ISP_CALLBACK_TIMEOUT), 60000000);
	table->WriteBoolL(TPtrC(ISP_IP_ADDR_FROM_SERVER), ETrue);
	table->WriteTextL(TPtrC(ISP_IP_ADDR), _L("0.0.0.0"));
	table->WriteTextL(TPtrC(ISP_IP_NETMASK), _L(""));
	table->WriteTextL(TPtrC(ISP_IP_GATEWAY), _L("0.0.0.0"));
	table->WriteBoolL(TPtrC(ISP_IP_DNS_ADDR_FROM_SERVER), ETrue);
	table->WriteTextL(TPtrC(ISP_IP_NAME_SERVER1), _L("0.0.0.0"));
	table->WriteTextL(TPtrC(ISP_IP_NAME_SERVER2), _L("0.0.0.0"));
	table->WriteBoolL(TPtrC(ISP_ENABLE_IP_HEADER_COMP), EFalse);
	table->WriteBoolL(TPtrC(ISP_ENABLE_LCP_EXTENSIONS), EFalse);
	table->WriteBoolL(TPtrC(ISP_DISABLE_PLAIN_TEXT_AUTH), EFalse); 
	table->WriteBoolL(TPtrC(ISP_ENABLE_SW_COMP), ETrue);
	table->WriteUintL(TPtrC(ISP_BEARER_CE), 2);		// investigate : apparently always 2.
	table->WriteUintL(TPtrC(ISP_BEARER_NAME), 0);
	table->WriteUintL(TPtrC(ISP_BEARER_SPEED), 1);
	//table->WriteUintL(TPtrC(ISP_INIT_STRING), NULL);
	table->WriteUintL(TPtrC(ISP_BEARER_TYPE), 0);
	table->WriteUintL(TPtrC(ISP_BEARER_SERVICE), 1);	// investigate ???
	table->WriteUintL(TPtrC(ISP_CHANNEL_CODING), 0);
	table->WriteUintL(TPtrC(ISP_AIUR), 0);
	table->WriteUintL(TPtrC(ISP_REQUESTED_TIME_SLOTS), 0);
	table->WriteUintL(TPtrC(ISP_MAXIMUM_TIME_SLOTS), 0);
	table->WriteBoolL(TPtrC(ISP_USER_INIT_UPGRADE), EFalse);
	
	table->PutRecordChanges();
	CleanupStack::PopAndDestroy(table);
	table = NULL;


	// TABLE IAP --------------------------------------------------------
	table=db->OpenTableLC(TPtrC(IAP));
	err = table->InsertRecord(iapId);
	
	table->WriteTextL(TPtrC(COMMDB_NAME), KContextAP);
	table->WriteTextL(TPtrC(IAP_SERVICE_TYPE), TPtrC(DIAL_OUT_ISP));
	
	table->WriteUintL(TPtrC(IAP_SERVICE), dialOutIspId); //
	table->WriteUintL(TPtrC(IAP_BEARER), newModemId); 
	
	//table->WriteUintL(TPtrC(IAP_SERVICE), 3); 
	//table->WriteUintL(TPtrC(IAP_BEARER), 3); 
		
	table->WriteTextL(TPtrC(IAP_BEARER_TYPE), TPtrC(MODEM_BEARER));		
	table->WriteUintL(TPtrC(IAP_NETWORK), networkId);
	table->WriteUintL(TPtrC(IAP_NETWORK_WEIGHTING), 0);
	//table->WriteUintL(TPtrC(IAP_LOCATION), locationId);   
	table->WriteUintL(TPtrC(IAP_LOCATION), 2);		//FIXME
	//table->WriteUintL(TPtrC(IAP_DIALOG_PREF), 0);		
		
	table->PutRecordChanges();
	CleanupStack::PopAndDestroy(table);
	table = NULL;


	// TABLE WAP_ACCESS_POINT ---------------------------------------------------
	table = db->OpenTableLC(TPtrC(WAP_ACCESS_POINT));
	
	err = table->InsertRecord(wapAPId);
	table->WriteTextL(TPtrC(COMMDB_NAME), KContextAP);
	table->WriteTextL(TPtrC(WAP_CURRENT_BEARER), TPtrC(WAP_IP_BEARER));
	table->WriteTextL(TPtrC(WAP_START_PAGE), _L("http://www.google.com/wml"));

	table->PutRecordChanges();
	CleanupStack::PopAndDestroy(table);
	table = NULL;

	// TABLE WAP_IP_BEARER -----------------------------------------------------
	table = db->OpenTableLC(TPtrC(WAP_IP_BEARER));
	table->InsertRecord(wapIpBearerId);

	table->WriteUintL(TPtrC(WAP_ACCESS_POINT_ID), wapAPId );
	table->WriteUintL(TPtrC(WAP_IAP), iapId);
	table->WriteUintL(TPtrC(WAP_WSP_OPTION), 1);
	table->WriteUintL(TPtrC(WAP_PROXY_PORT), 0);

	table->PutRecordChanges();
	CleanupStack::PopAndDestroy(table);
	table = NULL;

	CleanupStack::PopAndDestroy(db);
	return iapId;
	#endif
}

class CApSettingMaintainerImpl : public CApSettingMaintainer, public MContextBase, public MSettingListener
{
public:
	~CApSettingMaintainerImpl();
private:
	CApSettingMaintainerImpl(MApp_context& Context);
	void ConstructL();
	void SettingChanged(TInt setting);

	friend class CApSettingMaintainer;
	TInt iAP;
};

CApSettingMaintainer* CApSettingMaintainer::NewL(MApp_context& Context)
{
	auto_ptr<CApSettingMaintainerImpl> ret(new (ELeave) CApSettingMaintainerImpl(Context));
	ret->ConstructL();
	return ret.release();
}

CApSettingMaintainer::~CApSettingMaintainer()
{
}

CApSettingMaintainerImpl::CApSettingMaintainerImpl(MApp_context& Context) : MContextBase(Context)
{
}

CApSettingMaintainerImpl::~CApSettingMaintainerImpl()
{
	Settings().CancelNotifyOnChange(SETTING_IP_AP, this);
}

void CApSettingMaintainerImpl::ConstructL()
{
	Settings().NotifyOnChange(SETTING_IP_AP, this);
	Settings().GetSettingL(SETTING_CURRENT_AP, iAP);
}

void CApSettingMaintainerImpl::SettingChanged(TInt)
{
	TInt ap=0;
	Settings().GetSettingL(SETTING_CURRENT_AP, ap);
	if (ap==iAP) {
		Settings().GetSettingL(SETTING_IP_AP, ap);
		Settings().WriteSettingL(SETTING_CURRENT_AP, ap);
		iAP=ap;
	}
}
