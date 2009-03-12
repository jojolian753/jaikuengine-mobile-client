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

#ifndef CONTEXT_APPUIBASE_H_INCLUDED
#define CONTEXT_APPUIBASE_H_INCLUDED 1

#include <aknviewappui.h>
class CAppLogView;
class CBTDeviceList;
#include "reporting.h"
#include "sensorrunner.h"
#include "status_notif.h"
#include "app_context.h"
#include "contextappui.h"
#include "connectioninit.h"
#include "locationing.h"
#include "timeout.h"
#include "cm_autotaglist.h"
#include "ntpconnection.h"
#include <AknProgressDialog.h>
#include "autoap.h"
#include "independent.h"
#include "cbbsession.h"

class CContextLogAppUiBase : public CAknViewAppUi, public i_status_notif,
	public MContextBase, public MSocketObserver, 
	public MContextAppUi, public MAppEvents,
	public MNTPObserver, public MProgressDialogCallback,
	public MAccessPointTestResult,
	public MSettingListener,
	public MBBObserver
{
protected:
	CContextLogAppUiBase(MApp_context& Context) : MContextBase(Context) { initialising=true; }
        void ConstructL();
	void ConstructAfterPresenceMaintainerL();
	void FinalConstructL();
        ~CContextLogAppUiBase();
	static TInt InitializationSteps();

	bool initialising;
	// i_Clog_comm_notif
	void finished();
	void error(const TDesC& descr);
	void status_change(const TDesC& status);
	
	// MNTPObserver
	virtual void NTPInfo(const TDesC& aMsg);
	virtual void NTPError(const TDesC& aMsg, TInt aErrorCode);
	virtual void NTPSuccess(TTime aNewTime);

	// MSocketObserver
	virtual void success(CBase* source) {
		if (source==iLastErrorFrom) {
			iLastErrorFrom=0;
			error(_L(""));
		}
	}
	virtual void error(CBase* source, TInt code, const TDesC& reason);
	virtual void info(CBase* source, const TDesC& msg) {
		if (source==iLastErrorFrom) {
			iLastErrorFrom=0;
			error(_L(""));
		}
		status_change(msg);
	}
	
	virtual void SettingChanged(TInt Setting);
	void StartMediaRunner();

        TBool BaseHandleCommandL(TInt aCommand); // did handle?
	virtual TErrorHandlerResponse HandleError
	    (TInt aError,
	     const SExtendedError& aExtErr,
	     TDes& aErrorText,
	     TDes& aContextText);
	virtual void LogFormatted(const TDesC& aMsg);
	void LogAppEvent(const TDesC& msg);
	virtual void HandleSystemEventL  (  const TWsEvent &    aEvent  );
	virtual void HandleWsEventL(const TWsEvent& aEvent, CCoeControl* aDestination);
	virtual TBool ProcessCommandParametersL(TApaCommand aCommand, 
		TFileName& aDocumentName,const TDesC8& aTail);
	void expired(CBase* aSource);

	virtual void  DialogDismissedL (TInt aButtonId);
	void SyncTimeL(TBool aFromUser);
	virtual void TimeSynced(TBool aSuccess);

	void TestApL(TInt aAp);
	// MAccessPointTestResult
	virtual void Done(TInt aAp, TInt aError);
	void ReleaseCContextLogAppUiBase(void);

	class CContext_logContainer* iAppContainer; 
	class CStatusView*		iStatusView;
	class Mlog_base_impl *app_log;
	
	class sms * smsh;
	CBase		*iLastErrorFrom;
	class CCircularLog	*iLastLog;

	TBuf<30> state;
	class CAppLogView		*iLogView; // OWNED BY FRAMEWORK
	CArrayPtrFlat<class Mlog_base_impl>	*iLoggers;
	class CDb			*iBTDb;
	class CBTDeviceList		*iBuddyBTs, *iLaptopBTs, *iDesktopBTs, *iPDABTs;
	class CTimeOut		*iWatchDogTimer;
	class CNTPConnection	*iNTPConnection;
	//class CSensorRunner	*iSensorRunner;
	independent_worker	iSensorWorker;
	class CBBSubSession	*iBBSubSession;
	class CCellNaming	*iCellNaming;
	class CAknWaitDialog	*iNTPWait;

	class MPresenceMaintainer *iPresenceMaintainer; //owned by derived class
	class MNetworkConnection *iNetworkConnection; //owned by derived class

	const TBBCellId*	iAskedFor; TDes* iAskedInto; TBool iGotAsked;
	TInt iNtpErrors;
	TBool iUserIniatedNtp;
	class CUninstallSupport*	iUninstallSupport;
	class CAccessPointTester*	iAccessPointTester;
	RLibrary			iMediaPluginLibrary;
	class CMediaRunner*		iMediaRunner;
	class CIndependentWatcher* iSensorsWatcher;
	
	TBuf<100> iComponentMessage;
	virtual void NewValueL(TUint aId, const TTupleName& aName, const TDesC& aSubName, 
		const TComponentName& aComponentName, const MBBData* aData);
	virtual void DeletedL(const TTupleName& aName, const TDesC& aSubName);

public:
	void start_app(TUid app, TInt view);
};

#endif
