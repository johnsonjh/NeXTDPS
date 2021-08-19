/*
  gray.c

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

Original version: Doug Brotz: August 8, 1983
Edit History:
Larry Baer: Tue Nov 21 14:26:21 1989
Doug Brotz: Fri Jan  9 14:29:55 1987
Chuck Geschke: Sun Oct 27 12:25:31 1985
Ed Taft: Thu Jul 28 15:41:15 1988
Bill Paxton: Wed Jun  1 09:54:26 1988
Don Andrews: Mon Jan 13 15:43:32 1986
Ivor Durham: Wed May 10 21:25:53 1989
Linda Gass: Thu Jun 11 10:07:10 1987
Jim Sandman: Wed Dec 13 09:09:16 1989
Paul Rovner: Fri Dec 29 11:18:28 1989
Joe Pasqua: Tue Feb 28 11:00:45 1989
Terry Donahue: Tue Jul 21 0:00:45 1990
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include DEVICE
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include FP
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include SIZES
#include RECYCLER
#include VM

#include "graphdata.h"
#include "gray.h"
#include "graphicspriv.h"
#include "graphicsnames.h"

public integer grayPatternLimit;

#define DEF_PAT_SIZE 2044

/* private global variables. make change in both arms if changing variables */
#if (OS != os_mpw)

private Screen screenList;
private PGrayQ defaultPatBase;
private boolean defaultInUse;
private Pool screenPool;

#else (OS != os_mpw)

typedef struct {
  Screen g_screenList;
  PGrayQ g_defaultPatBase;
  boolean g_defaultInUse;
  Pool g_screenPool;
  } GlobalsRec, *Globals;
  
private Globals globals;

#define screenList globals->g_screenList
#define defaultPatBase globals->g_defaultPatBase
#define defaultInUse globals->g_defaultInUse
#define screenPool globals->g_screenPool

#endif (OS != os_mpw)

private Screen NewScreen()
  {
  register Screen screen;
  if (!screenPool)
      screenPool = os_newpool(sizeof(ScreenRec),5,0);
  screen = (Screen)os_newelement(screenPool);
  if (screen == NIL) LimitCheck();
  os_bzero((char *)screen, (long int)sizeof(ScreenRec));
  return screen;
  } /* end of NewScreen */


private procedure DeleteScreen(screen) register Screen screen; {
  integer i;
  if (screen->halftone != NULL) DevFreeHalftone(screen->halftone);
  if ((gs != NIL) && (gs->screen == screen)) gs->screen = NIL;
  os_freeelement(screenPool, (char *)screen);
  } /* end of DeleteScreen */


public integer GCD(u, v)  integer u, v;
{integer t; while (v != 0) {t = u % v; u = v; v = t;} return u;}

public procedure AddScrRef(screen) Screen screen; {
  if (screen != NIL) screen->ref++;}


public procedure RemScrRef(screen) Screen screen; {
  if (screen == NIL) return;
  if ((--screen->ref) == 0) {
    if (screenList == screen) screenList = screen->next;
    else {
      Screen prev = screenList;
      for (;;) {
	if (prev == NIL) break;
	if (prev->next == screen) {
	  prev->next = screen->next; break; }
	prev = prev->next;
	}
      }
    DeleteScreen(screen);
    }
  } /* end of RemScrRef */
  

private Screen MakeType1Screen(dict, pSpot)
  DictObj dict; PSpotFunction pSpot; {
  register Screen screen;
  DevScreen thresholds;
  integer dims[2];
  screen = NewScreen();
  screen->type = 1;
  screen->dict = dict;
  GetValidFreqAnglePair(pSpot, dims);
  screen->halftone = DevAllocHalftone(dims[0], dims[1], 0, 0, 0, 0, 0, 0);
  if (screen->halftone == NULL) LimitCheck();
  screen->fcns[colorIndexGray] = *pSpot;
  DURING {
    GenerateThresholds(pSpot, screen->halftone->white);
    }
  HANDLER {
    DeleteScreen(screen); RERAISE;}
  END_HANDLER;
  screen->haveThresholds = true;
  return screen;
  } /* end of MakeType1Screen */


private Screen MakeType2Screen(dict, fcns)
  DictObj dict; PSpotFunction fcns; {
  integer i;
  integer dims[8];
  DevHalftone *ht;
  register Screen screen;
  screen = NewScreen();
  screen->type = 2;
  screen->dict = dict;
  GetValidFreqAngleOctet(fcns, dims);
  screen->halftone = ht = DevAllocHalftone(
    dims[colorIndexGray*2], dims[colorIndexGray*2 + 1],
    dims[colorIndexRed*2], dims[colorIndexRed*2 + 1],
    dims[colorIndexGreen*2], dims[colorIndexGreen*2 + 1],
    dims[colorIndexBlue*2], dims[colorIndexBlue*2 + 1]);
  if (screen->halftone == NULL) LimitCheck();
  DURING {
    for (i = 0; i < 4; i++) 
      screen->fcns[i] = fcns[i];
    GenerateThresholds(&screen->fcns[colorIndexRed], ht->red);
    GenerateThresholds(&screen->fcns[colorIndexGreen], ht->green);
    GenerateThresholds(&screen->fcns[colorIndexBlue], ht->blue);
    GenerateThresholds(&screen->fcns[colorIndexGray], ht->white);
    }
  HANDLER {
    DeleteScreen(screen); RERAISE;}
  END_HANDLER;
  screen->haveThresholds = true;
  return screen;
  } /* end of MakeType2Screen */


private procedure GetSpotDictEntries(dict, names, pSpot)
  DictObj dict; PNameObj names; PSpotFunction pSpot; {
  Object ob;
  if (DictTestP(dict, *names, &ob, true)) {
    switch (ob.type) {
      case intObj: {pSpot->angle = ob.val.ival; break; }
      case realObj: {pSpot->angle = ob.val.rval; break; }
      default: TypeCheck();
      }
    }
  else {TypeCheck(); return; }
  names++;
  if (DictTestP(dict, *names, &ob, true)) {
    switch (ob.type) {
      case intObj: {pSpot->freq = ob.val.ival; break; }
      case realObj: {pSpot->freq = ob.val.rval; break; }
      default: TypeCheck();
      }
    }
  else TypeCheck();
  names++;
  if (!DictTestP(dict, *names, &pSpot->ob, true)) 
    TypeCheck();
  } /* end of GetSpotDictEntries */


private Screen GetType1Screen(dict) DictObj dict; {
  SpotFunction spotFcn;
  GetSpotDictEntries(dict, &graphicsNames[nm_Angle], &spotFcn);
  return MakeType1Screen(dict, &spotFcn);
  } /* end of GetType1Screen */


private Screen GetType2Screen(dict) DictObj dict; {
  SpotFunction fcns[4];
  GetSpotDictEntries(dict, &graphicsNames[nm_GrayAngle], &fcns[colorIndexGray]);
  GetSpotDictEntries(dict, &graphicsNames[nm_RedAngle], &fcns[colorIndexRed]);
  GetSpotDictEntries(dict, &graphicsNames[nm_GreenAngle], &fcns[colorIndexGreen]);
  GetSpotDictEntries(dict, &graphicsNames[nm_BlueAngle], &fcns[colorIndexBlue]);
  return MakeType2Screen(dict, fcns);
  } /* end of GetType1Screen */


private procedure GetThresholdDictEntries(dict, names, pDevS)
  DictObj dict; PNameObj names; DevScreen *pDevS; {
  Object ob;
  if (DictTestP(dict, *names, &ob, true)) {
    switch (ob.type) {
      case intObj: {pDevS->width = ob.val.ival; break; }
      default: TypeCheck();
      }
    }
  else {TypeCheck(); return; }
  names++;
  if (DictTestP(dict, *names, &ob, true)) {
    switch (ob.type) {
      case intObj: {pDevS->height = ob.val.ival; break; }
      default: TypeCheck();
      }
    }
  else TypeCheck();
  if (pDevS->width == 0 || pDevS->height == 0) RangeCheck();
  names++;
  if (DictTestP(dict, *names, &ob, true)) {
    switch (ob.type) {
      case strObj: {
        if (ob.length != pDevS->width * pDevS->height) {
          RangeCheck();
          }
        pDevS->thresholds = ob.val.strval;
        break;
        }
      default: TypeCheck();
      }
    }
  else TypeCheck();
  } /* end of GetThresholdDictEntries */


private procedure CopyThresholds(to, from) DevScreen *to, *from; {
  unsigned char *p1, *p2;
  integer i;
  p1 = to->thresholds;
  p2 = from->thresholds;
  for (i = to->width * to->height; i > 0; i--)
    *p1++ = *p2++;
  return;
  }

private Screen GetType3Screen(dict) DictObj dict; {
  register Screen screen;
  DevScreen ds;
  GetThresholdDictEntries(dict, &graphicsNames[nm_Width], &ds);
  if (!(DevCheckScreenDims(ds.width, ds.height)))
    LimitCheck();
  screen = NewScreen();
  screen->type = 3;
  screen->dict = dict;
  screen->halftone = DevAllocHalftone(ds.width, ds.height, 0, 0, 0, 0, 0, 0);
  CopyThresholds(screen->halftone->white, &ds);
  screen->haveThresholds = true;
  return screen;
  } /* end of GetType3Screen */


private Screen GetType4Screen(dict) DictObj dict; {
  register Screen screen;
  integer i;
  Object ob;
  DevScreen ds[4];
  GetThresholdDictEntries(
    dict, &graphicsNames[nm_RedWidth], &ds[colorIndexRed]);
  GetThresholdDictEntries(
    dict, &graphicsNames[nm_GreenWidth], &ds[colorIndexGreen]);
  GetThresholdDictEntries(
    dict, &graphicsNames[nm_BlueWidth], &ds[colorIndexBlue]);
  GetThresholdDictEntries(
    dict, &graphicsNames[nm_GrayWidth], &ds[colorIndexGray]);
  if (!DevCheckScreenDims(
    (DevShort)LCM4(
      (integer)ds[colorIndexRed].width,
      (integer)ds[colorIndexGreen].width,
      (integer)ds[colorIndexBlue].width,
      (integer)ds[colorIndexGray].width),
    (DevShort)LCM4(
      (integer)ds[colorIndexRed].height,
      (integer)ds[colorIndexGreen].height,
      (integer)ds[colorIndexBlue].height,
      (integer)ds[colorIndexGray].height)))
    LimitCheck();
  screen = NewScreen();
  screen->type = 4;
  screen->dict = dict;
  screen->halftone = DevAllocHalftone(
    ds[colorIndexGray].width, ds[colorIndexGray].height,
    ds[colorIndexRed].width, ds[colorIndexRed].height,
    ds[colorIndexGreen].width, ds[colorIndexGreen].height,
    ds[colorIndexBlue].width, ds[colorIndexBlue].height);
  CopyThresholds(screen->halftone->white, &ds[colorIndexGray]);
  CopyThresholds(screen->halftone->red, &ds[colorIndexRed]);
  CopyThresholds(screen->halftone->green, &ds[colorIndexGreen]);
  CopyThresholds(screen->halftone->blue, &ds[colorIndexBlue]);
  screen->haveThresholds = true;
  return screen;
  } /* end of GetType3Screen */


private procedure InstallNewScreen(screen) register Screen screen; {
  Screen existingScreen;
  screen->next = screenList;
  screenList = screen;
  if (screen != gs->screen) {
    RemScrRef(gs->screen);
    gs->screen = screen;
    screen->ref++;
    }
  }

public procedure PSSetHalftone();

public procedure PSSetScreen() {
  register Screen screen;
  SpotFunction spot;
  Object ob;
  PopP(&spot.ob);
  PopPReal(&spot.angle);
  PopPReal(&spot.freq);
  if ((spot.ob.type == arrayObj || spot.ob.type == pkdaryObj) &&
    spot.ob.length == 0)
    return;
  if (spot.ob.type == dictObj) {
    PushP(&spot.ob);
    PSSetHalftone();
    return;
    }
  ConditionalInvalidateRecycler (&spot.ob);
  LNullObj(ob);
  screen = MakeType1Screen(ob, &spot);
  InstallNewScreen(screen);
  } /* end of PSSetScreen */


public procedure PSSetColorScreen() {
  register Screen screen;
  integer i;
  SpotFunction fcns[4];
  Object ob;
  boolean recycleTest;
  PopP(&fcns[colorIndexGray].ob);
  ConditionalInvalidateRecycler (&fcns[colorIndexGray].ob);
  PopPReal(&fcns[colorIndexGray].angle);
  PopPReal(&fcns[colorIndexGray].freq);
  PopP(&fcns[colorIndexBlue].ob);
  ConditionalInvalidateRecycler (&fcns[colorIndexBlue].ob);
  PopPReal(&fcns[colorIndexBlue].angle);
  PopPReal(&fcns[colorIndexBlue].freq);
  PopP(&fcns[colorIndexGreen].ob);
  ConditionalInvalidateRecycler (&fcns[colorIndexGreen].ob);
  PopPReal(&fcns[colorIndexGreen].angle);
  PopPReal(&fcns[colorIndexGreen].freq);
  PopP(&fcns[colorIndexRed].ob);
  ConditionalInvalidateRecycler (&fcns[colorIndexRed].ob);
  PopPReal(&fcns[colorIndexRed].angle);
  PopPReal(&fcns[colorIndexRed].freq);
  if ((fcns[colorIndexGray].ob.type == arrayObj || fcns[colorIndexGray].ob.type == pkdaryObj) &&
    fcns[colorIndexGray].ob.length == 0)
    return;
  if  (fcns[colorIndexGray].ob.type == dictObj) {
    PushP(&fcns[colorIndexGray].ob);
    PSSetHalftone();
    return;
    }
  LNullObj(ob);
  screen = MakeType2Screen(ob, fcns);
  InstallNewScreen(screen);
  } /* end of PSSetColorScreen */


public procedure SetDefaultHalftone() {
  DevHalftone *halftone = (*gs->device->procs->DefaultHalftone)(gs->device);
  register Screen s = screenList;
  integer i;
  while (s != NIL) {
    if (s->halftone == halftone) {
      if (s != gs->screen) {
	RemScrRef(gs->screen);
	gs->screen = s;
	s->ref++;
	}
      return;
      }
    s = s->next;
    }
  s = NewScreen();
  s->halftone = halftone;
  s->type = (
    halftone->red != halftone->green && halftone->red != halftone->blue &&
    halftone->red != halftone->white) ? 4 : 3;
  s->dict = iLNullObj;
  s->haveThresholds = true;
  s->devHalftone = true;
  InstallNewScreen(s);
  }

public procedure PSSetHalftone() {
  DictObj dict;
  Object ob;
  register Screen screen;
  PopP(&dict);
  if (dict.type == nullObj) {
    SetDefaultHalftone();
    return;
    }
  else if (dict.type != dictObj) {
    TypeCheck(); return; }
  if (!DictTestP(dict, graphicsNames[nm_HalftoneType], &ob, true) ||
      ob.type != intObj) { TypeCheck(); return; }
  switch (ob.val.ival) {
    case 1: {screen = GetType1Screen(dict); break;}
    case 2: {screen = GetType2Screen(dict); break;}
    case 3: {screen = GetType3Screen(dict); break;}
    case 4: {screen = GetType4Screen(dict); break;}
    default: RangeCheck(); return;
    }
  InstallNewScreen(screen);
  } /* end of PSSetHalftone */


private procedure PutThresholdEntries(dict, names, devS) 
  DictObj dict; PNameObj names; DevScreen *devS; {
  IntObj val;
  StrObj str;
  LIntObj(val, devS->width);
  DictPut(dict, *names++, val);
  LIntObj(val, devS->height);
  DictPut(dict, *names++, val);
  AllocPString((cardinal)(devS->width * devS->height), &str);
  ConditionalInvalidateRecycler (&str);
  DictPut(dict, *names++, str);
  os_bcopy(
    (char *)devS->thresholds, (char *)str.val.strval,
    (long int)(devS->width * devS->height));
  return;
  }

private procedure MakeScreenDict() {
  register Screen s = gs->screen;
  if (s->devHalftone && s->dict.type == nullObj) {
    boolean origShared = CurrentShared();
    SetShared(true);
    DURING {
      StrObj str;
      IntObj type;
      DictP((cardinal)(s->type == 3 ? 4 : 16), &s->dict);
      LIntObj(type, s->type);
      DictPut(s->dict, graphicsNames[nm_HalftoneType], type);
      if (s->type == 3)
        PutThresholdEntries(
          s->dict, &graphicsNames[nm_Width], s->halftone->white);
      else {
        PutThresholdEntries(
          s->dict, &graphicsNames[nm_RedWidth], s->halftone->red);
        PutThresholdEntries(
          s->dict, &graphicsNames[nm_GreenWidth], s->halftone->green);
        PutThresholdEntries(
          s->dict, &graphicsNames[nm_BlueWidth], s->halftone->blue);
        PutThresholdEntries(
          s->dict, &graphicsNames[nm_GrayWidth], s->halftone->white);
        }
      }
    HANDLER {
      SetShared(origShared); RERAISE;
      }
    END_HANDLER;
    SetShared(origShared);
    }
  } /* end of MakeScreenDict */


public procedure PSCrHalftone() {
  register Screen s = gs->screen;
  if (s == NIL) {
    SetDefaultHalftone();
    s = gs->screen;
    }
  if (s->devHalftone && s->dict.type == nullObj)
    MakeScreenDict();
  PushP(&(s->dict));
  } /* end of PSCrHalftone */

private procedure PushFakeFreqAngle() {
  PushInteger((integer)60);
  PushInteger((integer)0);
  }

private procedure Push3NullScreens() {
  Object ob;
  integer i;
  XAryObj(ob, 0, 0);
  for (i = 0; i < 3; i++) {
    PushFakeFreqAngle();
    PushP(&ob);
    }
  }

private procedure PushSpotFunction(pSpot) PSpotFunction pSpot; {
  PushPReal(&pSpot->freq);
  PushPReal(&pSpot->angle);
  PushP(&pSpot->ob);
  }

public procedure PSCrScreen() {
  register Screen s = gs->screen;
  if (s == NIL || s->dict.type == dictObj || s->type > 2) {
    PushFakeFreqAngle(); PSCrHalftone();
    }
  else
    PushSpotFunction(&s->fcns[colorIndexGray]);
  }  /* end of PSCrScreen */


public procedure PSCrColorScreen() {
  register Screen s = gs->screen;
  integer i;
  if (s == NIL || s->dict.type == dictObj) {
    Push3NullScreens();
    PushFakeFreqAngle();
    PSCrHalftone();
    }
  else if (s->type == 1) {
    Push3NullScreens();
    PushSpotFunction(&s->fcns[colorIndexGray]);
    }
  else {
    for (i = 0; i < 4; i++) {
      PushSpotFunction(&s->fcns[i]);
      }
    }
  }  /* end of PSCrColorScreen */




public PGrayQ GetPatternBase(area) cardinal area; {
  PGrayQ pb;
  if (!defaultInUse && area <= DEF_PAT_SIZE) {
    if (!defaultPatBase)
      defaultPatBase = (PGrayQ)NEW(DEF_PAT_SIZE, sizeof(GrayQ));
    defaultInUse = true;
    return defaultPatBase;
    }
  pb = (PGrayQ)NEW(area, sizeof(GrayQ));	/* May raise LimitCheck */
  return pb;
  };
  
public procedure FreePatternBase(pb) PGrayQ pb; {
  if (pb == defaultPatBase) defaultInUse = false;
  else FREE(pb);
  };
  
  

public procedure GrayInit(reason)  InitReason reason; {
  integer i;
  switch (reason)
    {
    case init:
#if (OS == os_mpw)
      globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
      screenList = NIL;
      grayPatternLimit = ps_getsize(SIZE_MAX_HALFTONE_PATTERN, 8188);
      defaultInUse = false;
      break;
    case romreg:
      break;
    endswitch}
  } /* end of GrayInit */
