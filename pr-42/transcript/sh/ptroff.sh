#! /bin/sh
# sh/ptroff.bsd
# Copyright 1985,1987 Adobe Systems Incorporated. All rights reserved.
# GOVERNMENT END USERS: See notice of rights in Notice file in TranScript
# library directory -- probably /usr/lib/ps/Notice
# RCSID: $Header: ptroff.bsd,v 2.2 87/11/17 16:48:50 byron Rel $
#
# run troff in an environment to print on a PostScript printer
#
# ptroff - troff | pscat [| lpr]
#PATH=/bin:/usr/bin:$PATH       # Make sure we get system programs.
#export PATH

opt= spool= 
psfontlib=TROFFFONTDIR
font=-F${psfontlib}
family=Times
printer=`dread System Printer`
if test "$?" -ne 0 ; then
	printer=-PLocal_Printer
else
	printer=-P`dread System Printer | sed -e 's/.* //'`
fi
while test $# != 0
do	case "$1" in
	-F)	if test "$#" -lt 2 ; then
			echo '-F takes following font family name' 1>&2
			exit 1 
		fi
		family=$2 ; shift ;;
	-F*)	echo 'use -F familyname' 1>&2 ;
		exit 1 ;;
	-t)	nospool=1 ;;
	-#*|-h|-m)	spool="$spool $1" ;;
	-P*)	printer=$1 ;;
	-C)	spool="$spool $1 $2"
		classname=$2 ; shift ;;
	-J)	jobname=$2 ; shift ;;

	-)	fil="$fil $1" ;;
	-*)	opt="$opt $1" ;;

	*)	fil="$fil $1" ; jobname=${jobname-$1} ;;
	esac
	shift
done
spool="$printer $spool"
if test "$jobname" = "" ; then
	jobname="Troff"
fi
spool="-J $jobname $spool"
if test "$fil" = "" ; then
	fil="-"
fi
troff="troff -F${psfontlib}/${family}/ftXX -t $opt ${psfontlib}/${family}/font.head $fil "
pscat="pscat -F${psfontlib}/${family}/font.ct "

if test "$nospool" = "1" ; then
	$troff | $pscat
else
	$troff | $pscat | lpr $spool
fi
