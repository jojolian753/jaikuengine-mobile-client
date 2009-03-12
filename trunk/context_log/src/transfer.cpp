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
#include "transfer.h"
#include "contextlog_resource.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"
#include "local_defaults.h"
#include <bautils.h>
#include "file_output_base.h"
#include "app_context_impl.h"
#include "csd_battery.h"

#include "raii_f32file.h"

#define BASE_BUSY_WAIT	20

#if defined(__S60V2__)
#  include <etelmm.h>
#else
#  ifndef NO_ETELAGSM_H
#    include <etelagsm.h>
#  endif
#endif


#ifdef __WINS__
#define WAIT_PERIOD 60*60
#else
#define WAIT_PERIOD 60*60
#endif

#define UPLOAD_FROM 2
#define UPLOAD_TO 5

//const TTimeIntervalMinutes	KUploadInterval=5;
//const TTimeIntervalMinutes	KUploadIntervalForce=15;

void ToPacket(const TDesC& filen, TDes& packet)
{
	TInt dirpos=filen.LocateReverse('\\');
	packet=filen.Left(dirpos);
	packet.Append(_L("\\context"));
	packet.Append(filen.Mid(dirpos));
	TInt extpos=packet.LocateReverse('.');
	if (extpos!=KErrNotFound) {
		packet[extpos]='_';
	}
	packet.Append(_L(".xml"));
}

void ToDel(const TDesC& filen, TDes& del)
{
	TInt dirpos=filen.LocateReverse('\\');
	del=filen.Left(dirpos);
	del.Append(_L("\\context"));
	del.Append(filen.Mid(dirpos));

	TInt extpos=del.LocateReverse('.');
	if (extpos!=KErrNotFound) {
		del[extpos]='_';
	}
	del.Append(_L(".del"));
}


TDummyPrompt::TDummyPrompt(bool Delete) : iDelete(Delete), iUpload(true) { }

void TDummyPrompt::SetUpload(bool aUpload)
{
	iUpload=aUpload;
}

void TDummyPrompt::SetDelete(bool aDelete)
{
	iDelete=aDelete;
}


void TDummyPrompt::Prompt(const TDesC& /*FileName*/, MUploadCallBack* CallBack)
{
	CALLSTACKITEM_N(_CL("TDummyPrompt"), _CL("Prompt"));

	CallBack->Back(iUpload, iDelete, 0);
}


CTransferBase::CTransferBase(MApp_context& Context, MUploadPrompt& Prompt, bool move_to_mmc) : 
		MContextBase(Context), iPrompt(Prompt), move_to_memory_card(move_to_mmc)
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("CTransferBase"));

}

CSendUITransfer::CSendUITransfer(MApp_context& Context) : CTransferBase(Context, iDummyPrompt)
{
	CALLSTACKITEM_N(_CL("CSendUITransfer"), _CL("CSendUITransfer"));
}


void CTransferBase::ConstructL(i_status_notif* callback, const TDesC& dir1, const TDesC& dir2)
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("ConstructL"));

	send_dirs=new CDesC16ArrayFlat(5);
	send_leave=new CArrayFixFlat<bool>(5);

	TBool use_mmc=ETrue;
	Settings().GetSettingL(SETTING_USE_MMC, use_mmc);
	has_memory_card=false;
	TDriveInfo i;
	if (use_mmc && Fs().Drive(i, EDriveE)==KErrNone) {
		// memory card
		has_memory_card=true;
	}

	dir_prefixes=CList<TFileName>::NewL();
	TFileName prefix;
	if (dir1.Length()) {
		prefix=dir1;
		if (dir1.Right(1).Compare(_L("\\")) ) { prefix.Append(_L("\\")); }
		dir_prefixes->AppendL(prefix);
		if (has_memory_card) {
			prefix.Replace(0, 1, _L("e"));
			dir_prefixes->AppendL(prefix);
		}
	}

	if (dir2.Length()) {
		prefix=dir2;
		if (dir2.Right(1).Compare(_L("\\")) ) { prefix.Append(_L("\\")); }
		dir_prefixes->AppendL(prefix);
		if (has_memory_card) {
			prefix.Replace(0, 1, _L("e"));
			dir_prefixes->AppendL(prefix);
		}
	}

	CList<TFileName>::Node *n=dir_prefixes->iFirst;
	TFileName contextdir;
	while(n) {
		prefix=n->Item;
		if (prefix.Length()) {
			contextdir=prefix;
			contextdir.Append(_L("context\\"));
			TInt err;
			CC_TRAPIGNORE(err, KErrCorrupt, BaflUtils::EnsurePathExistsL(Fs(), contextdir));
			// ignore errors, since the drive might not exists, of course
			// there might be other errors which will cause a failure
			// in the packet/del files
		}
		n=n->Next;
	}

	cb=callback;
	iCallBack=CTimeOut::NewL(*this);
}

void CSendUITransfer::ConstructL(i_status_notif* callback, TInt cmdid, const TDesC& dir1, const TDesC& dir2)
{
	CALLSTACKITEM_N(_CL("CSendUITransfer"), _CL("ConstructL"));

	CTransferBase::ConstructL(callback, dir1, dir2);
#ifndef __S60V3__
	//FIXME
	sendui=CSendAppUi::NewL(cmdid, NULL);
#endif
}

CSendUITransfer* CSendUITransfer::NewL(MApp_context& Context, i_status_notif* callback, TInt cmdid, 
				       const TDesC& dir1, const TDesC& dir2)
{
	CALLSTACKITEM2_N(_CL("CSendUITransfer"), _CL("NewL"), &Context);

	auto_ptr<CSendUITransfer> ret(new (ELeave) CSendUITransfer(Context));
	ret->ConstructL(callback, cmdid, dir1, dir2);
	return ret.release();
}

CTransferBase::~CTransferBase()
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("~CTransferBase"));

	delete dir_prefixes;
	delete dir;
	delete iPacket8;
	delete send_dirs;
	delete send_leave;
	delete file_names; delete list_file_names;
	delete iCallBack;
}

CSendUITransfer::~CSendUITransfer()
{
	CALLSTACKITEM_N(_CL("CSendUITransfer"), _CL("~CSendUITransfer"));

#ifndef __S60V3__
	//FIXME
	delete sendui;
#endif
}

void CSendUITransfer::DisplayMenuL(CEikMenuPane& aMenuPane)
{
	CALLSTACKITEM_N(_CL("CSendUITransfer"), _CL("DisplayMenuL"));

#ifndef __S60V3__
	//FIXME
	sendui->DisplaySendCascadeMenuL(aMenuPane);
#endif
}

void CSendUITransfer::DisplaySendMenuL(CEikMenuPane& aMenuPane, TInt pos)
{
	CALLSTACKITEM_N(_CL("CSendUITransfer"), _CL("DisplaySendMenuL"));

#ifndef __S60V3__
	//TSendingCapabilities c(0, 100000, 0);
	TSendingCapabilities c(0, 0, 0);
	//FIXME
	sendui->DisplaySendMenuItemL(aMenuPane, pos, 
		c);
#endif
}

void CTransferBase::add_filesL(const TDesC& file, bool leave_last)
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("add_filesL"));

	CList<TFileName>::Node *n=dir_prefixes->iFirst;
	TFileName filen;
	while (n) {
		filen=n->Item;
		filen.Append(file);
		send_dirs->AppendL(filen);
		send_leave->AppendL(leave_last);
		n=n->Next;
	}

}

void CTransferBase::FileStep()
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("FileStep"));

	TEntry fe;
	TBuf<250> msg;

next_step:
	msg=_L("");
	while (dir_i >= dir_count) {
		if (send_dir_i >= send_dirs->Count()) {
			if (again) {
				again=false;
				busy_files=false;
				send_dir_i=0;
			} else {
				GotFiles();
				return;
			}
		}
		dirname=(*send_dirs)[send_dir_i];
		leave=(*send_leave)[send_dir_i];
		send_dir_i++;

		delete dir; dir=0;
		TInt err=Fs().GetDir(dirname, KEntryAttNormal, ESortByName, dir);
		if (err != KErrNone) {
			// msg.Format(_L("error %d getting dir %S"), err, &dirname);
			// cb->status_change(msg);
			goto next_step;
		}
		dir_count=dir->Count();
		if (leave) --dir_count;
		p.Set(dirname, 0, 0);
		dir_i=0;
	}

	filen=p.DriveAndPath();
	filen.Append((*dir)[dir_i].iName);
	TBool use_mmc=ETrue;
	if (move_to_memory_card && has_memory_card && filen.Left(1).CompareF(_L("c"))==0 && 
			Settings().GetSettingL(SETTING_USE_MMC, use_mmc) && use_mmc &&
			(*dir)[dir_i].iName.Left(12).Compare(_L("cellid_names")) ) {
		filen2=filen;
		filen2.Replace(0, 1, _L("e"));
		if (BaflUtils::CopyFile(Fs(), filen, filen2) == KErrNone) {
			BaflUtils::DeleteFile(Fs(), filen);
			filen=filen2;
		}
	}
	dir_i++;
	TInt err=Fs().Entry(filen, fe);
	if (err==KErrNone) {
		if (fe.iSize>0 && fe.IsArchive()) {
			TFileName packet; ToPacket(filen, packet);
			if (BaflUtils::FileExists(Fs(), packet) ) {
				if (file_names) {
					//msg=_L("in progress ");
					//msg.Append(filen);
				} else {
					RFile f;
					TInt err;
					err=f.Open(Fs(), filen, EFileRead|EFileShareAny);
					f.Close();
					if (err==KErrAccessDenied || err==KErrInUse) {
						msg=_L("skipping busy ");
						msg.Append(filen);
						busy_files=true;
					} else {
						list_file_names->AppendL(filen);
						msg=_L("adding file ");
						msg.Append(filen);
					}
				}
			} else {
				if (fe.iModified>iAfterTime) {
					msg=_L("prompting file");
					in_call=true;
					iPrompt.Prompt(filen, this);
					in_call=false;
					return;
				}
			}
		} else if (fe.iSize==0) {
			//msg=_L("size 0");
			Fs().Delete(filen);
		} else {
			//msg=_L("skip file ");
			//msg.Append(filen);
		}
	} else {
		msg.Format(_L("error getting file attr %d"), err);
	}
	if (msg.Length()>0) {
		cb->status_change(msg);
	}
	goto next_step;
}

void CTransferBase::PutIntoFilesL(bool Upload, bool DeleteFromPhone,
				  const TDesC& filename)
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("PutIntoFilesL"));
	if (Upload) {
		if (DeleteFromPhone) {
			TFileName del; ToDel(filename, del);
			RAFile f; f.ReplaceLA(Fs(), del, EFileWrite|EFileShareAny);
		}
	} else if (DeleteFromPhone) {
		Fs().Delete(filen);
	} else {
		Fs().SetAtt(filen, 0, KEntryAttArchive);
	}
}

void CTransferBase::DoBackL(bool Upload, bool DeleteFromPhone,
	MBBData* /*Packet*/)
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("DoBackL"));

	PutIntoFilesL(Upload, DeleteFromPhone, filen);
	if (Upload) {
		RFile f;
		TInt err;
		err=f.Open(Fs(), filen, EFileRead|EFileShareAny);
		f.Close();
		if (err==KErrAccessDenied || err==KErrInUse) {
			busy_files=true;
		} else {
			list_file_names->AppendL(filen);
		}
	}
}

TFileOpStatus CTransferBase::Back(bool Upload, bool DeleteFromPhone,
	MBBData* Packet)
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("Back"));

	CC_TRAPD(err, DoBackL(Upload, DeleteFromPhone, Packet));

	//TODO: report to user
	if (err!=KErrNone) {
		TBuf<50> msg;
		msg.Format(_L("Error %d in Back"), err);
		cb->error(msg);
	}

	if (in_call) {
		iCallBack->Wait(0);
	} else {
		FileStep();
	}
	return TFileOpStatus();
}

void CTransferBase::expired(CBase* source)
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("expired"));

	if (source==iCallBack) 
		FileStep();
}

void CTransferBase::GetFiles(TTime AfterTime)
{
	CALLSTACKITEM_N(_CL("CTransferBase"), _CL("GetFiles"));

	if (listing_files) {
		again=true;
		return;
	}
	busy_files=false;

	if (!file_names) file_index=0;
	listing_files=true;

	if (send_dirs->Count()==0) {
		GotFiles();
		return;
	}

	iAfterTime=AfterTime;

	delete list_file_names; list_file_names=0;
	list_file_names=new (ELeave) CDesC16ArrayFlat(10);

	send_dir_i=dir_i=0;
	dir_count=-1; delete dir; dir=0;

	FileStep();
}

bool CSendUITransfer::transfer_files(TInt cmdid)
{
	CALLSTACKITEM_N(_CL("CSendUITransfer"), _CL("transfer_files"));

	if (in_progress) {
		cb->status_change(_L("busy 1"));
		return false;
	}
	if (file_names) {
		cb->status_change(_L("busy 2"));
		return false;
	}
	if (list_file_names) {
		cb->status_change(_L("busy 3"));
		return false;
	}

#ifndef __S60V3__
#if __WINS__
	TUid MtmUid=sendui->MtmForCommand(cmdid);
	RDebug::Print(MtmUid.Name());
#endif
#endif

	iCmdId=cmdid;

	in_progress=true;

	GetFiles();

	return true;
}

void CSendUITransfer::GotFiles()
{
	CALLSTACKITEM_N(_CL("CSendUITransfer"), _CL("GotFiles"));

	if (listing_files) {
		listing_files=false;
		if (!file_names) {
			file_names=list_file_names;
			list_file_names=0;
		} else {
			return;
		}
	}

	if (!file_names || file_names->Count()==0) {
		delete file_names; file_names=0;
		in_progress=false;
		cb->finished();
		return;
	}

#ifndef __S60V3__
	sendui->CreateAndSendMessageL (iCmdId, 0, file_names);
#endif

	CAknQueryDialog* dlg = new(ELeave) CAknQueryDialog(CAknQueryDialog::ENoTone);
	CleanupStack::PushL(dlg);
	
	_LIT(pr, "Delete files?");
	dlg->SetPromptL(pr);

	CleanupStack::Pop(); //dlg

	if (dlg->ExecuteLD(R_CL_CONFIRMATION_QUERY_DIALOG)) {
		if (cb) cb->status_change(_L("deleting files"));
		for (int i=0; i<file_names->Count(); i++) {
			Fs().Delete((*file_names)[i]);
			TFileName d;
			ToDel((*file_names)[i], d); Fs().Delete(d);
			ToPacket((*file_names)[i], d); Fs().Delete(d);

		}
		if (cb) cb->status_change(_L("deleted files"));
	}

	delete file_names; file_names=0;
	in_progress=false;
	cb->finished();

	return;
}


#if 0
CPeriodicTransfer* CPeriodicTransfer::NewL(MApp_context& Context, int hours, MSocketObserver* callback,CTransferDir* aTransferDir)
{
	CALLSTACKITEM2_N(_CL("CPeriodicTransfer"), _CL("NewL"), &Context);

	auto_ptr<CPeriodicTransfer> ret(new (ELeave) 
		CPeriodicTransfer(Context, hours, callback, aTransferDir));
	ret->ConstructL();
	return ret.release();
}

CPeriodicTransfer::~CPeriodicTransfer()
{
	CALLSTACKITEM_N(_CL("CPeriodicTransfer"), _CL("~CPeriodicTransfer"));

	delete iTimer;
#if !defined(FLICKR) && !defined(CONTEXTLOCA)
	delete iCommLog;
#endif
}

CPeriodicTransfer::CPeriodicTransfer(MApp_context& Context, int hours, MSocketObserver* callback,
				     CTransferDir* aTransferDir) : 
MContextBase(Context), iHours(hours), iCb(callback), iTransferDir(aTransferDir)
{
	CALLSTACKITEM_N(_CL("CPeriodicTransfer"), _CL("CPeriodicTransfer"));

}

void CPeriodicTransfer::NewSensorEventL(const TTupleName& aName, 
	const TDesC& /*aSubName*/, const CBBSensorEvent& aEvent)
{
	const TBBSysEvent* data=bb_cast<TBBSysEvent>(aEvent.iData());
	if (!data) return;

	if (aName==KBatteryTuple) {
		iBattery=data->iState();
	} else if (aName==KChargerTuple) {
		iCharger=data->iState();
	}

	if ( ( (iBattery > 2 && iCharger>0 && iCharger<4) || iCharger==3 || iCharger==2)
		&&
		current_state==IDLE) {

		TTime now;
		now.HomeTime();

		TTime prev_sending(0);
		Settings().GetSettingL(SETTING_LAST_COMMLOG_UPLOAD, prev_sending);

		if (prev_sending < now-TTimeIntervalHours(10)) {
			Transfer();
		}
	}
}

void CPeriodicTransfer::ConstructL()
{
	CALLSTACKITEM_N(_CL("CPeriodicTransfer"), _CL("ConstructL"));

	iTimer=CTimeOut::NewL(*this);
	Mlogger::ConstructL(AppContext());
	SubscribeL(KBatteryTuple);
	SubscribeL(KChargerTuple);

#if !defined(FLICKR) && !defined(CONTEXTLOCA)
	iCommLog=Clog_comm::NewL(AppContext(), this);
#endif
	current_state=IDLE;
	iTimer->Wait(WAIT_PERIOD);
}


void CPeriodicTransfer::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CPeriodicTransfer"), _CL("expired"));

const TTimeIntervalHours	KUploadInterval=60;
const TTimeIntervalHours	KUploadIntervalForce=80;

	TTime now;
	now.HomeTime();

	TInt hour=now.DateTime().Hour();
	TTime prev_sending(0);
	Settings().GetSettingL(SETTING_LAST_COMMLOG_UPLOAD, prev_sending);

	if ( (iBattery>2 || iCharger==3 || iCharger==2)
		&& ( (hour>=UPLOAD_FROM && hour<UPLOAD_TO && prev_sending+KUploadInterval<now) ||
			(prev_sending+KUploadIntervalForce<now) ) ) {
		RDebug::Print(_L("Periodic transfer!"));
		Transfer();
	} else {
		iTimer->Wait(WAIT_PERIOD);
	}
}

void CPeriodicTransfer::Transfer(bool MakeCommLog)
{
	CALLSTACKITEM_N(_CL("CPeriodicTransfer"), _CL("Transfer"));

	iStarted.HomeTime();
	iTimer->Reset();
	iCb->info(this, _L("Getting comm log"));
	current_state=GETTING_COMM_LOG;
#if !defined(FLICKR) && !defined(CONTEXTLOCA)
	if (MakeCommLog && ! NoSpaceLeft()) {
		iCommLogFrom=0;
		TTime existing(0);
		CC_TRAPD(err, Settings().GetSettingL(SETTING_LAST_COMMLOG_UPLOAD, iCommLogFrom));
		iCommLog->LogsExist(existing);
		if (existing>iCommLogFrom) iCommLogFrom=existing;
		if (! iCommLog->write_comm_log(iCommLogFrom)) {
			finished();
		}
	} else 
#endif
	{
		finished();
	}
}

void CPeriodicTransfer::ProcessDir(const TDesC& aFiles)
{
	TBool enabled=EFalse;
	Settings().GetSettingL(SETTING_LOG_UPLOAD_ENABLE, enabled);

	TFileName dir=DataDir();
	if (dir.Right(1).Compare(_L("\\"))) {
		dir.Append(_L("\\"));
	}
	dir.Append(aFiles);
	TDriveInfo i;

	if (enabled) {
		iDummyPrompt.SetUpload(true);
		iDummyPrompt.SetDelete(true);
	} else {
		iDummyPrompt.SetUpload(false);
		iDummyPrompt.SetDelete(false);
	}

	if ( enabled && Fs().Drive(i, EDriveE)==KErrNone ) {
		dir.Replace(0, 1, _L("e"));
		iTransferDir->ProcessDir(dir, SETTING_UPLOAD_URLBASE, SETTING_UPLOAD_SCRIPT, _L(""),
			&iDummyPrompt, TTime(0), ETrue, ETrue);
		dir.Replace(0, 1, _L("c"));
	}
	iTransferDir->ProcessDir(dir, SETTING_UPLOAD_URLBASE, SETTING_UPLOAD_SCRIPT, _L(""),
		&iDummyPrompt, TTime(0), ETrue, ETrue);
}

void CPeriodicTransfer::error(const TDesC& descr)
{
	iCb->error(this, KErrGeneral, descr);
}

void CPeriodicTransfer::status_change(const TDesC& status)
{
	iCb->info(this, status);
}

void CPeriodicTransfer::finished()
{
	CALLSTACKITEM_N(_CL("CPeriodicTransfer"), _CL("finished"));

	if (current_state==GETTING_COMM_LOG) {
		TBool enabled;
		iCb->info(this, _L("FTPing"));
		current_state=FTPING;	
		ProcessDir(_L("*txt"));
		ProcessDir(_L("*amr"));
		ProcessDir(_L("mms*"));
	} else {
		iCb->info(this, _L("Transferred"));
	}
	iCount=0;
	current_state=IDLE;

	CC_TRAPD(err, Settings().WriteSettingL(SETTING_LAST_COMMLOG_UPLOAD, iStarted));
	iTimer->Wait(WAIT_PERIOD);
}

#endif

class CMultiPromptImpl : public CMultiPrompt, public MSettingListener, public MContextBase {
private:
	virtual void AddPromptL(TInt Code, MUploadPrompt* Prompt);
	CMultiPromptImpl(MApp_context& Context);
	void ConstructL();

	virtual void Prompt(const TDesC& FileName, MUploadCallBack* CallBack);

	virtual void SettingChanged(TInt Setting);

	struct TPromptItem {
		MUploadPrompt	*iPrompt;
		TInt		iCode;

		TPromptItem() : iPrompt(0), iCode(-1) { }
		TPromptItem(TInt aCode, MUploadPrompt *aPrompt) : iPrompt(aPrompt), iCode(aCode) { }
	};
	CList<TPromptItem>	*iPrompts;
	TInt			iCurrentCode;
	MUploadPrompt		*iCurrentPrompt;

	friend class CMultiPrompt;

public:
	virtual ~CMultiPromptImpl();
};

CMultiPrompt* CMultiPrompt::NewL(MApp_context& Context)
{
	auto_ptr<CMultiPromptImpl> ret(new (ELeave) CMultiPromptImpl(Context));
	ret->ConstructL();
	return ret.release();
}

CMultiPrompt::~CMultiPrompt()
{
}

CMultiPromptImpl::CMultiPromptImpl(MApp_context& Context) : MContextBase(Context)
{
}

CMultiPromptImpl::~CMultiPromptImpl()
{
	Settings().CancelNotifyOnChange(SETTING_UPLOAD_PROMPT_TYPE, this);
	delete iPrompts;
}

void CMultiPromptImpl::ConstructL()
{
	iPrompts=CList<TPromptItem>::NewL();
	Settings().GetSettingL(SETTING_UPLOAD_PROMPT_TYPE, iCurrentCode);
	Settings().NotifyOnChange(SETTING_UPLOAD_PROMPT_TYPE, this);
}

void CMultiPromptImpl::AddPromptL(TInt Code, MUploadPrompt* Prompt)
{
	iPrompts->AppendL( TPromptItem(Code, Prompt) );
	if (Code==iCurrentCode) iCurrentPrompt=Prompt;
}

void CMultiPromptImpl::SettingChanged(TInt /*Setting*/)
{
	Settings().GetSettingL(SETTING_UPLOAD_PROMPT_TYPE, iCurrentCode);
	CList<TPromptItem>::Node *i=iPrompts->iFirst;
	while (i) {
		if (i->Item.iCode==iCurrentCode) {
			iCurrentPrompt=i->Item.iPrompt;
			break;
		}
		i=i->Next;
	}
}

void CMultiPromptImpl::Prompt(const TDesC& FileName, MUploadCallBack* CallBack)
{
	if (iCurrentPrompt) {
		iCurrentPrompt->Prompt(FileName, CallBack);
	} else {
		CallBack->Back(false, false, 0);
	}
}

