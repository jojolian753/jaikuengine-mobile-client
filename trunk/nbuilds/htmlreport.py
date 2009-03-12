import buildresult
import jbuild

def report(bresults, file):
    r = """
    <html><head><title>Build %s</title></head>
    <body>    
    <h1> Build %s </h1>
    """ % (bresults.dir, bresults.dir)

    r += compilation(bresults)
    r += unittest(bresults)
    r += """
    </body>
    </html>
    """

    f = open(file, 'w')
    f.write(r)
    f.close()
    

def unittest(bresults):
    r = ""

    r += """
    <h2> Unit tests </h2>
    """
    if bresults.unittests:
        r += """
        %d / %d  ( %f %% ) <br>
        <a href=\"SOSUnit.log\">Log file</a>
        """ % ( bresults.fails, bresults.tests, bresults.pct())
    else:
        r += "No unit test results"
    return r

def compilation(bresults):
    r = ""
    
    r += """
    <h2> Compilation </h2>
    <table>
    <tr>  <th>Build</th>   <th>Errors</th>  <th>Warnings</th> <th>Log file</th></tr>
    """
    
    for b,cur in bresults.builds:
        log = jbuild.logname(b)
        e,w,p = cur
        r += """
        <tr>
        <th>%s</th>
        <td>%d</td>
        <td>%d</td>
        <td><a href=\"%s\">%s</a></td>
        </tr>
        """ % ( b, e, w, log, log )
    r += "</table>"
    return r

