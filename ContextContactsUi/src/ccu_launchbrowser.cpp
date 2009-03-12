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

#include "ccu_launchbrowser.h"

#include "ccu_platforminspection.h"

#include "break.h"
#include "app_context.h"
#include <apgcli.h> 
#include <apgtask.h>
#include <eikenv.h>
// Based on reference code from Nokia Technical Library
// http://forum.nokia.com/document/Forum_Nokia_Technical_Library/contents/FNTL/Launching_Web_browser_on_S60_3rd_Ed_to_open_specified_URL.htm
//  

void LaunchBrowserImplL(const TDesC& aUrl, TUid aUid) 
{
	RApaLsSession apaLsSession;
	
	HBufC* param = HBufC::NewLC( aUrl.Length() + 10 );
	if ( aUrl.Find( _L("://") ) == KErrNotFound )
		{
			param->Des().Append( _L("http://") );
		}
	param->Des().Append( aUrl );
	
	TUid id( aUid );	
	TApaTaskList taskList(CEikonEnv::Static()->WsSession());
	TApaTask task = taskList.FindApp(id);

	if(task.Exists())
		{
			task.BringToForeground();
			HBufC8* param8 = HBufC8::NewLC(param->Length());
			param8->Des().Append(*param);
			task.SendMessage(TUid::Uid(0), *param8); // UID not used
			CleanupStack::PopAndDestroy(param8);
		}    
	else
		{
			if(!apaLsSession.Handle())
				{
					User::LeaveIfError(apaLsSession.Connect());
				}
			TThreadId thread;
			User::LeaveIfError(apaLsSession.StartDocument(*param, aUid, thread));
			apaLsSession.Close();        
		}
	CleanupStack::PopAndDestroy(param);
}

EXPORT_C void LaunchBrowserL(const TDesC& aUrl)
{
	const TUid KOSSBrowserUidValue3rdEd = {0x1020724D};
	const TUid KOSSBrowserUidValue3rdEdFP1 = {0x10008D39};
	
	CC_TRAPD( err, LaunchBrowserImplL( aUrl, KOSSBrowserUidValue3rdEd ) );
	if (err == KErrNotFound )
		CC_TRAP( err, LaunchBrowserImplL( aUrl, KOSSBrowserUidValue3rdEdFP1 ) );
	User::LeaveIfError( err );
}

 

    
