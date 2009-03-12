import os
import os.path
import re
import subprocess
import shutil
import socket
import errno

supportedTargets = "winscw thumb armv5 wins"
verbose = False

def runAndLog(cmd, log, errlog, shell=False):
    logfile = open( log, 'a+' )
    errlogfile = open( errlog, 'a+' )
    
    try: 
        subprocess.Popen(cmd, shell=shell, stdout=logfile, stderr=errlogfile).wait()
    except Exception, e:
        print "Error: couldn't run %s" % (cmd)
        raise e

    logfile.close()
    errlogfile.close()
    if ( os.path.exists(errlog) and os.path.getsize( errlog ) == 0 ):
        os.remove( errlog )


def checkCallAndLog(cmd, log, shell=False):
    logfile = open( log, 'a+' )
    subprocess.check_call(cmd, shell=shell, stdout=logfile, stderr=logfile)
    logfile.close()

def callAndLog(cmd, log, shell=False):
    logfile = open( log, 'a+' )
    ret = subprocess.call(cmd, shell=shell, stdout=logfile, stderr=logfile)
    logfile.close()
    return ret

def cygwinCommand(cmd):
    return 'c:\\cygwin\\bin\\bash --login -c "%s"' % (cmd)

def connectLicenseServer(outputdir):
    log = os.path.join( outputdir, "connect.log")
    cmd = cygwinCommand('ssh-add -l')
    checkCallAndLog( cmd , log )

    s = socket.socket( socket.AF_INET, socket.SOCK_STREAM)
    for i in range(0,2):
        try:
            s.connect(('localhost', 1700))
            print "License server connection is ok"
            s.close()
            break
        except socket.error, e:
            if e[0] == errno.WSAECONNREFUSED:
                print "Connecting to license server"
                connectCmd = cygwinCommand('ssh -f jaiku.com -N -L 1700:jaiku.com:1700')
                ret = callAndLog( connectCmd, log )
            else:
                raise e
        
    
#    connectCmd = cygwinCommand('ssh -f jaiku.com -N -L 1700:jaiku.com:1700')
        
#    runAndLog( 

def checkout(svnbranch, workarea, outputdir):
    log = os.path.join( outputdir, "svn.log")

    workarea_cygwinpath = workarea.replace('\\', '/')
    svn_checkout = cygwinCommand( 'svn export %s %s' % (svnbranch, workarea_cygwinpath) )
    runAndLog( svn_checkout, log, log )

def logname(build):
    return "compile_%s.log" % (build)


def errlogname(build):
    return "compile_%s_err.log" % (build)


def prepare_build(buildsdk, compiledir, outputdir):
    showinfo( "Preparing build" )
    log = os.path.join( outputdir, logname("prepare"))

    # epocroot must not have drive letter and must end with a backslash
    setEpocRoot(buildsdk)
    setEnvironment()

    os.chdir( compiledir )

    sdkname = os.path.split(buildsdk)[1]
    confcmd = os.path.join( "configuration", sdkname, "configurebuild.bat" )
    if os.path.exists( confcmd ):
        os.chdir( os.path.split(confcmd)[0] )
        cmd = os.path.split(confcmd)[1]
        checkCallAndLog( cmd, log, shell=True )

    os.chdir( compiledir )
    cmd = "call ..\\prepare_build.bat"
    runAndLog( cmd, log, log, shell=True )
    
def aftermath_build( buildsdk, compiledir, outputdir ):
    showinfo( "After build" )
    log = os.path.join( outputdir, logname("aftermath"))

    os.chdir( compiledir )    

    cmd = "call ..\\after_build.bat"
    runAndLog( cmd, log, log, shell=True )

    cmd = "call sis.bat"
    try:
        runAndLog( cmd, log, log, shell=True )
    except Exception, e:
        print e
    

    
def compilebuild(outputdir, compiledir, build):       
    cmds = {
        "wins" : ("perl ..\\run_abld_for_bldinf.pl wins udeb", False),
        "winscw" : ("perl ..\\run_abld_for_bldinf.pl winscw udeb", False),
        "thumb" : ("perl ..\\run_abld_for_bldinf.pl thumb urel", False),
        "armv5" : ("call ..\\build_arm.bat", True)
        }
    
    log = os.path.join( outputdir, logname(build) )

    os.chdir( compiledir )
    (cmd, shell) = cmds[build]
    runAndLog(cmd, log, log, shell=shell)

    
def showinfo(msg):
    if ( verbose ): print msg

                     
def compile_all(buildsdk, compiledir, outputdir):
    prepare_build( buildsdk, compiledir, outputdir )

    for b in targets:
        showinfo( "Compiling %s" % (b) )
        compilebuild(outputdir, compiledir, b)
    
    aftermath_build( buildsdk, compiledir, outputdir )

    # sis file sizes
    logbinarysizes(compiledir, outputdir)

def logbinarysizes(compiledir, outputdir):
    os.chdir( compiledir )
    errlog = os.path.join(outputdir, "binarysize_errlog.txt")
    for v,p in [ ("urel", "armv5"), ("urel", "thumb") ]:
        log = os.path.join(outputdir, "codesize%s_%s.txt" % (p,v))
        cmd = "perl ..\\object_sizes.pl jaiku.pkg %s %s" % (v, p)
        runAndLog(cmd, log, errlog)



def setEpocRoot(buildsdk):
    epocroot = os.path.splitdrive(buildsdk)[1] + "\\" 
    os.environ[ "EPOCROOT" ] = epocroot

    
import glob
def copysisfiles(compiledir, outputdir):
    os.chdir(compiledir)
    for f in glob.glob("*signed.sis"):
        shutil.copy(f, outputdir)

    
def compile_unittests(buildsdk, workarea, outputdir):
    # epocroot must not have drive letter and must end with a backslash

    cdir = os.path.join(workarea, "unittests", "group")

    log = os.path.join( outputdir, logname("unittests"))
    setEnvironment()

    #print os.environ

    os.chdir( cdir )

    cmd = "bldmake bldfiles"
    runAndLog( cmd, log, log )

    cmd = "abld build winscw udeb"
    runAndLog( cmd, log, log, shell=True )



def run_unittests(workarea, outputdir):
    log = os.path.join( outputdir, logname("unittests"))

    exepath = "epoc32\\release\\winscw\\udeb\\symbianosunit.exe"    
    cmd = os.path.join( os.environ["EPOCROOT"], exepath)
    print cmd
    if os.path.exists( cmd ):
        runAndLog( cmd, log, log )
    else:
        print "Unit tests not compiled"
    
def unittests(buildsdk, workarea, outputdir):
    setEpocRoot(buildsdk)
    compile_unittests(buildsdk, workarea, outputdir)
    run_unittests(workarea, outputdir)
    testresults = "epoc32\\winscw\\c\\logs\\SOSUnit.log"
    shutil.copy( os.path.join( buildsdk, testresults ), outputdir )
    

def setEnvironment():
    os.environ['ARMDLL']             = r"C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Bin"
    os.environ['MWCSym2Includes']    = r"C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\MSL\MSL_C\MSL_Common\Include;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\MSL\MSL_C\MSL_Win32\Include;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\MSL\MSL_C\MSL_X86;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\MSL\MSL_C++\MSL_Common\Include;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\MSL\MSL_Extras\MSL_Common\Include;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\MSL\MSL_Extras\MSL_Win32\Include;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\Win32-x86 Support\Headers\Win32 SDK"
    os.environ['MWSym2Libraries']    = r"C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\Win32-x86 Support\Libraries\Win32 SDK;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Support\Runtime\Runtime_x86\Runtime_Win32\Libs"
    os.environ['MWSym2LibraryFiles'] = r"MSL_ALL_MSE_Symbian_D.lib;gdi32.lib;user32.lib;kernel32.lib;"
    os.environ['NOKIA_LICENSE_FILE'] = r"C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\license.dat"
    os.environ['PATH']               = r"%EPOCROOT%\Epoc32\tools;%EPOCROOT%\epoc32\gcc\bin;c:\Perl\bin;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Bin;C:\Program Files\Nokia\CodeWarrior for Symbian v3.1\Symbian_Tools\Command_Line_Tools;C:\Program Files\CSL Arm Toolchain\bin;C:\Program Files\Common Files\Symbian\Tools;C:\Program Files\CSL Arm Toolchain\bin;C:\Program Files\Common Files\Symbian\Tools;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\tools\;c:\python25;C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\IDE;C:\Program Files\Microsoft Visual Studio .NET 2003\VC7\BIN;C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\Tools"
    os.environ['CPATH']              = r"%EPOCROOT%\epoc32\include"
    os.environ['CW_SYMBIAN_VERSION'] = r"3.1"
