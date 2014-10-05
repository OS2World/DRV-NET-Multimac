# Copyright (C) 2014 David Azarewicz
#
#
# To build this driver you must have installed :
#    OpenWatcom
#    ZIP (must be in PATH)
#    DDK
#    OS2 Toolkit
#
# See SETENV.CMD.TEMPLATE for the environment setup

!ifndef %ROOT # if not defined in the environment
!error ROOT must be set in the environment
!endif

!ifndef %DDK # if not defined in the environment
!error DDK must be set in the environment.
!endif

!ifndef %WATCOM # if not defined in the environment
!error WATCOM must be set in the environment.
!endif

TARGET=r8169

# The firmware files are not free and are not in the repository because of copyright.
# If you want to build a package that includes the firmware files, you must acquire
# them yourself, put them in the rtl_nic directory, and uncomment the following line.
#EXTRADIR=rtl_nic
ROOT=$(%ROOT)
DDK=$(%DDK)
WATCOM=$(%WATCOM)
TOOLS=$(ROOT)\tools
COMMON=$(ROOT)\common32
TARGETDIR=$(ROOT)\$(TARGET)
ZIPDIR=$(ROOT)\$(TARGET)\tmp
CURDIR=$+$(%cwd)$-

MKMSGF=mkmsgf
TRCUST=trcust
TRACEFILE=TRC00F8.TFF

%PATH=$(DDK)\base\TOOLS;$(WATCOM)\BINP;$(WATCOM)\BINW;$(WATCOM)\BINB;$(%PATH)

WIC=d:\ecs\install\warpin\wic.exe

!include $(TARGETDIR)\$(TARGET).inc

!ifeqi FIXPACK beta
WPIFILE= $(ROOT)\$(TARGET)-$(VERSION)-BETA.wpi
!else
WPIFILE= $(ROOT)\$(TARGET)-$(VERSION).wpi
!endif

$(TARGET): .SYMBOLIC
  wmake -h $(__MAKEOPTS__) -f $(TOOLS)\makefile32.drv TARGET=$(TARGET) DRIVERNAME=$(TARGET)
  wmake -h $(__MAKEOPTS__) -f $(TOOLS)\makefile32.drv TARGET=$(TARGET) DRIVERNAME=$(TARGET) TRACING=1
  wmake -h $(__MAKEOPTS__) -f $(TOOLS)\makefile32.drv TARGET=$(TARGET) DRIVERNAME=$(TARGET) DEBUG=1
  @$(TRCUST) $(TARGET).tsf
  @$(MKMSGF) $(TARGET).acs  $(TARGET).msg
  #@$(MKMSGF) $(TARGET)h.acs $(TARGET)h.msg
  @!rm -rf $(ZIPDIR)
  @md $(ZIPDIR)
  @md $(ZIPDIR)\MACS
  @copy $(TARGETDIR)\retail\$(TARGET).os2 $(ZIPDIR)\MACS >NUL
  @copy $(TARGETDIR)\retail\$(TARGET).sym $(ZIPDIR)\MACS >NUL
  @copy $(TARGETDIR)\retail\$(TARGET).nif $(ZIPDIR)\MACS >NUL
  @copy $(TARGETDIR)\$(TARGET).txt $(ZIPDIR)\MACS >NUL
  @copy $(TARGETDIR)\$(TARGET).msg $(ZIPDIR) >NUL
  #@copy $(COMMON)\LICENSE $(ZIPDIR) >NUL
! ifdef EXTRADIR
  @md $(ZIPDIR)\$(EXTRADIR)
  @copy $(TARGETDIR)\$(EXTRADIR)\* $(ZIPDIR)\$(EXTRADIR) >NUL
! endif
  @-del $(WPIFILE)
  @$(WIC) $(WPIFILE) -a 1 -r -c$(ZIPDIR) * # -u
  @!rm -rf $(ZIPDIR)
  @md $(ZIPDIR)
  @md $(ZIPDIR)\IBMCOM
  @md $(ZIPDIR)\IBMCOM\MACS
  @md $(ZIPDIR)\OS2
  @md $(ZIPDIR)\OS2\SYSTEM
  @md $(ZIPDIR)\OS2\SYSTEM\TRACE
  @copy $(TARGETDIR)\trace\$(TARGET).os2 $(ZIPDIR)\IBMCOM\MACS >NUL
  @copy $(TARGETDIR)\trace\$(TARGET).sym $(ZIPDIR)\IBMCOM\MACS >NUL
  @copy $(TARGETDIR)\$(TRACEFILE) $(ZIPDIR)\OS2\SYSTEM\TRACE >NUL
  @$(WIC) $(WPIFILE) -a 2 -r -c$(ZIPDIR) *
  @sed -e "s/\\X\\X\\X/\\$(VERSION:.=\\)/" $(TARGETDIR)\$(TARGET).wis >$(ZIPDIR)\$(TARGET).wis
  @$(WIC) $(WPIFILE) -s $(ZIPDIR)\$(TARGET).wis
  @!rm -rf $(ZIPDIR)

$(TRACEFILE): $(TARGET).tsf
  $(TRCUST) $[@

clean: .SYMBOLIC
  @!rm -rf debug retail trace
  @!rm -f version.h
  @!rm -f TRC00F8.TFF
  @!rm -f $(TARGET).msg

