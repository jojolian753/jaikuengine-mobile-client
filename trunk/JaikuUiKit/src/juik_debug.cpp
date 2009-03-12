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

#include "juik_debug.h"

#include "app_context.h"
#include "reporting.h"  
#include "callstack.h"

#include <e32std.h>
#include <w32std.h>


void JuikDebug::DrawBoundingBox(CWindowGc& aGc, const TRect& aRect)
{
#ifdef JUIK_BOUNDINGBOXES
	TRect r = aRect;
	TRgb c = KRgbRed;
	switch ( r.iTl.iY % 3 )
		{
		case 0: c = KRgbGreen; break;
		case 1: c = KRgbYellow; break;
		case 2: c = KRgbRed; break;
		};
	DrawBoundingBox( aGc, r, c );
#endif
}
 
void JuikDebug::DrawBoundingBox(CWindowGc& aGc, const TRect& aRect, TRgb aColor)
{
#ifdef JUIK_BOUNDINGBOXES
	TRect r = aRect;
	TRgb c = aColor;
	aGc.SetPenColor( c );
	aGc.SetBrushColor( c );
	aGc.SetBrushColor( aGc.ESolidBrush );
	aGc.DrawRect( r );
#endif
}

void JuikDebug::PrintRect(MApp_context_access* aContext, TInt aDebugId, const TDesC& aMsg, const TRect& r) 
{
#ifdef JUIK_DEBUGGING_ENABLED 
	if ( aDebugId != KErrNotFound )
		{
			const TInt KMaxLength(100);
			const TInt KNonMsgLength(50); // approx. 
			TBuf<KMaxLength> buf;
			TInt depth = aContext->CallStackMgr().CallStackDepth();
			if ( aMsg.Length() < KMaxLength - KNonMsgLength )
				buf.Format( _L("%d %S: %d (%d,%d),(%d,%d) [%d,%d]"), 
							depth,
							&aMsg, 
							aDebugId, r.iTl.iX, r.iTl.iY, r.iBr.iX, r.iBr.iY, r.Width(), r.Height() );
			else
				buf = _L("Error : JuikDebug::PrintRect : Too long message");
			if ( aContext )
				aContext->Reporting().DebugLog( buf );
		}
#endif
}

void JuikDebug::PrintSize(MApp_context_access* aContext, TInt aDebugId, const TDesC& aMsg, const TSize& aSize) 
{
#ifdef JUIK_DEBUGGING_ENABLED 
	if ( aDebugId != KErrNotFound )
		{
			const TInt KMaxLength(100);
			const TInt KNonMsgLength(50); // approx. 
			TBuf<KMaxLength> buf;
			TInt depth = aContext->CallStackMgr().CallStackDepth();
			
			if ( aMsg.Length() < KMaxLength - KNonMsgLength )
				buf.Format( _L("%d %S: %d [%d,%d]"), depth, &aMsg, aDebugId, aSize.iWidth, aSize.iHeight );
			else 
				buf = _L("Error : JuikDebug::PrintSize : Too long message");
			
			if ( aContext )
				aContext->Reporting().DebugLog( buf );
		}
#endif
}
