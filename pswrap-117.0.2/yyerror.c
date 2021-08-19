/*
  yyerror.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Andrew Shore: Tue Jun  7 15:53:35 1988
Richard Cohn: Fri Oct 21 15:39:29 1988
End Edit History.
*/

#include <stdio.h>
#include <ctype.h>

/* ErrIntro prints a standard intro for error messages */
/* change it if your system uses something */

#define INTRO	"# In function %s -\n"

#ifdef os_mpw
#define FMT		"File \"%s\"; Line %d # "
#else
/* for BSD use: */
#define FMT "\"%s\", line %d: "
/* for gnu/gcc use: #define FMT "%s:%d: " */
#endif

void ErrIntro(line) int line; {
    extern char *ifile;
    extern int reportedPSWName;
    extern char	*currentPSWName;
    extern int errorCount;

    if (! reportedPSWName && currentPSWName) {
		reportedPSWName = 1;
		fprintf(stderr,INTRO,currentPSWName);
    }
    fprintf(stderr,FMT,ifile,line);
    errorCount++;
}


yyerror(errmsg)
char *errmsg;
{
    extern char yytext[];
    extern int yylineno;

    ErrIntro(yylineno);
    fprintf(stderr,"%s near text \"%s\"\n",errmsg,yytext);
}

