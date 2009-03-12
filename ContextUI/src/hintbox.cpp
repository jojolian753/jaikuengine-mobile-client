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

#include "hintbox.h"

#include "app_context.h"
#include "cbbsession.h"
#include "list.h"
#include <w32std.h>
#include "bbdata.h"
#include "bbutil.h"
#include "concretedata.h"
#include <eikenv.h>
#include "symbian_auto_ptr.h"

#define MAX_LINES 20

class CTextBoxImpl : public CTextBox {
	TRgb iBgColor, iFgColor;

	void ZeroRect() {
		SetRect( TRect(TPoint(0, 0), TSize(0, 0)) );
	}
	void ConstructL(TRgb aBackgroundColor, TRgb aForegroundColor,
			TRgb aBorderColor, TInt aBorder, TInt aMargin) {
		/* if we just CreateWindowL() we get a 'floating' window,
		   since each new window is created on top of the 
		   previous ones. No key events will be routed if
		   we don't add it to the control stack.*/
		CreateWindowL();
		/* default window covers the whole screen, we
		   don't want that in case somebody Activate()s
		   without calling SetText() */
		ZeroRect();

		iBgColor=aBackgroundColor;
		iFgColor=aForegroundColor;
		iFont=CEikonEnv::Static()->DenseFont();
		/* 1.1 times font height is a standard estimate
		   for line height when we don't know any better */
		iLineHeight=iFont->HeightInPixels()*1.1;
		iMargin=aMargin;
		iBorder=aBorder;
		iBorderColor=aBorderColor;
	}
	void Draw(const	TRect& aRect) const {
		/* we don't care if aRect is actually smaller than
		   this window:
		    a) it won't normally happen, since this will
			float on top of all other windows in the app,
			and menus and other transient windows use a
			copy of the window below to redraw
			b) if we did, we'd have to worry about how to
			draw the border correctly, instead of just
			using the pen on the rect
		*/
		CWindowGc& gc =	SystemGc();
		gc.SetPenStyle(CGraphicsContext::ESolidPen);
		gc.SetPenSize(TSize(iBorder, iBorder));
		gc.SetPenColor(iBorderColor);
		gc.SetBrushColor(iBgColor);
		gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
		gc.DrawRect(Rect());

		if (! iText || iText->Length()==0) return;

		gc.UseFont(iFont);
		TInt baseline=iFont->HeightInPixels()-iFont->DescentInPixels();
		TPoint p(iBorder+iMargin, baseline+iBorder+iMargin);
		TInt pos=0;
		gc.SetPenStyle(CGraphicsContext::ESolidPen);
		gc.SetPenColor(iFgColor);
		for (int i=0; i<iCurrentLineCount; i++) {
			/* pos will be one over length of string at end of
			   loop */
			TPtrC text=(*iText).Mid(pos, iLines[i]-pos);
			gc.DrawText(text, p);
			pos=iLines[i];

			/* we have to check whether the line was broken
			   at whitespace, and advance if so */
			if ( pos<(*iText).Length() && 
				TChar((*iText)[pos]).IsSpace() ) pos++;
			p.iY+=iLineHeight;
		}
		gc.DiscardFont();
	}
	const CFont* iFont;
	TInt iLines[MAX_LINES];
	TInt iCurrentWidth, iCurrentLineCount, iLineHeight;
	TInt iMargin, iBorder; TRgb iBorderColor;
	HBufC* iText;
	void Reset() {
		iText=0;
		ZeroRect();
	}
	void SetText(TInt aMaxWidth, HBufC* aText, TPosition aPosFrom,
			TPoint aPos) {
		iText=aText;
		if (! iText || iText->Length()==0) {
			ZeroRect();
			return;
		}

		/* Calculate the line breaks. We go one over
		 * the string length in the loop so that the 
		 * last line width is calculated.
		 */
		TInt i, w, l, previous_space=0, max_w=0, previous_w=0;
		for (i=0, w=0, l=0; i<=aText->Length() && l<MAX_LINES; i++) {
			if ( i==aText->Length() || TChar((*aText)[i]).IsSpace()) {
				previous_w=w;
				previous_space=i;
			}
			TInt this_w=0;
			if (i<aText->Length()) 
				this_w=iFont->CharWidthInPixels( (*aText)[i] );
			if (i==aText->Length() || w+this_w > aMaxWidth) {
				if (previous_space==0) {
					/* if there is no break opportunity,
					   we just cut the word */
					previous_w=w;
					previous_space=i-1;
				}
				if (previous_w > max_w) max_w=previous_w;
				iLines[l]=previous_space;
				l++;
				i=previous_space;
				previous_space=0;
				w=0;
			} else {
				w+=this_w;
			}
		}
		iCurrentLineCount=l;
		iCurrentWidth=max_w+2*iMargin+2*iBorder;
		TPoint tl; 
		TSize s(iCurrentWidth, iCurrentLineCount*iLineHeight+2*iMargin+2*iBorder);
		switch(aPosFrom) {
			case ETopLeft:
				tl=aPos;
				break;
			case ETopRight:
				tl.iX=aPos.iX-s.iWidth;
				tl.iY=aPos.iY;
				break;
			case EBottomLeft:
				tl.iX=aPos.iX;
				tl.iY=aPos.iY-s.iHeight;
				break;
			case EBottomRight:
				tl.iX=aPos.iX-s.iWidth;
				tl.iY=aPos.iY-s.iHeight;
				break;
		}
		SetRect(TRect(tl, s));
	}
	friend class CTextBox;
};

EXPORT_C CTextBox* CTextBox::NewL(TRgb aBackgroundColor, TRgb aForegroundColor,
		TRgb aBorderColor, TInt aBorder, TInt aMargin)
{
	auto_ptr<CTextBoxImpl> ret(new (ELeave) CTextBoxImpl);
	ret->ConstructL(aBackgroundColor, aForegroundColor, aBorderColor,
		aBorder, aMargin);
	return ret.release();
}

class CHintBoxImpl : public CHintBox, public MContextBase {
	CHintBoxImpl() { }
	struct THint {
		TInt iHintId;
		TInt iPriority;
		CTextBox::TPosition iPosFrom;
		TPoint iPos;
		TInt iMaxWidth;
		TInt iResourceId;
		TInt iDismissed;
		THint(TInt aHintId,	TInt aPriority,
			CTextBox::TPosition aPosFrom, 
			const class TPoint& aPos, TInt aMaxWidth,
			TInt aHintResourceId,
			TInt aDismissed) : iHintId(aHintId), iPriority(aPriority),
			iPosFrom(aPosFrom), iPos(aPos), iMaxWidth(aMaxWidth),
			iResourceId(aHintResourceId),
			iDismissed(aDismissed) { }
		THint() : iHintId(-1), iPriority(-1), iResourceId(-1) { }
		// default copy constructor and assignment are fine
	};
	CList<THint> *iHints;
	CBBSubSession* iBBSession;
	TTupleName iTupleName;
	TInt iScale;
	void ConstructL(const struct TTupleName& aTupleName, TInt aScale) {
		iHints=CList<THint>::NewL();
		iTupleName=aTupleName;
		iBBSession=BBSession()->CreateSubSessionL(0);
		iScale=aScale;
		TRgb bg(0xF0,0xE6,0x8C);
		iTextBox=CTextBox::NewL(bg, KRgbBlack, KRgbBlack, 1*iScale, 4*iScale);
		iTextBox->ActivateL();
	}
	~CHintBoxImpl() {
		delete iHints;
		delete iTextBox;
		delete iCurrentText;
		delete iBBSession;
	}
	HBufC* iCurrentText;
	THint iCurrentHint;
	CTextBox* iTextBox;
	virtual void AddHintL(TInt aHintId, TInt aPriority,
		CTextBox::TPosition aPosFrom, const class TPoint& aPos,
		TInt aMaxWidth,
		TInt aHintResourceId) {
			TBuf<15> num; num.AppendNum(aHintId);
			TInt dismissed=0;
			{
				MBBData* data=0;
				TRAPD(err, iBBSession->GetL(iTupleName, num, data));
				bb_auto_ptr<MBBData> holder(data);
				if (err==KErrNone) {
					TBBInt* b=bb_cast<TBBInt>(data);
					if (b!=0) {
						dismissed=(*b)();
					}
				} else if (err!=KErrNotFound){
					User::Leave(err);
				}
			}
			iHints->AppendL(THint(aHintId, aPriority, aPosFrom, aPos,
				aMaxWidth, aHintResourceId,	dismissed));
		}
		virtual void ShowHintIfNotDismissed(TInt aHintId) {
			TBool changed=EFalse;
			if (aHintId!=iCurrentHint.iHintId) {
				CList<THint>::Node* n=0;
				changed=ETrue;
				TBool found=EFalse;
				for (n=iHints->iFirst; n; n=n->Next) {
					if (n->Item.iHintId==aHintId) {
						if (n->Item.iPriority < iCurrentHint.iPriority) return;
						iCurrentHint=n->Item;
						found=ETrue;
						break;
					}
				}
				if (! found) iCurrentHint.iHintId=-1;
			}
			if (iCurrentHint.iHintId==-1 || iCurrentHint.iDismissed) {
				iTextBox->Reset();
				return;
			}
			if (changed) {
				delete iCurrentText; iCurrentText=0;
				iCurrentText=CEikonEnv::Static()->AllocReadResourceL(iCurrentHint.iResourceId);
			}
			iTextBox->SetText(iCurrentHint.iMaxWidth, iCurrentText,
				iCurrentHint.iPosFrom, iCurrentHint.iPos);
			iTextBox->DrawNow();
		}
		virtual void DontShow() {
			iTextBox->Reset();
			iCurrentHint=THint();
		}
		virtual void DismissHint(TInt aHintId) {
			_LIT(KB, "b");
			if (iCurrentHint.iHintId==aHintId) {
				iTextBox->Reset();
				iCurrentHint=THint();
			}
			TInt dismissed;
			CList<THint>::Node* n=0;
			for (n=iHints->iFirst; n; n=n->Next) {
				if (n->Item.iHintId==aHintId) {
					/* don't increment the dismissed count here
					   only increment between instances
					   this way the caller doesn't have to keep
					   track of 'really how many times the user
					   has used this feature' */
					dismissed=n->Item.iDismissed+1;
					break;
				}
			}
			TTime expires=GetTime();
			if (dismissed < 2) {
				expires+=TTimeIntervalDays(2);
			} else if (dismissed < 3) {
				expires+=TTimeIntervalDays(7);
			} else if (dismissed < 5) {
				expires+=TTimeIntervalDays(3*7);
			} else if (dismissed==5) {
				expires=Time::MaxTTime();
			} else {
				return;
			}
			TBuf<15> num; num.AppendNum(aHintId);
			TBBInt b(dismissed, KB);
			TRAPD(err, iBBSession->PutL(iTupleName, num, &b, expires));
		}

		friend class CHintBox;
};

EXPORT_C CHintBox* CHintBox::NewL(const struct TTupleName& aTupleName,
	TInt aScale)
{
	auto_ptr<CHintBoxImpl> ret(new (ELeave) CHintBoxImpl);
	ret->ConstructL(aTupleName, aScale);
	return ret.release();
}
