import compilation
import sys

compilation.printErrors( sys.argv[1] )
print "Errors " + str(compilation.errorCount( sys.argv[1] ))  
print "Warnings " + str(compilation.warningCount( sys.argv[1] ))
