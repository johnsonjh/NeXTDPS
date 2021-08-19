/*
  colorops1.c

Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
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
Jim Sandman: Thu Sep 21 11:41:30 1989
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

Pool gtrStorage;

/* private global variables. make change in both arms if changing variables */

#if (OS != os_mpw)

/*-- BEGIN GLOBALS --*/
private  Pool clrStorage;    /* Blind pointer to pool for ColorRecs */
private real colorWr, colorWg, colorWb;
/*-- END GLOBALS --*/

#else (OS != os_mpw)

typedef struct {
  Pool g_clrStorage;
  real g_colorWr, g_colorWg, g_colorWb;
  } GlobalsRec, *Globals;
  
private Globals globals;

#define clrStorage globals->g_clrStorage
#define colorWr globals->g_colorWr
#define colorWg globals->g_colorWg
#define colorWb globals->g_colorWb

#endif (OS != os_mpw)

private procedure PopLightness(v) Preal v; {
  PopPReal(v);
  if (RealLt0(*v)) *v = fpZero;
  else if (*v > fp100) *v = fp100;
  }
  
public procedure LimitColor(r) Preal r; {
  real v = *r;
  if (RealLt0(v))
    *r = fpZero;
  else if (v > fpOne)
    *r = fpOne;
  }

public procedure PopPColorVal(v) Preal v; {
  PopPReal(v);
  LimitColor(v);
  }

public procedure PopColorValues(colorSpace, values)
  integer colorSpace; DevInputColor *values; {
  switch (colorSpace) {
    case DEVGRAY_COLOR_SPACE: {
      PopPColorVal(&values->value.gray);
      break;
      }
    case DEVRGB_COLOR_SPACE: {
      real red, green, blue;
      PopPColorVal(&blue);
      PopPColorVal(&green);
      PopPColorVal(&red);
      values->value.rgb.red = red;
      values->value.rgb.green = green;
      values->value.rgb.blue = blue;
      break;
      }
    case DEVCMYK_COLOR_SPACE: {
      real cyan, magenta, yellow, black;
      PopPColorVal(&black);
      PopPColorVal(&yellow);
      PopPColorVal(&magenta);
      PopPColorVal(&cyan);
      values->value.cmyk.black = black;
      values->value.cmyk.yellow = yellow;
      values->value.cmyk.magenta = magenta;
      values->value.cmyk.cyan = cyan;
      break;
      }
    case CIELIGHTNESS_COLOR_SPACE: {
      PopLightness(&values->value.lightness);
      break;
      }
    case CIELAB_COLOR_SPACE: {
      real a, b;
      PopPReal(&values->value.lab.b);
      PopPReal(&values->value.lab.a);
      PopLightness(&values->value.lab.l);
      values->value.lab.a = a;
      values->value.lab.b = b;
      break;
      }
    }
  }

public procedure RemGTRef(gt, free) GamutTfr gt; boolean free; {
  if (gt == NIL) return;
  if ((--gt->ref) == 0) {
    if (gs != NIL && gs->device != NIL) 
      (*gs->device->procs->FreeGamutTransfer)(gs->device, gt->devGT);
    if (free)
      os_freeelement(gtrStorage, gt);
    }
  } /* end of RemColorRef */

public procedure RemRndrRef(r, free) Rendering r; boolean free; {
  if (r == NIL) return;
  if ((--r->ref) == 0) {
    if (gs != NIL && gs->device != NIL) 
      (*gs->device->procs->FreeRendering)(gs->device, r->devRndr);
    if (free)
      os_freeelement(gtrStorage, r);
    }
  } /* end of RemRndrRef */
    
public procedure RemColorRef(c, free) Color c; boolean free; {
  if (c == NIL) return;
  if ((--c->ref) == 0) {
    if (gs != NIL && gs->device != NIL) 
      (*gs->device->procs->FreeColor)(gs->device, c->color);
    if (free)
      os_freeelement(clrStorage, c);
    }
  } /* end of RemColorRef */
  

public Color ChangeColor() {
  register PGState gsr = gs;
  Color c = gsr->color;
  RemColorRef(c, false);
  if (c == NIL || c->ref > 0) {
    c = (Color)os_newelement(clrStorage);
    if (gsr->color == NIL)
      os_bzero(c, sizeof(ColorRec));
    else *c = *gsr->color;
    gsr->color = c;
    }
  c->ref = 1;
  return c;
  }


public procedure SetDevColor(c) Color c; {
  register PGState g = gs;
  TfrFcn t = g->tfrFcn;
  if (!c) return;
  g->color = c;
  if (t && !t->active) ActivateTfr(t);
  c->color = (*g->device->procs->ConvertColor)(
    g->device, c->colorSpace, &c->inputColor,
    ((t != NIL) ? t->devTfr : NIL), &g->extension,
    ((g->gt != NIL) ? g->gt->devGT : NIL),
    ((g->rendering != NIL) ? g->rendering->devRndr : NIL)
    );
  }


public procedure SetGray(gray) Preal gray;
{
  Color c = ChangeColor();
  c->colorSpace = DEVGRAY_COLOR_SPACE;
  c->inputColor.value.gray = *gray;
  SetDevColor(c);
  }  /* end of SetGray */



public procedure PSSetGray() {
  real gray;
  if (gs->noColorAllowed)
    Undefined ();
  PopPColorVal(&gray);
  SetGray(&gray);}


public procedure PSCrGray() {
  Color c = gs->color;
  real r;
  DebugAssert(c);
  switch(c->colorSpace) {
    case DEVGRAY_COLOR_SPACE: {
      r = c->inputColor.value.gray;
      break;
      }
    case DEVRGB_COLOR_SPACE: {
      r =
        colorWr * c->inputColor.value.rgb.red +
        colorWg * c->inputColor.value.rgb.green +
        colorWb * c->inputColor.value.rgb.blue;
      break;
      }
    case DEVCMYK_COLOR_SPACE: {
      r =
        colorWr * (fpOne - c->inputColor.value.cmyk.cyan) +
        colorWg * (fpOne - c->inputColor.value.cmyk.magenta) +
        colorWb * (fpOne - c->inputColor.value.cmyk.yellow) +
        c->inputColor.value.cmyk.black;
      r = fpOne - r;
      LimitColor(&r);
      break;
      }
    default: {
      r = fpZero;
      break;
      }
    }
  PushPReal(&r);
  }

public procedure SetRGBColor(r, g, b) Preal r, g, b;
{
  Color c;
  if (gs->noColorAllowed)
    Undefined ();
  c = ChangeColor();
  c->colorSpace = DEVRGB_COLOR_SPACE;
  c->inputColor.value.rgb.red = *r;
  c->inputColor.value.rgb.green = *g;
  c->inputColor.value.rgb.blue = *b;
  SetDevColor(c);
  }

public procedure PSSetRGBColor()
{
real red, green, blue;
PopPColorVal(&blue);
PopPColorVal(&green);
PopPColorVal(&red);
SetRGBColor(&red, &green, &blue);
} /* end of PSSetRGBColor */

private procedure CrRGBColor(r, g, b)  real *r, *g, *b;
{
  Color c = gs->color;
  DebugAssert(c);
  switch(c->colorSpace) {
    case DEVGRAY_COLOR_SPACE: {
      *r = *g = *b = c->inputColor.value.gray;
      break;
      }
    case DEVRGB_COLOR_SPACE: {
      *r = c->inputColor.value.rgb.red;
      *g = c->inputColor.value.rgb.green;
      *b = c->inputColor.value.rgb.blue;
      break;
      }
    case DEVCMYK_COLOR_SPACE: {
      real t, black;
      black = c->inputColor.value.cmyk.black;
      t = fpOne - c->inputColor.value.cmyk.cyan - black;
      LimitColor(&t);
      *r = t;
      t = fpOne - c->inputColor.value.cmyk.magenta - black;
      LimitColor(&t);
      *g = t;
      t = fpOne - c->inputColor.value.cmyk.yellow - black;
      LimitColor(&t);
      *b = t;
      break;
      }
    default: {
      *r = *g = *b = fpZero;
      break;
      }
    }
  } /* end of CrRGBColor */


public procedure PSCrRGBColor()
{
real red, green, blue;
CrRGBColor(&red, &green, &blue);
PushPReal(&red);  PushPReal(&green);  PushPReal(&blue);
} /* end of PSCrRGBColor */

public procedure PSSetCMYKColor()  /* setcmykcolor */
{
  real cyan, magenta, yellow, black;
  Color c;
  if (gs->noColorAllowed)
    Undefined ();
  c = ChangeColor();
  PopPColorVal(&black);
  PopPColorVal(&yellow);
  PopPColorVal(&magenta);
  PopPColorVal(&cyan);
  c->inputColor.value.cmyk.black = black;
  c->inputColor.value.cmyk.yellow = yellow;
  c->inputColor.value.cmyk.magenta = magenta;
  c->inputColor.value.cmyk.cyan = cyan;
  c->colorSpace = DEVCMYK_COLOR_SPACE;
  SetDevColor(c);
}  /* end of PSSetCMYKColor */

public procedure PSCrCMYKColor()  /* currentcmykcolor */
{
  Color c = gs->color;
  real cyan, magenta, yellow, black;
  DebugAssert(c);
  switch(c->colorSpace) {
    case DEVGRAY_COLOR_SPACE: {
      cyan = magenta = yellow = fpZero;
      black = fpOne - c->inputColor.value.gray;
      break;
      }
    case DEVRGB_COLOR_SPACE: {
      real removal;
      TfrFcn tf = gs->tfrFcn;
      cyan = fpOne - c->inputColor.value.rgb.red;
      magenta = fpOne - c->inputColor.value.rgb.green;
      yellow = fpOne - c->inputColor.value.rgb.blue;
      black = (cyan < magenta) ? cyan : magenta;
      if ( yellow < black) black = yellow;
      removal = black;
      if (tf) {
	if (tf->tfrUCRFunc.length != 0)
	  {
	  PushPReal(&removal);
	  if (psExecute(tf->tfrUCRFunc)) return;
	  if (removal < -fpOne) removal = -fpOne;
	  else if (removal > fpOne) removal = fpOne;
	  }
	if (tf->tfrBGFunc.length != 0)
	  {
	  PushPReal(&black);
	  if (psExecute(tf->tfrUCRFunc)) return;
	  PopPColorVal(&black);
	  }
        }
      cyan -= removal;
      magenta -= removal;
      yellow -= removal;
      LimitColor(&cyan);
      LimitColor(&magenta);
      LimitColor(&yellow);
      break;
      }
    case DEVCMYK_COLOR_SPACE: {
      cyan = c->inputColor.value.cmyk.cyan;
      magenta = c->inputColor.value.cmyk.magenta;
      yellow = c->inputColor.value.cmyk.yellow;
      black = c->inputColor.value.cmyk.black;
      break;
      }
    default: {
      cyan = magenta = yellow = fpZero;
      black = fpOne;
      break;
      }
    }
  PushPReal(&cyan);
  PushPReal(&magenta);
  PushPReal(&yellow);
  PushPReal(&black);
  } /* end of PSCrCMYKColor */

public procedure PSSetHSBColor()
{
real hue, sat, red, green, blue, f, m, n, k, value;
integer i;
PopPColorVal(&value);
PopPColorVal(&sat);
PopPColorVal(&hue);
hue *= fp6;  if (hue >= fp6) hue = fpZero;
i = hue;
f = hue - (real)i;
m = value * (fpOne - sat);
n = value * (fpOne - (sat * f));
k = value * (fpOne - (sat * (fpOne - f)));
switch (i)
  {
  case 0:  red = value;  green = k;  blue = m;  break;
  case 1:  red = n;  green = value;  blue = m;  break;
  case 2:  red = m;  green = value;  blue = k;  break;
  case 3:  red = m;  green = n;  blue = value;  break;
  case 4:  red = k;  green = m;  blue = value;  break;
  case 5:  red = value;  green = m;  blue = n;
  }
SetRGBColor(&red, &green, &blue);
}


public procedure PSCrHSBColor()
{
real hue, sat, red, green, blue;
real diff, value, x, r, g, b;
CrRGBColor(&red, &green, &blue);
hue = sat = fpZero;
value = x = red;
if (green > value) value = green;  else x = green;
if (blue > value) value = blue;
if (blue < x) x = blue;
if (RealNe0(value))
  {
  diff = value - x;
  if (RealNe0(diff))
    {
    sat = diff / value;
    r = (value - red) / diff;
    g = (value - green) / diff;
    b = (value - blue) / diff;
    if      (red == value)   hue = (green == x) ? fp5 + b : fpOne - g;
    else if (green == value) hue = (blue == x) ? fpOne + r : fp3 - b;
    else                     hue = (red == x) ? fp3 + g : fp5 - r;
    hue /= fp6;  if (hue >= fp6 || RealLt0(hue)) hue = fpZero;
    }
  }
PushPReal(&hue);
PushPReal(&sat);
PushPReal(&value);
} /* end of PSCrHSBColor */


public procedure IniClrSpace(reason)  InitReason reason;
{
integer i;
switch (reason)
  {
  case init:
#if (OS == os_mpw)
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    clrStorage =  (Pool) os_newpool(sizeof(ColorRec),3,0);
    colorWr = fpp3;  colorWg = fpp59;  colorWb = fpOne - (colorWr + colorWg);
    break;
  case romreg:
    break;
  case ramreg:
    break;
  }
} /* end of IniGraphics */
