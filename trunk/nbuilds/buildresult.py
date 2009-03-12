import jbuild
import operator
import os
import os.path
import re


class BuildResult:
    def __init__(self, outputdir, targets):
        self.dir = outputdir
        self.targets = targets

        # this is the beef, actual results
        self.builds = [] # (b, (errors, warnings, problems))
        self.totals = (-1,-1,-1) # (errors, warnings, problems)
        self.unittests = unittest_results(self.dir) # (None, 
        if self.unittests: self.fails,self.tests = self.unittests
        self.sissize = self.sissize()
         
        self.calcCompilation()

        dateR = re.compile( '(\d{8})_(\d{6})$' )
        m = dateR.search( outputdir )
        self.tstamp = m.group(1) + m.group(2)
        
#        f = open(os.path.join(outputdir, "buildinfo.txt"), 'r')
#        tstampRe = re.compile('
#        self.tstamp = tstampDt

    def statString(self):
        # sis size, unit test fails, unit tests run, total errors, total warnings, winscw errors, winscw warnings, thumb errors, thumb warnings, armv5 errors, armv5 warnings
        tstamp = self.tstamp
        sissize = self.sissize
        testsFail = self.fails
        testsRun= self.tests
        totalErrors = self.totals[0]
        totalWarnings = self.totals[1]

        def getBuildStats(build):
            for b in self.builds:
                if b[0] == build: return b[1]
            return (-1,-1)


        winscwErrors, winscwWarnings, ignore = getBuildStats('winscw')
        armv5Errors, armv5Warnings, ignore = getBuildStats('armv5')
        thumbErrors, thumbWarnings, ignore = getBuildStats('thumb')
        
        s = "%s" % (tstamp)
        for i in (sissize, testsFail, testsRun, totalErrors, totalWarnings, winscwErrors, winscwWarnings, thumbErrors, thumbWarnings, armv5Errors, armv5Warnings):
            s += "\t%d" % (i)
        return s
        
        
    def calcCompilation(self):        
        g = lambda r, b: self.getSomething( r, self.dir, b)
        totals = (0,0,0)
        for b in self.targets:            
            err = len(g(errorRegex(), b))
            war = len(g(warningRegex(), b))
            pro = len(g(problemRegex(), b))
            cur = (err,war,pro)
            self.builds.append( (b, cur) )
            totals = tuple(map(operator.add, cur, totals))
        self.totals = totals    

    def sispath(self):
        return os.path.join( self.dir, 'jaiku_armv5-signed.SIS' )

    def sissize(self):
        sispath = self.sispath()
        if os.path.exists( sispath ):
            return os.path.getsize(sispath)
        else:
            return 0

    def pct(self):
        if self.unittests: 
            if self.tests > 0:
                return ((self.tests - self.fails) / float(self.tests)) * 100
            else:
                return 0.0
        else:
            return -1.0
        
        
    def printSummary(self):
        for (b,cur) in self.builds:
            print "%s:\t" % (b),
            print "%d errors, %d warnings, %d problems" % cur
            
        print "Total:\t%d errors, %d warnings, %d problems" % (self.totals)
        if self.unittests:
            print "Unit tests: %d / %d (%.1f%%)" % (self.fails,self.tests,self.pct())
        else:
            print "No unit test results"
#    def logStats(self, statsdir):
#         for (t,cur) in self.builds:
#             if t == target:
#                 f = open( os.path.join(stats_dir, "compilation" + t + ".log"),'a+')
#                 # errors warnings (ut pass) (ut tests) (ut pct) binary size
#                 f.write("%s\t%d\t%d\n" % (tstamp, cur[0], cur[1]) 
#         f = open( os.path.join(stats_dir, "compilation" + t + ".log"),'a+')
#         f.write( (tstamp, self.tests, self.fails, self.pct())

    def forAllBuilds(self, f):
        result = []
        for b in self.builds:
            result += f(b)          
        return result
    
    def getErrors(self, outputdir):
        return self.forAllBuilds( lambda b: self.getSomething( errorRegex(), outputdir, b) )

    def getWarnings(self, outputdir):
        d = self.forAllBuilds( lambda b: self.getSomething( warningRegex(), outputdir, b) )
        return d
                         
    def getProblems(self, outputdir):
        return self.forAllBuilds( lambda b: self.getSomething( problemRegex(), outputdir, b) )
    
    def errorCount(self, outputdir):
        return len( self.getErrors(outputdir) )
    
    def warningCount(self, outputdir):
        return len( self.getWarnings(outputdir) )
    
    def problemCount(self, outputdir):
        return len( self, self.getProblems(outputdir) )
    
    def printErrors(self, outputdir):
        print "------------------------- " 
        
        for e in self.getErrors(outputdir):
            print e,
        print "------------------------- "



    def getSomething(self, somethingR, outputdir, build):
        result = []
        f = open( os.path.join(outputdir, jbuild.logname(build)), 'r' )
        for l in f.readlines():
            if somethingR.search(l):
                result.append(l)
        return result


def problemRegex():
    # problematic line
    problemR = re.compile(r"""
    ^                  # start of line
    ([./\\A-Za-z0-9_]+) # a path to file
    :(\d+):            # line number
    (.*)               # error report 
    """, re.VERBOSE)
    return problemR


def errorRegex(): 
    errR = re.compile("""
    ([^?]\berror(?!(s|\w*\.(cpp|h))))
    |
    (error\s.\d+:) # wins errors
    |
    (^[^:]*:\d+:\serror:\s.*)
    |
    (^ERROR: .*) # Symbian build tool errors
    """, re.VERBOSE | re.IGNORECASE)
    return errR

def warningRegex():
    warningR = re.compile("""
    (^[^:]*:\d+:\swarning:\s.*)
    |
    (warning\s.\d+:) # wins warnings
    |
    (^WARNING: .*) # Symbian build tool warnings 
    """, re.VERBOSE | re.IGNORECASE)
    return warningR



        
def unittest_results(outputdir):
    logfile = os.path.join( outputdir, "SOSUnit.log" )
    if os.path.exists( logfile ):
        resultsR = re.compile("^Failed (\d+) of (\d+)")
        #""", re.VERBOSE)
        
        f = open(logfile)
        for line in f:
            m = resultsR.search(line)
            if m:
                fails = int(m.group(1))
                tests = int(m.group(2))
                return (fails, tests)
    return None


