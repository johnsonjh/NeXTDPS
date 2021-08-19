/*****************************************************************************

    windowdevice.c

    Device vector for the NeXT Display PostScript window device.

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
	
    Created 12Apr88 Leo

    Modified:

    02May88 Leo  Added initialization of defaultHalftone
    13May88 Jack Add FlushHalftoneInfo to *wdProcs
    08Jul88 Leo  Update to v1001
    27Jul88 Jack New WdShowPage returns true so PSShowPage won't call ErasePage
    28Jul88 Jack Stop overriding FmFlushHalftone
    17Jan89 Jack Make WdConvertColor round components
    18May89 Jack TEMP switch back to GrayPattern 'cause BUG in GryPat4Of4
    12Jun89 Jack Switch back to GryPat4Of4
    10Oct89 Jack Begin conversion to 24bit color
    19Oct89 Jack Put back minimal wdDevHalftone so imsxxd3x will work
    16Oct89 Terry CustomOps conversion
    15Nov89 Ted  ANSI C prototyping.  ONEMINUSALPHA -> ALPHAVALUE
    29Nov89 Terry IBM compatibility modifications
    04Dec89 Ted  Integratathon!
    13Dec89 Ted  Sorted routines, reformatting, cleanup
    21Dec89 Ted  Removed "nextgs.h" import, defines moved to windowdevice.h
    30Jan90 Terry Made color and monochrome code coexist
    08Mar90 Ted   Added remapY to map y coords. between user and device space.
    12Apr90 Jack  Removed colortowhite
    29May90 Terry Reduced code size from 744 to 656 bytes

******************************************************************************/

#import PACKAGE_SPECS
#import BINTREE
#import WINDOWDEVICE
#import "framedev.h"

public DevProcs *wdProcs;
public PMarkProcs wdMarkProcs;

extern procedure LMark();			/* layer.c */
extern procedure LInitPage();			/* layer.c */
extern procedure TermWindowDevice();		/* windowops.c */
extern DevColor ConvertColorRGB();		/* devcommon.c */
extern procedure DevNoOp();			/* devcommon.c */

#define MASKGA 0x0ff00ff
#define MASKRB ~MASKGA

public unsigned int AMulInPlace(unsigned int *src, unsigned char a)
{
    unsigned int s = *src, rb, ga, m;
    
    m = MASKGA; /* Which also equals HALFGA */
    rb = ((s & MASKRB) >> 8) * a;
    rb += ((rb >> 8) & m) + m;
    ga = (s & m) * a;
    ga += ((ga >> 8) & m) + m;
    *src = (rb & MASKRB) | ((ga >> 8) & m);
}

public DevColor WdConvertColor(PDevice device, integer colorSpace,
	DevInputColor *input, DevTfrFcn *tfr, DevPrivate *priv,
	DevGamutTransfer gt, DevRendering r)
{
    PNextGSExt ep;
    DevColor result;
    
    result = ConvertColorRGB(device, colorSpace, input, tfr, priv, gt, r);
    ep = *((PNextGSExt *)PSGetGStateExt(NULL));
    if (ep->alpha != OPAQUE)
	AMulInPlace((unsigned int *)&result, ep->alpha);
    /* now, perform nx gray pattern hackery (for setgraypattern) */
    if(ep->patternpending)
	ep->patternpending = 0;	/* clear flag for next setcolor call */
    else
	ep->graypatstate = NOGRAYPAT; /* setcolor was called,clear pattern */
    return result;
}

public procedure WdDefaultBounds(WindowDevice *device, DevLBounds *bBox)
{
    short width, height;
    
    bBox->x.l = bBox->y.l = 0;
    LGetSize(device->layer, &width, &height);
    bBox->x.g = width;
    bBox->y.g = height;
}

public procedure WdDefaultMtx(WindowDevice *device, PMtx matrix)
{
    short width, height;
    
    LGetSize(device->layer, &width, &height);
    matrix->a = REALSCALE(PSGetGStateExt(NULL)) ? 1.2777777778:1.0; /* 92dpi */
    matrix->b = matrix->c = matrix->tx = 0.0;
    matrix->d = -matrix->a;
    matrix->ty = (float) height;
}

private procedure WdDeviceInfo(PDevice device, DeviceInfoArgs *args)
{
    int grayValues,colors;

    switch(2) {
	case 2:  grayValues = 4;  colors = 1; break;
	case 32: grayValues = 256;  colors = 3; break;
	default: CantHappen();
    }
    args->dictSize = 2;
    (*args->AddIntEntry)("GrayValues", grayValues, args);
    (*args->AddIntEntry)("Colors", colors, args);
}

private DevLong DevAlwaysTrue(PDevice device) { return 1; }

/* SetupImageArgs, SetupMark, Wakeup, and Sleep
 * are replaced with device specific procedures in screen device drivers.
 * Whenever a Mark or Composite operation is received by a NXSDevice driver,
 * such as MP or ND, they check to see if wdProcs->Wakeup contains their
 * own wakeup proc (NDWakeup, MPWakeup, etc.).  If not, they initialize
 * the device package globals (defaultHalftone, framelog2BD, etc.) and install
 * their driver's Sleep, Wakeup, SetupMark, and SetupImageArgs procs
 * into wdProcs and wdMarkProcs.
 */

public procedure IniWdDevImpl()
{
    wdProcs = (DevProcs *) os_malloc(sizeof(DevProcs));
    *wdProcs = *fmProcs;
    wdProcs->Mark = LMark;
    wdProcs->ConvertColor = WdConvertColor;
    wdProcs->DefaultMtx = WdDefaultMtx;
    wdProcs->DefaultBounds = WdDefaultBounds;
    wdProcs->GoAway = TermWindowDevice;
    wdProcs->Wakeup = DevNoOp;
    wdProcs->Sleep = DevNoOp;
    wdProcs->InitPage = LInitPage;
    wdProcs->ShowPage = (boolean (*)())DevAlwaysTrue;
    wdProcs->ReadRaster = DevAlwaysTrue;
    wdProcs->DeviceInfo = WdDeviceInfo;
    wdProcs->Proc1 = (PIntProc)DevNoOp;

    wdMarkProcs = (PMarkProcs) os_malloc(sizeof(MarkProcsRec));
    *wdMarkProcs = *fmMarkProcs;
    wdMarkProcs->SetupImageArgs = DevNoOp;
}














