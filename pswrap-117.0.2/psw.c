/*
  psw.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Paul Rovner: Tuesday, May 24, 1988 4:20:21 PM
Edit History:
Andrew Shore: Mon Aug 15 11:49:04 1988
Bill Bilodeau: Wed Sep 7 11:09:00 1988
Richard Cohn: Mon Jan 16 16:30:27 1989
Perry Caro: Tue Sep 12 12:56:41 1989
End Edit History.

Loose ends
  complete the document

  PORTABILITY (of pswrap and of generated C code)
  checkout
  keep PS types and C types separate; do away with the conversions

*/

/***********/
/* Imports */
/***********/

#include <stdio.h>
#include <string.h>

#ifdef os_mpw
#include <CursorCtl.h>
#endif

#include "pswdict.h"
#include "pswpriv.h"

char *psw_malloc();
char *psw_calloc();

extern FILE *header;
extern PSWDict wellKnownPSNames;
extern int PSWStringLength();
extern int PSWHexStringLength();
extern void PSWOutputStringChars();
extern void PSWOutputHexStringChars();
extern int outlineno;
extern int reentrant;
extern int maxstring;

extern Token PSWToken(/* type, val */);

extern int doANSI;	/* -a flag */
extern int pad;		/* -p flag */

#define DPS_HEADER_SIZE 4
#define DPS_LONG_HEADER_SIZE 8
#define DPS_BINOBJ_SIZE 8	/* sizeof(DPSBinObjGeneric) */
#define WORD_ALIGN 3
#define HNUMTOKEN 149
#define NUMSTR_HEADER_SIZE 4

/* for debuging */
#if 0
#define printf myprintf
extern myprintf();
#endif

#ifdef os_mpw
extern int nWraps;
extern FILE *rdata, *rtype;
#define datafil rdata	/* send statics to resource file */
#else
#define datafil stdout	/* send statics to stdout (with code) */
#endif

/********************/
/* Global Variables */
/********************/

/* Wrap-specific globals */

static char *ctxName;

static TokenList nameTokens;
static int nNames;
static TokenList namedInputArrays, namedInputStrings;
static TokenList literalStrings;
static boolean writable;	/* encoding is not constant */
static boolean twoStatics;	/* true if strings are separate from objects */
static boolean large;
static int dpsHeaderSize;
static int stringBytes;

#ifdef os_mpw
static int nameTokenBytes;	/* num bytes for names */
static int objBytes;
static int fixedBytes;		/* bytes for fixed part of bos -- includes strings if ! twoStatics */
static int thisOffset;		/* offset in resource of this wrap */

int rOffset = 0;		/* cumulative offset in resource of wrap data */
#endif


/**************************/
/* Procedure Declarations */
/**************************/


/**********************/
/* Utility procedures */


static void CantHappen() {
  int *p;
  fprintf(stderr, "CantHappen");
  /* now dump core with the following gross and disgusting kludge */
  p = (int *) (-1);
  *p = 0;
  }
  
#define Assert(b)  if (!(b)) CantHappen(); else ;

#define SafeStrCpy(dst,src) \
	dst = psw_malloc(strlen(src)+1) , \
	strcpy(dst, src)

static int NumArgs(args) Args args; {
  register int n = 0;
  register Arg arg;
  register Item item;
  for (arg = args; arg; arg = arg->next)
    for (item = arg->items; item; item = item->next)
      n++;
  return n;
  }

static int NumTokens(body) register Body body; {
  register int n = 0;
  while (body) { n++; body = body->next; }
  return n;
  }

static TokenList ConsToken (t, ll) Token t; TokenList ll; {
  TokenList tt = (TokenList) psw_calloc(sizeof(TokenListRec), 1);
  tt->token = t;
  tt->next = ll;
  return tt;
  }
  
static TokenList ConsNameToken (t, ll) Token t; TokenList ll; {
  TokenList temp, tt = (TokenList) psw_calloc(sizeof(TokenListRec), 1);
  
  tt->token = t;
  tt->next = ll;
  if(ll == NULL)
  	return (tt);
  temp = ll;
  while((temp->next != NULL) && strcmp((char *)(temp->token->val), (char *)(t->val)))
  	temp = temp->next;
  tt->next = temp->next;
  temp->next = tt;
  return (ll);
  }
  
static boolean IsCharType(t) Type t; {
  return (t == T_CHAR || t == T_UCHAR);
  }
  
static boolean IsNumStrType(t) Type t; {
	return (t == T_NUMSTR
			|| t == T_FLOATNUMSTR
			|| t == T_LONGNUMSTR
			|| t == T_SHORTNUMSTR);
}

static boolean IsPadNumStrType(t) Type t; {
	return (t == T_NUMSTR || t == T_SHORTNUMSTR);
}
  
/*************************/
/* Type-code conversions */

static char *TypeToText(type) Type type; {
  switch ((int) type) {
    case T_CONTEXT:
      return "DPSContext";
    case T_BOOLEAN:
      return "int";
    case T_FLOAT:
      return "float";
    case T_DOUBLE:
      return "double";
    case T_CHAR:
      return "char";
    case T_UCHAR:
      return "unsigned char";
    case T_USEROBJECT:
    case T_INT:
      return "int";
    case T_LONGINT:
      return "long int";
    case T_SHORTINT:
      return "short int";
    case T_ULONGINT:
      return "unsigned long int";
    case T_USHORTINT:
      return "unsigned short int";
    case T_UINT:
      return "unsigned int";
    case T_NUMSTR:
	  return "int";
    case T_FLOATNUMSTR:
	  return "float";
    case T_LONGNUMSTR:
	  return "long int";
    case T_SHORTNUMSTR:
	  return "short int";
    default:
      CantHappen();
    }
    /*NOTREACHED*/
  }

static char *CTypeToDPSType(type) int type; {
  switch (type) {
    case T_BOOLEAN:
      return("DPS_BOOL");
    case T_INT:
    case T_LONGINT:
    case T_SHORTINT:
    case T_UINT:
    case T_ULONGINT:
    case T_USHORTINT:
    case T_USEROBJECT:
      return("DPS_INT");
    case T_FLOAT:
    case T_DOUBLE:
      return("DPS_REAL");
    case T_CHAR:
    case T_UCHAR:
      return("DPS_STRING");
    default: CantHappen();
    }
    /*NOTREACHED*/
  }

static char *CTypeToResultType(type) int type; {
  switch (type) {
    case T_BOOLEAN:
      return("dps_tBoolean");
    case T_USEROBJECT:
    case T_INT:
      return("dps_tInt");
    case T_LONGINT:
      return("dps_tLong");
    case T_SHORTINT:
      return("dps_tShort");
    case T_UINT:
      return("dps_tUInt");
    case T_ULONGINT:
      return("dps_tULong");
    case T_USHORTINT:
      return("dps_tUShort");
    case T_FLOAT:
      return("dps_tFloat");
    case T_DOUBLE:
      return("dps_tDouble");
    case T_CHAR:
      return("dps_tChar");
    case T_UCHAR:
      return("dps_tUChar");
    default: CantHappen();
    }
    /*NOTREACHED*/
  }

static void FreeTokenList(tokenList)
  register TokenList tokenList;
{
  extern int bigFile;
  register TokenList tl;
  if (bigFile)
    while (tokenList) {
      tl = tokenList->next;
      free((char *)tokenList);
      tokenList = tl;
    }
}


/********************************************/
/* Support procedures that generate no code */

static void SetNameTag(t) Token t; {
  PSWDictValue tag;
  Assert(t->type == T_NAME || t->type == T_LITNAME);
  tag = PSWDictLookup(wellKnownPSNames, (char *)(t->val));
  if (tag == -1) { /* this is not a well-known name */
    nameTokens = ConsNameToken(t, nameTokens);
    nNames++;
#ifdef os_mpw
    nameTokenBytes += strlen((char *)t->val) + 1;
#endif
    }
  else { /* a well-known (system) name */
    t->wellKnownName = true;
    t->body.cnst = tag;
    }
  }

static Body InsertArray(body) Body body; {
  Token token;
  
  if (body == NULL) return NULL;
  token = PSWToken(T_PROC,body);
  token->next = PSWToken(T_NAME,"exec");	/* exec */
  token->next->next = (Token) NULL;
  return((Body)token);
}

/* If the wrap has result parameters, DPSAwaitReturnValues
   must be told when execution if the body is complete. The 
   following boilerplate is tacked on to the end of the body
   for this purpose by AppendResultFlush:
   	0 doneTag printobject flush
   where doneTag = (last result parameter tag + 1).
*/
static Body AppendResultFlush(body, n) Body body; int n; {
  Token t, token;
  char *ss;

  if (body == NULL) return NULL;
  for (t = body; t->next; t = t->next) ;

  token = PSWToken(T_INT, 0);
  token->next = PSWToken(T_INT, n);

  SafeStrCpy(ss, "printobject");
  token->next->next = PSWToken(T_NAME, ss);

  SafeStrCpy(ss, "flush");
  token->next->next->next = PSWToken(T_NAME, ss);

  t->next = token;
  return body;
  }


/*****************************************/
/* Support procedures that generate code */

static void EmitArgPrototypes(stm, hdr) FILE *stm; Header hdr; {
  register Arg arg;
  register Item item;
  for (arg = hdr->inArgs; arg; arg = arg->next) {
    fprintf(stm, "%s ", TypeToText(arg->type));
    for (item = arg->items; item; item = item->next) {
      if (item->starred) fprintf(stm, "*");
      fprintf(stm, item->name);
      if (item->subscripted) fprintf(stm, "[]");
      if (item->next) fprintf(stm, ", ");
      }
    fprintf(stm, "; ");
    }
  for (arg = hdr->outArgs; arg; arg = arg->next) {
    fprintf(stm, "%s ", TypeToText(arg->type));
    for (item = arg->items; item; item = item->next) {
      if (item->starred) fprintf(stm, "*");
      fprintf(stm, item->name);
      if (item->subscripted) fprintf(stm, "[]");
      if (item->next) fprintf(stm, ", ");
      }
    fprintf(stm, "; ");
    }
  }

static void EmitANSIPrototypes(stm, hdr) FILE *stm; Header hdr; {
  register Arg arg;
  register Item item;
  register char *type;

  if ((hdr->inArgs == NULL) && (hdr->outArgs == NULL)) {
      fprintf(stm, " void "); return;
  }
  for (arg = hdr->inArgs; arg; arg = arg->next) {
    type = TypeToText(arg->type);
    for (item = arg->items; item; item = item->next) {
      if (arg->type == T_CONTEXT) ctxName = item->name;
      fprintf(stm, "%s %s%s", type, item->starred ? "*" : "", item->name);
      if (item->subscripted) fprintf(stm, "[]");
      if (item->next) fprintf(stm, ", ");
      }
    if (arg->next) fprintf(stm, ", ");
    }
  if (hdr->inArgs && hdr->outArgs) fprintf(stm, ", ");
  for (arg = hdr->outArgs; arg; arg = arg->next) {
    type = TypeToText(arg->type);
    for (item = arg->items; item; item = item->next) {
      fprintf(stm, "%s %s%s", type, item->starred ? "*" : "", item->name);
      if (item->subscripted) fprintf(stm, "[]");
      if (item->next) fprintf(stm, ", ");
      }
    if (arg->next) fprintf(stm, ", ");
    }
  }

/* Procedures for generating type declarations in the body */

static void StartBinObjSeqDef() 
{
  /* start type defn of binobjseq */
#ifdef os_mpw
  extern char	*currentPSWName;
  fprintf(rtype,"\n  /* wrap:  %s */\n",currentPSWName);
  fprintf(rtype,"\tunsigned byte;\t\t/* tokenType */\n");
  if(large) {
  	fprintf(rtype,"\tunsigned byte;\t\t/* sizeFlag */\n");
  	fprintf(rtype,"\tunsigned integer;\t/* topLevelCount */\n");
  	fprintf(rtype,"\tunsigned longint;\t/* nBytes */\n");
	outlineno++;
  } else {
  	fprintf(rtype,"\tunsigned byte;\t\t/* topLevelCount */\n");
  	fprintf(rtype,"\tunsigned integer;\t/* nBytes */\n");
  }
#else
  printf("  typedef struct {\n");
  printf("    unsigned char tokenType;\n");
  if(large) {
  	printf("    unsigned char sizeFlag;\n");
  	printf("    unsigned short topLevelCount;\n");
  	printf("    unsigned long nBytes;\n\n");
	outlineno++;
  } else {
  	printf("    unsigned char topLevelCount;\n");
  	printf("    unsigned short nBytes;\n\n");
  }
  outlineno += 5;
#endif
}

#ifdef os_mpw
static void EmitObjectFormat(iObj)
    int iObj;
{
	fprintf(rtype,"\tarray [%d] {\n",iObj);
	fprintf(rtype,"\t\twide array [1] {\n");
	fprintf(rtype,"\t\t\tunsigned bitstring[1];\t/* literal/exec */\n");
	fprintf(rtype,"\t\t\tunsigned bitstring[7];\t/* attributedType */\n");
	fprintf(rtype,"\t\t\tunsigned byte;\t\t\t/* tag */\n");
	fprintf(rtype,"\t\t\tinteger;\t\t\t\t/* length */\n");
	fprintf(rtype,"\t\t\tlongint;\t\t\t\t/* value */\n");
	fprintf(rtype,"\t\t};\n\t};\n");
} /* EmitObjectFormat */
#endif
  
#ifndef os_mpw
static void EmitFieldType(t) Token t; {
  if ((t->type == T_FLOAT)
  || (t->type == T_NAME && t->namedFormal
     && (!t->namedFormal->subscripted)
     && (t->namedFormal->type == T_FLOAT || t->namedFormal->type == T_DOUBLE))
  || ((t->type == T_SUBSCRIPTED) && ((t->namedFormal->type == T_FLOAT)
                                  || (t->namedFormal->type == T_DOUBLE))))
  {
    printf("    DPSBinObjReal");
  } else {
    printf("    DPSBinObjGeneric");
    }
  printf (" obj%d;\n", t->tokenIndex); outlineno++;
  }
#endif

static int CheckSize(body)
  Body body;
{
  long int objN = 0;
  Adr nextAdr;
  register TokenList bodies = NULL;
  register TokenList tl;
  boolean firstBody = true;
 
  bodies = ConsToken(body, (TokenList) NULL); /* the work list */

  nextAdr.cnst = 0;
  nextAdr.var = NULL;
  namedInputArrays = NULL;
  namedInputStrings = NULL;
  literalStrings = NULL;

  while (bodies) {
    register Token t;
    register TokenList c = bodies;
    bodies = c->next;

    if (firstBody) firstBody = false;
    else {
      c->token->body = nextAdr;
      c->token = (Body)c->token->val;
      }
    for (t = c->token; t; t = t->next) {
      /* foreach token in this body */
      nextAdr.cnst += DPS_BINOBJ_SIZE;


      switch (t->type) {
        case T_STRING: /* token is a string literal */
        case T_HEXSTRING: /* token is a hexstring literal */
          if (t->namedFormal == NULL) {
	    if ((t->type == T_STRING) ? PSWStringLength(t->val)
				      : PSWHexStringLength(t->val))
		literalStrings = ConsToken(t, literalStrings);
            }
          else {
            Assert(IsCharType(t->namedFormal->type));
            namedInputStrings = ConsToken(t, namedInputStrings);
            }
          break;

        case T_NAME:
          if (t->namedFormal != NULL) {
	    	if (IsCharType(t->namedFormal->type) || IsNumStrType(t->namedFormal->type))
				namedInputStrings = ConsToken(t, namedInputStrings);
            else
	      		if (t->namedFormal->subscripted)
                	namedInputArrays = ConsToken(t, namedInputArrays);
		  }
          break;

        case T_LITNAME:
          if (t->namedFormal != NULL) {
	    namedInputStrings = ConsToken(t, namedInputStrings);
	    writable = true;
	  }
          break;
				
		case T_SUBSCRIPTED:
		case T_FLOAT:
        case T_INT:
        case T_BOOLEAN:
          break;

        case T_ARRAY:
          bodies = ConsToken(t, bodies);
          break;
        case T_PROC:
          bodies = ConsToken(t, bodies);
          break;
        default:
          CantHappen();
        } /* switch */
      } /* for */
    free(c);
    } /* while */


  for (tl = namedInputArrays; tl; tl = tl->next) {
    Token t = tl->token;
    if (t->namedFormal->subscript->constant)
	nextAdr.cnst += t->namedFormal->subscript->val * DPS_BINOBJ_SIZE;
    }

  for (tl = literalStrings; tl; tl = tl->next) {
    Token t = tl->token;
    int ln;
    if (t->type == T_STRING) 
    	ln = PSWStringLength((char *)t->val);
    else 
    	ln = PSWHexStringLength((char *)t->val);
    nextAdr.cnst += ln;
  }

  /* process name and litname tokens that reference formal string arguments */
  for (tl = namedInputStrings; tl; tl = tl->next) {
    Token t = tl->token;
    if (t->namedFormal->subscripted && t->namedFormal->subscript->constant) {
    	if (IsNumStrType(t->namedFormal->type)) 
			nextAdr.cnst += NUMSTR_HEADER_SIZE;
		else
			nextAdr.cnst += t->namedFormal->subscript->val;
		if(pad) {
			nextAdr.cnst += WORD_ALIGN;
			nextAdr.cnst &= ~WORD_ALIGN;
		}
  	}
  }
  
  if (nextAdr.cnst > 0xffff)
  	return(1);
  else
  	return(0);
	
} /* CheckSize */
  
static void BuildTypesAndAssignAddresses(body, sz, nObjs, psize)
  Body body; Adr *sz; long int *nObjs; unsigned *psize;
{
  long int objN = 0;
  Adr nextAdr;
  register TokenList bodies = NULL;
  register TokenList tl;
  boolean firstBody = true;
  extern int yylineno;
 
  bodies = ConsToken(body, (TokenList) NULL); /* the work list */

  nextAdr.cnst = 0;
  nextAdr.var = NULL;
  namedInputArrays = NULL;
  namedInputStrings = NULL;
  literalStrings = NULL;
  writable = false;
  stringBytes = 0;

  /* emit boilerplate for the binobjseq record type */
  StartBinObjSeqDef();

  while (bodies) {
    register Token t;
    register TokenList c = bodies;
    bodies = c->next;

    if (firstBody) firstBody = false;
    else {
      c->token->body = nextAdr;
      c->token = (Body)c->token->val;
      }
    for (t = c->token; t; t = t->next) {
      /* foreach token in this body */
      t->adr = nextAdr;
      nextAdr.cnst += DPS_BINOBJ_SIZE;
      t->tokenIndex = objN++;

#ifndef os_mpw
      /* emit the token type as the next record field */
      EmitFieldType(t);
#endif

      switch (t->type) {
        case T_STRING: /* token is a string literal */
        case T_HEXSTRING: /* token is a hexstring literal */
          if (t->namedFormal == NULL) {
	    if ((t->type == T_STRING) ? PSWStringLength(t->val)
				      : PSWHexStringLength(t->val))
		literalStrings = ConsToken(t, literalStrings);
            }
          else {
            Assert(IsCharType(t->namedFormal->type));
            namedInputStrings = ConsToken(t, namedInputStrings);
            if (!(t->namedFormal->subscripted && t->namedFormal->subscript->constant))
            	writable = true;
            }
          break;

        case T_NAME:
          if (t->namedFormal == NULL) SetNameTag(t);
          else 
			if (IsCharType(t->namedFormal->type) 
				|| IsNumStrType(t->namedFormal->type)) {
            	namedInputStrings = ConsToken(t, namedInputStrings);
            	if (!(t->namedFormal->subscripted 
					&& t->namedFormal->subscript->constant))
            		writable = true;
            } else 
	    		if (t->namedFormal->subscripted) {
            		namedInputArrays = ConsToken(t, namedInputArrays);
            		if (!(t->namedFormal->subscript->constant))
            			writable = true;
            	} else
	         		writable = true;
          break;

        case T_LITNAME:
          Assert(t->namedFormal == NULL || IsCharType(t->namedFormal->type));
          if (t->namedFormal == NULL) SetNameTag(t);
          else {
	    namedInputStrings = ConsToken(t, namedInputStrings);
	    writable = true;
	  }
          break;

	case T_SUBSCRIPTED:
	  writable = true;
	  break;
        case T_FLOAT:
        case T_INT:
        case T_BOOLEAN:
          break;

        case T_ARRAY:
          bodies = ConsToken(t, bodies);
          break;
        case T_PROC:
          bodies = ConsToken(t, bodies);
          break;
        default:
          CantHappen();
        } /* switch */
      } /* for */
    free(c);
    } /* while */
    
*psize = nextAdr.cnst;
    
if(nNames)
	writable = true; 	/* SetNameTag couldn't find the name */	
  
#ifdef os_mpw
  EmitObjectFormat(objN);
#endif
  if (namedInputArrays && literalStrings) {
    twoStatics = true;
#ifndef os_mpw
    printf("    } _dpsQ;\n\n");
    printf("  typedef struct {\n");
    outlineno += 3;
#endif
    }
  else twoStatics = false;

#ifdef os_mpw
  thisOffset = rOffset;
  objBytes = nextAdr.cnst;
  rOffset += nextAdr.cnst;
#endif

  for (tl = namedInputArrays; tl; tl = tl->next) {
    Token t = tl->token;
    Assert(t && t->type == T_NAME && t->namedFormal);
    Assert(t->namedFormal->subscripted && !t->namedFormal->starred);

    /* this input array token requires its own write binobjs call */
    t->body = nextAdr;
    if (t->namedFormal->subscript->constant)
	nextAdr.cnst += t->namedFormal->subscript->val * DPS_BINOBJ_SIZE;
    }
  
  for (tl = literalStrings; tl; tl = tl->next) {
    Token t = tl->token;
    int ln;
    t->body = nextAdr;
    if (t->type == T_STRING) 
    	ln = PSWStringLength((char *)t->val);
    else 
    	ln = PSWHexStringLength((char *)t->val);
    nextAdr.cnst += ln;
    stringBytes += ln;

    /* emit the string type as the next record field */
#ifdef os_mpw
    fprintf(rtype, "\tstring [%d]; /* obj %d */\n", ln, objN++);
    rOffset += ln;
#else
    printf("    char obj%d[%d];\n", objN++, ln); outlineno++;
#endif
    }

  /* process name and litname tokens that reference formal string arguments */
  for (tl = namedInputStrings; tl; tl = tl->next) {
    Token t = tl->token;
    t->body = nextAdr;
    if (t->namedFormal->subscripted && t->namedFormal->subscript->constant) {
    	if (IsNumStrType(t->namedFormal->type)) {
			nextAdr.cnst += NUMSTR_HEADER_SIZE;
			writable = true;
		} else
			nextAdr.cnst += t->namedFormal->subscript->val;
	  	if(pad) {
		  nextAdr.cnst += WORD_ALIGN;
		  nextAdr.cnst &= ~WORD_ALIGN;
		}
	}
  }

#ifndef os_mpw
  /* emit boilerplate to end the last record type */
  if (twoStatics) printf("    } _dpsQ1;\n");
  else printf("    } _dpsQ;\n");
  outlineno++;
#endif

#ifdef os_mpw
  fixedBytes = objBytes + dpsHeaderSize;
  if (! twoStatics)
    fixedBytes += stringBytes;
#endif

  *nObjs = objN;
    /* total number of objects plus string bodies in objrecs */

  *sz = nextAdr;
  } /* BuildTypesAndAssignAddresses */
  

/* Procedures for generating static declarations for local types */

static void StartStatic(first) boolean first; {
  /* start static def for bin obj seq or for array data (aux) */
#ifdef os_mpw
  if (first)
    if(reentrant && writable)
      printf("  char _dpsF[%d];\n", fixedBytes);
    else
      printf("  char *_dpsF;\n");
  else
    printf("  char *_dpsF1;\n");
#else
  if (first) {
    if(reentrant && writable) {
      if(doANSI)
       	printf("  static const _dpsQ _dpsStat = {\n");
      else
		printf("  static _dpsQ _dpsStat = {\n");
    } else {
      if (doANSI)
		printf("  static const _dpsQ _dpsF = {\n");
      else
		printf("  static _dpsQ _dpsF = {\n");
    } 
  } else {
    	if(doANSI)
      		printf("  static const _dpsQ1 _dpsF1 = {\n");
    	else
      		printf("  static _dpsQ1 _dpsF1 = {\n");
  }
#endif
  
  outlineno++;
}

static void FirstStatic(nTopObjects, sz)
  int nTopObjects; Adr *sz; {
  char *numFormat = "DPS_DEF_TOKENTYPE";

#ifdef os_mpw
  rOffset += dpsHeaderSize;
  fprintf(datafil,"\n  /* WRAP: %s */\n",currentPSWName);
#else
  outlineno++;
#endif
  if(large) {
	fprintf(datafil, "    %s, 0, %d, ", numFormat, nTopObjects);
	fprintf(datafil, "%d,\n", sz->cnst + dpsHeaderSize);
  } else {
	fprintf(datafil, "    %s, %d, ", numFormat, nTopObjects);
	fprintf(datafil, "%d,\n", sz->cnst + dpsHeaderSize);
  }
}

#ifndef os_mpw
static void EndStatic(first) boolean first; {
  /* end static template defn */
  if (first)
    printf("    }; /* _dpsQ */\n");
  else
    printf("    }; /* _dpsQ1 */\n");
  outlineno++;
  }
#endif

/* char that separates object attributes */
#ifdef os_mpw
#define ATT_SEP ','
#else
#define ATT_SEP '|'
#endif

static void EmitFieldConstructor(t) register Token t; {
    char *comment = NULL, *commentName = NULL;
    fprintf(datafil, "    {");

    switch (t->type) {
      case T_BOOLEAN:
        fprintf(datafil, "DPS_LITERAL%cDPS_BOOL, 0, 0, %d", ATT_SEP, t->val);
        break;
      case T_INT:
        fprintf(datafil, "DPS_LITERAL%cDPS_INT, 0, 0, %d", ATT_SEP, t->val);
        break;
      case T_FLOAT:
#ifdef os_mpw
	    {	float ff; int fl; extended atof();
		fprintf(datafil,"DPS_LITERAL%cDPS_REAL, 0, 0, /* %s */", ATT_SEP, (char *)t->val);
		ff = (float) atof((char *)t->val);
		fl = *((int *) ((float *) &ff));
		fprintf(datafil," $%lx",fl);
	    }
#else
        fprintf(datafil, "DPS_LITERAL%cDPS_REAL, 0, 0, %s", ATT_SEP, (char *)t->val);
#endif
        break;

      case T_ARRAY:
        fprintf(datafil, "DPS_LITERAL%cDPS_ARRAY, 0, %d, %d", ATT_SEP,
	  NumTokens((Body) (t->val)), t->body.cnst);
        break;
      case T_PROC:
        fprintf(datafil, "DPS_EXEC%cDPS_ARRAY, 0, %d, %d", ATT_SEP,
	  NumTokens((Body) (t->val)), t->body.cnst);
        break;

      case T_STRING:
      case T_HEXSTRING:
        if (t->namedFormal == NULL) {
          int ln;
          if (t->type == T_STRING)
            ln = PSWStringLength((char *)t->val);
          else ln = PSWHexStringLength((char *)t->val);
          fprintf(datafil, "DPS_LITERAL%cDPS_STRING, 0, %d, %d", ATT_SEP,
	    ln, t->body.cnst);
        } else {
          Item item = t->namedFormal;
	  if (item->subscripted && item->subscript->constant) {  
            fprintf(datafil, "DPS_LITERAL%cDPS_STRING, 0, %d, %d",
	    		    ATT_SEP,item->subscript->val, t->body.cnst);
            comment = "param[const]: ";
	  } else {
            fprintf(datafil, "DPS_LITERAL%cDPS_STRING, 0, 0, %d",
	    						ATT_SEP,t->body.cnst);
            comment = "param ";
	  }
	  commentName = (char *)t->val;
        }
        break;

      case T_LITNAME:
        commentName = (char *)t->val;
        if (t->wellKnownName) {
          fprintf(datafil, "DPS_LITERAL%cDPS_NAME, 0, DPSSYSNAME, %d", ATT_SEP, t->body.cnst);
          }
        else if (t->namedFormal == NULL) {
          fprintf(datafil, "DPS_LITERAL%cDPS_NAME, 0, 0, 0", ATT_SEP);
          }
        else {
          fprintf(datafil, "DPS_LITERAL%cDPS_NAME, 0, 0, %d", ATT_SEP, t->body.cnst);
          comment = "param ";
          }
        break;

      case T_NAME:
        commentName = (char *)t->val;
        if (t->wellKnownName) {
          fprintf(datafil, "DPS_EXEC%cDPS_NAME, 0, DPSSYSNAME, %d", ATT_SEP, t->body.cnst);
          }
        else if (t->namedFormal == NULL) {
          fprintf(datafil, "DPS_EXEC%cDPS_NAME, 0, 0, 0", ATT_SEP);
          }
        else {
          Item item = t->namedFormal;
          if (IsCharType(item->type)) {
            if (item->subscripted && t->namedFormal->subscript->constant) {
              fprintf(datafil, "DPS_EXEC%cDPS_NAME, 0, %d, %d", ATT_SEP,
		t->namedFormal->subscript->val, t->body.cnst);
              comment = "param[const]: ";
              }
	    else {
              fprintf(datafil, "DPS_EXEC%cDPS_NAME, 0, 0, %d", ATT_SEP, t->body.cnst);
              comment = "param ";
              }
            }
          else {
            if (item->subscripted) {
              if (t->namedFormal->subscript->constant) {
	      		if(IsNumStrType(item->type))
                	fprintf(datafil, "DPS_LITERAL%cDPS_STRING, 0, %d, %d", ATT_SEP,
		  				t->namedFormal->subscript->val + NUMSTR_HEADER_SIZE,
						t->body.cnst);
				else 
                	fprintf(datafil, "DPS_LITERAL%cDPS_ARRAY, 0, %d, %d", ATT_SEP,
		  				t->namedFormal->subscript->val, t->body.cnst);
                comment = "param[const]: ";
              } else {
	      		if(IsNumStrType(item->type))
                	fprintf(datafil, "DPS_LITERAL%cDPS_STRING, 0, 0, %d", ATT_SEP,
						t->body.cnst);
				else 
                	fprintf(datafil, "DPS_LITERAL%cDPS_ARRAY, 0, 0, %d", ATT_SEP,
						t->body.cnst);
                comment = "param[var]: ";
                }
              }
            else {
              char *dt = CTypeToDPSType(item->type);
              fprintf(datafil, "DPS_LITERAL%c%s, 0, 0, 0", ATT_SEP, dt);
              comment = "param: ";
              }
            }
          }
        break;
      case T_SUBSCRIPTED: {
	  Item item = t->namedFormal;
	  char *dt = CTypeToDPSType(item->type);

          /* Assert(t->namedFormal) */
	  fprintf(datafil, "DPS_LITERAL%c%s, 0, 0, 0", ATT_SEP, dt);
	  comment = "indexed param: ";
	  commentName = (char *)t->val;
        }
        break;

      default:
        CantHappen();
      } /* switch */

    if (comment == NULL) {
      if (commentName == NULL) fprintf(datafil, "},\n");
      else fprintf(datafil, "},	/* %s */\n", commentName);
    }
    else {
      if (commentName == NULL) fprintf(datafil, "},	/* %s */\n", comment);
      else fprintf(datafil, "},	/* %s%s */\n", comment, commentName);
    }
#ifndef os_mpw
    outlineno++;
#endif
  } /* EmitFieldConstructor */

static void ConstructStatics(body, sz, nObjs)
  Body body; Adr *sz; int nObjs; {
  int objN = 0;
  register TokenList strings = NULL, bodies = NULL;
  register TokenList tl;
  boolean isNamedInputArrays = false, isNumStr = false;

  bodies = ConsToken(body, (TokenList) NULL); /* the work list */

  /* emit boilerplate for the binobjseq static */
  StartStatic(true);
  FirstStatic(NumTokens(body), sz);

#ifdef os_mpw
  fprintf(rdata,"  { /* bodies */\n");
#endif

  while (bodies) {
    register Token t;
    TokenList c = bodies;
    bodies = c->next;

    for (t = c->token; t; t = t->next) {
      /* foreach token in this body */

      /* emit the next record field constructor */
      EmitFieldConstructor(t);
#ifdef os_mpw
      SpinCursor(1);
#endif
      objN++;

      switch (t->type) {
        case T_STRING: /* token is a string literal */
	  		if ((t->namedFormal == NULL) && PSWStringLength(t->val))
            	strings = ConsToken(t, strings);
	  		break;

        case T_HEXSTRING: /* token is a hexstring literal */
          	if ((t->namedFormal == NULL) && PSWHexStringLength(t->val))
            	strings = ConsToken(t, strings);
          	break;

        case T_NAME:
          if ((t->namedFormal)
              && (t->namedFormal->subscripted)
              && (!IsCharType(t->namedFormal->type))
             )
		   		if(IsNumStrType(t->namedFormal->type))
					isNumStr = true;
				else
					isNamedInputArrays = true;
           break;

        case T_LITNAME:
        case T_FLOAT:
        case T_INT:
        case T_BOOLEAN:
		case T_SUBSCRIPTED:
          break;

        case T_ARRAY:
        case T_PROC:
          bodies = ConsToken((Body)t->val, bodies);
          break;
        default:
          CantHappen();
        } /* switch */
      } /* for */
    free(c);
    } /* while */

#ifdef os_mpw
  fprintf(rdata,"  },\n");
#endif

  if (strings && isNamedInputArrays) {
#ifndef os_mpw
    EndStatic(true);
#endif
    StartStatic(false);
    }

  for (tl = strings; tl; tl = tl->next) {
    Token t = tl->token;
#ifndef os_mpw
    printf("    {");
#endif
    if (t->type == T_STRING)
    	PSWOutputStringChars((char *)t->val);
    else 
    	PSWOutputHexStringChars((char *)t->val);
#ifndef os_mpw
    printf("},\n"); outlineno++;
#endif
    objN++;
    }

  FreeTokenList(strings); strings = NULL;

#ifndef os_mpw
  EndStatic(! twoStatics); /* end the last static record */
#endif
  if(isNumStr) {
  	printf("  unsigned char HNumHeader[%d];\n", NUMSTR_HEADER_SIZE);
	outlineno ++;
  }

  Assert(objN  == nObjs);

  } /* ConstructStatics */
  

/* Procedures for managing the result table */

#ifdef os_mpw
static void EmitResultTagTableDecls(outArgs) Args outArgs; {
  register Arg arg;
  register Item item;
  int count = 0, na;

  na = NumArgs(outArgs);
  printf("  DPSResultsRec _dpsR[%d];\n",na); outlineno++;

  for (arg = outArgs; arg; arg = arg->next) {
    for (item = arg->items; item; item = item->next) {
      printf("    _dpsR[%d].type = %s;\n",count,CTypeToResultType(item->type));
      outlineno++;
      if (item->subscripted) {
        Subscript s = item->subscript;
		if (s->constant) {
		  printf("    _dpsR[%d].count = %d;\n",count,s->val);
		} else {
		  printf("    _dpsR[%d].count = %s;\n",count,s->name);
		}
      } else { /* not subscripted */
		printf("    _dpsR[%d].count = -1;\n",count);
      }
      outlineno++;
      printf("    _dpsR[%d].value = (char *)%s;\n",count++,item->name);
      outlineno++;
      }
    }
  printf("\n"); outlineno++;
  }
#else
static void EmitResultTagTableDecls(outArgs) Args outArgs; {
  register Arg arg;
  register Item item;
  int count = 0;
 
  if(reentrant) {
  	for (arg = outArgs; arg; arg = arg->next)
    	   for (item = arg->items; item; item = item->next)
			count++;
  	printf("  DPSResultsRec _dpsR[%d];\n", count); outlineno++;
	count = 0;
	if(doANSI)
  		printf("  static const DPSResultsRec _dpsRstat[] = {\n");
	else
  		printf("  static DPSResultsRec _dpsRstat[] = {\n");
	outlineno++;
  } else {
  	printf("  static DPSResultsRec _dpsR[] = {\n"); outlineno++;
  }
  for (arg = outArgs; arg; arg = arg->next) {
    for (item = arg->items; item; item = item->next) {
      if (item->subscripted) {
        Subscript s = item->subscript;
        printf("    { %s },\n",CTypeToResultType(item->type));
      }
      else { /* not subscripted */
        printf("    { %s, -1 },\n",CTypeToResultType(item->type));
      }
      outlineno++;
    }
  }
  printf("    };\n"); outlineno++;
  for (arg = outArgs; arg; arg = arg->next) {
    for (item = arg->items; item; item = item->next) {
      if(reentrant) {
  		printf("    _dpsR[%d] = _dpsRstat[%d];\n", count, count);
		outlineno++;
      }
      if (item->subscripted) {
        Subscript s = item->subscript;
        if (!(s->constant)) {
			printf("    _dpsR[%d].count = %s;\n",count, s->name);
		} else {
			printf("    _dpsR[%d].count = %d;\n",count, s->val);
		}
		outlineno++;
    } else { /* not subscripted */
		if (IsCharType(item->type)) {
			printf("    _dpsR[%d].count = -1;\n",count);
			outlineno++;
		}
    }
    printf("    _dpsR[%d].value = (char *)%s;\n",count++,item->name);
    outlineno++;
    }
  }
  printf("\n"); outlineno++;
}
#endif

static void EmitResultTagTableAssignments(outArgs) Args outArgs; {
  printf("  DPSSetResultTable(%s, _dpsR, %d);\n",ctxName,NumArgs(outArgs));
  outlineno++;
  }

/* Procedure for acquiring name tags */

#ifdef os_mpw
static void EmitNameTagAcquisition(start) int start; {
    register TokenList n;
    int i; int nOffset; int nameOffset;
    char *last_str;

    printf("    {\n");
    printf("    char *_dps_names[%d]; int *_dps_nameVals[%d];\n",nNames,nNames);
    printf("    register char *_dpsF = wrap_gp->wb[%d];\n", nWraps);
    printf("    %s_dpsP = _dpsF + %d;\n",
      writable ? "" : "register DPSBinObjRec *", dpsHeaderSize);
    outlineno += 4;

    last_str = psw_malloc(maxstring+1);
    i = 0;
    nameOffset = start;
    fprintf(rdata,"\t/* user names: %d */\n",nameOffset);
    for (n = nameTokens; n!= NULL; n = n->next) {
      if (strcmp(last_str, (char *) n->token->val)) {
		strcpy(last_str, (char *) n->token->val);
		printf("    _dps_names[%d] = _dpsF + %d;\n", i, nameOffset);
		fprintf(rtype,"\tcstring;	/* %s */\n", last_str);
		fprintf(rdata,"\t\"%s\",\n", last_str);
		nameOffset += strlen(last_str) + 1;
      } else {
		printf("    _dps_names[%d] = (char *) 0;\n", i);
      	printf("    _dps_nameVals[%d] = (long int *)&_dpsP[%d].val.nameVal;\n",
        i++, n->token->tokenIndex);
      	outlineno += 2;
      }
	}
	rOffset += nameOffset - start;
	nOffset = ((rOffset + 3) >> 2) << 2;
	fprintf(rdata,"\t/* offset is %ld -> %ld */\n", rOffset,nOffset);
	fprintf(rtype,"\n\talign long; /* %ld -> %ld */\n",rOffset,nOffset);
	rOffset = nOffset;

    printf("    DPSMapNames(%s, %d, _dps_names, _dps_nameVals);\n    }\n",ctxName, nNames);
    outlineno += 2;
  }
#else
static void EmitNameTagAcquisition() {
    register TokenList n;
    int i;
    char *last_str;
    
    last_str = (char *) psw_malloc((unsigned) (maxstring+1));

    printf("  {\n");
    if(!doANSI) {
    	printf("  static int _dpsT = 1;\n\n");
    	printf("  if (_dpsT) {\n");
		outlineno += 4;
    } else {
    	printf("if (_dpsCodes[0] < 0) {\n");
		outlineno += 2;
    }
    if(doANSI)
    	printf("    static const char * const _dps_names[] = {\n");
    else
    	printf("    static char *_dps_names[] = {\n");
    outlineno ++;
    
    for (n = nameTokens; n!= NULL; n = n->next) {
      if (strcmp(last_str,(char *)n->token->val)) {
      		strcpy(last_str,(char *)n->token->val);
      		printf("\t\"%s\"", (char *)n->token->val);
      } else {
      		printf("\t(char *) 0 ");
      }
      if (n->next) {printf(",\n"); outlineno++;}
    }
    printf("};\n"); outlineno++;
    printf("    long int *_dps_nameVals[%d];\n",nNames);outlineno++;
    if (!doANSI) {
    	if (!writable) {
      	    printf("    register DPSBinObjRec *_dpsP = (DPSBinObjRec *) &_dpsF.obj0;\n");
      	    outlineno++;
    	} else {
	    if (reentrant) {
      		printf("    _dpsP = (DPSBinObjRec *) &_dpsStat.obj0;\n");
      	  	outlineno++;
    	    }
	}
    }
    i = 0;
    if (doANSI) {
      for(i=0; i<nNames; i++) {
	   printf("    _dps_nameVals[%d] = &_dpsCodes[%d];\n",i,i);
 	   outlineno ++;
	  }
    } else {
        for (n = nameTokens; n!= NULL; n = n->next) {
        	printf("    _dps_nameVals[%d] = (long int *)&_dpsP[%d].val.nameVal;\n",
               i++, n->token->tokenIndex);
           outlineno++;
        }
    }
    printf("\n    DPSMapNames(%s, %d, _dps_names, _dps_nameVals);\n",
           ctxName, nNames);
    outlineno += 2;
    if (reentrant && writable && !doANSI) {
      printf("    _dpsP = (DPSBinObjRec *) &_dpsF.obj0;\n");
      outlineno++;
    }
    if (!doANSI) {
    	printf("    _dpsT = 0;\n");
		outlineno ++;
    }
    printf("    }\n  }\n\n");
    outlineno += 3;
  } /* EmitNameTagAcquisition */
#endif


/* Miscellaneous procedures */

static void EmitLocals(sz)
unsigned sz;
{
#ifndef os_mpw
  if(reentrant && writable) {
	printf("  _dpsQ _dpsF;	/* local copy  */\n");
   	outlineno++;
  }
#endif
  if (ctxName == NULL) {
      printf("  register DPSContext _dpsCurCtxt = DPSPrivCurrentContext();\n");
      ctxName = "_dpsCurCtxt";
      outlineno++;
  }
#ifndef NeXT
  if(pad) {
  	printf("  char pad[3];\n");
	outlineno++;
  }
#endif
  if (writable) {
#ifdef os_mpw
    printf("  register DPSBinObjRec *_dpsP;\n");
#else
    printf("  register DPSBinObjRec *_dpsP = (DPSBinObjRec *)&_dpsF.obj0;\n");
    if(doANSI && nNames) {
    	printf("  static long int _dpsCodes[%d] = {-1};\n",nNames);
		outlineno++;
    }
#endif
    outlineno++;
    if (namedInputArrays || namedInputStrings) {
#ifdef os_mpw
      printf("  register int _dps_offset = %d;\n", fixedBytes-dpsHeaderSize);
#else
      printf("  register int _dps_offset = %d;\n", 
      						twoStatics ? sz : sz + stringBytes);
#endif
      outlineno++;
    }
  }
}

static boolean AllLiterals(body) Body body; {
  Token t;

  for (t = body; t; t = t->next) {
    switch (t->type) {

      case T_NAME:
        if (t->namedFormal == NULL) return false;
        break;

      case T_ARRAY:
        if (!AllLiterals((Body)t->val)) return false;
        break;

      case T_PROC:
      case T_FLOAT:
      case T_INT:
      case T_BOOLEAN:
      case T_LITNAME:
      case T_HEXSTRING:
      case T_STRING:
      case T_SUBSCRIPTED: 
        break;

      default:
        CantHappen();
      } /* switch */
    } /* for */
  return true;
  } /* AllLiterals */

static void FlattenSomeArrays(body, inSquiggles)
  Body body; boolean inSquiggles; {
  Token t;
  for (t = body; t; t = t->next) {
    switch (t->type) {

      case T_ARRAY:
        if (!AllLiterals((Body)t->val)) {
		  Token t1, b, tlsq, trsq;
		  char *s;
          t1 = t->next;
          b = (Body)t->val;
		  SafeStrCpy(s, "[");
          tlsq = PSWToken(T_NAME, s);
		  SafeStrCpy(s, "]");
          trsq = PSWToken(T_NAME, s);
          tlsq->sourceLine = t->sourceLine;
          trsq->sourceLine = t->sourceLine;
          *t = *tlsq;
          t->next = b;
          trsq->next = t1;
          if (b == NULL) t->next = trsq;
          else {
            Token last;
            for (last = b; last->next; last = last->next) ;
            last->next = trsq;
            }
          }
        else FlattenSomeArrays((Body)t->val, inSquiggles);
        break;

      case T_PROC:
        FlattenSomeArrays((Body)t->val, true);
          /* flatten all arrays below here */ 
        break;
      		
      case T_NAME:
      case T_FLOAT:
      case T_INT:
      case T_BOOLEAN:
      case T_LITNAME:
      case T_HEXSTRING:
      case T_STRING:
      case T_SUBSCRIPTED:
      case T_NUMSTR:
      case T_FLOATNUMSTR:
      case T_LONGNUMSTR:
      case T_SHORTNUMSTR:
        break;

      default:
        CantHappen();
      } /* switch */
    } /* for */
  } /* FlattenSomeArrays */


static void FixupOffsets()
{
    register TokenList tl; Token t;
    register Item item;
    int stringOffset = 0;

    for (tl = namedInputArrays; tl; tl = tl->next) {
		t = tl->token; item = t->namedFormal;
		printf("  _dpsP[%d].val.arrayVal = _dps_offset;\n",t->tokenIndex);
		printf("  _dps_offset += ");
		if (item->subscript->constant)
	    	printf("%d * sizeof(DPSBinObjGeneric);\n",item->subscript->val);
		else
	    	printf("%s * sizeof(DPSBinObjGeneric);\n",item->subscript->name);
		outlineno += 2;
    } /* named input arrays */

    for (tl = namedInputStrings; tl; tl = tl->next) {
		t = tl->token; item = t->namedFormal;
		printf("  _dpsP[%d].val.stringVal = _dps_offset;\n",t->tokenIndex);
		printf("  _dps_offset += ");
		if (item->subscripted) {
	    	if (item->subscript->constant) {
				if(IsNumStrType(t->namedFormal->type)) {
					if(pad & IsPadNumStrType(t->namedFormal->type))
						printf("((%d * sizeof(%s)) + %d) & ~%d;\n",
							item->subscript->val,TypeToText(t->namedFormal->type),
							NUMSTR_HEADER_SIZE+WORD_ALIGN, WORD_ALIGN);
					else
						printf("(%d * sizeof(%s)) + %d;\n",
							item->subscript->val,TypeToText(t->namedFormal->type),
							NUMSTR_HEADER_SIZE);
				} else
					if(pad) {
						int val = item->subscript->val;
						val += WORD_ALIGN;
						val &= ~WORD_ALIGN;
						printf("%d;\n", val);
					} else 
						printf("%d;\n",item->subscript->val);
	    	} else {
				if(IsNumStrType(t->namedFormal->type)) {
					if(pad & IsPadNumStrType(t->namedFormal->type))
						printf("((%s * sizeof(%s)) + %d) & ~%d;\n",
							item->subscript->name,TypeToText(t->namedFormal->type),
							NUMSTR_HEADER_SIZE+WORD_ALIGN, WORD_ALIGN);
					else
						printf("(%s * sizeof(%s)) + %d;\n",
							item->subscript->name,TypeToText(t->namedFormal->type),
							NUMSTR_HEADER_SIZE);
				} else
					if(pad)
						printf("(%s + %d) & ~%d;\n",
								item->subscript->name, WORD_ALIGN, WORD_ALIGN);
					else
						printf("%s;\n",item->subscript->name);
		   }
	} else
		if(pad)
			printf("(_dpsP[%d].length + %d) & ~%d;\n",
			       t->tokenIndex, WORD_ALIGN, WORD_ALIGN);
	    else
	        printf("_dpsP[%d].length;\n",t->tokenIndex);
	outlineno += 2;
    } /* named input strings */

    if (namedInputArrays) {
	for (tl = literalStrings; tl; tl = tl->next) {
	    t = tl->token;
	    if (stringOffset == 0)
			printf("  _dpsP[%d].val.stringVal = _dps_offset;\n",
		    	t->tokenIndex);
	    else
			printf("  _dpsP[%d].val.stringVal = _dps_offset + %d;\n",
		    	t->tokenIndex,stringOffset);
	    outlineno++;
	    stringOffset +=
		(t->type == T_STRING)
		    ? PSWStringLength((char *)t->val)
		    : PSWHexStringLength((char *)t->val);
	} /* literalStrings */
	if (stringOffset) {
	    printf("  _dps_offset += %d;\n",stringOffset);
	    outlineno++;
	}
    }
} /* FixupOffsets */


static int EmitValueAssignments(body,item)
Body body; Item item;
{
    register Token t;
    int gotit = 0;

    for (t = body; t; t = t->next) {
	switch (t->type) {
	    case T_STRING:
	    case T_HEXSTRING:
	    case T_LITNAME:
	        if (t->namedFormal && t->namedFormal == item) {
		    	printf("\n  _dpsP[%d].length =",t->tokenIndex);
		    	outlineno++;
		    	gotit++;
			}
		break;
	    case T_NAME:
		if (t->namedFormal && t->namedFormal == item) {
		    if (item->subscripted && !item->subscript->constant ||
		        item->starred && IsCharType(item->type) || 
				IsNumStrType(item->type)) {  
				printf("\n  _dpsP[%d].length =",t->tokenIndex);
				outlineno++;
				gotit++;
		    }
		    switch (item->type) {
			case T_BOOLEAN:
			  if (!item->subscripted) {
			    printf("\n  _dpsP[%d].val.booleanVal =",
				   t->tokenIndex);
			    gotit++; outlineno++;
			    }
			 break;
			case T_INT:
			case T_LONGINT:
			case T_SHORTINT:
			case T_UINT:
			case T_ULONGINT:
			case T_USHORTINT:
			case T_USEROBJECT:
			  if (!item->subscripted) {
			    printf("\n  _dpsP[%d].val.integerVal =",
				   t->tokenIndex);
			    gotit++; outlineno++;
			  }
			  break;
			case T_FLOAT:
			case T_DOUBLE:
			  if (!item->subscripted) {
			    printf("\n  _dpsP[%d].val.realVal =",
				   t->tokenIndex);
			    gotit++; outlineno++;
			    }
			  break;
			case T_CHAR:
			case T_UCHAR: /* the executable name is an arg */
			case T_NUMSTR:
			case T_FLOATNUMSTR:
			case T_LONGNUMSTR:
			case T_SHORTNUMSTR:
			  break;
			default: CantHappen();
			}
		      }
		    break;

	    case T_SUBSCRIPTED:
	    case T_FLOAT:
	    case T_INT:
	    case T_BOOLEAN:
		break;

	    case T_ARRAY:
	    case T_PROC:
	        /* recurse */
	    	gotit += EmitValueAssignments((Body) (t->val),item);
		break;
	    default:
		CantHappen();
	} /* switch */
    } /* token */
    return (gotit);
  } /* EmitValueAssignments */


static void EmitElementValueAssignments(body,item)
Body body; Item item;
{
    register Token t;

    for (t = body; t; t = t->next) {
	if (t->type != T_SUBSCRIPTED) continue;
	if (t->namedFormal == item) {
	    switch (item->type) {
		case T_BOOLEAN:
		  printf("\n  _dpsP[%d].val.booleanVal = (long)(0 != %s[%s]);",
			t->tokenIndex, item->name, t->body.var);
		  outlineno++;
		  break;
		case T_INT:
		case T_LONGINT:
		case T_SHORTINT:
		case T_UINT:
		case T_ULONGINT:
		case T_USHORTINT:
		  printf("\n  _dpsP[%d].val.integerVal = %s[%s];",
			   t->tokenIndex, item->name, t->body.var);
		  outlineno++;
		  break;
		case T_FLOAT:
		case T_DOUBLE:
		  printf("\n  _dpsP[%d].val.realVal = %s[%s];",
			   t->tokenIndex, item->name, t->body.var);
		  outlineno++;
		  break;
		case T_CHAR:
		case T_UCHAR:
		  CantHappen();
		  break;
		default: CantHappen();
	    }
	}
    } /* token */
} /* EmitElementValueAssignments */


static void ScanParamsAndEmitValues(body,args)
Body body; Args args;
{
    register Arg arg;	/* a list of parameters */
    register Item item;	/* a parameter */
    int gotit;	/* flag that we found some token with this length */

    /* for each arg */
    for (arg = args; arg; arg = arg->next) {
	/* for each arg item */
	  for (item = arg->items; item; item = item->next) {
		  if (item->type == T_CONTEXT) continue;
		  gotit = EmitValueAssignments(body,item);
		  if (gotit != 0) {
			if (item->subscripted) {
				if (item->subscript->constant) {
					if(IsNumStrType(item->type))
						printf(" (%d * sizeof(%s)) + %d;",item->subscript->val,
						       TypeToText(item->type), NUMSTR_HEADER_SIZE);
					else
						printf(" %d;",item->subscript->val);
				} else {
					if(IsNumStrType(item->type))
						printf(" (%s * sizeof(%s)) + %d;",item->subscript->name,
						       TypeToText(item->type), NUMSTR_HEADER_SIZE);
					else
						printf(" %s;",item->subscript->name);
				}
			} else switch(item->type) {
				case T_CHAR:
				case T_UCHAR:
					printf(" strlen(%s);",item->name);
				break;
				case T_INT:
				case T_LONGINT:
				case T_SHORTINT:
				case T_UINT:
				case T_ULONGINT:
				case T_USHORTINT:
				case T_FLOAT:
				case T_DOUBLE:
				case T_USEROBJECT:
				printf(" %s;",item->name);
				break;
				case T_BOOLEAN:
					printf(" (long) (0 != %s);",item->name);
					break;
				default: CantHappen();
			} /* switch */
		  } /* gotit */
		  if (item->subscripted) {
			  EmitElementValueAssignments(body,item);
		  }
	  } /* item */
    } /* arg */
    printf("\n"); outlineno++;
  }

static void EmitMappedNames()
{
register TokenList n;
int i=0;
    for (n = nameTokens; n!= NULL; n = n->next) {
	printf("  _dpsP[%d].val.nameVal = _dpsCodes[%d];\n",
	   		n->token->tokenIndex, i++);
        outlineno++;
    }
}

static void AssignHNumHeader(t)
Token t;
{
  switch (t->namedFormal->type) {
  		case T_NUMSTR:
			printf("  HNumHeader[1] = ((sizeof(int) == 4) ? 0 : 32)\n");
			if(t->namedFormal->scaled) {
				if(t->namedFormal->scale->constant)
				  printf("                + %d + ((DPS_DEF_TOKENTYPE %% 2) * 128);\n",
				  		t->namedFormal->scale->val);
				else
				  printf("                + %s + ((DPS_DEF_TOKENTYPE %% 2) * 128);\n",
				  		t->namedFormal->scale->name);
			} else
				printf("                + ((DPS_DEF_TOKENTYPE %% 2) * 128);\n");
			outlineno += 2;
			break;
		case T_FLOATNUMSTR:
			printf("  HNumHeader[1] = 48 + ((DPS_DEF_TOKENTYPE %% 2) * 128)\n");
			printf("                + ((DPS_DEF_TOKENTYPE >= 130) ? 1 : 0);\n");
			outlineno += 2;
			break;
		case T_LONGNUMSTR:
			if(t->namedFormal->scaled) {
				if(t->namedFormal->scale->constant)
					printf("  HNumHeader[1] = %d + (DPS_DEF_TOKENTYPE %% 2) * 128;\n",
							t->namedFormal->scale->val);
				else
					printf("  HNumHeader[1] = %s + (DPS_DEF_TOKENTYPE %% 2) * 128;\n",
							t->namedFormal->scale->name);
			} else
				printf("  HNumHeader[1] = (DPS_DEF_TOKENTYPE %% 2) * 128;\n");
			outlineno ++;
			break;
		case T_SHORTNUMSTR:
			if(t->namedFormal->scaled) {
			  if(t->namedFormal->scale->constant)
			    printf("  HNumHeader[1] = %d + 32 + (DPS_DEF_TOKENTYPE %% 2) * 128;\n",
							t->namedFormal->scale->val);
			  else
			    printf("  HNumHeader[1] = %s + 32 + (DPS_DEF_TOKENTYPE %% 2) * 128;\n",
							t->namedFormal->scale->name);
			} else
				printf("  HNumHeader[1] = 32 + ((DPS_DEF_TOKENTYPE %% 2) * 128);\n");
			outlineno ++;
			break;
		default:
			CantHappen();
	}
	if(t->namedFormal->subscript->constant) {
		printf("  HNumHeader[(DPS_DEF_TOKENTYPE %% 2) ? 2 : 3] = (unsigned char) %d;\n", t->namedFormal->subscript->val);
		printf("  HNumHeader[(DPS_DEF_TOKENTYPE %% 2) ? 3 : 2] = (unsigned char) (%d >> 8);\n", t->namedFormal->subscript->val);
	} else {
		printf("  HNumHeader[(DPS_DEF_TOKENTYPE %% 2) ? 2 : 3] = (unsigned char) %s;\n", t->namedFormal->subscript->name);
		printf("  HNumHeader[(DPS_DEF_TOKENTYPE %% 2) ? 3 : 2] = (unsigned char) (%s >> 8);\n", t->namedFormal->subscript->name);
	}
	outlineno += 2;
}
  
static void WriteObjSeq(sz)
unsigned sz;
{
  register TokenList tl;
   
#ifdef os_mpw
  printf("  DPSBinObjSeqWrite(%s,_dpsF,%d);\n",
  	ctxName, (twoStatics ? sz : sz + stringBytes) + dpsHeaderSize);
#else
  printf("  DPSBinObjSeqWrite(%s,(char *) &_dpsF,%d);\n",
    ctxName, (twoStatics ? sz : sz + stringBytes) + dpsHeaderSize);
#endif
  outlineno++;

  for (tl = namedInputArrays; tl; tl = tl->next) {
    Token t = tl->token;
    printf("  DPSWriteTypedObjectArray(%s, %s, (char *)%s, ",
    	ctxName,
        CTypeToResultType(t->namedFormal->type),
        t->namedFormal->name);
    if (t->namedFormal->subscript->constant)
      printf("%d);\n", t->namedFormal->subscript->val);
    else
      printf("%s);\n", t->namedFormal->subscript->name);
    outlineno++;
    }

  for (tl = namedInputStrings; tl; tl = tl->next) {
    Token t = tl->token;
    boolean firstNumStr = true;
    if(IsNumStrType(t->namedFormal->type)) {
    	if(firstNumStr) {
			printf("  HNumHeader[0] = %d;\n", HNUMTOKEN);
			outlineno ++;
			firstNumStr = false;
		}
    	AssignHNumHeader(t);
		printf("  DPSWriteStringChars(%s, (char *)HNumHeader, %d);\n",
				ctxName, NUMSTR_HEADER_SIZE);
		outlineno ++;
	}
    printf("  DPSWriteStringChars(%s, (char *)%s, ",
        ctxName, t->namedFormal->name);
    if (!t->namedFormal->subscripted) {
      if(IsNumStrType(t->namedFormal->type))
      	printf("_dpsP[%d].length * sizeof(%s));\n",
			 t->tokenIndex, TypeToText(t->namedFormal->type));
	  else
        printf("_dpsP[%d].length);\n", t->tokenIndex);
      if(pad) {
#ifdef NeXT
      	printf("  DPSWriteStringChars(%s, (char *)%s, ~(_dpsP[%d].length + %d) & %d);\n",
        ctxName,ctxName,t->tokenIndex,WORD_ALIGN,WORD_ALIGN);
#else
      	printf("  DPSWriteStringChars(%s, (char *)pad, ~(_dpsP[%d].length + %d) & %d);\n",
        ctxName,t->tokenIndex,WORD_ALIGN,WORD_ALIGN);
#endif
		outlineno ++;
	  }
    } else 
    	if (t->namedFormal->subscript->constant) {
			int val = t->namedFormal->subscript->val;
			if(IsNumStrType(t->namedFormal->type)) {
      			printf("%d * sizeof(%s));\n", val, TypeToText(t->namedFormal->type));
				if(pad & IsPadNumStrType(t->namedFormal->type)){
#ifdef NeXT
					printf("  DPSWriteStringChars(%s, (char *)%s, ~((%d * sizeof(%s)) + %d) & %d);\n",
						ctxName,ctxName,val,TypeToText(t->namedFormal->type),
						WORD_ALIGN+NUMSTR_HEADER_SIZE,WORD_ALIGN);
#else
					printf("  DPSWriteStringChars(%s, (char *)pad, ~((%d * sizeof(%s)) + %d) & %d);\n",
						ctxName,val,TypeToText(t->namedFormal->type),
						WORD_ALIGN+NUMSTR_HEADER_SIZE,WORD_ALIGN);
#endif
					outlineno ++;
				}
	  		} else {
      			printf("%d);\n", val);
				if(pad){
					val = ~(val + WORD_ALIGN) & WORD_ALIGN;
					if(val) {
#ifdef NeXT
					  printf("  DPSWriteStringChars(%s, (char *)%s, %d);\n",
					  		  ctxName,ctxName,val);
#else
					  printf("  DPSWriteStringChars(%s, (char *)pad, %d);\n",
					  		  ctxName,val);
#endif
					  outlineno ++;
					}
				}
			}
        } else {
			if(IsNumStrType(t->namedFormal->type)) {
      			printf("%s * sizeof(%s));\n", t->namedFormal->subscript->name, 
						TypeToText(t->namedFormal->type));
				if(pad & IsPadNumStrType(t->namedFormal->type)) {
#ifdef NeXT
					printf("  DPSWriteStringChars(%s, (char *)%s, ~((%s * sizeof(%s)) + %d) & %d);\n",
						ctxName,ctxName,t->namedFormal->subscript->name,
						TypeToText(t->namedFormal->type),
#else
					printf("  DPSWriteStringChars(%s, (char *)pad, ~((%s * sizeof(%s)) + %d) & %d);\n",
						ctxName,t->namedFormal->subscript->name,
						TypeToText(t->namedFormal->type),
#endif
						WORD_ALIGN+NUMSTR_HEADER_SIZE,WORD_ALIGN);
					outlineno ++;
				}
			} else {
      			printf("%s);\n", t->namedFormal->subscript->name);
				if(pad) {
#ifdef NeXT
					printf("  DPSWriteStringChars(%s, (char *)%s, ~(%s + %d) & %d);\n",
						ctxName,ctxName,t->namedFormal->subscript->name,
						WORD_ALIGN,WORD_ALIGN);
#else
					printf("  DPSWriteStringChars(%s, (char *)pad, ~(%s + %d) & %d);\n",
						ctxName,t->namedFormal->subscript->name,
						WORD_ALIGN,WORD_ALIGN);
#endif
					outlineno ++;
				}
		   }
	   }
    outlineno ++;
    }

  if (twoStatics) {
#ifdef os_mpw
    printf("  DPSWriteStringChars(%s,_dpsF1,%d);\n",ctxName,stringBytes);
#else
    printf("  DPSWriteStringChars(%s,(char *) &_dpsF1,%d);\n",
      ctxName,stringBytes);
#endif
    outlineno++;
    }
  }  /* WriteObjSeq */


/*************************************************************/
/* Public procedures, called by the semantic action routines */

void EmitPrototype(hdr) Header hdr; {
  /* emit procedure prototype to the output .h file, if any */
  
  fprintf(header, "\n");
  fprintf(header, "extern void %s(", hdr->name);
  if (doANSI) EmitANSIPrototypes(header, hdr);
  else if (hdr->inArgs || hdr->outArgs) {
    fprintf(header, " /* ");
    EmitArgPrototypes(header, hdr);
    fprintf(header, "*/ ");
    }
  fprintf(header, ");\n");
  }

void EmitBodyHeader(hdr) Header hdr; {
  /* emit procedure header */
  register Arg arg;
  register Item item;
  
  nameTokens = NULL;
  nNames = 0;
  ctxName = NULL;
#ifdef os_mpw
  nameTokenBytes = 0;
#endif

  if (hdr->isStatic) printf("static ");
  printf("void %s(", hdr->name);

  if (doANSI) {
    EmitANSIPrototypes(stdout,hdr);
    printf(")\n");
    outlineno++;
  }
  else { /* not ANSI */
    for (arg = hdr->inArgs; arg; arg = arg->next) {
      for (item = arg->items; item; item = item->next) {
	if (arg->type == T_CONTEXT) ctxName = item->name;
	printf(item->name);
	if (item->next) printf(", ");
	}
      if (arg->next || hdr->outArgs) printf(", ");
      } /* inArgs */
    for (arg = hdr->outArgs; arg; arg = arg->next) {
      for (item = arg->items; item; item = item->next) {
	printf(item->name);
	if (item->next) printf(", ");
	}
      if (arg->next) printf(", ");
      } /* outArgs */
    printf(")\n"); outlineno++;
    if (hdr->inArgs || hdr->outArgs) {
      EmitArgPrototypes(stdout, hdr);
      printf("\n");
      outlineno++;
      }
    }
} /* EmitBodyHeader */

void EmitBody(body, hdr) Tokens body; Header hdr; {
  Args arg, outArgs = hdr->outArgs;
  Item item;
  long int nObjs;
  unsigned structSize;
    /* total number of objects plus string bodies in objrecs.
       Not including array arg expansions */
  Adr sizeAdr;

  if(NumTokens(body) == 0) 
  	return;				/* empty wrap */
  	
  if (outArgs) body = AppendResultFlush(body, NumArgs(outArgs));

  FlattenSomeArrays(body, false);
  
  if (large = ((NumTokens(body) > 0xff)) || CheckSize(body))
  	dpsHeaderSize = DPS_LONG_HEADER_SIZE;
  else
  	dpsHeaderSize = DPS_HEADER_SIZE;
	
  /* check for char * input args */
  for (arg = hdr->inArgs; arg && !large; arg = arg->next) {
	for (item = arg->items; item; item = item->next) {
		if ((arg->type == T_CHAR) && item->starred) {
			/* if arg is char * then need to use large format since
			   size of arg is unknown */
			large = true;
  			dpsHeaderSize = DPS_LONG_HEADER_SIZE;
		}
	}
  }
	 
  BuildTypesAndAssignAddresses(body, &sizeAdr, &nObjs, &structSize);
  /* also constructs namedInputArrays, namedInputStrings and literalStrings */
  
  ConstructStatics(body, &sizeAdr, nObjs);

  EmitLocals(structSize);
  
  if (outArgs) EmitResultTagTableDecls(outArgs);

#ifdef os_mpw
  printf("  if (wrap_gp == NULL || wrap_gp->wb[%d] == NULL) {\n", nWraps);
  printf("    InitWrap(%d, %d);\n", nWraps, thisOffset);
  outlineno += 2;

  if (nameTokens) EmitNameTagAcquisition(objBytes + dpsHeaderSize + stringBytes);
  else if (stringBytes > 0) { /* round for strings */
    int rounded = ((stringBytes + 3) >> 2) << 2;
	if (rounded != stringBytes) {
		int nOffset = (rOffset - stringBytes) + rounded;
		fprintf(rdata,"\t/* offset is %ld -> %ld */\n", rOffset,nOffset);
		fprintf(rtype,"\n\talign long; /* %ld -> %ld */\n",rOffset,nOffset);
		rOffset = nOffset;
	}
  }
  printf("  }\n");
  outlineno++;
#endif

  if (nameTokens) {
#ifndef os_mpw
    EmitNameTagAcquisition();
#endif
  }

#ifdef os_mpw
  if(reentrant && writable)
    printf("  BlockMove(wrap_gp->wb[%d], _dpsF, sizeof(_dpsF));\n", nWraps);
  else
    printf("  _dpsF = wrap_gp->wb[%d];\n", nWraps);
  outlineno++;
  if (twoStatics) {
    printf("  _dpsF1 = _dpsF + %d;\n", dpsHeaderSize + objBytes);
    outlineno++;
  }
  if (writable) {
    printf("  _dpsP = _dpsF + %d;\n", dpsHeaderSize);
    outlineno++;
  }
#else
  if(reentrant && writable) {
    printf("  _dpsF = _dpsStat;	/* assign automatic variable */\n");
    outlineno++;
  }
#endif
  if(writable) {
  	ScanParamsAndEmitValues(body,hdr->inArgs);
  }
  
  if(doANSI && nameTokens) {
    EmitMappedNames();
    FreeTokenList(nameTokens);
    nameTokens = NULL;
  }

  /* Fixup offsets and the total size */

  if (writable && (namedInputArrays || namedInputStrings))  {
    FixupOffsets();
#ifdef os_mpw
    printf("\n  ((DPSBinObjSeq) _dpsF)->length = _dps_offset+%d;\n",dpsHeaderSize);
#else
    printf("\n  _dpsF.nBytes = _dps_offset+%d;\n", dpsHeaderSize);
#endif
    outlineno += 2;
  }

  if (outArgs) EmitResultTagTableAssignments(outArgs);

  WriteObjSeq(structSize);

  FreeTokenList(namedInputArrays); namedInputArrays = NULL;
  FreeTokenList(namedInputStrings); namedInputStrings = NULL;
  FreeTokenList(literalStrings); literalStrings = NULL;

  if (outArgs) {
    printf("  DPSAwaitReturnValues(%s);\n", ctxName);
    outlineno++;
    }
  } /* EmitBody */

static void AllocFailure()
{
    extern int yylineno;
    extern int bigFile;
    ErrIntro(yylineno);
    fprintf(stderr, "pswrap is out of storage; ");
    if (bigFile)
	fprintf(stderr, "try splitting the input file\n");
    else
	fprintf(stderr, "try -b switch\n");
    exit(1);
}

char *psw_malloc(s) int s; {
    char *temp;
    extern char *malloc();
    if ((temp = malloc((unsigned) s)) == NULL)
        AllocFailure();
    return(temp);
}

char *psw_calloc(n,s) int n,s; {
    char *temp;
    extern char *calloc();
    if ((temp = calloc((unsigned) n, (unsigned) s)) == NULL)
        AllocFailure();
    return(temp);
}

FreeBody(body) Body body; {
  register Token t, nexttoken;

  for (t = body; t; t = nexttoken) {
#ifdef os_mpw
      SpinCursor(1);
#endif
      nexttoken = t->next;
      if (t->adr.var) free(t->adr.var);
      switch (t->type) {
	  case T_STRING:
	  case T_NAME:
	  case T_LITNAME:
	  case T_HEXSTRING:
	     free (t->val);
	     break;
	  case T_FLOAT:
	  case T_INT:
	  case T_BOOLEAN:
	     break;
	  case T_SUBSCRIPTED:
	     free (t->val); free(t->body.var);
	     break;
	  case T_ARRAY:
	  case T_PROC:
	     FreeBody((Body) (t->val));
	     break;
	  default:
	     CantHappen();
      }
      free (t);
  }
}
