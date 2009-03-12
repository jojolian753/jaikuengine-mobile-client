
// Penrillian
// http://www.penrillian.com/
// sosunit@penrillian.com
// +441768214400

//TestConsole.cpp


#include "cxxtest/TestConsole.h"
#include <bautils.h>

CEikConsole* CEikConsole::NewL()
{
	CEikConsole* eikConsole = new (ELeave) CEikConsole();
	CleanupStack::PushL(eikConsole);
	eikConsole->ConstructL();
	CleanupStack::Pop();
	return eikConsole;
}

CEikConsole::~CEikConsole()
{
	delete iConsole;
	iFile.Close();
	iRfs.Close();
}

void CEikConsole::Getch()
{
	iConsole->Getch();
}

void CEikConsole::ConstructL()
{
	iConsole = Console::NewL(_L("Console Window"), TSize(KConsFullScreen, KConsFullScreen));
	User::LeaveIfError(iRfs.Connect());
	TParse parse;
	parse.Set(KLogFileName,NULL,NULL);
	if(!BaflUtils::FolderExists(iRfs,parse.DriveAndPath()))
		User::LeaveIfError(iRfs.MkDir(parse.DriveAndPath()));
	User::LeaveIfError(iFile.Replace(iRfs,KLogFileName,EFileWrite));
}

CEikConsole& CEikConsole::operator<<(const char* output)
{	
	const TText8 * buf=(const TText8*) (output);	
	TPtrC8 temp(buf,strlen(output));
	return operator<<(temp);								
}	

CEikConsole& CEikConsole::operator<<(TDesC8& _des)
{
	TRAPD(err,WriteToConsoleAndFileL(_des));
	if(err != KErrNone)
		iConsole->Printf(_L("Error Writing to file !"));
	return *this;
}

void CEikConsole::WriteToConsoleAndFileL(TDesC8& _des)
{
	HBufC16* unicode = HBufC16::NewLC(_des.Length()+3);
	TPtr16 buf = unicode->Des();
	CnvUtfConverter::ConvertToUnicodeFromUtf8(buf,_des);
	iConsole->Printf(*unicode);
	CleanupStack::PopAndDestroy(); //unicode
	HBufC8* des = ReplaceLFWithCRLFL(_des);
	iFile.Write(*des);
	delete des;
}

HBufC8* CEikConsole::ReplaceLFWithCRLFL(TDesC8& aDes)
{
	TPtrC8 temp(aDes.Ptr(),aDes.Length());
	_LIT8(KNewLine,"\n");
	_LIT8(KCRLF,"\r\n");
	TInt pos = KErrNotFound;
	HBufC8* hbuf = HBufC8::NewLC(aDes.Length()+3);
	TPtr8 buf =hbuf->Des();
	while ( (pos = temp.Find(KNewLine())) != KErrNotFound )
	{
		buf.Append(temp.Mid(0,pos));
		buf.Append(KCRLF);
		temp.Set(temp.Right(temp.Length()-pos-1));
	}
	buf.Append(temp.Ptr(),temp.Length());
	CleanupStack::Pop(); //hbuf
	return hbuf;
}


