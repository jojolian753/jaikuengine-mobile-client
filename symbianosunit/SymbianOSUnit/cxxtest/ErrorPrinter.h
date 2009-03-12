#ifndef __CXXTEST__ERRORPRINTER_H
#define __CXXTEST__ERRORPRINTER_H

//
// The ErrorPrinter is a simple TestListener that
// just prints "OK" if everything goes well, otherwise
// reports the error in the format of compiler messages.
//

//Changed by Penrillian Ltd 14 August 2002
//Symbian C++ does not support iostreams, so an alternative method
//for printing to the console is provided.
// www.penrillian.com

#ifndef CXXTEST_HAVE_STD
#   define CXXTEST_HAVE_STD
#endif // CXXTEST_HAVE_STD

#include <cxxtest/TestRunner.h>
#include <cxxtest/TestListener.h>
#include <cxxtest/CountingListenerProxy.h>
#include "cxxtest/TestOutput.h"
#include <e32def.h>

class CSymbianOSUnitAppView;

namespace CxxTest 
{
    class ErrorPrinter : public TestListener
    {
	private :
		CTestOutput* _cout;	
		RHeap* iTestHeap;
		RHeap* iNormalHeap;
    public:	
		static bool runAllTestsL(WorldDescription & world,HBufC* aOutputText, MUiUpdater *aUI, TInt aSuite )
		{				
			ErrorPrinter* ep = ErrorPrinter::NewL(aOutputText);
			TestRunner::runAllTests( ep,world, aUI, aSuite, ep->iTestHeap);
			bool allTestsPassed = ep->allTestsPassed();
			delete ep;
			return allTestsPassed;
		}
		
		static ErrorPrinter* ErrorPrinter::NewL(HBufC* aOutputText)
		{
			ErrorPrinter* ep = new ErrorPrinter();
			ep->CreateTestOutputL(aOutputText);
			ep->CreateTestHeapL();
			return ep;
		}

		ErrorPrinter() : _cout(0), _dotting(0){	
			_counter = new CountingListenerProxy(this);
		}
		
		void CreateTestHeapL() {
			iNormalHeap=&User::Heap();
			iTestHeap=User::ChunkHeap(0, 256*1024, 1024*1024, 16*1024);
		}

		void CreateTestOutputL(HBufC* aOutputText)
		{
			_cout = CTestOutput::NewL(aOutputText);
		}

		
		~ErrorPrinter()
		{
			delete _cout;
			delete _counter;
			if (iTestHeap) iTestHeap->Close();
			//delete iTestHeap;
		}
        TestListener *listener( void ) { return _counter; }        
		
		bool allTestsPassed( void ) const
		{
			return _counter->failedTestsNo() == 0;
		}
		
		void enterWorld( const WorldDescription &desc, TInt aSuite )
		{
			if(aSuite == KRunAllSuites)
			{
				*_cout << "Running "	<< desc.numTotalTests() << " tests ";
			}
			else
			{
				*_cout << "Running "	<< desc.suiteDescription(aSuite).name <<": " << desc.suiteDescription(aSuite).numTests << " tests ";
			}
			_dotting = true;
		}
		
		void enterTest( const TestDescription& td )
		{
			*_cout << "Running "	<< td.name << "\n";
		}

        void failedTest( const char *suiteName, const char *file, unsigned line,
			const char *expression )
        {
            failed( suiteName, file, line ) ;*_cout << "Test failed: " <<
                expression << "\n";
        }
		
		void failedAssert( const char *suiteName, const char *file, unsigned line,
			const char *expression )
		{
			failed( suiteName, file, line ) ;*_cout << "Assertion failed: "  << expression << "\n";
			leave();
		}
		
		virtual void failedAssertThrows( const char *suiteName, const char *file, unsigned line,
			const char *expression, const char *type,
			bool otherThrown ) \
        {
            failed( suiteName, file, line ) ;*_cout << "Expected (" << expression << ") to throw (" <<
                type << ") but it threw " << (otherThrown ? "something else" : "nothing") <<
                "\n";
        }
        
		void failedAssertEquals( const char *suiteName, const char *file, unsigned line,
			const char *xStr, const char *yStr,
			const char *x, const char *y )
		{
			failed( suiteName, file, line ) ;*_cout << "Expected (" <<
				xStr << " == "  << yStr  << "), found ("  <<
				x  << " != "  << y  << ")"  << "\n";
			leave();
		}
		
		void failedAssertDelta( const char *suiteName, const char *file, unsigned line,
			const char *xStr, const char *yStr, const char *dStr,
			const char *x, const char *y, const char *d )
		{
			failed( suiteName, file, line ) ;*_cout << "Expected ("  <<
				xStr  << " == "  << yStr  << ") up to "  << dStr  << " ("  << d  << "), found ("  <<
				x  << " != " << y  << ")"  << "\n";
			leave();
		}
		
		void failedAssertDiffers( const char *suiteName, const char *file, unsigned line,
			const char *xStr, const char *yStr,
			const char *x, const char *y )
		{
			failed( suiteName, file, line ) ;*_cout << "Expected ("  <<
				xStr  << " != "  << yStr  << "), found ("  <<
				x  << " == "  << y  << ")"  << "\n";
			leave();
		}
		
        virtual void failedExpectation( const char *suiteName, const char *file, unsigned line,
			const char *expected, const char *found )
        {
            failed( suiteName, file, line ) ;*_cout << "Expected \""  << expected  << "\", got \""  <<
                found  << "\""  << "\n";
			leave();
        }
        
		void leaveTest( const TestDescription& /*td*/ )
		{
			if ( !_counter->testFailed() ) {
				*_cout << ".";
				_dotting = true;
			}
		}
		
		void leaveWorld( const WorldDescription &desc, TInt aSuite )
		{
			if ( !_counter->failedTestsNo() ) {
				*_cout << "OK!" << "\n";
				return;
			}
			newLine();
			if(aSuite == KRunAllSuites)
			{
				*_cout << "Failed "  << _counter->failedTestsNo()  << " of "  << desc.numTotalTests()  << " test(s)"  << "\n";
				unsigned numPassed = desc.numTotalTests() - _counter->failedTestsNo();
				*_cout << "Success rate: "  << (numPassed * 100 / desc.numTotalTests())  << "%"  << "\n";
			}
			else
			{
				*_cout << "Failed "  << _counter->failedTestsNo()  << " of "  << desc.suiteDescription(aSuite).numTests  << " test(s)"  << "\n";
				unsigned numPassed = desc.suiteDescription(aSuite).numTests - _counter->failedTestsNo();
				*_cout << "Success rate: "  << (numPassed * 100 / desc.suiteDescription(aSuite).numTests)  << "%"  << "\n";
			}
		}
		
    private:
		void failed( const char *suiteName, const char *file, unsigned line )
		{
			newLine();
			_dotting = false;
			char* fileName = strrchr( file, '\\' );
			file = (fileName ? fileName+1 : file );
			*_cout << suiteName<<":"<<file  << ":"  << line  << ": ";
			//__DEBUGGER();
		}

		void leave()
		{
			User::Leave( KErrNone );
		}
		
		void newLine( void )
		{
			if ( _dotting )
				*_cout << "\n";
		}
		
		bool _dotting;
        CountingListenerProxy* _counter;
    };
}

#endif // __CXXTEST__ERRORPRINTER_H
