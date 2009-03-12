//TestOutput.h
/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/


#ifndef __CXXTEST__TESTOUTPUT_H
#define __CXXTEST__TESTOUTPUT_H

#include <e32cons.h>
#include <e32base.h>
#include <e32std.h>
#include <e32des16.h>
#include <libc/string.h>
#include <utf.h>
#include <f32file.h>

_LIT(KLogFileName,"c:\\logs\\SOSUnit.log");

class CTestOutput : public CBase 
{
	
public:
	~CTestOutput();
	CTestOutput& operator<<(const char* output);	
	CTestOutput& operator<<(TDesC8& _des);
	
	inline CTestOutput& operator<<(char _s){
		TBuf8<2> buf;
		buf.Append(_s);
		return operator<<(buf);
	}

	inline CTestOutput& operator<<(unsigned _n){
		return operator<<((int) _n);
	}

	inline CTestOutput& operator<<(int _n){
		TBuf8<32> buf;
		buf.AppendNum(_n);
		return operator<<(buf);
	}

	inline CTestOutput& operator<<(const unsigned char * _s) {  return operator<<((const char *) _s); }
	inline CTestOutput& operator<<(const signed char * _s) {  return operator<<((const char *) _s); }
	
	static CTestOutput* NewL(HBufC* iOutputText);			

private:
	void ConstructL();
	void WriteToConsoleAndFileL(TDesC8& _des);
	void ReplaceLFWithEdwinSpecificLFL(TDes16& aDes);
	HBufC8* ReplaceLFWithCRLFL(TDesC8& _des);
	CTestOutput(HBufC* aOutputText):iOutputText(aOutputText){}

	HBufC* iOutputText;
	RFs iRfs;
	RFile iFile;
	TBuf8<80> iLogTrace;
};


#endif // __CXXTEST__TESTOUTPUT_H
