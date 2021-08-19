#!/bin/csh -fb 
#
# This shell script prepares two directories in /tmp that become the 
# roots for source releases of the NeXTstep window server to IBM.
# The first directory created is /tmp/IBMWindowServerSource, which contains
# all the NeXT-developer or -modified files that can be given directly to
# IBM.  The other directory is /tmp/AdobeWindowServerSource, which
# contains the Adobe-proprietary sources which must be returned to Adobe
# and recompiled for IBM.
#
# This script must be run in the clone of the ps_proj which it is desired
# to release.
#
# This script written by Leovitch, 25Aug90.
#
# Modified:
# 09Sep90 Leo    Added include file treatment
#
# Make sure umask doesn't get us
umask 0
#
# Define destinations dirs
set ibmDest = /tmp/IBMWindowServerSource
set adobeDest = /tmp/AdobeWindowServerSource
#
# Definitions (use either first or second set)
# set cp = "echo /bin/cp"
# set rm = "echo /bin/rm"
# set mkdirs = "echo mkdirs"
# set touch = "echo touch"
# set ibmReadme = /dev/tty
# set adobeReadme = /dev/tty
set cp = /bin/cp
set rm = /bin/rm
set mkdirs = mkdirs
set touch = touch
set ibmReadme = $ibmDest/README
set adobeReadme = $adobeDest/README
#
# First, define the subdirectories that only go to one company.
set ibmOnlyDirs = (mousekeyboard bintree mp bitmap coroutine device stodev stream fp pslib product)
set adobeOnlyDirs = (postscript fonts unix_product graphics devpattern language vm)
#
# Now, define the total list of directories each side gets 
set ibmDirs = ($ibmOnlyDirs config)
set adobeDirs = ($adobeOnlyDirs include config)
#
# Define the include files that go to IBM 
# (Adobe gets all include files to they can build their half of the release)
set ibmHeaders = (bintree.h bitmap.h copyright.h coroutine.h customops.h devcreate.h device.h devicetypes.h devimage.h devmark.h devpattern.h environment.h event.h except.h foreground.h fp.h fpfriends.h imagemessage.h io.h mousekeyboard.h mp.h package_specs.h postscript.h printmessage.h pslib.h publictypes.h sizes.h stodev.h stream.h unixstream.h windowdevice.h)
#
# OK, now begin the IBM portion of the release
if (-e $ibmDest) then
echo Removing existing $ibmDest ...
$rm -rf $ibmDest
endif
$mkdirs $ibmDest
foreach i ($ibmDirs)
	echo Copying directory $i to $ibmDest ...
	$cp -pr $i $ibmDest
end
$mkdirs $ibmDest/include
foreach i ($ibmHeaders)
	$cp -p include/$i $ibmDest/include
end
foreach i (*)
	if (! -d $i) then
		$cp -p $i $ibmDest
	endif
end
$touch $ibmDest/README
echo This directory contains a source release of >$ibmReadme
echo the NextStep Window Server.  Note that you must have the >>$ibmReadme
echo gnumake command installed in order to successfully make in >>$ibmReadme
echo this directory.  You also will need to get a number of >>$ibmReadme
echo libraries that should be stored in the configuration  >>$ibmReadme
echo subdirectories from Adobe. >>$ibmReadme
#
# OK, and now do the Adobe stuff
if (-e $adobeDest) then
echo Removing existing $adobeDest ...
$rm -rf $adobeDest
endif
$mkdirs $adobeDest
foreach i ($adobeDirs)
	echo Copying directory $i to $adobeDest ...
	$cp -pr $i $adobeDest
end
foreach i (*)
	if (! -d $i) then
		$cp -p $i $adobeDest
	endif
end
$touch $adobeDest/README
echo This directory contains a partial source release of the >>$adobeReadme
echo NextStep window server.  Note that you must have the  >>$adobeReadme
echo gnumake command installed locally in order to make this >>$adobeReadme
echo release.  Furthermore, you will probably need to modify the >>$adobeReadme
echo  configuration files in the config subdirectory to suit >>$adobeReadme
echo your particular environment.  You will then need to build >>$adobeReadme
echo archives for all the packages represented here and  >>$adobeReadme
echo forward them to IBM. >>$adobeReadme

