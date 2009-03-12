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

#include "splash.h"
#include "app_context.h"
#include <symbian_auto_ptr.h>
#include <eikenv.h> 
#include <w32std.h>
#ifdef FLICKR
#include <contextflickr.mbg>
#else
#include <context_log.mbg>
#endif
#include <bautils.h>


class CSplashScreenImpl : public CSplashScreen {
private:
	TInt iSteps, iDone;
	CWsBitmap* iBitmap;
	RWindowGroup iWg;
	TInt progress_left;
	TInt progress_width;
	TInt progress_height;
	TInt progress_top;
	void ConstructL(TInt aSteps) {


		iWg=RWindowGroup(iEikonEnv->WsSession());
		User::LeaveIfError(iWg.Construct((TUint32)&iWg, EFalse));
		iWg.SetOrdinalPosition(1, ECoeWinPriorityAlwaysAtFront+1);
		iWg.EnableReceiptOfFocus(EFalse);

		CreateWindowL(&iWg);

		iSteps=aSteps;
		iBitmap=new (ELeave) CWsBitmap(iEikonEnv->WsSession());
		TBuf<50> bm=_L("c:\\");
#ifndef __S60V3__
		bm.Append(_L("system\\data\\"));
#else
		bm.Append(_L("resource\\"));
#endif

#ifdef FLICKR
		bm.Append(_L("contextflickr.mbm"));
#else
		bm.Append(_L("context_log.mbm"));
#endif
#ifndef __WINS__
		if (! BaflUtils::FileExists(iEikonEnv->FsSession(), bm) ) bm.Replace(0, 1, _L("e"));
#else
		bm.Replace(0, 1, _L("z"));
#endif
#ifdef FLICKR
		User::LeaveIfError(iBitmap->Load(bm, EMbmContextflickrMeaning));
#else
		User::LeaveIfError(iBitmap->Load(bm, EMbmContext_logMeaning));
#endif

		SetExtentToWholeScreen();
		TInt scale=1;
		if (Rect().Width()>176) scale=2;

		progress_left=28*scale;
		progress_width=120*scale;
		progress_height=6*scale;
		progress_top=165*scale;
	}
	~CSplashScreenImpl() {
		iWg.Close();
		delete iBitmap;
	}
	virtual void Draw(const TRect& aRect) const {
		TRgb back(127, 120, 120);
		TRgb front(216, 214, 214);
		CWindowGc& gc = SystemGc();
		gc.DrawBitmap(Rect(), iBitmap);

		gc.SetPenStyle(CGraphicsContext::ENullPen);
		gc.SetBrushColor(back);
		gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
		TRect progress( TPoint(progress_left, progress_top), TSize(progress_width, progress_height) );
		gc.DrawRect(progress);

		progress.SetSize( TSize( progress_width * iDone/iSteps, progress_height ) );
		gc.SetBrushColor(front);
		gc.DrawRect(progress);
	}
	virtual void StepDone() {
		++iDone;
		CWindowGc& gc = SystemGc();
		ActivateGc();

		TRgb front(216, 214, 214);
		TRect progress( TPoint(progress_left, progress_top), TSize(progress_width, progress_height) );
		progress.SetSize( TSize( progress_width * iDone/iSteps, progress_height ) );
		gc.SetPenStyle(CGraphicsContext::ENullPen);
		gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
		gc.SetBrushColor(front);
		progress.SetSize( TSize( progress_width * iDone/iSteps, progress_height ) );
		gc.DrawRect(progress);
		iEikonEnv->Flush();
		DeactivateGc();
	}

	friend class CSplashScreen;
};

CSplashScreen* CSplashScreen::NewL()
{
	return new (ELeave) CSplashScreenImpl;
}
