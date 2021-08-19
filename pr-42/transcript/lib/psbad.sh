#! /bin/sh
# dummy entry for unavailable filters
# GOVERNMENT END USERS: See Notice file in TranScript library directory
# -- probably /usr/lib/ps/Notice
# RCSID: $Header: psbad.sh,v 2.2 87/11/17 16:40:23 byron Rel $

# argv is: psbad filtername user host
prog=`basename $0`
filter=$1
printer=$2
user=$3
host=$4

cat <<bOGUSfILTER
%!
72 732 moveto /Courier-Bold findfont 10 scalefont setfont
(WARNING:  Unable to convert document to PostScript) show
72 708 moveto
(Printer:  ${printer}) show
72 696 moveto
(User:  $user) show
72 684 moveto
(Host:  $host) show 
72 672 moveto
(Filter:  \"$filter\" not available) show
72 648 moveto
(This filter needs to be installed and the appropriate shell scripts need to be)
show
72 636 moveto
(edited.  For the NeXT 400 dpi Laser Printer edit /usr/lib/NextPrinter/comm.  For) show
72 624 moveto
(other PostScript printers edit /usr/lib/transcript/psint.sh.  Contact your) show
72 612 moveto
(system administrator for assistance.) show
showpage

bOGUSfILTER
exit 0
