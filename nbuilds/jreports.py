def report(buildreport):
    r = """
    <html><head><title>Build %s</title></head>
    <body>    
    <h1> Build %s </h1>
    """ % (buildreport.dir, buildreport.dir)

    r += compilation(buildreport)

    r += """
    <h2> Unit tests <h2>
    <h2> Binary size <h2>
    <h2> </h2>
    </body>
    </html>
    """

    print r


def compilation(buildreport):
    r = ""
    
    r += """
    <h2> Compilation </h2>
    <table>
    <tr>  <th>&nbsp;</th>   <th>Errors</th>  <th>Warnings</th> </tr>
    """
    
    for b in buildreport.targets:
        r += """
        <tr>
        <th>%s</th>
        <th>%s</th>
        <th>%s</th>
        </tr>
        """ % ( b, warnings, 0)


    r += "</table>"
    return r
