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

#include "ap_creation.h"
#include <cdbcols.h>
#include <commdb.h>
#include "symbian_auto_ptr.h"
#ifndef __S60V2__
#include <cdbpreftable.h>
#endif

TUint32 OpenOrCreateL(CCommsDbTableView* aTable, const TDesC& aColumnName,
					  TUint32 aValue)
{
	TUint32 id;
	TUint32 value;
	TInt err=aTable->GotoFirstRecord();
	while(err==KErrNone) {
		aTable->ReadUintL(aColumnName, value);
		if (aValue==value) {
			aTable->ReadUintL(TPtrC(COMMDB_ID), id);
			User::LeaveIfError(aTable->UpdateRecord());
			return id;
		}
		err=aTable->GotoNextRecord();
	}
	if (err==KErrNotFound) {
		User::LeaveIfError(aTable->InsertRecord(id));
		return id;
	}
	User::LeaveIfError(err);
}

TUint32 OpenOrCreateL(CCommsDbTableView* aTable, const TDesC& aName)
{
	TUint32 id;
	auto_ptr<HBufC> name(HBufC::NewL(50));
	TInt err=aTable->GotoFirstRecord();
	while(err==KErrNone) {
		TPtr p=name->Des();
		aTable->ReadTextL(TPtrC(COMMDB_NAME), p);
		if ( (*name).CompareF(aName)==0) {
			aTable->ReadUintL(TPtrC(COMMDB_ID), id);
			User::LeaveIfError(aTable->UpdateRecord());
			return id;
		}
		err=aTable->GotoNextRecord();
	}
	if (err==KErrNotFound) {
		User::LeaveIfError(aTable->InsertRecord(id));
		return id;
	}
	User::LeaveIfError(err);
}

EXPORT_C void DeleteAllOtherAPs(const TDesC& aName)
{
	auto_ptr<CCommsDatabase> dbp(CCommsDatabase::NewL(EDatabaseTypeIAP));
	CCommsDatabase& db=*dbp;
	User::LeaveIfError(db.BeginTransaction());
	CCommsDbTableView* aTable=db.OpenTableLC(TPtrC(IAP));
	TInt err=aTable->GotoFirstRecord();
	TBuf<50> name;
	while(err==KErrNone) {
		aTable->ReadTextL(TPtrC(COMMDB_NAME), name);
		if (name.CompareF(aName)!=0) {
			aTable->DeleteRecord();
		}
		err=aTable->GotoNextRecord();
	}
	User::LeaveIfError(db.CommitTransaction());
}

EXPORT_C TUint32 CreateAPL(const TDesC& aName,
						const TDesC& aAPN,
						const TDesC& aUsername,
						const TDesC& aPassword)
{
	auto_ptr<CCommsDatabase> dbp(CCommsDatabase::NewL(EDatabaseTypeIAP));
	CCommsDatabase& db=*dbp;
	User::LeaveIfError(db.BeginTransaction());

	TUint32 gprsId;
	{
		CCommsDbTableView* gprsTable=db.OpenTableLC(TPtrC(OUTGOING_GPRS));
		
		gprsId=OpenOrCreateL(gprsTable, aName);
		gprsTable->WriteTextL(TPtrC(COMMDB_NAME), aName);
		gprsTable->WriteTextL(TPtrC(GPRS_APN), aAPN);
		gprsTable->WriteUintL(TPtrC(GPRS_PDP_TYPE), 0);

		gprsTable->WriteUintL(TPtrC(GPRS_REQ_PRECEDENCE), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_REQ_DELAY), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_REQ_RELIABILITY), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_REQ_PEAK_THROUGHPUT), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_REQ_MEAN_THROUGHPUT), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_MIN_PRECEDENCE), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_MIN_DELAY), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_MIN_RELIABILITY), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_MIN_PEAK_THROUGHPUT), 0);
		gprsTable->WriteUintL(TPtrC(GPRS_MIN_MEAN_THROUGHPUT), 0);

		gprsTable->WriteBoolL(TPtrC(GPRS_DATA_COMPRESSION), EFalse);
		gprsTable->WriteBoolL(TPtrC(GPRS_HEADER_COMPRESSION), EFalse);
		gprsTable->WriteBoolL(TPtrC(GPRS_ANONYMOUS_ACCESS), EFalse);
	#ifndef __S60V2__
	#  ifdef __WINS__
		gprsTable->WriteTextL(TPtrC(GPRS_IF_NAME), _L("PPP"));
	#  else
		gprsTable->WriteTextL(TPtrC(GPRS_IF_NAME), _L("pppgprs"));
	#  endif
	#endif
		gprsTable->WriteTextL(TPtrC(GPRS_IF_NETWORKS), _L("ip"));
		gprsTable->WriteBoolL(TPtrC(GPRS_IF_PROMPT_FOR_AUTH), EFalse);
	#ifdef __S60V2__
		gprsTable->WriteUintL(TPtrC(GPRS_AP_TYPE), 2);
	#endif
		//gprsTable->WriteTextL(TPtrC(GPRS_IP_GATEWAY), _L("0.0.0.0"));
		gprsTable->WriteTextL(TPtrC(GPRS_IF_AUTH_NAME), aUsername);
		gprsTable->WriteTextL(TPtrC(GPRS_IF_AUTH_PASS), aPassword);
		gprsTable->WriteBoolL(TPtrC(GPRS_DISABLE_PLAIN_TEXT_AUTH),
				EFalse);
		gprsTable->WriteUintL(TPtrC(GPRS_IF_AUTH_RETRIES), 0);

		gprsTable->WriteBoolL(TPtrC(GPRS_IP_ADDR_FROM_SERVER), ETrue);
		gprsTable->WriteTextL(TPtrC(GPRS_IP_ADDR), _L("0.0.0.0"));
		gprsTable->WriteBoolL(TPtrC(GPRS_IP_DNS_ADDR_FROM_SERVER), ETrue);
		gprsTable->WriteTextL(TPtrC(GPRS_IP_NAME_SERVER1), _L("0.0.0.0"));
		gprsTable->WriteTextL(TPtrC(GPRS_IP_NAME_SERVER2), _L("0.0.0.0"));

		gprsTable->WriteBoolL(TPtrC(GPRS_ENABLE_LCP_EXTENSIONS), EFalse);

		User::LeaveIfError(gprsTable->PutRecordChanges());
		CleanupStack::PopAndDestroy();
	}

#ifdef __S60V2__
	TUint32 locationId=0;
	TUint32 networkId=0;
	{
		CCommsDbTableView* locationTable=db.OpenTableLC(TPtrC(LOCATION));
		locationId=OpenOrCreateL(locationTable, _L("Mobile"));
		locationTable->CancelRecordChanges();
		CleanupStack::PopAndDestroy();

		CCommsDbTableView* networkTable=db.OpenTableLC(TPtrC(NETWORK));
		networkId=OpenOrCreateL(networkTable, aName);
		networkTable->WriteTextL(TPtrC(COMMDB_NAME), aName);
		User::LeaveIfError(networkTable->PutRecordChanges());
		CleanupStack::PopAndDestroy();
	}
#endif

	TUint32 iapId=0;
	{
	CCommsDbTableView* iapTable=db.OpenTableLC(TPtrC(IAP));
	iapId=OpenOrCreateL(iapTable, aName);
	iapTable->WriteTextL(TPtrC(COMMDB_NAME), aName);
	iapTable->WriteTextL(TPtrC(IAP_SERVICE_TYPE), TPtrC(OUTGOING_GPRS));
	iapTable->WriteUintL(TPtrC(IAP_SERVICE), gprsId); // ID FROM GPRS TABLE
#ifdef __S60V2__
	iapTable->WriteUintL(TPtrC(IAP_NETWORK), networkId); // ID FROM NETWORK TABLE
	iapTable->WriteUintL(TPtrC(IAP_LOCATION), locationId); // ID FROM LOCATION
	iapTable->WriteUintL(TPtrC(IAP_NETWORK_WEIGHTING), 0);
	iapTable->WriteUintL(TPtrC(IAP_BEARER), 2);
	iapTable->WriteTextL(TPtrC(IAP_BEARER_TYPE), TPtrC(MODEM_BEARER));
#endif
	User::LeaveIfError(iapTable->PutRecordChanges());
	CleanupStack::PopAndDestroy();
	}

	TUint32 wapapId=0;
	{
	CCommsDbTableView* wapapTable=db.OpenTableLC(TPtrC(WAP_ACCESS_POINT));
	wapapId=OpenOrCreateL(wapapTable, aName);
	wapapTable->WriteTextL(TPtrC(COMMDB_NAME), aName);
	wapapTable->WriteTextL(TPtrC(WAP_CURRENT_BEARER), TPtrC(WAP_IP_BEARER));
	User::LeaveIfError(wapapTable->PutRecordChanges());
	CleanupStack::PopAndDestroy();
	}

	{
	CCommsDbTableView* wapipTable=db.OpenTableLC(TPtrC(WAP_IP_BEARER));
	TUint32 wapipId=OpenOrCreateL(wapipTable, 
		TPtrC(WAP_ACCESS_POINT_ID), wapapId);
	wapipTable->WriteUintL(TPtrC(WAP_ACCESS_POINT_ID), wapapId);
#ifndef __S60V2__
	wapipTable->WriteUintL(TPtrC(WAP_CHARGECARD), 0);
	wapipTable->WriteUintL(TPtrC(WAP_ISP), gprsId);
	wapipTable->WriteUintL(TPtrC(WAP_ISP_TYPE), 2);
	wapipTable->WriteUintL(TPtrC(WAP_LOCATION), 0);
#else
	wapipTable->WriteUintL(TPtrC(WAP_PROXY_PORT), 0);
#endif
	wapipTable->WriteTextL(TPtrC(WAP_GATEWAY_ADDRESS), _L("0.0.0.0"));
	wapipTable->WriteUintL(TPtrC(WAP_IAP), iapId);
	wapipTable->WriteBoolL(TPtrC(WAP_SECURITY), EFalse);
	wapipTable->WriteUintL(TPtrC(WAP_WSP_OPTION), 1);	
	User::LeaveIfError(wapipTable->PutRecordChanges());
	CleanupStack::PopAndDestroy();
	}

	User::LeaveIfError(db.CommitTransaction());

#ifndef __S60V2__
	{
	CCommsDbConnectionPrefTableView* ptv = 
		db.OpenConnectionPrefTableLC(
			ECommDbConnectionDirectionOutgoing);
	TInt err=ptv->GotoFirstRecord();
	while (err==KErrNone) {
		ptv->DeleteConnectionPreferenceL();
		err=ptv->GotoNextRecord();
	}
	
	CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref pref;
	pref.iDirection = ECommDbConnectionDirectionOutgoing;
	pref.iDialogPref = ECommDbDialogPrefDoNotPrompt;
	pref.iBearer.iBearerSet = ECommDbBearerGPRS;
	pref.iBearer.iIapId = iapId;
	pref.iRanking = 1;
	ptv->InsertConnectionPreferenceL(pref);
	CleanupStack::PopAndDestroy();
	}
#endif
	return iapId;
}
