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

#if !defined(TRANSFER_H_INCLUDED)

#define TRANSFER_H_INCLUDED

#include <e32base.h>
#include <f32file.h>
#include <e32std.h>
#include <sendui.h>
#include <AknQueryDialog.h>

#include "file_output_base.h"
#include "status_notif.h"
//#include "ftp.h"

#include "app_context.h"

#include "log_comm.h"
#include "xmlbuf.h"
#include "timeout.h"
#include "transferdir2.h"
#include "bbdata.h"
#include "i_logger.h"
#include "csd_system.h"


class TDummyPrompt : public MUploadPrompt{
public:
	TDummyPrompt(bool Delete=true);
	virtual void Prompt(const TDesC& FileName, MUploadCallBack* CallBack);
	void SetUpload(bool aUpload);
	void SetDelete(bool aDelete);
private:
	bool	iDelete;
	bool	iUpload;
};

class CMultiPrompt : public MUploadPrompt, public CBase {
public:
	static CMultiPrompt* NewL(MApp_context& Context);
	virtual void AddPromptL(TInt Code, MUploadPrompt* Prompt) = 0;
	virtual ~CMultiPrompt();
};

class CTransferBase: public MContextBase, public CBase, public MUploadCallBack, public MTimeOut
{
public:
	virtual ~CTransferBase();
	
	void add_filesL(const TDesC& file, bool leave_last);
	void PutIntoFilesL(bool Upload, bool DeleteFromPhone,
		const TDesC& filename);
private:
	virtual TFileOpStatus Back(bool Upload, bool DeleteFromPhone,
		MBBData* Packet);
	void DoBackL(bool Upload, bool DeleteFromPhone,
		MBBData* Packet);
protected:
	void GetFiles(TTime AfterTime=TTime(0));
	void FileStep();
	virtual void GotFiles() = 0;
	virtual void expired(CBase* source);

	CDesC16ArrayFlat* file_names, *list_file_names; bool listing_files; bool again, busy_files;
	int	file_index;

	CTransferBase(MApp_context& Context, MUploadPrompt& Prompt, bool move_to_mmc=true);
	void ConstructL(i_status_notif* callback, const TDesC& dir1, const TDesC& dir2);
	CDesC16ArrayFlat* send_dirs;
	CArrayFixFlat<bool>* send_leave;
	CList<TFileName> *dir_prefixes;
	i_status_notif* cb;
	MUploadPrompt&	iPrompt;
	HBufC8*		iPacket8;
	CTimeOut*	iCallBack; bool in_call;

	TInt		send_dir_i, dir_i, dir_count;
	CDir		*dir;
	TTime		iAfterTime;
	TFileName	filen, filen2;
	TFileName	dirname; bool leave;
	TParse		p;
	bool		iFilesInUse;
	bool		has_memory_card, move_to_memory_card;

};

class CSendUITransfer: public CTransferBase {
public:
	static CSendUITransfer* NewL(MApp_context& Context, 
		i_status_notif* callback, TInt cmdid, const TDesC& dir1, const TDesC& dir2);
	void DisplayMenuL(CEikMenuPane& aMenuPane);
	void DisplaySendMenuL(CEikMenuPane& aMenuPane, TInt pos);
	bool transfer_files(TInt cmdid); // true when starting to write, false if already writing
	~CSendUITransfer();
private:
	virtual void GotFiles();

	bool in_progress;
	CSendUITransfer(MApp_context& Context);
	void ConstructL(i_status_notif* callback, TInt cmdid, const TDesC& dir1, const TDesC& dir2);
#ifndef __S60V3__
	CSendAppUi* sendui;
#endif
	TDummyPrompt	iDummyPrompt;
	TInt	iCmdId;
};

/*
class CPeriodicTransfer: public CBase, public MContextBase, public MTimeOut, public i_status_notif,
	public Mlogger
{
public:
	static CPeriodicTransfer* NewL(MApp_context& Context, int hours, MSocketObserver* callback,
		CTransferDir* aTransferDir);
	~CPeriodicTransfer();
	void Transfer(bool MakeCommLog=true);
private:
	CPeriodicTransfer(MApp_context& Context, int hours, MSocketObserver* callback, CTransferDir* aTransferDir);
	void ConstructL();

	virtual void NewSensorEventL(const TTupleName& aName, 
		const TDesC& aSubName, const CBBSensorEvent& aEvent);
	enum state { IDLE, GETTING_COMM_LOG, FTPING };
	state current_state;
	void expired(CBase*);
	virtual void finished();
	virtual void error(const TDesC& descr);
	virtual void status_change(const TDesC& status);

	void ProcessDir(const TDesC& aFiles);

	CTimeOut	*iTimer;
	int		iHours, iCount;
	MSocketObserver	*iCb;
	CTransferDir	*iTransferDir;
	Clog_comm*	iCommLog;
	TTime		iCommLogFrom, iStarted;
	TDummyPrompt	iDummyPrompt;
	TInt		iBattery, iCharger;
};

*/
#endif
