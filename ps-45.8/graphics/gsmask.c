/* PostScript Graphics Gray Scale Mask Procedures

              Copyright 1988 -- Adobe Systems, Inc.
            PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: Bill Paxton: Mon May  9 09:48:21 1988
Edit History:
Bill Paxton: Fri Sep  9 11:04:35 1988
Joe Pasqua: Thu Jan 12 11:37:02 1989
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
#include VM

#include "graphicspriv.h"
#include "path.h"

private integer bmscanlines, bmbytewidth;
private string bmbase, bml0, bml1, bml2, bml3, bml4;
private string tempgray, finalgray;
private integer scanlines, dwidth;
private readonly character bmlside[4] =
  {0, 1, 1, 2};
private readonly character lftBitArray[8] =
  {0xff, 0x7f, 0x3f, 0x1f, 0xf, 0x7, 0x3, 0x1};
private readonly character rhtBitArray[8] =
  {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};

private procedure GSMRun(run) DevRun *run; {
  DevShort y, pairs, ylast, bmbitwidth;
  register character maskl, maskr;
  register int units, xl, xr, unitl;
  register string destunit, destbase;
  register DevShort *buffptr;
  register string lba;
  string rba;
  y = run->bounds.y.l;
  buffptr = run->data;
  destbase = (string)((integer)bmbase + y * bmbytewidth);
  ylast = run->bounds.y.g;
  if (ylast > bmscanlines) ylast = bmscanlines;
  bmbitwidth = bmbytewidth << 3;
  lba = lftBitArray; rba = rhtBitArray;
  while (y < ylast) {
    pairs = *(buffptr++);
    while (--pairs >= 0) {
      xl = *(buffptr++); if (xl < 0) xl = 0;
      xr = *(buffptr++); if (xr > bmbitwidth) xr = bmbitwidth;
      if (y < 0 || xl >= xr) continue;
      maskl = lba[xl & 7];
      maskr = rba[xr & 7];
      unitl = xl >> 3;
      destunit = destbase + unitl;
      units = (xr >> 3) - unitl;
      if (units == 0) *destunit |= maskl & maskr;
      else {
        *(destunit++) |= maskl;
        maskl = -1;
        while (--units > 0) *(destunit++) = maskl;
        if (maskr) *destunit |= maskr;
        }
      }
    destbase = (string)((integer)destbase + bmbytewidth);
    y++;
    }
  }

private procedure GSMTraps(t, items) DevTrap *t; integer items; {
  register int xl, xr;
  register string destbase;
  Fixed lx, ldx, rx, rdx;
  DevShort ylast, xlt, xrt, bmbitwidth, y;
  boolean leftSlope, rightSlope;
  bmbitwidth = bmbytewidth << 3;
nextTrap:
  {
    register DevTrap *tt;
    if (--items < 0)
      return;
    tt = t++;
    y = tt->y.l;
    ylast = tt->y.g;
    if (ylast <= y)
      goto nextTrap;
    destbase = (string) ((integer) bmbase + y * bmbytewidth);
    xl = tt->l.xl;
    xr = tt->g.xl;
    xlt = tt->l.xg;
    xrt = tt->g.xg;
    leftSlope = (xl != xlt);
    if (leftSlope) { lx = tt->l.ix; ldx = tt->l.dx; }
    rightSlope = (xr != xrt);
    if (rightSlope) { rx = tt->g.ix; rdx = tt->g.dx; }
    }

  { register string lba = lftBitArray, rba = rhtBitArray;
    register character maskl, maskr;
    register int unitl, units;
    register string destunit;

    while (true) {
      if (y >= bmscanlines) goto nextTrap;
      if (y < 0) goto nextLine;
      if (xl < 0) xl = 0;
      if (xr > bmbitwidth) xr = bmbitwidth;
      if (xl >= xr) goto nextLine;
      maskl = lba[xl & 7];
      maskr = rba[xr & 7];
      unitl = xl >> 3;
      destunit = destbase + unitl;
      units = (xr >> 3) - unitl;
      if (units == 0)
        *destunit |= maskl & maskr;
      else {
        *(destunit++) |= maskl;
        maskl = -1;
        while (--units > 0)
          *(destunit++) = maskl;
        if (maskr) *destunit |= maskr;
        }
     nextLine:
      y++;
      switch (ylast - y) {
        case 0:
          goto nextTrap;
        case 1:           /* next line is the last line of the trap */
          xl = xlt; xr = xrt;
          break;
        default:
          if (leftSlope) { xl = IntPart(lx); lx += ldx; }
          if (rightSlope) { xr = IntPart(rx); rx += rdx; }
        }
      destbase = (string) ((integer) destbase + bmbytewidth);
      }
    }
  }

private procedure GSMSetupBuff(p) PPath p; {
  /* FIX THIS FOR STROKED CHARACTERS */
  bmbytewidth = os_floor(p->bbox.tr.x) + 1.0;
  bmscanlines = (os_floor(p->bbox.tr.y) + 1.0) * 8.0;
  bmbase = (string)NEW(1, bmbytewidth * bmscanlines);
  os_bzero((char *)bmbase, bmbytewidth * bmscanlines);
  }

private procedure GSMMark(dp) DevPrim *dp; {
  DevRun *run;
  integer i;
  while (dp) {
    switch (dp->type) {
      case trapType:
        GSMTraps(dp->value.trap, (integer)dp->items);
	break;
      case runType:
        run = dp->value.run;
	for (i = dp->items; i--; run++)
          GSMRun(run);
        break;
      default: CantHappen();
      }
    dp = dp->next;
    }
  }

private procedure GSMTrapsFilled(m) PMarkState m; {
  GSMMark(m->trapsDP);
  if (!m->haveTrapBounds)
    EmptyDevBounds(&m->trapsDP->bounds);
  m->trapsDP->items = 0;
  }

private procedure GSMAddRun(run) DevRun *run; {
  DevPrim aRunPrim;
  InitDevPrim(&aRunPrim, (DevPrim *)NULL);
  aRunPrim.type = runType;
  aRunPrim.items = 1;
  aRunPrim.maxItems = 1;
  aRunPrim.value.run = run;
  aRunPrim.bounds = run->bounds;
  GSMMark(&aRunPrim);
  }

private integer ReadGrayPix(row, col) integer row, col; {
  unsigned int d;
  if (col < 0 || col >= bmbytewidth || row < 0 || row >= scanlines)
    return 0;
  d = tempgray[row*dwidth + (col >> 1)];
  if ((col & 1) != 0) d &= 0xF;
  else d >>= 4;
  return d;
  }

private integer GrayRowMax(row) integer row; {
  integer mx = 0, col, d;
  for (col = 0; col < bmbytewidth; col++) {
    d = ReadGrayPix(row, col);
    if (d > mx) mx = d;
    }
  return mx;
  }

private procedure SetGrayPix(row, col, val) integer row, col, val; {
  unsigned int d, i;
  if (col < 0 || col >= bmbytewidth || row < 0 || row >= scanlines)
    CantHappen();
  i = row*dwidth + (col >> 1);
  d = finalgray[i];
  if ((col & 1) != 0) d = (d & 0xF0) | val;
  else d = (d & 0xF) | (val << 4);
  finalgray[i] = d;
  }

private boolean HasWhiteEdge(row, col, bot) integer row, col; boolean bot; {
  integer whitebyte;
  if (col < 0 || col >= bmbytewidth || row < 0 || row >= scanlines)
    return true;
  whitebyte = bot? bmbytewidth * 7 : 0;
  whitebyte += (row * bmbytewidth) << 3;
  whitebyte += col;
  return bmbase[whitebyte] == 0;
  }

private integer AdjustPixVal(slf, rowmax) integer slf, rowmax; {
  integer t0, t1, result;
  slf = FixInt(slf); rowmax = FixInt(rowmax);
  t0 = rowmax >> 2; t1 = rowmax >> 1;
  result = (slf < t0) ? (slf << 1) : t1 + (((slf - t0) << 1) / 3);
  return (result + 0x8000L) >> 16;
  }

private procedure GSAdjustRow(row, bot) integer row; boolean bot; {
  integer col, m2, slf, rowmax;
  rowmax = GrayRowMax(bot ? row - 1 : row + 1);
  m2 = GrayRowMax(row);
  if (m2 > rowmax) rowmax = m2;
  for (col = 0; col < bmbytewidth; col++) {
    if (ReadGrayPix(row, col) > 0 && HasWhiteEdge(row, col, bot)
        && HasWhiteEdge(row, col-1, bot) && HasWhiteEdge(row, col+1, bot)) {
      slf = ReadGrayPix(row, col);
      SetGrayPix(row, col, AdjustPixVal(slf, rowmax));
      }
    }
  }

private procedure GSAdjust(botRows, lenBotRows, topRows, lenTopRows)
  real *botRows, *topRows; integer lenBotRows, lenTopRows; {
  integer i, scanlines, y;
  /* FIX THIS SOMEDAY: we are ignoring the 90 degree rotate case */
  /* we are also ignoring 180 degree rotates */
  scanlines = bmscanlines >> 3;
  for (i = 0; i < lenBotRows; i++) {
    y = botRows[i]; /* convert to integer */
    if (y > 0 && y <= scanlines) GSAdjustRow(y - 1, true);
    }
  for (i = 0; i < lenTopRows; i++) {
    y = topRows[i]; /* convert to integer */
    if (y >= 0 && y < scanlines) GSAdjustRow(y, false);
    }
  }

private integer GSFilterPixel(row, col) integer row, col; {
  string src = bmbase + row * 8 * bmbytewidth + col;
  integer sum = 0;
  if (row > 0) {
    sum += bml4[src[-bmbytewidth]];
    sum += bml4[src[-2*bmbytewidth]];
    if (col > 0) {
      sum += bmlside[src[-bmbytewidth-1] & 3];
      if (src[-2*bmbytewidth-1] & 1) sum++;
      }
    if (col+1 < bmbytewidth) {
      sum += bmlside[(src[-bmbytewidth+1] >> 6) & 3];
      if (src[-2*bmbytewidth+1] & 0x80) sum++;
      }
    }
  if (row+1 < scanlines) {
    sum += bml4[src[8*bmbytewidth]];
    sum += bml4[src[9*bmbytewidth]];
    if (col > 0) {
      sum += bmlside[src[8*bmbytewidth-1] & 3];
      if (src[9*bmbytewidth-1] & 1) sum++;
      }
    if (col+1 < bmbytewidth) {
      sum += bmlside[(src[8*bmbytewidth+1] >> 6) & 3];
      if (src[9*bmbytewidth+1] & 0x80) sum++;
      }
    }
  /* byte 0 */
  if (col > 0) sum += bmlside[src[-1] & 3];
  if (col+1 < bmbytewidth) sum += bmlside[(src[1] >> 6) & 3];
  sum += bml0[*src]; src += bmbytewidth;
  /* byte 1 */
  if (col > 0) sum += bmlside[src[-1] & 3];
  if (col+1 < bmbytewidth) sum += bmlside[(src[1] >> 6) & 3];
  sum += bml1[*src]; src += bmbytewidth;
  /* byte 2 */
  if (col > 0) sum += bmlside[src[-1] & 3];
  if (col+1 < bmbytewidth) sum += bmlside[(src[1] >> 6) & 3];
  sum += bml2[*src]; src += bmbytewidth;
  /* byte 3 */
  if (col > 0) sum += bmlside[src[-1] & 3];
  if (col+1 < bmbytewidth) sum += bmlside[(src[1] >> 6) & 3];
  sum += bml3[*src]; src += bmbytewidth;
  /* byte 4 */
  if (col > 0) sum += bmlside[src[-1] & 3];
  if (col+1 < bmbytewidth) sum += bmlside[(src[1] >> 6) & 3];
  sum += bml3[*src]; src += bmbytewidth;
  /* byte 5 */
  if (col > 0) sum += bmlside[src[-1] & 3];
  if (col+1 < bmbytewidth) sum += bmlside[(src[1] >> 6) & 3];
  sum += bml2[*src]; src += bmbytewidth;
  /* byte 6 */
  if (col > 0) sum += bmlside[src[-1] & 3];
  if (col+1 < bmbytewidth) sum += bmlside[(src[1] >> 6) & 3];
  sum += bml1[*src]; src += bmbytewidth;
  /* byte 7 */
  if (col > 0) sum += bmlside[src[-1] & 3];
  if (col+1 < bmbytewidth) sum += bmlside[(src[1] >> 6) & 3];
  sum += bml0[*src];
  Assert(sum <= 512);
  sum += 16; sum >>= 5;
  if (sum > 15) sum = 15;
  return sum;
  }

private procedure GSMMake(imso, bmso, botRows, lenBotRows, topRows, lenTopRows)
  StrObj imso, bmso; real *botRows, *topRows; integer lenBotRows, lenTopRows; {
  integer sum, len, row, col, bmlen;
  string s, dest, temp;
  boolean tophalf;
  scanlines = bmscanlines >> 3;
  dwidth = (bmbytewidth + 1) >> 1; /* 4 bit per pixel output */
  len = dwidth * scanlines;
  bmlen = bmbytewidth * bmscanlines;
  s = (string)NEW(1, len);
  temp = (string)NEW(1, len);
  dest = temp;
  /* FIX: modify this to keep track of bounds while do the filtering */
    /* so can discard 0s around the margins */
    /* fix for storage leaks too */
  for (row = 0; row < scanlines; row++) {
    tophalf = true;
    for (col = 0; col < bmbytewidth; col++) {
      sum = GSFilterPixel(row, col);
      if (tophalf) { *dest = sum << 4; tophalf = false; }
      else { *dest++ |= sum; tophalf = true;  }
      }
    if (!tophalf) dest++;
    }
  os_bcopy((char *)temp, (char *)s, len);
  tempgray = temp; finalgray = s;
  GSAdjust(botRows, lenBotRows, topRows, lenTopRows);
  if (imso.type != strObj || imso.length < len) TypeCheck();
  os_bcopy((char *)s, (char *)(imso.val.strval), len);
  PushInteger(bmbytewidth);
  PushInteger(bmscanlines >> 3);
  imso.length = len;
  PushP(&imso);
  if (bmso.type != strObj || bmso.length < bmlen) TypeCheck();
  os_bcopy((char *)bmbase, (char *)(bmso.val.strval), bmlen);
  bmso.length = bmlen;
  PushP(&bmso);
  FREE(s);
  FREE(temp);
  }

private procedure GSMTlatPath(p, botRows, lenBotRows, topRows, lenTopRows)
  real *botRows, *topRows; integer lenBotRows,  lenTopRows;
  PPath p; {
  Cd delta;
  integer i;
  delta.x = - os_floor(p->bbox.bl.x);
  delta.y = - os_floor(p->bbox.bl.y);
  TlatPath(p, delta);
  for (i = 0; i < lenBotRows; i++) botRows[i] += delta.y;
  for (i = 0; i < lenTopRows; i++) topRows[i] += delta.y;
  }

private real gserosion, gsvarcoeff;

private procedure PSgserosion() {
  PopPReal(&gserosion);
  }

private procedure PSgsvarcoeff() {
  PopPReal(&gsvarcoeff);
  }

public procedure GSMaskFill(p, ow, varcoeff,
    botRows, lenBotRows, topRows, lenTopRows, locktype, pcn)
  PPath p; double ow, varcoeff;
  real *botRows, *topRows; integer lenBotRows, lenTopRows, locktype;
  Object *pcn; {
  procedure (*trapsFilled)();
  StrObj imso, bmso;
  real flateps;
  /* this is just a test implementation to produce image output */
  if (gs->isCharPath) { Fill(p, false); return;}
  if (!PopBoolean()) return;
  if (ow > gserosion) { /* limit erosion */
    /*  varcoeff *= gserosion / ow;  */
    ow = gserosion;
    }
  varcoeff = gsvarcoeff;
  GSMTlatPath(p, botRows, lenBotRows, topRows, lenTopRows);
  Assert(os_floor(p->bbox.bl.x) == 0.0 && os_floor(p->bbox.bl.y) == 0.0);
  Assert(p->bbox.tr.x < 1000.0 && p->bbox.tr.y < 1000.0);
  GSMSetupBuff(p);
  PopP(&bmso); PopP(&imso);
  PathForAll(true);
  trapsFilled = ms->procs->trapsFilled;
  ms->procs->trapsFilled = GSMTrapsFilled;
  flateps = gs->flatEps;
  gs->flatEps = 0.25;
  OFill(p, ow, varcoeff, 8.0, GSMAddRun);
  ms->procs->trapsFilled = trapsFilled;
  gs->flatEps = flateps;
  GSMMake(imso, bmso, botRows, lenBotRows, topRows, lenTopRows);
  FREE(bmbase);
  }

private procedure InitFilter(s, f) register string s, f; {
  register integer i, j, sum, msk;
  for (i = 0; i < 256; i++) {
    sum = 0; msk = 0x80; j = 0;
    while (msk) {
      if (i & msk) sum += (j < 4)? f[j] : f[7-j];
      msk >>= 1; j++;
      }
    *s++ = sum;
    }
  }

public IniGSMask(reason) InitReason reason; {
/* character f0[8] = { 2, 4, 5, 5 };
   character f1[8] = { 4, 6, 8, 9 };
   character f2[8] = { 5, 8, 9, 10 };
   character f3[8] = { 5, 9, 10, 10 };
   character f4[8] = { 1, 1, 1, 1 }; */
static character f0[8] = { 1, 2, 4, 4 };
static character f1[8] = { 1, 4, 6, 6 };
static character f2[8] = { 4, 6, 13, 15 };
static character f3[8] = { 4, 6, 15, 19 };
static character f4[8] = { 1, 1, 1, 1 };
integer i;
switch (reason)
  {
  case init:
    bml0 = (string)os_malloc((integer)256);
    bml1 = (string)os_malloc((integer)256);
    bml2 = (string)os_malloc((integer)256);
    bml3 = (string)os_malloc((integer)256);
    bml4 = (string)os_malloc((integer)256);
    InitFilter(bml0, f0);
    InitFilter(bml1, f1);
    InitFilter(bml2, f2);
    InitFilter(bml3, f3);
    InitFilter(bml4, f4);
    gserosion = 0.06;
    gsvarcoeff = 0.08;
    break;
  case romreg:
    RgstExplicit("gserosion", PSgserosion);
    RgstExplicit("gsvarcoeff", PSgsvarcoeff);
    break;
  case ramreg:
    break;
  }
}
