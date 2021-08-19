/*
  classarray.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft, May 1988
Edit History:
Ed Taft: Sun May 22 13:40:26 1988
End Edit History.

This defines the contents of the character code to character class map.
It is used twice to produce two tables: one with the binary token types
treated specially, once with them treated as ordinary characters.
The domain of the map is [-1, 255].
*/

  eos,						/* EOF = -1 */
  dlm,						/* NUL */
  oth, oth, oth, oth, oth, oth, oth, oth,	/* \001 - \010 */
  dlm,						/* TAB */
  lff,						/* LF */
  oth,						/* \013 */
  lff,						/* FF */
  crt,						/* CR */
  oth, oth,					/* \016 - \037 */
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  dlm,						/* SP */
  oth, oth,					/* !" */
  nbs,						/* # */
  oth,						/* $ */
  com,						/* % */
  oth, oth,					/* &' */
  lpr,						/* ( */
  rpr,						/* ) */
  oth,						/* * */
  sgn,						/* + */
  oth,						/* , */
  sgn,						/* - */
  dot,						/* . */
  nme,						/* / */
  oct, oct, oct, oct, oct, oct, oct, oct,	/* 01234567 */
  dec, dec,					/* 89 */
  oth, oth,					/* :; */
  lab,						/* < */
  oth,						/* = */
  rab,						/* > */
  oth, oth,					/* ?@ */
  hex, hex, hex, hex,				/* ABCD */
  exp,						/* E */
  hex,						/* F */
  ltr,						/* G - Z */
  ltr, ltr, ltr, ltr, ltr, ltr, ltr, ltr,
  ltr, ltr, ltr, ltr, ltr, ltr, ltr, ltr,
  ltr, ltr, ltr,
  lbk,						/* [ */
  qot,						/* \ */
  rbk,						/* ] */
  oth, oth, oth,				/* ^-` */
  hex,						/* a */
  escbf,					/* b */
  hex, hex,					/* cd */
  exp,						/* e */
  escbf,					/* f */
  ltr, ltr, ltr, ltr, ltr, ltr, ltr,		/* g - m */
  escnrt,					/* n */
  ltr, ltr, ltr,				/* opq */
  escnrt,					/* r */
  ltr,						/* s */
  escnrt,					/* t */
  ltr, ltr, ltr, ltr, ltr, ltr,			/* u - z */
  lbr,						/* { */
  oth,						/* | */
  rbr,						/* } */
  oth, oth,					/* ~ DEL */
#if BINARY_ENCODINGS
  btokhdr, btokhdr, btokhdr, btokhdr,		/* 128 - 131 */
  btokhi,					/* 132 */
  btoklo,					/* 133 */
  btokhi,					/* 134 */
  btoklo,					/* 135 */
  btokn,					/* 136 */
  bfn,						/* 137 */
  btokhi,					/* 138 */
  btoklo,					/* 139 */
  btokn, btokn, btokn,				/* 140 - 142 */
  btokhi,					/* 143 */
  btoklo,					/* 144 */
  btokn, btokn, btokn, btokn,			/* 145 - 148 */
  btokhdr,					/* 149 */
  bund, bund, bund, bund, bund, bund, bund,	/* 150 - 159 */
  bund, bund, bund,
#else BINARY_ENCODINGS
  oth, oth, oth, oth, oth, oth, oth, oth,	/* 128 - 159 */
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
#endif BINARY_ENCODINGS
  oth, oth, oth, oth, oth, oth, oth, oth,	/* 160 - 255 */
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth,
  oth, oth, oth, oth, oth, oth, oth, oth

