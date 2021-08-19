/* PostScript Graphics User Path Cache Procedures

              Copyright 1988 -- Adobe Systems, Inc.
            PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: Bill Paxton: Tue Apr  5 15:33:33 1988
Edit History:
Larry Baer: Fri Nov 10 13:45:21 1989
Scott Byer: Thu Jun  1 15:18:39 1989
Ivor Durham: Fri Sep 23 16:07:24 1988
Bill Paxton: Wed Jan 31 14:09:54 1990
Ed Taft: Thu Jul 28 16:46:05 1988
Jim Sandman: Wed Dec 13 13:02:16 1989
Perry Caro: Thu Nov 17 10:13:11 1988
Joe Pasqua: Tue Jan 17 13:41:56 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include EXCEPT
#include FP
#include GRAPHICS
#include DEVICE
#include LANGUAGE
#include SIZES
#include VM

#include "path.h"
#include "stroke.h"
#include "graphicspriv.h"
#include "userpath.h"
#include "graphdata.h"

#if DPSXA
extern Cd UOffset, uXAc1, uXAc2;
extern boolean uXARectangle;
#endif DPSXA

extern procedure NoOp();
extern procedure PSClrToMrk();
extern integer DevPrimBytes();
extern procedure PSMark(), MarkDevPrim(), UsrPthBBox();
extern DevPrim *DoRdcPth(), *GetDevClipPrim(), *DoRdcStroke();
extern boolean UsrPthQRdcOk(), QEnumOk();
extern procedure FillUserPathEnumerate(), QFillUserPathEnumerate();
extern procedure StrokeUserPathEnumerate(), QStrokeUserPathEnumerate();

#define BMAX     (300000) /* total size for everything */
#define BLIMMAX  (666)     /* max for blimit, *BMAX/1000 */
#define BLIMIT   (333)  /* max size for reduction plus path int, *BMAX/1000 */
#define RMAX     (3)    /* max number of reduced paths */
#define PMAX	 (33)   /* max size for path info *BMAX/1000*/

typedef struct { /* for circle masks */
  PMask mask;
  boolean flushed:8;
  Card16 refcnt;
  } CircMask;

typedef struct { /* all the stuff from GState that controls a stroke */
  real flatEps;
  real lineWidth;
  real miterlimit;
  BitField lineCap:2;
  BitField lineJoin:2;
  boolean  strokeAdjust:1;
  boolean ismtx:1;
  boolean circletraps:1;
  BitField maskcount:3; /* [0..4] */
    BitField unused:6;
  Card16 dashCnt; /* length of gs->dashArray */
  real *dashArray;     /* contents of gs->dashArray */
  real dashOffset;
  DevShort maskID;
  character circles[4]; /* indexes into CircMaskTable */
  Mtx strkmtx;  /* for userpath matrix ustroke kind of operations */
  } StrkRec, *PStrk;

typedef struct _rdcrec {
  struct _rdcrec *next;
  struct _rdcrec *lrunext, *lruprev;
  Mtx mtx;
  boolean fill:1;
  boolean evenOdd:1;
  boolean circleAdjust:1;
  Card32 size:29;
  PStrk strk; /* null if fill */
  DevPrim *dp;
  struct _ucacherec *pth;
  } RdcRec, *PRdc;

typedef struct _ucacherec {
  struct _ucacherec *next;
  boolean encoded:1;
  boolean packed:1;
    BitField unused:14;
  Card16 ctrllen;
  string ctrlstr;
  string datastr;
  PCard32 bodyvals;
  Card16 len;
  Card32 hash;
  Card16 size;
  PRdc prdc;
  } UCacheRec, *PUCache;

#define UCMSK (0x1F)
#define UCSZ (UCMSK+1)

#define CIRCMASKMAX (32)

#define HASHSHIFT (3)
#define TABLEINDEX(hash) (((hash) >> HASHSHIFT) & UCMSK)

/* this is global data shared by all contexts */
/* private global variables. make change in both arms if changing variables */

#if (OS != os_mpw)

/*-- BEGIN GLOBALS --*/
private PUCache *UCache;
private PRdc lruNewest, lruOldest;
private Pool upcStorage, rdcStorage, strkStorage;
private integer bsize, bmax, rsize, rmax, pmax;
private CircMask *CircMaskTable;
#if STAGE==DEVELOP
private integer rdcprobes, rdchits, pthprobes, pthhits;
#endif STAGE==DEVELOP
/*-- END GLOBALS --*/

#else (OS != os_mpw)

typedef struct {
  PUCache *g_UCache;
  PRdc g_lruNewest, g_lruOldest;
  Pool g_upcStorage, g_rdcStorage, g_strkStorage;
  integer g_bsize, g_bmax, g_rsize, g_rmax, g_pmax;
  CircMask *g_CircMaskTable;
#if STAGE==DEVELOP
  integer g_rdcprobes, g_rdchits, g_pthprobes, g_pthhits;
#endif STAGE==DEVELOP
  } GlobalsRec, *Globals;
  
private Globals globals;

#define UCache globals->g_UCache
#define lruNewest globals->g_lruNewest
#define lruOldest globals->g_lruOldest
#define upcStorage globals->g_upcStorage
#define rdcStorage globals->g_rdcStorage
#define strkStorage globals->g_strkStorage
#define bsize globals->g_bsize
#define bmax globals->g_bmax
#define rsize globals->g_rsize
#define rmax globals->g_rmax
#define pmax globals->g_pmax
#define CircMaskTable globals->g_CircMaskTable
#define rdcprobes globals->g_rdcprobes
#define rdchits globals->g_rdchits
#define pthprobes globals->g_pthprobes
#define pthhits globals->g_pthhits

#endif (OS != os_mpw)

#define	blimit	(graphicsStatics->uCache._blimit)

private procedure FreePath(pc) register PUCache pc; {
  register Card16 tableIndex = TABLEINDEX(pc->hash);
  /* remove from table */
  if (UCache[tableIndex] == pc) UCache[tableIndex] = pc->next;
  else {
    register PUCache prev = UCache[tableIndex], next;
    while (true) {
      next = prev->next;
      if (next == pc) break;
      prev = next;
      }
    prev->next = pc->next;
    }
  if (pc->bodyvals) FREE(pc->bodyvals);
  if (pc->datastr) FREE(pc->datastr);
  if (pc->ctrlstr) FREE(pc->ctrlstr);
  Assert(pc->prdc == NULL);
  bsize -= pc->size;
  os_freeelement(upcStorage, (char *)pc);
  }

public boolean FlushCircle(mask) PMask mask; {
  register integer i;
  register CircMask *cm = CircMaskTable;
  if(FlushStrokeCircle(mask))
  	return true;
  for (i = 0; i < CIRCMASKMAX; i++) {
    if (cm->mask == mask) {
      cm->flushed = true;
      return true;
      }
    cm++;
    }
  return false;
  }

public procedure FreeStrokeCircle(mask, flushed) PMask mask; boolean flushed; {
  register integer i;
  register CircMask *cm = CircMaskTable;
  for (i = 0; i < CIRCMASKMAX; i++) {
    if (cm->mask == mask) {
 		cm->flushed = flushed;
      	return; /* found the mask in CircMaskTable */
    }
    cm++;
  }
  if (flushed) /* no references to mask in CircMaskTable, ok to flush */
      DevFlushMask(mask, NIL);
}

private procedure FreeCircles(strk) PStrk strk; {
  register integer maskcount = strk->maskcount;
  register integer circ;
  register CircMask *cm;
  while (maskcount--) {
    circ = strk->circles[maskcount];
    Assert(circ < CIRCMASKMAX);
    cm = CircMaskTable + circ;
    Assert((cm->refcnt) > 0 && (cm->mask != NULL));
    cm->refcnt--;
    if (cm->refcnt == 0) {
      if (cm->flushed)
        DevFlushMask(cm->mask, NIL);
      cm->mask = NULL;
      }
    }
  }

private procedure FreeRdc(prdc, lockpc) register PRdc prdc; PUCache lockpc; {
  register PUCache pc;
  /* remove from path list */
  pc = prdc->pth;
  if (pc->prdc == prdc) {
    pc->prdc = prdc->next;
    if (pc->prdc == NULL && pc != lockpc) FreePath(pc);
    }
  else {
    register PRdc prev = pc->prdc, nxt;
    while (true) {
      nxt = prev->next;
      if (nxt == prdc) break;
      prev = nxt;
      }
    prev->next = prdc->next;
    }
  /* remove from lru list */
  { register PRdc prev, nxt;
    prev = prdc->lruprev; nxt = prdc->lrunext;
    if (prev) prev->lrunext = nxt;
    if (nxt)  nxt->lruprev = prev;
    if (lruNewest == prdc) lruNewest = nxt;
    if (lruOldest == prdc) lruOldest = prev;
    }
  bsize -= prdc->size;
  rsize--;
  if (prdc->strk) {
    if (prdc->strk->maskcount) FreeCircles(prdc->strk);
    if (prdc->strk->dashArray) FREE(prdc->strk->dashArray);
    os_freeelement(strkStorage, prdc->strk);
    }
  DisposeDevPrim(prdc->dp);
  os_freeelement(rdcStorage, prdc);
  }

private PUCache FindPathInCache(context, tableIndex, hash)
  PUserPathContext context; integer tableIndex; Card32 hash; {
  /* return NULL if cannot find it */
  boolean failed;
  register PUCache pc;
#if STAGE==DEVELOP
  pthprobes++;
#endif STAGE==DEVELOP
  for (pc = UCache[tableIndex]; pc != NULL; pc = pc->next) {
    if (pc->hash != hash ||
        context->encoded != pc->encoded ||
        context->packed  != pc->packed) continue;
    if (context->encoded) {
      register integer len;
      len = pc->len;
      if (context->datastrm.len != len) continue;
      len = pc->ctrllen;
      if (context->ctrllen != len) continue;
      { register string ctrlstr = context->ctrlstr;
        register string cachstr = pc->ctrlstr;
	failed = false;
        while (--len >= 0) {
          if (*ctrlstr++ != *cachstr++) { failed = true; break; }
	  }
        if (failed) continue;
        }
      if (!EqNumStrmCache(&context->datastrm, pc->bodyvals, pc->datastr))
        continue;
      }
    else {
      register PCard32 pcval;
      register string pctype;
      register integer len = pc->len;
      if (len != context->aryObj.length) continue;
      pcval = pc->bodyvals; pctype = pc->datastr; failed = false;
      if (context->packed) {
        Object ao, ob;
	ao = context->aryObj;
	while (--len >= 0) {
          VMCarCdr(&ao, &ob);
          if (ob.type != *pctype++ || ob.val.ival != *pcval++) {
            failed = true; break; }
          }
        }
      else {
        register PObject argument;
	argument = context->aryObj.val.arrayval;
        while (--len >= 0) {
          if (argument->type != *pctype++ || argument->val.ival != *pcval++) {
            failed = true; break; }
	  argument++;
          }
        }
      if (failed) continue;
      }
    /* when get here we have found match for path */
    if (UCache[tableIndex] != pc) { /* move pc to front of table list */
      register PUCache pcprev = UCache[tableIndex], pcnxt;
      while (true) {
        pcnxt = pcprev->next;
	if (pcnxt == pc) break;
	pcprev = pcnxt;
	}
      pcprev->next = pc->next; /* remove pc from list */
      pc->next = UCache[tableIndex];
      UCache[tableIndex] = pc; /* insert pc at front */
      }
#if STAGE==DEVELOP
    pthhits++;
#endif STAGE==DEVELOP
    return pc;
    }
  return NULL;
  }

private PUCache EnterPathInCache(context, tableIndex, hash, dpsize)
  PUserPathContext context; integer tableIndex; Card32 hash, dpsize; {
  register PUCache pc;
  Card16 size, room;
  size = sizeof(UCacheRec);
  if (context->encoded) {
    size += SizeNumStrmForCache(&context->datastrm) + context->ctrllen;
    room = size + sizeof(RdcRec) + dpsize;
    if (size > pmax || room > blimit) return NULL;
    while (bsize + room > bmax && lruOldest) FreeRdc(lruOldest, NULL);
    pc = NULL;
    DURING
    pc = (PUCache)os_newelement(upcStorage);
    pc->len = context->datastrm.len;
    pc->ctrllen = context->ctrllen;
    pc->ctrlstr = NULL; pc->datastr = NULL; pc->bodyvals = NULL;
    pc->ctrlstr = (string)NEW(context->ctrllen, 1);
    os_bcopy(context->ctrlstr, pc->ctrlstr, context->ctrllen);
    CopyNumStrmForCache(&context->datastrm, &pc->bodyvals, &pc->datastr);
    HANDLER {
      if (pc) {
        if (pc->ctrlstr) FREE(pc->ctrlstr);
	if (pc->datastr) FREE(pc->datastr);
	if (pc->bodyvals) FREE(pc->bodyvals);
	os_freeelement(upcStorage, pc);
	}
      RERAISE;
      }
    END_HANDLER;
    }
  else {
    register PCard32 pcval;
    register string pctype;
    register integer len = context->aryObj.length;
    size += len * (1 + sizeof(Card32));
    room = size + sizeof(RdcRec) + dpsize;
    if (size > pmax || room > blimit) return NULL;
    while (bsize + room > bmax && lruOldest) FreeRdc(lruOldest, NULL);
    pc = NULL; pcval = NULL; pctype = NULL;
    DURING
    pc = (PUCache)os_newelement(upcStorage);
    pc->len = len;
    pcval = pc->bodyvals = (PCard32)NEW(len, sizeof(Card32));
    pctype = pc->datastr = (string)NEW(len, 1);
    pc->ctrlstr = NULL;
    HANDLER {
      if (pcval) FREE(pcval);
      if (pctype) FREE(pctype);
      if (pc) os_freeelement(upcStorage, pc);
      RERAISE;
      }
    END_HANDLER;
    if (context->packed) {
      Object ao, ob;
      ao = context->aryObj;
      while (--len >= 0) {
        VMCarCdr(&ao, &ob);
	*pctype++ = (character)(ob.type);
	*pcval++ = ob.val.ival;
        }
      }
    else {
      register PObject argument = context->aryObj.val.arrayval;
      while (--len >= 0) {
        *pctype++ = (character)(argument->type);
        *pcval++ = argument->val.ival;
        argument++;
        }
      }
    }
  pc->encoded = context->encoded;
  pc->packed = context->packed;
  pc->hash = hash;
  pc->prdc = NULL;
  pc->size = size;
  bsize += size;
  pc->next = UCache[tableIndex];
  UCache[tableIndex] = pc;
  return pc;
  }

private Card32 RdcSize(dpsize, fill) Card32 dpsize; boolean fill; {
  Card32 room = dpsize + sizeof(RdcRec);
  if (!fill) {
    room += sizeof(StrkRec);
    room += gs->dashArray.length * sizeof(real);
    }
  return room;
  }

extern PCIItem FindCircleInCache();

public integer EnterMask(mask, pc)
  register PMask mask; PUCache pc; {
  register integer i, j;
  register CircMask *cm;
  cm = CircMaskTable; j = CIRCMASKMAX;
  for (i = 0; i < CIRCMASKMAX; i++) {
    if (cm->mask == mask) {
      cm->refcnt++; return i;
      }
    if (cm->mask == NULL) j = i;
    cm++;
    }
  while (true) {
    if (j < CIRCMASKMAX) {
      cm = CircMaskTable + j;
      cm->refcnt = 1; cm->flushed = false;
      cm->mask = mask; 
      return j;
      }
    Assert(lruOldest != NULL);
    FreeRdc(lruOldest, pc);
    cm = CircMaskTable;
    for (j = 0; j < CIRCMASKMAX; j++) {
      if (cm->mask == NULL) break;
      cm++;
      }
    }
  }

private procedure EnterCirclesInCache(pc, strk, devprim)
  PUCache pc; PStrk strk; register DevPrim *devprim; {
  PMask masks[4];
  register PMask *pmask = masks;
  register integer maskcount = 0, i;
  register integer mask, mask0, mask1, mask2, mask3;
  register DevMask *devmask;
  boolean found;
  while (devprim) {
    switch (devprim->type) {
      case maskType:
        devmask = devprim->value.mask;
	for (i = devprim->items; i--; devmask++) {
          mask = (integer)(devmask->mask); found = false;
	  switch (maskcount) {
            case 4: if (mask == mask3) found = true;
	    case 3: if (mask == mask2) found = true;
	    case 2: if (mask == mask1) found = true;
	    case 1: if (mask == mask0) found = true;
	    default: break;
	    }
          if (!found) {
            pmask[maskcount] = (PMask)mask;
	    switch (maskcount) {
              case 0: mask0 = mask; break;
	      case 1: mask1 = mask; break;
	      case 2: mask2 = mask; break;
	      case 3: mask3 = mask; break;
	      default: CantHappen();
	      }
	    maskcount++;
	    }
	  }
        break;
      default: break;
      }
    devprim = devprim->next;
    }
  for (i = 0; i < maskcount; i++) {
    strk->circles[i] = EnterMask(pmask[i], pc);
    }
  strk->maskcount = maskcount;
  strk->maskID = gs->device->maskID;
  }

private procedure EnterRdcInCache(pc, context, smtx, devprim, dpsize)
  PUCache pc; PUserPathContext context; DevPrim *devprim; Card32 dpsize;
  PMtx smtx; {
  register PRdc prdc;
  boolean fill = context->fill;
  Card32 room = RdcSize(dpsize, fill);
  while ((bsize + room > bmax || rsize >= rmax) && lruOldest)
    FreeRdc(lruOldest, pc);
  prdc = (PRdc)os_newelement(rdcStorage);
  if (fill) prdc->strk = NULL;
  else {
    register PStrk strk = NULL;
    register PGState g = gs;
    integer i;
    DURING
    prdc->strk = strk = (PStrk)os_newelement(strkStorage);
    strk->flatEps = g->flatEps;
    strk->lineWidth = g->lineWidth;
    strk->miterlimit = g->miterlimit;
    strk->lineCap = g->lineCap;
    strk->lineJoin = g->lineJoin;
    strk->strokeAdjust = g->strokeAdjust;
    if (smtx) { strk->ismtx = true; strk->strkmtx = *smtx; }
    else strk->ismtx = false;
    strk->circletraps = context->circletraps;
    strk->dashArray = NULL;
    strk->dashOffset = g->dashOffset;
    strk->dashCnt = i = g->dashArray.length;
    if (i > 0) {
      real *p;
      Object ob, ao;
      strk->dashArray = p = (real *)NEW(i, sizeof(real));
      ao = g->dashArray;
      while (--i >= 0) {
        VMCarCdr(&ao, &ob); PRealValue(ob, p); p++; }
      }
    HANDLER {
      if (strk) {
        if (strk->dashArray) FREE(strk->dashArray);
        os_freeelement(strkStorage, strk);
        }
      os_freeelement(rdcStorage, prdc);
      RERAISE;
      }
    END_HANDLER;
    EnterCirclesInCache(pc, strk, devprim);
    }
  prdc->size = room;
  bsize += room;
  rsize++;
  { register Mtx *mtx = &prdc->mtx;
    register Mtx *gmtx = &gs->matrix;
    mtx->a = gmtx->a;   mtx->b = gmtx->b;
    mtx->c = gmtx->c;   mtx->d = gmtx->d;
    mtx->tx = gmtx->tx; mtx->ty = gmtx->ty;
    }
  prdc->pth = pc;
  prdc->fill = fill;
  prdc->circleAdjust = gs->circleAdjust;
  prdc->evenOdd = context->evenOdd;
  prdc->dp = devprim;
  prdc->next = pc->prdc;
  pc->prdc = prdc;
  prdc->lrunext = lruNewest;
  if (lruNewest) lruNewest->lruprev = prdc;
  lruNewest = prdc;
  if (lruOldest == NULL) lruOldest = prdc;
  prdc->lruprev = NULL;
  }

private PRdc FindRdcInCache(pc, context, smtx)
  PUCache pc; PUserPathContext context; PMtx smtx; {
  /* return NULL if cannot find it */
  register PRdc prdc;
  PRdc prev, nxt;
  boolean fill = context->fill, evenOdd = context->evenOdd;
  boolean circleAdjust = gs->circleAdjust;
  boolean circletraps = context->circletraps;
  register real mtx_a, mtx_b, mtx_c, mtx_d;
#if STAGE==DEVELOP
  rdcprobes++;
#endif STAGE==DEVELOP
  { register Mtx *m = &gs->matrix;
    mtx_a = m->a; mtx_b = m->b; mtx_c = m->c; mtx_d = m->d;
    }
  prev = pc->prdc;
  for (prdc = prev; prdc != NULL; prdc = prdc->next) {
    { register Mtx *m = &prdc->mtx;
      if (m->a != mtx_a || m->b != mtx_b || m->c != mtx_c || m->d != mtx_d)
        continue;
      }
    if (prdc->circleAdjust != circleAdjust) continue;
    if (fill) {
      if (!prdc->fill || prdc->evenOdd != evenOdd) continue;
      }
    else if (prdc->fill) continue;
    else {
      register PStrk strk = prdc->strk;
      { register PGState g = gs;
      if (strk->lineWidth != g->lineWidth ||
          strk->lineCap != g->lineCap ||
	  strk->lineJoin != g->lineJoin ||
	  strk->strokeAdjust != g->strokeAdjust ||
	  strk->flatEps != g->flatEps ||
	  strk->miterlimit != g->miterlimit)
        continue;
      if (strk->dashCnt == 0) { if (g->dashArray.length > 0) continue; }
      else if (g->dashArray.length == 0) continue;
      else if (strk->dashCnt != g->dashArray.length) continue;
      else if (strk->dashOffset != g->dashOffset) continue;
      else if (strk->maskcount && strk->maskID != g->device->maskID) continue;
      else if (strk->circletraps != circletraps &&
               (strk->lineCap == roundCap || strk->lineJoin == roundJoin))
	   continue;
      else {
        integer i;
	Object ob, ao;
	real r, *p;
	boolean equal = true;
	ao = g->dashArray; p = strk->dashArray;
	for (i = 0; i < strk->dashCnt; i++) {
          VMCarCdr(&ao, &ob); PRealValue(ob, &r);
	  if (r != *p++) { equal = false; break; }
	  }
        if (!equal) continue;
        }
      }
      if (!strk->ismtx) { if (smtx) continue; }
      else if (!smtx) continue;
      else {
	register PMtx m = &strk->strkmtx;
	register PMtx s = smtx;
	if (s->a != m->a || s->b != m->b || s->c != m->c || s->d != m->d)
          continue;
        }
      }
    /* if you fall through to here, you've got a match */
    if (prdc != prev) { /* move to front of path list */
      while (true) {
        nxt = prev->next;
	if (nxt == prdc) break;
	prev = nxt;
	}
      prev->next = prdc->next;
      prdc->next = pc->prdc;
      pc->prdc = prdc;
      }
    if (lruNewest != prdc) { /* move to front of lru list */
      prev = prdc->lruprev; nxt = prdc->lrunext;
      if (prev != NULL) prev->lrunext = nxt;
      if (nxt != NULL) nxt->lruprev = prev;
      if (lruOldest == prdc) lruOldest = prev;
      prdc->lrunext = lruNewest;
      if (lruNewest) lruNewest->lruprev = prdc;
      prdc->lruprev = NULL;
      lruNewest = prdc;
      if (lruOldest == NULL) lruOldest = prdc;
      }
#if STAGE==DEVELOP
    rdchits++;
#endif STAGE==DEVELOP
    return prdc;
    }
  return NULL;
  }

private Card32 HashArray(ao) Object ao; {
  register Card32 hash = 0, len = ao.length;
  switch (ao.type) {
    case arrayObj: {
      register PObject argument = ao.val.arrayval;
      register PObject last = argument + (MIN(len,50) - 1);
      argument += 2; /* move beyond the ucache operator */
      while (argument <= last) {
        hash <<= 1; hash += argument->val.ival; argument += 5; }
      break;
      }
    case pkdaryObj: {
      register integer i = (MIN(len,22) - 3);
      Object ob;
      while (i >= 0) {
        VMCarCdr(&ao, &ob); VMCarCdr(&ao, &ob);
        hash <<= 1; hash += ob.val.ival; i -= 2;
        }
      break;
      }
    default: CantHappen();
    }
  hash += len << HASHSHIFT;
  return hash;
  }

private Card32 HashPath(context) register PUserPathContext context; {
  register Card32 hash = 0, len;
  register integer i;
  register string s;
  register PNumStrm ns;
  if (!context->encoded) return HashArray(context->aryObj);
  ns = &context->datastrm;
  if (ns->ao.type != strObj) return HashArray(ns->ao);
  s = ns->str;
  len = ns->bytespernum * ns->len;
  i = MIN(12, (len-1) >> 3);
  while (--i >= 0) { hash <<= 2; hash += *s; s += 7; }
  hash += len << HASHSHIFT;
  return hash;
  }

public procedure TransDevRun(run, dx, dy) DevRun *run; integer dx, dy; {
  if (dy) { run->bounds.y.l += dy; run->bounds.y.g += dy; }
  if (dx) {
    register DevShort lines, pairs;
    register DevShort *buffptr;
    run->bounds.x.l += dx; run->bounds.x.g += dx;
    lines = run->bounds.y.g - run->bounds.y.l;
    buffptr = run->data;
    while (--lines >= 0) {
      pairs = *(buffptr++);
      while (--pairs >= 0) {
        *buffptr += dx; buffptr++;
	*buffptr += dx; buffptr++;
	}
      }
    }
  }

#if DPSXA
public boolean TransDevPrim(dp, cd) register DevPrim *dp; Cd cd; {
  /* return true if have successfully translated */
 register DevShort dx, dy;
 if (RealEq0(cd.x) && RealEq0(cd.y)) return true;
{
  BBoxRec bb;
  PPath clip = &gs->clip;
  DevPrim *pdp;
  RRoundP(&cd.x, &cd.x); RRoundP(&cd.y, &cd.y);
  if((cd.x < minDevShort) || (cd.x > maxDevShort)) return false;
  if((cd.y < minDevShort) || (cd.y > maxDevShort)) return false;
  dx = cd.x; dy = cd.y;
  pdp = dp;
  while(pdp != NULL) {
    if(dx) {
		pdp->xaOffset.x += dx;
		pdp->bounds.x.l += dx; 
		pdp->bounds.x.g += dx; 
	}
    if (dy) { 
		pdp->xaOffset.y += dy;
    	pdp->bounds.y.l += dy; 
		pdp->bounds.y.g += dy; 
	}
	pdp = pdp->next;
  }
  return true;
 }
}
#else /* DPSXA */
public boolean TransDevPrim(dp, cd) register DevPrim *dp; Cd cd; {
  /* return true if have successfully translated */
  register DevShort dx, dy;
  integer i;
  DevBounds bounds;
  if (RealEq0(cd.x) && RealEq0(cd.y)) return true;
  if (os_fabs(cd.x) > fp16k || os_fabs(cd.y) > fp16k) return false;
  RRoundP(&cd.x, &cd.x); RRoundP(&cd.y, &cd.y);
  dx = cd.x; dy = cd.y;
  FullBounds(dp, &bounds);
  if (dx < 0) { if (bounds.x.l + dx < -16000) return false; }
  else if (dx > 0) { if (bounds.x.g + dx > 16000) return false; }
  if (dy < 0) { if (bounds.y.l + dy < -16000) return false; }
  else if (dy > 0) { if (bounds.y.g + dy > 16000) return false; }
  while (dp) {
    if (dx) { dp->bounds.x.l += dx; dp->bounds.x.g += dx; }
    if (dy) { dp->bounds.y.l += dy; dp->bounds.y.g += dy; }
    switch (dp->type) {
      case runType: {
        register DevRun *run;
	run = dp->value.run;
	for (i = dp->items; i--; run++) {
	  TransDevRun(run, dx, dy);
          }
        break; }
      case trapType: {
        register DevTrap *trap;
	register Fixed fdx;
	trap = dp->value.trap;
	fdx = FixInt(dx);
	for (i = dp->items; i--; trap++) {
          if (dy) { trap->y.l += dy; trap->y.g += dy; }
	  if (dx) {
            trap->l.xl += dx; trap->l.xg += dx; trap->l.ix += fdx;
	    trap->g.xl += dx; trap->g.xg += dx; trap->g.ix += fdx;
            }
          }
        break; }
      case maskType: {
        register DevMask *mask = dp->value.mask;
	for (i = dp->items; i--; mask++) {
          mask->dc.x += dx;
	  mask->dc.y += dy;
          }
        break; }
      case noneType:
        break;
      default:
        CantHappen();
      }
    dp = dp->next;
    }
  return true;
  }
#endif DPSXA

private boolean TransRdc(prdc) register PRdc prdc; {
  /* return true if have successfully translated */
  Cd t, d;
  RRoundP(&gs->matrix.tx, &t.x);
  RRoundP(&gs->matrix.ty, &t.y);
  d.x = t.x - prdc->mtx.tx;
  d.y = t.y - prdc->mtx.ty;
  if (!TransDevPrim(prdc->dp, d)) return false;
  prdc->mtx.tx = t.x;
  prdc->mtx.ty = t.y;
  return true;
  }

#if STAGE==DEVELOP
private procedure CheckUCache() {
  integer size, tableIndex, i, pthcnt, rdccnt;
  PUCache pc;
  PRdc prdc, prev;
  /* check lru list */
  prdc = lruNewest; rdccnt = 0; prev = NULL;
  Assert(prdc == NULL || prdc->lruprev == NULL);
  while (prdc) {
    Assert(++rdccnt <= rsize);
    Assert(prdc->lruprev == prev);
    prev = prdc;
    prdc = prdc->lrunext;
    }
  Assert(prev == lruOldest);
  Assert(rdccnt == rsize);
  /* check UCache table entries */
  pthcnt = 0; rdccnt = 0; size = 0;
  for (i = 0; i < UCSZ; i++) {
    pc = UCache[i];
    while (pc) {
      Assert(++pthcnt <= rsize);
      size += pc->size;
      Assert(TABLEINDEX(pc->hash) == i);
      prdc = pc->prdc;
      Assert(prdc); /* must have at least one reduction */
      while (prdc) {
        Assert(prdc->pth == pc);
	Assert(prdc->dp);
	size += prdc->size;
	rdccnt++;
        prdc = prdc->next;
	}
      pc = pc->next;
      }
    }
  Assert(size == bsize);
  Assert(rdccnt == rsize);
  }

private procedure PSFlushUCache() {
  CheckUCache();
  while (lruOldest) FreeRdc(lruOldest, NULL);
  CheckUCache();
  }

private procedure PSNormalUCache() {
  integer blim;
  bmax = ps_getsize(SIZE_UPATH_CACHE, BMAX);
  blim = bmax / 3;
  PSFlushUCache();
  WriteContextParam (&blimit, &blim, sizeof(blimit), NIL);
  rmax = RMAX * bmax;
  rmax /= 1000;
  pmax = PMAX * bmax;
  pmax /= 1000;
  }

private procedure PSTinyUCache() {
  PSNormalUCache();
  bmax = 5000;
  rmax = 8;
  }

private procedure PSUCacheHist() {
  integer i, j;
  PUCache pc;
  for (i = 0; i < UCSZ; i++) {
    j = 0; pc = UCache[i];
    while (pc) { j++; pc = pc->next; }
    PushInteger(j);
    }
  }

#endif STAGE==DEVELOP
#if DPSXA
public DevPrim *XARdc(RdcProc,context)
DevPrim *(*RdcProc)();
PUserPathContext context;
{
DevPrim *context_dp, *dp, *list = NULL;
register int	x,y;
Cd xydelta, delta;
DevCd offset;

	offset.x = 0; offset.y = 0;
	xydelta.x = -xChunkOffset;
	xydelta.y = -yChunkOffset;
	delta.x = xydelta.x;
	delta.y = 0;
	UOffset.x = 0;
	UOffset.y = 0;
	for(y=0; y<maxYChunk; y++) {
		for(x=0; x<=maxXChunk; x++) {
			if(BBCompare(&context->bbox, &chunkBBox) != outside) {
				dp = context_dp = (*RdcProc)(context);
				if((dp != NULL) && ((dp->type != noneType) || (list == NULL))) {
					dp->xaOffset = offset;
					while(dp->next != NULL) {
						dp = dp->next;
						dp->xaOffset = offset;
					}
					/* add DevPrim to list */
					dp->next = list; 						
					list = context_dp;
				}
			}
			offset.x += xChunkOffset;
			UOffset.x = -offset.x;
			TlatBBox(&context->bbox,delta);
		}
		offset.x = 0;
		offset.y += yChunkOffset;
		UOffset.x = -offset.x;
		UOffset.y = -offset.y;
		delta.x = (maxXChunk+1) * -xydelta.x;
		delta.y = xydelta.y;
		TlatBBox(&context->bbox,delta);
		delta.x = xydelta.x;
		delta.y = 0;
	}
	delta.x = 0;
	delta.y = maxYChunk * -xydelta.y;
	TlatBBox(&context->bbox,delta);
	if(list == NULL) {
		/* create a placeholder for a null clip region */
			DevPrim *tmpDP;
  			DURING
  			tmpDP = (DevPrim *)NewDevPrim();
  			HANDLER {
    			if (tmpDP) DisposeDevPrim(tmpDP);
    		RERAISE;
    		}
  			END_HANDLER;
			InitDevPrim(tmpDP, (DevPrim *)NULL);
			list = tmpDP;
	}
	return(list);
} /* end of XARdc */

public DevPrim *XADoRdcPth(context)
PUserPathContext context;
{
  return(DoRdcPth(context->evenOdd, context, &context->bbox,
             UsrPthQRdcOk, FillUserPathEnumerate, QEnumOk, QFillUserPathEnumerate));
}

private DevPrim *XADoRdcStroke(context)
PUserPathContext context;
{
  uXAc1.x += UOffset.x;
  uXAc1.y += UOffset.y;
  uXAc2.x += UOffset.x;
  uXAc2.y += UOffset.y;
  return(DoRdcStroke(context, &context->bbox, uXARectangle, uXAc1, uXAc2,
              StrokeUserPathEnumerate, QEnumOk, QStrokeUserPathEnumerate,
	      	  context->circletraps));
}
#endif /* DPSXA */

public DevPrim * UCGetDevPrim(context, smtx)
  register PUserPathContext context; PMtx smtx; {
  DevPrim *dp = NULL, *newdp = NULL;
  PUCache pc = NULL;
  PRdc prdc = NULL;
  Card32 hash, dpsize;
  integer tableIndex;
  boolean cannot;
  real tx, ty;
  UserPathContext localctx;
  hash = HashPath(context);
  tableIndex = TABLEINDEX(hash);
  pc = FindPathInCache(context, tableIndex, hash);
  if (pc) prdc = FindRdcInCache(pc, context, smtx);
  if (prdc == NULL) { /* try to enter in cache */
    localctx = *context;
    UsrPthBBox(&localctx);
    { register BBox bbox = &localctx.bbox;
      if (bbox->bl.x < -fp16k ||
          bbox->bl.y < -fp16k ||
	  bbox->tr.x >  fp16k ||
	  bbox->tr.y >  fp16k) return NULL;
      }
    { register Mtx *gmtx = &gs->matrix;
      tx = gmtx->tx; ty = gmtx->ty;
      gmtx->tx = localctx.matrix.tx; /* rounded versions of tx and ty */
      gmtx->ty = localctx.matrix.ty;
      }
    DURING
    cannot = false;
    if (localctx.fill)
#if DPSXA
      dp = XARdc(XADoRdcPth,&localctx);
#else /* DPSXA */
      dp = DoRdcPth(localctx.evenOdd, &localctx, &localctx.bbox,
                UsrPthQRdcOk, FillUserPathEnumerate,
                QEnumOk, QFillUserPathEnumerate);
#endif /* DPSXA */
    else {
      boolean rectangle;
      Cd c1, c2;
      Mtx gMtx;
      if (localctx.encoded)
        rectangle = EUsrPthCheckMtLt(&localctx.aryObj, &c1, &c2);
      else
        rectangle = UsrPthCheckMtLt(&localctx.aryObj, &c1, &c2);
      if (smtx) { gMtx = gs->matrix; Cnct(smtx); }
      DURING
#if DPSXA
      uXARectangle = rectangle;
      uXAc1 = c1;
      uXAc2 = c2;
      dp = XARdc(XADoRdcStroke, &localctx);
#else /* DPSXA */
      dp = DoRdcStroke(&localctx, &localctx.bbox, rectangle, c1, c2,
              StrokeUserPathEnumerate, QEnumOk, QStrokeUserPathEnumerate,
	      localctx.circletraps);
#endif /* DPSXA */
      HANDLER { if (smtx) SetMtx(&gMtx); RERAISE; }
      END_HANDLER;
      if (smtx) SetMtx(&gMtx);
      }
    dpsize = DevPrimBytes(dp);
#if STAGE==DEVELOP
    CheckUCache();
#endif STAGE==DEVELOP
    if (pc == NULL)
      pc = EnterPathInCache(context, tableIndex, hash, dpsize);
    if (pc == NULL || pc->size + RdcSize(dpsize, localctx.fill) > blimit) {
      context->dispose = true; cannot = true;
      if (pc != NULL && pc->prdc == NULL) FreePath(pc);
      }
    else {
      newdp = CopyDevPrim(dp); /* make minimum size copy */
      DisposeDevPrim(dp); /* get rid of the original */
      dp = newdp; newdp = NULL;
      EnterRdcInCache(pc, &localctx, smtx, dp, dpsize);
#if STAGE==DEVELOP 
      CheckUCache();
#endif STAGE==DEVELOP
      }
    HANDLER {
      if (pc != NULL && pc->prdc == NULL) FreePath(pc);
      gs->matrix.tx = tx; gs->matrix.ty = ty;
      if (dp) DisposeDevPrim(dp);
      if (newdp) DisposeDevPrim(newdp);
      SetAbort(0);
      return NULL;
      }
    END_HANDLER;
    if (cannot) return dp;
    gs->matrix.tx = tx; gs->matrix.ty = ty;
    }
  else if (!TransRdc(prdc))
    return NULL;
  else dp = prdc->dp;
  context->dispose = false;
  return dp;
  }

public boolean UCacheMark(context, mtx)
  PUserPathContext context; PMtx mtx; {
  register DevPrim *dp;
  dp = UCGetDevPrim(context, mtx);
  if (dp == NULL)
    return false;
  if (dp->type != noneType)
    MarkDevPrim(dp, GetDevClipPrim());
  if (context->dispose)
    DisposeDevPrim(dp);
  return true;
  }

public procedure PSUCacheStatus() {
  PSMark();
  PushInteger(bsize);
  PushInteger(bmax);
  PushInteger(rsize);
  PushInteger(rmax);
  PushInteger(blimit);
  }

public procedure PSSetUCacheParams() {
integer blim,n;
integer blimmax = bmax * BLIMIT;
IntObj ob;
  n = CountToMark(opStk);
  if(n>0) {
    blimmax /= 1000;
    blim = PopInteger();
    if (blim < 0) blim = 0;
    if (blim > blimmax) blim = blimmax;
    WriteContextParam (&blimit, &blim, sizeof(blimit), NIL);
    n--;
  }
  PSClrToMrk();
  }

public procedure UCacheDataHandler (code)
  StaticEvent code;
{
  switch (code) {
   case staticsCreate:
     blimit = bmax * BLIMIT;
     blimit /= 1000;
     break;
  }
}

#if STAGE==DEVELOP
private readonly RgCmdTable debugCmdDPSUCache = {
  "flushucache", PSFlushUCache,
  "checkucache", CheckUCache,
  "tinyucache", PSTinyUCache,
  "normalucache", PSNormalUCache,
  "ucachehist", PSUCacheHist,
  NIL};
#endif STAGE==DEVELOP

public procedure IniUCache(reason)  InitReason reason;
{
integer i;
switch (reason)
  {
  case init:
#if (OS == os_mpw)
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    upcStorage = os_newpool(sizeof(UCacheRec),5,0);
    rdcStorage = os_newpool(sizeof(RdcRec),5,0);
    strkStorage = os_newpool(sizeof(StrkRec),5,0);
    UCache = (PUCache *)os_sureMalloc(UCSZ * sizeof(PUCache));
    CircMaskTable = (CircMask *)os_sureMalloc(CIRCMASKMAX * sizeof(CircMask));
    bmax = ps_getsize(SIZE_UPATH_CACHE, BMAX);
    rmax = RMAX * bmax;
    rmax /= 1000;
    pmax = PMAX * bmax;
    pmax /= 1000;
    bsize = 0;
    rsize = 0;
    for (i = 0; i < UCSZ; i++) UCache[i] = NULL;
    for (i = 0; i < CIRCMASKMAX; i++) CircMaskTable[i].mask = NULL;
    lruNewest = lruOldest = NULL;
#if STAGE==DEVELOP
    pthprobes = pthhits = rdcprobes = rdchits = 0;
#endif STAGE==DEVELOP
    break;
  case romreg:
#if STAGE==DEVELOP
    if (vSTAGE == DEVELOP)
      RgstMCmds(debugCmdDPSUCache);
#endif STAGE==DEVELOP
    break;
  }
}
