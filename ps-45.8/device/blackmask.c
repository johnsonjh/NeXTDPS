/*
  blackmask.c

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
#include EXCEPT

#include "framemaskdev.h"
#if (MASKCOMPRESSION)
#include "compress.h"
#endif

public procedure BlackMasksMarkReal();

#define uint unsigned int
#define uchar unsigned char

void ExpandMask(PMask m, PSCANTYPE dst)
{
    int h = m->height;
    int w = (m->width + 7) >> 3;
    uchar *src = m->data;

    do {
	*dst = 0;
	bcopy(src, (char *)dst, w);
	dst++; src += w;
    } while (--h != 0);
}

void BlackMasksMark(DevMask *masks, int items, MarkArgs *args)
{
    unsigned char *table, *sourcebyte;
    SCANTYPE t;

    if (framelog2BD != 1) { BlackMasksMarkReal(masks, items, args); return; }
    table = (uchar *)source2bits;
    for (; items; masks++, items--) {
	PMask mask = masks->mask;
	int width, destinc, x, y, shiftl, shiftr;
	short height;
	uchar *sp;
	PSCANTYPE destunit;
	
	if (!(width = mask->width)) continue;
	if (width > 16 || !mask->unused) break;
	x = masks->dc.x + args->markInfo->offset.x;
	x <<= 1;
	y = masks->dc.y + args->markInfo->offset.y;
	destinc = framebytewidth;
	destunit = (PSCANTYPE)((int)framebase + y*destinc) + (x >> SCANSHIFT);
	sp = mask->data;
	height = mask->height - 1;
	shiftr = x & SCANMASK;
	if (width <= 8) {
	    if (shiftr <= 16)
		do {
		    t =*((PCard16)(table+(*sp++*2))) LSHIFT (16-shiftr);
		    *destunit |= t;
		    destunit = (PSCANTYPE)((integer)destunit + destinc);
		} while (--height != -1);
	    else {
	        shiftr -= 16;
		do {
		    t = *((PCard16)(table+(*sp++*2)));
		    *destunit     |= t RSHIFT shiftr;
		    *(destunit+1) |= t LSHIFT (SCANUNIT - shiftr);
		    destunit = (PSCANTYPE)((integer)destunit + destinc);
		} while (--height != -1);
	    }
	} else {
	    do {
		t  = *((PCard16)(table + (*sp++*2))) << 16;
		t |= *((PCard16)(table + (*sp++*2)));
		*destunit      |= (t RSHIFT shiftr);
		*(destunit+1)  |= (t LSHIFT (SCANUNIT - shiftr));
		destunit = (PSCANTYPE)((integer)destunit + destinc);
	    } while (--height != -1);
	}
    }
    if (items > 0) BlackMasksMarkReal(masks, items, args);
}

public procedure BlackMasksMarkReal(masks, items, args)
  DevMask *masks; integer items; MarkArgs *args; {
  DevMarkInfo *info = args->markInfo;
  integer xoffset = info->offset.x;
  integer yoffset = info->offset.y;

  for (; items--; masks++) {
#if (MASKCOMPRESSION)
    boolean decompressing = false;
#endif
    PMask mask = masks->mask;
    integer height = mask->height;
#if (MULTICHROME == 1)
    integer wunits = ((mask->width << framelog2BD) + SCANUNIT - 1) >> SCANSHIFT;
#else
    integer wunits = (mask->width + SCANUNIT - 1) >> SCANSHIFT;
#endif
    integer sourceinc = ((mask->width + SCANUNIT - 1) >> SCANSHIFT) * sizeof(SCANTYPE);
    integer destinc = framebytewidth - (wunits * sizeof(SCANTYPE));
    register PSCANTYPE sourceunit = (PSCANTYPE)(mask->data), destunit;
    DevPoint dc;
    register integer shiftl, shiftr;
    SCANTYPE expand[25];

    if (wunits == 0) continue;
    if (mask->unused) ExpandMask(mask, sourceunit = expand);
    dc.x = masks->dc.x + xoffset;
#if (MULTICHROME == 1)
    dc.x <<= framelog2BD;
#endif
    dc.y = masks->dc.y + yoffset;
    shiftr = dc.x & SCANMASK;
    shiftl = SCANUNIT - shiftr;
#if DPSXA
  destunit = (PSCANTYPE) ((integer) framebase
    	+ ((devXAOffset.x << framelog2BD) >> 3) 
		+ (dc.y + devXAOffset.y) * framebytewidth)
		+ (dc.x >> SCANSHIFT);
#else /* DPSXA */
    destunit = (PSCANTYPE)((integer)framebase + dc.y * framebytewidth) +
      (dc.x >> SCANSHIFT);
#endif /* DPSXA */

    if (mask->maskID == BANDMASKID) {
      /* The given mask is being imaged for a band device */
#if (MASKCOMPRESSION)
      if (*sourceunit != -1) {
	decompressing = true;
	SetUpDecompression((PCard8)(sourceunit+1), 0);
	sourceunit = DecompressOneLine();
        Assert(sourceunit);
	}
      else
#endif
      sourceunit++;
      }
      
#if (MULTICHROME == 1)
    switch (framelog2BD) {
    
    case 0:
#endif
      if (shiftr == 0) {
	do {
	  register integer w = wunits;
	  do {
	    *(destunit++) |= *(sourceunit++);
	    } while (--w != 0);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      else {
	do {
	  register integer w = wunits;
	  do {
	    register SCANTYPE t = *(sourceunit++);
	    *(destunit++) |= (t RSHIFT shiftr);
	    *destunit |= (t LSHIFT shiftl);
	    } while (--w != 0);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
#if (MULTICHROME == 1)
      break;

    case 1:
      if (shiftr == 0) {
	do {
	  integer w = wunits;
	  register unsigned char *sourcebyte, index;
	  SCANTYPE t;
	  sourcebyte = ((unsigned char *)sourceunit);
	  do {
	    t  = *((PCard16)((char *)source2bits + (*sourcebyte++ * 2))) << 16;
	    t |= *((PCard16)((char *)source2bits + (*sourcebyte++ * 2)));
	    *(destunit++) |= t;
	  } while (--w != 0);
	  sourceunit = (PSCANTYPE)((integer)sourceunit + sourceinc);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      else {
	do {
	  integer w = wunits;
	  register unsigned char *sourcebyte, index;
	  SCANTYPE t;
	  sourcebyte = ((unsigned char *)sourceunit);
	  do {
	    t  = *((PCard16)((char *)source2bits + (*sourcebyte++ * 2))) << 16;
	    t |= *((PCard16)((char *)source2bits + (*sourcebyte++ * 2)));
	    *(destunit)   |= (t RSHIFT shiftr);
	    *(destunit+1) |= (t LSHIFT shiftl);
	    destunit++;
	    } while (--w != 0);
	  sourceunit = (PSCANTYPE)((integer)sourceunit + sourceinc);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      break;
      

    case 2:
      if (shiftr == 0) {
	do {
	  integer w = wunits;
	  register unsigned char *sourcebyte, index;
	  sourcebyte = ((unsigned char *)sourceunit);
	  do {
	    if (index = *sourcebyte++)
	      *(destunit) |= *((PSCANTYPE)((char *)source4bits + (index * 4)));
	    destunit++;
	  } until (--w == 0);
	  sourceunit = (PSCANTYPE)((integer)sourceunit + sourceinc);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      else {
	do {
	  register integer w = wunits;
	  unsigned char *sourcebyte, index;
	  sourcebyte  = (unsigned char *)sourceunit;
	  do {
	    SCANTYPE *sourcebits;
	    register SCANTYPE t;
	    if (index = *sourcebyte++) {
	      t = *((PSCANTYPE)((char *)source4bits + (index * 4)));
	      *(destunit)   |= (t RSHIFT shiftr);
	      *(destunit+1) |= (t LSHIFT shiftl);
	    }
	    destunit++;
	  } until (--w == 0);
	  sourceunit = (PSCANTYPE)((integer)sourceunit + sourceinc);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      break;
      

    case 3:
      if (shiftr == 0) {
	do {
	  integer w = wunits;
	  register unsigned char *sourcebyte, index;
	  sourcebyte  = ((unsigned char *)sourceunit);
	  while (true) {
	    if (w == 1) {
	      if (index = *sourcebyte)
		*(destunit) |= *((PSCANTYPE)((char *)source8bits + (index * 8)));
	      destunit++;
	      break;
	    }
	    if (index = *sourcebyte) {
              PSCANTYPE sourcebits = (PSCANTYPE)((char *)source8bits + (index * 8));
	      *(destunit)   |= *(sourcebits);
	      *(destunit+1) |= *(sourcebits+1);
	    } 
	    destunit += 2;
	    if ((w -= 2) == 0) break;
	    sourcebyte++;
	  } 
	  sourceunit = (PSCANTYPE)((integer)sourceunit + sourceinc);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      else {
	do {
	  register integer w = wunits;
	  unsigned char *sourcebyte, index;
	  sourcebyte = (unsigned char *)sourceunit;
	  while (true) {
	    SCANTYPE *sourcebits;
	    register SCANTYPE t;
	    if (w == 1) {
	      if (index = *sourcebyte) {
		t = *((PSCANTYPE)((char *)source8bits + (index * 8)));
		*(destunit) |= (t RSHIFT shiftr);
		*(destunit+1) |= (t LSHIFT shiftl);
	      }
	      destunit++;
	      break;
	    }
	    if (index = *sourcebyte) {
	      sourcebits = (PSCANTYPE)((char *)source8bits + (index * 8));
	      if (t = *(sourcebits)) {
		*(destunit)   |= (t RSHIFT shiftr);
		*(destunit+1) |= (t LSHIFT shiftl);
	      }
	      if (t = *(sourcebits+1)) {
		*(destunit+1) |= (t RSHIFT shiftr);
		*(destunit+2) |= (t LSHIFT shiftl);
	      } 
	    }
	    destunit += 2;
	    if ((w -= 2) == 0) break;
	    sourcebyte++;
	  }
	  sourceunit = (PSCANTYPE)((integer)sourceunit + sourceinc);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      break;

    case 4:
      if (shiftr == 0) {
	do {
	  register integer w = wunits;
	  integer i = 0;
	  SCANTYPE su = *(sourceunit++), du;
	  do {
	    if (i++ == 16) { i = 0; su = *(sourceunit++); }
	    du = 0;
	    if (su & 0x80000000) du = 0xFFFF0000; su <<= 1; 
	    if (su & 0x80000000) du |= 0xFFFF; su <<= 1;
	    (*destunit++) = du;
	  } while (--w != 0);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      else {
	do {
	  register integer w = wunits;
	  integer i = 0;
	  SCANTYPE su = *(sourceunit++), du;
	  do {
	    if (i++ == 16) { i = 0; su = *(sourceunit++); }
	    du = 0;
	    if (su & 0x80000000) du = 0xFFFF0000; su <<= 1; 
	    if (su & 0x80000000) du |= 0xFFFF; su <<= 1;
	    *(destunit++) |= du RSHIFT shiftr;
	    *(destunit)   |= du LSHIFT shiftl;
	  } while (--w != 0);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      break;

    case 5:
      if (shiftr == 0) {
	do {
	  register integer w = wunits;
	  integer i = 0;
	  SCANTYPE su = *(sourceunit++);
	  do {
	    if (i++ == 32) { i = 0; su = *(sourceunit++); }
	    if (su & 0x80000000) *destunit |= 0xFFFFFFFF;
	    su <<= 1; destunit++;
	  } while (--w != 0);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      else {
	do {
	  register integer w = wunits;
	  integer i = 0;
	  SCANTYPE tl = 0xFFFFFFFF LSHIFT shiftl;
	  SCANTYPE tr = 0xFFFFFFFF RSHIFT shiftr;
	  SCANTYPE su = *(sourceunit++);
	  do {
	    if (i++ == 32) { i = 0; su = *(sourceunit++); }
	    if (su & 0x80000000) {
	      *(destunit)   |= tr;
	      *(destunit+1) |= tl;
	    }
	    su <<= 1; destunit++;
	  } while (--w != 0);
	  destunit = (PSCANTYPE)((integer)destunit + destinc);
#if (MASKCOMPRESSION)
	  if (decompressing) {
	    sourceunit = DecompressOneLine();
	    Assert(sourceunit || height == 1);
	    }
#endif
	  } while (--height != 0);
	}
      break;

      } /* switch */
#endif
    } /* foreach mask */
  } /* BlackMasksMark */

