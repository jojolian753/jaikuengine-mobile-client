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

#ifndef BB_CS_H_INCLUDED
#define BB_CS_H_INCLUDED 1

#include <e32base.h>
#include "bbdata.h"

//--------------------- SERVER PANICS --------------------

//#define NO_COMPONENTNAME

//Panic Category
_LIT(KBlackBoardServer, "BlackBoard");

_LIT(KSubscriptionDir, "\\system\\data\\context\\bbsubs\\");

_LIT(KInsertsDir, "bbinsert\\");

//Panic Codes
enum TBlackBoardServPanic
{
        EBBBadRequest,
        EBBBadDescriptor,
        ESrvCreateServer,
        ECreateTrapCleanup
};

enum TTupleType
{
	ETupleDataOrRequest,	
	ETuplePermanentSubscriptionEvent,
	ETupleSpaceInternal,
	ETupleRequest, // not used
	ETupleReply,
	ETupleFailed
};

enum TBBPriority
{
	EBBPriorityLow,
	EBBPriorityNormal,
	EBBPriorityHigh,
	EBBPriorityOOB,
};

const TUint KPriorityCount = 4;
const TBBPriority KNotifyDirectLimit = EBBPriorityHigh;

_LIT(KBlackBoardServerName,"myBlackBoardServer");
_LIT(KBlackBoardServerSemaphoreName, "AmyBlackBoardServerSemaphore");

//the server version. A version must be specified when
//creating a session with the server
const TUint KBlackBoardServMajorVersionNumber=1;
const TUint KBlackBoardServMinorVersionNumber=0;
const TUint KBlackBoardServBuildVersionNumber=1;

/*
 * basic message layout:
 *	flags
 *	IN argument struct pointer 
 *	OUT argument struct pointer / IN priority
 *	return buffer pointer
 */

//opcodes used in message passing between client and server
enum TBlackBoardServRqst
{
	EGetByTuple,	// flags, IN TTupleName|TTupleArgs, OUT TFullArgs, OUT data
	EGetByComponent, // not used, IN TComponentName, OUT TFullArgs, OUT data
	EPut,		// not used, IN TFullArgs, OUT_OPT TUint id, IN data
	EDeleteByTuple, // flags,  IN TTupleName|TTupleArgs, not in use, not in use
	EDeleteByComponent, // not in use,  IN TComponentName, not in use, not in use
	EDeleteById,	// not in use,  IN TUint, not in use, not in use
	EAddNotifyByTupleFilter, // flags,  IN TTupleName.iModule.iUid, IN TTupleName.iId, IN TBBPriority
	EDeleteNotifyByTupleFilter, // flags,  IN TTupleName, not in use, not in use
	EAddNotifyByComponentFilter, // not in use,  IN TComponentName.iModule.iUid, IN TComponentName.iId, not in use
	EDeleteNotifyByComponentFilter, // not in use,  IN TComponentName, not in use, not in use
	ENotifyOnChange, // not used, not used, OUT TFullArgs, OUT data
	ETerminateBlackBoardServer, // no args

	ECancelOther, // no args
	ECancelNotify,
};

const TUint KArgFlagNoSubName = 1;
const TUint KArgFlagReplace = 2;
const TUint KArgFlagGetExisting = 4;
const TUint KArgFlagDontPersist = 8; // for testing
const TUint KArgFlagDoNotNotifySender = 16;
const TUint KArgFlagKeepExisting = 32;

struct TFullArgs {
	TUint		iId;
	TTupleName	iTupleName;
	TBuf<KMaxTupleSubNameLength> iSubName;
	TComponentName	iComponentName;
	TBBPriority	iPriority;
	TTupleType	iTupleType;
	TTime		iLeaseExpires;
};

struct TTupleArgs {
	TTupleName	iTupleName;
	TBuf<KMaxTupleSubNameLength> iSubName;
};

// TComponentName passed as is

//opcodes used by server to indicate which asynchronous service
//has completed
enum TBlackBoardServRqstComplete
{
	ERequestCompleted = 1,
	EDeleteNotification = 2,
	EBufferTooSmall = -3,
	EBlackBoardServerTerminated = -4
};

#if defined(__WINS__)
IMPORT_C TInt BlackBoardThreadFunction(TAny*);
#endif

#endif
