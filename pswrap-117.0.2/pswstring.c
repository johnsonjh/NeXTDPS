/*
  pswstring.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Shore 
Edit History:
Andrew Shore: Fri Jul  1 10:24:17 1988
Richard Cohn: Fri Oct 21 15:38:26 1988
End Edit History.
*/

#include <stdio.h>
#include <ctype.h>

#ifdef os_mpw
#define outfil rdata
extern FILE *rdata;
#define MAX_PER_LINE 64
#else
#define outfil stdout
#define MAX_PER_LINE 16
#endif

extern int outlineno;		/* line number in output file */

int PSWStringLength(s) char *s; {
    register char *c = s;
    register int len = 0;

    while (*c != '\0') {	/* skip \\ and \ooo */
	if (*c++ == '\\') {
	    if (*c++ != '\\') c += 2;
	}
	len++;
    }
    return (len);
}

PSWOutputStringChars(s) char *s; {
    register char *c = s;
    register char b;
    register int perline = 0;
    
#ifdef os_mpw
    fprintf(outfil,"    \"");
#endif
    while (*c != '\0') {
#ifndef os_mpw
    putc('\'',outfil);
#endif
    switch (b = *c++) {
	    case '\\':
	        putc('\\',outfil);
		fputc(b = *c++,outfil);
	        if (b != '\\') {putc(*c++,outfil);putc(*c++,outfil);}
		break;
	    case '\'':
	        fprintf(outfil,"\\'");
		break;
	    case '\"':
	        fprintf(outfil,"\\\"");
		break;
	    case '\b':
	        fprintf(outfil,"\\b");
		break;
	    case '\f':
	        fprintf(outfil,"\\f");
		break;
/* avoid funny interpretations of \n, \r by MPW */
	    case '\012':
	        fprintf(outfil,"\\012"); perline++;
		break;
	    case '\015':
	        fprintf(outfil,"\\015"); perline++;
		break;
	    case '\t':
	        fprintf(outfil,"\\t");
		break;
	    default:
		putc(b,outfil); perline--;
		break;
	}
#ifdef os_mpw
	if (*c != '\0') {
	    if (++perline >= MAX_PER_LINE) {
		fprintf(outfil,"\"\n    \"");
		outlineno++;
	    }
	    perline %= MAX_PER_LINE;
	}
#else
	putc('\'',outfil);
	if (*c != '\0') {
	    if (++perline >= MAX_PER_LINE) {
		fprintf(outfil,",\n     ");
		outlineno++;
	    }
	    else {putc(',',outfil);}
	    perline %= MAX_PER_LINE;
	}
#endif
    }
#ifdef os_mpw
    fprintf(outfil,"\",\n");
#endif
}


int PSWHexStringLength(s) char *s; {
    return ((int) (strlen(s)+1)/2);
}

PSWOutputHexStringChars(s)
    register char *s;
{
    register int perline = 0;
#ifdef os_mpw
    register char b;
    register int n;
    
    fprintf(outfil,"    \"");
    while ((b = *s++) != '\0') {
	if (('0' <= b) && (b <= '9')) n = (b - '0') << 4;
	else if (('a' <= b) && (b <= 'f')) n = (b - 'a' + 10) << 4;
	else if (('A' <= b) && (b <= 'F')) n = (b - 'A' + 10) << 4;
	if ((b = *s++) != '\0') {
	    if (('0' <= b) && (b <= '9')) n |= (b - '0');
	    else if (('a' <= b) && (b <= 'f')) n |= (b - 'a' + 10);
	    else if (('A' <= b) && (b <= 'F')) n |= (b - 'A' + 10);
	}

	/* n is hex char */
	if (isascii(n) && isprint(n) && n != (int) '\\' && n != (int) '\"')
	    {putc(n,outfil); perline++;}
	else
	    {fprintf(outfil,"\\%.3o",n&0377); perline += 4;}
	
	if (b == '\0')
	    break;
	if (*s != '\0') {
	    if (perline >= MAX_PER_LINE) {
		fprintf(outfil,"\"\n    \"");
		outlineno++;
	    }
	    perline %= MAX_PER_LINE;
	}
    }
    fprintf(outfil,"\",\n");
#else
    char tmp[3];

    tmp[2] ='\0';
    while ((tmp[0] = *s++)!= '\0') {
	tmp[1] = *s ? *s++ : '\0';
	fprintf(outfil,"0x%s",tmp);
	if (*s != '\0') {
	    if (++perline >= MAX_PER_LINE) {
		fprintf(outfil,",\n     ");
		outlineno++;
	    }
	    else {putc(',',outfil);}
	    perline %= MAX_PER_LINE;
	}
    } /* while */
#endif
}
