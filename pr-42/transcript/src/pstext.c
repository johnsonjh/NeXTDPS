#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1987 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Header: pstext.c,v 2.2 87/11/17 16:52:41 byron Rel $";
#endif
/* pstext.c
 *
 * Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * ultra-simple text formatter for PostScript
 *
 * pstext gets called when a text file gets spooled to a
 * PostScript printer.  In this case, a text file is a
 * file without the PostScript magic number (%!).
 *
 * In BSD systems, pstext is fork/execed from the spooler
 * communications program, and has an argv like the spooler's.
 * In SYSV systems, pstext is run by the top-level interface
 * shell script, and has more control(?) and different args.
 *
 * If you want nicer listings, use enscript.
 *
 * RCSLOG:
 * $Log:	pstext.c,v $
 * Revision 2.2  87/11/17  16:52:41  byron
 * Release 2.1
 * 
 * Revision 2.1.1.3  87/11/12  13:42:06  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.2  87/04/23  10:26:53  byron
 * Copyright notice.
 * 
 * Revision 2.1.1.1  86/06/02  10:30:19  shore
 * fixed formfeed processing -- produced incorrect PS files
 * 
 * Revision 2.1  85/11/24  11:51:14  shore
 * Product Release 2.0
 * 
 * Revision 1.4  85/11/20  00:55:32  shore
 * Support for System V
 * argv changes to work with both
 * 4.2bsd and Sys V spooler interface
 * 
 * Revision 1.3  85/06/16  20:31:19  shore
 * fixed page-break bug
 * 
 * Revision 1.2  85/05/14  11:27:46  shore
 * fixed blank page bug
 * 
 *
 */

#include <stdio.h>
#include <ctype.h>
#include "transcript.h"

#define MAXWIDTH 132
#define MAXLINES 12

VOID StartPage();

private char	buf[MAXLINES][MAXWIDTH];/* MAXLINE lines of MAXWIDTH chars */
private int	maxcol[MAXLINES] = {-1};/* max col used in each lines */

private int	width = 132;
private int	length = 66;
private int	indent = 0;
private int	controls;
private char	*prog;

main(argc, argv)
int argc;
char **argv;
{
    register char *cp;
    register int ch = 0;
    int	lineno = 0;
    int npages = 1;
    int blanklines = 0;
    int donepage = 0;
    int done, linedone, maxline, i, col;
    int prevch;
    char tempfile[512];
    char *l, *libdir;

    prog = *argv;

    /* initialize line buffer to blanks */
    done = 0;
    for (cp = buf[0], l = buf[MAXLINES]; cp < l; *cp++ = ' ');

    /* put out header */
    if ((libdir = envget("PSLIBDIR")) == NULL) libdir = LibDir;
    if (copyfile(mstrcat(tempfile,libdir,TEXTPRO,sizeof tempfile), stdout)) {
	fprintf(stderr,"%s: trouble copying text prolog\n",prog);
	exit(2);
    }
    while (!done) {
	col = indent;
	maxline = -1;
	linedone = 0;
	while (!linedone) {
	    prevch = ch;
	    switch (ch = getchar()) {
		case EOF:
			linedone = done = 1;
			break;
		case '\f':
			if ((lineno == 0) && (prevch == '\f')) {
			    StartPage(npages);
			    donepage = 1;
			}
			lineno = length;
			linedone = 1;
			break;
		case '\n':
			linedone = 1;
			break;
		case '\b':
			if (--col < indent) col = indent;
			break;
		case '\r':
			col = indent;
			break;
		case '\t':
			col = ((col - indent) | 07) + indent + 1;
			break;

		default:
			if ((col >= width) ||
			    (!controls && (!isascii(ch) || !isprint(ch)))) {
			    col++;
			    break;
			}
			for (i = 0; i < MAXLINES; i++) {
			    if (i > maxline) maxline = i;
			    cp = &buf[i][col];
			    if (*cp == ' ') {
				*cp = ch;
				if (col > maxcol[i])
				    maxcol[i] = col;
				break;
			    }
			}
			col++;
			break;
	    }
	}
	/* print out lines */
	if (maxline == -1) {
	    blanklines++;
	}
	else {
	    if (blanklines) {
		if (!donepage) {
		    StartPage(npages);
		    donepage = 1;
		}
		if (blanklines == 1) {
		    printf("B\n");
		}
		else {
		    printf("%d L\n", blanklines);
		}
		blanklines = 0;
	    }
	    for (i = 0; i <= maxline; i++) {
		if (!donepage) {
		    StartPage(npages);
		    donepage = 1;
		}
		putchar('(');
		for (cp = buf[i], l = cp+maxcol[i]; cp <= l;) {
		    switch (*cp) {
			case '(': case ')': case '\\':
			    putchar('\\');
			default:
			    putchar(*cp);
			    *cp++ = ' ';
		    }
		}
		printf(")%s\n", (i < maxline) ? "" : "S");
	        maxcol[i] = -1;
	    }
	}
	if (++lineno >= length) {
	    if (donepage) {
		npages++;
		printf("EndPage\n");
		donepage = 0;
	    }
	lineno = 0;
	blanklines = 0;
	}
    }
    if (lineno && donepage) {
	printf("EndPage\n");
	donepage = 0;
	npages++;
    }
    printf("%%%%Trailer\n");
    VOIDC fclose(stdout);
    exit(0);
}

VOID StartPage(n) int n;
{
    printf("%%%%Page: %d %d\nStartPage\n",n,n);
}
