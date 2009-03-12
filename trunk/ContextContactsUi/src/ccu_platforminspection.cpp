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

#include "ccu_platforminspection.h"

#include "app_context.h"
#include "symbian_auto_ptr.h"
#include "reporting.h"

#include "juik_fonts.h"


void LogFontL(MReporting& aReporting, const CFont* aFont)
{
	TInt h = aFont->HeightInPixels();
	TInt maxH = aFont->FontMaxHeight();
	TInt ascent = aFont->AscentInPixels();
	TInt maxAscent = aFont->FontMaxAscent();
	TInt capitalAscent = aFont->FontCapitalAscent();
	TInt descent = aFont->DescentInPixels();
	TInt stdDescent = aFont->FontStandardDescent();
	TInt maxDescent = aFont->FontMaxDescent();
	TInt baseline = aFont->BaselineOffsetInPixels();	 
	TInt gap = aFont->FontLineGap();

	TBuf<100> buf;
	buf.Format(_L("\t%d %d %d %d %d\t\t%d %d %d %d"),
			   h, maxH, ascent, maxAscent, capitalAscent, 
			   descent, stdDescent, baseline,gap);
	aReporting.DebugLog( buf );
}


void LogFontStatsL(MReporting& aReporting)
{
	aReporting.DebugLog( _L("LogFontStats") );
	aReporting.DebugLog( _L("\th, maxH, ascent, maxAscent, capitalAscent | tdescent, stdDescent, baseline, gap") );

	const CFont* font = NULL;

	aReporting.DebugLog( _L("EPrimary") );
	font = JuikFonts::GetLogicalFont( JuikFonts::EPrimary );
	LogFontL( aReporting, font );

	aReporting.DebugLog( _L("ESecondary") );
	font = JuikFonts::GetLogicalFont( JuikFonts::ESecondary );
	LogFontL( aReporting, font );

	aReporting.DebugLog( _L("ETitle") );
	font = JuikFonts::GetLogicalFont( JuikFonts::ETitle );
	LogFontL( aReporting, font );

	aReporting.DebugLog( _L("EPrimarySmall") );
	font = JuikFonts::GetLogicalFont( JuikFonts::EPrimarySmall );
	LogFontL( aReporting, font );

	aReporting.DebugLog( _L("EDigital") );
	font = JuikFonts::GetLogicalFont( JuikFonts::EDigital );
	LogFontL( aReporting, font );
	
}


class CPlatformInspectionImpl : public CPlatformInspection, public MContextBase
{
	void LogAllL()
	{
		CALLSTACKITEM_N(_CL("CPlatformInspectionImpl"), _CL("LogAllL"));
		LogFontStatsL( Reporting() );
	}
};


EXPORT_C CPlatformInspection* CPlatformInspection::NewL()
{
	auto_ptr<CPlatformInspectionImpl> self( new (ELeave) CPlatformInspectionImpl );
	//self->ConstructL();
	return self.release();
}

	

// Runtime check for platform version
// See: http://forum.nokia.com/document/Forum_Nokia_Technical_Library/contents/FNTL/Checking_S60_platform_version_during_installation_or_at_run_time.htm

//#include <f32file.h> 
_LIT(KS60ProductIDFile, "Series60v*.sis");
_LIT(KROMInstallDir, "z:\\system\\install\\");

EXPORT_C void GetS60PlatformVersionL( RFs& aFs, TUint& aMajor, TUint& aMinor )
{
    TFindFile ff( aFs );
    CDir* result;
    User::LeaveIfError( ff.FindWildByDir( KS60ProductIDFile, KROMInstallDir, result ) );
    CleanupStack::PushL( result );
    User::LeaveIfError( result->Sort( ESortByName|EDescending ) );
    aMajor = (*result)[0].iName[9] - '0';
    aMinor = (*result)[0].iName[11] - '0';
    CleanupStack::PopAndDestroy(); // result
}
