#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1987 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Header: psplot.c,v 2.2 87/11/17 16:52:07 byron Rel $";
#endif
/* psplot.c
 *
 * Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * convert plot(5) file to a PostScript print file
 *
 * all the smarts are in the prolog, so the conversion is very "soft"
 *
 * this is a straightforward filter from stdin to stdout.
 *
 * RCSLOG:
 * $Log:	psplot.c,v $
 * Revision 2.2  87/11/17  16:52:07  byron
 * Release 2.1
 * 
 * Revision 2.1.1.4  87/11/12  13:41:48  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.3  87/04/23  10:26:36  byron
 * Copyright notice.
 * 
 * Revision 2.1.1.2  87/04/22  10:04:17  byron
 * Normal exit now returns 0 explicitly.
 * 
 * Revision 2.1.1.1  87/04/09  11:19:36  byron
 * Fixed setting of donegraphics.  Was printing extra blank pages.
 * 
 * Revision 2.1  85/11/24  11:50:57  shore
 * Product Release 2.0
 * 
 * Revision 1.3  85/11/20  00:47:08  shore
 * bug fix in quoted characters (\ddd)
 * uses getopt
 * 
 * Revision 1.2  85/05/14  11:26:32  shore
 * 
 * 
 *
 */

#include <stdio.h>
#include <pwd.h>
#ifdef SYSV
extern unsigned short getuid();
extern struct passwd *getpwuid();
#include <string.h>
#else
#include <strings.h>
#endif
#include "transcript.h"

private int SeenFile = 0;	/* true once a file is processed */
private int UserProlog = 0;	/* true if user specified a prolog */
private char prologfile[512];	/* user specified prolog file name */
private char *prog;		/* argv[0] program name */
private char *libdir;		/* ps library directory path */
extern char *optarg;		/* getopt current opt char */
extern int optind;		/* getopt argv index */
extern int getopt();

private short int getint() {
   register int b1, b2;
   b1 = getchar() & 0377;
   b2 = getchar();
   return ((((b2>128)?(b2-256):b2)<<8) + b1);
}

private VOID CopyFile() {
    int c,d;
    short int x,y,x0,y0,x1,y1,r;
    static int curpage = 1;
    int donegraphics = 0;

    printf("%%%%Page: %d %d\n", curpage, curpage);
    while ((c = getchar()) != EOF) {
	switch (c) {
	    case 'm': /* move */
	    case 'n': /* cont */
	    case 'p': /* point */
		x = getint();
		y = getint();
		printf("%d %d %c\n", x, y, c);
		if (c != 'm') donegraphics = 1;
		break;

	    case 'l': /* line */
		x0 = getint();
		y0 = getint();
		x1 = getint();
		y1 = getint();
		/* turn it around for current point */
		printf("%d %d %d %d l\n",x1,y1,x0,y0);
		donegraphics = 1;
		break;

	    case 't': /* label */
		putchar('(');
		for(d = getchar(); d != '\n'; d = getchar()) {
		    if ((d == ')') || (d == '(') || (d == '\\'))
			putchar('\\');
		    if ((d > 0176) || (d <040)) {
			putchar('\\');
			putchar(((d>>6)&07)+'0');
			putchar(((d>>3)&07)+'0');
			putchar((d&07)+'0');
		    }
		    else putchar(d);
		}
		printf(")t\n");
		donegraphics = 1;
		break;

	    case 'a': /* arc */
		x = getint();
		y = getint();
		x0 = getint();
		y0 = getint();
		x1 = getint();
		y1 = getint();
		printf("%d %d %d %d %d %d a\n", x, y, x0, y0, x1, y1);
		donegraphics = 1;
		break;

	    case 'c': /* circle */
		x = getint();
		y = getint();
		r = getint();
		printf("%d %d %d c\n", x, y, r);
		donegraphics = 1;
		break;

	    case 'e': /* erase */
		if (donegraphics){
		    printf("e\n");
		    curpage++;
		    printf("%%%%Page: %d %d\n", curpage, curpage);
		    donegraphics = 0;
		}
		break;

	    case 'f': /* linemod */
		for(d = '/'; d != '\n'; d = getchar()) putchar(d);
		printf(" f\n");
		break;

	    case 's': /* space */
		x0 = getint();
		y0 = getint();
		x1 = getint();
		y1 = getint();
		printf("%d %d %d %d s\n", x0, y0, x1, y1);
		break;

	    default:
		fprintf(stderr, "%s: unknown plot(5) command %03o\n",prog,c);
		exit(2);
	}
    }
    if (donegraphics) {
	printf("e\n");
    }
}

main(argc, argv)
char **argv;
{
    register int argp;
    long clock;
    struct passwd *pswd;
    char hostname[256];

    prog = *argv;
    /* put out comment header, lie about magic number to always
     * avoid page reversal!
     */
    printf("%%!\n");
    pswd = getpwuid((int) getuid());
    VOIDC gethostname(hostname, sizeof hostname);
    printf("%%%%Creator: %s:%s(%s)\n",hostname,pswd->pw_name, pswd->pw_gecos);
    printf("%%%%Title: Unix plot file\n");
    printf("%%%%CreationDate: %s",(VOIDC time(&clock),ctime(&clock)));
    printf("%%%%DocumentFonts: Courier\n");
    printf("%%%%EndComments\n");

    while ((argp = getopt(argc, argv, "g:")) != EOF) {
	switch (argp) {
	    case 'g':	/* user prolog */
		VOIDC strcpy(prologfile,optarg);
		UserProlog = TRUE;
		break;

	    case '?':
	    default:
		fprintf(stderr,"%s: unknown option %c ignored\n",prog,argp);
	}
    }

    /* put out prologue */
    if (!UserProlog) {
	if ((libdir = envget("PSLIBDIR")) == NULL) libdir = LibDir;
	VOIDC sprintf(prologfile,"%s/psplot.pro",libdir);
    }
    if (copyfile(prologfile, stdout)) {
	    fprintf(stderr,"%s: can't open plot prolog file %s\n",
	     prog, prologfile);
	    exit(2);
    }
    printf("StartPSPlot\n");
    printf("%%%%EndProlog\n");

    for (; optind < argc ; optind++) {
	if (freopen(argv[optind],"r",stdin) == NULL) {
	    fprintf(stderr,"%s: can't open %s\n",prog,argv[optind]);
	    exit(1);
	}
	CopyFile();
	VOIDC fclose(stdin);
	SeenFile = TRUE;
    }
    if (!SeenFile) {
	CopyFile();
    }
    printf("%%%%Trailer\n");
    printf("EndPSPlot\n");
    VOIDC fclose(stdout);
    exit(0);
}
