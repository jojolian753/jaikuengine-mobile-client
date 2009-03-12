import autobuild
import fileutils


import datetime
import optparse
import os
import re

parser = optparse.OptionParser()
parser.add_option("-d", "--days", default="5", 
                      help="Delete directories older than DAYS")
(options, args) = parser.parse_args()

olderThan = datetime.timedelta( int(options.days) )

root = autobuild.workarea_rootdir
dateR = re.compile( '(\d{4})(\d{2})(\d{2})_\d{6}$' )

today = datetime.date.today()

for d in os.listdir( root ):
    m = dateR.search(d)
    if not m: continue

    y,m,day = m.group(1,2,3)
    created = datetime.date( int(y), int(m), int(day) )
    if today - created > olderThan:
        print "Removing " + d
        fileutils.safeRemoveTree( os.path.join(root,d) )
