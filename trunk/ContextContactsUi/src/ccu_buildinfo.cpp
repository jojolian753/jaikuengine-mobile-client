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

#include "ccu_buildinfo.h"

#include "app_context.h"
#include "app_context_impl.h"
#include "bbxml.h"
#include "csd_buildinfo.h"
#include "symbian_auto_ptr.h"
#include "raii_f32file.h"

#include "contextvariant.hrh"
#include <contextcontactsui.rsg>

#include <StringLoader.h>
#include <aknconsts.h>
_LIT(KBuildInfoFile, "buildinfo.xml");
namespace BuildInfo
{
	void BuildInfoPathL(TDes& aPath)
	{
		aPath.Append( GetContext()->DataDir() );
		aPath.Append( KBuildInfoFile );
	}

	
	EXPORT_C void ReadL(TBBBuildInfo& aInfo)
	{	
		CALLSTACKITEMSTATIC_N(_CL("BuildInfo"), _CL("ReadL"));
		TFileName fileName;
		BuildInfoPathL( fileName );

		RFs& fs = GetContext()->Fs();	
		auto_ptr<HBufC8> xml( ReadFileL(fs, fileName) );		
		auto_ptr<CSingleParser> parser( CSingleParser::NewL(&aInfo, 
															EFalse,
															EFalse) );
		parser->ParseL( *xml );
	}

	EXPORT_C HBufC* UserStringL(const TBBBuildInfo& aInfo)
	{
		CALLSTACKITEMSTATIC_N(_CL("BuildInfo"), _CL("UserStringL"));
		// Version 0.41.0
		// (11.2.2007 13:14)
		// (Development variant)
		auto_ptr<HBufC> result( HBufC::NewL( 50 + 50 + 50 ) );
		
		auto_ptr< CArrayFix<TInt> > versions( new (ELeave) CArrayFixFlat<TInt>(3));
		versions->AppendL(aInfo.iMajorVersion());
		versions->AppendL(aInfo.iMinorVersion());
		versions->AppendL(aInfo.iInternalVersion());
		auto_ptr<HBufC> versionStr( StringLoader::LoadL(R_TEXT_VERSION_FMT,
														*versions) );		
		result->Des().Append( *versionStr );

		result->Des().Append( KNewLine );
		TBuf<50> date;
		_LIT(KDateFmt, "%F%D.%M.%Y %H:%T");
		aInfo.iWhen().FormatL( date,  KDateFmt );
		result->Des().Append( date );
		
#ifdef __DEV__
		result->Des().Append( KNewLine );		
		_LIT( KDevVariant, "Development variant");
		result->Des().Append( KDevVariant );

		result->Des().Append( KNewLine );		

		_LIT( KBuildBy, "Build by:");
		result->Des().Append( KBuildBy );
		result->Des().Append( aInfo.iBuildBy() );
#endif
		result->Des().Append( KNewLine );
		return result.release();
	}

	EXPORT_C HBufC8* ReadFileL(RFs& aFs, const TDesC& aName)
	{
		RAFile file; file.OpenLA(aFs, aName, EFileRead);
		//User::LeaveIfError( file.Open(aFs, aName, EFileRead) );
		TInt fileSize;
		User::LeaveIfError( file.Size( fileSize ) );
		auto_ptr<HBufC8> buf( HBufC8::NewL(fileSize) );
		TPtr8 pBuf = buf->Des();
		User::LeaveIfError( file.Read(pBuf) );
		return buf.release();
	}
}
	
