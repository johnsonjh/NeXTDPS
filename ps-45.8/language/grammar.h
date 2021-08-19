/*
  grammar.h

Copyright (c) 1984, 1986, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: January 24, 1983
Edit History:
Chuck Geschke: Wed Jul 11 16:00:06 1984
Ed Taft: Tue Jun 14 16:18:28 1988
Ivor Durham: Mon Jun 20 09:26:55 1988
End Edit History.

Grammar-related definitions for the PostScript scanner.
*/

#if 0
/* The following declarations are processed by the new_grammar
   shell script, which regenerates the list of #define statements
   below and rewrites grammar.h. Each enumeration is declared by:

   enumeration =
     name1
     name2
     ...

   The script can deal with blank lines, single-line comments,
   and line-ending comments; it cannot deal with multi-line comments.
 */

/* BEGIN_GRAMMAR */

Class =
  eos		/* end of source */
  crt		/* CR	comment terminating white space */
  lff		/* LF FF  comment terminating white space */
  dlm		/* SP TAB NUL  white space, excluding crt and lff */
  com		/* %	comment */
  qot		/* \	quoting char in strings */
  lpr		/* (	start of ordinary string token */
  rpr		/* )	end of ordinary string token */
  lab		/* <	start of hex string token */
  rab		/* >	end of hex string token */
  lbr		/* {	start of (executable) array token */
  rbr		/* }	end of (executable) array token */
  lbk		/* [	start of (data) array token */
  rbk		/* ]	end of (data) array token */
  nme		/* /	start of name token */
  sgn		/* +-	sign */
  dot		/* .	period */
  nbs		/* #	number base */
  oct		/* 01234567  octal digit */
  dec		/* 89	decimal digit, excluding oct */
  exp		/* Ee	exponent */
  escbf		/* bf	char after "\" escape in string */
  escnrt	/* nrt	char after "\" escape in string */
  hex		/* ABCDFacd  hex digit, excluding exp and escbf */
  ltr		/* A-Z, a-z, excluding exp, escbf, escnrt, hex */
  oth		/* other */

  /* remaining classes are binary token types */
  btokhi	/* tokens scanned high byte first */
  btoklo	/* tokens scanned low byte first */
  btokn		/* tokens scanned in native order */
  bfn		/* 16- or 32-bit fixed point (type doesn't encode order) */
  btokhdr	/* token with descriptive header (obj seq, num array) */
  bund		/* undefined binary token type */

  nClasses


State =
  start		/* skipping leading white space */
  comnt		/* skipping over a comment */

  /* following states perform parallel recognition of number and executable */
  /* name, since we don't know which it is until we reach the end */
  /* "num" means oct|dec; "bnum" means oct|dec|hex|exp|escbf|escnrt|ltr */
  msign		/* so far: sgn */
  ipart		/* so far: num | msign num | ipart num */
  point		/* so far: dot | msign dot */
  fpart		/* so far: point num | ipart dot | fpart num */
  expon		/* so far: ipart E | fpart E */
  esign		/* so far: expon + | expon - */
  epart		/* so far: esign num | epart num */
  bbase		/* so far: ipart # */
  bpart		/* so far: bbase bnum | bpart bnum */

  /* following states perform recognition only of name or string */
  ident		/* scanning an executable name that can't be a number */
  slash		/* just scanned initial "/" */
  ldent		/* scanning a literal name */
  edent		/* scanning an immediately evaluated name */
  litstr	/* scanning normal string literal */
  escchr	/* just scanned "\" within string */
  escoct1	/* scanned 1 octal digit after "\" */
  escoct2	/* scanned 2 octal digits after "\" */
  hexstr	/* scanning hex string */

  /* no states associated with binary tokens; actions consume entire tokens */

  nStates


/* Actions whose names begin with "_" are associated with potential state */
/* changes; for other actions, no state change ever occurs. */
/* Actions flagged with "?" perform a conditional state change. */

Action =
  discard	/* discard character */
  _discard	/* discard character */
  disceol	/* discard CR and following LF if there is one; */
		/* LF is scanned next even if not present in input */
  cdisceol	/* = disceol, but for use only when CR terminates token */

  /* following actions performed within token only */
  _begnum	/* first character of text for possible number or name */
  _begxnum	/* one of "+-." seen, begin text and possible number */
  _begname	/* first character of text for executable name */
  _begxtext	/* begin text with next character for name or string */
  appname	/* append to name (buffer not expandable) */
  _appname	/* append to name (buffer not expandable) */
  apphex	/* append hex digit to text */
  appint	/* append integer digit */
  _appint	/* append integer digit */
  appfrac	/* append fraction digit */
  _appfrac	/* append fraction digit */
  _begbnum	/* ? first digit of mantissa of based number */
  appbnum	/* ? append digit to based number */
  _begexp	/* begin exponent */
  appstr	/* append to string (buffer expandable) */
  _appstr	/* append to string (buffer expandable) */
  _strnest	/* append nested "(" to string */
  _appeol	/* handle CR|LF|FF and append EOL to string */
  _appesc	/* append "\" escaped byte to string */
  _esceol	/* handle "\" followed by CR|LF|FF */
  _begoct	/* handle first octal digit after "\" */
  _appoct	/* handle subsequent octal digit after "\" */

  /* following actions complete a token */
  _uxname	/* back up and finish executable name */
  _xname	/* finish executable name */
  _ulname	/* back up and finish literal name */
  _lname	/* finish literal name */
  _uename	/* back up and finish immediately evaluated name */
  _ename	/* finish immediately evaluated name */
  _strg		/* ? finish string literal (if not nested paren) */
  _hstrg	/* finish string literal */
  _uinum	/* back up and finish integer number */
  _inum		/* finish integer number (convert to real if overflow) */
  _ubnum	/* back up and finish based integer number */
  _bnum		/* finish based integer number (limitcheck if overflow) */
  _urnum	/* back up and finish real number */
  _rnum		/* finish real number */
  axbeg		/* process "{" -- begin executable array */
  axend		/* process "}" -- end executable array */
  adbeg		/* process "[" -- emit executable name for it */
  adend		/* process "]" -- emit executable name for it */
  _empty	/* end of source with no token encountered */
  _errstr	/* syntaxerror: source ended with unterminated string */
  _errini	/* syntaxerror: illegal initial char, e.g., extraneous ">)}" */
  _errhex	/* syntaxerror: malformed hex string */

  /* following actions construct a binary token, consuming one or more */
  /* additional characters in the process */
  abtokhi	/* scan token, high byte first */
  abtoklo	/* scan token, low byte first */
  abtokn	/* scan token, native order */
  abfn		/* scan 16- or 32-bit fixed point number */
  abtokhdr	/* scan token with descriptive header */

  nActions


/* Following actions used in the processing of binary encoded tokens. */
/* They are here just to get them enumerated and aren't used in STbuild.c */

BTAction =
  ba_objSeq	/* binary object sequence */
  ba_int32	/* integer object from 32 bits of data */
  ba_int16	/* integer object from 16 bits of data */
  ba_int8	/* integer object from 8 bits of data */
  ba_fixed	/* fixed (integer or real) from 16 or 32 bits of data */
  ba_realIEEE	/* real object from IEEE */
  ba_realNative	/* real object from native */
  ba_bool	/* boolean object */
  ba_sstr	/* string object given 8 bit length */
  ba_lstr	/* string object given 16 bit length */
  ba_systemName	/* name from system name index */
  ba_userName	/* name from user name index */
  ba_numArray	/* homogeneous number array */


/* END_GRAMMAR */
#endif 0

/* The following definitions are regenerated automatically by the
   new_grammar script; they should not be edited manually.
 */

/* BEGIN_DEFINES */

/* Class enumeration: */
#define eos 0
#define crt 1
#define lff 2
#define dlm 3
#define com 4
#define qot 5
#define lpr 6
#define rpr 7
#define lab 8
#define rab 9
#define lbr 10
#define rbr 11
#define lbk 12
#define rbk 13
#define nme 14
#define sgn 15
#define dot 16
#define nbs 17
#define oct 18
#define dec 19
#define exp 20
#define escbf 21
#define escnrt 22
#define hex 23
#define ltr 24
#define oth 25
#define btokhi 26
#define btoklo 27
#define btokn 28
#define bfn 29
#define btokhdr 30
#define bund 31
#define nClasses 32

/* State enumeration: */
#define start 0
#define comnt 1
#define msign 2
#define ipart 3
#define point 4
#define fpart 5
#define expon 6
#define esign 7
#define epart 8
#define bbase 9
#define bpart 10
#define ident 11
#define slash 12
#define ldent 13
#define edent 14
#define litstr 15
#define escchr 16
#define escoct1 17
#define escoct2 18
#define hexstr 19
#define nStates 20

/* Action enumeration: */
#define discard 0
#define _discard 1
#define disceol 2
#define cdisceol 3
#define _begnum 4
#define _begxnum 5
#define _begname 6
#define _begxtext 7
#define appname 8
#define _appname 9
#define apphex 10
#define appint 11
#define _appint 12
#define appfrac 13
#define _appfrac 14
#define _begbnum 15
#define appbnum 16
#define _begexp 17
#define appstr 18
#define _appstr 19
#define _strnest 20
#define _appeol 21
#define _appesc 22
#define _esceol 23
#define _begoct 24
#define _appoct 25
#define _uxname 26
#define _xname 27
#define _ulname 28
#define _lname 29
#define _uename 30
#define _ename 31
#define _strg 32
#define _hstrg 33
#define _uinum 34
#define _inum 35
#define _ubnum 36
#define _bnum 37
#define _urnum 38
#define _rnum 39
#define axbeg 40
#define axend 41
#define adbeg 42
#define adend 43
#define _empty 44
#define _errstr 45
#define _errini 46
#define _errhex 47
#define abtokhi 48
#define abtoklo 49
#define abtokn 50
#define abfn 51
#define abtokhdr 52
#define nActions 53

/* BTAction enumeration: */
#define ba_objSeq 0
#define ba_int32 1
#define ba_int16 2
#define ba_int8 3
#define ba_fixed 4
#define ba_realIEEE 5
#define ba_realNative 6
#define ba_bool 7
#define ba_sstr 8
#define ba_lstr 9
#define ba_systemName 10
#define ba_userName 11
#define ba_numArray 12
/* END_DEFINES */


/* Remainder of this file contains arbitrary definitions that are
   not altered by new_grammar.
 */

typedef unsigned char Class, State, Action;

typedef struct {
  Action actions[nClasses];
  State newStates[nClasses];
} StateRec, *PStateRec;

/* Symbolic names for certain characters */
#define BS  '\010'
#define TAB '\011'
#define LF  '\012'
#define FF  '\014'
#define CR  '\015'

/* Canonical end-of-line character -- what appears in a string literal
   when CR, LF, CR LF, or \n is scanned. This is LF normally, but
   CR in an environment whose standard end-of-line is CR only.
   Note that the definition of EOL does not affect the language accepted
   by the scanner (all end-of-line sequences are always accepted); it
   determines only what character is actually put in the string object.
 */

#if ('\n' == CR)
#define EOL CR
#else
#define EOL LF
#endif
