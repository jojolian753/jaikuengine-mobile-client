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

#ifndef CONTEXT_PRESENCEMAINTAINER_H_INCLUDED
#define CONTEXT_PRESENCEMAINTAINER_H_INCLUDED 1

#include "app_context.h"
#include "i_logger.h"
#include "locationing.h"
#include "settings.h"
#include "timeout.h"
#include "circular.h"

class MPresencePublisherListener {
public:
	virtual void NotifyNewPresence(const class CBBPresence* Data)=0;
};

class CBTDeviceList;

class MPresenceMaintainer : public Mlogger, public MContextBase, public MNaming, public MTimeOut, public MListObserver {
protected:
	MPresenceMaintainer(MApp_context& aContext, CBTDeviceList* aBuddyList,
					  CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, 
					  CBTDeviceList *aPDABTs);
	void ConstructL();

	virtual void GotSignificantChange(TBool frozenChanged, TBool aQuickRate=EFalse) = 0;
	// Mlogger
	void MPresenceMaintainer::NewValueL(TUint , const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponent,
		const MBBData* aData);
	virtual void NewSensorEventL(const TTupleName& aName, const TDesC& aSubName, 
		const CBBSensorEvent& aData);
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName);

	class CBBPresence*	iData;
	class CBBPresence*	iFrozen; // must be created/deleted by the derived class
	TTime				iUnfreezeTime;
	CBTDeviceList		*iBuddyList, *iLaptopBTs, *iDesktopBTs, *iPDABTs;
	class MAskForNames* iCellNames;
	CTimeOut*			iNonsignificantTimer;
	TTime				iLastUpdateTime;
	TBool				iNewNameComing;
	virtual void expired(CBase* Source);
	virtual void ContentsChanged() { }
	virtual TBool EvaluateSuspendL(TBool aEvaluateOnly=EFalse) { return EFalse; }
	virtual void ContentsChanged(CBase* );
	TBool EvaluateBt(const class CBBBtDeviceList* devs);
	TBool			iInJaiku, iIdle;
public:
	virtual ~MPresenceMaintainer();
	virtual const class CBBPresence* Data() const;
	virtual void add_cellid_name(const TBBCellId& cellid, const TDesC& name);
};

class CPresenceMaintainer: public CBase, public MPresenceMaintainer {
public:
	static CPresenceMaintainer* NewL(MApp_context& aContext, CBTDeviceList* aBuddyList,
					  CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, 
					  CBTDeviceList *aPDABTs);
protected:
	CPresenceMaintainer(MApp_context& aContext, CBTDeviceList* aBuddyList,
					  CBTDeviceList *aLaptopBTs, CBTDeviceList *aDesktopBTs, 
					  CBTDeviceList *aPDABTs);

};

#endif
