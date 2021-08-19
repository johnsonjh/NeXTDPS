/*****************************************************************************

    coordinates.c

    contains routines for dealing with the various coordinate systems within
    the PostScript implementation.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
    
    Created 09Mar87 Leo
    
    Modified:

    31Mar87 Leo   Made CdSize2Bounds overscan
    03May87 Jack  fix clipping added 23Apr87 to 20Apr PSSizeImage
    01Sep87 Jack  add include ORPHANS for RgCmdTable
    14Dec87 Jack  fix PSCurrentWindowBounds for BASEPSWINDOWID case
    12Apr88 Jack  generalize CdSize2Bounds for rotated coordinate systems
    22Apr88 Jack  Bug fix in CdSize2Bounds
    24Apr88 Leo	  GlobalToLocal and LocalToGlobal take PWindowDevice
    22Aug88 Leo   BASEPSWINDOWID gone; now window 0
    14Jan89 Leo	  Imported Jim Sandman's RealEq1 defs
    24Jan89 Jack  Add <multiproc> <ncolors> as returns from sizeimage
    04Dec89 Ted   Integratathon!
    06Dec89 Ted   ANSI C Prototyping, formatting.
    25Jan90 Terry update multiproc and framebitdepth for color in sizeimage
    30Jan90 Terry Made color and monochrome code coexist
    19Feb90 Terry Corrected bounds in sizeimage for LBitmapFormat call
    29May90 Terry Reduced code size from 3116 to 2880 bytes
    07Jun90 Terry Reduced code size from 2880 to 2428 bytes
    21Jul90 Ted   Modified PSSizeImage to use new depth constants (new API)

******************************************************************************/

#import PACKAGE_SPECS
#import CUSTOMOPS
#import FP
#import BINTREE
#import WINDOWDEVICE

/* Floating-point rounding */
#define RealRound(a) ((integer)((a) > 0 ? (a) + 0.5 : (a) - 0.5))

#define CheckWindowDev(d) if (((PWindowDevice)d)->fd.procs != wdProcs) \
PSInvalidID()

extern DevProcs *wdProcs;
extern Layer *ID2Layer();

public procedure ScreenToBase(PWindowDevice wd, Cd *p)
{
    Bounds winBounds;

    GetWinBounds(Wd2Layer(wd), &winBounds);
    p->y -= winBounds.miny;
    p->x -= winBounds.minx;
}

#define MyCeil(f) ((f)+(float)0.999999)

public procedure PopBounds(PMtx m, Bounds *devBounds)
{
    Cd userPt, userSize;
    real px, py, wx, hy, wy, hx;
    
    PSPopPCd(&userSize);
    PSPopPCd(&userPt);
    if (RealEq0(m->b) && RealEq0(m->c)) { /* common case */
	px = userPt.x * m->a + m->tx;
	wx = userSize.x * m->a;
	py = userPt.y * m->d + m->ty;
	hy = userSize.y * m->d;
    } else { /* general case */
	px = userPt.x * m->a + userPt.y * m->c + m->tx;
	py = userPt.y * m->d + userPt.x * m->b + m->ty;
	wx = userSize.x * m->a;
	wy = userSize.x * m->b;
	hx = userSize.y * m->c;
	hy = userSize.y * m->d;
	if (((hx <= 0) && (wx <= 0)) || ((hx >= 0) && (wx >= 0)))
	    wx += hx;
	else {
	    px += wx;
	    wx = hx - wx;
	}
	if (((hy <= 0) && (wy <= 0)) || ((hy >= 0) && (wy >= 0)))
	    hy += wy;
	else {
	    py += wy;
	    hy -= wy;
	}
    }
    if (wx < 0) {
	devBounds->minx = (int)(px + wx);
	devBounds->maxx = (int)MyCeil(px);
    } else {
	devBounds->minx = (int)px;
	devBounds->maxx = (int)MyCeil(px + wx);
    }
    if (hy < 0) {
	devBounds->miny = (int)(py + hy);
	devBounds->maxy = (int)MyCeil(py);
    } else {
	devBounds->miny = (int)py;
	devBounds->maxy = (int)MyCeil(py + hy);
    }
}

/*****************************************************************************
	DUserToDevice
	transforms a user coordinate distance to a device 
	coordinate distance in the given window.
******************************************************************************/

public procedure DUserToDevice(Mtx *matrix, Cd userCd, DevCd *deviceCd)
{
    Cd intCd;
    
    DTfmPCd(userCd, matrix, &intCd);
    deviceCd->x = RealRound(intCd.x);
    deviceCd->y = RealRound(intCd.y);
}

public procedure GlobalToLocal(PWindowDevice wd, int *xp, int *yp)
{
    Bounds winBounds;

    if (wd == NULL) wd = (PWindowDevice) PSGetDevice(NULL);
    CheckWindowDev(wd);
    GetWinBounds(Wd2Layer(wd), &winBounds);
    *xp -= winBounds.minx;
    *yp -= winBounds.miny;
}

public procedure LocalToGlobal(PWindowDevice wd, int *xp, int *yp)
{
    Bounds winBounds;

    if (wd == NULL) wd = (PWindowDevice) PSGetDevice(NULL);
    CheckWindowDev(wd);
    GetWinBounds(Wd2Layer(wd), &winBounds);
    *xp += winBounds.minx;
    *yp += winBounds.miny;
}

/*****************************************************************************
	UserToDevice
	transforms a user coordinate point to a device coordinate
	point in the given window.
******************************************************************************/

public procedure UserToDevice(Mtx *matrix, Cd userCd, DevCd *deviceCd)
{
    Cd intCd;
    
    TfmPCd(userCd, matrix, &intCd);
    deviceCd->x = RealRound(intCd.x);
    deviceCd->y = RealRound(intCd.y);
}

private GetBoundsPopCd(Bounds *b, Cd *pt)
{
    PDevice device;

    PSPopPCd(pt);
    device = PSGetDevice(NULL);
    CheckWindowDev(device);
    GetWinBounds(Wd2Layer(device), b);
}

private procedure PSBaseToCurrent()
{
    Cd pt;
    Bounds winBounds;

    GetBoundsPopCd(&winBounds, &pt);
    pt.y = (winBounds.maxy - winBounds.miny) - pt.y;
    pt = ITfmCd(pt, PSGetMatrix(NULL));
    PSPushPCd(&pt);
}

private procedure PSBaseToScreen()
{
    Cd pt;
    Bounds winBounds;

    GetBoundsPopCd(&winBounds, &pt);
    pt.x += winBounds.minx;
    pt.y += winBounds.miny;
    PSPushPCd(&pt);
}

private procedure PSCurrentToBase()
{
    Cd pt;
    Bounds winBounds;

    GetBoundsPopCd(&winBounds, &pt);
    pt = TfmCd(pt, PSGetMatrix(NULL));
    pt.y = (winBounds.maxy - winBounds.miny) - pt.y;
    PSPushPCd(&pt);
}

private procedure PSCurrentToScreen()
{
    Cd pt;
    Bounds winBounds;

    GetBoundsPopCd(&winBounds, &pt);
    pt = TfmCd(pt, PSGetMatrix(NULL));
    pt.y = winBounds.maxy - pt.y;
    pt.x += winBounds.minx;
    PSPushPCd(&pt);
}

/*****************************************************************************
    <win> currentwindowbounds <x> <y> <w> <h>
******************************************************************************/

private procedure PSCurrentWindowBounds()
{
    Bounds winBounds;
    integer wID;

    wID = PSPopInteger();
    if (wID == 0)
	GetWinBounds(NULL, &winBounds);
    else
	GetWinBounds(ID2Layer(wID), &winBounds);
    PSPushInteger(winBounds.minx);
    PSPushInteger(winBounds.miny);
    PSPushInteger(winBounds.maxx-winBounds.minx);
    PSPushInteger(winBounds.maxy-winBounds.miny);
}

private procedure PSScreenToBase()
{
    Cd pt;
    PDevice device = PSGetDevice(NULL);

    PSPopPCd(&pt);
    CheckWindowDev(device);
    ScreenToBase((PWindowDevice) device, &pt);
    PSPushPCd(&pt);
}

private procedure PSScreenToCurrent()
{
    Cd pt;
    Bounds winBounds;

    GetBoundsPopCd(&winBounds, &pt);
    pt.x -= winBounds.minx;
    pt.y =  winBounds.maxy - pt.y;
    pt = ITfmCd(pt, PSGetMatrix(NULL));
    PSPushPCd(&pt);
}

private procedure PSSizeImage()
{
    Mtx m;
    PSObject a;
    Cd userSize, userPt;
    Bounds bounds, winBounds;
    PDevice device = PSGetDevice(NULL);
    PMtx gsMtx = PSGetMatrix(NULL);
    Layer *layer = Wd2Layer(device);
    
    CheckWindowDev(device);
    PSPopTempObject(dpsArrayObj, &a);

    /* Clip image bounds to window bounds */
    PopBounds(gsMtx, &bounds);					/* in window */
    GetWinBounds(layer, &winBounds);				/* in screen */
    OFFSETBOUNDS(winBounds, -winBounds.minx, -winBounds.miny);
    sectBounds(&bounds, &winBounds, &bounds);			/* clipped */

    /* Push pixel width + height of clipped image bounds */
    PSPushInteger((bounds.minx<bounds.maxx) ? bounds.maxx-bounds.minx : 0);
    PSPushInteger((bounds.miny<bounds.maxy) ? bounds.maxy-bounds.miny : 0);

    /* Create a matrix that transforms to image's coordinate system */
    m = *gsMtx;
    m.tx -= (real)(bounds.minx);
    m.ty -= (real)(bounds.miny);

    switch(LPreCopyBitsFrom(layer)) {
	case NX_TWOBITGRAY:
	    PSPushInteger(2);    /* bps */
	    PSPushPMtx(&a, &m);
	    PSPushBoolean(1);    /* multiproc */
	    PSPushInteger(1);    /* ncolors */
	    break;
	case NX_EIGHTBITGRAY:
	    PSPushInteger(8);    /* bps */
	    PSPushPMtx(&a, &m);
	    PSPushBoolean(1);    /* multiproc */
	    PSPushInteger(1);    /* ncolors */
	    break;
	case NX_TWELVEBITRGB:
	    PSPushInteger(4);    /* bps */
	    PSPushPMtx(&a, &m);
	    PSPushBoolean(0);    /* multiproc */
	    PSPushInteger(3);    /* ncolors */
	    break;
    	case NX_TWENTYFOURBITRGB:
	    PSPushInteger(8);    /* bps */
	    PSPushPMtx(&a, &m);
	    PSPushBoolean(0);    /* multiproc */
	    PSPushInteger(3);    /* ncolors */
	    break;
	default: CantHappen();
    }
}

private readonly RgOpTable cmdCoordinates = {
    { "currentwindowbounds", PSCurrentWindowBounds },
    { "sizeimage", PSSizeImage },
    { "currenttobase",PSCurrentToBase },
    { "basetocurrent",PSBaseToCurrent },
    { "screentocurrent",PSScreenToCurrent },
    { "currenttoscreen",PSCurrentToScreen },
    { "basetoscreen",PSBaseToScreen },
    { "screentobase",PSScreenToBase },
    NIL };
  
public procedure IniCoordinates(int reason)
{
    if (reason == 1) PSRgstOps(cmdCoordinates);
}











