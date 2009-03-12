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

#include "cs_music.h"

#ifdef __S60V3__
#include "app_context.h"
#include <centralrepository.h>
#include "log_base_impl.h"
#include <e32property.h>
#include "timeout.h"
#include "csd_feeditem.h"
#include "cc_uuid.h"
#include "cl_settings.h"
#include "csd_clienttuples.h"
#include "connectioninit.h"

_LIT(KMusic, "music");
_LIT(KValue, "value");

class CCenRepMusicImpl: public CCenRepMusic, public MTimeOut {
public:
	CCenRepMusicImpl(MApp_context& aAppContext) : 
		CCenRepMusic(aAppContext), iStringValue(KValue) { }

	//CRepository* iRepository;
	RProperty iProperty; TBool iPropertyIsOpen;
	TBuf<128> iArtist, iTrack;
	TBuf<128> iPrevArtist, iPrevTrack;
	CBBString	iStringValue;
	CTimeOut*	iTimeOut;
			
	void ConstructL() {
		Mlog_base_impl::ConstructL();
		CActiveScheduler::Add(this);
		iTimeOut=CTimeOut::NewL(*this);
		TRAPD(err, StartL());
		if (err!=KErrNone) CheckedRunError(err);
		Settings().GetSettingL( SETTING_JABBER_NICK, iNick);
		TInt at = iNick.Locate('@');
		if (at >= 0) iNick = iNick.Left(at);		
		iUuidGenerator = CUuidGenerator::NewL(iNick, KUidContextSensors, 62);
	}
	CUuidGenerator* iUuidGenerator;
	TBuf<100> iNick;
	void StartL() {
		//iRepository=CRepository::NewL(TUid::Uid(0x102072C3));
		User::LeaveIfError(iProperty.Attach(TUid::Uid(0x102072C3),
			0x2));
		iPropertyIsOpen=ETrue;
		GetValuesL();
		ListenL();
	}
	TInt CheckedRunError(TInt aError) {
		TBuf<50> msg=_L("run error ");
		msg.AppendNum(aError);
		post_error(msg, aError);
		return KErrNone;
	}
	TBool iPosted;
	
	void expired(CBase*) {
		if (iArtist.Length()==0 || iPosted) {
			iPosted=ETrue;
			return;
		}
		refcounted_ptr<CBBFeedItem> item( new (ELeave) CBBFeedItem );
		item->iContent.Append( _L("Listening to ") );
		item->iContent.Append( iTrack );
		
		if (iArtist.Length()>0) {
			item->iContent.Append( _L(" by ") );
			item->iContent.Append( iArtist );
		}
		{
			//item->iContent.Append(_L(" http://musica.tre.it/cerca/?&q="));
			item->iContent.Append(_L(" http://music.tre.it/p-music/H3GMSCHP/cerca/?q="));
			auto_ptr<HBufC> urlbuf( HBufC::NewL( (iArtist.Length()+iTrack.Length()+2)*3) );
			TPtr p=urlbuf->Des();
			AppendUrlEncoded(p, iArtist);
			//AppendUrlEncoded(p, _L(" "));
			//AppendUrlEncoded(p, iTrack);
			item->iContent.Append(p);
			item->iContent.Append(_L("&w=nome_artista"));
		}
		
		
		iUuidGenerator->MakeUuidL(item->iUuid.iValue);
		TTime now=GetTime();
		item->iCreated = now;
		item->iAuthorNick() = iNick;
		TTime expires = now; expires+=TTimeIntervalDays(2);
		BBSession()->PutRequestL( KOutgoingFeedItem, KNullDesC, item.get(), expires, KOutgoingTuples);
		iPosted=ETrue;
	}
	void GetValuesL() {
		//User::LeaveIfError(iRepository->Get(0x00000002, iTrack));
		//User::LeaveIfError(iRepository->Get(0x00000005, iArtist));
		
		TInt err=iProperty.Get(TUid::Uid(0x102072C3), 0x2, iTrack);
		if (err==KErrNone) {
			err=iProperty.Get(TUid::Uid(0x102072C3), 0x5, iArtist);
		}
		if (err!=KErrNone) {
			iTrack.Zero();
			iArtist.Zero();
			TBuf<50> msg=_L("Error getting value ");
			msg.AppendNum(err);
			post_error(msg, err);
			iTimeOut->Reset();
			return;
		}
		if (iPrevTrack != iTrack || iPrevArtist != iArtist) {
			iStringValue.Zero();
			iStringValue.Append(iArtist);
			iStringValue.Append(_L(": "));
			iStringValue.Append(iTrack);
			post_new_value(&iStringValue);
			iPrevTrack=iTrack;
			iPrevArtist=iArtist;
			iPosted=EFalse;
		}
	}
	void CheckedRunL() {
		if (iStatus.Int()!=KErrNone) {
			//delete iRepository; iRepository=0;
			//iRepository=CRepository::NewL(TUid::Uid(0x102072C3));
			iProperty.Close(); iPropertyIsOpen=EFalse;
			StartL();
			return;
		}
		iTimeOut->Wait(10);
		GetValuesL();
		ListenL();
	}
	void DoCancel() {
		//iRepository->NotifyCancel(0, 0);
		iProperty.Cancel();
	}
	void ListenL() {
		//User::LeaveIfError(iRepository->NotifyRequest(0, 0, iStatus));
		iProperty.Subscribe(iStatus);
		SetActive();
	}
	~CCenRepMusicImpl() {
		Cancel();
		//delete iRepository;
		if (iPropertyIsOpen) iProperty.Close();
		delete iUuidGenerator;
	}
};

CCenRepMusic::CCenRepMusic(MApp_context& aAppContext) : CCheckedActive(CActive::EPriorityStandard,
	_L("CCenRepMusic")), Mlog_base_impl(aAppContext,
		KMusic, KMusicTuple, 10*60) { }
	
	
#include "app_context_impl.h"
EXPORT_C CCenRepMusic* CCenRepMusic::NewL()
{
	auto_ptr<CCenRepMusicImpl> ret(new (ELeave) CCenRepMusicImpl(*::GetContext()));
	ret->ConstructL();
	return ret.release();
}

#endif
