#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1987 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Header: map.c,v 2.2 87/11/17 16:49:57 byron Rel $";
#endif
/* map.c
 *
 * Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * front end to mapname -- font mapping for users
 *
 * for non-4.2bsd systems (e.g., System V) which do not
 * allow long Unix file names
 *
 * RCSLOG:
 * $Log:	map.c,v $
 * Revision 2.2  87/11/17  16:49:57  byron
 * Release 2.1
 * 
 * Revision 2.1.1.2  87/11/12  13:40:11  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.1  87/04/23  10:25:24  byron
 * Copyright notice.
 * 
 * Revision 2.2  86/11/02  14:15:21  shore
 * Product Update
 * 
 * Revision 2.1  85/11/24  11:49:13  shore
 * Product Release 2.0
 * 
 * Revision 1.1  85/11/20  00:14:39  shore
 * Initial revision
 * 
 *
 */

#include <stdio.h>
#include "transcript.h"

main(argc,argv)
int argc;
char **argv;
{
    char result[128];

    if (argc != 2) exit(1);
    if (mapname(argv[1],result) == NULL) exit(1);

    printf("%s\n",result);
    exit(0);
}
