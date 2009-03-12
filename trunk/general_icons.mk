#
# General makefile for icon MIF files
# Usage:
# Define APPNAME, AIFNAME, ICONLISTFILENAME 
# and include this file for applicaton specific makefile
# 


ifndef ICONFILE
$(error Icon file  name not defined)
endif

ifndef ICONLISTFILENAME
$(error Icon list file not defined)
endif	


include $(EPOCROOT)\epoc32\include\sdk_version.mk


ifeq (WINS,$(findstring WINS, $(PLATFORM)))
ZDIR=$(EPOCROOT)\epoc32\release\$(PLATFORM)\$(CFG)\Z
else
ZDIR=$(EPOCROOT)\epoc32\data\z
endif


ifdef __S60V3__
TARGETDIR=$(ZDIR)\resource
else
TARGETDIR=$(ZDIR)\SYSTEM\DATA
endif

HEADERDIR=$(EPOCROOT)\epoc32\include

ICONTARGETFILENAME=$(TARGETDIR)\$(ICONFILE).MIF
HEADERFILENAME=$(HEADERDIR)\$(ICONFILE).MBG

do_nothing :
	@rem do_nothing

MAKMAKE : do_nothing

BLD : do_nothing

CLEAN : do_nothing

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE : $(ICONLISTFILENAME)
	mifconv $(ICONTARGETFILENAME)  /H$(HEADERFILENAME) /F$(ICONLISTFILENAME)


FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES :
	@echo $(ICONTARGETFILENAME)

FINAL : do_nothing
