
// Penrillian
// http://www.penrillian.com/
// sosunit@penrillian.com
// +441768214400

//TestConsole.h


#ifndef __CXXTEST__TESTCONSOLE_H
#define __CXXTEST__TESTCONSOLE_H

#include <e32cons.h>
#include <e32base.h>
#include <e32std.h>
#include <e32des16.h>
#include <libc/string.h>
#include <utf.h>
#include <f32file.h>

_LIT(KLogFileName,"c:\\logs\\CxxTestUint.log");

class CEikConsole : public CBase {
private:
	CConsoleBase* iConsole;
	RFs iRfs;
	RFile iFile;
	void ConstructL();
	void WriteToConsoleAndFileL(TDesC8& _des);
	HBufC8* ReplaceLFWithCRLFL(TDesC8& _des);
	
public:
	~CEikConsole();
	CEikConsole& operator<<(const char* output);	
	CEikConsole& operator<<(TDesC8& _des);
	
	inline CEikConsole& operator<<(char _s){
		TBuf8<2> buf;
		buf.Append(_s);
		return operator<<(buf);
	}

	inline CEikConsole& operator<<(unsigned _n){
		return operator<<((int) _n);
	}

	inline CEikConsole& operator<<(int _n){
		TBuf8<32> buf;
		buf.AppendNum(_n);
		return operator<<(buf);
	}

	inline CEikConsole& operator<<(const unsigned char * _s) {  return operator<<((const char *) _s); }
	inline CEikConsole& operator<<(const signed char * _s) {  return operator<<((const char *) _s); }
	
	static CEikConsole* NewL();			
	
	void Getch();	
};


#endif // __CXXTEST__TESTCONSOLE_H
