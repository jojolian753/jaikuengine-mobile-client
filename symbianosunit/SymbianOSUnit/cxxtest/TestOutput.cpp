//TestOutput.cpp
/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/


#include "cxxtest/TestOutput.h"
#include <bautils.h>

_LIT8(KNewLine,"\n");
_LIT(KNewLine16,"\n");
_LIT8(KCRLF,"\r\n");
//const TInt KEdwinNewLine = 0x2028; // force newline character

CTestOutput* CTestOutput::NewL(HBufC* aOutputText)
{
	CTestOutput* eikConsole = new (ELeave) CTestOutput(aOutputText);
	CleanupStack::PushL(eikConsole);
	eikConsole->ConstructL();
	CleanupStack::Pop();
	return eikConsole;
}

CTestOutput::~CTestOutput()
{
	iFile.Close();
	iRfs.Close();
}

void CTestOutput::ConstructL()
{
	User::LeaveIfError(iRfs.Connect());
	TParse parse;
	parse.Set(KLogFileName,NULL,NULL);
	if(!BaflUtils::FolderExists(iRfs,parse.DriveAndPath()))
		User::LeaveIfError(iRfs.MkDir(parse.DriveAndPath()));
	User::LeaveIfError(iFile.Replace(iRfs,KLogFileName,EFileWrite));
}

CTestOutput& CTestOutput::operator<<(const char* output)
{	
	const TText8 * buf=(const TText8*) (output);	
	TPtrC8 temp(buf,strlen(output));
	return operator<<(temp);								
}	

CTestOutput& CTestOutput::operator<<(TDesC8& _des)
{
	TRAPD(err,WriteToConsoleAndFileL(_des));
	if(err != KErrNone)
		iOutputText->Des().Append(_L("Error Writing to file !"));
	return *this;
}

void CTestOutput::WriteToConsoleAndFileL(TDesC8& _des)
{
	HBufC16* unicode = HBufC16::NewLC(_des.Length()+3);
	TPtr16 buf = unicode->Des();
	CnvUtfConverter::ConvertToUnicodeFromUtf8(buf,_des);
	ReplaceLFWithEdwinSpecificLFL(buf);
	iOutputText->Des().Append(*unicode);
	CleanupStack::PopAndDestroy(); //unicode

	HBufC8* des = ReplaceLFWithCRLFL(_des);
	iFile.Write(*des);

	if (_des.Length() > iLogTrace.MaxLength() - iLogTrace.Length())
	{
		iLogTrace.Zero();
	}
	else
	{
		iLogTrace.Append( _des );
		if (iLogTrace.Locate( '\n' )!=KErrNotFound)
		{
			iLogTrace.Zero();
		}
	}
	delete des;
}

void CTestOutput::ReplaceLFWithEdwinSpecificLFL(TDes16& aDes)
{
	TInt pos = KErrNotFound;
	TBuf<1> KEdwinNewLine;
	KEdwinNewLine.Append(0x2028);
	while ( (pos = aDes.Find(KNewLine16)) != KErrNotFound )
	{
		aDes.Replace(pos,1,KEdwinNewLine);
	}
}


HBufC8* CTestOutput::ReplaceLFWithCRLFL(TDesC8& aDes)
{
	TPtrC8 temp(aDes.Ptr(),aDes.Length());
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


