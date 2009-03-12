/* 
	Copyright (c) 2003-2006, Penrillian Ltd. All rights reserved 
	Web: www.penrillian.com
*/

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <e32base.h>

#ifdef SERIES60
#include <aknnotewrappers.h>
#endif

#ifdef _DEBUG

#undef ASSERT
#define __ASSERT_FILE__(s) _LIT(KPanicFileName,s)
#define __ASSERT_PANIC__(l) User::Panic(KPanicFileName().Right(12),l)
#define ASSERT(x) { __ASSERT_FILE__(__FILE__); __ASSERT_ALWAYS(x, __ASSERT_PANIC__(__LINE__) ); }

#endif

class RFile;

/**
	Does logging.  Also supports Symbian OS's simple profiling.

	The logging functions do nothing unless the directory "C:\logs\<suitename>" exists.
	On the emulator, logging output is also written to the debug output (if "C:\logs\<suitename>" exists).
*/
class Logger 
{
public:
	static void Log( TRefByValue<const TDesC> /*aFmt*/,... );
	static void Log( TRefByValue<const TDesC8> /*aFmt*/,... );
	static void Log( char* aFmt,... );
	static void WriteTraceL( const TDesC8& /*aLogText*/ );
	static void StartL( const TDesC& aDirName );
	static void Finish();
	static TBool DebugEnabled();
#ifdef __PROFILING__
	static void WriteProfileResults();
#endif // __PROFILING__

private:
	static void LogAppName();
	static TFileName GenerateFileName( const TDesC& aSuiteName );
	static TFileName AppName();
};

#ifndef LF
/** Utility to enable logging on block entry and exit. */
class LogFunc
{	public:
	LogFunc( char* aName ) { iName = aName; Logger::Log( "%s", aName ); }
	~LogFunc() { Logger::Log( "Exit %s", iName ); }
private:
	char* iName; 
};
#define LF( x )  LogFunc __dummy( #x );
#endif // LF
#define _LOGPOINT   Logger::Log( "Log point: %s:%d", __FILE__, __LINE__ );

#ifdef SERIES60
/**
	Two display macros for use debugging:
*/
#define MESSAGEBOX( text ) ((new (ELeave) CAknInformationNote)->ExecuteLD(text))
#define MESSAGEBOX_PRINTF( format, val ) \
{ TBuf<30> buf; buf.Format( format, val ); MESSAGEBOX(buf);}
#endif

#endif // __LOGGER_H__

