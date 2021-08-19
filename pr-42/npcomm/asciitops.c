#include <sys/file.h>
#include <stdio.h>
#include <ctype.h>
#include "transcript.h"

#define MAXWIDTH 132
#define MAXLINES 12

private char    buf[MAXLINES][MAXWIDTH];
private int	maxcol[MAXLINES] = {-1};/* max col used in each lines */

#define StartPage(n) { sprintf( temp, "%%%%Page: %d %d\nStartPage\n",n,n);\
  write( outfd, temp, strlen(temp) ); }

private int	width = 132;
private int	length = 66;
private int	indent = 0;
private int	controls;
private char	*prog;

char *header1 = 
"%!PS-Adobe-1.0\n";
char *header2 = 
"%%Creator: asciitops
%%DocumentFonts: Courier
%%BoundingBox: (atend)
%%Pages: (atend)
%%EndComments
/StartPage{/sv save def 48 760 moveto}def
/ld -11.4 def
/S{count{gsave show grestore}repeat 0 ld rmoveto}def
/L{ld mul 0 exch rmoveto}def
/B{0 ld rmoveto}def
/EndPage{showpage sv restore}def
/Courier findfont 11 scalefont setfont
%%EndProlog\n";


#define OUT_NAME "/tmp/atops.XXXXXX"
static char outName[32];

char *
asciiToPS( char *fname, int infd )
{
  register char *cp;
  char ch = 0;
  int outfd; 
  register int	lineno = 0;
  int npages = 1;
  int blanklines = 0;
  int donepage = 0;
  register int i, col, maxline;
  int done, linedone;
  int prevch;
  char temp[512];
  char *l;
  
  /* initialize line buffer to blanks */
  done = 0;
  for( cp = (char *)buf[0], l = (char *)buf[MAXLINES]; cp < l; *cp++ = ' ' );
  
  strcpy( outName, OUT_NAME );
  mktemp( outName );
  if( (outfd = open( outName, O_WRONLY|O_CREAT|O_TRUNC, 0666 )) < 0 ){
    return( 0 );
  }
  
  /* put out header */
  write( outfd, header1, strlen( header1 ) );
  if( fname ){
    write( outfd, "%%Title: ", strlen( "%%Title: " ) );
    write( outfd, fname, strlen( fname ) );
    write( outfd, "\n", 1 );
  }
  write( outfd, header2, strlen( header2 ) );
  
  while (!done) {
    col = indent;
    maxline = -1;
    linedone = 0;
    while (!linedone) {
      prevch = ch;

      if( read( infd, &ch, 1 ) == 0 ){
	linedone = done = 1;
	continue;
      }
	
      switch (ch) {
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
	  if (i > maxline) 
	    maxline = i;
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
	  write( outfd, "B\n", 2 );  /* printf("B\n"); */
	}
	else {
	  sprintf( temp, "%d L\n", blanklines );
	  write( outfd, temp, strlen(temp) );
	}
	blanklines = 0;
      }
      for (i = 0; i <= maxline; i++) {
	if (!donepage) {
	  StartPage(npages);
	  donepage = 1;
	}
	write( outfd, "(", 1 );   /*	putchar('('); */
	for (cp = (char *)buf[i], l = cp+maxcol[i]; cp <= l;) {
	  switch (*cp) {
	  case '(': case ')': case '\\':
	    write( outfd, "\\", 1 );  /*	    putchar('\\'); */
	  default:
	    write( outfd, cp, 1 );    /*	    putchar(*cp); */
	    *cp++ = ' ';
	  }
	}
	sprintf( temp, ")%s\n", (i < maxline) ? "" : "S");
	write( outfd, temp, strlen(temp) );
	maxcol[i] = -1;
      }
    }

    if (++lineno >= length) {
      if (donepage) {
	npages++;
	sprintf( temp, "EndPage\n");
	write( outfd, temp, strlen(temp) );
	donepage = 0;
      }
      lineno = 0;
      blanklines = 0;
    }
  }

  if (lineno && donepage) {
    sprintf( temp, "EndPage\n");
    write( outfd, temp, strlen(temp) );
    donepage = 0;
    npages;
  }

  sprintf( temp, "%%%%Trailer\n" );
  write( outfd, temp, strlen(temp) );

  sprintf( temp, "%%%%BoundingBox: %f %f %f %f\n",
	  48.0, (float)(npages > 1 ? 7.6 : (771.4 - ((float)lineno * 11.4))),
	  564.0, 771.4 );
  write( outfd, temp, strlen(temp) );

  sprintf( temp, "%%%%Pages: %d 1\n", npages );
  write( outfd, temp, strlen(temp) );

  close( outfd );

  return( outName );
}


