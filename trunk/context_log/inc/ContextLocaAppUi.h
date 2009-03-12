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

#ifndef CONTEXT_LOGAPPUI_H
#define CONTEXT_LOGAPPUI_H

#define NULL 0

// INCLUDES
#include <eikapp.h>
#include <eikdoc.h>
#include <e32std.h>
#include <coeccntx.h>

#include "timeout.h"

#include <sendas.h>

#include "csd_cell.h"
#include "bb_logger.h"

class CSmsSocket;

class CNotifyState;

#include "appuibase.h"
// CONSTANTS
//const ?type ?constant_var = ?constant;


// CLASS DECLARATION

/**
* Application UI class.
* Provides support for the following features:
* - EIKON control architecture
* 
*/
class CContextLocaAppUi : public CContextLogAppUiBase,
	public MSendAsObserver, 
	public MSettingListener
{
public: // // Constructors and destructor
	
	CContextLocaAppUi(MApp_context& Context) : CContextLogAppUiBase(Context) { }
        /**
        * EPOC default constructor.
        */      
        void ConstructL();
	
        /**
        * Destructor.
        */      
        ~CContextLocaAppUi();
        
public: // New functions
	
public: // Functions from base classes
	
private:
        // From MEikMenuObserver
        void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane);

private:
	
	// MSettingListener
	virtual void SettingChanged(TInt Setting);
	virtual void TimeSynced(TBool aSuccess);

        /**
        * From CEikAppUi, takes care of command handling.
        * @param aCommand command to be handled
        */
        void HandleCommandL(TInt aCommand);
private:

public:
	void DialogTest();	
private:
	// MSendAsObserver
	virtual TBool CapabilityOK(TUid /*aCapabilty*/, TInt /*aResponse*/) { return ETrue; }
	virtual void expired(CBase*);

private: //Data

	class CSendUITransfer* transferer;

	class CAknGlobalNote* globalNote;
	
	bool case_closed;
	bool ringing;

	class CBBLogger*	bbl;

	class CConnectionOpener* iConnectionOpener;

	RSocketServ	iServ;
#ifdef __S60V2__
	RConnection	iConnection;
#endif

	class CNotifyState		*iContextRunning, *iLoggingRunning;
	class CLocaSender	*iLocaSender;
	class CSmsStatusReplier	*iSmsStatus;

	class CBBLocalRunner	*iBBLocal;
	class CTimeOut			*iNtpTimeOut;
	TInt	iNtpCount;
};

#endif

// End of File
