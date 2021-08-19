%{
/*
  lexer.l

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Andrew Shore 
Edit History:
Andrew Shore: Wed Aug 17 10:59:46 1988
Ivor Durham: Tue May 24 19:53:49 1988
William Bilodeau: Fri Jun 29 17:38:35 PDT 1990
Richard Cohn: Fri Oct 21 15:17:07 1988
End Edit History.
*/

#ifdef os_mpw
#include <cursorctl.h>
#endif
#include <ctype.h>
#include "pswpriv.h"
#include "y.tab.h"


extern int outlineno;		/* output line number */

extern int maxstring;		/* max PS string length to scan (-s) */
extern char *string_temp;	/* buffer of that size for scanning strings */
extern char *psw_malloc();
extern void stringerr();	/* report string overflow */
extern char *ifile;		/* input file name for error messages */

static int parens = 0;		/* current paren balancing */
static char *sbody;		/* cur pointer into string_temp */
static int curleng = 0;		/* current scanned string length */
static int strlineno;		/* start line of current string */

static int nonComment = true;	/* are we outside C comments? */
static int newLine = true;	/* are we starting a new line? */
static int hexstringerrs = 0;	/* found an error in this hex string */

int errorCount = 0;		/* number of non-fatal errors */


/* for debugging */
#ifdef PSWDEBUG
int lexdebug = 1;		/* debug flag */
#define DEBUGP(x) {if (lexdebug) { fprintf x ; };}
#else
#define DEBUGP(x)
#endif
#ifdef LEXDEBUG
#ifdef os_mpw
void allprint(c)
	char c;
{
	if (c >= ' ') putc((c), yyout);
	else if (c == 10) fputs("\\n", yyout);
	else fprintf(yyout, "\\%x", c);
}
void sprint(s)
	char *s;
{
	char *p = s;
	while(*p) allprint(*p++);
}
#endif
#endif


#ifdef os_mpw
#undef input
#undef unput
#define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==13?(yylineno++,yytchar=10):yytchar)==EOF?0:yytchar)
#define unput(c) {yytchar= (c);if(yytchar==10){yylineno--;yytchar=13;}*yysptr++=yytchar;}
#endif

#undef YYLMAX
#define YYLMAX 16384
%}

%o 3500
%a 2500

%START PS STR HEX DEF SUB

DIGIT		([0-9])
HEXDIGIT	([a-fA-F0-9])
RADIXDIGIT	([a-zA-Z0-9])
LETTER		([a-zA-Z])
SIGN		([+-])
EXP		([eE]{SIGN}?{DIGIT}+)
W		([ \t\n\r])

%%

<PS>^defineps 	{	/* inside a wrap - just a PS token */
	if (nonComment) {
	    newLine = false;
	    yylval.object = psw_malloc(strlen(yytext) + 1);
	    strcpy(yylval.object, yytext);
	    DEBUGP((stderr,"PSNAME ->%s\n",yytext));
	    return PSNAME;
	}
}

<DEF>^defineps	{ 	/* treat as a C name inside the def part */
	if (nonComment) {
	    yylval.object = psw_malloc(strlen(yytext) + 1);
	    strcpy(yylval.object, yytext);
	    DEBUGP((stderr,"CNAME ->%s\n",yytext));
	    return CNAME;
	}
}

<STR>^defineps	{	/* part of string text */
	register int i = yyleng;
	register char *c = yytext;

	if (yyleng + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	curleng += yyleng;
	for (; i > 0; --i) *sbody++ = *c++;
}

^{W}*defineps { 		/* start of a wrap definition */
	if (nonComment) {
	    BEGIN DEF;
	    DEBUGP((stderr,"DEFINEPS\n"));
	    return DEFINEPS;
	} else ECHO;
}

<PS>endps {		/* end of a wrap */
	if (nonComment){
	    if (newLine) {
	        newLine = false;
	    	DEBUGP((stderr,"ENDPS\n"));
	    	BEGIN 0;
	    	return ENDPS;
	    } else {
	    	DEBUGP((stderr,"PSNAME %s\n",yytext));
	    	yylval.object = psw_malloc(strlen(yytext) + 1);
	    	strcpy(yylval.object, yytext);
	    	return PSNAME;
	    }
	}	    
}

<DEF>^endps	{ 	/* treat as a C name inside the def part? */
	if (nonComment) {
	    yylval.object = psw_malloc(strlen(yytext) + 1);
	    strcpy(yylval.object, yytext);
	    DEBUGP((stderr,"CNAME ->%s\n",yytext));
	    return CNAME;
	}
}

<STR>^defineps	{	/* part of string text */
	register int i = yyleng;
	register char *c = yytext;

	if (yyleng + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	curleng += yyleng;
	for (; i > 0; --i) *sbody++ = *c++;
}

endps { 		/* BOGUS end of a wrap */
	if (nonComment) {
	    ErrIntro(yylineno);
	    fprintf(stderr,"endps without matching defineps\n");
	} else ECHO;
}

%{
    	/* real definition stuff */
%}

<DEF>"/*" {		/* C comments on */
	nonComment = false;
}

<DEF>"*/" {		/* C comments off */
	nonComment = true;
}

<DEF>"("	{	/* formals start */
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->(\n"));
	    return '(';
	}
}

<DEF>")"	{	/* formals end */
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->)\n"));
	    BEGIN PS;
	    return ')';
	}
}

<DEF>[|,;[\]*:]	{	/* special single chars */
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->%c\n",yytext[0]));
	    return (yytext[0]);
	}
}

%{
	/* formals type names */
%}

<DEF>"static" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->static\n"));
	    return STATIC;
	}
}

<DEF>"boolean" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->boolean\n"));
	    return BOOLEAN;
	}
}

<DEF>"char" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->char\n"));
	    return CHAR;
	}
}

<DEF>"int" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->int\n"));
	    return INT;
	}
}

<DEF>"short" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->short\n"));
	    return SHORT;
	}
}

<DEF>"long" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->long\n"));
	    return LONG;
	}
}

<DEF>"unsigned" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->unsigned\n"));
	    return UNSIGNED;
	}
}

<DEF>"float" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->float\n"));
	    return FLOAT;
	}
}

<DEF>"double" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->double\n"));
	    return DOUBLE;
	}
}

<DEF>"userobject" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->userobject\n"));
	    return USEROBJECT;
	}
}

<DEF>"numstring" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->numstring\n"));
	    return NUMSTRING;
	}
}

<DEF>"DPSContext" {
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->DPScontext\n"));
	    return PSCONTEXT;
	}
}

%{
	/* C format numbers in formals part */
%}

<DEF>[1-9]{DIGIT}*	{ 		/* base 10 number */
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->CINTEGER %s\n",yytext));
	    yylval.intobj = atoi(yytext);
	    return CINTEGER;
	}
}

<DEF>0{DIGIT}*		{		/* octal number (and zero) */
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->CINTEGER %s\n",yytext));
	    sscanf(yytext,"%lo",&yylval.intobj);
	    return CINTEGER;
	}
}

<DEF>0[xX]{HEXDIGIT}+	{		/* hex number */
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->CINTEGER %s\n",yytext));
	    sscanf(yytext,"0x%lx",&yylval.intobj);
	    return CINTEGER;
	}
}

<DEF>[a-zA-Z_][a-zA-Z0-9_]* {	/* C identifier */
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->CNAME %s\n",yytext));
	    yylval.object = psw_malloc(strlen(yytext) + 1);
	    strcpy(yylval.object, yytext);
	    return CNAME;
	}
}

<DEF>{W}+ {
}

<DEF>[^a-zA-Z0-9_|,;[\]*:\(\)/ \t\n\r]+ {	/* all else - ERROR */
	if (nonComment) {
	    DEBUGP((stderr,"DEF ->JUNK %s\n",yytext));
	    ErrIntro(yylineno);
	    fprintf(stderr,"invalid characters '%s' in definition\n",
	    	yytext);
	}
}


%{
	/* body part - PostScript code */
%}

<PS>\%.*$ {			/* a PS comment, ignore */
	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"comment %s\n",yytext));
	    /* yylval = yytext; return PSCOMMENT; */
	}
}

<PS>{SIGN}?{DIGIT}+ {		/* a decimal integer */
	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"PSINTEGER ->%s\n",yytext));
	    yylval.intobj = atoi(yytext);
	    return PSINTEGER;
	}
}

<PS>{DIGIT}+#{RADIXDIGIT}+ {	/* a radix number */
#define MAX_ULONG ((unsigned long) -1)
	int base = 0;
	char *s = yytext;
	register unsigned long x, limit;
	register unsigned long i = 0;

	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"PSINTEGER radix->%s\n",yytext));
	    while (*s != '#') {
		base *= 10;
		base += *s++ - '0';
		if (base > 32) break;
	    }

	    if ((base < 2) || (base > 36)) {goto error;}
	    else {
		limit = MAX_ULONG / base;
		s++; /* skip # sign */
		while (*s != '\0') {
		    if (i > limit) goto error;
		    i *= base;
		    if (isdigit(*s)) { x = *s - '0';}
		    else { x = (*s | 040) - ('a'-10);}
		    if (x >= base) goto error;
		    if (x > (MAX_ULONG - i)) goto error;
		    i += x;
		    s++;
		}
	    }
	    yylval.intobj = i;
	    return PSINTEGER;

	    error:;
	    ErrIntro(yylineno);
	    fprintf(stderr,"invalid radix number %s\n",yytext);
	    yylval.intobj = 0; /* ERROR */
	    return PSINTEGER;
	}
}

<PS>(({SIGN}?{DIGIT}+(\.{DIGIT}*)?{EXP}?)|({SIGN}?{DIGIT}*\.{DIGIT}+{EXP}?)) {
				/* a float */
	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"PSREAL ->%s\n",yytext));
	    yylval.object = psw_malloc(strlen(yytext) + 1);
	    strcpy(yylval.object,yytext);
	    return PSREAL;
	}
}

<PS>"(" {			/* start PS string */
	if (nonComment) {
	    newLine = false;
	    BEGIN STR;
	    parens=1;
	    sbody = string_temp;
	    curleng = 0;
	    strlineno = yylineno;
	    *sbody = '\0';
	}
}

%{
	/* inside PS strings */
%}

<STR>"(" {			/* balancing parens */
	if (yyleng + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	curleng += yyleng;
	parens++;
	*sbody++ = '(';
}

<STR>")" {			/* balancing parens or end string */
	if ((--parens) == 0) {
		BEGIN PS;
		*sbody = '\0';
		yylval.object = psw_malloc(strlen(string_temp) + 1);
		strcpy(yylval.object, string_temp);
		DEBUGP((stderr,"PSSTRING ->%s\n",string_temp));
		return PSSTRING;
	}
	else if (yyleng + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	curleng += yyleng;
	*sbody++ = ')';
}

<STR>"\\"[0-7]([0-7]?)([0-7]?) {	/* quoted octal byte */
	register char *c = yytext;

	if (4 + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	curleng += 4;
	sprintf(sbody,"\\%03.3s",(yytext + 1));
	sbody += 4;
}


<STR>"\\\n"	{ /* ignore quoted newline */
}

<STR>"\\\\"	{ /* keep quoted backslashes in */
	if (yyleng + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	*sbody++ = '\\';
	*sbody++ = '\\';
	curleng += 2;
}


<STR>("\\".) {		/* other quoted char, including parens */
	if (yyleng + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	switch (yytext[1]) {
	case 'b':
	    *sbody++ = '\b';
	    break;
	case 'f':
	    *sbody++ = '\f';
	    break;
	case 'n':
	    *sbody++ = '\012';
	    break;
	case 'r':
	    *sbody++ = '\015';
	    break;
	case 't':
	    *sbody++ = '\t';
	    break;
	default:
	    *sbody++ = yytext[1];
	    break;
	}
	curleng++;
}


<STR>"\n" {		/* newline in a string */
	if (yyleng + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	curleng += yyleng;

	*sbody++ = '\n';
}

<STR>[^()\\\n]+ {		/* anything else */
	register int i = yyleng;
	register char *c = yytext;

	if (yyleng + curleng >= maxstring) {
		stringerr(1);
		BEGIN PS;
		*sbody = '\0';
		return PSSTRING;
	}
	curleng += yyleng;
	for (; i > 0; --i) *sbody++ = *c++;
}

%{
	/* hexidecimal strings "< >" */
%}

<PS>"<" {			/* begin hex string */
	if (nonComment) {
	    newLine = false;
	    BEGIN HEX;
	    sbody = string_temp;
	    *sbody = '\0';
	    curleng = 0;
	    strlineno = yylineno;
	    hexstringerrs = 0;
	}
}

<HEX>{W}+ {			/* hex whitespace */
	/* ignore */
}

<HEX>[0-9A-Fa-f]+ {		/* hex string data */
	if (yyleng + curleng >= maxstring) {
		stringerr(2);
		*sbody = '\0';
		BEGIN PS;
		return PSHEXSTRING;
	}
	curleng += yyleng;
	strcpy(sbody,yytext);
	sbody += yyleng;
}

<HEX>">" {			/* end hex string */
	BEGIN PS;
	*sbody = '\0';
	DEBUGP((stderr,"PSHEXSTRING ->%s\n",string_temp));
	yylval.object = psw_malloc(strlen(string_temp) + 1);
	strcpy(yylval.object, string_temp);
	return PSHEXSTRING;
}

<HEX>[^a-fA-F0-9> \t\n\r]+	{	/* ERROR */
	if (hexstringerrs == 0) { /* only complain once per string */
	    ErrIntro(yylineno);
	    fprintf(stderr,"invalid characters in hex string '%s'\n",yytext);
	    hexstringerrs++;
	}
}


%{
	/* straight postscript */
%}

<PS>[\[\]\{\}] {		/* PS self delimiter */
	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"PS ->%s\n",yytext));
	    return (yytext[0]);
	}
}

<PS>"true"|"false" {		/* boolean */
	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"PSBOOLEAN %s\n",yytext));
	    yylval.intobj = (*yytext == 't');
	    return PSBOOLEAN;
	}
}

<PS>"/"[^ <>\(\)\[\]\{\}\%\/\t\n\r]* {	/* literal name */
	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"PSLITNAME %s\n",yytext));
	    yylval.object = psw_malloc(strlen(yytext));
	    strcpy(yylval.object, yytext+1);
	    return PSLITNAME;
	}
}

<PS>[^ <>\(\)\[\]\{\}\%\/\\\t\n\r]+ {	/* executable name */
	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"PSNAME %s\n",yytext));
	    yylval.object = psw_malloc(strlen(yytext) + 1);
	    strcpy(yylval.object, yytext);
	    return PSNAME;
	}
}

<PS>"*/" {				/* special case */
	if (nonComment) {
	    newLine = false;
	    unput('/');
	    yylval.object = psw_malloc(2);
	    strcpy(yylval.object, "*");
	    return PSNAME;
	}
}

<PS>"\\"[^ <>\(\)\[\]\{\}\%\/\\\t\n\r]+/"[" {	/* \name[index] */
	if (nonComment) {
	    DEBUGP((stderr,"\\PSNAME %s\n",yytext));
	    yylval.object = psw_malloc(strlen(yytext));
	    strcpy(yylval.object, yytext+1);
	    BEGIN SUB;
	    return PSSUBNAME;
	}
}

<SUB>[\[][^ \t\]]*[\]] {	/* [index] */
	if (nonComment) {
	    int len = strlen(yytext);
	    DEBUGP((stderr,"PSINDEX %s\n",yytext));
	    if (len == 2) {
		ErrIntro(yylineno);
		fprintf(stderr,"parameter index expression empty\n");
		yylval.object = "0";
	    }
	    else {
		yylval.object = psw_malloc(len);
		strncpy(yylval.object, yytext+1, len-2);
		yylval.object[len-2] = '\0';
	    }
	    BEGIN PS;
	    return PSINDEX;
	}
}

<SUB>[\[][^\]]*[\]] { /* error */
	if (nonComment) {
	    ErrIntro(yylineno);
	    fprintf(stderr,"parameter index expression error\n");
	    yylval.object = "0";
	    BEGIN PS;
	    return PSINDEX;
	}
}

<PS>[\)\>\\]+	{	/* ERROR */
	if (nonComment) {
	    newLine = false;
	    DEBUGP((stderr,"PS JUNK ->%s\n",yytext));
	    ErrIntro(yylineno);
	    fprintf(stderr,"invalid characters '%s'\n",yytext);
	}
}

<PS>[ \t\r]+ { }

<PS>[\n]+ {
	newLine = true;
}

[ \t]+	{		/* passthru stuff */
	ECHO;
}

\n	{
	outlineno++;
#ifdef os_mpw
	SpinCursor(1);
	output('\n');
#else
	ECHO;
#endif
};

"/*" {		/* C comments on */
	nonComment = false;
	ECHO;
}

"*/" {		/* C comments off */
	if (!nonComment) {
		nonComment = true;
		ECHO;
	}
}

%%

int yywrap () {
    if (!feof(yyin))
    	return (0);
    if (yybgin != (yysvec+1)) {	/* make sure we are in state 0 */
	ErrIntro(yylineno);
	fprintf(stderr,"end of input file/missing endps\n");
    }
    return (1);
}

void stringerr(type)
	int type;
{
    extern char *currentPSWName;
    extern char *ifile;

    ErrIntro(strlineno);
    fprintf(stderr,"%sstring too long (caught at line %d)\n",
    	((type==1)?"":"hex "),yylineno);
    errorCount++;
    return;
}
