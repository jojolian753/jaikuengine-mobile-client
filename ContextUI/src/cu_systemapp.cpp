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

#include "cu_systemapp.h"

#if defined(__S60V2__) && !defined(__S60V3__)
#include <sysutil.h>
#include "i_logger.h"
#include "csd_current_app.h"
#include <eikenv.h>
#include "symbian_auto_ptr.h"
#include "reporting.h"

class CUninstallSupportImpl: public CUninstallSupport, public Mlogger, public MContextBase {
public:
	TBool iIsSystem, iInManager;
	TBool iNeedsSupport;
	void ConstructL() {
		//Reporting().DebugLog(_L("CUninstallSupportImpl::ConstructL"));
		HBufC8* agent=SysUtil::UserAgentStringL();
		if ( (*agent).Left(8).Compare(_L8("NokiaN70"))==0) iNeedsSupport=ETrue;
		delete agent;
		iIsSystem=CEikonEnv::Static()->IsSystem();
		// FIXME: testing -MR 
		iNeedsSupport=ETrue;
#ifndef __WINS__
		if (iNeedsSupport && iIsSystem) 
#endif
		{
			Reporting().DebugLog(_L("Uninstall: enabling"));
			Mlogger::ConstructL( *this );
			SubscribeL(KCurrentAppTuple);
		}
	}
	virtual void NewSensorEventL(const TTupleName& aName, 
		const TDesC& aSubName, const CBBSensorEvent& aEvent) {
		
		Reporting().DebugLog(_L("Uninstall: new event"));
		const TBBCurrentApp* app=bb_cast<TBBCurrentApp>(aEvent.iData());
		if (!app) return;
		TBuf<100> msg=_L("Uninstall: appuid ");
		msg.AppendNum( app->iUid(), EHex );
		Reporting().DebugLog(msg);
#ifndef __WINS__
		if (app->iUid() == 0x101f8512) {
#else
		if (app->iUid() == 0x101f4cd2) {
#endif
			if (! iInManager) {
				Reporting().DebugLog(_L("Uninstall: unset system"));
				CEikonEnv::Static()->SetSystem(EFalse);
				iInManager=ETrue;
			}
		} else if (iInManager) {
			Reporting().DebugLog(_L("Uninstall: set system"));
			CEikonEnv::Static()->SetSystem(ETrue);
		}
	}
};

EXPORT_C CUninstallSupport* CUninstallSupport::NewL()
{
	auto_ptr<CUninstallSupportImpl> ret(new (ELeave) CUninstallSupportImpl);
	ret->ConstructL();
	return ret.release();
}
#else
EXPORT_C CUninstallSupport* CUninstallSupport::NewL() { return 0; }
#endif
