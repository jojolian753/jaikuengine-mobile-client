import os
import os.path
import shutil
import stat

def safeRemoveTree(path):
    shutil.rmtree( path, True )
    if os.path.exists( path ):
        # change permission for rest and remove them

        for root, dirs, files in os.walk(path, topdown=False):
            for f in files:
                os.chmod( os.path.join( root, f ), stat.S_IWRITE )
            for d in dirs:
                os.chmod( os.path.join( root, d ), stat.S_IWRITE )    
        shutil.rmtree( path )

def ensurePathExists(path):
    try:
        os.makedirs( path )
    except os.error, e:
        if e.errno != 17: # already exists
            raise e

