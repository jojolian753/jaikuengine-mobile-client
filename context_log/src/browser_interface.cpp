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

#include "browser_interface.h"

#include "app_context.h"

//#pragma message("TODO: change the InfoWinL() calls depending on how you handle error situations.")

CDorisBrowserInterface *CDorisBrowserInterface::NewL()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("NewL"));

    CDorisBrowserInterface *self = CDorisBrowserInterface::NewLC();
    CleanupStack::Pop();
    return self;
}

CDorisBrowserInterface *CDorisBrowserInterface::NewLC()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("NewLC"));

    CDorisBrowserInterface *self = new (ELeave) CDorisBrowserInterface();
    CleanupStack::PushL (self);
    self->ConstructL();
    return self;
}

void CDorisBrowserInterface::ConstructL()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("ConstructL"));

	iPackedString = new (ELeave) TBuf<2048>;

	if( KErrNone != iApaSession.Connect() )
	{
		CEikonEnv::Static()->InfoWinL(_L("iApaSession.Connect() failed."),_L(""));
	}
}

CDorisBrowserInterface::CDorisBrowserInterface()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("CDorisBrowserInterface"));

}

CDorisBrowserInterface::~CDorisBrowserInterface()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("~CDorisBrowserInterface"));

	delete iPackedString;
	iPackedString = NULL;

	iApaSession.Close();
}

void CDorisBrowserInterface::StartAppIfNotRunningL()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("StartAppIfNotRunningL"));

	StartAppIfNotRunningL( _L("") );
}

void CDorisBrowserInterface::StartAppIfNotRunningL( const TDesC &parameter )
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("StartAppIfNotRunningL"));

	TFileName appName;

	if(!IsRunning() && GetAppFullPath(appName))
	{
		CApaCommandLine *cmdLine = CApaCommandLine::NewLC();
		cmdLine->SetLibraryNameL(appName);
		cmdLine->SetDocumentNameL(parameter);
		cmdLine->SetCommandL(EApaCommandRun);
		EikDll::StartAppL(*cmdLine);
		CleanupStack::PopAndDestroy();
	}
}

TBool CDorisBrowserInterface::IsRunning()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("IsRunning"));

	TApaTaskList tlist( CCoeEnv::Static()->WsSession() );
	TApaTask task = tlist.FindApp( KDorisBrowserUid );
	return task.Exists();
}

void CDorisBrowserInterface::SendCommandToRunningApp( const TDesC &docName )
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("SendCommandToRunningApp"));

	TApaTaskList tlist( CCoeEnv::Static()->WsSession() );
	TApaTask task = tlist.FindApp( KDorisBrowserUid );
	if( task.Exists() )
	{
		task.BringToForeground();
		(void)task.SwitchOpenFile( docName );
	}
}

void CDorisBrowserInterface::SendKey( TInt aKeyCode, TInt aModifiers )
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("SendKey"));

	TApaTaskList tlist( CCoeEnv::Static()->WsSession() );
	TApaTask task = tlist.FindApp( KDorisBrowserUid );
	if( task.Exists() )
	{
		// task.BringToForeground();
		task.SendKey( aKeyCode, aModifiers );
	}
}

void CDorisBrowserInterface::Zero()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("Zero"));

	iPackedString->Zero();
}

void CDorisBrowserInterface::AppendL( const TDorisCommand aCommand )
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("AppendL"));

	AppendL( aCommand, _L("") );
}

void CDorisBrowserInterface::AppendL( const TDorisCommand aCommand, const TInt aIntpar )
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("AppendL"));

	TBuf<32> aParameter;
	aParameter.Format( _L("%d"), aIntpar );
	AppendL( aCommand, aParameter );
}

void CDorisBrowserInterface::AppendL( const TDorisCommand aCommand, const TDesC &aParameter )
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("AppendL"));

	if( (aCommand == EEncoding_STRING || aCommand == EOpenURL_STRING) &&
			(aParameter.Length() == 0)
		)
	{
		CEikonEnv::Static()->InfoWinL(_L("This command requires a string parameter."),_L(""));
	} else {
		TBuf<32> asmallstr;
		asmallstr.Format( _L("<%d>"), aCommand );

		if( iPackedString->MaxLength() >= iPackedString->Length() + asmallstr.Length() )
		{
			iPackedString->Append( asmallstr );
		} else {
			CEikonEnv::Static()->InfoWinL(_L("Out of command buffer space."),_L(""));
		}

		if( iPackedString->MaxLength() >= iPackedString->Length() + aParameter.Length() )
		{
			iPackedString->Append( aParameter );
		} else {
			CEikonEnv::Static()->InfoWinL(_L("Out of command buffer space."),_L(""));
		}
	}
}

void CDorisBrowserInterface::ExecuteL()
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("ExecuteL"));

	ExecutePackedCommandLineL( *iPackedString );
}

void CDorisBrowserInterface::ExecutePackedCommandLineL( const TDesC &docName )
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("ExecutePackedCommandLineL"));

	if(!IsRunning())
	{
		TFileName full_path;
		if(GetAppFullPath( full_path ))
		{
			StartAppIfNotRunningL(docName);
		}
	} else {
		SendCommandToRunningApp( docName );
	}
}

TBool CDorisBrowserInterface::GetAppFullPath( TDes &full_path )
{
	CALLSTACKITEM_N(_CL("CDorisBrowserInterface"), _CL("GetAppFullPath"));

	TBool ret = EFalse;

	TApaAppInfo browser_info;
	if( KErrNone == iApaSession.GetAppInfo( browser_info, KDorisBrowserUid ) )
	{
		if( full_path.MaxLength() >= browser_info.iFullName.Length() ) {
			full_path.Copy( browser_info.iFullName );
			ret = ETrue;
		}
	}
	return ret;
}
