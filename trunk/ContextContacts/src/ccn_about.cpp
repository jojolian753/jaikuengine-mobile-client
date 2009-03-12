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

#include "ccn_about.h"

#include <contextcontacts.rsg>

#include "app_context.h"
#include "cn_datacounter.h"
#include "ccu_buildinfo.h"
#include "csd_buildinfo.h"
#include "reporting.h"

#include <StringLoader.h>
#include <aknmessagequerydialog.h> 

static TInt ShowMessageDialogWithHeaderL(TInt aDialog, TInt aHeaderTextResource, TDesC& aBodyText)
{	
	auto_ptr<HBufC> title( StringLoader::LoadL( aHeaderTextResource ) );
	TPtrC titleP( *title );
	
	auto_ptr<CAknMessageQueryDialog> note( CAknMessageQueryDialog::NewL(aBodyText) );
	note->SetHeaderTextL(titleP);
	TInt result = note->ExecuteLD(aDialog);
	note.release();
	return result;
}

class CAboutUiImpl : public CAboutUi, public MContextBase, 
					 public MDataCounterObserver
{
public:
	void ConstructL()
	{
		CALLSTACKITEM_N(_CL("CAboutUiImpl"), _CL("ConstructL"));
		iDataCounters=CDataCounterReader::NewL(*this);
	}
	
	virtual ~CAboutUiImpl() 
	{
		CALLSTACKITEM_N(_CL("CAboutUiImpl"), _CL("~CAboutUiImpl"));
		delete iDataCounters;
	}
	
	
	virtual void CountersChanged(TInt aReadCounter, TInt aWriteCounter)
	{
		CALLSTACKITEM_N(_CL("CAboutUiImpl"), _CL("CountersChanged"));
		iReadDataCounter = aReadCounter;
		iWriteDataCounter = aWriteCounter;
	}

	virtual void AppendNetworkTraffic(TDes& aBuf)
	{
		CALLSTACKITEM_N(_CL("CAboutUiImpl"), _CL("AppendNetworkTraffic"));
		TReal dataTransferred = iReadDataCounter + iWriteDataCounter; 
		dataTransferred /= 1024.0;
		TBuf<20> value;
		TRealFormat f; 
		f.iTriLen=0;
		f.iPoint='.'; 
		f.iPlaces=2;
		f.iType=KRealFormatFixed;
		value.Num( dataTransferred, f);
		
		aBuf.Append( value );
		aBuf.Append( _L(" kb") );
	}
	
	virtual void ShowNoteL()
	{		
		CALLSTACKITEM_N(_CL("CAboutUiImpl"), _CL("ShowNoteL"));
		
		TBuf<200> msg; 
		msg.Append(_L("Data transferred: "));
		AppendNetworkTraffic( msg );
		msg.Append( _L("\n\n") );

		TBBBuildInfo info(_L("buildinfo"));
		BuildInfo::ReadL(info);
		auto_ptr<HBufC> buildStr( BuildInfo::UserStringL(info) );
		msg.Append(*buildStr );
		
		ShowMessageDialogWithHeaderL(R_EMPTYBACK_QUERY_DIALOG, 
									 R_TEXT_ABOUT_JAIKU_HEADER,
									 msg);
	}


private:
	CDataCounterReader* iDataCounters;

	TInt iReadDataCounter;
	TInt iWriteDataCounter;
	
};

CAboutUi* CAboutUi::NewL()
{
	auto_ptr<CAboutUiImpl> self( new (ELeave) CAboutUiImpl);
	self->ConstructL();
	return self.release();
}
