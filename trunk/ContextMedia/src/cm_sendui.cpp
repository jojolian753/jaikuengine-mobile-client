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

#include "cm_sendui.h"

#include <sendui.h>
#include "cm_post.h"
#include <txtrich.h>
#include "symbian_auto_ptr.h"
#include "raii_f32file.h"
#include "app_context_impl.h"
#ifdef __S60V3__
#include <cmessagedata.h>
#endif

EXPORT_C void OpenPostInMessageEditorL(MApp_context& Context,
#ifndef __S60V3__
				       CSendAppUi& aSendUi,
#else
				       CSendUi& aSendUi,
#endif
				 const CCMPost& aPost,
				 TInt	aCommandId) 
{

	TFileName body_filen;
#ifndef __S60V3__
	auto_ptr<CDesCArrayFlat> attach(new (ELeave) CDesCArrayFlat(2));
#else
	auto_ptr<CMessageData> data(CMessageData::NewL());
#endif

	if (aPost.iMediaFileName().Length()>0) {
#ifndef __S60V3__
		attach->AppendL(aPost.iMediaFileName());
#else
		data->AppendAttachmentL(aPost.iMediaFileName());
#endif
	}

#ifndef __S60V3__
	TUid mtm_uid=aSendUi.MtmForCommand(aCommandId) ;
	TSendingCapabilities cap=aSendUi.MtmCapabilitiesL(mtm_uid);
#else
	TUid mtm_uid=aSendUi.ShowSendQueryL(data.get());
	if (mtm_uid==KNullUid) return;
	TSendingCapabilities cap;
	User::LeaveIfError(aSendUi.ServiceCapabilitiesL(mtm_uid, cap));
#endif
	auto_ptr<CRichText> body(0);
	auto_ptr<CParaFormatLayer> paraf(0);
	auto_ptr<CCharFormatLayer> charf(0);

	if (cap.iFlags & TSendingCapabilities::ESupportsBodyText) {
		paraf.reset(CParaFormatLayer::NewL());

		charf.reset(CCharFormatLayer::NewL());
		body.reset(CRichText::NewL(paraf.get(), charf.get()));

		body->InsertL(body->DocumentLength(), aPost.iBodyText->iPtr);
		body->InsertL(body->DocumentLength(), CEditableText::EParagraphDelimiter);
		body->InsertL(body->DocumentLength(), aPost.iSender.iName());
#ifdef __S60V3__
		data->SetBodyTextL(body.get());
#endif
	} else {
		if (aPost.iBodyText->Value().Length()) {
			body_filen=Context.DataDir();
			body_filen.Append( _L("mediabody") );
			body_filen.Append(_L(".txt"));
			Context.Fs().Delete(body_filen);
			RAFile f;
			f.ReplaceLA(Context.Fs(), body_filen, EFileWrite|EFileShareAny);
			TPtrC8 body( (TText8*)aPost.iBodyText->Value().Ptr(), aPost.iBodyText->Value().Length()*2);
			User::LeaveIfError(f.Write(body));
#ifdef __S60V3__
			data->AppendAttachmentL(body_filen);
#else
			attach->AppendL(body_filen);
#endif
		}
	}

#ifndef __S60V3__
#  ifndef __S80__
	aSendUi.CreateAndSendMessageL(aCommandId, body.get(), attach.get(),
		KNullUid, 0, 0, EFalse);
#  else
	aSendUi.CreateAndSendMessageL(aCommandId, body.get(), attach.get(),
		KNullUid, 0, 0);
#  endif
#else
	aSendUi.CreateAndSendMessageL(mtm_uid, data.get(), KNullUid, EFalse);
#endif
}
