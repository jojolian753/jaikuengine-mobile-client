This is the Jaikuengine mobile client.

For code documentation look in the documentation/ subdir (it's pretty sparse).

This code will only build for S60 3rd MR.

The code uses 4-space tabs. The style is not very consistent.

It should be buildable by:
   - setting the necessary environment variables for Symbian builds
     (EPOCROOT, PATH, MW variables - we do not use devices)
   - EPOCROOT should have S60_3rd_MR in it to be parsed correctly
   - having cygwin with perl and Data::UUID installed in c:\cygwin
   - going to jaikuv3
   - running ..\prepare_build.bat
   - running ..\build_arm.bat
   - running ..\after_build.bat

cw.pl will create CodeWarrior projects.

Optional things:

To use the 2d barcodes (not currently building), put the Visual Codes code
under VisualCodeSystem.

To build Merkitys-Meaning, you need to create a file called flickr_keys.h and
define FLICKR_API_KEY and FLICKR_SHARED_SECRET strings in it.

to be added:

expat
symbianosunit
