/*
  pswpriv.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Paul Rovner: Wednesday, May 18, 1988 2:08:19 PM
Edit History:
Paul Rovner: Friday, May 20, 1988 3:28:46 PM
Ivor Durham: Tue May 24 19:51:52 1988
Andrew Shore: Wed Jul 13 16:38:47 1988
Richard Cohn: Fri Oct 21 14:48:59 1988
End Edit History.
*/

#ifndef PSWPRIV_H
#define PSWPRIV_H

#include "pswtypes.h"
#include "psw.h"

/********************/
/* Types */
/********************/

typedef struct _t_ItemRec *Item;
/* Forward type designator */

typedef struct {
  boolean constant;
  int val;      /* valid if constant */
  char *name;	/* valid if not constant */
  } SubscriptRec, *Subscript, ScaleRec, *Scale;

typedef int Type;

typedef struct _t_ItemRec { /* see above */
  struct _t_ItemRec *next;
  char *name;
  boolean starred, subscripted, scaled;
  Subscript subscript;         /* valid if subscripted */
  Scale scale;

  /* the fields below are filled in by PSWHeader */
  boolean isoutput;        /* true if this is an output parameter */
  int tag;        /* valid if output is true; the index of
                             this output parameter. starting from 0. */
  Type type;        /* copied from parent Arg */
  int sourceLine;
  } ItemRec;

typedef Item Items;

typedef struct _t_ArgRec {
  struct _t_ArgRec *next;
  Type type;
  Items items;
  } ArgRec, *Arg;

typedef Arg Args;

typedef struct {
  boolean isStatic;
  char *name;
  Args inArgs, outArgs;
  } HeaderRec, *Header;

typedef struct {
  long cnst;
  char *var;
  } Adr, *PAdr;

typedef struct _t_TokenRec {
  struct _t_TokenRec *next;
  Type type;
  Adr adr; /* of this token in the binary object sequence. */
  int val; /* loopholed */
  int tokenIndex;
  boolean wellKnownName; /* valid if type is T_NAME or T_LITNAME */
  int sourceLine;
  Item namedFormal;
    /* non-NIL if this token is a reference to a formal.
       (T_STRING, T_HEXSTRING, T_NAME, and T_LITNAME) */
  Adr body;
    /* Meaning depends on the token type, as follows:
         simple => unused
         array or proc => adr of body in binobjseq
         string or hexstring => adr of body in binobjseq
         name or litname => adr of namestring or array in binobjseq (named arg)
                            or cnst = the nametag (well-known name)
                            or cnst = 0 (name index filled in at runtime)
	 subscripted => index for element
    */
} TokenRec, *Token;

typedef Token Tokens;

typedef Tokens Body;

typedef struct _t_TokenListRec {
  struct _t_TokenListRec *next;
  Token token;
  } TokenListRec, *TokenList;


#endif PSWPRIV_H
