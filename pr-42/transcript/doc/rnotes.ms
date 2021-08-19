.na
.DA "25 September 1987"
\" Copyright (c) 1985,1987 Adobe Systems Incorporated. All Rights Reserved.
\" GOVERNMENT END USERS: See notice of rights in Notice file in TranScript
\" library directory -- probably /usr/lib/ps/Notice
\" RCSID: $Header: rnotes.ms,v 2.2 87/11/17 16:09:40 byron Rel $
.ds PS P\s-2OST\s+2S\s-2CRIPT\s+2
.ds TS T\s-2RAN\s+2S\s-2CRIPT\s+2
.ds TR \fItroff\fP
.ds DI \fIditroff\fP
.de IR
\fI\\$1\fP\\$2
..
.de BB
.RS
.nr PD 0
..
.de BE
.RE
.nr PD .3v
..
.TL
\*(TS 2.1
.br
Release Notes
.FS
\*(PS is a registered trademark of Adobe Systems Incorporated.
.br
\*(TS is a trademark of Adobe Systems Incorporated.
.br
UNIX is a trademark of AT&T Bell Laboratories.
.FE
.AU
Adobe Systems Incorporated
.AI
1585 Charleston Road
P.O. Box 7900
Mountain View, CA \ \ 94309-7900
.NH
Introduction
.LP
These \fIRelease Notes\fP are included in the product for the following
purposes:
.IP \(bu
to outline new features and bug fixes in the release
.IP \(bu
to identify any known problems with the release
.IP \(bu
to mention some of the areas for future work
.LP
Detailed descriptions of the various \*(TS filters are documented 
in their UNIX \fIman\fP pages.
.LP
The \*(TS Overview details the product
structure and the function of all \*(TS programs.
The \*(TS Installation Instructions gives more information to
systems administrators about the \*(TS software, file locations, etc.
.NH
Release 2.1 \- Changes since release 2.0
.NH 2
New features
.IP "Font families" .9i
The various \*(TR tools (\fIpsroff, ptroff, psdit, and pscat\fP)
accept the specification of a \fIfont family\fP on the command line.
This family is mounted for the R/I/B/BI fonts, thus making it
the "default" font family for the document.
The following font families are provided with this release:
.ta 1.5i 3i
.br
AvantGarde	Bookman	Courier
.br
Garamond	HelvNarrow (Helvetica-Narrow)
.br
Helvetica	Lubalin	NewCentury
.br
Palatino	Souvenir	Zapf
.br
(The Zapf family consists of ZapfChancery-MediumItalic as the
\fIroman\fP font, Times-Roman as the \fIitalic\fP font, and Times-Bold
as the bold font.
.IP Fonts
Font information has been provided for each of the fonts in the
font families listed above.
this list includes most of the resident fonts in \*(PS printers.
\*(TR names for the fonts have been defined.
See the psfonts(1) man page.
.IP Enscript
Enscript now does line-wrapping for long lines, rather than truncation.
Truncation can be specified as an option.
.IP "\fIman\fP pages"
Three new \fIman\fP pages have been added:
.br
transcript(1), which gives a general overview of the \*(TS
product,
.br
psfonts(1), which describes the font families and font names,
and
.br
pssymbols(1), which lists the symbol character codes (such as \\(oe)
recognized by \fIpsroff\fP and \fIpsdit\fP.
This last man page can be printed on a \*(PS printer using psroff to
show the symbol characters themselves.
.br
Also, the old transcript(8) man page, which detailed the spooling software,
has been renamed pscomm(8).
A list of the important variables in the \fIpsint/psinterface\fP
script, and their functions, has been added to pscomm(8).
The cross-references among various man pages has been improved.
.IP Installation
There are a number of new options in the \*(TS installation.
See the Installation Instructions, and the "config" file.
.IP "BANNERLAST on BSD UNIX"
\*(TS now allows two different options for handling
the BANNERLAST setting under the BSD UNIX operating system.
See the Overview for more details.
.NH 2
Bug fixes to release 2.0
.IP enscript .9i
Fixes to formfeed processing.
Fixed backspace to move back a column, so lines don't get truncated early.
Fixed margin setting in two-column mode; small point sizes work better.
Fixed handling of input files that are not found.
Fixed parsing of ENSCRIPT variable, and added parsing of new AFM file
format.
.IP ps630
Fixed output creeping up on succeeding pages.
Fixed justification to work for: leading blanks and tabs,
compressed and expanded print,
and lines that are longer than the justification margins.
Fixed problem reading from standard input on some machines.
Fixed some problems that caused the first or final line on a page
not be be printed.
Increased maximum output line length to 1024.
Fixed underlining to be located in same place as underscore character.
ESCAPE-S now resets HMI.
.IP ps4014
Pages with nothing drawn on them are not usually printed now.
Fixed bug that caused the line style to change back to "solid".
Fixed problem that caused lines containing many vectors (more than 500)
to create a \*(PS error.
Fixed problem that caused the final vector not to be printed.
Error messages now report where in the input file the error was
encountered.
Fixed problem that moved some vectors to left-hand side of page.
ESCAPE-FORMFEED has changed meaning: it now prints the current page and
starts a new one.
ESCAPE-ETB is now ignored.
.IP psplot
Pages with nothing drawn on them are not usually printed now.
Added "disconnected" line type, which is the same as the "solid" line type.
.IP psroff
Added "-F" and "-D" switches for font family support.
PATH variable is set explicitly.
Fixed problem in which spooler options were lost unless job name was
specified.
(See also psdit).
.IP psdit
The location and shape of some special characters have been moved;
see "\*(TR special characters" below.
Character overprinting is partially fixed; \\z works better.
Fixed the psdit \fIman\fP page to give a correct example for
incorporating \*(PS into a document; the old example was incorrect.
An error message is now printed if font information cannot be found.
Added support for \\X command, which is a method for incorporating
user \*(PS into a document.
Fixed problem that caused an infinite loop for certain cases
of early termination.
Some of the drawing primitives (draw arc, draw line, etc.) have been
changed to be smaller and more efficient, and some drawing
location problems have been fixed.
Fixed problem that caused coordinates to be written to the screen when
certain drawing primitives were invoked.
Fixed problem in which characters were being overprinted before a font
change in some cases.
.IP afmdit
Added check to give characters that are too wide the widest width
possible for the resolution \*(TS uses.
.IP ptroff
Added "-F" switch for font family support.
PATH variable is set explicitly.
Fixed "while" loop test to work for any option string.
(See also pscat).
.IP pscat
The psdit and pscat special character definitions are now identical.
The locations of some special characters have been moved;
see "\*(TR special characters" below.
The initial vertical offset for BSD UNIX systems has been changed to be higher
than the top of the page; this reflects the BSD \*(TR assumptions.
Fixed problems that occasionally caused the first character on a page, and
some special characters, to be printed in incorrect locations.
.IP pscatmap
Added @INCLUDE command, and created a character correspondence table
file that is included in most pscatmap input files.
Added DEFSYM font, which corresponds to new special character font.
Added parsing of new AFM file format.
Fixed uninitialized variable; on some systems this would result in
pscatmap crashing with parsing errors.
.IP "\*(TR special characters"
This section documents changes to many of the "symbol" characters that
are used in \*(TR/\*(DI.
The following location and width changes were made:
.BB
.IP \(bu .20i
Changed box rule and underline so that tbl boxes would meet at the corners.
.IP \(bu
Changed the "root extender" so that the square root symbol looks somewhat
better.
.IP \(bu
Decreased the width of bracket characters, which makes eqn brackets look
better.
.IP \(bu
The "box rule" character width is now 0.
.IP \(bu
Vertical rule-type characters have been moved to look better at
high resolution.
.IP \(bu
The circle character has been moved right slightly.
.BE
The "vertical bar" character is always taken from the symbol font;
there is now no method for getting an italic vertical bar.
Changed charset ordering of characters in \*(DI to make it
less likely the .tc and .lc commands will fail.
Fixed Courier fonts not to use ligatures.
.IP "Spooling programs: pscomm, psint, pstext, psrv"
pscomm has been re-arranged to be more reliable:
.BB
.IP \(bu .20i
On startup, \*(TS "synchronizes" with the printer.
This should eliminate problems that caused errors in the log file, jobs
vanishing from the queue, etc.
.IP \(bu
A number of race conditions have been eliminated.
These gave rise to the printer queue hanging, "longjmp botches", and other
random behavior.
.IP \(bu
Child processes are explicitly killed, and pscomm will not exit until its
children have finished.
.IP \(bu
pscomm waits for the current job to end before starting the next one.
This should eliminate some accounting errors, banner page problems, and
jobs hanging.
.IP \(bu
Accounting is more likely to work correctly.
.IP \(bu
Error reporting and error handling have been improved somewhat.
.BE
The following problems were also fixed in the spooling software:
.BB
.IP \(bu .20i
Fixed problem that caused very small text files (less than 12 characters)
to vanish.
.IP \(bu
Fixed pscomm problem that handled standard input incorrectly when it was
a pipe.
.IP \(bu
Banner line buffer overflow is now caught as an error;
this should fix some problems that caused files with random names to be
created, random crashes and hangings of the spooling system, etc.
.IP \(bu
Fixed problem with blank pages in text files that sometimes resulted in
a \*(PS error.
.IP \(bu
Changed page reversal algorithm to work correctly in more cases, and
to print out an error when page reversal can't be done correctly and the
\*(PS file says that page reversal can be done.
.IP \(bu
A bug was fixed which caused the string "%%" to echo as "%" in the
log file; this only concerns \*(PS files which \fIgenerate\fP
output themselves.
.BE
.IP "All filters"
All \*(TS filters now explicitly exit with a value of 0 when processing
has been completed successfully (that is, without significant errors).
.NH
Known Deficiencies in Release 2.1
.NH 2
The following bugs are known to exist in Release 2.1:
.IP \(bu
Overprinting in psroff/psdit does not always work correctly, due to
the method by which round-off errors are handled in psdit.
.IP \(bu
The spooling software does not truncate long lines in spooled text files.
This can result in partial characters at the ends of lines.
.IP \(bu
Mounting a font in psdit may get an overflow error message.
This is because the space for the font is allocated when psdit starts,
rather than dynamically.
A work-around is to mount "large" fonts for psdit on startup.
The Courier fonts are smaller than most others.
.IP \(bu
ps630 output cannot be page-reversed.
Also, long documents may run output of \*(PS "VM", and
cannot be printed at all.
Long documents will print correctly when they are split up into
smaller pieces.
.IP \(bu
enscript output in two-column mode will occasionally run over the
margins for small points sizes.
.IP \(bu
\*(TS assumes that a number of the UNIX system calls will never return
an error; they are not checked for error returns.
This can lead to obscure errors when system software is not
configured correctly.
The case that was reported involved getpwuid() failing.
.NH 2
The following problems have been encountered in system software:
.IP \(bu
The BSD UNIX spooler daemon will often not be restarted after
an \fIactive\fP job has been removed from the queue.
An unprivileged user can use the restart command in the \fIlpc\fP
program to restart the demon.
This is a problem in the implementation of the lprm command.
Note that the \fIlpc\fP command may not be on a user's path, and
that the command must be given on the \fIhost\fP machine that is
communicating directly to the printer.
.IP \(bu
Under BSD UNIX, the 
BANNERLAST feature behaves incorrectly when multiple files
or multiple copies are spooled with one \fIlpr\fP request.
This is due to the order in which \fIpsof\fP and \fIpsif\fP
(the ``of'' and ``if'' printcap filters) are invoked by the
spooling daemon.
As of this release, BANNERLAST may be set either of two ways;
see the Overview for more information.
This problem does not otherwise effect the operation of \*(TS
or the correct printing of documents.
.IP \(bu
Under BSD UNIX, a system doing a reboot with items currently in
a \*(TS spooler queue will print out the first job in the queue,
and then that queue will hang.
This is a problem in the startup code for the spooler daemon.
There may be a workaround involving stopping all \*(TS printers
before starting the spooler daemon during a boot, starting the daemon,
sleeping for a while, and starting all the printers.  This workaround
has not been tested.
.NH
Further work is needed in the following areas:
.LP
Please note that this list does not constitute a commitment about
the content of the next \*(TS release, and that there will probably
be other areas that will be worked on in addition to those listed.
.IP \(bu
Paper handling.
There are a number of issues here.
The default paper size should be configurable, so that European sites
can get A4 size paper by default.
Multiple paper sizes and multiple paper trays should be supported.
Paper orientation (landscape vs. portrait) should be supported.
Offsets of the image should be supported for both dimensions.
.IP \(bu
Version 2 of the \*(PS \fIComment Conventions\fP should be supported.
.IP \(bu
Currently, installing information about new fonts is relatively
difficult.
There should be a tool for installing font information for \*(TR and
\*(DI.
.IP \(bu
Support for the UCB Gremlin \*(DI graphics package.
.IP \(bu
The installation procedure should be easier to use.
Also, it should be more modular, so that if part of the installation
goes awry, only that part need be repeated.
