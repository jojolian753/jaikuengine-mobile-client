/* 
	Copyright (c) 2003-2006, Penrillian Ltd. All rights reserved 
	Web: www.penrillian.com
*/

#ifndef __logger_h__
#define __logger_h__


#include <flogger.h>

#define LOG_START() RFileLogger::WriteFormat(_L("SOSUnit"), _L("log.txt"), EFileLoggingModeOverwrite, _L8("Logging started, version compiled: %s %s"), __DATE__, __TIME__ );

#define LOG(x) RFileLogger::Write(_L( "SOSUnit" ),_L( "log.txt" ),EFileLoggingModeAppend, x)
#define LOG1(x, a1) RFileLogger::WriteFormat(_L( "SOSUnit" ),_L( "log.txt" ),EFileLoggingModeAppend, x, a1)
#define LOG2(x, a1, a2) RFileLogger::WriteFormat(_L( "SOSUnit" ),_L( "log.txt" ),EFileLoggingModeAppend, x, a1, a2)
#define LOG3(x, a1, a2, a3) RFileLogger::WriteFormat(_L( "SOSUnit" ),_L( "log.txt" ),EFileLoggingModeAppend, x, a1, a2, a3)
#define LOG6(x, a1, a2, a3, a4, a5, a6) RFileLogger::WriteFormat(_L( "SOSUnit" ),_L( "log.txt" ),EFileLoggingModeAppend, x, a1, a2, a3, a4, a5, a6)

#ifndef LF
class _LF
{
public:
	_LF(const char* text) : iText(text) { LOG1(_L8("+%s"), iText); }
	~_LF() { LOG1(_L8("-%s"), iText); }

private:
	_LF();
	const char* iText;
};

#define LF(x) _LF __foobar(x)
#endif // LF

#define _L_2( a, b ) (TPtrC(reinterpret_cast<const TText *>(L ## a L ## b)))
#define _L8_2( a, b ) (TPtrC8(reinterpret_cast<const TText8 *>(a b)))

#define LOGPOINT() LOG2(_L8("%s - %d"), __FILE__, __LINE__)

#define LOG_INT( anInt ) LOG1( _L_2(#anInt, "=%d" ), anInt )
#define LOG_DES16( aDes ) LOG1( _L_2(#aDes, "=%S"), &aDes )
#define LOG_DES8( aDes ) LOG1( _L8_2(#aDes, "=%S"), &aDes )
#define LOG_HEXINT( anInt ) LOG1( _L_2(#anInt, "=0x%x" ), anInt )

#endif
