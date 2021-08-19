#!/bin/sh
# 4.2BSD line printer spooler interface for PostScript/TranScript printer
# this is the printcap/lpd-invoked top level filter program for ALL file types
# Copyright (c) 1985,1987 Adobe Systems Incorporated. All Rights Reserved. 
# GOVERNMENT END USERS: See Notice file in TranScript library directory
# -- probably /usr/lib/ps/Notice
# RCSID: $Header: psint.proto,v 2.2 87/11/17 16:40:51 byron Rel $

PATH=/bin:/usr/bin:/usr/ucb
export PATH

# set up initial undefined variable values
width= length= indent= login= host= afile=
prog=$0
cwd=`pwd`
pname=`basename $cwd`

# define the default printer-specific and TranScript
# configuration options, these may be overridden by
# real printer-specific options in the .options file

PSLIBDIR=//usr/lib/transcript
REVERSE=//usr/lib/transcript/psrv
VERBOSELOG=1
BANNERFIRST=0
BANNERLAST=0
BANNERPRO=$PSLIBDIR/banner.pro
PSTEXT=$PSLIBDIR/pstext
PSCOMM=$PSLIBDIR/pscomm
export PSLIBDIR VERBOSELOG BANNERFIRST BANNERLAST BANNERPRO REVERSE PSTEXT

# load the values from the .options file if present
test -r ./.options && . ./.options

# parse the command line arguments, most get ignored
# the -hhost vs. -h host is due to one report of someone doing it wrong.
# you could add or filter out non-standard options here (-p and -f)

while test $# != 0
do	case "$1" in
	-c)	;;
	-w*)	width=$1 ;;
	-l*)	length=$1 ;;
	-i*)	indent=$1 ;;
	-x*)	width=$1 ;;
	-y*)	length=$1 ;;
	-n)	user=$2 ; shift ;;
	-n*)	user=`expr $1 : '-.\(.*\)'` ;;
	-h)	host=$2 ; shift ;;
	-h*)	host=`expr $1 : '-.\(.*\)'` ;;
	-*)	;; 
	*)	afile=$1 ;;
	esac
	shift
done

PATH=$PSLIBDIR:$PATH
export PATH

# now exec the format-specific filter, based on $prog
# if - communications filter [default]
# of - banner filter [execed directly]
# nf - ditroff, tf - troff (CAT), gf - plot
# vf - raster, df - TeX DVI, cf - cifplot, rf - fortran

prog=`basename $prog`
comm="$PSCOMM -P $pname -p $prog -n $user -h $host $afile"

case $prog in
	psif) exec $comm ;;
	psof) exec psbanner $pname ; exit 0 ;;
	psnf) psdit | $comm ;;
	pstf) pscat | $comm ;;
	psgf) psplot | $comm ;;
	psvf|pscf|psdf|psrf) echo "$prog: filter not available." 1>&2  ;
			psbad $prog $pname $user $host | $comm ;;
esac
