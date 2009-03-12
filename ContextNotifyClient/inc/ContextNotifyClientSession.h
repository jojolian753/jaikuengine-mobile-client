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

#ifndef __CONTEXTNOTIFYCLIENTSESSION_H__
#define __CONTEXTNOTIFYCLIENTSESSION_H__

#include <e32base.h>
#include <fbs.h>
#include <coecntrl.h>
#ifndef __S80__
#include <akntitle.h>
#endif

class RContextNotifyClientSession : public RSessionBase
{
public:
	IMPORT_C RContextNotifyClientSession();
	IMPORT_C TInt Connect();
	IMPORT_C void Close();
	IMPORT_C TVersion Version() const;
	IMPORT_C void TerminateContextNotify(TRequestStatus& aStatus);
	IMPORT_C void AddIcon(CFbsBitmap* aIcon, CFbsBitmap* aMask, TInt& aId, TRequestStatus& aStatus);
	IMPORT_C void RemoveIcon(TInt aId, TRequestStatus& aStatus);
	IMPORT_C void ChangeIcon(CFbsBitmap* aIcon, CFbsBitmap* aMask, TInt aId, TRequestStatus& aStatus);
	IMPORT_C void SetIdleBackground(CFbsBitmap* aBitmap, TRequestStatus& aStatus);
	IMPORT_C void NotifyOnIconChange(TInt* aIconIds, TInt aCount, TRequestStatus& aStatus);
	
	IMPORT_C void Cancel() const;
private: 
#ifndef __S60V2__
	void SendReceive(TInt aFunction, TRequestStatus& aStatus);
	void SendReceive(TInt aFunction);
	using RSessionBase::SendReceive;
#endif
	TPckg<TInt> *iIdPckg;
	TPtr8	iNotifyBuffer;
};

#ifndef __S80__
class CLocalNotifyWindow : public CCoeControl {
public:
	IMPORT_C static void CreateAndActivateL();
	IMPORT_C static void Destroy();
	IMPORT_C static CLocalNotifyWindow* Global(); 
	//virtual TInt AddIconL(CFbsBitmap* aIcon, CFbsBitmap* aMask) = 0;
	//virtual void RemoveIcon(TInt aId) = 0;
	//virtual void ChangeIconL(CFbsBitmap* aIcon, CFbsBitmap* aMask, TInt Id) = 0;
	virtual void DrawOnGc(CWindowGc& gc, TPoint& aFromPos) = 0;
	virtual void AddNotifiedControl(CCoeControl* aControl) = 0;
	virtual void RemoveNotifiedControl(CCoeControl* aControl) = 0;
	virtual void IconsChanged(TInt* aIconIds, TInt aCount) = 0;
protected:
	~CLocalNotifyWindow();
};

class CNotifyWindowControl : public CCoeControl {
public:
	IMPORT_C void Draw(const TRect& aRect) const;
	IMPORT_C void ConstructL(CCoeControl *aTopLevel);
	IMPORT_C ~CNotifyWindowControl();
private:
	CLocalNotifyWindow* iLocal;
	CCoeControl *iTopLevel;
};

#endif

#endif
