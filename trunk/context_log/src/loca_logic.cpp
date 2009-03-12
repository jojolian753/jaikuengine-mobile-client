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
#include "loca_logic.h"

#include "cbbsession.h"
#include "db.h"
#include "csd_loca.h"
#include <e32math.h>
#include "csd_event.h"
#include "cl_settings.h"
#include "i_logger.h"
#include "app_context_impl.h"
#include "reporting.h"
#include <Python.h>
#include <charconv.h>
#include "symbian_python_ptr.h"
#include <cspyinterpreter.h>

_LIT(KLocaLogic, "localogic");

#define MAX_STATS 5000
#define STATS_HYSTERESIS 50

_LIT(KTime, "t");

class CAcceptedMsgs : 
	public CBase, public MContextBase, public MDBStore {
public:
	CAcceptedMsgs(RDbDatabase& aDb) : MDBStore(aDb) { }
	enum TColumn {
		EMac = 1,
		EMessageCode =2,
		ELastSeen,
	};
	TInt iCount;
	void ConstructL() {
		TInt cols[]= { EDbColText8, EDbColInt32, EDbColDateTime, -1 };
		TInt idx[]= { EMac, EMessageCode, -2, ELastSeen, -1 };

		MDBStore::SetTextLen(6);
		MDBStore::ConstructL(cols, idx, false, _L("ACCEPTED"), ETrue);
		iCount=iTable.CountL();
	}

	TBool HasBeenAcceptedL(const TDesC8& aMac, TInt aCode)
	{
		TDbSeekMultiKey<2> rk;
		rk.Add(aMac);
		rk.Add(aCode);
		SwitchIndexL(0);
		return iTable.SeekL(rk);
	}
	static CAcceptedMsgs* NewL(RDbDatabase& aDb) {
		auto_ptr<CAcceptedMsgs> ids(new (ELeave) CAcceptedMsgs(aDb));
		ids->ConstructL();
		return ids.release();
	}
	void SetAsAcceptedL(const TDesC8& aMac, TInt aMessageCode) {
		if (iCount>MAX_STATS || NoSpaceLeft()) {
			SwitchIndexL(1);
			iTable.FirstL();
			TInt deleted=0;
			while(deleted<STATS_HYSTERESIS) {
				DeleteL();
				iCount--;
				deleted++;
				iTable.NextL();
			}
		}
		iTable.InsertL();
		iTable.SetColL(EMac, aMac);
		iTable.SetColL(EMessageCode, aMessageCode);
		iTable.SetColL(ELastSeen, GetTime());
		PutL();
	}
};

struct TMessageStats {
	TInt	iFailureCount; 
	TInt	iLocalFailureCount;
	TInt	iSuccessCount; 
	TTime	iPreviousLocalSuccess; 
	TTime	iPreviousRemoteSuccess; 
	TTime	iPreviousLocalFailure; 
	TTime	iPreviousRemoteFailure;
	TTime	iLastSeen;

	bool operator==(const TMessageStats& aRhs) const {
		return  (
			iFailureCount == aRhs.iFailureCount &&
			iLocalFailureCount == aRhs.iLocalFailureCount &&
			iSuccessCount == aRhs.iSuccessCount && 
			iPreviousLocalSuccess == aRhs.iPreviousLocalSuccess &&
			iPreviousRemoteSuccess == aRhs.iPreviousRemoteSuccess &&
			iPreviousLocalFailure == aRhs.iPreviousLocalFailure &&
			iPreviousRemoteFailure == aRhs.iPreviousRemoteFailure &&
			iLastSeen == aRhs.iLastSeen
				);
	}
	void IntoStringL(TDes& aInto) const {
		const TInt* ints[]={ &iFailureCount, &iLocalFailureCount,
			&iSuccessCount, 0 };
		for (const TInt** i=&ints[0]; *i; i++) {
			TBBInt t(**i, KTime);
			t.IntoStringL(aInto);
			if (*i!=&iFailureCount) AppendCheckingSpaceL(aInto, _L(";"));
		}
		const TTime* times[]={ &iPreviousLocalSuccess, 
			&iPreviousRemoteSuccess,
			&iPreviousLocalFailure, 
			&iPreviousRemoteFailure, 
			&iLastSeen, 0 };
		for (const TTime** time=&times[0]; *time; time++) {
			TBBTime t(**time, KTime);
			t.IntoStringL(aInto);
			AppendCheckingSpaceL(aInto, _L(";"));
		}
	}

};


struct TDevStats {
	TTime	iLastSeen;
	TTime	iVisitBegin;
	TTime	iPreviousVisitBegin;
	TTime	iPreviousVisitEnd;
	TTime	iFirstSeen;
	TInt	iSumStay;
	TInt	iSquareSumStay;
	TInt	iCountStay;
	TInt	iMaxStay;

	bool operator==(const TDevStats& aRhs) const {
		return  (
		iLastSeen == aRhs.iLastSeen &&
		iVisitBegin == aRhs.iVisitBegin &&
		iPreviousVisitBegin == aRhs.iPreviousVisitBegin &&
		iPreviousVisitEnd == aRhs.iPreviousVisitEnd &&
		iSumStay == aRhs.iSumStay &&
		iSquareSumStay == aRhs.iSquareSumStay &&
		iCountStay == aRhs.iCountStay &&
		iMaxStay == aRhs.iMaxStay &&
		iFirstSeen == aRhs.iFirstSeen );
	}
	void IntoStringL(TDes& aInto) const {
		const TTime* times[]={ &iLastSeen, &iVisitBegin,
			&iPreviousVisitBegin, &iPreviousVisitEnd, 
			&iFirstSeen, 0 };
		for (const TTime** time=&times[0]; *time; time++) {
			TBBTime t(**time, KTime);
			t.IntoStringL(aInto);
			AppendCheckingSpaceL(aInto, _L(";"));
		}
		const TInt* ints[]={ &iSumStay, &iSquareSumStay,
			&iCountStay, &iMaxStay, 0 };
		for (const TInt** i=&ints[0]; *i; i++) {
			TBBInt t(**i, KTime);
			t.IntoStringL(aInto);
			if (*i!=&iCountStay) AppendCheckingSpaceL(aInto, _L(";"));
		}
	}
};

class CBtStats : public CBase, public MContextBase, public MDBStore {
protected:
	CBtStats(MApp_context& aContext, RDbDatabase& aDb);
	TTime	GetColTime(TInt aColumn);
	TInt	GetColInt(TInt aColumn);
};

class CMessageStats : public CBtStats {
public:
	static CMessageStats* NewL(MApp_context& aContext, RDbDatabase& aDb);

	void GetStatsL(const TBBBtDeviceInfo& aDevice,
		TMessageStats& aStats);

	void SetStatsL(const TBBBtDeviceInfo& aDevice,
		const TMessageStats& aStats);
private:
	enum TColumns {
		EDevAddr = 1,
		EFailureCount,
		ESuccessCount,
		EPreviousLocalSuccess,
		EPreviousRemoteSuccess,
		EPreviousLocalFailure,
		EPreviousRemoteFailure,
		ELocalFailureCount,
		ELastSeen,
	};
	enum TIndices {
		EIdxDev,
		EIdxLastSeen
	};

	void ConstructL();
	CMessageStats(MApp_context& aContext, RDbDatabase& aDb);
	TBool	SeekToDevL(const TBBBtDeviceInfo& aDevice);
	TInt iCount;
};

class CNodeTable : public CBase, public MDBStore {
public:
	CNodeTable(RDbDatabase& aDb) : MDBStore(aDb) { }
	enum TColumn {
		EId = 1,
		ENodeName
	};
	enum TIndices {
		EIdxId = 0,
		EIdxNodeName
	};
	void ConstructL() {
		TInt cols[]= { 
			EDbColUint32, /* id */
			EDbColText, /* node */
			-1 };
		TInt idxs[]= { EId, -2, ENodeName, -1 };
		TInt col_flags[]={ TDbCol::EAutoIncrement, 0, };

		MDBStore::ConstructL(cols, idxs, true, _L("NODENAME"), ETrue,
			col_flags);
	}
	void GetNodeNameL(TUint aId, TDes& aName)  {
		SwitchIndexL(EIdxId);
		TDbSeekKey k(aId);
		if (iTable.SeekL(k)) {
			iTable.GetL();
			aName=iTable.ColDes(ENodeName);
		} else {
			User::Leave(KErrNotFound);
		}
	}
	TUint GetNodeIdL(const TDesC& aName) {
		SwitchIndexL(EIdxNodeName);
		TDbSeekKey k(aName);
		if (iTable.SeekL(k)) {
			iTable.GetL();
			return iTable.ColUint32(EId);
		} else {
			TUint id;
			iTable.InsertL();
			iTable.SetColL(ENodeName, aName);
			id=iTable.ColUint32(EId);
			PutL();
			return id;
		}
	}
};

class CDevStats : public CBtStats {
public:
	static CDevStats* NewL(MApp_context& aContext, RDbDatabase& aDb);

	void GetStatsL(const TBBBtDeviceInfo& aDevice,
		const TDesC& aNode,
		TDevStats& aStats);

	TBool FirstStats(const TBBBtDeviceInfo& aDevice, TDes& aNode, TDevStats& aStats);
	TBool NextStats(const TBBBtDeviceInfo& aDevice, TDes& aNode, TDevStats& aStats);
	void GetCurrentStatsL(TDevStats& aStats);

	void SetStatsL(const TBBBtDeviceInfo& aDevice,
		const TDesC& aNode,
		const TDevStats& aStats);
	~CDevStats() {
		delete iNodes;
	}
private:
	enum TColumns {
		EDevAddr = 1,
		ENode,
		ELastSeen,
		EVisitBegin,
		EPreviousVisitBegin,
		EPreviousVisitEnd,
		ESumStay,
		ESquareSumStay,
		ECountStay,
		EMaxStay,
		EFirstSeen
	};
	enum TIndices {
		EIdxDev = 0,
		EIdxLastSeen = 1
	};

	CDevStats(MApp_context& aContext, RDbDatabase& aDb);
	void ConstructL();
	TBool	SeekToDevL(const TBBBtDeviceInfo& aDevice,
		const TDesC& aNode);
	CNodeTable*	iNodes;
	TInt	iCount;
};

class CLocaRemoteEvents : public CBase, public MContextBase,
	public Mlogger {
public:
	static CLocaRemoteEvents* NewL(MApp_context& aContext,
		CLocaLogic* aLogic);
	~CLocaRemoteEvents() { }
private:
	CLocaRemoteEvents(MApp_context& aContext,
		CLocaLogic* aLogic);
	void ConstructL();

	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
			const TComponentName& /*aComponent*/,
			const MBBData* aData);

	virtual void NewSensorEventL(const TTupleName& , 
		const TDesC& , const CBBSensorEvent& ) { }

	CLocaLogic*	iLogic;
};

CLocaRemoteEvents* CLocaRemoteEvents::NewL(MApp_context& aContext, CLocaLogic* aLogic)
{
	auto_ptr<CLocaRemoteEvents> ret(new (ELeave) CLocaRemoteEvents(aContext, aLogic));
	ret->ConstructL();
	return ret.release();
}

CLocaRemoteEvents::CLocaRemoteEvents(MApp_context& aContext, CLocaLogic* aLogic) :
	MContextBase(aContext), iLogic(aLogic) { }

void CLocaRemoteEvents::ConstructL()
{
	Mlogger::ConstructL(AppContextAccess());
	SubscribeL(KRemoteLocaLogicTuple);
}

void CLocaRemoteEvents::NewValueL(TUint aId, const TTupleName& , const TDesC& , 
		const TComponentName& /*aComponent*/,
		const MBBData* aData)
{
	const CBBSensorEvent* e=bb_cast<CBBSensorEvent>(aData);
	if (!e) 
		return;
	const TBBLocaMsgStatus* s=bb_cast<TBBLocaMsgStatus>(e->iData());
	if (!s) 
		return;

	TBBBtDeviceInfo dev(s->iRecipientAddress(), KNullDesC, 0, 0, 0);

	if (s->iSucceeded()) {
		iLogic->Success(dev, s->iMessageId(), s->iAtTime(), EFalse);
	} else {
		iLogic->Failed(dev, s->iMessageId(), s->iAtTime(),
			CLocaLogic::EUnknown, EFalse);
	}
	CC_TRAPD(err, iBBSubSessionNotif->DeleteL(aId));
	if (err!=KErrNone) {
		err=KErrNone;
	}
}

class CLocaLogicImpl : public CLocaLogic, public MContextBase,
	public MSettingListener, public MBBObserver {
private:

	CLocaLogicImpl(MApp_context& Context, RDbDatabase& aDb);
	void ConstructL();
	~CLocaLogicImpl();

public:
	CBBBtDeviceList* previous_devices;

	TBool DeviceListContains(const CBBBtDeviceList* aList,
		const TBBBtDeviceInfo* aToCheck) {
			const TBBBtDeviceInfo* i=0;
			if (!aToCheck) return EFalse;
			for (i=aList->First(); i; i=aList->Next()) {
				if (i->Equals(aToCheck)) return ETrue;
			}
			return EFalse;
	}
	// name and title max length 240
	virtual void GetMessage(const CBBBtDeviceList* devices,
		const TTime& aAtTime,
		TInt& doSendToIndex, 
		TInt& aMessageCode, TDes& aWithName,
		TDes& aWithTitle, auto_ptr<HBufC8>& aBody);
	virtual void GetMessageL(const CBBBtDeviceList* devices,
		const TTime& aAtTime,
		TInt& doSendToIndex, 
		TInt& aMessageCode, TDes& aWithName,
		TDes& aWithTitle, auto_ptr<HBufC8>& aBody);
	virtual void UpdateStats(const TDesC& aNodeName,
		const CBBBtDeviceList* devices, const TTime& aAtTime);

	virtual void Failed(const TBBBtDeviceInfo& aDevice,
		TInt aMessageCode,
		const TTime& aAtTime,
		TSendFailure aErrorCode,
		TBool	aLocal);
	virtual void Success(const TBBBtDeviceInfo& aDevice,
		TInt aMessageCode,
		const TTime& aAtTime,
		TBool	aLocal);

	virtual void SettingChanged(TInt Setting);
	void GetSettingsL();
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData);
	virtual void NewScriptL(const TDesC& aSubName, const MBBData* aData);
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName);
	void ConvertFromPythonString(TDes& aBuf, PyObject* aString);
	void ConvertFromPythonString(auto_ptr<HBufC8>& aBuf, PyObject* aString);
	void ReportPythonError(const TDesC& aPrefix);

	enum TScriptError {
		EIgnored,
		EHighestPriority,
		EPythonError,
		EOtherError
	};
	TScriptError RunScript(
		PyObject* main_module, PyObject* global_dict,
		const TDesC8& aName, TInt& max_seen_priority,
		PyObject* dict_general, PyObject* dict_devstats, 
		PyObject* dict_msgstats,
		TInt& aMessageCode, TDes& aWithName,
		TDes& aWithTitle, auto_ptr<HBufC8>& aBody,
		TDes& aErrorMessage, const TDesC8& aMac);

	RDbDatabase&	iDb;
	CDevStats*	iDevStats;
	CMessageStats* iMessageStats;

	TInt		iMaxFailures, iMaxSuccesses;

	TBBLocaMsgStatus	iMsgStatus;
	CBBSensorEvent		iEvent;
	CBBSubSession*		iBBSubSession;
	CLocaRemoteEvents*	iRemote;
	TBuf<50>			iNodeName;
	CCnvCharacterSetConverter* iCC;
	CSPyInterpreter* iInterpreter;
	CDesC8ArraySeg*	iFunctions;
	CDesC8ArraySeg*	iErrorFunctions;
	TBuf<256>			iErrorMsgBuf;
	CAcceptedMsgs*		iAcceptedMessages;
	TBuf<100>		msg;

	friend class CLocaLogic;
	friend class auto_ptr<CLocaLogicImpl>;
};

CLocaLogic* CLocaLogic::NewL(MApp_context& Context, RDbDatabase& aDb)
{
	CALLSTACKITEM_N(_CL("CLocaLogic"), _CL("NewL"));

	auto_ptr<CLocaLogicImpl> ret(new (ELeave) CLocaLogicImpl(Context, aDb));
	ret->ConstructL();
	return ret.release();
}

CLocaLogicImpl::CLocaLogicImpl(MApp_context& Context, RDbDatabase& aDb) :
	MContextBase(Context), iDb(aDb), iEvent(KLocaMsgStatus, KLocaMessageStatusTuple, 
		Context.BBDataFactory()) { }

void CLocaLogicImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CLocaLogicImpl"), _CL("ConstructL"));

	iDevStats=CDevStats::NewL(AppContext(), iDb);
	iMessageStats=CMessageStats::NewL(AppContext(), iDb);
	iRemote=CLocaRemoteEvents::NewL(AppContext(), this);
	iAcceptedMessages=CAcceptedMsgs::NewL(iDb);

	iBBSubSession=BBSession()->CreateSubSessionL(this);
	iBBSubSession->AddNotificationL(KLocaScriptTuple, ETrue);

	iEvent.iData.SetValue(&iMsgStatus);
	iEvent.iData.SetOwnsValue(EFalse);
	iEvent.iPriority()=CBBSensorEvent::VALUE;

	GetSettingsL();

	Settings().NotifyOnChange(SETTING_LOCA_BLUEJACK_MAX_MESSAGES, this);
	Settings().NotifyOnChange(SETTING_LOCA_BLUEJACK_MAX_RETRIES, this);
	Settings().NotifyOnChange(SETTING_PUBLISH_AUTHOR, this);

	iInterpreter = CSPyInterpreter::NewInterpreterL();
	iCC=CCnvCharacterSetConverter::NewL();
	iCC->PrepareToConvertToOrFromL(KCharacterSetIdentifierIso88591, Fs());
	iFunctions=new (ELeave) CDesC8ArraySeg(4);
	iErrorFunctions=new (ELeave) CDesC8ArraySeg(4);
}

_LIT(KErr, "err");

void AppendPyUnicode(CBBString* aInto, PyObject* aObject)
{
	if (aObject) {
		python_ptr<PyObject> str(PyObject_Unicode(aObject));
		if (PyUnicode_Check(str.get())) {
			int bytes=PyUnicode_GET_DATA_SIZE(str.get());
			aInto->Append( TPtrC( 
				(const TUint16*)PyUnicode_AS_UNICODE(str.get()), bytes/2) );
		} else {
			aInto->Append(_L("[could not convert to unicode]"));
		}
	} else {
		aInto->Append(_L("None"));
	}
}

void CLocaLogicImpl::ReportPythonError(const TDesC& aPrefix)
{
	PyObject *ptype=0, *pvalue=0, *ptraceback=0;
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
	python_ptr<PyObject> p1(ptype), p2(pvalue), p3(ptraceback);

	auto_ptr<CBBString> s(CBBString::NewL(KErr));
	s->Append(aPrefix);
	s->Append(_L("\nException type: "));
	AppendPyUnicode(s.get(), ptype);
	s->Append(_L("\nValue: "));
	AppendPyUnicode(s.get(), pvalue);
	s->Append(_L("\nBacktrace: "));
	AppendPyUnicode(s.get(), ptraceback);
	Reporting().UserErrorLog(s->Value());
}

void CLocaLogicImpl::NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
	const TComponentName& aComponentName, const MBBData* aData)
{
	if (aName==KLocaScriptTuple) {
		CC_TRAPD(err, NewScriptL(aSubName, aData));
		if (err!=KErrNone) {
			TBuf<70> rest=aSubName;
			rest.Append(_L(" err: "));
			rest.AppendNum(err);
			Reporting().UserErrorLog(_L("Failed to handle change to script "),
				rest);
		}
	}
}

void AddToArrayL(CDesC8Array* aArray, const TDesC8& aString)
{
	TInt ignored;
	if (aArray->Find(aString, ignored)!=KErrNone) {
		aArray->AppendL(aString);
	}
}
TBool IsInArray(CDesC8Array* aArray, const TDesC8& aString)
{
	TInt ignored;
	if (aArray->Find(aString, ignored)!=KErrNone) {
		return EFalse;
	}
	return ETrue;
}
void DeleteFromArrayL(CDesC8Array* aArray, const TDesC8& aString)
{
	TInt pos;
	if (aArray->Find(aString, pos)==KErrNone) {
		if (pos!=aArray->Count()-1) {
			aArray->InsertL(pos, 
				aArray->MdcaPoint(aArray->Count()-1));
		}
		aArray->Delete(aArray->Count()-1);
	}
}

void CLocaLogicImpl::NewScriptL(const TDesC& aSubName, const MBBData* aData)
{
	TInterpreterAutoLock interp;

	const CBBString* code=bb_cast<CBBString>(aData);	
	if (!code) {
		Reporting().UserErrorLog(_L("Could not read Python script named "),
			aSubName, _L(" (wrong bb type)"));
		return;
	}
	auto_ptr<HBufC8> code8(HBufC8::NewL(code->Value().Length()+10));
	auto_ptr<HBufC8> subname8(HBufC8::NewL(aSubName.Length()+10));
	{
		TInt err=KErrNone;
		TInt unconverted=0, unconverted_pos=0;
		{
			TPtr8 p=code8->Des();
			err=iCC->ConvertFromUnicode(p, code->Value(), unconverted, unconverted_pos);
		}
		if (err==KErrNone && unconverted==0) {
			TPtr8 p=subname8->Des();
			err=iCC->ConvertFromUnicode(p, aSubName, unconverted, unconverted_pos);
		}
		if (err!=KErrNone || unconverted>0) {
			TBuf<30> rest;
			if (err!=KErrNone) {
				rest=_L("to Latin1, err: ");
				rest.AppendNum(err);
			} else {
				rest=_L("to Latin1, at position ");
				rest.AppendNum(unconverted_pos);
			}
			Reporting().UserErrorLog(_L("Could not convert Python script named"),
				aSubName, rest);
			return;
		}
	}

	if ( code8.get() && code8->Length()>1 && (*code8)[code8->Length()-1] != '\n' ) {
		code8->Des().Append('\n');
	}
	code8->Des().Append('\0');
	TInt res=PyRun_SimpleString( (char*)code8->Ptr() );
	if (res < 0) {
		msg=_L("Cannot compile code in script ");
		msg.Append(aSubName);
		ReportPythonError(msg);
		return;
	}
	if (aSubName.Length()>0 && aSubName[0]!='_') {
		// borrowed
		PyObject* main_module = PyImport_AddModule("__main__");
		if (! main_module ) User::Leave(KErrNoMemory);
		// borrowed
		PyObject* global_dict = PyModule_GetDict(main_module);
		if (! global_dict ) User::Leave(KErrNoMemory);
		// borrowed
		subname8->Des().Append('\0');
		PyObject* func=PyDict_GetItemString(global_dict, (const char*)subname8->Ptr());
		if (func==0) {
			Reporting().UserErrorLog(_L("Running Python script "),
				aSubName, _L(" didn't create a function with that name"));
		} else {
			AddToArrayL(iFunctions, *subname8);
			DeleteFromArrayL(iErrorFunctions, *subname8);
		}
	}
}

void CLocaLogicImpl::DeletedL(const TTupleName& aName, const TDesC& aSubName)
{
	if (aName==KLocaScriptTuple) {
		if (aSubName.Length()>0 && aSubName[0]!='_') {
			TInterpreterAutoLock interpr;
			// borrowed
			PyObject* main_module = PyImport_AddModule("__main__");
			if (! main_module ) return;
			// borrowed
			PyObject* global_dict = PyModule_GetDict(main_module);
			if (! global_dict ) return;

			auto_ptr<HBufC8> subname8(HBufC8::NewL(aSubName.Length()+10));
			TPtr8 p=subname8->Des();
			TInt err=KErrNone;
			TInt unconverted=0, unconverted_pos=0;
			err=iCC->ConvertFromUnicode(p, aSubName, unconverted, unconverted_pos);
			if (err!=KErrNone || unconverted!=0) return;
			subname8->Des().Append('\0');

			PyDict_DelItemString(global_dict, (char*)subname8->Ptr());
			TInt pos;
			DeleteFromArrayL(iFunctions, *subname8);
			DeleteFromArrayL(iErrorFunctions, *subname8);
		}
	}
}

void CLocaLogicImpl::SettingChanged(TInt )
{
	GetSettingsL();
}

void CLocaLogicImpl::GetSettingsL()
{
	Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, iNodeName);

	iMaxFailures=100;
	iMaxSuccesses=30;
	Settings().GetSettingL(SETTING_LOCA_BLUEJACK_MAX_MESSAGES, iMaxSuccesses);
	Settings().GetSettingL(SETTING_LOCA_BLUEJACK_MAX_RETRIES, iMaxFailures);

	if (iMaxFailures < 0) iMaxFailures=0;
	if (iMaxSuccesses < 0) iMaxSuccesses=0;
	if (iMaxSuccesses>30) iMaxSuccesses=30;
}

CLocaLogicImpl::~CLocaLogicImpl()
{
	CALLSTACKITEM_N(_CL("CLocaLogicImpl"), _CL("~CLocaLogicImpl"));

	Settings().CancelNotifyOnChange(SETTING_LOCA_BLUEJACK_MAX_MESSAGES, this);
	Settings().CancelNotifyOnChange(SETTING_LOCA_BLUEJACK_MAX_RETRIES, this);
	Settings().CancelNotifyOnChange(SETTING_PUBLISH_AUTHOR, this);

	delete iBBSubSession;
	delete iDevStats;
	delete iMessageStats;
	delete iAcceptedMessages;
	delete iRemote;
	delete iCC;
	delete iInterpreter;
	delete iFunctions;
	delete iErrorFunctions;
}

void AppendTimeDifference(TDes8& aInto, TTime from, TTime now)
{
	CALLSTACKITEM_N(_CL("CLocaLogicImpl"), _CL("~CLocaLogicImpl"));


	if (from > now) {
		TTime s=from;
		from=now;
		now=s;
	}
	for(;;) {
		TTimeIntervalMinutes mins;
		if (now.MinutesFrom(from, mins)!=KErrNone) {
			mins=61;
		} 
		TTimeIntervalHours hours;
		if (now.HoursFrom(from, hours)!=KErrNone) {
			hours=25;
		} 
		TTimeIntervalDays days;
		days=now.DaysFrom(from);
		if (mins < TTimeIntervalMinutes(60)) {
			aInto.AppendNum(mins.Int());
			aInto.Append(_L8(" minutes "));
			break;
		} else if ( hours < TTimeIntervalDays(24)) {
			aInto.AppendNum(hours.Int());
			aInto.Append(_L8(" hours "));
			from+=TTimeIntervalHours(hours);
		} else {
			aInto.AppendNum(days.Int());
			aInto.Append(_L8(" days "));
			from+=TTimeIntervalDays(days);
		}
	}
}

void CLocaLogicImpl::UpdateStats(const TDesC& aNodeName,
	const CBBBtDeviceList* devices, const TTime& aAtTime)
{
	const TBBBtDeviceInfo* info=0;
	for (info=devices->First(); info; info=devices->Next()) {
		if (info->iMajorClass()==0x02 ||
			info->iMajorClass()==0x01) {
		} else {
			continue;
		}
		TDevStats s;
		iDevStats->GetStatsL(*info, aNodeName, s);
		if (aAtTime > s.iLastSeen ) {
			TInt span=0;
			if (s.iFirstSeen==TTime(0)) s.iFirstSeen=aAtTime;
			if (s.iLastSeen + TTimeIntervalMinutes(3) < aAtTime) {
				s.iPreviousVisitBegin=s.iVisitBegin;
				s.iPreviousVisitEnd=s.iLastSeen;
				s.iVisitBegin=aAtTime;
				s.iCountStay++;
				span=1;
			} else {
				TTimeIntervalMinutes prev_span;
				s.iLastSeen.MinutesFrom(s.iVisitBegin, prev_span);
				if (prev_span.Int()==0) prev_span=1;
				else prev_span=TTimeIntervalMinutes(prev_span.Int()+1);
				s.iSumStay-=prev_span.Int();
				s.iSquareSumStay-=(prev_span.Int()*prev_span.Int());
				TTimeIntervalMinutes this_span;
				aAtTime.MinutesFrom(s.iVisitBegin, this_span);
				span=this_span.Int() + 1;
			}
			s.iLastSeen=aAtTime;
			s.iSumStay+=span;
			s.iSquareSumStay+=span*span;
			if (span > s.iMaxStay) s.iMaxStay=span;
			iDevStats->SetStatsL(*info, aNodeName, s);
		}
	}
}

void AddToPyDict(python_ptr<PyObject> &dict, char* name, python_ptr<PyObject> value)
{
	if (! value.get()) User::LeaveNoMemory();
	if (PyDict_SetItemString(dict.get(), name, value.get())==0) {
		value.release();
	} else {
		User::LeaveNoMemory();
	}
}

void AddToPyDict(python_ptr<PyObject> &dict, char* name, const TDesC& value)
{
	python_ptr<PyObject> val(Py_BuildValue("u#", value.Ptr(), value.Length()));
	AddToPyDict(dict, name, val);
}
void AddToPyDict(python_ptr<PyObject> &dict, char* name, TInt value)
{
	python_ptr<PyObject> val(PyLong_FromLong(value));
	AddToPyDict(dict, name, val);
}
void AddToPyDict(python_ptr<PyObject> &dict, char* name, double value)
{
	python_ptr<PyObject> val(PyFloat_FromDouble(value));
	AddToPyDict(dict, name, val);
}

void AddToPyDict(python_ptr<PyObject> &dict, char* name, const TTime& value)
{
	TDateTime epoch; epoch.Set(1970, EJanuary, 0, 0, 0, 0, 0);
	TTime e(epoch);
	TInt unixtime=0;
	TTimeIntervalSeconds secs;
	if (value!=TTime(0)) {
		User::LeaveIfError(value.SecondsFrom(e, secs));
		unixtime=secs.Int();
	}
	AddToPyDict(dict, name, unixtime);
}

void CLocaLogicImpl::GetMessage(const CBBBtDeviceList* devices,
	const TTime& aAtTime,
	TInt& doSendToIndex, 
	TInt& aMessageCode, TDes& aWithName,
	TDes& aWithTitle, auto_ptr<HBufC8>& aBody) 
{
	CC_TRAPD(err, GetMessageL(devices, aAtTime, doSendToIndex,
		aMessageCode, aWithName, aWithTitle, aBody));
}

PyObject* DevStatToPyDict(TDevStats& s)
{
	python_ptr<PyObject> dict_dev(PyDict_New());
	if (!dict_dev.get()) User::LeaveNoMemory();
	AddToPyDict(dict_dev, "last_seen", s.iLastSeen);
	AddToPyDict(dict_dev, "visitbegin", s.iVisitBegin);
	AddToPyDict(dict_dev, "prev_visitbegin", s.iPreviousVisitBegin);
	AddToPyDict(dict_dev, "prev_visitend", s.iPreviousVisitEnd);
	AddToPyDict(dict_dev, "count", s.iCountStay);
	AddToPyDict(dict_dev, "avg", s.iSumStay/s.iCountStay);
	double avg= (double)s.iSumStay/(double)s.iCountStay;
	AddToPyDict(dict_dev, "var",
		(double)s.iSquareSumStay / (double)s.iCountStay -
		avg*avg );
	AddToPyDict(dict_dev, "total", s.iSumStay);
	AddToPyDict(dict_dev, "max", s.iMaxStay);
	AddToPyDict(dict_dev, "first_seen", s.iFirstSeen);
	return dict_dev.release();
}
PyObject* MessageStatToPyDict(TMessageStats& s)
{
	python_ptr<PyObject> dict_msgstats(PyDict_New());
	if (!dict_msgstats.get()) User::LeaveNoMemory();
	AddToPyDict(dict_msgstats, "failurecount", s.iFailureCount);
	AddToPyDict(dict_msgstats, "local_failurecount", s.iLocalFailureCount);
	AddToPyDict(dict_msgstats, "successcount", s.iSuccessCount);
	AddToPyDict(dict_msgstats, "prev_local_success", s.iPreviousLocalSuccess);
	AddToPyDict(dict_msgstats, "prev_remote_success", s.iPreviousRemoteSuccess);
	AddToPyDict(dict_msgstats, "prev_local_failure", s.iPreviousLocalFailure);
	AddToPyDict(dict_msgstats, "prev_remote_failure", s.iPreviousRemoteFailure);
	return dict_msgstats.release();
}

void CLocaLogicImpl::ConvertFromPythonString(TDes& aBuf, PyObject* aString)
{
	aBuf.Zero();
	if (PyString_Check(aString)) {
		TInt state=CCnvCharacterSetConverter::KStateDefault;
		TInt len=PyString_Size(aString);
		if (len > aBuf.MaxLength()) len=aBuf.MaxLength();
		iCC->ConvertToUnicode(aBuf, TPtrC8((TUint8*)PyString_AsString(aString), 
			len), state);
	} else if (PyUnicode_Check(aString)) {
		TInt len=PyUnicode_GetSize(aString)/2;
		if (len > aBuf.MaxLength()) len=aBuf.MaxLength();
		aBuf=TPtrC((TUint16*)PyUnicode_AsUnicode(aString), len);
	}
}

TBool IsPythonString(PyObject* aString)
{
	if (PyString_Check(aString)) {
		return PyString_Size(aString)>0;
	} else if (PyUnicode_Check(aString)) {
		return PyUnicode_GetSize(aString)>0;
	}
	PyErr_Clear();
	return EFalse;
}

void CLocaLogicImpl::ConvertFromPythonString(auto_ptr<HBufC8>& aBuf, PyObject* aString)
{
	if (PyString_Check(aString)) {
		TInt len=PyString_Size(aString);
		if (! aBuf.get() || aBuf->Des().MaxLength() < len) {
			aBuf.reset(HBufC8::NewL(len*2));
		}
		aBuf->Des()=TPtrC8((TUint8*)PyString_AsString(aString), len);
	} else if (PyUnicode_Check(aString)) {
		TInt len=PyUnicode_GetSize(aString);
		if (! aBuf.get() || aBuf->Des().MaxLength() < len) {
			aBuf.reset(HBufC8::NewL(len*2));
		}
		TPtr8 p=aBuf->Des();
		iCC->ConvertFromUnicode(p, TPtrC((TUint16*)PyUnicode_AsUnicode(aString), len));
	}
	PyErr_Clear();
}

CLocaLogicImpl::TScriptError CLocaLogicImpl::RunScript(
	PyObject* main_module, PyObject* global_dict,
	const TDesC8& aName, TInt& max_seen_priority,
	PyObject* dict_general, PyObject* dict_devstats, 
	PyObject* dict_msgstats,
	TInt& aMessageCode, TDes& aWithName,
	TDes& aWithTitle, auto_ptr<HBufC8>& aBody,
	TDes& aErrorMessage, const TDesC8& aMac)
{
	PyObject* func=PyDict_GetItemString(global_dict, 
		(const char*)aName.Ptr());
	if (!func) return EIgnored;

	python_ptr<PyObject> res(PyObject_CallFunction(func, "(OOO)",
		dict_general, dict_devstats, dict_msgstats));
	if (! res.get() ) {
		aErrorMessage=_L("Call to function failed ");
		return EPythonError;
	} else {
		if (! PyTuple_Check(res.get()) ) {
			aErrorMessage=_L("Return from Python script is not a tuple");
			return EOtherError;
		} else {
			TInt priority=-1;
			PyObject *with_name, *with_title, *with_body;
			{
				PyObject *priority_num;
				priority_num=PyTuple_GetItem(res.get(), 0);
				python_ptr<PyObject> priority_int(
					PyNumber_Int(priority_num));
				if (priority_int.get()) {
					priority=PyInt_AsLong(priority_int.get());
				} else {
					aErrorMessage=_L("First return from Python script could not be converted to an integer. ");
					return EOtherError;
				}
				if (priority<=0) return EIgnored;
				if (priority<max_seen_priority) return EIgnored;
			}
			{
				PyObject* msgcode_num=PyTuple_GetItem(res.get(), 1);
				python_ptr<PyObject> msgcode_int(
					PyNumber_Int(msgcode_num));
				if (msgcode_int.get()) {
					TInt code=PyInt_AsLong(msgcode_int.get());
					if (iAcceptedMessages->HasBeenAcceptedL(aMac, code)) {
						return EIgnored;
					}
					aMessageCode=PyInt_AsLong(msgcode_int.get());
				} else {
					aErrorMessage=_L("Second return from Python script could not be converted to an integer. ");
					return EOtherError;
				}
			}
			with_name=PyTuple_GetItem(res.get(), 2);
			with_title=PyTuple_GetItem(res.get(), 3);
			with_body=PyTuple_GetItem(res.get(), 4);
			if (!with_name || !with_title || !with_body) {
				aErrorMessage=_L("Python script returned wrong number of values");
				return EOtherError;
			} else {
				if (!IsPythonString(with_name)) {
					aErrorMessage=_L("Third return (name) from Python script could not be converted to a string");
					return EOtherError;
				}
				if (!IsPythonString(with_title)) {
					aErrorMessage=_L("Fourth return (title) from Python script could not be converted to a string");
					return EOtherError;
				}
				if (!IsPythonString(with_body)) {
					aErrorMessage=_L("Fifth return (body) from Python script could not be converted to a string");
					return EOtherError;
				}
				ConvertFromPythonString(aWithName, with_name);
				ConvertFromPythonString(aWithTitle, with_title);
				ConvertFromPythonString(aBody, with_body);
				max_seen_priority=priority;
				return EHighestPriority;
			}
		}
	}
}
void CLocaLogicImpl::GetMessageL(const CBBBtDeviceList* devices,
	const TTime& aAtTime,
	TInt& doSendToIndex, 
	TInt& aMessageCode, TDes& aWithName,
	TDes& aWithTitle, auto_ptr<HBufC8>& aBody) 
{
	CALLSTACKITEM_N(_CL("CLocaLogicImpl"), _CL("GetMessage"));
	TInterpreterAutoLock interpr;

	doSendToIndex=-1;

	TInt count=-1;
	const TBBBtDeviceInfo* dev=0;

	TInt max_seen_priority=-1;

	for (dev=devices->First(); dev; dev=devices->Next()) count++;
	
	python_ptr<PyObject> dict_general(PyDict_New());
	if (! dict_general ) User::Leave(KErrNoMemory);
	AddToPyDict(dict_general, "nodename", iNodeName);
	AddToPyDict(dict_general, "time", aAtTime);
	AddToPyDict(dict_general, "bt_count", count);

	// borrowed
	PyObject* main_module = PyImport_AddModule("__main__");
	if (! main_module ) User::Leave(KErrNoMemory);
	// borrowed
	PyObject* global_dict = PyModule_GetDict(main_module);
	if (! global_dict ) User::Leave(KErrNoMemory);
	// borrowed

	TInt index=0;
	dev=devices->First(); // first is the node itself
	for (dev=devices->Next(); dev; dev=devices->Next()) {
		index++;
		TBuf<15> mac;
		dev->iMAC.IntoStringL(mac);
		AddToPyDict(dict_general, "mac", mac);
		AddToPyDict(dict_general, "majorclass", dev->iMajorClass());
		AddToPyDict(dict_general, "minorclass", dev->iMinorClass());

		TBool found=EFalse;
		python_ptr<PyObject> dict_devstats(PyDict_New());
		{
			TBuf<50> node;
			TDevStats s;
			for(found=iDevStats->FirstStats(*dev, node, s); found; 
					found=iDevStats->NextStats(*dev, node, s)) {
				python_ptr<PyObject> dict_dev(
					DevStatToPyDict(s));
				auto_ptr<HBufC8> node8(HBufC8::NewL(node.Length()+2));
				TPtr8 p=node8->Des();
				iCC->ConvertFromUnicode(p, node);
				node8->Des().ZeroTerminate();
				AddToPyDict(dict_devstats, (char*)node8->Ptr(), dict_dev);
			}
		}
		python_ptr<PyObject> dict_msgstats(0);
		{
			TMessageStats s;
			iMessageStats->GetStatsL(*dev, s);
			dict_msgstats.reset(MessageStatToPyDict(s));
		}
		for (int i=0; i<iFunctions->Count(); i++) {
			TScriptError err=RunScript(main_module,
				global_dict, iFunctions->MdcaPoint(i), max_seen_priority,
				dict_general.get(), dict_devstats.get(), 
				dict_msgstats.get(),
				aMessageCode, aWithName,
				aWithTitle, aBody, iErrorMsgBuf, dev->iMAC()
				);
			if (err>EHighestPriority) {
#if !defined(__WINS__) && !defined(CONTEXTLOCA)
				if (IsInArray(iErrorFunctions, iFunctions->MdcaPoint(i))) {
					PyErr_Clear();
					continue;
				}
#endif
				AddToArrayL(iErrorFunctions, iFunctions->MdcaPoint(i));

				TBuf<50> name;
				TInt state=CCnvCharacterSetConverter::KStateDefault;
				iCC->ConvertToUnicode(name, iFunctions->MdcaPoint(i), state);
				name.SetLength(name.Length()-1);
				if (err==EPythonError) {
					AppendCheckingSpaceL(iErrorMsgBuf, _L(" in script "));
					AppendCheckingSpaceL(iErrorMsgBuf, name);
					ReportPythonError(iErrorMsgBuf);
				} else {
					Reporting().UserErrorLog( iErrorMsgBuf,
						_L(" in script "), name);
				}
			} else if (err==EHighestPriority) {
				doSendToIndex=index;
			}
		}
	}
	/*
	// phone or PDA
	if (aDevice.iMajorClass()==0x02 || (
		aDevice.iMajorClass()==0x01 && 
		aDevice.iMinorClass()>0x03 )) {
	} else {
		return;
	}
	*/

}

void CLocaLogicImpl::Failed(const TBBBtDeviceInfo& aDevice,
	TInt aMessageCode,
	const TTime& aAtTime,
	TSendFailure aErrorCode,
	TBool	aLocal) 
{
	CALLSTACKITEM_N(_CL("CLocaLogicImpl"), _CL("Failed"));

	TMessageStats s;
	iMessageStats->GetStatsL(aDevice, s);
	s.iFailureCount++;
	if (aLocal) {
		s.iPreviousLocalFailure=aAtTime;
		s.iLocalFailureCount++;;
	} else {
		s.iPreviousRemoteFailure=aAtTime;
	}

	iMessageStats->SetStatsL(aDevice, s);

	if (aLocal) {
#ifdef __WINS__
		msg=_L("localogic: failed to send message ");
		msg.AppendNum(aErrorCode);
		Reporting().DebugLog(msg);
#endif

		iMsgStatus.iMessageId()=aMessageCode;
		iMsgStatus.iRecipientAddress()=aDevice.iMAC();
		iMsgStatus.iSucceeded()=EFalse;
		iMsgStatus.iAtTime()=aAtTime;
		iEvent.iStamp()=aAtTime;

		TTime expires=GetTime(); expires+=TTimeIntervalDays(5);
		iBBSubSession->PutRequestL(KLocaMessageStatusTuple, KNullDesC,
			&iEvent, expires, KNoComponent);
	}
}

void CLocaLogicImpl::Success(const TBBBtDeviceInfo& aDevice,
	TInt aMessageCode,
	const TTime& aAtTime,
	TBool aLocal)
{
	CALLSTACKITEM_N(_CL("CLocaLogicImpl"), _CL("Success"));

	iAcceptedMessages->SetAsAcceptedL(aDevice.iMAC(), aMessageCode);
	TMessageStats s;
	iMessageStats->GetStatsL(aDevice, s);
	s.iSuccessCount++;
	if (aLocal) {
		//s.iPreviousSeen=aAtTime;
		s.iPreviousLocalSuccess=aAtTime;
		//s.iLocalFailureCount=0;
	} else {
		s.iPreviousRemoteSuccess=aAtTime;
	}
	//s.iFailureCount=0;

	iMessageStats->SetStatsL(aDevice, s);

	if (aLocal) {
		//Reporting().DebugLog(_L("localogic: sent message"));

		iMsgStatus.iMessageId()=aMessageCode;
		iMsgStatus.iRecipientAddress()=aDevice.iMAC();
		iMsgStatus.iSucceeded()=ETrue;
		iMsgStatus.iAtTime()=aAtTime;
		iEvent.iStamp()=aAtTime;
		TTime expires=GetTime(); expires+=TTimeIntervalDays(5);
		iBBSubSession->PutRequestL(KLocaMessageStatusTuple, KNullDesC,
			&iEvent, expires, KNoComponent);
	}
}


CDevStats* CDevStats::NewL(MApp_context& aContext, RDbDatabase& aDb)
{
	CALLSTACKITEM2_N(_CL("CDevStats"), _CL("NewL"), &aContext);

	auto_ptr<CDevStats> ret(new (ELeave) CDevStats(aContext, aDb));
	ret->ConstructL();
	return ret.release();
}

CMessageStats* CMessageStats::NewL(MApp_context& aContext, RDbDatabase& aDb)
{
	CALLSTACKITEM2_N(_CL("CMessageStats"), _CL("NewL"), &aContext);

	auto_ptr<CMessageStats> ret(new (ELeave) CMessageStats(aContext, aDb));
	ret->ConstructL();
	return ret.release();
}

CBtStats::CBtStats(MApp_context& aContext, RDbDatabase& aDb) : 
	MContextBase(aContext), MDBStore(aDb) { }

TTime CBtStats::GetColTime(TInt aColumn)
{
	if (iTable.IsColNull(aColumn) ) {
		return TTime(0);
	} else {
		return iTable.ColTime(aColumn);
	}
}

TInt CBtStats::GetColInt(TInt aColumn)
{
	if (iTable.IsColNull(aColumn) ) {
		return TInt(0);
	} else {
		return iTable.ColInt(aColumn);
	}
}

TBool CDevStats::FirstStats(const TBBBtDeviceInfo& aDevice, TDes& aNode, TDevStats& aStats)
{
	TDbSeekKey k(aDevice.iMAC());
	if (! iTable.SeekL(k)) return EFalse;
	iTable.GetL();
	iNodes->GetNodeNameL(iTable.ColUint32(ENode), aNode);
	GetCurrentStatsL(aStats);
	return ETrue;
}

TBool CDevStats::NextStats(const TBBBtDeviceInfo& aDevice, TDes& aNode, TDevStats& aStats)
{
	if (!iTable.NextL()) return EFalse;
	iTable.GetL();
	TBuf8<6> mac=iTable.ColDes8(EDevAddr);
	if (mac.Compare(aDevice.iMAC())!=0) return EFalse;
	iNodes->GetNodeNameL(iTable.ColUint32(ENode), aNode);
	GetCurrentStatsL(aStats);
	return ETrue;
}

void CDevStats::GetCurrentStatsL(TDevStats& aStats)
{
	aStats.iLastSeen=GetColTime(ELastSeen);
	aStats.iVisitBegin=GetColTime(EVisitBegin);
	aStats.iPreviousVisitBegin=GetColTime(EPreviousVisitBegin);
	aStats.iPreviousVisitEnd=GetColTime(EPreviousVisitEnd);
	aStats.iFirstSeen=GetColTime(EFirstSeen);
	aStats.iSumStay=GetColInt(ESumStay);
	aStats.iSquareSumStay=GetColInt(ESquareSumStay);
	aStats.iCountStay=GetColInt(ECountStay);
	aStats.iMaxStay=GetColInt(EMaxStay);
}

void CDevStats::GetStatsL(const TBBBtDeviceInfo& aDevice,
						  const TDesC& aNodeName,
		TDevStats& aStats)
{
	CALLSTACKITEM_N(_CL("CDevStats"), _CL("GetStatsL"));


	if (SeekToDevL(aDevice, aNodeName)) {
		iTable.GetL();

		GetCurrentStatsL(aStats);
	} else {
		Mem::FillZ( &aStats, sizeof(TDevStats) );
	}
}

void CMessageStats::GetStatsL(const TBBBtDeviceInfo& aDevice,
		TMessageStats& aStats)
{
	CALLSTACKITEM_N(_CL("CDevStats"), _CL("GetStatsL"));


	if (SeekToDevL(aDevice)) {
		iTable.GetL();

		aStats.iFailureCount=iTable.ColInt(EFailureCount);
		aStats.iSuccessCount=iTable.ColInt(ESuccessCount);
		aStats.iPreviousLocalSuccess=GetColTime(EPreviousLocalSuccess);
		aStats.iPreviousRemoteSuccess=GetColTime(EPreviousRemoteSuccess);
		aStats.iPreviousLocalFailure=GetColTime(EPreviousLocalFailure);
		aStats.iPreviousRemoteFailure=GetColTime(EPreviousRemoteFailure);
		aStats.iLocalFailureCount=GetColInt(ELocalFailureCount);
		aStats.iLastSeen=GetColTime(ELastSeen);

	} else {
		Mem::FillZ( &aStats, sizeof(TDevStats) );
	}
}

void CDevStats::SetStatsL(const TBBBtDeviceInfo& aDevice,
						  const TDesC& aNodeName,
		const TDevStats& aStats)
{
	CALLSTACKITEM_N(_CL("CDevStats"), _CL("SetStatsL"));

	if (SeekToDevL(aDevice, aNodeName)) {
		iTable.UpdateL();
	} else {
		iCount++;
		iTable.InsertL();
		iTable.SetColL(EDevAddr, aDevice.iMAC());
		iTable.SetColL(ENode, iNodes->GetNodeIdL(aNodeName));
	}

	iTable.SetColL(ELastSeen, aStats.iLastSeen);
	iTable.SetColL(EVisitBegin, aStats.iVisitBegin);
	iTable.SetColL(EPreviousVisitBegin, aStats.iPreviousVisitBegin);
	iTable.SetColL(EPreviousVisitEnd, aStats.iPreviousVisitEnd);
	iTable.SetColL(ECountStay, aStats.iCountStay);
	iTable.SetColL(ESumStay, aStats.iSumStay);
	iTable.SetColL(ESquareSumStay, aStats.iSquareSumStay);
	iTable.SetColL(EMaxStay, aStats.iMaxStay);
	iTable.SetColL(EFirstSeen, aStats.iFirstSeen);

	PutL();
	if (iCount > MAX_STATS + STATS_HYSTERESIS) {
		SwitchIndexL(EIdxLastSeen);
		iTable.FirstL();
		while (iCount > MAX_STATS - STATS_HYSTERESIS) {
			DeleteL();
			iCount--;
			iTable.NextL();
		}
	} else if ( AppContext().NoSpaceLeft() ) {
		for (int i=0; i<STATS_HYSTERESIS && iCount>0; i++) {
			DeleteL();
			iCount--;
			iTable.NextL();
		}
	}
}

void CMessageStats::SetStatsL(const TBBBtDeviceInfo& aDevice,
		const TMessageStats& aStats)
{
	CALLSTACKITEM_N(_CL("CDevStats"), _CL("SetStatsL"));


	if (SeekToDevL(aDevice)) {
		iTable.UpdateL();
	} else {
		iTable.InsertL();
		iTable.SetColL(EDevAddr, aDevice.iMAC());
		iCount++;
	}

	iTable.SetColL(EFailureCount, aStats.iFailureCount);
	iTable.SetColL(ESuccessCount, aStats.iSuccessCount);
	iTable.SetColL(EPreviousLocalSuccess, aStats.iPreviousLocalSuccess);
	iTable.SetColL(EPreviousRemoteSuccess, aStats.iPreviousRemoteSuccess);
	iTable.SetColL(EPreviousLocalFailure, aStats.iPreviousLocalFailure);
	iTable.SetColL(EPreviousRemoteFailure, aStats.iPreviousRemoteFailure);
	iTable.SetColL(ELocalFailureCount, aStats.iLocalFailureCount);
	iTable.SetColL(ELastSeen, aStats.iLastSeen);

	PutL();

	if (iCount > MAX_STATS + STATS_HYSTERESIS) {
		SwitchIndexL(EIdxLastSeen);
		iTable.FirstL();
		while (iCount > MAX_STATS - STATS_HYSTERESIS) {
			DeleteL();
			iCount--;
			iTable.NextL();
		}
	} else if ( AppContext().NoSpaceLeft() ) {
		SwitchIndexL(EIdxLastSeen);
		iTable.FirstL();
		for (int i=0; i<STATS_HYSTERESIS && iCount>0; i++) {
			DeleteL();
			iCount--;
			iTable.NextL();
		}
	}

}

TBool	CMessageStats::SeekToDevL(const TBBBtDeviceInfo& aDevice)
{
	CALLSTACKITEM_N(_CL("CDevStats"), _CL("SeekToDevL"));


	TDbSeekKey k(aDevice.iMAC());
	return iTable.SeekL(k);
}

TBool	CDevStats::SeekToDevL(const TBBBtDeviceInfo& aDevice, const TDesC& aNode)
{
	CALLSTACKITEM_N(_CL("CDevStats"), _CL("SeekToDevL"));

	SwitchIndexL(EIdxDev);

	TDbSeekMultiKey<2> k;
	TUint id=iNodes->GetNodeIdL(aNode);
	k.Add(aDevice.iMAC());
	k.Add(id);
	return iTable.SeekL(k);
}

CDevStats::CDevStats(MApp_context& aContext, RDbDatabase& aDb) : 
	CBtStats(aContext, aDb) { }

CMessageStats::CMessageStats(MApp_context& aContext, RDbDatabase& aDb) : 
	CBtStats(aContext, aDb) { }

void CDevStats::ConstructL()
{
	CALLSTACKITEM_N(_CL("CDevStats"), _CL("ConstructL"));

	TInt cols[]= { 
		EDbColText8, /* devaddr */
		EDbColUint32, /* node */
		EDbColDateTime, /* last seen */
		EDbColDateTime, EDbColDateTime,  /* last seen continuousl, begin */
		EDbColDateTime, /* previous seen */
		EDbColInt32, /* sum stay */
		EDbColInt32, /* square-sum stay */
		EDbColInt32, /* count stay */
		EDbColInt32, /* max stay */
		EDbColDateTime, /* first seen */
		-1 };
	TInt idxs[]= { EDevAddr, ENode, -2, ELastSeen, -1 };
	MDBStore::SetTextLen(6);
	MDBStore::ConstructL(cols, idxs, true, _L("DEVSTATS"), ETrue);

	iNodes=new (ELeave) CNodeTable(iDb);
	iNodes->ConstructL();

	iCount=iTable.CountL();
}

void CMessageStats::ConstructL()
{
	CALLSTACKITEM_N(_CL("CDevStats"), _CL("ConstructL"));

	TInt cols[]= { EDbColText8, EDbColInt32, EDbColInt32, 
		EDbColDateTime, EDbColDateTime, EDbColDateTime, EDbColDateTime,
		EDbColInt32, EDbColDateTime,
		-1 };
	TInt idxs[]= { EDevAddr, -2, ELastSeen, -1 };
	MDBStore::SetTextLen(6);
	MDBStore::ConstructL(cols, idxs, true, _L("MSGSTATS"), ETrue);

	iCount=iTable.CountL();
}
