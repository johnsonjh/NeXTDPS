/*
  transferimpl.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Scott Byer: Mon May 22 16:40:10 1989
Bill Paxton: Mon Apr  4 12:02:34 1988
Ivor Durham: Sun May 14 09:10:11 1989
Jim Sandman: Wed Oct 25 11:50:36 1989
Joe Pasqua: Mon Dec 19 12:25:03 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include EXCEPT
#include FP
#include DEVICE
#include GRAPHICS
#include LANGUAGE
#include PSLIB
#include RECYCLER
#include VM

#include "graphdata.h"
#include "graphicspriv.h"
#include "gray.h"

public integer tfrTableLimit;
#define DEFAULTTABLELIMIT 14


/* private global variables. make change in both arms if changing variables */

#if (OS != os_mpw)

/*-- BEGIN GLOBALS --*/
private  Pool tfrFcnStorage;    /* Blind pointer to pool for tfr fncs */
private TfrFcn tfrFcnFirst, tfrFcnLast;
  /* Keep ordered list of objects so we can collect tables of values
     for inactive objects. Most recently used object is at end of list. */ 
private integer nTfrTables;
/*-- END GLOBALS --*/

#else (OS != os_mpw)

typedef struct {
  Pool g_tfrFcnStorage;
  TfrFcn g_tfrFcnFirst, g_tfrFcnLast;
  integer g_nTfrTables;
  } GlobalsRec, *Globals;
  
private Globals tfrGlobals;

#define tfrFcnStorage tfrGlobals->g_tfrFcnStorage
#define tfrFcnFirst tfrGlobals->g_tfrFcnFirst
#define tfrFcnLast tfrGlobals->g_tfrFcnLast
#define nTfrTables tfrGlobals->g_nTfrTables

#endif (OS != os_mpw)


private procedure ReleaseTables (tfrFcn) TfrFcn tfrFcn; {
  if (tfrFcn->devTfr != NULL) {
    DevFreeTfrFcn(tfrFcn->devTfr);
    tfrFcn->devTfr = NULL;
    }
  }
  

private procedure UnlinkTfr(tfrFcn) register TfrFcn tfrFcn; {
  integer i;
  if (tfrFcnFirst == tfrFcn) {
    tfrFcnFirst = tfrFcn->next;
    if (tfrFcnLast == tfrFcn) tfrFcnLast = NIL;
    }
  else {
    TfrFcn prev = tfrFcnFirst;
    for (;;) {
      if (prev == NIL) break;
      if (prev->next == tfrFcn) {
        prev->next = tfrFcn->next;
        if (tfrFcnLast == tfrFcn) tfrFcnLast = prev;
        break; }
      prev = prev->next;
      }
    }
  } /* end of UnlinkTfr */


private procedure LinkTfr(t) register TfrFcn t; {
  if (tfrFcnLast != NIL) tfrFcnLast->next = t; 
  else tfrFcnFirst = t;
  tfrFcnLast = t;
  } /* end of LinkTfr */


private TfrFcn GetTfr() {
  register TfrFcn t;
  Object nilFunc;
  XAryObj(nilFunc,0,NIL);
  t = (TfrFcn)os_newelement(tfrFcnStorage);
  os_bzero(t, sizeof(TfrFcnRec));
  t->tfrGryFunc = t->tfrRedFunc = t->tfrGrnFunc = t->tfrBluFunc =
    t->tfrUCRFunc = t->tfrBGFunc = nilFunc;
  t->refcnt = 1;
  return t;
  } /* end of GetTfr */


public procedure AddTfrRef(tfrFcn) TfrFcn tfrFcn; {
  if (tfrFcn != NIL) tfrFcn->refcnt++;}


public procedure RemTfrRef(tfrFcn) TfrFcn tfrFcn; {
  if (tfrFcn == NIL) return;
  if ((--tfrFcn->refcnt) == 0) {
    if (gs != NIL && gs->tfrFcn == tfrFcn) gs->tfrFcn = NIL;
    ReleaseTables(tfrFcn);
    UnlinkTfr(tfrFcn);
    os_freeelement(tfrFcnStorage, tfrFcn);
    }
  else tfrFcn->active = false;
  } /* end of RemScrRef */
  

public procedure PSSetHalftonePhase() {
  DevPoint p;
  p.y = PopInteger();
  p.x = PopInteger();
  /* pop both before store in gs to catch errors */
  gs->screenphase = p;
  }

public procedure PSCurrentHalftonePhase() {
  PushInteger(gs->screenphase.x);
  PushInteger(gs->screenphase.y);
  }

public procedure SetTransfer(
  redFunc, grnFunc, bluFunc, gryFunc, ucrFunc, bgFunc)
  PObject redFunc, grnFunc, bluFunc, gryFunc, ucrFunc, bgFunc;
{
register TfrFcn t;
TfrFcn oldT = gs->tfrFcn;
real red, green, blue, gray, maxColor = MAXCOLOR;
boolean nonNil;
if (gs->noColorAllowed) Undefined();
nonNil = (boolean)(gryFunc->length != 0);
if (!nonNil)
  nonNil = (redFunc->length != 0 || grnFunc->length != 0 ||
            bluFunc->length != 0 || ucrFunc->length != 0 ||
            bgFunc->length != 0);
if (nonNil) {
  t = GetTfr();
  t->tfrGryFunc = *gryFunc;
  t->tfrRedFunc = *redFunc;
  t->tfrGrnFunc = *grnFunc;
  t->tfrBluFunc = *bluFunc;
  t->tfrUCRFunc = *ucrFunc;
  t->tfrBGFunc = *bgFunc;
  gs->tfrFcn = t;
  ActivateTfr(t);
  }
else {
  gs->tfrFcn = NULL;
  }
if (oldT) {
  RemTfrRef(oldT);
  }
SetDevColor(ChangeColor());
}  /* end of SetTransfer */


public procedure PSSetBlkGeneration()  /* setblackgeneration */
{
TfrFcn t = gs->tfrFcn;
Object bgFunc, nilFunc;
PopPArray(&bgFunc); ConditionalInvalidateRecycler (&bgFunc);
if (t != NULL) 
  SetTransfer(&t->tfrRedFunc, &t->tfrGrnFunc, &t->tfrBluFunc,
    &t->tfrGryFunc, &t->tfrUCRFunc, &bgFunc);
else {
  XAryObj(nilFunc,0,NIL);
  SetTransfer(&nilFunc, &nilFunc, &nilFunc, &nilFunc, &nilFunc, &bgFunc);
  }
} /* end of PSSetBlkGeneration */


public procedure PSCrBlkGeneration()  /* currentblackgeneration */
{
  if (gs->tfrFcn)
    PushP(&gs->tfrFcn->tfrBGFunc);
  else {
    Object nilFunc;
    XAryObj(nilFunc,0,NIL);
    PushP(&nilFunc);
    }
} /* end of PSCrBlkGeneration */


public procedure PSSetUCRemoval()  /* setundercolorremoval */
{
TfrFcn t = gs->tfrFcn;
Object ucrFunc, nilFunc;
PopPArray(&ucrFunc); ConditionalInvalidateRecycler (&ucrFunc);
if (t != NULL) 
  SetTransfer(&t->tfrRedFunc, &t->tfrGrnFunc, &t->tfrBluFunc,
    &t->tfrGryFunc, &ucrFunc, &t->tfrBGFunc);
else {
  XAryObj(nilFunc,0,NIL);
  SetTransfer(&nilFunc, &nilFunc, &nilFunc, &nilFunc, &ucrFunc, &nilFunc);
  }
} /* end of PSSetUCRemoval */


public procedure PSCrUCRemoval()  /* currentundercolorremoval */
{
  if (gs->tfrFcn)
    PushP(&gs->tfrFcn->tfrUCRFunc);
  else {
    Object nilFunc;
    XAryObj(nilFunc,0,NIL);
    PushP(&nilFunc);
    }
} /* end of PSCrUCRemoval */


public procedure PSSetTransfer()
{
TfrFcn t = gs->tfrFcn;
Object gryFunc, ucrFunc, bgFunc;
PopPArray(&gryFunc); ConditionalInvalidateRecycler (&gryFunc);
if (t) {ucrFunc = t->tfrUCRFunc; bgFunc = t->tfrBGFunc;}
else {XAryObj(ucrFunc,0,NIL); XAryObj(bgFunc,0,NIL);}
SetTransfer(&gryFunc, &gryFunc, &gryFunc, &gryFunc, &ucrFunc, &bgFunc);
}  /* end of PSSetTransfer */


public procedure PSCrTransfer()  {
  if (gs->tfrFcn)
    PushP(&gs->tfrFcn->tfrGryFunc);
  else {
    Object nilFunc;
    XAryObj(nilFunc,0,NIL);
    PushP(&nilFunc);
    }
  }

public procedure PSSetClrTransfer()
{
TfrFcn t = gs->tfrFcn;
Object gryFunc, redFunc, grnFunc, bluFunc, ucrFunc, bgFunc;

PopPArray(&gryFunc); ConditionalInvalidateRecycler (&gryFunc);
PopPArray(&bluFunc); ConditionalInvalidateRecycler (&bluFunc);
PopPArray(&grnFunc); ConditionalInvalidateRecycler (&grnFunc);
PopPArray(&redFunc); ConditionalInvalidateRecycler (&redFunc);

if (t) {ucrFunc = t->tfrUCRFunc; bgFunc = t->tfrBGFunc;}
else {XAryObj(ucrFunc,0,NIL); XAryObj(bgFunc,0,NIL);}
SetTransfer(&redFunc, &grnFunc, &bluFunc, &gryFunc, &ucrFunc, &bgFunc);
}  /* end of PSSetClrTransfer */


public procedure PSClrTransfer()
{
TfrFcn t;
t = gs->tfrFcn;
if (t == NIL) {
  integer i;
  Object nilFunc;
  XAryObj(nilFunc,0,NIL);
  for (i = 0; i < 4; i++)
    PushP(&nilFunc);
  return;
  }
PushP(&t->tfrRedFunc);
PushP(&t->tfrGrnFunc);
PushP(&t->tfrBluFunc);
PushP(&t->tfrGryFunc);
}  /* end of PSClrTransfer */



private boolean GetInactiveTables() {
  TfrFcn tf = tfrFcnFirst;
  for (tf = tfrFcnFirst; tf != NULL; tf = tf->next) {
    if (!tf->active && tf->devTfr != NULL) {
      ReleaseTables(tf); return true; }
    }
  return false;
  }

private procedure FillInTfrTbl(ob, tbl)
  Object *ob; PCard8 tbl; {
  register integer i, j;
  register PCard8 pval;
  real gray, maxcolor, invmax;
  maxcolor = (real)MAXCOLOR;
  invmax = fpOne / maxcolor;
  for (i = 0; i <= MAXCOLOR; i++) {
    gray = (real)i * invmax;
    if (ob->length != 0) {
      PushPReal(&gray);
      if (psExecute(*ob)) break;
      PopPReal(&gray);
      }
    j = (integer)(gray * maxcolor + fpHalf);
    j = (j < 0) ? 0 : ((j > MAXCOLOR) ? MAXCOLOR : j);
    *(tbl++) = j;
    }
  }

private procedure FillInUCR(ob, tbl)
  Object *ob; DevShort *tbl; {
  register integer i, j;
  real gray, maxcolor, invmax;
  maxcolor = (real)MAXCOLOR;
  invmax = fpOne / maxcolor;
  for (i = 0; i <= MAXCOLOR; i++) {
    gray = (real)i * invmax;
    if (ob->length != 0) {
      PushPReal(&gray);
      if (psExecute(*ob)) break;
      PopPReal(&gray);
      }
    j = (integer)(gray * maxcolor + fpHalf);
    j = (j < -MAXCOLOR) ? -MAXCOLOR : ((j > MAXCOLOR) ? MAXCOLOR : j);
    *(tbl++) = j;
    }
  }

private procedure FillInBG(bgOb, grayOb, tbl)
  Object *bgOb, *grayOb; PCard8 tbl; {
  register integer i, j;
  real gray, gray0, maxcolor, invmax;
  maxcolor = (real)MAXCOLOR;
  invmax = fpOne / maxcolor;
  for (i = 0; i <= MAXCOLOR; i++) {
    gray = (real)i * invmax;
    if (bgOb->length != 0) {
      PushPReal(&gray);
      if (psExecute(*bgOb)) break;
      PopPReal(&gray);
      }
    LimitColor(&gray);
    if (grayOb->length != 0) {
      PushPReal(&gray);
      if (psExecute(*grayOb)) break;
      PopPReal(&gray);
      }
    j = (integer)(gray * maxcolor + fpHalf);
    *(tbl++) = (j < 0) ? 0 : ((j > MAXCOLOR) ? MAXCOLOR : j);
    }
  }

public procedure ActivateTfr(t) 
  register TfrFcn t;
  {
   boolean white, red, green, blue, ucr, bg;
   DevTfrFcn *dt = NULL;
   if (t == NULL || t->active) return;
   if (t->devTfr == NULL) {
      white = (t->tfrGryFunc.length != 0);
      red = (t->tfrRedFunc.length != 0);
      green = (t->tfrGrnFunc.length != 0);
      blue = (t->tfrBluFunc.length != 0);
      ucr = (t->tfrUCRFunc.length != 0);
      bg = (t->tfrBGFunc.length != 0);
      while(true) {
	 t->devTfr = dt = DevAllocTfrFcn(white, red, green, blue, ucr, bg);
	 if (dt != NULL)
	   break;
	 if (!GetInactiveTables())
	   LimitCheck();
	}
      if (white) FillInTfrTbl(&t->tfrGryFunc, dt->white);
      if (red) FillInTfrTbl(&t->tfrRedFunc, dt->red);
      if (green) FillInTfrTbl(&t->tfrGrnFunc, dt->green);
      if (blue) FillInTfrTbl(&t->tfrBluFunc, dt->blue);
      if (ucr) FillInUCR(&t->tfrUCRFunc, dt->ucr);
      if (bg)
	FillInBG(&t->tfrBGFunc, &t->tfrGryFunc, dt->bg);
     }
   t->active = true;
   UnlinkTfr(t);		/* put on end of list */
   LinkTfr(t);
  }



public procedure IniTransfer(reason)  InitReason reason;
{
integer i;
switch (reason)
  {
  case init:
#if (OS == os_mpw)
    tfrGlobals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    tfrFcnStorage =  (Pool) os_newpool(sizeof(TfrFcnRec),3,0);
    tfrFcnFirst = tfrFcnLast = NULL;
    nTfrTables = 0;
    if (tfrTableLimit == 0) tfrTableLimit = DEFAULTTABLELIMIT;
    break;
  case romreg:
    break;
  case ramreg:
    break;
  }
} /* end of IniGraphics */
