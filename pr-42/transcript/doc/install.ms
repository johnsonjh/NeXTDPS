.na
.nr PS 12
.nr VS 14
.DA "24 July 1987"
\" Copyright (c) 1985,1987 Adobe Systems Incorporated. All Rights Reserved.
\" GOVERNMENT END USERS: See notice of rights in Notice file in TranScript
\" library directory -- probably /usr/lib/ps/Notice
\" RCSID: $Header: install.ms,v 2.2 87/11/17 16:01:50 byron Rel $
.ds PS P\s-2OST\s+2S\s-2CRIPT\s+2
.ds TS T\s-2RAN\s+2S\s-2CRIPT\s+2
.de IR
\fI\\$1\fP\\$2
..
.TL
\*(TS 2.1
.br
Installation Instructions
.AU
Adobe Systems Incorporated
.AI
1585 Charleston Road
P.O. Box 7900
Mountain View, CA \ \ 94309-7900
.NH
Installation
.PP
This document is the \*(TS installation guide.
It is, of necessity, fairly detailed and 
assumes that the reader/installer has a working knowledge of
UNIX system organization (e.g., the locations and contents of
a few system files, etc.).  On almost all UNIX systems, installation
of the \*(TS package will require super-user (root) privileges.
Please read these instructions thoroughly before installing \*(TS.
.PP
If the reader has no experience with the UNIX line printer spooling
system (lpr/lpd/printcap on 4.2BSD, lp/lpsched on System V),
please take the time to read the 
documents and man pages for the spooling system.
.LP
On 4.2BSD systems, look at 
lpr(1), lpq(1), lprm(1), printcap(5), lpc(8), lpd(8), and pac(8)
and \fI4.2BSD Line Printer Spooler Manual\fP
in volume 2C of the UNIX documentation.
.LP
On System V systems, look at
lp(1), lpstat(1), lpadmin(1M), lpsched(1M), accept(1M), enable(1)
and \fIUNIX System V Line Printer Spooling Utilities\fP.  
.sp 4
.LP
.SM
\*(PS is a registered trademark of Adobe Systems Incorporated.
.br
\*(TS is a trademark of Adobe Systems Incorporated.
.br
UNIX is a trademark of AT&T Bell Laboratories.
.br
Documenter's Workbench is a trademark of AT&T Technologies.
.br
Apple and LaserWriter are trademarks of Apple Computer.
.br
QMS and LaserGrafix are registered trademarks of QMS Incorporated.
.br
Dataproducts is a registered trademark of Dataproducts Incorporated.
.br
Linotype and Linotronic are trademarks of LinoType.
.br
VAX and Ultrix are trademarks of Digital Equipment Corporation.
.br
Sun Workstation is a trademark of Sun Microsystems Incorporated.
.br
Times and Helvetica are registered trademarks of Linotype.
.br
Scribe and U\s-1NILOGIC\s+1 are registered trademarks of U\s-1NILOGIC\s+1, Ltd.
.br
Diablo Systems is a Xerox company.
.br
Tektronix is a registered trademark of Tektronix, Inc.
.NL
.NH 2
Unload and Configure
.PP
Unload the contents of the distribution tape or diskettes.
To do this, 
.I cd
to some directory of your choice.
Make sure the the current directory (.) is on your search path.
If you have a 
.I tar
tape, type
.DS
tar x
.DE
If you have received a 
.I cpio
diskette, type
.DS
cpio \-iBd </dev/rdiskette
.DE
This will create a master directory called 
.I transcript,
with subdirectories as noted in the \*(TS overview.
.I cd 
to the 
.I transcript
directory.  If you are on a 4.2BSD system, run the \*Qbsd\*U shell
script by typing
.DS
bsd
.DE
If you are on a System V system, run the \*Qsysv\*U shell
script by typing
.DS
sysv
.DE
These rename a few files that are necessary for installation on
those systems.
.LP
.B
Please Note:
.R
If you have received the 3B Binary distribution of \*(TS
and are installing \*(TS on a machine \fIwithout\fP a \*QC\*U 
compiler and \*QMake\*U, go immediately to section 1.3 of this 
document!
.PP
Carefully examine the 
files \*Qconfig\*U and \*Qprinter\*U in the top-level
directory and make whatever modifications you see fit. 
These two files in the top-level directory determine where and
how the \*(TS software will be installed.
The software and Makefiles included in this distribution have
been configured so that you can change where the files reside
for installation.  
There should be enough information in those files to help
you determine how you might want to change them.
In the default distribution configuration,
the user-executable binaries and shell scripts go in
.IR /usr/bin ,
the library files goes in
.IR /usr/lib/ps ,
and manual pages go under
.IR /usr/man/man? .
On 4.2BSD systems, the spool directories are under 
.IR /usr/spool ,
and the printer log and accounting files are in
.IR /usr/adm .
On System V systems, the log files go under
.IR /usr/spool/lp/transcript .
.PP
If you decide you want other locations, you must edit
the \*Qconfig\*U and \*Qprinter\*U files to change the 
directory names.  
Under 4.2BSD, another possibility is to change the file locations by using
symbolic links. If you would prefer that the actual
files be located under /usr/local/lib/transcript, you can do
the following:
.DS
ln \-s /usr/local/lib/transcript /usr/lib/ps
.DE
.PP
.B
Warning about font installation:
.R
If the entire set of \fItroff\fP and \fIditroff\fP fonts is installed,
the font data bases will take up approximately 1.3 MB of disk space and
2700 hard links.
It can also take a couple hours to install on a slow system.
Set DITROFFFONTS, TROFFFONTS and FONTFAMILIES carefully.
There is more info about how to estimate the disk space an installation
will take in the Appendix on font installation.
.NH 2
Software Installation
.PP
Now configure and install the \*(TS software.  (You only do this 
once on each host running \*(TS.)  In the main directory, do a
.DS
make programs
.DE
This will take some time.  It will configure the \*(TS software for your site.
If this goes smoothly, you can then
.B
become super-user 
.R 
and do a 
.DS
make install
.DE
This also takes time.
This installs the programs, library files, and manual pages in the system
directories according to the settings in the \*Qconfig\*U file,
as noted above.
.PP
.B N.B.
The installation procedure for the \fIditroff\fP
(and Documenter's Workbench)
font metrics files requires a program called 
.B makedev
which is part of the ditroff distribution.  
Since not all systems have ditroff, this section of the installation
may be avoided by assigning DITROFFFONTS=0 in the \*Qconfig\*U
file.  Moreover, the Documenter's Workbench binary distribution
for AT&T 3B2 computers does not contain the makedev program.
(This is a glaring error on the part of AT&T; contact your AT&T service
representative to obtain a copy of the makedev program!)  
.PP
The \*(TS
software is now installed.  The next step is to install a printer
into the spooling system.
.NH 2
Binary Installation on a non-programmer's 3B
.PP
The binary distribution for System V AT&T 3B2/3B5 has
additional structure for those with a minimal UNIX system
configuration.  If you do not have a \*QC\*U compiler and Make,
you can install the complete binaries.  In this configuration,
however, you do not have a choice as to where the various
binary and library files will be installed.
This configuration does not allow \*(TS to be configured to use other than
the default directories as specified in the distributed
\*Qconfig\*U file.  To install these 
programs, \fBbecome super-user\fP and type
.DS
etc/3Bbin
.DE
from the top-level \*(TS directory.  
This will install the \*(TS binaries.
.NH 2
Hardware Setup and Configuration
.PP
Different kinds of \*(PS printers will have different
communications interfaces and configuration needs.  For the
most detailed description pertaining to your printer, please
check the documentation available from the manufacturer.  The
notes below were written with the Apple LaserWriter in mind, but
apply to all existent \*(PS printers.
.PP
\*(TS wants to communicate with \*(PS printers over
serial communications lines.  Pick a tty port on your
host to connect to your printer and
.B
disable the getty on that line.
.R
On 4.2BSD systems, this entails editing the file
/etc/ttys and doing a \*Qkill \-HUP 1\*U.
On System V systems, use
.I "sysadm ttymgmt"
or edit /etc/inittab and do a \*Qkill \-1 1\*U.
.PP
One of the most frequent problems in setting up a \*(PS
printer is getting the cabling right.  The 25-pin 
connector on the printer has a \*QDTE\*U type RS232 interface.
This means it can be connected directly to most hosts or
modems with no signal reversals required.  Connecting to a 
terminal requires interposing a \*Qnull modem,\*U which at a
minimum involves reversing the Transmit Data and Receive Data
signals.  
.B N.B. 
Some host computer RS232 interfaces are wired
such that you will need to use a null modem also.
This is true of Sun Workstations, for example.
The Apple LaserWriter printer has a four position rotary
switch.  (The positions are labeled
\fIAppleTalk, Special, 9600,\fP
and
.IR 1200. )
This switch should be turned to the
\*Q9600\*U position
.I
while the printer is powered off.
.R
Once in this position, the power-on page (printed automatically by the 
LaserWriter printer shortly after it has been powered on)
will show one tick-mark on the X axis
of the line graph (indicating the switch position),
and the middle bar-chart will
have a bar labeled \*Q25\*U extending up to a height
labeled \*Q9600\*U (indicating the communications parameters
as set in the printer's persistent memory).
.PP
Other \*(PS printers have other power-on behavior, and
all but the typesetters print a power-on page which
displays their communications settings.  Most printers support
higher baud rates (e.g., 19200).  You may wish to 
use that setting on those printers.  Many printers
also can support hardware DTR flow-control; you may want to 
use it if your UNIX system can support it.
See the documentation for the specific printer to determine
its capabilities.
In order to change \fIany\fP communications settings, you
will need to change the spooling database for the printer
(BSD: /etc/printcap, SysV: stty flags in mkprinter script),
and also download an appropriate \*(PS job to the printer to change the
printer settings (see the etc/setscc.ps file as an example).
.NH 2
UNIX Spooler Configuration
.PP
Once the printer is cabled up, you must
create the necessary UNIX system references to it.  
Most of this process can be accomplished automatically by the
\*Q\fImkprinter\fP\*U shell script in the etc/ 
directory of the \*(TS distribution.
Decide on a name for this printer.
While your first \*(PS printer might be called
\*Qps\*U, or \*Qqms\*U, or \*Qlw\*U, or \*Qdp\*U,
plan for the day when you will have more than one \*(PS printer
available.
At Adobe, we name our printers after typefaces.
On 4.2BSD systems, 
.I
at least one
.R
\*(PS printer at your site should have the alias \*QPostScript\*U,
as some of the \*(TS software will spool to this printer
as the last-ditch default.  On System V systems, at least one
\*(PS printer at your site should belong to a printer 
.I class
named \*QPostScript\*U for the same reason.
For the sake of these examples, our printer will be called
.IR aleph .
Make sure you have edited the top-level files named
\*Qconfig\*U and \*Qprinter\*U as the values in these
files are important for printer installation.
Make sure you determine the SPOOLUSER and SPOOLGROUP
values correctly for your site.
At Adobe and many other 4.2BSD UNIX sites, these are both
\fBdaemon\fP, but the 4.2BSD spooler documentation talks about
user \fBdaemon\fP and group \fBspooling\fP.  Check the owner
and group membership of the program /usr/lib/lpd and
already existing printer spooling directories on your system
to help find out what these should be.
For System V systems, these are typically \fBlp\fP and \fBbin\fP
respectively, check the owner and group of
the directory /usr/spool/lp and /usr/bin/lp.
.PP
When you have plugged the printer into a tty port (say
.IR tty11 ), turn the printer on,
.B cd 
to the \*(TS \*Qetc/\*U directory,
.B 
become super-user
.R
and type
.IP
mkprinter \fIaleph tty11\fP
.LP
The first argument is the name you want the printer to be known by
in all spooling requests, the second is the name of the 
device (under /dev) for the tty port you plugged the RS232 line into.
This will set up a hard link of /dev/tty11 to /dev/aleph,
so that the printer can be referenced symbolically.
Should you chose to move the printer to another tty port, you
need only change the link, and not the various files
that reference the printer.  The \fImkprinter\fP script will set the access
permissions to be those necessary for the spooler to
access the port.  It will also search /etc/ttys or /etc/inittab
for an entry for this device, to remind you about removing
any getty/login from that line.
.LP
.B Note:
There may be some programs used by \fImkprinter\fP that are not on the
search path.
You may need to find these programs, change your search path
to include the appropriate directories, and run \fImkprinter\fP again.
.PP
The \fImkprinter\fP script also sets up the spooling directories,
printer log files, and (on 4.2BSD) printer accounting files.
\fImkprinter\fP is a rather brave program and attempts to do
a lot of operations at once.  At the end of the script, it does
an 
.B ls
of the files and directories it has created.
.PP
On 4.2BSD systems, \fImkprinter\fP creates a printcap
entry called \*Q\fIprintcap.new\fP\*U for use with this printer.
The 
.I /etc/printcap
file (described in printcap(5)) has
functional descriptions of all printers known to the 4.2BSD
print spooler, including printers accessible over a network.
The file
.I etc/printcap.proto
in this distribution contains a sample printcap entry for 
a \*(PS printer run by \*(TS software; 
.I etc/printcap.notes
contains some comments about this entry, as well as
a sample printcap entry for a remote networked printer. 
Examine the file
.I printcap.new
which was created by \fImkprinter\fP
and install it in /etc/printcap (usually just append it),
when you are satisfied with the entries.
.PP
On System V systems, \fImkprinter\fP will create and install
a printer interface program for your printer.  It runs the
lpshut, lpadmin, and lpsched programs to install the spooler 
interface for your printer.
.PP
You will want to do a \fImkprinter\fP
for each \*(PS printer you install on your system.
.NH
Interactive Use of a \*(PS printer.
.PP
\*(PS printers have a built-in interactive \*Qexecutive\*U mode.
This mode is often useful for setting persistent printer parameters.
On 4.2BSD systems, you can use the
.I tip
program to talk to a printer.
You can add the following entry to your 
.I tip
host tables (usually in
.IR /etc/remote ).
.DS
aleph:el=^C^D^S^Q^U:br#9600:dv=/dev/aleph:pn=:
.DE
(See the file etc/remote.sample in this distribution 
and the manual pages for remote(5), and tip(1) for more information.)
You should then be able to say
.DS
tip aleph
.DE
to the shell.  
On System V, you can use
.I cu
to access the tty port.
Initially, the printer will not echo things you
type.  In order to get the printer to echo (and perform simple
line-editing functions), you should carefully type
.DS
executive
.DE
followed by a carriage return.  You should get a short message
and a \*QPS>\*U prompt; this is the interactive \*(PS
interpreter.  You can then type \*Qshowpage\*U followed by a 
carriage return to get the printer
to kick out a (blank) sheet of paper, and so on.
To get out of the \*(PS executive, type 
.I quit
or a control-D; then type \*Q~.\*U to get out of \fItip\fP or \fIcu\fP.
.NH
Troubleshooting
.PP
Occasionally, the spooler or printer may get \*Qhung\*U, that is,
stopped in such a way that it will not recover by itself.
For the sake of example, say it's
.I aleph
again.
The first thing to do is check 
the queue and the last few lines of the printer log file.
If you really think it's hung (e.g., the spooler says the printer is
busy, but no lights are flashing) try power-cycling the printer.
This is often the only thing necessary to kill off the hung processes 
and start up printing again.
Please wait at least 30 seconds for the software to notice these events
before trying something more drastic.
.PP
In some circumstances, it may be
necessary to disable printing and kill off processes.  To do
this, follow these steps:
.NH 2
4.2BSD Systems
.IP 1.
Make sure you are logged on to the system that is directly connected
to the printer, not a remote host.
.IP 2.
Turn off printing by doing 
.DS
lpc stop \fIaleph\fP
.DE
.IP 3.
Determine which processes are still active for this printer
and kill them.  Look at the process id (pid) in the \*Qlock\*U file 
(in the printer's spooling directory) and do
.DS
ps aux | fgrep daemon
.DE
(or SPOOLUSER if it isn't daemon) to help here.  
Kill them with the \-9 signal, which they can't ignore.
.IP 4.
Check the status of the printer now:
.DS
lpc status \fIaleph\fP
.DE 
should report \*Qno daemon present\*U.
.IP 5.
Now you can restart printing with
.DS
lpc start \fIaleph\fP
.DE
.IP 6.
If this doesn't do the trick, the printer itself may be hung.
Repeat the above steps except, after
you kill off the processes and before you do the \*Qstart\*U,
power-cycle the printer, waiting until it 
is fully operational (the green light stops blinking and the printer
prints the power-on page).
.LP
Occasionally on 4.2BSD, doing an 
.I lprm
or
.I
lpc abort
.R
on the currently active (i.e., printing) job will result in the 
printer daemon going away and not restarting for the next job in the queue.
This appears to be a problem in lpd (not in the \*(TS filters).  The \*Qfix\*U
for this is simple, but may take some user education.  The command
.DS
lpc restart \fIaleph\fP
.DE
is available to all users (i.e., it is not a privileged command),
and should be used under these circumstances.
The lpc program is often not on the user's search path, so an absolute
path to it must be typed.  For example:
.DS
/etc/lpc restart \fIaleph\fP
.DE
.LP
A related bug exists in the 4.2BSD spooler (as we received it)
which, on removing the active job, results in the error message:
.DS
/usr/lib/lpd: \fIhostname\fP: unknown printer
lprm: \fIaleph\fP: cannot restart printer daemon
.DE
If you have 4.2 UNIX sources, this can be fixed with an edit to
rmjob.c (in /usr/src/usr.lib/lpr): change the string \*Qstartdaemon(host)\*U
to \*Qstartdaemon(printer)\*U.
.LP
In any case, an \*Qlpc restart \fIaleph\fP\*U will restart the printer.
.NH 2
System V
.IP 1.
Turn off printing by doing
.DS
disable \-c \fIaleph\fP
.DE
.IP 2.
Determine which processes are still active for this printer
and kill them.  Do
.DS
ps \-ef | fgrep lp
.DE
(or SPOOLUSER if it isn't lp) to help here.  
Kill them with the \-9 signal, which they can't ignore.
.IP 3.
Check the printer status:
.DS
lpstat \-t
ps \-ef | fgrep lp
.DE
.IP 4.
Restart printing with
.DS
enable \fIaleph\fP
.DE
.NH
Test Files
.PP
The 
.I test
subdirectory includes some sample \*(PS print files, as well as
some files in other formats to exercise various \*(TS filters.
Once you have fully installed the printer and software, 
you can try out some of these files.  See the README
file in that directory for more information.
.PP
The files in the 
.I cookbook
directory are more examples of interesting print files from 
the \*(PS cookbook.  See the README file there for more information.
.NH 
Printer Pragmatics
.PP
Each type of \*(PS printer has its own special user interface
for choosing communications parameters, paper trays and paper handling,
printer error reporting, etc.  \*QSupplements\*U to 
the \fI\*(PS Language Reference Manual\fP 
which detail the specifics of \*(PS printers
are be available from your printer dealer or the manufacturer or from 
Adobe Systems.  
.PP
Most \*(PS printers have some way of letting the outside world
know what they are doing.  They have flashing lights, or
LED displays to let a viewer know that the printer is busy
processing a job or idle or waiting for more input.
The Apple LaserWriter has three small lights on its front panel just below
the Apple logo.  These lights are used by the printing engine and the
controlling \*(PS software to help indicate what the printer is doing
in various ways.  The lights are green (with a semi-circular
pictogram), yellow (with a paper-out picture), and red (with a zig-zag
jam picture).  The following table describes the meanings of various state
of these lights:
.sp
.SM
.ta 2.2i
No Lights	printer off or no toner cartridge inside
.br
Solid Green	idle and awaiting a job
.br
Solid Yellow	out of paper, no paper tray, awaiting manual feed
.br
Solid Red	paper misfeed or jam needs to be cleared
.br
Flashing Green	warming up or reinitializing printer hardware interface
.br
Green & 1 yellow flash	busy computing \- executing \*(PS
.br
Green & 2 yellow flashes	execution in progress, waiting for more input
.NL
.NH
APPENDIX: Font Installation
.PP
This section is intended to help the \*(TS installer to estimate how
much disk space and how many hard links a font installation will consume.
There are 13 font families in this release, and 4 font faces within
each family.
.NH 2
\fItroff\fP
.PP
Each \fItroff\fP font family takes approximately 20 KB of disk space,
and 9 hard links. For all font families, this is 13\(**20=260 KB.
Note that if you don't want to install any of these fonts, set
TROFFFONTS=0 in the \fIconfig\fP file before the installation.
.NH 2
\fIditroff\fP (or \fItroff\fP in Documenter's Workbench)
.PP
This is a little more complex, since the information for each font
can appear both in the main (Times) font, and in all
the other font family directories.
"Font families" are different from "font family directories".
Installing a "font family" puts that font in the main \fIditroff\fP
font data base, with its unique two-letter \fIditroff\fP font name.
Installing the directory for that font family creates a directory
that can be used with the psroff "\-F" option to set the default
font family for a document.
In the \fIconfig\fP file, the FONTFAMILIES variable controls
which fonts are installed, while the DITROFFDIRS variable controls
whether font family directories will be created.
.PP
Each \fIditroff\fP font family uses approximately 70 KB of disk space:
35 KB for the font information, and 35 for the font family directory.
The usage formula is:
.DS
<Font families>\(**35 + <Font family directories>\(**35
.DE
For all font families, this is 13\(**35+13\(**35=910 KB.
Note that if you don't want to install any of these fonts, set
DITROFFFONTS=0 in the \fIconfig\fP file before the installation.
.PP
16 hard links (inodes) are used for each font family,
and fonts are installed in \fIall\fP font family directories using
hard links.
The usage formula is:
.DS
<Font family directories> \(** <Font families> \(** 16
.DE
For all font families, this is 13\(**13\(**16=2704 inodes used.
