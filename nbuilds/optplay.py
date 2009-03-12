import optparse

parser = optparse.OptionParser()

parser.set_defaults(svn=False)
parser.set_defaults(sdk=False)
parser.add_option("", "--svn", action="store_true")
parser.add_option("", "--sdk", action="store_true")

(options, args) = parser.parse_args()

actions = [options.svn, options.sdk]
doAll = not any(actions)

print parser.get_option( "--svn" )
print parser.get_option( "--sdk" )
 
