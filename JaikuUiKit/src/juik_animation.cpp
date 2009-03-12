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

#include "juik_animation.h"

#include "app_context.h"
#include "break.h"
#include "symbian_auto_ptr.h"

class CJuikAnimationImpl : public CJuikAnimation, public MContextBase
{
public:

	static CJuikAnimationImpl* NewL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CJuikAnimationImpl"), _CL("NewL"));
		auto_ptr<CJuikAnimationImpl> self( new (ELeave) CJuikAnimationImpl );
		self->ConstructL();
		return self.release();
	}

	~CJuikAnimationImpl()
	{
		if ( iPeriodic )
			iPeriodic->Cancel();
		delete iPeriodic;
	}

	
	void ConstructL()
	{
		CALLSTACKITEMSTATIC_N(_CL("CJuikAnimationImpl"), _CL("ConstructL"));
		iPeriodic = CPeriodic::NewL( EPriorityHigh );
	}
	
	TTime Now()
	{
		TTime now;
		now.UniversalTime();
		return now;
	}
	
	void StartL(MAnimated& aAnimated, TTimeIntervalMicroSeconds aDuration) 
	{
		CALLSTACKITEM_N(_CL("CJuikAnimationImpl"), _CL("StartL"));
		iAnimated = &aAnimated;
		iDuration = aDuration;
		iDurationR = I64REAL(iDuration.Int64());
		iProgress = 0.0;
		iInterval = TTimeIntervalMicroSeconds32( 20 * 1000 ); // 1000 * 1000 / 20
		iStarted = Now();

		iPeriodic->Start( iInterval, iInterval, TCallBack( DoStepCallbackL, this ) );
	}

	void Stop()
	{
		TBool active = iPeriodic->IsActive();
		iPeriodic->Cancel();
		if (active)
			{
				iAnimated->FinishedL(iProgress);
			}
	}


	static TInt DoStepCallbackL(TAny* aObj)
	{
		CJuikAnimationImpl* anim = static_cast<CJuikAnimationImpl*>(aObj);
		return anim->DoStepL();
	}

	TInt DoStepL()
	{
		CALLSTACKITEM_N(_CL("CJuikAnimationImpl"), _CL("DoStepL"));
		TTime now = Now();

		if ( now < iStarted + iDuration )
			{
				TTimeIntervalMicroSeconds done = now.MicroSecondsFrom( iStarted );
				TReal doneR = I64REAL( done.Int64() ); 
				iProgress = doneR / iDurationR;
				iAnimated->ProgressL( iProgress );
			}
		else
			{
				iProgress = 1.0;
				iAnimated->ProgressL( iProgress );
 				Stop();
			}
		return 1;
	}

	MAnimated* iAnimated;
		
	TTime iStarted;
	TTimeIntervalMicroSeconds iDuration;
	TReal iDurationR;
	TReal iProgress;

	CPeriodic* iPeriodic;
	TTimeIntervalMicroSeconds32 iInterval;
};


EXPORT_C CJuikAnimation* CJuikAnimation::NewL()
{
	CALLSTACKITEMSTATIC_N(_CL("CJuikAnimation"), _CL("NewL"));
	return CJuikAnimationImpl::NewL();
}

