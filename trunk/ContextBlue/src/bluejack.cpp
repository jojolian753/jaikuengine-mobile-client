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

#include "bluejack.h"

#include "symbian_auto_ptr.h"

#ifdef __WINS__
//#define NO_BT
#endif

#ifndef NO_BT
/*
 * Concepts:
 * !Bluejacking!
 * !Setting Bluetooth name!
 */

// reverse engineered from BTENG.LIB:
enum TBTDiscoverabilityMode { EBTDiscoverabilityMode0, EBTDiscoverabilityMode1 };
enum TBTSearchMode { EBTSearchMode0, EBTSearchMode1 };
class CBTMCMSettings : public CBase {
public:
        IMPORT_C static int  GetAllSettings(int &, enum TBTDiscoverabilityMode &, enum TBTSearchMode &, class TDes16 &, int &);
        IMPORT_C static int  GetDiscoverabilityModeL(enum TBTDiscoverabilityMode &);
        IMPORT_C static int  GetLocalBDAddress(class TBTDevAddr &);
        IMPORT_C static int  GetLocalBTName(class TDes16 &);
        IMPORT_C static int  GetPowerStateL(int &);
        IMPORT_C static int  GetSearchModeL(enum TBTSearchMode &);
        IMPORT_C static int  IsLocalNameModified(int &);
        IMPORT_C static class CBTMCMSettings *  NewL(class MBTMCMSettingsCB *);
        IMPORT_C static class CBTMCMSettings *  NewLC(class MBTMCMSettingsCB *);
        IMPORT_C int  SetDefaultValuesL(void);
        IMPORT_C int  SetDiscoverabilityModeL(enum TBTDiscoverabilityMode, int);
        IMPORT_C int  SetLocalBTName(class TDesC16 const &);
        IMPORT_C int  SetPowerStateL(int, int);
        IMPORT_C int  SetSearchModeL(enum TBTSearchMode);
};
#endif

class CBlueJackImpl : public CBlueJack, public MContextBase {
private:
	CBlueJackImpl(MApp_context& aContext, MObexNotifier& aNotifier);
	void ConstructL();
	~CBlueJackImpl();

	virtual void SendMessageL(const TBTDevAddr& aToAddress,
		const TDesC& aWithDeviceName,
		const TDesC& aWithMessageName,
		const TDesC8& aMessage,
		TInt	aConnectCount);
	virtual void CancelSend(); 

	CObexBufObject* iObject;
	CBufFlat*	iBuffer;
	CBtObex*	iObex;
	MObexNotifier&	iNotifier;
#ifndef NO_BT
	CBTMCMSettings* iSettings;
#endif

	friend class CBlueJack;
	friend class auto_ptr<CBlueJackImpl>;
};

EXPORT_C CBlueJack* CBlueJack::NewL(MApp_context& aContext, MObexNotifier& aNotifier)
{
	CALLSTACKITEM_N(_CL("CBlueJack"), _CL("NewL"));


	auto_ptr<CBlueJackImpl> ret(new (ELeave) CBlueJackImpl(aContext, aNotifier));
	ret->ConstructL();
	return ret.release();
}

CBlueJackImpl::CBlueJackImpl(MApp_context& aContext, MObexNotifier& aNotifier) :
	MContextBase(aContext), iNotifier(aNotifier)
	{ }

void CBlueJackImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CBlueJackImpl"), _CL("ConstructL"));


	iBuffer=CBufFlat::NewL(1024);
	iObject=CObexBufObject::NewL(iBuffer);

	iObex=CBtObex::NewL(AppContext(), iNotifier);
#ifndef NO_BT
	iSettings=CBTMCMSettings::NewL(0);
#endif
}

CBlueJackImpl::~CBlueJackImpl()
{
	CALLSTACKITEM_N(_CL("CBlueJackImpl"), _CL("~CBlueJackImpl"));


	delete iObex;
	delete iObject;
	delete iBuffer;
#ifndef NO_BT
	if (iSettings)
		iSettings->SetLocalBTName(_L("name"));
	delete iSettings;
#endif
}

void CBlueJackImpl::SendMessageL(const TBTDevAddr& aToAddress,
	const TDesC& aWithDeviceName,
	const TDesC& aWithMessageName,
	const TDesC8& aMessage,
	TInt	aConnectCount)
{
	CALLSTACKITEM_N(_CL("CBlueJackImpl"), _CL("SendMessageL"));

#ifndef NO_BT
	iSettings->SetLocalBTName(aWithDeviceName);
#endif

	iBuffer->Reset();
	iBuffer->InsertL(0, aMessage);
	iObject->SetTypeL(_L8("text/plain"));
	iObject->SetNameL(aWithMessageName);
	iObject->SetDescriptionL(_L("description of message"));

	iObex->SendMessage(aToAddress, iObject, aConnectCount);
}

void CBlueJackImpl::CancelSend()
{
	CALLSTACKITEM_N(_CL("CBlueJackImpl"), _CL("CancelSend"));

	iObex->CancelSend();
}
