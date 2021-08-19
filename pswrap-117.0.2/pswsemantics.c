/*
  pswsemantics.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Paul Rovner: Wednesday, May 18, 1988 2:05:42 PM
Edit History:
Andrew Shore: Mon Aug 15 10:44:24 1988
Richard Cohn: Fri Oct 21 15:23:15 1988
Bill Bilodeau: Fri Jun 29 17:39:12 PDT 1990
End Edit History.
*/

/***********/
/* Imports */
/***********/

#include <stdio.h>
#include <string.h>

#ifdef os_mpw
#include <ErrMgr.h>
#include <Errors.h>
#include <CursorCtl.h>
#endif

#include "pswdict.h"
#include "pswpriv.h"

extern char *hfile;
extern char *ofile;
extern char *ifile;
extern FILE *header;
extern int yylineno; /* current line number in pswrap source file */
extern int outlineno;

extern void EmitPrototype();
extern void EmitBodyHeader();
extern void EmitBody();

extern char *psw_malloc();
extern char *psw_calloc();

/***********************/
/* Module-wide globals */
/***********************/

char *currentPSWName = NULL;
int reportedPSWName = 0;

static PSWDict currentDict = NULL;


/*************************************************/
/* Procedures called by the parser's annotations */
/*************************************************/

static boolean IsCharType(t) Type t; {
  return (t == T_CHAR || t == T_UCHAR);
  }

static boolean IsNumStrType(t) Type t; {
	return (t == T_NUMSTR
			|| t == T_FLOATNUMSTR
			|| t == T_LONGNUMSTR
			|| t == T_SHORTNUMSTR);
}
  
void PSWName(s) char *s; {
  currentPSWName = psw_malloc(strlen(s)+1);
  strcpy(currentPSWName, s);
  reportedPSWName = 0;
  }
  
  /* Generate the code for this wrap now */
void FinalizePSWrapDef(hdr, body)
  Header hdr; Body body;
{
  extern int bigFile;
#ifdef os_mpw
  extern int nWraps;
#endif
  
  if (header && ! hdr->isStatic) EmitPrototype(hdr); 

  printf("#line %d \"%s\"\n", ++outlineno, ofile);
  EmitBodyHeader(hdr);
  
  printf("{\n"); outlineno++;
  EmitBody(body, hdr);
  printf("}\n"); outlineno++;
  printf("#line %d \"%s\"\n", yylineno, ifile); outlineno++;
#ifdef os_mpw
  nWraps++;
#endif

  /* release storage for this wrap */
  /* Omit if you have lots of memory and want pswrap lean and mean */
  if (bigFile) {
    register Arg arg, nextarg; register Item item, nextitem;
    for(arg = hdr->inArgs; arg; arg = nextarg) {
	nextarg = arg->next;
	for(item = arg->items; item; item = nextitem) {
	    nextitem = item->next;
	    if (item->subscripted) {
		  if (!item->subscript->constant) free(item->subscript->name);
		  free(item->subscript);
		  if(item->scaled) {
		  	if (!item->scale->constant) free(item->scale->name);
		  	free(item->scale);
		  }
	    }
	    free(item->name); free(item);
	}
	free(arg);
    }
    for(arg = hdr->outArgs; arg; arg = nextarg) {
	nextarg = arg->next;
	for(item = arg->items; item; item = nextitem) {
	    nextitem = item->next;
	    if (item->subscripted) {
		if (!item->subscript->constant) free(item->subscript->name);
		free(item->subscript);
	    }
	    free(item->name); free(item);
	}
	free(arg);
    }
    free(hdr->name); free(hdr);
    FreeBody(body);
  }

  DestroyPSWDict(currentDict);
  currentDict = NULL;
  currentPSWName = NULL;
  reportedPSWName = 0;
  }

  /* Complete construction of the Header tree and make some semantic checks */
Header PSWHeader(isStatic, inArgs, outArgs)
  boolean isStatic; Args inArgs, outArgs; {
  char *name = currentPSWName;
  register Arg arg, prevArg;
  register Item item, prevItem;
  int nextTag = 0;
  
  Header hdr = (Header)psw_calloc(sizeof(HeaderRec), 1);
  hdr->isStatic = isStatic;
  hdr->name = name;

  currentDict = CreatePSWDict(511);

  prevArg = NULL;
  for (arg = inArgs; arg; arg = arg->next) { /* foreach input arg */
    prevItem = NULL;
    for (item = arg->items; item; item = item->next) {
      if (IsCharType(arg->type)
          && !(item->starred || item->subscripted)) {
          ErrIntro(item->sourceLine);
          fprintf(stderr,
                 "char input parameter %s must be starred or subscripted\n",
                 item->name);
          /* remove item from list */
          if (prevItem) {prevItem->next = item->next;}
          else if (item == arg->items) {arg->items = item->next;};
          /* free(item);  XXX? */
          continue;
      }
      if(item->scaled && !IsNumStrType(arg->type)) {
          ErrIntro(item->sourceLine);
          fprintf(stderr,"only numstring parameters may be scaled\n");
	  }
      if (IsNumStrType(arg->type)
	      && (item->starred || !item->subscripted)) {
          ErrIntro(item->sourceLine);
          fprintf(stderr,
                 "numstring parameter %s may only be subscripted\n",
                 item->name);
          /* remove item from list */
          if (prevItem) {prevItem->next = item->next;}
          else if (item == arg->items) {arg->items = item->next;};
          /* free(item);  XXX? */
          continue;
      }
      if (arg->type != T_CONTEXT) {
          if (PSWDictLookup(currentDict, item->name) != -1) {
             ErrIntro(item->sourceLine);
             fprintf(stderr,"parameter %s reused\n", item->name);
             if (prevItem) {prevItem->next = item->next;}
	         else if (item == arg->items) {arg->items = item->next;};
	         /* free this ? */
	         continue;
          }
	      PSWDictEnter(currentDict, item->name, (PSWDictValue) item);
          item->isoutput = false;
          item->type = arg->type;
          prevItem = item;
      }
    }
    if (arg->items == NULL) {
      	if (prevArg) { prevArg->next = arg->next;}
	    else if (arg == inArgs) {inArgs = arg->next;}
	    continue;
    }
      prevArg = arg;
  }

  prevArg = NULL;
  for (arg = outArgs; arg; arg = arg->next) { /* foreach output arg */
    prevItem = NULL;
    for (item = arg->items; item; item = item->next) {
      if (arg->type == T_USEROBJECT) {
 	     ErrIntro(item->sourceLine);
         fprintf(stderr,"output parameter can not be of type userobject\n",
               item->name);
 	     /* remove item from list */
 	     if (prevItem) {prevItem->next = item->next;}
 	     else if (item == arg->items) {arg->items = item->next;};
 	    /* free(item); XXX */
 	   continue;
      }
      if (arg->type == T_NUMSTR || arg->type == T_FLOATNUMSTR
      	  || arg->type == T_LONGNUMSTR || arg->type == T_SHORTNUMSTR) {
 	     ErrIntro(item->sourceLine);
         fprintf(stderr,"output parameter %s can not be of type numstring\n",
               item->name);
 	     /* remove item from list */
 	     if (prevItem) {prevItem->next = item->next;}
 	     else if (item == arg->items) {arg->items = item->next;};
 	    /* free(item); XXX */
 	   continue;
      }
      if (!(item->starred || item->subscripted)) {
	    ErrIntro(item->sourceLine);
        fprintf(stderr,"output parameter %s must be starred or subscripted\n",
              item->name);
	    /* remove item from list */
	    if (prevItem) {prevItem->next = item->next;}
	    else if (item == arg->items) {arg->items = item->next;};
	    /* free(item); XXX */
	    continue;
      }
      if (PSWDictLookup(currentDict, item->name) != -1) {
	    ErrIntro(item->sourceLine);
	    fprintf(stderr,"parameter %s reused\n", item->name);
	    /* remove item from list */
	    if (prevItem) {prevItem->next = item->next;}
	    else if (item == arg->items) {arg->items = item->next;};
	    /* free the storage? XXX */
        continue;
      }
      PSWDictEnter(currentDict, item->name, (PSWDictValue) item);
      item->isoutput = true;
      item->type = arg->type;
      item->tag = nextTag++;
      prevItem = item;
   } /* inside for loop */
   if (arg->items == NULL) {
    if (prevArg) { 
    	prevArg->next = arg->next;
    } else if (arg == outArgs) {
    	outArgs = arg->next;
    }
    continue;
   }
      prevArg = arg;
  } /* outside for loop */

  /* now go looking for subscripts that name an input arg */
  for (arg = inArgs; arg; arg = arg->next) { /* foreach input arg */
    for (item = arg->items; item; item = item->next) {
      if (item->subscripted && !item->subscript->constant) {
        PSWDictValue v = PSWDictLookup(currentDict, item->subscript->name);
        if (v != -1) {
          Item subItem = (Item)v;
          if (subItem->isoutput) {
	    ErrIntro(subItem->sourceLine);
            fprintf(stderr,"output parameter %s used as a subscript\n",
	    	subItem->name);
            continue;
            }
          if (subItem->type != T_INT) {
	    ErrIntro(subItem->sourceLine); 
            fprintf(stderr,
	        "input parameter %s used as a subscript is not an int\n",
                subItem->name);
            continue;
            }
          }
        }
      }
    }
  
  for (arg = outArgs; arg; arg = arg->next) { /* foreach output arg */
    for (item = arg->items; item; item = item->next) {
      if (item->subscripted && !item->subscript->constant) {
        PSWDictValue v = PSWDictLookup(currentDict, item->subscript->name);
        if (v != -1) {
          Item subItem = (Item)v;
          if (subItem->isoutput) {
	    ErrIntro(subItem->sourceLine);
            fprintf(stderr,"output parameter %s used as a subscript\n",
	    	subItem->name);
            continue;
            }
          if (subItem->type != T_INT) {
	    ErrIntro(subItem->sourceLine);
	    fprintf(stderr,
	       "input parameter %s used as a subscript is not an int\n",
               subItem->name);
            continue;
            }
          }
        }
      }
    }
  
  hdr->inArgs = inArgs;
  hdr->outArgs = outArgs;

  return hdr;
  }

Token PSWToken(type, val)
  Type type;
  int val; {
  register Token token = (Token)psw_calloc(sizeof(TokenRec), 1);
  extern int errorCount;
  extern char *ifile;
  
  token->next = NULL;
  token->type = type;
  token->val = val;
  token->sourceLine = yylineno;

#ifdef os_mpw
  SpinCursor(1);
#endif

  switch (type) {
    case T_STRING:
    case T_NAME:
    case T_LITNAME: {
      Item dictVal = (Item) PSWDictLookup(currentDict, (char *)val);
      if ((PSWDictValue) dictVal != -1) {
	  if ((type != T_NAME) && (dictVal->isoutput)) {
	      ErrIntro(yylineno);
	      fprintf(stderr,"output parameter %s used as %s\n",
		dictVal->name,
		(type == T_STRING) ? "string": "literal name");
	  } else 
	      if ((type != T_NAME) && !IsCharType(dictVal->type)) {
	      	ErrIntro(yylineno);
	      	fprintf(stderr,"non-char input parameter %s used as %s\n",
			dictVal->name,
			(type == T_STRING) ? "string": "literal name");
	      } else 
	      	token->namedFormal = dictVal; /* ok, so assign a value */
      }
      break;
    }
    default:
      break;
    }

  return token;
  }

Token PSWToken2(type, val, ind)
  Type type;
  int val; char *ind; {
  register Token token = (Token)psw_calloc(sizeof(TokenRec), 1);
  extern int errorCount;
  extern char *ifile;
  Item dictVal = (Item) PSWDictLookup(currentDict, (char *)val);
  Item dvi;

  token->next = NULL;
  token->type = type;
  token->val = val;
  token->sourceLine = yylineno;

  /* Assert(type == T_SUBSCRIPTED); */
  if (((PSWDictValue) dictVal == -1) || (dictVal->isoutput)) {
    ErrIntro(yylineno);
    fprintf(stderr,"%s not an input parameter\n", (char *) val);
  }
  else if (IsCharType(dictVal->type)) {
    ErrIntro(yylineno);
    fprintf(stderr,"%s not a scalar type\n", (char *) val);
  }
  else {
    dvi = (Item) PSWDictLookup(currentDict, (char *)ind);
    if (((PSWDictValue) dvi != -1)
    && ((dvi->isoutput) || IsCharType(dvi->type))) {
      ErrIntro(yylineno);
      fprintf(stderr,"%s wrong type\n",(char *) ind);
    }
    else {
      token->body.var = (char *) ind;
      token->namedFormal = dictVal; /* ok, so assign a value */
      return token;
    }
  }

  /*  ERRORS fall through */
  free(token);
  return (PSWToken(T_NAME,val));
  }

Arg PSWArg(type, items)
  Type type; Items items; {
  register Arg arg = (Arg)psw_calloc(sizeof(ArgRec), 1);
  arg->next = NULL;
  arg->type = type;
  arg->items = items;
  return arg;
  }

Item PSWItem(name)
  char *name; {
  register Item item = (Item)psw_calloc(sizeof(ItemRec), 1);
  item->next = NULL;
  item->name = name;
  item->sourceLine = yylineno;
  return item;
  }

Item PSWStarItem(name)
  char *name; {
  register Item item = (Item)psw_calloc(sizeof(ItemRec), 1);
  item->next = NULL;
  item->name = name;
  item->starred = true;
  item->sourceLine = yylineno;
  return item;
  }

Item PSWSubscriptItem(name, subscript)
  char *name; Subscript subscript; {
  register Item item = (Item)psw_calloc(sizeof(ItemRec), 1);
  item->next = NULL;
  item->name = name;
  item->subscript = subscript;
  item->subscripted = true;
  item->sourceLine = yylineno;
  return item;
  }

Item PSWScaleItem(name, subscript, nameval, val)
  char *name;
  Subscript subscript;
  char *nameval;
  int val;
{
  Item item;
  Scale scale = (Scale)psw_calloc(sizeof(ScaleRec), 1);
  item = PSWSubscriptItem(name, subscript);
  item->scaled = true;
  if(nameval)
  	scale->name = nameval;
  else {
	scale->constant = true;
  	scale->val = val;
  }
  item->scale = scale;
  return(item);
}
  
Subscript PSWNameSubscript(name)
  char *name; {
  Subscript subscript = (Subscript)psw_calloc(sizeof(SubscriptRec), 1);
  subscript->name = name;
  return subscript;
  }

Subscript PSWIntegerSubscript(val)
  int val; {
  Subscript subscript = (Subscript)psw_calloc(sizeof(SubscriptRec), 1);
  subscript->constant = true;
  subscript->val = val;
  return subscript;
  }

Args ConsPSWArgs(arg, args)
  Arg arg; Args args; {
  arg->next = args;
  return arg;
  }

Tokens AppendPSWToken(token, tokens)
  register Token token; Tokens tokens; {
  register Token t;
  static Token firstToken, lastToken;	/* cache ptr to last */
  
  if ((token->type == T_NAME) && (token->namedFormal)) {
    if( token->namedFormal->isoutput) {
    	char *pos = "printobject";
    	char *ss = psw_malloc(strlen(pos) + 1);
   	strcpy(ss, pos);
    	free((char *)token->val);
    	free((char *)token);
   	token = PSWToken(T_INT, token->namedFormal->tag);
    	token->next = PSWToken(T_NAME, ss);
     } else 
    	if (token->namedFormal->type == T_USEROBJECT) {
   		char *pos = "execuserobject";
    		char *ss = psw_malloc(strlen(pos) + 1);
    		strcpy(ss, pos);
   		token->next = PSWToken(T_NAME, ss);
   	}
   }
   	
  if (tokens == NULL) {
    firstToken = lastToken = token;
    return token;
  }
  
  if (tokens != firstToken)
    firstToken = lastToken = tokens;
  for (t = lastToken; t->next; t = t->next);
  lastToken = t->next = token;

  return tokens;
  }

Args AppendPSWArgs(arg, args)
  Arg arg; Args args; {
  register Arg a;
  arg->next = NULL;
  if (args == NULL) return arg;
  
  for (a = args; a->next; a = a->next);

  a->next = arg; 
  return args;
  }

Items AppendPSWItems(item, items)
  Item item; Items items; {
  register Item t;
  item->next = NULL;
  if (items == NULL) return item;
  
  for (t = items; t->next; t = t->next);

  t->next = item; 
  return items;
  }

