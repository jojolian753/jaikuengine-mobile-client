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

#ifndef __CBLACKBOARDSERVERSESSION__
#define __CBLACKBOARDSERVERSESSION__

#include <e32base.h>
#include "BlackBoardServer.h"
#include "blackboard_cs.h"
#include "list.h"

class CIdleCallBack;
class MBack { public: virtual void Back() = 0; };
#include "compat_server.h"

class CBlackBoardServerSession : public SESSION_CLASS, public MBlackBoardObserver, public MBack
{

public: 
#ifndef __IPCV2__
	static CBlackBoardServerSession* NewL(RThread& aClient, CBlackBoardServer& aServer);
#else
	static CBlackBoardServerSession* NewL(CBlackBoardServer& aServer);
#endif
	~CBlackBoardServerSession();
	
	void ServiceL(const MESSAGE_CLASS& aMessage);
	void Exiting();

private:
#ifndef __IPCV2__
	CBlackBoardServerSession(RThread& aClient, CBlackBoardServer& aServer);
#else
	CBlackBoardServerSession(CBlackBoardServer& aServer);
#endif
	void ConstructL() ;
	void PanicClient(const MESSAGE_CLASS& aMessage, TInt aPanic) const;

	virtual void NotifyDirectL(TUint aId, TBBPriority aPriority,
			TTupleType aTupleType,
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponent,
			const TDesC8& aSerializedData,
			const TTime& aExpires);
	virtual void NotifyL(TUint aId, TBBPriority aPriority,
			TTupleType aTupleType,
			const TTupleName& aTupleName, const TDesC& aSubName, 
			const TComponentName& aComponent,
			const TDesC8& aSerializedData,
			const TTime& aExpires);
	virtual void NotifyL(TUint aId, TBBPriority aPriority);
	virtual void NotifyDeletedL(const TTupleName& aTupleName, const TDesC& aSubName);
	virtual void Back();

	void GetOneWaitingL(TUint id);
private: 
	enum TMessageIdxs {
		ENotifyMsgIndex,
		EOtherMsgIndex
	};

	void GetByTupleL();
	void GetByComponentL();
	void PutL();
	void DeleteByTupleL();
	void DeleteByComponentL();
	void DeleteByIdL();
	void AddTupleFilterL();
	void DeleteTupleFilterL();
	void AddComponentFilterL();
	void DeleteComponentFilterL();
	TBool SendWaitingL();
	void TerminateServer();

	void ReadTupleAndSubL(TMessageIdxs aWhich);
	void ReadComponentL(TMessageIdxs aWhich);
	void ReadId(TMessageIdxs aWhich);
	TUint iSize;
	TUint iFlags;
	void ReadFlags(TMessageIdxs aWhich);
	TFullArgs iFullArgs;
	void ReadFullArgsL(TMessageIdxs aWhich);
	void ReadPriorityL(TMessageIdxs aWhich);
	void ReadTupleL(TMessageIdxs aWhich);

	HBufC8* ReadDataL(TMessageIdxs aWhich);

	
	MESSAGE_CLASS iMessage[2]; TInt iMessageThreadId[2];
	void CompleteMessage(TMessageIdxs aWhich, TInt Code);
	void WriteDataL(RADbColReadStream& rs, TMessageIdxs aIdx);
	void WriteDataL(const TDesC8& data, TMessageIdxs aIdx);

	void SetMessage(TMessageIdxs aWhich, const MESSAGE_CLASS& aMsg);
	CBlackBoardServer&     iServer;

	void			AppendNotificationL(TUint aId);
	TUint			PopNotification();
	CCirBuf<TUint>		*iNotifications; TInt iNotificationsMaxLength;

	void			AppendDeleteNotification(const TTupleName& aTupleName, const TDesC& aSubName);
	TBool			TopDeleteNotification(TTupleName& aTupleName, TDes& aSubName);
	void			DeleteTopDeleteNotification();
	void			DeleteDeleteNotification(const TTupleName& aTupleName, const TDesC& aSubName);
	CArrayFixSeg<TTupleName> *iDeleteTupleNames;
	CDesCArraySeg		*iDeleteSubNames;

	CIdleCallBack*		iIdle;
	TBool			iWaitingForNotify;
	TBool			iInPut, iInDelete; // ignore notifications for puts on this session, if so requested
	TBool			iExiting;
};

#endif
