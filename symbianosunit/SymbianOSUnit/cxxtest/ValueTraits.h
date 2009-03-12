#ifndef __CXXTEST__VALUETRAITS_H
#define __CXXTEST__VALUETRAITS_H

//
// ValueTraits are used by CxxTest to convert arbitrary
// values used in TS_ASSERT_EQUALS() to a string representation.
// 
// This header file contains value traits for builtin integral types.
// To declare value traits for new types you should instantiate the class
// ValueTraits<YourClass>.
//
// Despite the name, the ValueTraits class is really just a holder for the 
// temporary string created for the conversion.

//Changed by Penrillian Ltd 14 August 2002
//Modified to use Symbian C++ descriptors, not ANSI C++ strings

#include <e32base.h>
#include <e32std.h>
#include <e32des16.h>
#include <e32des8.h>
#include <bautils.h>

#include "cxxtest/testoutput.h"

namespace CxxTest 
{
	const TInt KSensibleSize=100; // reasonable max size of an epoc string to print out.  
								// longer strings will just get truncated.
	class ETS { public:
	static TBuf8<KSensibleSize> DescriptorAsBuf8( const TDesC& aDes )
		{
		TBuf8<KSensibleSize+1> res;
		CnvUtfConverter::ConvertFromUnicodeToUtf8( res ,aDes);
		if (res.Length() == KSensibleSize+1)
			res[KSensibleSize]='\0';
		else
			res.Append( '\0' );
		
		return res;
		}    
	};
	
	//
    // This is how we use the value traits
    //
#   define TS_AS_STRING(x) CxxTest::traits(x).asString()
    
    //
    // The default ValueTraits class just returns "(unprintable value)"
    //
    template <class T>
    class ValueTraits
    {
    public:
	ValueTraits( T );
	const char *asString( void ) const { return "(unprintable value)";}
    };

    //
    // traits( T t )
    // Creates an object of type ValueTraits<T>
    //
    template <class T>
    inline ValueTraits<T> traits( T t )
    {
	return ValueTraits<T>( t );
    }

	typedef ValueTraits<const TDesC8&> VTTDC8;
	inline VTTDC8 traits(const TDesC8& t)
	{
		return VTTDC8(t);
	}

	typedef ValueTraits<const TDesC16&> VTTDC16;
	inline VTTDC16 traits(const TDesC16& t)
	{
		return VTTDC16(t);
	}


	//
    // Char representation of a digit
    //
    inline char digitToChar( unsigned digit )
    {
	return (char)((digit < 10) ? ('0' + digit) :
	    (digit < 36) ? ('A' + digit - 10) : '?');
    }


	template<class N>
		inline char* positiveNumberToString(N n, char *s, unsigned base = 10, unsigned skipDigits = 0, unsigned maxDigits = (unsigned)-1)
	{    
		N digit = 1;
		while ( (digit * base <= (unsigned) n) && (digit * base > (unsigned) digit) )
			digit *= base;
		N digitValue;
		for ( ; digit >= 1 && skipDigits; n -= digit * digitValue, digit /= base, -- skipDigits )
			digitValue = (unsigned)(n / digit);
		for ( ; digit >= 1 && maxDigits; n -= digit * digitValue, digit /= base, -- maxDigits )
			*s++ = digitToChar( (unsigned)(digitValue = (unsigned)(n / digit)) );
		
		*s = '\0';
		return s;
    }

    //
    // Represent (integral) number as a string
    // Remember -- we can't use the standard library!
    //
    template<class N>
    inline char *numberToString(N n, char *s )
    {
		if ( n < 0 ) {
			*s++ = '-';
			n = -n;
		}
	
		return positiveNumberToString(n, s);
	}
	
	template<>
    inline char *numberToString(unsigned int n, char *s )
    {
		return positiveNumberToString(n, s);
	}

	template<>
    inline char *numberToString(unsigned long n, char *s )
    {
		return positiveNumberToString(n, s);
	}


	template<class N>
	inline char *numberToString(N n, char *s, unsigned base , unsigned skipDigits , unsigned maxDigits ){
		if ( n < 0 ) {
			*s++ = '-';
			n = -n;
		}
		return positiveNumberToString(n, s, base, skipDigits, maxDigits);	
	}

	template<>
	inline char *numberToString(unsigned int n, char *s, unsigned base , unsigned skipDigits , unsigned maxDigits ){
		return positiveNumberToString(n, s, base, skipDigits, maxDigits);	
	}

	template<>
	inline char *numberToString(unsigned long n, char *s, unsigned base , unsigned skipDigits , unsigned maxDigits ){
		return positiveNumberToString(n, s, base, skipDigits, maxDigits);	
	}
    //
    // ValueTraits<signed int>
    //
	 template<> class ValueTraits<signed int>
    //class ValueTraits<signed int>
    {
	typedef int T;
	char _asString[2 + 3 * sizeof(T)];
    public:
	ValueTraits( T t ) { numberToString<T>( t, _asString ); }
	const char *asString( void ) const { return _asString; }
    };

    //
    // ValueTraits<unsigned int>
    //
	template<> class ValueTraits<unsigned int>
    //class ValueTraits<unsigned int>
    {
	typedef unsigned int T;
	char _asString[1 + 3 * sizeof(T)];
    public:
	ValueTraits( T t ) { numberToString<signed int>( t, _asString ); }
	const char *asString( void ) const { return _asString; }
    };

    //
    // ValueTraits<signed long>
    //
	template<> class ValueTraits<signed long>
    //class ValueTraits<signed long>
    {
	typedef long T;
	char _asString[2 + 3 * sizeof(T)];
    public:
	ValueTraits( T t ) { numberToString<T>( t, _asString ); }
	const char *asString( void ) const { return _asString; }
    };

    //
    // ValueTraits<unsigned long>
    //
	template<> class ValueTraits<unsigned long>
    //class ValueTraits<unsigned long>
    {
	typedef unsigned long T;
	char _asString[1 + 3 * sizeof(T)];
    public:
	ValueTraits( T t ) { numberToString<signed long>( t, _asString ); }
	const char *asString( void ) const { return _asString; }
    };

    //
    // ValueTraits<char>
    // Returns 'x' for printable chars, '\x??' for others
    //
	template<> class ValueTraits<char>
    //class ValueTraits<char>
    {
	char _asString[7];
	void plainChar( char c )
	{
	    _asString[0] = '\'';
	    _asString[1] = c;
	    _asString[2] = '\'';
	    _asString[3] = '\0';
	}
	void hexChar( char c )
	{
	    _asString[0] = '\'';
	    _asString[1] = '\\';
	    _asString[2] = 'x';
	    _asString[3] = digitToChar( c >> 4 );
	    _asString[4] = digitToChar( c & 15 );
	    _asString[5] = '\'';
	    _asString[6] = '\0';
	}
    public:
	ValueTraits( char c ) { (c >= 32 && (unsigned)c <= 127) ?
				    plainChar(c) : hexChar(c); }
	const char *asString( void ) const { return _asString; }
    };

	
	template<> class ValueTraits<bool>   
    {	
		bool _b;
    public:
		ValueTraits( bool uc ) : _b(uc){		
		}
		
		const char *asString( void ) const {
			if(_b)
				return "true"; 
			else
				return "false";
		}
		
    };

    //
    // ValueTraits<signed char>
    // Same as char, some compilers need it
    //
	template<> class ValueTraits<signed char> : public ValueTraits<char>
    //class ValueTraits<signed char> : public ValueTraits<char>
    {
//	typedef ValueTraits<char> super;
    public:
		ValueTraits( signed char uc ) : ValueTraits<char> (static_cast <char> (uc)){		
	}
    };

    //
    // ValueTraits<unsigned char>
    // Prints value as number
    //
	template<> class ValueTraits<unsigned char> : public ValueTraits<unsigned int>
    //class ValueTraits<unsigned char> : public ValueTraits<unsigned int>
    {
	//typedef ValueTraits<unsigned int> super;
    public:
	ValueTraits( unsigned char uc ) : ValueTraits<unsigned int>( uc ) {}
    };

    //
    // ValueTraits<double>
    //
	template<> class ValueTraits<double>
    //class ValueTraits<double>
    {
	enum { maxDigitsOnLeft = 24, digitsOnRight = 4, modOnRight = 10000 };
	char _asString[1 + maxDigitsOnLeft + 1 + digitsOnRight + 1];

	static unsigned requiredDigitsOnLeft( double t )
	{
	    unsigned digits = 1;
	    for ( t = (t < 0) ? -t : t; t > 1; t /= 10 )
		++ digits;
	    return digits;
	}

	char *doNegative( double &t )
	{
	    if ( t >= 0 )
		return _asString;
	    _asString[0] = '-';
	    t = -t;
	    return _asString + 1;
	}

	void hugeNumber( double t )
	{
	    char *s = doNegative( t );
	    s = numberToString( t, s, 10, 0, 1 );
	    *s++ = '.';
	    s = numberToString( t, s, 10, 1, digitsOnRight );
	    *s++ = 'E';
	    s = numberToString( requiredDigitsOnLeft( t ) - 1, s );
	}
	
	void normalNumber( double t )
	{
	    char *s = doNegative( t );
	    s = numberToString( t, s );
	    *s++ = '.';
	    numberToString( ((unsigned long)(t * modOnRight)) % modOnRight, s );
	}
	
    public:
	ValueTraits( double t )
	{
	    (requiredDigitsOnLeft( t ) > maxDigitsOnLeft) ?
		hugeNumber( t ) : normalNumber( t );
	}

	const char *asString( void ) const { return _asString; }
    };

	template<> class ValueTraits<TTime>
    //class ValueTraits<double>
    {
	char _asString[16];

	
    public:
	ValueTraits( TTime t )
	{
		TPtr8 p( (TUint8*)_asString, 16, 16);
		TDateTime dt(t.DateTime());
		p.AppendNum(dt.Year());
		p.AppendNum(dt.Month());
		p.AppendNum(dt.Day());
		p.Append(_L8("T"));
		p.AppendNum(dt.Hour());
		p.AppendNum(dt.Minute());
		p.AppendNum(dt.Second());
		p.ZeroTerminate();		
	}

	const char *asString( void ) const { return _asString; }
    };

    //
    // ValueTraits<float>
    //
	template<> class ValueTraits<float> : public ValueTraits<double>
    //class ValueTraits<float> : public ValueTraits<double>
    {
    public:
	ValueTraits( float f ) : ValueTraits<double>( f ) {}
    };


	template<> class ValueTraits<class TDesC8&>
    {
       TBuf8<KSensibleSize+1> _s;
		
    public:
        ValueTraits( const TDesC8& s ) {
			TPtrC8 truncated(s.Ptr(), Min( KSensibleSize, s.Length() ));
			_s = truncated;
			_s.ZeroTerminate();
		}
        const char* asString( void ) const { 
			return  (const char *) (_s.Ptr()); }
    };  

	template<> class ValueTraits<class TPtrC8> : public ValueTraits<class TDesC8&>
    {        
    public:
        ValueTraits( const TPtrC8 s ) : ValueTraits<class TDesC8&> ((TDesC8&)s) {}       
    };


	
	template<> class ValueTraits<class TDesC16&>
    {  
      TBuf8<KSensibleSize+1> _s;
		
    public:
        ValueTraits( const TDesC16& s ) {
			CnvUtfConverter::ConvertFromUnicodeToUtf8(_s,s);
			if (_s.Length() == KSensibleSize+1)
				_s[KSensibleSize]='\0';
			else
				_s.ZeroTerminate();
		}    

		const char* asString( void ) const { 
			return  (const char *) (_s.Ptr()); }
    };

	template<> class ValueTraits<class TPtrC16> : public ValueTraits<class TDesC16&>
    {        
    public:
        ValueTraits( const TPtrC16 s ) : ValueTraits<class TDesC16&> ((TDesC16&)s) {}       
    };

	template<> class ValueTraits<char*>
    {
        const char* _s;
    public:
        ValueTraits( char* s ) : _s(s) {			
		}
        const char* asString( void ) const { 
			return  _s; }
    };

    //
    // ValueTraits<char const *>
    // Same as char, some compilers need it
    //
	template<> class ValueTraits<char const *> : public ValueTraits<char*>
    {
    public:
		ValueTraits( char const* uc ) : ValueTraits<char*> (CONST_CAST( char*,uc)){		
	}
    };

	template<> class ValueTraits<signed char*> : public ValueTraits<char*>
    {
	 public:
		 ValueTraits( signed char* s ) : ValueTraits<char*> ((char*) s) {} 
	};

	template<> class ValueTraits<unsigned char*> : public ValueTraits<char*>
    {
	 public:
		 ValueTraits( unsigned char* s ) : ValueTraits<char*> ((char*) s) {} 
	};

}

#define TS_PRINT_AS_INTEGER( type ) \
namespace CxxTest { \
template<> class ValueTraits<type> { \
	char _asString[2 + 3 * sizeof(int)]; \
    public: \
ValueTraits( type t ) { numberToString<int>( t, _asString ); } \
const char *asString( void ) const { return _asString; } \
}; }

#endif // __CXXTEST__VALUETRAITS_H
