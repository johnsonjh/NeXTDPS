/*
  fontlist.c

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Nash, Sandman, Sweet: 1987
Edit History:
John Nash: Tue Apr 14 13:17:12 PDT 1987
Jim Sandman: Wed Apr 20 16:59:21 1988
Dick Sweet: Thu Apr 9 09:48:49 PST 1987
Ed Taft: Fri Jul 29 09:34:47 1988
Ivor Durham: Sun Aug 14 14:00:37 1988
End Edit History.
*/
#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include GRAPHICS
#include LANGUAGE
#include VM

#include "fontcache.h"
#include "fontsnames.h"

extern integer listCacheKey;

private boolean FoundFont (fid, fontNameP) FID fid; PNameObj fontNameP; {
  PDictBody FDdp; FontObj fobj;
  register PKeyVal FDkvp; DictObj FD;
  GetFontDirectory (&FD);
  FDdp = FD.val.dictval;
  for (FDkvp = FDdp->begin; FDkvp != FDdp->end; FDkvp++)
    {
    if ((FDkvp->key.type != nullObj) && (FDkvp->value.type == dictObj))
      {
      DictGetP(FDkvp->value, fontsNames[nm_FID], &fobj);
      if (fid == (FID)(fobj.val.fontval))
        {*fontNameP = FDkvp->key; return true;}
      }
    }
  return false;
  }
  
private procedure PSListFontCache () {
  register MID m; register PMTItem mp;
  register PCIItem cip; 
  register CIOffset cio;
  Object fontProc, charProc;
  AryObj array;
  Mtx mtx1, mtx2;
  NameObj fontName, name;
  integer key;
  boolean abort = false;
  key = PopInteger();
  if (key != listCacheKey) {
    InvlAccess();
    return;
    }
  PopP(&charProc);
  PopP(&fontProc);
  PopP(&array);
  m = MSEldest;
  while (!abort) {
    mp = &MT[m];
    if ((MIDRefCnt[m] & MIDNotPurgedFlag) != 0 &&
	FoundFont(mp->fid, &fontName)) {
      PushP(&fontName);
      DfMtx(&mtx1);
      mtx1.tx = mtx1.ty = fpZero;
      MtxInvert(&mtx1, &mtx2);
      MtxCnct(&mp->mtx, &mtx2, &mtx1);
      PushPMtx(&array, &mtx1);
      abort = psExecute(fontProc);
      cio = 0;
      forallCI(cip){
	if (abort) break;
	cio++;
	if ((cip->mid == m) && ((cip->type & VDmem) != 0)) {
	  LNameObj(name, CN[cio]);
	  PushP(&name);
	  abort = psExecute(charProc);
	  }
	}
      }
    m = mp->younger;
    if (m == MSEldest) break;
    }
  }

public procedure FontListInit(reason)
	InitReason reason;
{
  switch (reason){
    case ramreg: {
      Begin(rootShared->trickyDicts.val.arrayval[tdStatusDict]);
      if (!MAKEVM) RgstExplicit("listfontcache",PSListFontCache);
      End();
      break;}
    endswitch}
}
