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

#include "settings.h"
#include "settings_impl.h"
#include "symbian_auto_ptr.h"
#include "db.h"
#include "db.inl"
#include "list.h"
#include <bautils.h>

EXPORT_C CSettings* CSettings::NewL(MApp_context& Context, 
				    const MDefaultSettings& DefaultSettings, const TDesC& SettingName, bool ReadOnly)
{
	auto_ptr<CSettings> ret(new (ELeave) CSettings(Context, DefaultSettings, ReadOnly));
	ret->ConstructL(SettingName);
	return ret.release();
}

CSettings::~CSettings()
{
	delete iListeners;
	delete iIntStore;
	delete iTimeStore;
	delete iDesStore;
	delete iDes8Store;
	delete iDb;
}

bool CSettings::GetSettingL(TInt Setting, TDes& Value)
{
	if(iDesStore->GetValueL(Setting, Value)) return true;
	return iDefaultSettings.GetDefaultL(Setting, Value);
}

bool CSettings::GetSettingL(TInt Setting, TDes8& Value)
{
	if(iDes8Store->GetValueL(Setting, Value)) return true;
	return iDefaultSettings.GetDefaultL(Setting, Value);
}

bool CSettings::GetSettingL(TInt Setting, TInt& Value)
{
	if(iIntStore->GetValueL(Setting, Value)) return true;
	return iDefaultSettings.GetDefaultL(Setting, Value);
}

bool CSettings::GetSettingL(TInt Setting, TTime& Value)
{
	if(iTimeStore->GetValueL(Setting, Value)) return true;
	return iDefaultSettings.GetDefaultL(Setting, Value);
}

void CSettings::WriteSettingL(TInt Setting, const TDesC& Value)
{
	if (iReadOnly) User::Leave(-1026);

	iDesStore->SetValueL(Setting, Value);
	NotifyOfChange(Setting);
}

void CSettings::WriteSettingL(TInt Setting, const TDesC8& Value)
{
	if (iReadOnly) User::Leave(-1026);

	iDes8Store->SetValueL(Setting, Value);
	NotifyOfChange(Setting);
}

void CSettings::WriteSettingL(TInt Setting, const TInt& Value)
{
	if (iReadOnly) User::Leave(-1026);

	iIntStore->SetValueL(Setting, Value);
	NotifyOfChange(Setting);
}

void CSettings::WriteSettingL(TInt Setting, const TTime& Value)
{
	if (iReadOnly) User::Leave(-1026);

	iTimeStore->SetValueL(Setting, Value);
	NotifyOfChange(Setting);
}

CSettings::CSettings(MApp_context& Context, const MDefaultSettings& DefaultSettings, bool ReadOnly) : MContextBase(Context),
	iReadOnly(ReadOnly), iDefaultSettings(DefaultSettings)
{
	iDb=0;
	iIntStore=0;
	iTimeStore=0;
	iDesStore=0;
	iDes8Store=0;
	iListeners=0;
}

void ListDeletor(void* p)
{
	CList<MSettingListener*>* l=(CList<MSettingListener*>*)p;
	delete l;
}

void CSettings::ConstructL(const TDesC& SettingName)
{
	_LIT(db_base, "SETTINGS");
	TBuf<100> db;
	db.Append(db_base);
	db.Append(SettingName);
	TFileName oldfilen, newfilen;
	oldfilen.Format(_L("%S%S.db"), &AppDir(), &db_base);
	newfilen.Format(_L("%S%S.db"), &AppDir(), &db);
	if (BaflUtils::FileExists(Fs(), oldfilen)) {
		User::LeaveIfError(Fs().Rename(oldfilen, newfilen));
	}

	iListeners=CGenericIntMap::NewL();
	iListeners->SetDeletor(ListDeletor);

	if (!iReadOnly) {
		iDb=CDb::NewL(AppContext(), db, EFileRead|EFileWrite|EFileShareAny );
	} else {
		iDb=CDb::NewL(AppContext(), db, EFileRead|EFileShareAny );
	}

	iDesStore=CSingleColDb<TDesC, TDes>::NewL(AppContext(), iDb->Db(), _L("DES"));
	iDes8Store=CSingleColDb<TDesC8, TDes8>::NewL(AppContext(), iDb->Db(), _L("DES8"));
	iIntStore=CSingleColDb<TInt>::NewL(AppContext(), iDb->Db(), _L("INT"));
	iTimeStore=CSingleColDb<TTime>::NewL(AppContext(), iDb->Db(), _L("TIME"));
}

void CSettings::NotifyOnChange(TInt Setting, MSettingListener* Listener)
{
	if (iReadOnly) User::Leave(-1026);

	CList<MSettingListener*>* l=(CList<MSettingListener*>*)iListeners->GetData(Setting);
	if (!l) {
		l=CList<MSettingListener*>::NewL();
		iListeners->AddDataL(Setting, (void*)l);
	}
	l->AppendL(Listener);
}

void CSettings::CancelNotifyOnChange(TInt Setting, MSettingListener* Listener)
{
	CList<MSettingListener*>* l=(CList<MSettingListener*>*)iListeners->GetData(Setting);
	if (!l) {
		return;
	}
	CList<MSettingListener*>::Node *i, *prev, *tmp;
	i=l->iFirst; prev=0;
	while (i) {
		if (i->Item==Listener) {
			if (prev) {
				prev->Next=i->Next;
			} else {
				l->iFirst=i->Next;
			}
			if (i==l->iCurrent) l->iCurrent=prev;
			tmp=i;
			i=i->Next;
			delete tmp;
		} else {
			prev=i;
			i=i->Next;
		}
	}
}

void CSettings::NotifyOfChange(TInt Setting)
{
	CList<MSettingListener*>* l=(CList<MSettingListener*>*)iListeners->GetData(Setting);
	if (!l) {
		return;
	}
	CList<MSettingListener*>::Node *i;
	i=l->iFirst;
	while (i) {
		i->Item->SettingChanged(Setting);
		i=i->Next;
	}
}

EXPORT_C bool TNoDefaults::GetDefaultL(TInt , TDes& ) const
{
	return false;
}

EXPORT_C bool TNoDefaults::GetDefaultL(TInt , TDes8& ) const
{
	return false;
}

EXPORT_C bool TNoDefaults::GetDefaultL(TInt , TInt& ) const
{
	return false;
}

EXPORT_C bool TNoDefaults::GetDefaultL(TInt , TTime& ) const
{
	return false;
}
