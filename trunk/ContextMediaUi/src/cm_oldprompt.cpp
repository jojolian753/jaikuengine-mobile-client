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

#include "break.h"
#include "cm_oldprompt.h"
#include "cl_settings.h"
#include <fbs.h>
#include "util.h"


EXPORT_C TCMOldPrompt::TCMOldPrompt(CPostStorage &aStorage, CCMNetwork &aNetwork,
				    TInt64 aParentId) : iStorage(aStorage), iNetwork(aNetwork), iParentId(aParentId)
{
}

void TCMOldPrompt::Prompt(const TDesC& FileName, MUploadCallBack* CallBack)
{
	auto_ptr<CFbsBitmap> bm;
	auto_ptr<CCMPost> b(CCMPost::NewL(0));
	TFileOpStatus ret;
	TTime t; t=GetTime();

	b->iPostId() = CPostStorage::AddMeToMediaPoolId();
	b->iParentId() = iParentId;
	b->iMediaUrl() = FileName;

	TBuf<40> mime;
	GetMimeTypeL(FileName, mime);
	

	b->iContentType() = mime;
	b->iUnreadCounter() = 1;
	b->iTimeStamp() = t;

	TBuf<50> buf;
	if ( Settings().GetSettingL(SETTING_PUBLISH_AUTHOR, buf ) && buf.Length() != 0) {
		b->iSender.iName() = buf;
	}

	iStorage.AddLocalL(b.get(), bm);

	ret = CallBack->Back(false, false, b.get());
	

	CCMPost * anotherPost = iStorage.GetByPostIdL(0, b->iPostId());
	if (ret.err == KErrNone && ret.fn.Length()>0) {
		anotherPost->iMediaUrl() = ret.fn;
		anotherPost->iMediaFileName() = ret.fn;
		CC_TRAPD(ignore, iStorage.UpdatePostL(anotherPost));
	}
	iNetwork.FetchMedia(anotherPost);
	iStorage.Release(anotherPost, 0);
}
