# Create build script:

#     * Input: SDK, Jaiku source branch
#     * Output: SIS file, compiled version for emulator, compilation error & warning logs, (unit test results), (binary size of client) 

#    1. create output dir for build. Format SVNBRANCH_SDK_TSTAMP
#    2. delete whole SDK directory
#    3. delete whole Jaiku source tree
#    4. clone fresh SDK directory
#    5. run dependency installation script on top of fresh SDK
#    6. export Jaiku source branch from svn
#    7. run compilation scripts, redirect compilation logs to output_dir/compilation.log
#    8. run unit tests, redirect output to output_dir/unittest.log
#    9. copy SIS file to output_dir
#   10. output binary size of client dlls to output_dir/binarysize.log
#   11. (Provide summary to web page / RSS feed / email) 

import jbuild
import fileutils
import buildresult
import htmlreport

import datetime
import optparse
import os
import os.path
import re
import shutil
import stat
import subprocess
import sys


# Set variables
source_drive  = "k:\\"
work_drive    = "e:\\" 

# output and workareas
output_rootdir = os.path.join( work_drive, "output")
workarea_rootdir = os.path.join( work_drive, "workarea")

def main():
    # Parse options 

    parser = optparse.OptionParser()
    
    steps = [ "getsdk", "compile", "unittest", "report", "summary" ];
    for s in steps:
        parser.add_option("", "--" + s, action="store_true", default=False)
    parser.add_option("-w", "--workarea", dest="workarea",
                      help="Use given WORKAREA instead of newly checkedout workarea")
    parser.add_option("-b", "--branch", default="trunk",
                      help="Checkout branch BRANCH from svn")
    parser.add_option("-s", "--sdk", default="Symbian\\9.1\\S60_3rd_MR",
                      help="Checkout branch BRANCH from svn")
    parser.add_option("-d", "--builddir", default="jaikuv3",
                      help="Build script directory that is used")    
    parser.add_option("-v", "--verbose", action="store_true", default=False)
    parser.add_option("", "--clean", action="store_true", default=False,
                      help="Clean old workareas and output dirs")
    parser.add_option("-t", "--targets", default="winscw thumb")
    (options, args) = parser.parse_args()


    # INPUT (run specific)
    # branch
    jaikubranch = options.branch

    # sdk 
    sdkname=os.path.split(options.sdk)[1]
    buildsdk = os.path.join( work_drive, options.sdk )
    freshsdk = os.path.join( source_drive, options.sdk )

    # compilation dir
    compilation_subdir = options.builddir
        
    # svn 
    svn_repository = "svn+ssh://kaksi.org/svn/mobile"
    svnbranch = '/'.join( [svn_repository, jaikubranch] ) 

    
    tstampDt = datetime.datetime.utcnow()
    tstamp = tstampDt.strftime( "%Y%m%d_%H%M%S" );

    # targets
    targets = options.targets.split()
    for t in targets:
        if not t in jbuild.supportedTargets:
            print "Target %s not supported" % (t)
            sys.exit(1)
    jbuild.targets = targets
    jbuild.verbose = options.verbose

    
    if options.workarea:
        workarea = options.workarea
        branchname = os.path.split(workarea)[1].split('_')[0]
        buildname = os.path.split(workarea)[1]
    else:
        branchname = jaikubranch.replace('/', '_')
        buildname = "_".join( [branchname, sdkname, tstamp] )
        workarea = os.path.join( workarea_rootdir, buildname )

    outputdir = os.path.join( output_rootdir, buildname )

    # enable all steps
    stepEnables = [ getattr(options, s) for s in steps]    
    if not any(stepEnables):
        # enable all
        for s in steps: setattr(options, s, True)


    def showinfo(msg):
        if ( options.verbose ): print msg
    

    if options.clean:
        showinfo( "Cleaning old workareas" )
        fileutils.safeRemoveTree(workarea_rootdir)
        showinfo( "Cleaning old output dirs" )
        fileutils.safeRemoveTree(output_rootdir)

    fileutils.ensurePathExists(output_rootdir)

    
    if not options.workarea:
        showinfo( "Creating output directory" )
        os.makedirs( outputdir ) 

    jbuild.connectLicenseServer(outputdir)
    

    if not options.workarea:
        showinfo( "Checking out workarea" )
        #  create workarea for Jaiku source tree
        #  export Jaiku source branch from svn
        fileutils.ensurePathExists( workarea_rootdir )    
        jbuild.checkout(svnbranch, workarea, outputdir)
    
    if options.getsdk:
        showinfo( "Removing existing sdk" )
        fileutils.safeRemoveTree( buildsdk )
        showinfo( "Copying clean sdk" )
        shutil.copytree( freshsdk, buildsdk )
               

    if options.compile:
        showinfo( "Compiling all" )
        compiledir = os.path.join( workarea, compilation_subdir )
        jbuild.compile_all( buildsdk, compiledir, outputdir )
        showinfo( "Copying sis files" )
        try:
            jbuild.copysisfiles( compiledir, outputdir )
        except Exception, e:
            print e
            #just go on

    if options.unittest:
        showinfo( "Compiling & running unit tests" )
        try:
            jbuild.unittests( buildsdk, workarea, outputdir )
        except Exception, e:
            print e
            # just go on

    # #   10. output binary size of client dlls to output_dir/binarysize.log
    

    #   11. (Provide summary to web page / RSS feed / email)
    if options.report:
        showinfo( "Creating reports" )
        #jbuild.printErrors( outputdir )
        results = buildresult.BuildResult(outputdir, targets)
        results.printSummary()
        htmlreport.report(results, os.path.join(outputdir,"report.html"))
  #      print "Errors: " + str( jbuild.errorCount( outputdir ) )
  #      print "Warnings: " + str( jbuild.warningCount( outputdir ) )

    if options.summary:
        showinfo( "Appending to summaries" )

        results = buildresult.BuildResult(outputdir, targets)
        
        summary_dir = os.path.join(work_drive, "summary")
        summary_file = os.path.join(summary_dir, "%s_%s.txt" % (branchname, sdkname))
        f = open( summary_file, 'a+')
        s = results.statString()
        f.write( s + '\n')
        f.close()
        
    return 0

if __name__ == "__main__":
    sys.exit(main())
