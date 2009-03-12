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

#include "cu_sendmsg.h"

#include "break.h"
#include "app_context.h"
#include "app_context_impl.h"
#include "symbian_auto_ptr.h"
#include "raii_f32file.h"
#include "contextui.hrh"

#include <bautils.h>
#include <e32base.h>
#include <sendui.h>
#include <txtrich.h>
#ifndef __S60V3__
#include  <SENDNORM.RSG>
#endif

EXPORT_C void SendMessageWithSendUiL(const TDesC& aTitle, const TDesC& aMessage)
{
#ifndef __S60V3__
	//FIXME3RD
	auto_ptr<CSendAppUi> iSendUi(CSendAppUi::NewL(EContextUICmdSendUi, 0));
	TInt flags=TSendingCapabilities::ESupportsBodyText;
	TSendingCapabilities c( 0, 0, flags);

	auto_ptr<CParaFormatLayer> paraf(0);
	auto_ptr<CCharFormatLayer> charf(0);
	auto_ptr<CRichText> body(0);
	auto_ptr<CDesCArrayFlat> attach(new (ELeave) CDesCArrayFlat(1));
	paraf.reset(CParaFormatLayer::NewL());
	charf.reset(CCharFormatLayer::NewL());

	body.reset(CRichText::NewL(paraf.get(), charf.get()));
	body->InsertL(0, aMessage);
	iSendUi->CreateAndSendMessagePopupQueryL(aTitle, c, body.get(), 
		0, KNullUid, 0, 0, 0, EFalse);
#else
	User::Leave(KErrNotSupported);
#endif
}
