/*
  nulldevice.c

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
Ivor Durham: Wed Jun 15 16:34:06 1988
Ed Taft: Thu Jul 28 13:19:12 1988
Jim Sandman: Wed Aug  2 15:10:55 1989
Joe Pasqua: Fri Feb  3 13:49:03 1989
Paul Rovner: Thu Dec 28 17:07:07 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include DEVICE
#include GRAPHICS
#include ERROR
#include PSLIB

public PDevice psNulDev;

private procedure NullDefaultMtx(device, m)  PDevice device; PMtx m; {
  IdentityMtx(m);}

private procedure NullDefaultBounds(device, bbox)
  PDevice device; DevLBounds *bbox; {
  bbox->x.l = bbox->x.g = bbox->y.l = bbox->y.g = 0;}

private PDevice NullMakeMaskDevice(device) PDevice device; {
  return NIL;
  };

private PDevice NullMakeNullDevice(device) PDevice device; {
  return psNulDev;
  };

private procedure NoOp() {}
private boolean NoOpBool() {return false;}

/*ARGSUSED*/
public integer NullRdBytes(device, xbyte, ybit, wbytes, hbits, bytes)
  PDevice device; DevLong xbyte, ybit, wbytes, hbits; char *bytes; {
  os_bzero((char *)bytes, (long int)(wbytes * hbits));
  }
  
private procedure NullWinToDevTranslation (device, translation)
  PDevice device; DevPoint *translation; {
  translation->x = 0; translation->y = 0;
  }


public procedure NullDevice()  {
  NewDevice((*gs->device->procs->MakeNullDevice)(gs->device));}

typedef DevColor (*PDevColorProc)();
typedef DevGamutTransfer (*PDevGamutTransferProc)();
typedef DevRendering (*PDevRenderingProc)();
typedef DevHalftone * (*PDevHalftoneProc)();

public procedure IniNullDevice(reason)  InitReason reason;
{

switch (reason)
  {
  case init: {
    DevProcs *nullProcs =
      (DevProcs *)os_sureMalloc((long int)sizeof(DevProcs));
    psNulDev = (PDevice)os_sureCalloc((long int)sizeof(Device), 1);
    psNulDev->ref = 1;
    psNulDev->maskID = 0;
    psNulDev->procs = nullProcs;
    psNulDev->priv = (DevPrivate *)NIL;
    nullProcs->DefaultMtx = NullDefaultMtx;
    nullProcs->DefaultBounds = NullDefaultBounds;
    nullProcs->DefaultHalftone = (PDevHalftoneProc)NoOpBool;
    nullProcs->ConvertColor = (PDevColorProc)NoOpBool;
    nullProcs->FreeColor = NoOp;
    nullProcs->SeparationColor = (PIntProc)NoOpBool;
    nullProcs->ConvertGamutTransfer = (PDevGamutTransferProc)NoOpBool;
    nullProcs->FreeGamutTransfer = NoOp;
    nullProcs->ConvertRendering = (PDevRenderingProc)NoOpBool;
    nullProcs->FreeRendering = NoOp;
    nullProcs->WinToDevTranslation = NullWinToDevTranslation;
    nullProcs->GoAway = NoOp;
    nullProcs->Sleep = NoOp;
    nullProcs->Wakeup = NoOp;
    nullProcs->MakeMaskDevice = NullMakeMaskDevice;
    nullProcs->PreBuiltChar = NoOpBool;
    nullProcs->MakeNullDevice = NullMakeNullDevice;
    nullProcs->DeviceInfo = NoOp;
    nullProcs->InitPage = NoOp;
    nullProcs->ShowPage = NoOpBool;
    break;
    }
  case romreg:
    break;
  }
} /* end of IniNullDevice */


