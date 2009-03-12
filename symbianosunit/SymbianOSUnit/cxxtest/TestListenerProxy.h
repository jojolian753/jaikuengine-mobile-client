/*Copyright (c) Penrillian Ltd 2003-2006. All rights reserved. Web: www.penrillian.com*/
#ifndef __CXXTEST__TESTLISTENERPROXY_H
#define __CXXTEST__TESTLISTENERPROXY_H

//
// A TestListenerProxy is a TestListener that forwards
// all notifications to another TestListener
//

//Changed by Penrillian Ltd 
//14 August 2002
// Signature of failedAssertDiffers changed.
// Also, explicit use of base class default ctor
//4 November 2002
// introduced callback for UI updating, reformatted code

#include <e32base.h>


#include <cxxtest/TestListener.h>
#include "SymbianOSUnit.h"
namespace CxxTest 
{
    class TestListenerProxy : public TestListener
    {
		TestListener   *_l;
	protected:
		MUiUpdater* _ui;
		RHeap	*iDefaultHeap;
	protected: 
		void testCompleted(){
			if (_ui)
			{				
				_ui->DisplayEachTestResult();
			}
		}
		
		void testFailed()
		{
			//__DEBUGGER();
			if (_ui)
				{				
					_ui->FailedTest();
				}
		}
			
    public:
		TestListenerProxy( TestListener *alistener, MUiUpdater * aUi ) :TestListener(),  _l( alistener ), _ui(aUi) { }
		TestListenerProxy( TestListener *alistener) :TestListener(),  _l( alistener ), _ui(NULL) { }
		
		void setListener( TestListener *alistener ) { _l = alistener; }
		
		virtual void enterWorld( const WorldDescription &desc, TInt aSuite ) {
            if ( _l ) 
				_l->enterWorld( desc, aSuite ); 
		}
		
		virtual void enterSuite( const SuiteDescription &desc ) {
			if ( _l )
				_l->enterSuite( desc ); 
		}
		
		virtual void enterTest( const TestDescription &desc ) {
			if ( _l ){			     
				_l->enterTest( desc );
			}
		}
		
        virtual void failedTest( 
			const char* suiteName, 
			const char *file, 
			unsigned   line,
			const char *expression ) { 
            if ( _l ) 
            {
            	testFailed();
				_l->failedTest( suiteName, file, line, expression ); 				
			}
		}
		
		virtual void failedAssert( 
			const char* suiteName, 
			const char *file, 
			unsigned   line,
			const char *expression ) {
			if ( _l ) 
			{
				testFailed();
				_l->failedAssert( suiteName, file, line, expression );
			}
		}
		
		virtual void failedAssertThrows( 
			const char *suiteName, 
			const char *file, 
			unsigned   line,
			const char *expression, 
			const char *type,
			bool otherThrown ) {
			if ( _l ) 
			{
				testFailed();
				_l->failedAssertThrows( suiteName, file, line, expression, type, otherThrown ); 
			}
		}
		
		virtual void failedAssertEquals( 
			const char *suiteName, 
			const char *file, 
			unsigned   line,
			const char *xStr, 
			const char *yStr,
			const char *x, 
			const char *y ) {
			if ( _l ) 
			{
				testFailed();
				_l->failedAssertEquals( suiteName, file, line, xStr, yStr, x, y );
			}
		}
		
		virtual void failedAssertDelta( 
			const char *suiteName, 
			const char *file, 
			unsigned   line,
			const char *xStr, 
			const char *yStr,
			const char *dStr,
			const char *x, 
			const char *y, 
			const char *d ) {
            if ( _l ) 
            {
            	testFailed();
				_l->failedAssertDelta( suiteName, file, line, xStr, yStr, dStr, x, y, d ); 
			}
		}
		
		virtual void failedAssertDiffers( 
			const char *suiteName, 
			const char *file, 
			unsigned   line,
			const char *xStr, 
			const char *yStr,
			const char *x,
			const char *y ) {
			if ( _l ) 
			{
				testFailed();
				_l->failedAssertDiffers( suiteName, file, line, xStr, yStr, x,y );
			}
		}
		
        virtual void failedExpectation( 
			const char* suiteName, 
			const char *file, 
			unsigned   line,
			const char *expected, 
			const char *found ) {
			if ( _l ) 
			{
				testFailed();
				_l->failedExpectation( suiteName, file, line, expected, found );
			}
		}
		
		virtual void leaveTest( const TestDescription &desc ) {
			if ( _l ) 
			{				
				_l->leaveTest( desc );
				testCompleted();
			}
		}
		
		virtual void leaveSuite( const SuiteDescription &desc ) {
			if ( _l ) 
				_l->leaveSuite( desc ); 
		}
		
		virtual void leaveWorld( const WorldDescription &desc, TInt aSuite ) {
            if ( _l ) 
				_l->leaveWorld( desc, aSuite );
		}
    };
}


#endif // __CXXTEST__TESTLISTENERPROXY_H
