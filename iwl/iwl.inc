#This is a makefile include file - see makefile.drv
#commas are not allowed in these definitions

PROJECT=Intel WiFi MAC Driver
FIXPACK=ALFA
VERSION=0.0.2

#TODO: automatically create BUILDDATE
BUILDDATE=20110713

#DBG_MODE=0 turns debug off
#DBG_MODE=1 turns debug on to serial port
#DBG_MODE=2 turns debug on to memory buffer
#ifndef DBG_MODE
DBG_MODE=2
#endif

OBJS=$(TARGET)\iwl.obj $(TARGET)\iwl_hw.obj

