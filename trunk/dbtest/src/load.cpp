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

#include <e32std.h>
#include <f32file.h>
#include "load.h"

TInt LoadLogicalDriverL(RFs &fileSystem, const TDesC16 &lddName, i_status_notif* cb)
{
	TInt  ret;
	TUint val = 0x51000000U;
	TPtr8 bufPtr((unsigned char *)&val, 4, 4);
	TInt flag = 0;
	TInt cnt = 0;
	
	RFile ldd;
	
	do
	{ ret = User::LoadLogicalDevice(lddName);
	if(ret == KErrNotSupported)
	{ cb->error(_L("KErrNotSupported"));
	ldd.Open(fileSystem, lddName, EFileWrite);
	ldd.Write(0x48, bufPtr);
	cnt++;
	val += 0x40000;
	ldd.Close();
	} else  if(ret == KErrNone || ret == KErrAlreadyExists) flag = 1; else
		User::Leave(ret);
	} while(!flag && val != 0x51000000U);
	
	if(ret == KErrAlreadyExists) cb->error(_L("KErrAlreadyExists"));
	return cnt;
}

void SetReadOnly(RFs &fileSys, const TDesC16 &file, TBool attr)
{
	if(attr) fileSys.SetAtt(file, KEntryAttReadOnly, KEntryAttNormal);
	else fileSys.SetAtt(file, KEntryAttNormal, KEntryAttReadOnly);
}

void RestoreLDD(RFs &fileSystem, const TDesC16 &lddName)
{
	TInt  ret;
	TUint val = 0;
	TPtr8 bufPtr((unsigned char *)&val, 4, 4);
	RFile ldd;
	
	ldd.Open(fileSystem, lddName, EFileWrite);
	ldd.Write(0x48, bufPtr);
	ldd.Close();
	
	return;
}

void run(i_status_notif* cb)
{
	RFs fileSystem;
	_LIT(LDD_Name,"c:\\system\\lib\\test.ldd");
	TInt err;
	TBuf<100> buf;
	TInt ret = 0;
	
	fileSystem.Connect();
	TRAP(err, ret = LoadLogicalDriverL(fileSystem, LDD_Name, cb));
	if(err == KErrAccessDenied)
	{
		SetReadOnly(fileSystem, LDD_Name, ETrue);
		TRAP(err, ret = LoadLogicalDriverL(fileSystem, LDD_Name, cb));
	}
	if(err == KErrAlreadyExists)	{ 
		cb->error(_L("KErrAlreadyExists"));
		return;
	} else if(err == KErrNoMemory) { 
		cb->error(_L("KErrNoMemory"));
		return;
	} else if(err != KErrNone) {
		buf.Format(_L("Error: %d"), err);
		cb->error(buf);
	} else 
		cb->error(_L("KErrNone"));
	
	if(ret) RestoreLDD(fileSystem, LDD_Name);
}
