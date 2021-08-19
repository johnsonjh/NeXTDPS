/* PostScript nulldev implementation

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
#include DEVCREATE

#include "devcommon.h"
#include "nulldev.h"

public DevProcs *nullProcs;

private DevScreen nullDevScreen;
private DevHalftone nullDevHalftone;
private unsigned char nullThresholds[1] = {127};

private procedure NullDefaultMtx (device, matrix) PDevice device; PMtx matrix; {
  matrix->a = matrix->d = fpOne;
  matrix->b = matrix->c = matrix->tx = matrix->ty = 0;
  }

private procedure NullDefaultBounds (device, bBox)
  PDevice device; DevLBounds *bBox; {
  bBox->x.l = bBox->y.l = 0;
  bBox->x.g = bBox->y.g = 1;
  }

private procedure NullGoAway (device) PDevice device; {
  os_free((char *)device);
  }

private PDevice NullMakeMaskDevice (device, args)
  PDevice device; MakeMaskDevArgs *args; {
  PDevice parent = (PDevice)device->priv;
  return
    (parent == NULL) ? NULL : (*parent->procs->MakeMaskDevice)(parent, args);
  };

public PDevice MakeNullDevice(device) PDevice device; {
  PDevice new = (PDevice)os_sureCalloc((integer)sizeof(Device), (integer)1);
  new->priv = (DevPrivate *)device;
  new->maskID = (device == NULL) ? 0 : device->maskID;
  new->procs = nullProcs;
  /* ref and priv fields NULL from calloc. */
  return new;
  };

private procedure NullWinToDevTranslation (device, translation)
  PDevice device; DevPoint *translation; {
  translation->x = 0;
  translation->y = 0;
  /* add this to a buffer coordinate to get a PS device coordinate */
  }

public PDevice CreateNullDevice() { /* exported to devcreate.h */
  return ((*nullProcs->MakeNullDevice)(NIL));
  }

private procedure NullWakeup (device) PDevice device; {
  CurrentDevice = device;
  }

private DevHalftone * NullDefaultHalftone (device) PDevice device; {
  return &nullDevHalftone;
  }

private DevLong NullReadRaster (
  device, xbyte, ybit, wbytes, height, copybytes, arg)
  PDevice device; DevLong xbyte, ybit, wbytes, height; 
  procedure (*copybytes)(); char *arg; {
  return(1);
  }
  
typedef DevColor (*PDevColorProc)();
typedef DevGamutTransfer (*PDevGamutTransferProc)();
typedef DevRendering (*PDevRenderingProc)();

public procedure IniNullDevImpl() {
  nullDevScreen.width = nullDevScreen.height = 1;
  nullDevScreen.thresholds = &nullThresholds[0];
  nullDevHalftone.red = nullDevHalftone.green = nullDevHalftone.blue =
  nullDevHalftone.white = &nullDevScreen;
  nullDevHalftone.priv = (DevPrivate *)1;

  nullProcs = (DevProcs *) os_sureMalloc ((integer)sizeof (DevProcs));

  nullProcs->Mark = DevNoOp;
  nullProcs->DefaultMtx = NullDefaultMtx;
  nullProcs->DefaultBounds = NullDefaultBounds;
  nullProcs->DefaultHalftone = NullDefaultHalftone;
  nullProcs->ConvertColor = (PDevColorProc)DevAlwaysFalse;
  nullProcs->FreeColor = DevNoOp;
  nullProcs->SeparationColor = (PIntProc)DevAlwaysFalse;
  nullProcs->ConvertGamutTransfer = (PDevGamutTransferProc)DevAlwaysFalse;
  nullProcs->FreeGamutTransfer = DevNoOp;
  nullProcs->ConvertRendering = (PDevRenderingProc)DevAlwaysFalse;
  nullProcs->FreeRendering = DevNoOp;
  nullProcs->WinToDevTranslation = NullWinToDevTranslation;
  nullProcs->GoAway = NullGoAway;
  nullProcs->Sleep = DevNoOp;
  nullProcs->Wakeup = NullWakeup;
  nullProcs->MakeMaskDevice = NullMakeMaskDevice;
  nullProcs->PreBuiltChar = DevAlwaysFalse;
  nullProcs->MakeNullDevice = MakeNullDevice;
  nullProcs->DeviceInfo = DevNoOp;
  nullProcs->InitPage = DevNoOp;
  nullProcs->ShowPage = DevAlwaysFalse;
  nullProcs->ReadRaster = NullReadRaster;
  nullProcs->Proc1 = (PIntProc)NULL;
  nullProcs->Proc2 = (PIntProc)NULL;
  nullProcs->Proc3 = (PIntProc)NULL;

  } /* end of IniNullDevImpl */
