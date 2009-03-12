#ifndef __CXXTEST__EXPECTER_H
#define __CXXTEST__EXPECTER_H

#include <cxxtest/TestSuite.h>

namespace CxxTest
{
    class Expecter : public TestSuite
    {
    public:
        Expecter() : TestSuite(), _expected( "" ) {}

    protected:
        void expect( const char *expected )
        {
            _expected = fixNull( expected );
        }

        void found( const char *file, unsigned line, const char *foundString )
        {
            foundString = fixNull( foundString );
            if ( !stringsEqual( _expected, foundString ) )
                failedExpectation( file, line, _expected, foundString );
            nextExpected();
        }
        
#       define TS_EXPECT(ss) expect(ss)
#       define TS_FOUND(f) found(__FILE__, __LINE__, f )
#       define TS_END() found(__FILE__, __LINE__, "" )
        
    private:
        const char *_expected;

        static const char *fixNull( const char *str )
        {
            return str ? str : "";
        }

        static bool stringsEqual( const char *s1, const char *s2 )
        {
            char c;
            while ( (c = *s1++) == *s2++ )
                if ( c == '\0' )
                    return true;
            return false;
        }

        void nextExpected( void )
        {
            if ( *_expected == '\0' )
                return;
            while ( *_expected++ != '\0' )
                ;
        }
    };
};

#endif // __CXXTEST__EXPECTER_H
