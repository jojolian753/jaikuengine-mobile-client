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
#include "transferdir2.h"
#include "transfer2.h"
#include "list.h"
#include "symbian_auto_ptr.h"
#include "db.h"
#include "cl_settings.h"
#include <bautils.h>
#include "reporting.h"
#include "cm_post.h"

class CTransferDirImpl : public CTransferDir, public MContextBase, public MUploadCallBack {
private:
	TInt ProcessDir(const TDesC& aDirName, TInt UrlSetting,
		TInt ScriptSetting, const TDesC& aMeta,  MUploadPrompt* Prompt,
		TTime TimeLimit, TBool aMoveToMMC, TBool aDontMoveToSubDirs);
	CTransferDirImpl(MApp_context& Context, MSocketObserver& aObserver);
	void ConstructL(const TDesC& aDbName);
	TFileOpStatus Back(bool Upload, bool DeleteFromPhone, MBBData* Packet);
	virtual CHttpTransfer2* Transferer();

	struct TDirItem {
		TFileName	iDirName;
		TInt		iUrlSetting;
		TInt		iScriptSetting;
		TFileName	iMeta;
		MUploadPrompt*  iPrompt;
		TTime		iTimeLimit;
		TBool		iMoveToMMC;
		TBool		iDontMoveToSubDirs;

		TDirItem(const TDesC& aDirName, TInt UrlSetting,
			TInt ScriptSetting, const TDesC& aMeta, MUploadPrompt* Prompt, TTime TimeLimit,
			TBool aMoveToMMC, TBool aDontMoveToSubDirs) : 
				iDirName(aDirName),
				iUrlSetting(UrlSetting), iScriptSetting(ScriptSetting), iPrompt(Prompt),
				iTimeLimit(TimeLimit), iMoveToMMC(aMoveToMMC), iDontMoveToSubDirs(aDontMoveToSubDirs)
		{
			iMeta=aMeta.Left(KMaxFileName);
		}
	};

	CList<TDirItem*>	*iList;
	TDirItem		*iCurrentDir;
	TInt			iCurrentItem;
	CDir			*iDir;
	CDb			*iDb;
	CHttpTransfer2		*iTransfer;
	TFileName		iCurrentDirName;
	MSocketObserver&	iObserver;
	enum TState { EIdle, EProcessing };
	TState			iCurrentState;
	TInt			iErrorCount;

	void CheckedRunL();
	TInt CheckedRunError(TInt aError);
	void DoCancel();

	void FileStep();
	void Async();
	void DoBackL(bool Upload, bool DeleteFromPhone, MBBData* Packet, TFileOpStatus& f);

	friend class CTransferDir;
	bool has_memory_card;
public:
	~CTransferDirImpl();
};


EXPORT_C CTransferDir* CTransferDir::NewL(MApp_context& Context, MSocketObserver& aObserver, const TDesC& aDbName)
{
	CALLSTACKITEM_N(_CL("CTransferDir"), _CL("NewL"));


	auto_ptr<CTransferDirImpl> ret(new (ELeave) CTransferDirImpl(Context, aObserver));
	ret->ConstructL(aDbName);
	return ret.release();
}

CTransferDir::CTransferDir() : CCheckedActive(EPriorityIdle, _L("CTransferDir")) { }

EXPORT_C CTransferDir::~CTransferDir() { }

TInt CTransferDirImpl::ProcessDir(const TDesC& aDirName, TInt UrlSetting,
	TInt ScriptSetting, const TDesC& aMeta, MUploadPrompt* Prompt, TTime TimeLimit, 
	TBool aMoveToMMC, TBool aDontMoveToSubDirs)
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("ProcessDir"));

	TDirItem* i=new (ELeave) TDirItem(aDirName, UrlSetting, ScriptSetting, aMeta, 
		Prompt, TimeLimit, aMoveToMMC, aDontMoveToSubDirs);
	CC_TRAPD(err, iList->AppendL(i));
	if (err!=KErrNone) delete i;
	else {
		if (iCurrentState!=EIdle) return KErrNone;
		Async();
	}
	return err;
}

CTransferDirImpl::CTransferDirImpl(MApp_context& Context, MSocketObserver& aObserver) : 
		MContextBase(Context), iObserver(aObserver)
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("CTransferDirImpl"));

}

void CTransferDirImpl::ConstructL(const TDesC& aDbName)
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("ConstructL"));

	TBool use_mmc=ETrue;
	Settings().GetSettingL(SETTING_USE_MMC, use_mmc);
	has_memory_card=false;
	TDriveInfo i;
	if (use_mmc && Fs().Drive(i, EDriveE)==KErrNone) {
		// memory card
		has_memory_card=true;
	}

	iList=CList<TDirItem*>::NewL();
	iDb=CDb::NewL(AppContext(), aDbName, EFileRead|EFileWrite|EFileShareAny);

	CActiveScheduler::Add(this);

	iTransfer=CHttpTransfer2::NewL(AppContext(), iDb->Db(), &iObserver);
}

void CTransferDirImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("CheckedRunL"));


	if (iCurrentState==EIdle) {
		delete iCurrentDir; iCurrentDir=0;
		iCurrentDir=iList->Pop();
		if (!iCurrentDir) return;

		delete iDir; iDir=0;
		TInt err=Fs().GetDir(iCurrentDir->iDirName, KEntryAttNormal, ESortByName, iDir);
		{
			TParse p; p.Set(iCurrentDir->iDirName, 0, 0);
			iCurrentDirName = p.DriveAndPath();
			RDebug::Print(iCurrentDirName);
		}
		if (err!=KErrNone) {
			Async();
			return;
		}
		iCurrentItem=0;
		FileStep();
	} else {
		FileStep();
	}
}

TInt CTransferDirImpl::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("CheckedRunError"));


	return aError;
}

void CTransferDirImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("DoCancel"));
}

void CTransferDirImpl::DoBackL(bool Upload, bool DeleteFromPhone, MBBData* Packet, TFileOpStatus& f)
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("DoBackL"));

	TEntry e=(*iDir)[iCurrentItem-1];
	f.fn=iCurrentDirName; f.fn.Append(e.iName);
	if (Upload) {
		Reporting().DebugLog(_L("CTransferDirImpl::DoBackL queuing"));
		Reporting().DebugLog(f.fn);
		CC_TRAPD(err, f.fn=iTransfer->AddFileToQueueL(f.fn, iCurrentDir->iUrlSetting,
			iCurrentDir->iScriptSetting, DeleteFromPhone,
			iCurrentDir->iMeta, Packet, 0, iCurrentDir->iDontMoveToSubDirs));
		if (err!=KErrNone) {
			User::Leave(err);
		}
	} else {
		if (DeleteFromPhone) {
			f.fn.Zero();
			iTransfer->DeleteFile(f.fn);
		} else {
			const CCMPost* post=bb_cast<CCMPost>(Packet);
			TInt64 aPostId=0;
			if (post) aPostId=post->iPostId();
			if (! iCurrentDir->iDontMoveToSubDirs)
				iTransfer->DoNotTransfer(f.fn, aPostId, f);
		}
	}
}

TFileOpStatus CTransferDirImpl::Back(bool Upload, bool DeleteFromPhone, MBBData* Packet)
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("Back"));

	TFileOpStatus ret;
	CC_TRAP(ret.err, DoBackL(Upload, DeleteFromPhone, Packet, ret));
	if (ret.err!=KErrNone) {
		//TAknGlobalNoteType notetype=EAknGlobalErrorNote;
		TInt notetype=4;
		TBuf<100> msg=_L("Failed to queue file for upload: ");
		msg.AppendNum(ret.err);
		Reporting().ShowGlobalNote(notetype, msg);
	}
	Async();
	return ret;
}

void CTransferDirImpl::FileStep()
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("FileStep"));

	if (iCurrentItem < iDir->Count()) {
		iCurrentState=EProcessing;
		TEntry e=(*iDir)[iCurrentItem++];
		TFileOpStatus op;
		TFileName& filen=op.fn;
		filen=iCurrentDirName; filen.Append(e.iName);

		TBool use_mmc=EFalse;
		if (iCurrentDir->iMoveToMMC && has_memory_card && filen.Left(1).CompareF(_L("c"))==0 && 
				Settings().GetSettingL(SETTING_USE_MMC, use_mmc) && use_mmc &&
				e.iName.Left(12).Compare(_L("cellid_names")) ) {
			TFileName filen2;
			filen2=filen;
			filen2.Replace(0, 1, _L("e"));
			if (BaflUtils::CopyFile(Fs(), filen, filen2) == KErrNone) {
				BaflUtils::DeleteFile(Fs(), filen);
				filen=filen2;
			}
		}

		if (iTransfer->FileIsQueued(filen)) {
			Async();
		} else {
			if (e.iModified > iCurrentDir->iTimeLimit) {
				CC_TRAPD(err, iCurrentDir->iPrompt->Prompt( filen, this ));
				if (err!=KErrNone) {
					iErrorCount++;
					if (iErrorCount>3) {
						iErrorCount=0;
						Async();
					} else {
						iCurrentItem--;
						Async();
					}
				}
			} else {
				iTransfer->MoveToOld( filen, op );
				Async();
			}
		}
	} else {
		iCurrentState=EIdle;
		Async();
	}
}

void CTransferDirImpl::Async()
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("Async"));


	if (IsActive()) return;

	TRequestStatus *s=&iStatus;
	User::RequestComplete(s, KErrNone);
	SetActive();
}

CTransferDirImpl::~CTransferDirImpl()
{
	CALLSTACKITEM_N(_CL("CTransferDirImpl"), _CL("~CTransferDirImpl"));

	Cancel();
	delete iTransfer;
	delete iDb;

	if (iList) {
		TDirItem* i;
		while (i=iList->Pop()) delete i;
	}
	delete iCurrentDir;
	delete iList;
	delete iDir;
}

CHttpTransfer2* CTransferDirImpl::Transferer()
{
	return iTransfer;
}
