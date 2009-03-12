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

#include "cu_autosettings.h"

#include "cu_common.h"
#include "cu_resourcereaders.h"
#include <contextui.rsg>

#include "app_context.h"
#include "symbian_auto_ptr.h"
#include "cl_settings.h"
#include "cl_settings_impl.h"

#include <aknsettingitemlist.h>  
#include <akntextsettingpage.h>
#include <barsread.h> 
#include <commdb.h>

//HBufC* ReadHBufCL(TResourceReader& aReader, 


class CAutoSettingsImpl : public CAutoSettings, 
						  public MContextBase
{
public:
	CAutoSettingsImpl(TBool aOnlyContextLogPublishing): 
		iOnlyContextLogPublishing(aOnlyContextLogPublishing) {}

	void ConstructL()
	{
		iEikEnv = CEikonEnv::Static();
		for (TInt i=0; i < SETTINGS_COUNT; i++)
			iItems[i] = NULL;
		iResourceFile=LoadSystemResourceL( iEikEnv, _L("contextui"));

	}

	void ActivateL()
	{
		ReadSettingItemResourcesL();
	}

	void DeactivateL()
	{
		for (int i=0; i<SETTINGS_COUNT; i++) {
			if (! iData[i]) continue;
			const TSettingItem s=TClSettings::GetKClSettings(i);
			switch (s.iDatatype) {
			case ::EAP:
			case ::EInt:
			case ::EBool:
			case ::EEnum:
				delete (TInt*)iData[i];
				break;
			case ::EString:
				delete (TBuf<256>*)iData[i];
				break;
			}
			iData[i] = NULL;
		}
		delete iResources;
		iResources = NULL;
		delete iDefaultEmptyText;
		iDefaultEmptyText=NULL;

	}

 	~CAutoSettingsImpl()
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("~CAutoSettingsImpl"));
		DeactivateL();
		if (iResourceFile) iEikEnv->DeleteResourceFile(iResourceFile);
	}

	void LoadSettingL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("LoadSettingL"));
		TInt i = aIndex;
		
		TBuf<256> prev_string; TInt prev_int;
		
		
		if (! DataAt(i)) return;
		
		TSettingItem s=TClSettings::GetKClSettings(i);
		switch (s.iDatatype) {
		case ::EAP:
		case ::EInt:
		case ::EBool:
		case ::EEnum:
			{
				TInt *v=(TInt*)(DataAt(i));
				if (Settings().GetSettingL(i, prev_int)) {
					if (i==SETTING_MEDIA_UPLOAD_ENABLE && iOnlyContextLogPublishing) {
						if (prev_int>1) {
							prev_int=1;
							Settings().WriteSettingL(i, prev_int);
						}
					}
					*v=prev_int;
				}
			}
			break;
		case ::EString:
			{
				TBuf<256> *v=(TBuf<256>*)(DataAt(i));
				if (Settings().GetSettingL(i, prev_string)) *v=prev_string;
			}
			break;
		}
	}
	
	void StoreSettingL(TInt aIndex)
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("StoreSettingL"));
		TInt i = aIndex;
		
		TBuf<256> prev_string; TInt prev_int;

		if (! DataAt(i)) return;
		TSettingItem s=TClSettings::GetKClSettings(i);
		switch (s.iDatatype) {
		case ::EAP:
		case ::EInt:
		case ::EBool:
		case ::EEnum:
			{
				TInt *v=(TInt*)(DataAt(i));
				if (!Settings().GetSettingL(i, prev_int) || prev_int!=*v)
					Settings().WriteSettingL(i, *v);
			}
			break;
		case ::EString:
			{
				TBuf<256> *v=(TBuf<256>*)(DataAt(i));
				if (!Settings().GetSettingL(i, prev_string) || prev_string.Compare(*v))
					Settings().WriteSettingL(i, *v);
			}
			break;
		}
	}
	
	CAknSettingItem* CreateSettingItemL( TInt identifier )
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("CreateSettingItemL"));
		CAknSettingItem * settingItem = NULL;
		
		const TSettingItem i = TClSettings::GetKClSettings(identifier);
		
		switch (i.iDatatype) {
		case ::EAP:
			{
				TInt *p=new (ELeave) TInt;
				*p=1;
				if (i.iDefaultExists) *p=i.iIntDefault;
				iData[identifier]=p;
				CAknSettingItem * item = (new (ELeave) CAknEnumeratedTextPopupSettingItem(identifier, *p ));
				settingItem =  item;
				iItems[identifier] = item;
				break;
			}
		case ::EString:
			{
				TBuf<256>* p=new (ELeave) TBuf<256>;
				iData[identifier]=p;
				if (i.iDefaultExists && i.iStringDefault) *p=TPtrC((TText*)i.iStringDefault);
				CAknSettingItem * item = NULL;
				
				// Hack to make password secret.
				// Should enable 
				if ( i.iSettingNo == SETTING_JABBER_PASS )
					{
						item =  new (ELeave) CAknPasswordSettingItem(identifier, 
																	 CAknPasswordSettingItem::EAlpha,
																	 *p);
						
					}
				else
					{
						item = new (ELeave) CAknTextSettingItem(identifier, *p);
						/* !Allowing empty setting! */
						TInt flags=item->SettingPageFlags();
						item->SetSettingPageFlags( flags | CAknTextSettingPage::EZeroLengthAllowed );
					}
				settingItem = item;
				iItems[identifier] = item;
				break;
			}
		case ::EBool:
			{
				TInt *p=new (ELeave) TInt;
				*p=1;
				if (i.iDefaultExists) *p=i.iIntDefault;
				iData[identifier]=p;
				CAknSettingItem * item =new (ELeave) CAknBinaryPopupSettingItem(identifier, *p );
				settingItem =  item;
				iItems[identifier] =  item; 
				break;
			}
		case ::EEnum:
		{
			TInt *p=new (ELeave) TInt;
			*p=1;
			if (i.iDefaultExists) *p=i.iIntDefault;
			iData[identifier]=p;
			CAknSettingItem * item =new (ELeave) CAknEnumeratedTextPopupSettingItem(identifier, *p );
			settingItem =  item;
			iItems[identifier] =  item; 
			break;
		}
		case ::EInt:
			{
				TInt *p=new (ELeave) TInt;
				*p=0;
				if (i.iDefaultExists) *p=i.iIntDefault;
				iData[identifier]=p;
				CAknSettingItem * item = new (ELeave) CAknIntegerEdwinSettingItem(identifier, *p );
				settingItem =  item;
				iItems[identifier] =  item;
				break;
			}
		default:
			break;
		}
		if (!settingItem) {
			User::Leave(-1027);
		}
		return settingItem;
	}

	virtual void UpdateSettingItemsL() 
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("UpdateSettingItemsL"));
		SetListData();
		UpdateSettings();
	}


	virtual void SetItemDataL(TInt aIndex )
	{
		TInt i = aIndex; 

		if ( ! iItems[i] ) return;
		TSettingItem s=TClSettings::GetKClSettings(i);
		switch(s.iDatatype) {
			case EEnum:
				if ( iOnlyContextLogPublishing && s.iSettingNo == SETTING_MEDIA_UPLOAD_ENABLE ) {
					CAknEnumeratedTextPopupSettingItem *list=
						(CAknEnumeratedTextPopupSettingItem *)iItems[i];
					
					auto_ptr<CArrayPtr< CAknEnumeratedText > > enumarr(
																	   new (ELeave) CArrayPtrFlat< CAknEnumeratedText >(2));
					auto_ptr<CArrayPtr< HBufC > > popuparr( 
														   new (ELeave) CArrayPtrFlat< HBufC >(2));
					
					TInt resource=R_NO;
					for (int j=0; j<2; j++) {
						auto_ptr<HBufC> text(iEikEnv->AllocReadResourceL(resource));
						auto_ptr<HBufC> popup(text->AllocL());
						auto_ptr<CAknEnumeratedText> item(
														  new (ELeave) CAknEnumeratedText(TInt(j), text.get()));
						text.release();
						enumarr->AppendL(item.get());
						item.release();
						popuparr->AppendL(popup.get());
						popup.release();
						resource=R_YES;
					}
					
					list->SetEnumeratedTextArrays(enumarr.release(), popuparr.release());
					list->HandleTextArrayUpdateL();
					list->UpdateListBoxTextL ();
				}
				break;
			case EAP:
				{
					CAknEnumeratedTextPopupSettingItem *list=(CAknEnumeratedTextPopupSettingItem *)iItems[i];
					TInt *list_id=(TInt*)iData[i];
					auto_ptr<CCommsDatabase> db(CCommsDatabase::NewL(EDatabaseTypeIAP));
#if defined(__S60V3__) && defined(__WINS__)
					db->ShowHiddenRecords();
#endif
					CCommsDbTableView* iViewP=db->OpenTableLC(TPtrC(IAP));
					CleanupStack::Pop();
					auto_ptr<CCommsDbTableView> iView(iViewP);
					
					auto_ptr<CArrayPtr< CAknEnumeratedText > > enumarr(
																	   new (ELeave) CArrayPtrFlat< CAknEnumeratedText >(4));
					auto_ptr<CArrayPtr< HBufC > > popuparr( 
														   new (ELeave) CArrayPtrFlat< HBufC >(4));
					
					TInt err;
					TUint32 id=-1;
#ifndef __JAIKU__
					{
						TBuf<30> KNoAp=_L("No connection selected");
						auto_ptr<HBufC> text(KNoAp.AllocL());
						auto_ptr<HBufC> popup(KNoAp.AllocL());
						auto_ptr<CAknEnumeratedText> item(new (ELeave) CAknEnumeratedText(-1, text.get()));
						text.release();
						enumarr->AppendL(item.get());
						item.release();
						popuparr->AppendL(popup.get());
						popup.release();
					}
#endif
					bool seen=false;
					TUint first_id=-1;
					err=iView->GotoFirstRecord();
					while( err == KErrNone ) {
						auto_ptr<HBufC> text(HBufC::NewL(KCommsDbSvrMaxColumnNameLength));
						TPtr ptr(text->Des());
#if defined(__S60V3__) && !defined(__WINS__)
						iView->ReadTextL(TPtrC(IAP_BEARER_TYPE), ptr);
						//						if (ptr.FindF(_L("LAN")) != KErrNotFound) goto skip;
#endif
						
						{
							if (first_id==-1) first_id=id;
							iView->ReadUintL(TPtrC(COMMDB_ID), id);
							if ( (TInt)id==*list_id) { seen=true; }
							ptr.Set(text->Des());
							iView->ReadTextL(TPtrC(COMMDB_NAME), ptr);
							auto_ptr<HBufC> popup(text->AllocL());
							auto_ptr<CAknEnumeratedText> item(new (ELeave) CAknEnumeratedText(TInt(id), text.get()));
							text.release();
							enumarr->AppendL(item.get());
							item.release();
					popuparr->AppendL(popup.get());
					popup.release();
						}
					skip:
				err=iView->GotoNextRecord();
			}
					if (!seen) *list_id=first_id;
#ifdef __JAIKU__
					if (first_id==-1) {
						TBuf<30> KNoAp=_L("No connection selected");
				auto_ptr<HBufC> text(KNoAp.AllocL());
				auto_ptr<HBufC> popup(KNoAp.AllocL());
				auto_ptr<CAknEnumeratedText> item(new (ELeave) CAknEnumeratedText(-1, text.get()));
				text.release();
				enumarr->AppendL(item.get());
				item.release();
				popuparr->AppendL(popup.get());
				popup.release();
			}
#endif
					list->SetEnumeratedTextArrays(enumarr.release(), popuparr.release());
			list->HandleTextArrayUpdateL();
			list->UpdateListBoxTextL ();
				}
				break;
			default:
			break;
			}
}
	
	
	void SetListData()
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("SetListData"));
		
		for (int i=0; i< SETTINGS_COUNT; i++) {
			SetItemDataL(i);
		}
	}

	

	void UpdateSettingItemL(TInt aIndex)
	{
		TInt i = aIndex;
		if ( ! iItems[i] ) return;
		CAknSettingItem* list= iItems[i];
		list->LoadL();
		list->UpdateListBoxTextL();
	}
	void UpdateSettings()
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("UpdateSettings"));
		
		for (int i=0; i< SETTINGS_COUNT; i++) {
			UpdateSettingItemL( i );
		}
	}
	

	TAny* DataAt(TInt aId) const
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("DataAt"));
		if ( 0 <= aId && aId < SETTINGS_COUNT )
			return iData[aId];
		
		Bug( _L("Index out of range") ).Raise();
		return NULL; // to please compiler
	}




private: // for MSettingFactory 

	virtual class CAknSettingItem* CreateItemL(TInt aId, TInt aOrdinal)
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("CreateItemL") );
		if ( ! iResources )
			Bug( _L("Resources not read") ).Raise();
		
		CAknSettingItem* item = CreateSettingItemL( aId );
		ResourceReaders::CSettingItem* r = iResources->Find( aId );
		if ( ! r )
			{
				Bug( _L("Resource not found for identifier %d in ordinal %d") ).TechMsg( aId, aOrdinal ).Raise();
				return NULL;
			}

		item->SetEmptyItemTextL( r->EmptyItemText() );
		item->SetCompulsoryIndTextL( r->CompulsoryText() );
		item->ConstructL( EFalse, aOrdinal, r->Name(), NULL, r->iSettingPageResource, r->iType, r->iSettingEditorResource, r->iAssociatedResource );
		
		// clumsy and stupid, but 
		LoadSettingL( aId );
		SetItemDataL( aId );
		UpdateSettingItemL( aId );
		return iItems[aId];
	}
	

	virtual void StoreSettingL(TInt aId, TInt aOrdinal) 
	{
		StoreSettingL( aId );
	}


	ResourceReaders::CSettingItemList* iResources;
	
	HBufC* iDefaultEmptyText;
	
	virtual void ReadSettingItemResourcesL()
	{
		CALLSTACKITEM_N(_CL("CAutoSettingsImpl"), _CL("ReadSettingItemResourcesL") );


		TResourceReader reader;
		iEikEnv->CreateResourceReaderLC( reader,  R_CL_SETTINGS_LIST  );
		iResources = new (ELeave) ResourceReaders::CSettingItemList;
		iResources->ReadResourceL( reader );

		iDefaultEmptyText = _L("Empty").AllocL(); // StringLoader::LoadL(R_AVKON_SELEC_SETT_VAL_FIELD_NONE);
		CleanupStack::PopAndDestroy(); // reader
	}
	
private:
	// These two are indexed by Identifier
	CAknSettingItem	*iItems[SETTINGS_COUNT];
	TAny*		iData[SETTINGS_COUNT];
	TBool	iOnlyContextLogPublishing;
	
	TInt iResourceFile;
	CEikonEnv* iEikEnv;
};

EXPORT_C CAutoSettings* CAutoSettings::NewL(TBool aOnlyContextLogPublishing)
{
	auto_ptr<CAutoSettingsImpl> self( new (ELeave) CAutoSettingsImpl( aOnlyContextLogPublishing ) );
	self->ConstructL();
	return self.release();
}
