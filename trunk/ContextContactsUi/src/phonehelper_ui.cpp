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
#include "phonehelper_ui.h"
#include "app_context.h"
#include <txtrich.h>
#include <SenduiMtmUids.h>
#include <eikenv.h>
#include <bautils.h>
#include <cpbkcontacteditordlg.h> 
#include <MsgBioUids.h>
#include <msvapi.h>
#include <msvids.h>
#include <aknviewappui.h> 
#include <cpbkcontactitem.h> 
#include <bcardeng.h>
#include <CPbkContactEngine.h>

#include <CPbkSingleEntryFetchDlg.h> 
#include <aknutils.h>

#ifdef __S60V3__
#include <senduimtmuids.h>
#include <cmessagedata.h>
#endif

#include "app_context_impl.h"

CPbkContactEngine * phonehelper_ui::Engine()
{
	if (!iMyEngine) {
		iMyEngine=CPbkContactEngine::Static();
		if (!iMyEngine) {
			iMyEngine=CPbkContactEngine::NewL();
			AppContext().TakeOwnershipL(iMyEngine);
		}
	}
	return iMyEngine;
}

EXPORT_C void phonehelper_ui::ConstructL()
{
	CALLSTACKITEM_N(_CL("phonehelper_ui"), _CL("ConstructL"));
	phonehelper::ConstructL();

	iPbkRes.OpenL();

	TFileName tempDir;
	TempVCardDir( tempDir );
	CFileMan* fm=CFileMan::NewL( Fs() );
	CleanupStack::PushL(fm);
	fm->RmDir(tempDir);
	CleanupStack::PopAndDestroy(fm);
}

EXPORT_C phonehelper_ui::~phonehelper_ui()
{
	CALLSTACKITEM_N(_CL("phonehelper_ui"), _CL("~phonehelper_ui"));
	iPbkRes.Close();
}


EXPORT_C void phonehelper_ui::show_editor(TInt contact_id,  bool duplicate, TInt focus_field_index, CPbkFieldInfo * fieldInfo, const TDesC& no )
{
	CALLSTACKITEM_N(_CL("phonehelper_ui"), _CL("show_editor"));

	CCoeEnv *env = CEikonEnv::Static();

	CPbkContactItem* aContactItem=0;
	TBool create_new = EFalse;
	//CPbkContactEngine * engine = aBook.get_engine();
	
	if (contact_id == -1) 
	{
		aContactItem=Engine()->CreateEmptyContactL();
		CleanupStack::PushL(aContactItem);
		create_new=ETrue;
	} 
	else 
	{
		aContactItem=Engine()->OpenContactLCX(contact_id);
		create_new=EFalse;
		if (duplicate) create_new=ETrue;
	}

	if (fieldInfo) 
	{
		TPbkContactItemField * field = aContactItem->AddOrReturnUnusedFieldL(*fieldInfo) ;
		if (field) field->TextStorage()->SetTextL(no);
		if (contact_id != -1)
		{
			focus_field_index = aContactItem->FindFieldIndex(*field);
		}
	}

	CPbkContactEditorDlg *ipPabDlg = CPbkContactEditorDlg::NewL(*Engine(), *aContactItem, create_new, focus_field_index );
	
	TInt res = KErrNone;
	CC_TRAPD( err, res = ipPabDlg->ExecuteLD());
	if (contact_id == -1) 
	{
		CleanupStack::PopAndDestroy(1);  //contact_item, pbkRes
	} 
	else 
	{
		CleanupStack::PopAndDestroy(2);  //contact_item, lock, pbkRes
	}
}

EXPORT_C void phonehelper_ui::send_as(TUid mtmUid, CArrayFixFlat<TInt> * c)
{
	CALLSTACKITEM_N(_CL("phonehelper_ui"), _CL("send_as"));

	TFileName tempDir;
	TempVCardDir( tempDir );
	CFileMan* fm=CFileMan::NewL( Fs() );
	CleanupStack::PushL(fm);
	TInt err = fm->RmDir(tempDir);
	if ( err != KErrNone && err != KErrNotFound && err != KErrPathNotFound)
	  User::LeaveIfError( err );
	CleanupStack::PopAndDestroy(fm);

	CDesCArrayFlat* file_list = new CDesCArrayFlat(1);
	CleanupStack::PushL(file_list);
	
	// generate vcard files
	for (int i=0; i< c->Count(); i++)
	{
		TFileName filename = ExportVCardToFile(c->At(i));
		if (filename.Length() >0 ) file_list->AppendL(filename);
	}

	if (file_list->Count() <= 0) 
	{
		CleanupStack::PopAndDestroy();
		return;
	}
	
	if (mtmUid == KSenduiMtmSmsUid)  // sending as a BIO message
	{
		CParaFormatLayer* paraf=CParaFormatLayer::NewL();
		CleanupStack::PushL(paraf);

		CCharFormatLayer* charf=CCharFormatLayer::NewL();
		CleanupStack::PushL(charf);

		CRichText* body;
		body=CRichText::NewL(paraf, charf);
		CleanupStack::PushL(body);

		TInt last_char_idx = 0;
		
		for (int i=0; i<file_list->Count();i++)
		{
			RFile tempfile;
			TInt err = tempfile.Open(Fs(), (*file_list)[i], EFileRead);
			TInt size;
			err = tempfile.Size(size);
			HBufC8 * temp8 = HBufC8::NewL(size);
			CleanupStack::PushL(temp8);
			TPtr8 des=temp8->Des();
			tempfile.Read(des);
			tempfile.Close();
			// cleaning
			Fs().Delete((*file_list)[i]);
		
			HBufC * temp16 = HBufC::NewL(des.Length()+10);
			CleanupStack::PushL(temp16);
			temp16->Des().Copy(des);
			body->InsertL(last_char_idx, temp16->Des() );
			last_char_idx += temp16->Length();
			CleanupStack::PopAndDestroy(2);  //temp8, temp16
		}
		
		CDesCArrayFlat* a=new CDesCArrayFlat(1);
		CleanupStack::PushL(a);
					
#ifndef __S60V3__
        iSendAppUi->CreateAndSendMessageL(KSenduiMtmSmsUid , body, 0, KMsgBioUidVCard, a);
#else
		CMessageData* data=CMessageData::NewLC();
		data->SetBodyTextL(body);
		iSendAppUi->CreateAndSendMessageL(KSenduiMtmSmsUid, data, KMsgBioUidVCard, EFalse);
		CleanupStack::PopAndDestroy(1); // data
#endif
		CleanupStack::PopAndDestroy(4); //a, body, paraf, charf
	}
	else  // sending as attachment
	{
		CParaFormatLayer* paraf=CParaFormatLayer::NewL();
		CleanupStack::PushL(paraf);

		CCharFormatLayer* charf=CCharFormatLayer::NewL();
		CleanupStack::PushL(charf);

		CRichText* body;
		body=CRichText::NewL(paraf, charf);
		CleanupStack::PushL(body);

		CDesCArrayFlat* a=new CDesCArrayFlat(1);
		CleanupStack::PushL(a);			

#ifndef __S60V3__
		iSendAppUi->CreateAndSendMessageL(mtmUid , body, file_list, KNullUid, a);
#else
		CMessageData* data=CMessageData::NewLC();
		for (int i=0; i<file_list->Count();i++)
		{
			data->AppendAttachmentL( (*file_list)[i] );
		}
		iSendAppUi->CreateAndSendMessageL(mtmUid, data, KNullUid, EFalse);
		CleanupStack::PopAndDestroy(1); //data
#endif
                		
		CleanupStack::PopAndDestroy(4); //body, charf, paraf, a

		for (int i=0; i<file_list->Count();i++)
		{
		  // cleaning.
		  // Call is asynchronous and MMS sending needs files after returning.
		  // Don't delete them
		  //Fs().Delete((*file_list)[i]);
		}
	}
	CleanupStack::PopAndDestroy(file_list);
}

EXPORT_C void phonehelper_ui::TempVCardDir( TDes& aInto ) 
{
  aInto.Append( DataDir() );
  aInto.Append(_L("tmp\\"));
}  

// From: http://en.wikipedia.org/wiki/Filename

_LIT(KDisallowedFilenameChars, "|\\?*<\":>+[]/.");

EXPORT_C TFileName phonehelper_ui::ExportVCardToFile(TInt ContactId)
{
	CALLSTACKITEM_N(_CL("phonehelper_ui"), _CL("ExportVCardToFile"));


	TFileName filename;
	TempVCardDir( filename );
	BaflUtils::EnsurePathExistsL(Fs(), filename);
		
	CBCardEngine* cardEngine = CBCardEngine::NewL(Engine()); 
	CleanupStack::PushL(cardEngine);
	
	CPbkContactItem* con = Engine()->ReadContactLC(ContactId);
	
	HBufC * title = con->GetContactTitleOrNullL();
	CleanupStack::PushL(title);
	if (title) {
		TInt pos = filename.Length();
		filename.Append(*title);
		TPtr justFile = filename.MidTPtr( pos );
		AknTextUtils::ReplaceCharacters( justFile, KDisallowedFilenameChars, TChar('_') );
		filename.Append(_L(".vcf"));
	} else {
	  filename.AppendFormat(_L("unnamed%02d.vcf"), ContactId);
	}
	CleanupStack::PopAndDestroy(); //title

	RFile outfile;
	TInt err = outfile.Replace(Fs(), filename, EFileWrite);
	if (err != KErrNone) return _L("");
		
	RFileWriteStream writeStream(outfile);

	cardEngine->ExportBusinessCardL(writeStream, *con);
	CleanupStack::PopAndDestroy(con);

	writeStream.CommitL(); 
	writeStream.Close(); 
	outfile.Close(); 

	CleanupStack::PopAndDestroy(cardEngine);

	return filename;
}

EXPORT_C TContactItemId phonehelper_ui::AddNewContactL(const TDesC& aFirstName,
													   const TDesC& aLastName,
													   const TDesC& aNumber,
													   const TDesC& aEmail)
{	
	
	auto_ptr<CPbkContactItem> contact( Engine()->CreateEmptyContactL() );	
	if (aFirstName != KNullDesC) contact->FindField( EPbkFieldIdFirstName )->TextStorage()->SetTextL( aFirstName );
	if (aLastName != KNullDesC)  contact->FindField( EPbkFieldIdLastName )->TextStorage()->SetTextL( aLastName );
	if (aNumber != KNullDesC)     contact->FindField( EPbkFieldIdPhoneNumberGeneral )->TextStorage()->SetTextL( aNumber );
	if (aEmail != KNullDesC)     contact->FindField( EPbkFieldIdEmailAddress )->TextStorage()->SetTextL( aEmail );
	return Engine()->AddNewContactL( *contact );   
}


EXPORT_C TContactItemId phonehelper_ui::EditContactL(TContactItemId aContactId)
{	
	auto_ptr<CPbkContactItem> contact( Engine()->ReadContactL( aContactId ));
	CPbkContactEditorDlg *editor= CPbkContactEditorDlg::NewL(*Engine(), *contact, EFalse, -1, ETrue);
	TContactItemId res = KNullContactId;
	CC_TRAPD( err, res = editor->ExecuteLD() );
	return res;
}


EXPORT_C TContactItemId phonehelper_ui::FetchContactL( )
{
	CPbkSingleEntryFetchDlg::TParams params;
	params.iPbkEngine = Engine();
	
	
	CPbkSingleEntryFetchDlg* dlg = CPbkSingleEntryFetchDlg::NewL(params);
	TInt okPressed = dlg->ExecuteLD();
	
	return okPressed ? params.iSelectedEntry : KNullContactId;
}
