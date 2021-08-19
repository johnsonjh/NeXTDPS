#! /bin/sh
# sh/psroff.bsd
# Copyright 1985,1987 Adobe Systems Incorporated. All rights reserved.
# GOVERNMENT END USERS: See notice of rights in Notice file in TranScript
# library directory -- probably /usr/lib/ps/Notice
# RCSID: $Header: psroff.bsd,v 2.2 87/11/17 16:48:38 byron Rel $
#
# run ditroff in an environment to print on a PostScript printer
#
# psroff - ditroff | psdit [| lpr]
#
PATH=/bin:/usr/bin:$PATH       # Make sure we get system programs.
export PATH

ditroff=ditroff
psdit=psdit
nospool= dopt= fil= spool= dit=
printer=-P${PRINTER-PostScript}
family=Times
fontdir=DITDIR
while test $# != 0
do	case "$1" in
	-t)	nospool=1 ;;
	-Tpsc)	;;
	-T*)	echo only -Tpsc is valid 1>&2 ; exit 2 ;;
	-F)	if test "$#" -lt 2 ; then
			echo '-F takes following font family name' 1>&2
			exit 1 
		fi
		family=$2 ; shift ;;
	-F*)	echo 'use -F familyname' 1>&2 ;
		exit 1 ;;
	-D)	if test "$#" -lt 2 ; then
			echo '-D takes following font directory' 1>&2
			exit 1 
		fi
		fontdir=$2 ; shift ;;
	-D*)	echo 'use -D fontdirectory' 1>&2 ;
		exit 1 ;;
	-#*|-h|-m)	spool="$spool $1" ;;
	-P*)	printer=$1 ;;
	-C)	spool="$spool $1 $2" ; shift ;;
	-J)	spool="$spool $1 $2" ; jobname=$2 ; shift ;;
	-)	fil="$fil $1" ;;
	-*)	dopt="$dopt $1" ;;
	*)	fil="$fil $1" ; jobname=${jobname-$1} ;;
	esac
	shift
done
if test "$jobname" = "" ; then
	spool="-J Ditroff $spool"
fi
spool="lpr $printer $spool"
if test "$fil" = "" ; then
	fil="-"
fi
dit="$ditroff -Tpsc DITFLAGS -F$fontdir/$family $dopt $fil "
psdit="$psdit -F$fontdir/$family"

if test "$nospool" = "1" ; then
	$dit | $psdit
else
	$dit | $psdit | $spool
fi
