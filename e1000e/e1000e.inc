#This is a makefile include file - see makefile.drv
#commas are not allowed in these definitions

PROJECT=Intel PRO/1000 PCIe MAC Driver
VERSION=0.2.5
#If SVN, then the current SVN revision number is used
FIXPACK=SVN

OBJS=$(OBJDIR)\es2lan.obj $(OBJDIR)\ich8lan.obj $(OBJDIR)\lib.obj $(OBJDIR)\phy.obj $(OBJDIR)\82571.obj $(OBJDIR)\e1000hw.obj

