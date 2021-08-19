/* PostScript generic device module:

   Ancestor of all PostScript devices that build character masks in a
   rectangular pixel map in memory.

Copyright (c) 1983, '84, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

*/

#include PACKAGE_SPECS
#include DEVICE
#include DEVPATTERN

#include "devcommon.h"
#include "genericdev.h"
#include "nulldev.h"
#include "framemaskdev.h"

public DevProcs *genProcs;

private procedure GenDefaultMtx (device, matrix) PDevice device; PMtx matrix; {
  PGenStuff genStuff = (PGenStuff) device;
  *matrix = genStuff->matrix;
  }

private procedure GenDefaultBounds (device, bBox)
  PDevice device; DevLBounds *bBox; {
  PGenStuff genStuff = (PGenStuff) device;
  *bBox = genStuff->bbox;
  }

private procedure GenWinToDevTranslation (device, translation)
  PDevice device; DevPoint *translation; {
  DevLBounds bbox;
  (device->procs->DefaultBounds)(device,&bbox);
  translation->x = bbox.x.l;
  translation->y = bbox.y.l;
  }

private DevHalftone * GenDefaultHalftone (device) PDevice device; {
  return defaultHalftone;
  }

private procedure GenGoAway (device) PDevice device; {
  os_free((char *)device);
  }

private procedure GenWakeup (device) PDevice device; {
  CurrentDevice = device;
  }

private procedure GenInitPage (device, c) PDevice device; DevColor c; {
  DevMarkInfo info;
  DevTrap trap;
  DevPrim prim;
  DevLBounds bbox;

  info.offset.x = 0;
  info.offset.y = 0;
  info.halftone = NIL;
  info.color = c;
  info.screenphase.x = info.screenphase.y = 0;
  info.priv = NIL;

  (device->procs->DefaultBounds)(device,&bbox);
  trap.y.l = bbox.y.l;
  trap.y.g = bbox.y.g;
  trap.l.xl = trap.l.xg = bbox.x.l;
  trap.g.xl = trap.g.xg = bbox.x.g;
  prim.type = trapType;
  prim.bounds.x.l = bbox.x.l;
  prim.bounds.x.g = bbox.x.g;
  prim.bounds.y.l = bbox.y.l;
  prim.bounds.y.g = bbox.y.g;
  prim.next = NIL;
  prim.priv = NIL;
  prim.items = 1;
  prim.maxItems = 1;
  prim.value.trap = &trap;
  (*device->procs->Mark)(device, &prim, NULL, &info);
  } /* GenInitPage */


private DevLong GenReadRaster (device, xbyte, ybit, wbytes, height, 
  copybytes, arg)
  PDevice device; DevLong xbyte, ybit, wbytes, height; 
  procedure (*copybytes)(); char *arg;
{
  return(1); /* not implemented on this device */
}
  
private DevColor GenConvertColor
  (device, colorSpace, input, tfr, priv, gt, r)
  PDevice device; integer colorSpace; DevInputColor *input;
  DevTfrFcn *tfr; DevPrivate *priv; DevGamutTransfer gt; 
  DevRendering r; { /* XXX deal with whitepoint */
  PGenStuff genStuff = (PGenStuff) device;
#if (MULTICHROME == 1)
  if (genStuff->colors == 4)
    return ConvertColorCMYK(device, colorSpace, input, tfr, priv, gt, r);
  else
#endif
    return ConvertColorRGB(device, colorSpace, input, tfr, priv, gt, r);
  }

typedef DevGamutTransfer (*PDevGamutTransferProc)();
typedef DevRendering (*PDevRenderingProc)();

public procedure IniGenDevImpl()
{
  genProcs = (DevProcs *)  os_sureMalloc ((long int)sizeof (DevProcs));

  genProcs->Mark = DevNoOp;
  genProcs->DefaultMtx = GenDefaultMtx;
  genProcs->DefaultBounds = GenDefaultBounds;
  genProcs->DefaultHalftone = GenDefaultHalftone;
  genProcs->ConvertColor = GenConvertColor;
  genProcs->FreeColor = DevNoOp;
  genProcs->SeparationColor = (PIntProc)DevAlwaysFalse;
  genProcs->ConvertGamutTransfer = (PDevGamutTransferProc)DevAlwaysFalse;
  genProcs->FreeGamutTransfer = DevNoOp;
  genProcs->ConvertRendering = (PDevRenderingProc)DevAlwaysFalse;
  genProcs->FreeRendering = DevNoOp;
  genProcs->WinToDevTranslation = GenWinToDevTranslation;
  genProcs->GoAway = GenGoAway;
  genProcs->Sleep = DevNoOp;
  genProcs->Wakeup = GenWakeup;
  genProcs->MakeMaskDevice = FmMakeMaskDevice; /* in framemaskdev.c */;
  genProcs->PreBuiltChar = DevAlwaysFalse;
  genProcs->MakeNullDevice = MakeNullDevice; /* in nulldev.c */
  genProcs->DeviceInfo = DevNoOp;
  genProcs->InitPage = GenInitPage;
  genProcs->ShowPage = DevAlwaysFalse;
  genProcs->ReadRaster = GenReadRaster;
  genProcs->Proc1 = (PIntProc)NULL;
  genProcs->Proc2 = (PIntProc)NULL;
  genProcs->Proc3 = (PIntProc)NULL;

} /* end of IniGenDevImpl */
