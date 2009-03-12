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

#include "cl_settings.h"
#include "user_notifier.h"
#include "timeout.h"
#include "Contextclientsession.h"
#include "symbian_auto_ptr.h"
#include "contextcommon.h"
#include "eikenv.h"
#include "aknquerydialog.h"
#include "aknmessagequerydialog.h"
#include "contextlog_resource.h"
#include "browser_interface.h"
#include <e32std.h>

#include <viewcli.h> //CVwsSessionWrapper
#include <vwsdef.h>
#include <flogger.h>
#include <charconv.h>

#include "expat.h"


_LIT(KClassName, "CUserNotifierImpl");


class CUserNotifierImpl : public CUserNotifier, public CCheckedActive, public MContextBase, public MTimeOut {
public:
	~CUserNotifierImpl();
private:
	CUserNotifierImpl(MApp_context& Context, CAlerter * aRingp);
	void ConstructL();

	void CheckedRunL();
	TInt CheckedRunError(TInt aError);
	void DoCancel();

	void expired(CBase*);

	void Start();
	void Restart();

	void RequestMessageNotification();
	void CancelRequest();

	void NotifyUser(/*TDesC& contact, TDesC& subject, TDesC& message*/);

	RContextClientSession iSession;
	bool iSessionOpen;
	CTimeOut* iWait;

	static void startElement(void *userData, const XML_Char *el, const XML_Char **atts);
	void startElement(const XML_Char *el, const XML_Char **atts);
	
	static void endElement(void *userData, const XML_Char *name);
	void endElement(const XML_Char *name);

	static void charData(void *userData, const XML_Char *s, int len);
	void charData(const XML_Char *s, int len);

	enum TState
	{
      		EIdle,
        	EWaitingForMessage		
	};
	TState current_state;

	enum TParseState
	{
		EStackUndefined,
		EStackIgnoreElement,
		EStackUrl,
		EStackAlert,
		EStackDescription,
		EStackTitle,
		EStackMimeType,
		EStackError
	};

	TPtr iC;
	TPtr iS;
	TPtr iM;

	HBufC * iContact;
	HBufC * iSubject;
	HBufC * iMessage;

	CAlerter * iRingp;

	XML_Parser		iParser;
	CList<TInt>	* iStack;

	HBufC * iAlertDesc;
	HBufC * iAlertUrl;
	HBufC * iAlertTitle;
	HBufC * iAlertMimeType;

	const static TInt KDefaultBufferSize;

	friend class CUserNotifier;
};

const TInt CUserNotifierImpl::KDefaultBufferSize=512;

CUserNotifier* CUserNotifier::NewL(MApp_context& Context, CAlerter* aRingp)
{
	CALLSTACKITEM_N(_CL("CUserNotifier"), _CL("NewL"));

	auto_ptr<CUserNotifierImpl> self(new (ELeave) CUserNotifierImpl(Context, aRingp));
    	self->ConstructL();
    	return self.release();
}

CUserNotifierImpl::CUserNotifierImpl(MApp_context& Context, CAlerter * aRingp) : 
	CCheckedActive(EPriorityNormal, KClassName), MContextBase(Context), 
	iC(0,0), iS(0,0), iM(0,0), iRingp(aRingp)
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("CUserNotifierImpl"));

    	CActiveScheduler::Add(this);
}

void CUserNotifierImpl::ConstructL()
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("ConstructL"));

	iWait=CTimeOut::NewL(*this);
	current_state = EIdle;

	iStack = CList<TInt>::NewL();

	iParser = XML_ParserCreate(NULL);

	XML_ParserReset(iParser, NULL);
	XML_SetUserData(iParser,this);
	XML_SetElementHandler(iParser, CUserNotifierImpl::startElement, CUserNotifierImpl::endElement);
	XML_SetCharacterDataHandler(iParser, CUserNotifierImpl::charData);


	iContact = HBufC::NewL(KDefaultBufferSize/4);
	iSubject = HBufC::NewL(KDefaultBufferSize/4);
	iMessage = HBufC::NewL(KDefaultBufferSize);

	iAlertDesc=HBufC::NewL(256);
	iAlertUrl=HBufC::NewL(128);
	iAlertTitle=HBufC::NewL(128);
	iAlertMimeType=HBufC::NewL(32);

	Start();
}

void CUserNotifierImpl::startElement(void *userData, const XML_Char *el, const XML_Char **atts)
{

	CUserNotifierImpl* notifier=(CUserNotifierImpl*)userData;
	notifier->startElement(el, atts);
}

void CUserNotifierImpl::startElement(const XML_Char *el, const XML_Char ** /*atts*/)
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("startElement"));

	TInt currentState = EStackUndefined;
	
	if ( iStack->iCurrent != NULL )
	{
		currentState = iStack->iCurrent->Item;
	}
	
	// if we're already ignoring parent tag
	if ( currentState == EStackIgnoreElement)
	{
		// then let's ignore all children
		iStack->AppendL(EStackIgnoreElement);
		return;
	}
	
	//--------------- XML TAGS AND ATTR _LIT-------------
	
	_LIT(KAlert, "alert");
	_LIT(KDescription, "description");
	_LIT(KUrl, "url");
	_LIT(KTitle, "title");
	_LIT(KMimeType, "mime");

	TPtrC element = TPtrC( (TUint16*)el );
	
	if (element.Compare(KAlert) == 0) {
		// Case: <alert>
		if (currentState != EStackUndefined)
		{
			iStack->AppendL(EStackError);
		}else{
			iStack->AppendL(EStackAlert);
		}
	} else if ( element.Compare(KDescription) == 0) {
		//case: <description>
		if (currentState != EStackAlert)
		{
			iStack->AppendL(EStackError);
		}else{
			iStack->AppendL(EStackDescription);
		}
	} else if ( element.Compare(KUrl) == 0) {
		//Case: <url>
		if (currentState != EStackAlert)
		{
			iStack->AppendL(EStackError);
		}else{
			iStack->AppendL(EStackUrl);
		}
	} else if (element.Compare(KTitle) == 0) {
		//case: <title>
		if (currentState != EStackAlert)
		{
			iStack->AppendL(EStackError);
		}else{
			iStack->AppendL(EStackTitle);
		}
	} else if (element.Compare(KMimeType) == 0) {
		//case: <mime>
		if (currentState != EStackAlert)
		{
			iStack->AppendL(EStackError);
		}else{
			iStack->AppendL(EStackMimeType);
		}
	}
	else {
		// Case: unhandled tag
		iStack->AppendL(EStackIgnoreElement);
	}
}

void CUserNotifierImpl::endElement(void *userData, const XML_Char *name)
{

	CUserNotifierImpl* notifier=(CUserNotifierImpl*)userData;
	notifier->endElement(name);
}

void CUserNotifierImpl::endElement(const XML_Char * /*name*/)
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("endElement"));

#ifndef __S60V2__
	_LIT(KVideo, "video/3gpp");
	_LIT(KAudio, "audio/amr");
#endif
	switch (iStack->iCurrent->Item)
	{
		
	case EStackIgnoreElement:
		// just pop the last item
		iStack->DeleteLast();
		break;
		
	case EStackUrl:
		iStack->DeleteLast();
		break;

	case EStackDescription:
		iStack->DeleteLast();
		break;

	case EStackTitle:
		iStack->DeleteLast();
		break;

	case EStackMimeType:
		iStack->DeleteLast();
		break;

	case EStackAlert:
		iStack->DeleteLast();
		
		TBool notify;
		Settings().GetSettingL(SETTING_IGNORE_NOTIFICATIONS, notify);
		if (notify == EFalse)
		{
			#  ifndef __S60V2__
			
			//FIXME
			// doris on 7650 doesn't handle audio and video properly
			if ( (iAlertMimeType->Compare(KVideo) != 0) && (iAlertMimeType->Compare(KAudio) != 0) )
			{
				NotifyUser();
			}
			#  else

			// 6600 ok!
			NotifyUser();
			#  endif 
		}
		break;

	case EStackError:
		XML_ParserReset(iParser, NULL);
		XML_SetUserData(iParser,this);
		XML_SetElementHandler(iParser, CUserNotifierImpl::startElement, CUserNotifierImpl::endElement);
		XML_SetCharacterDataHandler(iParser, CUserNotifierImpl::charData);
		iStack->reset();
		break;

	}

}

void CUserNotifierImpl::charData(void *userData, const XML_Char *s, int len)
{

	CUserNotifierImpl* notifier=(CUserNotifierImpl*)userData;
	notifier->charData(s,len);
}

void CUserNotifierImpl::charData(const XML_Char *s, int len)
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("charData"));

	if (!iStack->iCurrent) return;
	
	if (iStack->iCurrent->Item == EStackDescription) {
		TPtrC t = TPtrC((TUint16*)s,len);
		while ( iAlertDesc->Length()+len > iAlertDesc->Des().MaxLength()) {
			iAlertDesc=iAlertDesc->ReAllocL(iAlertDesc->Des().MaxLength()*2);
		}
		iAlertDesc->Des().Append(t);
	} else if (iStack->iCurrent->Item == EStackUrl)	{
		TPtrC t = TPtrC((TUint16*)s,len);
		while ( iAlertUrl->Length()+len > iAlertUrl->Des().MaxLength()) {
			iAlertUrl=iAlertUrl->ReAllocL(iAlertUrl->Des().MaxLength()*2);
		}
		iAlertUrl->Des().Append(t);
	} else if (iStack->iCurrent->Item == EStackTitle) {
		TPtrC t = TPtrC((TUint16*)s,len);
		while ( iAlertTitle->Length()+len > iAlertTitle->Des().MaxLength()) {
			iAlertTitle=iAlertTitle->ReAllocL(iAlertTitle->Des().MaxLength()*2);
		}
		iAlertTitle->Des().Append(t);
	} else if (iStack->iCurrent->Item == EStackMimeType) {
		TPtrC t = TPtrC((TUint16*)s,len);
		while ( iAlertMimeType->Length()+len > iAlertMimeType->Des().MaxLength()) {
			iAlertMimeType=iAlertMimeType->ReAllocL(iAlertMimeType->Des().MaxLength()*2);
		}
		iAlertMimeType->Des().Append(t);
	}
}



//------------------------------------------------------------------------------

void CUserNotifierImpl::Start()
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("Start"));

	if (iSession.ConnectToContextServer() == KErrNone)
	{
		iSessionOpen=true;
		RequestMessageNotification();
	} else {
		Restart();
	}
}

void CUserNotifierImpl::Restart()
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("Restart"));

	Cancel();
	iWait->Cancel();
	if (iSessionOpen) {
		iSession.Close();
	}
	iSessionOpen=false;
	//RDebug::Print(_L("restarting"));
	iWait->Wait(20);
}


void CUserNotifierImpl::expired(CBase*)
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("expired"));

	Start();
}

CUserNotifierImpl::~CUserNotifierImpl()
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("~CUserNotifierImpl"));

	Cancel();
	if (iSessionOpen) iSession.Close();
	delete iWait;

	delete iContact;
	delete iSubject;
	delete iMessage;
	XML_ParserFree(iParser);
	delete iStack;
	delete iAlertUrl;
	delete iAlertDesc;
	delete iAlertTitle;
	delete iAlertMimeType;
}

void CUserNotifierImpl::RequestMessageNotification()
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("RequestMessageNotification"));

	if(!IsActive())
	{
		current_state = EWaitingForMessage;

		iC.Set(iContact->Des());
		iS.Set(iSubject->Des());
		iM.Set(iMessage->Des());

		iSession.MsgRequestMessageNotification(iC, iS, iM, iStatus);
		SetActive();
	}		
}

void CUserNotifierImpl::CancelRequest()
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("CancelRequest"));

    	Cancel() ;
}

void CUserNotifierImpl::DoCancel()
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("DoCancel"));

	iSession.Cancel();
	current_state = EIdle;
}

void CUserNotifierImpl::CheckedRunL()
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("CheckedRunL"));

	if (iStatus == ERequestCompleted)
	{
		switch (current_state)
		{
			case EWaitingForMessage:
			{
				XML_ParserReset(iParser, NULL);
				XML_SetUserData(iParser,this);
				XML_SetElementHandler(iParser, CUserNotifierImpl::startElement, CUserNotifierImpl::endElement);
				XML_SetCharacterDataHandler(iParser, CUserNotifierImpl::charData);

				// FIX ME: Quick and dirty replacement of & by &amp; to ensure expat parses without errors
				TInt count = 0;
				int i;
				for(i=0; i< iM.Length(); i++)
				{
					if (iM.Mid(i,1).Compare(_L("&")) == 0 )
					{
						count ++;
					}
				}
				HBufC * buf = HBufC::NewL(iM.Length() + count*4);
				TPtr temp = TPtr(0,0);
				temp.Set(buf->Des());

				for (i=0; i<iM.Length(); i++)
				{
					if (iM.Mid(i,1).Compare(_L("&")) == 0)
					{
						temp.Append( _L("&amp;"));
					}
					else
					{
						temp.Append( iM.Mid(i,1) );
					}
				}
				// END OF FIX ME
								
				if (!XML_Parse(iParser,(char*)(temp.Ptr()), temp.Size(), EFalse)) {
					
					XML_Error code=XML_GetErrorCode(iParser);
					//TPtrC descr((TUint16*)XML_ErrorString(code));
					//Log(_L("XML parse error"));
					//Log(descr);
					//ReportError(MEngineNotifier::EXmlParseError, code);

					// FIXME: should report the error somewhere...
					RFileLogger iLog;
					if (iLog.Connect()==KErrNone) {
						iLog.CreateLog(_L("Notif"),_L("log.txt"),EFileLoggingModeAppend);
						iLog.Write(_L("parsing error"));
						// Close the log file and the connection to the server.
						iLog.CloseLog();
						iLog.Close();
					}
					
				}

				delete buf;

				RequestMessageNotification();

				
				break;
			}
			default:
				Restart();
				// ASSERT(0); //Unexpected error
				break;
		}
	}
	else if (iStatus == EBufferTooSmall)
	{
		iContact = iContact->ReAllocL(iC.MaxLength() *2);
		iC.Set(iContact->Des());

		iSubject = iSubject->ReAllocL(iS.MaxLength() *2);
		iS.Set(iSubject->Des());

		iMessage = iMessage->ReAllocL(iM.MaxLength() *2);
		iM.Set(iMessage->Des());

		RequestMessageNotification();
	}
	else 
	{
		Restart();
	}
}

TInt CUserNotifierImpl::CheckedRunError(TInt aError)
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("CheckedRunError"));

	return aError;
}

void CUserNotifierImpl::NotifyUser(/*TDesC& contact, TDesC& subject, TDesC& message*/)
{
	CALLSTACKITEM_N(_CL("CUserNotifierImpl"), _CL("NotifyUser"));

	RWsSession& wsSession=CEikonEnv::Static()->WsSession();
	TInt id = CEikonEnv::Static()->RootWin().Identifier();
	TApaTask task(wsSession);
	task.SetWgId(id);
	TBool hide = EFalse;
	if ( wsSession.GetFocusWindowGroup() != id )
	{
		task.BringToForeground();
		hide = ETrue;
	}

	// cannot play tone if we want to play a ringtone
	CAknQueryDialog::TTone tone=CAknQueryDialog::ENoTone; 
	iRingp->AlertUser();
	CAknMessageQueryDialog* dlg = CAknMessageQueryDialog::NewL(*iAlertDesc, tone);
		 
	CleanupStack::PushL(dlg);
	dlg->PrepareLC(R_MESSAGE_QUERY);
	CleanupStack::Pop();
	if (iAlertTitle->Length() == 0)
	{
		dlg->QueryHeading()->SetTextL(_L("Notification"));
	}
	else
	{
		dlg->QueryHeading()->SetTextL(*iAlertTitle);
	}

	if ( dlg->RunLD() )
	{
		//FIXME: duplicated code!

		#ifdef __WINS2__
		CEikonEnv::Static()->BusyMsgL(*iAlertUrl, 2000);
		#endif

		#ifndef __WINS2__
		
		#  ifndef __S60V2__

		//FIXME: Doris Browser doesn't handle url like:
		//http://website.net/picture_viewer.php?image=kuva1
		//it seems to be able to handle only direct path to images like:
		//http://website.net/pictures/kuva1.jpg

		auto_ptr<CDorisBrowserInterface> ido(CDorisBrowserInterface::NewL());
		ido->AppendL(CDorisBrowserInterface::EOpenURL_STRING, *iAlertUrl);
		ido->ExecuteL();
		
		#  else
		auto_ptr<HBufC8> addr8(HBufC8::NewL(iAlertUrl->Length()));
		TPtr8 addrp=addr8->Des();
		CC()->ConvertFromUnicode(addrp, *iAlertUrl);
		
		TUid KUidOperaBrowserUid = {0x101F4DED};
		TUid KUidOperaRenderViewUid = {0};

		TVwsViewId viewId(KUidOperaBrowserUid, KUidOperaRenderViewUid);
		
		auto_ptr<CVwsSessionWrapper> vws(CVwsSessionWrapper::NewL());
		vws->ActivateView(viewId, KUidOperaRenderViewUid, *addr8);
		#  endif

		#endif
	}
	
	//hide
	if (hide) { task.SendToBackground();}
	
	iRingp->StopAlert();
	iAlertDesc->Des().Zero();
	iAlertUrl->Des().Zero();
	iAlertTitle->Des().Zero();
	iAlertMimeType->Des().Zero();
}
