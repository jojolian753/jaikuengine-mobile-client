#!perl -w
# Changed by Penrillian 2006
# www.penrillian.com
# always use default error printer (so option removed)
# move static data to local to enable use from Symbian OS app

use strict;
use Getopt::Long;

@::suiteTestCount = ();
$::i = -1;

sub usage() 
{
	print STDERR "Usage: $0 [-o <output file>] <input file> [<input file>...]\n";
	exit -1;
}

main();

sub main
{
	parseCommandline();
	@ARGV = expandWildcards( @ARGV );
    createOutputFile();
    writeHeader();
    scanInputFiles();
    writeTestDriver();
	close OUTPUT_FILE;
}

my ($outputFile);
my %TestTableForEachSuite;
my (@suites, $suite, $test, $numSuites, $numTotalTests);
my $TestObjects = "";

sub parseCommandline()
{
	GetOptions( 'output=s' => \$outputFile,)|| usage();
	
	scalar @ARGV || usage();
}

sub expandWildcards() 
{
	my @result = ();
	while( my $fn = shift @_ ) 
	{
		push @result, glob($fn);
	}
	return @result;
}

sub createOutputFile() 
{
	return unless $outputFile;
	open OUTPUT_FILE,">$outputFile" || die("Cannot create output file \"$outputFile\"");
	select OUTPUT_FILE;
}

#Writes the header and the include files.

sub writeHeader()
{
	print<<__EOF;
/* Generated file, do not edit */
#ifndef TESTDRIVER 
#define TESTDRIVER
#define CXXTEST_HAVE_EH
#define CXXTEST_RUNNING
#include <cxxtest/TestListener.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/ErrorPrinter.h>
	
__EOF
		
	foreach (@ARGV) 
	{
		print "#include \"$_\"\n";
    }
	
}

# Scan all the input files for suites and tests:
# Write out the test objects:

sub scanInputFiles() 
{
	foreach my $file (@ARGV) 
	{
		scanInputFile($file);
	}
}

sub scanInputFile
{
	my $file = $_[0];
	open INPUT_FILE,"<$file" || die("Cannot create output file \"$outputFile\"");
	while(<INPUT_FILE>)
	{
		if ( /\bclass\s+(\w+)\s*:\s*public\s+((::)?\s*CxxTest\s*::\s*)?TestSuite\b/ ) 
		{
			startNewSuite( $1 );
			$::i = $::i+1;
			$::suiteTestCount[$::i] = 0;
		}
		if ( /\bclass\s+(\w+)\s*:\s*public\s+((::)?\s*CxxTest\s*::\s*)?Expecter\b/ ) 
		{
			startNewSuite( $1 );
			$::i = $::i+1;
			$::suiteTestCount[$::i] = 0;
		}
		if ( $suite ) 
		{
			if ( /\bvoid\s+([Tt]est\w+)\s*\(\s*(void)?\s*\)/ ) 
			{
				$test = $1;
				dumpTest($file);
				++ $numTotalTests;
				$::suiteTestCount[$::i] += 1;
			}
		}
	}
	close INPUT_FILE;
} 


# Replace "\" with "\\" in the given string.
sub cstr($) 
{
	my $result = $_[0];
	$result =~ s/\\/\\\\/g;
	return $result;
}

# Start a new suite.
# Parameter: The suite name (string).

sub startNewSuite($) 
{
	$suite = $_[0];
	$TestTableForEachSuite{$suite} = "";
	push @suites, $suite;
}

#Writes out the runner class for $test
#Also adds the test table entries in TestTableForEachSuite.

sub dumpTest() 
{
	my $file = $_[0]; # Name of current file when reading from <>
	my $line = $.;    # Current line number
	print <<__EOT__;
	class Class_${suite}_${test}:public CxxTest::Runnable 
	{
	public:
		Class_${suite}_${test}(${suite}& aSuite_${suite}):suite_${suite}(aSuite_${suite}){}
		void run()
		{
			suite_${suite}.$test();
		}
	private:
		${suite}& suite_${suite};
	};
__EOT__
		
	$TestTableForEachSuite{$suite} .= "," if ($TestTableForEachSuite{$suite} ne "");
	$TestTableForEachSuite{$suite} .= "\n\t\t\t\t{ \"".cstr($file)."\",$line,\"$suite\"";
	$TestTableForEachSuite{$suite} .= ",\"$test\", \&object_${suite}_${test}}";
	
	$TestObjects .= "\t\t\tClass_${suite}_${test} object_${suite}_${test}(suite_${suite});\n";
}


#Write the world description and test driver classes

sub writeTestDriver() 
{
	($numSuites = scalar @suites) > 0 || die 'No tests defined!';
	
	print<<__EOT__;
	
	class class_WorldDescription : public CxxTest::WorldDescription
	{
	public:
		class_WorldDescription(CxxTest::SuiteDescription* aSuites):suites(aSuites){}
		signed int numSuites() const { return $numSuites; }
		unsigned numTotalTests() const { return $numTotalTests; }
		const CxxTest::SuiteDescription &suiteDescription( unsigned i ) const { return suites[i]; }

	private:
		CxxTest::SuiteDescription* suites;
	protected:
		void setUp( CxxTest::TestListener* /*l*/ ) const {}
		void tearDown( CxxTest::TestListener* /*l*/ ) const {}
	};
	
	/\*\* Wrapper for function runTestL, which does all the work.
		\*/
	class TestDriver
	{
    public:
		HBufC*  iTestOutput;
		MUiUpdater* iUI;
		
		TestDriver(HBufC* aTestOutput, MUiUpdater* aUI): iTestOutput(aTestOutput), iUI(aUI){}
		
		void runAllSuitesL(TInt aSuite = KRunAllSuites)
        {
__EOT__
				
			foreach $suite (@suites) 
			{
				print "\t\t\t$suite suite_${suite}(_L8(\"$suite\"));\n";
			}
			
			print $TestObjects;
			
			foreach $suite (@suites) 
			{
				print <<__EOT__;
				const CxxTest::TestDescription tests_$suite\[\] = 
				{
					$TestTableForEachSuite{$suite}
				};
__EOT__
			}
			
			print "\t\t\tCxxTest::SuiteDescription suites[] =
			\t\t\t{\n";
			my $continuation = "";
			$::i = 0;
			foreach $suite ( @suites ) 
			{
				print $continuation, "		{ \"$suite\", \&suite_$suite, $::suiteTestCount[$::i++], tests_$suite }";
				$continuation = ",\n";
			}
			
			print <<__EOT__;
			
			};
			
            class_WorldDescription object_world(suites);
            CxxTest::ErrorPrinter::runAllTestsL(object_world, iTestOutput, iUI, aSuite);
        }
__EOT__
		print <<__EOT__;
		static void TestSuiteNamesL(CDesCArray& aSuiteNames)
		{
__EOT__
			foreach $suite (@suites) 
			{
				print "\t\t\taSuiteNames.AppendL(_L(\"$suite\"));\n";
			}
			
			print <<__EOT__;
		}
		
	};
#endif
__EOT__
}

