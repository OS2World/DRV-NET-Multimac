# This source is the part of the generic ndis driver for OS/2
# Copyright (C) 2010-2012 Mensys
# Copyright (C) 2010-2012 David Azarewicz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# To build this driver you must have installed :
#    OpenWatcom
#    ZIP (must be in PATH)
#    DDK
#    OS2 Toolkit
#
# See SETENV.CMD.TEMPLATE for the environment setup

help: .symbolic
    @echo Please specify which driver to make: nveth, e1000e, r8169, or all
    @echo examples:
    @echo   wmake -a nveth
    @echo   wmake nveth e1000e
    @echo   wmake all

all: nveth r8169 e1000e

nveth: .SYMBOLIC
    cd nveth
    wmake -h $(__MAKEOPTS__)
    cd ..

e1000e: .SYMBOLIC
    cd e1000e
    wmake -h $(__MAKEOPTS__)
    cd ..

r8169: .SYMBOLIC
    cd r8169
    wmake -h $(__MAKEOPTS__)
    cd ..

iwl: .SYMBOLIC
    cd iwl
    wmake -h $(__MAKEOPTS__)
    cd ..

clean: .SYMBOLIC
    @cd nveth
    @wmake -h clean
    @cd ..
    @cd e1000e
    @wmake -h clean
    @cd ..
    @cd r8169
    @wmake -h clean
    @cd ..
    @!rm -f *.wpi
    #@cd iwl
    #@wmake -h clean
    #@cd ..
    #@!rm -f *.zip *.wpi

