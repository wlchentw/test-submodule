#!/usr/bin/env python
#
# This is a module that gathers a list of serial ports on POSIXy systems.
# For some specific implementations, see also list_ports_linux, list_ports_osx
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2011-2015 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

"""\
The ``comports`` function is expected to return an iterable that yields tuples
of 3 strings: port name, human readable description and a hardware ID.

As currently no method is known to get the second two strings easily, they are
currently just identical to the port name.
"""

import glob
import sys
import os
from pyserial.tools import list_ports_common

# try to detect the OS so that a device can be selected...
plat = sys.platform.lower()

if plat[:5] == 'linux':    # Linux (confirmed)  # noqa
    from pyserial.tools.list_ports_linux import comports

elif plat[:6] == 'darwin':   # OS X (confirmed)
    from pyserial.tools.list_ports_osx import comports

elif plat == 'cygwin':       # cygwin/win32
    # cygwin accepts /dev/com* in many contexts
    # (such as 'open' call, explicit 'ls'), but 'glob.glob'
    # and bare 'ls' do not; so use /dev/ttyS* instead
    def comports():
        devices = glob.glob('/dev/ttyS*')
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:7] == 'openbsd':    # OpenBSD
    def comports():
        devices = glob.glob('/dev/cua*')
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:3] == 'bsd' or plat[:7] == 'freebsd':

    def comports():
        devices = glob.glob('/dev/cua*[!.init][!.lock]')
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:6] == 'netbsd':   # NetBSD
    def comports():
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/dty*')
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:4] == 'irix':     # IRIX
    def comports():
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/ttyf*')
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:2] == 'hp':       # HP-UX (not tested)
    def comports():
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/tty*p0')
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:5] == 'sunos':    # Solaris/SunOS
    def comports():
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/tty*c')
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:3] == 'aix':      # AIX
    def comports():
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/tty*')
        return [list_ports_common.ListPortInfo(d) for d in devices]

else:
    # platform detection has failed...
    import pyserial as serial
    sys.stderr.write("""\
don't know how to enumerate ttys on this system.
! I you know how the serial ports are named send this information to
! the author of this module:

sys.platform = %r
os.name = %r
pySerial version = %s

also add the naming scheme of the serial ports and with a bit luck you can get
this module running...
""" % (sys.platform, os.name, serial.VERSION))
    raise ImportError("Sorry: no implementation for your platform ('%s') available" % (os.name,))

# test
if __name__ == '__main__':
    for port, desc, hwid in sorted(comports()):
        print("%s: %s [%s]" % (port, desc, hwid))
