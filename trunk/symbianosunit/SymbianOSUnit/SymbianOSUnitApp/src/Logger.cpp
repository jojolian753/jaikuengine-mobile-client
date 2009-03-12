/* Copyright (c) 2003-2006, Penrillian Ltd. All rights reserved 
	Web: www.penrillian.com
*/

#include <eikenv.h>
#include <BAUTILS.H>
#include "Logger.h"
#include <eikappui.h>
#include <eikapp.h>
#include <utf.h>

#include "Project.h"
#include "Logger.h"

const TInt KMaxTraceLength=256; 
const TInt KUidPenrillianLogger = 0x1020458B;
const TInt KNumberOfProfilingBins = 16;


/** 
Initialize logging.  aSuiteName is the directectory under "C:\logs" where the logging file will be created.

May be called again (without effect).
*/

void Logger::StartL( const TDesC& /*aDirName*/ )
{
	LogAppName();
	__PROFILE_RESET(KNumberOfProfilingBins);
}

/** 
	Finish logging.
	Must be called on exit from the app (else memory/file handle leak).  May be called twice or before init...
*/
void Logger::Finish()
{
#ifdef __PROFILING__
	WriteProfileResults();
#endif
}

/**
Create a log entry - for 8-bit data using just a C-string
Takes printf-style variable parameters:
See TDesC::Format for a description of format stings.

Panics if log entry is more than KMaxTraceLength chars.
*/
void Logger::Log( char* aFmt,... )
{
	if ( DebugEnabled() )
	{
		VA_LIST list;
		VA_START(list,aFmt);
		TBuf8<KMaxTraceLength> buf;
		buf.FormatList(TPtrC8((const TUint8*)aFmt),list);
		
		TRAPD(err, WriteTraceL(buf));
	}
}

/**
Create a log entry - for 8-bit data.
Takes printf-style variable parameters:
See TDesC::Format for a description of format stings.

 Panics if log entry is more than KMaxTraceLength chars.
*/
void Logger::Log( TRefByValue<const TDesC8> aFmt,... )
{
	if ( DebugEnabled() )
	{
		VA_LIST list;
		VA_START(list,aFmt);
		TBuf8<KMaxTraceLength> buf;
		buf.FormatList(aFmt,list);
		TRAPD(err, WriteTraceL(buf));
	}
}

/**
Create a log entry - for 16-bit data.
Takes printf-style variable parameters:
See TDesC::Format for a description of format stings.

  Panics if log entry is more than KMaxTraceLength chars.
*/
void Logger::Log( TRefByValue<const TDesC> aFmt,... )
{
	if ( DebugEnabled() )
	{
		VA_LIST list;
		VA_START(list,aFmt);
		TBuf<KMaxTraceLength> unicodeText;
		unicodeText.FormatList(aFmt,list);
		TBuf8<KMaxTraceLength*2+1> utf8;
		CnvUtfConverter::ConvertFromUnicodeToUtf8(utf8,unicodeText);
		TRAPD(err, WriteTraceL(utf8));
	}
}

/**
Do the actual logging.  
Write the given text (up to the first non-printable char) to log file with a timestamp.
In WINS also writes to debug output.
*/
void Logger::WriteTraceL( const TDesC8& aLogText )
{
	
	// Write the timestamp
	TTime time;
	time.HomeTime();
	_LIT(KTimeFormat,"%-B%:0%J%:1%T%:2%S%.%*C4%:3%+B");
	TBuf<64> format;
	TRAPD( err, time.FormatL( format, KTimeFormat ));
	TBuf8<KMaxTraceLength> utf8;
	CnvUtfConverter::ConvertFromUnicodeToUtf8(utf8,format);
	
	// Write all the printable text:
	TInt i = 0;
	for (; i<aLogText.Length(); i++)
	{
		if (!(TChar(aLogText[i]).IsPrint()||TChar(aLogText[i]).IsSpace()))
			break;
	}

	TPtrC8 toLog = aLogText.Left(i);
	LOG2( _L8("%S   %S"), &utf8, &toLog );

#ifdef __WINS__
	HBufC* output = HBufC::NewLC(aLogText.Length());
	TPtr outputText = output->Des();
	CnvUtfConverter::ConvertToUnicodeFromUtf8( outputText, aLogText );
	RDebug::Print( _L( "%S" ), &outputText );
	CleanupStack::PopAndDestroy();//output
#endif
}

/**
	Answers true if debugging has been initialized.
*/
TBool Logger::DebugEnabled()
{
	return ETrue;
}

/**
	Write the app name to the log.
*/
void Logger::LogAppName()
{
	CEikAppUi* appUi = static_cast<CEikAppUi*>(CEikonEnv::Static()->AppUi());
	CEikApplication* app = appUi->Application();
	if (app != NULL)
	{
		TFileName appName = app->AppFullName();
		Logger::Log(appName);
		TInt appUid = 0;
		app->AppDllUid().Uid(appUid);
		Logger::Log("UID = %d", appUid);
	}
}

/**
	Answers the root part of the application executable filename.
*/
TFileName Logger::AppName()
{
	CEikAppUi* ui = CEikonEnv::Static()->EikAppUi(); 
	CEikApplication* app = ui->Application(); 
	TFileName appName = app->AppFullName(); 
	TParse appParser; 
	appParser.Set(appName,NULL,NULL); 
	return appParser.Name(); 
}


TFileName Logger::GenerateFileName( const TDesC& aDirName )
{
	_LIT( KFormat, "%Fc:\\logs\\%%S\\%%S%Y%M%D_%H%T%S_%*C3.log" );
	
	TTime time;
	time.HomeTime();
	TFileName formatString;
	TRAPD( ignore, time.FormatL( formatString, KFormat ) );
	
	// Now have in formatString, e.g.
	// "c:\logs\%S\%S040930_114001_777.log"
	
	TFileName applicationName = AppName();
	TFileName result;
	result.Format( formatString, &aDirName, &applicationName );
	return result;
}

#ifdef __PROFILING__
void Logger::WriteProfileResults()
{
	TFixedArray<TProfile, KNumberOfProfilingBins> result; 
	RDebug::ProfileResult(result.Begin(), 0, KNumberOfProfilingBins); 
	for (TInt i=0; i<KNumberOfProfilingBins; i++)   
		Log( "Profile bin %d:  Calls: %d, Clock ticks: %d",i,result[i].iCount,result[i].iTime);  
}
#endif // __PROFILING__

