/*
  constantmask.c

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

#include "devmark.h"
#include "framemaskdev.h"
#if (MASKCOMPRESSION)
#include "compress.h"
#endif

public procedure ConstantMasksMark(masks, items, args)
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
    register SCANTYPE gray = args->patData.value;
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
    destunit = (PSCANTYPE)((integer)framebase + dc.y * framebytewidth) +
      (dc.x >> SCANSHIFT);

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
	    register SCANTYPE t = *(sourceunit++);
            *destunit = (*destunit & ~t) | (t & gray);
            destunit++;
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
            register SCANTYPE t1 = t LSHIFT shiftl;
            t RSHIFTEQ shiftr;
            *destunit = (*destunit & ~t) | (t & gray);
            destunit++;
            *destunit = (*destunit & ~t1) | (t1 & gray);
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
            *destunit = (*destunit & ~t) | (t & gray);
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
      else {
	do {
	  integer w = wunits;
	  register unsigned char *sourcebyte, index;
	  SCANTYPE t;
	  sourcebyte = ((unsigned char *)sourceunit);
	  do {
	    register SCANTYPE t1;
	    t  = *((PCard16)((char *)source2bits + (*sourcebyte++ * 2))) << 16;
	    t |= *((PCard16)((char *)source2bits + (*sourcebyte++ * 2)));
            t1 = t LSHIFT shiftl;
	    t RSHIFTEQ shiftr;
 	    *destunit = (*destunit & ~t) | (t & gray);
            destunit++;
            *destunit = (*destunit & ~t1) | (t1 & gray);
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
	    register SCANTYPE t = *((PSCANTYPE)((char *)source4bits + (*sourcebyte++ * 4)));
	    *(destunit) = (*destunit & ~t) | (t & gray);
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
	    register SCANTYPE t, t1;
	    t = *((PSCANTYPE)((char *)source4bits + (*sourcebyte++ * 4)));
            t1 = t LSHIFT shiftl;
	    t RSHIFTEQ shiftr;
 	    *destunit = (*destunit & ~t) | (t & gray);
            destunit++;
            *destunit = (*destunit & ~t1) | (t1 & gray);
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
	    register SCANTYPE t, t1, *sourcebits;
	    if (w == 1) {
	      t = *((PSCANTYPE)((char *)source8bits + (*sourcebyte * 8)));
	      *(destunit) = (*destunit & ~t) | (t & gray);
	      destunit++;
	      break;
	    }
            sourcebits = (PSCANTYPE)((char *)source8bits + (*sourcebyte++ * 8));
	    t = *(sourcebits);
	    *(destunit++)   = (*destunit & ~t) | (t & gray);
	    t = *(sourcebits+1);
	    *(destunit++) = (*destunit & ~t) | (t & gray);
	    if ((w -= 2) == 0) break;
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
	    register SCANTYPE t,t1;
	    if (w == 1) {
	      t = *((PSCANTYPE)((char *)source8bits + (*sourcebyte * 8)));
	      t1 = t LSHIFT shiftl;
	      t RSHIFTEQ shiftr;
	      *destunit = (*destunit & ~t) | (t & gray);
	      destunit++;
	      *destunit = (*destunit & ~t1) | (t1 & gray);
	      break;
	    }
	    sourcebits = (PSCANTYPE)((char *)source8bits + (*sourcebyte++ * 8));
	    t = *(sourcebits);
            t1 = t LSHIFT shiftl;
	    t RSHIFTEQ shiftr;
 	    *destunit = (*destunit & ~t) | (t & gray);
            destunit++;
            *destunit = (*destunit & ~t1) | (t1 & gray);
	    t = *(sourcebits+1);
            t1 = t LSHIFT shiftl;
	    t RSHIFTEQ shiftr;
 	    *destunit = (*destunit & ~t) | (t & gray);
            destunit++;
            *destunit = (*destunit & ~t1) | (t1 & gray);
	    if ((w -= 2) == 0) break;
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
	    if (i++ == 16) { i = 1; su = *(sourceunit++); }
	    du = 0;
	    if (su & 0x80000000) du = 0xFFFF0000; su <<= 1; 
	    if (su & 0x80000000) du |= 0xFFFF; su <<= 1;
	    (*destunit++) = (*destunit & ~du) | (du & gray);
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
	    SCANTYPE dur, dul;
	    if (i++ == 16) { i = 1; su = *(sourceunit++); }
	    du = 0;
	    if (su & 0x80000000) du = 0xFFFF0000; su <<= 1; 
	    if (su & 0x80000000) du |= 0xFFFF; su <<= 1;
	    dur = du RSHIFT shiftr; dul = du LSHIFT shiftl;
	    *(destunit++) = (*destunit & ~dur) | (dur & gray);
	    *(destunit)   = (*destunit & ~dul) | (dul & gray);
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
	    if (i++ == 32) { i = 1; su = *(sourceunit++); }
	    if (su & 0x80000000) *destunit = gray;
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
	    if (i++ == 32) { i = 1; su = *(sourceunit++); }
	    if (su & 0x80000000) {
	      *(destunit)   = (*destunit & ~tr) | (tr & gray);
	      *(destunit+1) = (*destunit & ~tl) | (tl & gray);
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
  } /* ConstantMasksMark */

