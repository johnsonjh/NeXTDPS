#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1987 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Header: mapname.c,v 2.2 87/11/17 16:50:02 byron Rel $";
#endif
/* mapname.c
 *
 * Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * Maps long PostScript font names to short file names via
 * mapping table
 *
 * for non-4.2bsd systems (e.g., System V) which do not
 * allow long Unix file names
 *
 * RCSLOG:
 * $Log:	mapname.c,v $
 * Revision 2.2  87/11/17  16:50:02  byron
 * Release 2.1
 * 
 * Revision 2.1.1.2  87/11/12  13:40:14  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.1  87/04/23  10:25:29  byron
 * Copyright notice.
 * 
 * Revision 2.2  86/11/02  14:15:25  shore
 * Product Update
 * 
 * Revision 2.1  85/11/24  11:49:15  shore
 * Product Release 2.0
 * 
 * Revision 1.1  85/11/20  00:15:39  shore
 * Initial revision
 * 
 *
 */

#include <stdio.h>
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#include "transcript.h"

/* psname (long name of a PostScript font) to a filename */
/* returns filename is successful, NULL otherwise */

char MapFile[512];

char *mapname(psname,filename)
char *psname, *filename;
{
    FILE *mapfile;
    char longname[128], shortname[128];
    char *libdir;
    int retcode;

    *filename = '\0';
    if ((libdir = envget("PSLIBDIR")) == NULL) libdir = LibDir;
    VOIDC mstrcat(MapFile,libdir,FONTMAP,sizeof MapFile);
    if ((mapfile = fopen(MapFile, "r")) == NULL) {
	fprintf(stderr,"can't open file %s\n",MapFile);
	exit(2);
    }

    while (fscanf(mapfile, " %s %s\n", longname, shortname) != EOF) {
	if ((retcode = strcmp(longname, psname)) > 0) break;
	else if (retcode == 0) {
	    strcpy(filename, shortname);
	    return (filename);
	}
    }
    return ((char *)NULL);
}

