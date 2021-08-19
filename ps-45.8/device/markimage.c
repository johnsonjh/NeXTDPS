/* Display PostScript generic image module

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

Edit History:
15May89 Jack	changed to *D11 from *D1X on Sandman's advice
10Jan90 Terry	Added ImIdent32 case for 32->2 bit ident imaging
22Jan90 Terry	Stop all interleaved alphaimages that don't go to ImIdent32
15Mar90 Jack	brought up to 2000.6 compatible
27Apr90 Terry	Added 1->1 and 1->2 bpp cases to callers of ImStretch
05May90 Terry	Added 8->1 case to callers of ImStretch
05May90 Terry	Only use 8->1 case when horizontal scaleup is 3x or more
06Jun90 Jack	also avoid ImIdent/ImStretch in imagemask case for 2-bit dest!

*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include EXCEPT
#include FP
#include POSTSCRIPT
#include DEVICE
#include DEVIMAGE

#include "devcommon.h"
#include "genericdev.h"
  
/* these are NeXT accellerators, currently of the form S1nD1n */
extern procedure ImIdent();  /* identity xform, no transfer func */
extern procedure ImIdent32();/* identity xform, no transfer func, 32->2 bit */
extern procedure ImStretch();/* non rotated xform, no transfer func */
extern procedure Im110();    /* general source into 32 bit color framebuffer */

private real breakeven = 0.005;

private procedure ImageTrapsOrRun(items, t, run, args)
    integer items;
    DevTrap *t;
    DevRun *run;
    ImageArgs *args;
{
    register DevImage *im = args->image;
    DevImageSource *source = im->source;
    procedure (*improc)();
    
    /* dest bitsPerPixel */
    switch (args->bitsPerPixel)
    {
        case 1:
            switch (source->nComponents)
            {
                case 1:
                    switch (source->bitspersample)
                    {
                        case 1:
			    /* Handle our printer transfer functions, which
			     * have no effect on 1 bit images.
			     */
                            if (!im->imagemask &&
				(!im->transfer || !im->transfer->white ||
			        (im->transfer->white[0] == 0 &&
				im->transfer->white[255] == 255)))
                            {
                                Mtx *m = im->info.mtx;
                                if (m->a > breakeven && m->b == 0.0 &&
				    m->c == 0.0 && m->d > 0.0)
				{
				    improc = ImStretch;
                                    break;
                                }
                            }
                            improc = ImS11D11;
                            break;
                        case 2:
                        case 4:
                        case 8:
                            improc = ImS1XD11;
                            break;
                        default:
                            CantHappen();
                    }
                    break;
                case 3: 
                case 4: 
                    improc = ImSXXD11;
                    break;
                default:
                    CantHappen();
            }
            break;
        case 2:    /* dest bitsPerPixel */
            switch (source->nComponents)
            {
                case 1:
                    switch (source->bitspersample)
                    {
                        case 1:
                        case 2: 
                            if (!im->imagemask &&
			      (!im->transfer || !im->transfer->white))
                            { /* no tranfer func */
                                Mtx *m = im->info.mtx;
                                if (m->a > breakeven && m->b == 0.0 &&
				    m->c == 0.0 && m->d > 0.0)
				{
                                    if (m->a < 1.0001 && m->a > 0.9999 && 
                                        m->d < 1.0001 && m->d > 0.9999 &&
					source->bitspersample == 2)
                                        improc = ImIdent;
				    else
					improc = ImStretch;
                                    break;
                                }
				if (source->bitspersample == 2) {
				    improc = ImS12D12NoTfr;
				    break;
				}
                            } /* else fall through */
                        case 4:
                        case 8:
			    if (source->bitspersample == 1)
				improc = ImS11D12;
			    else
				improc = ImS1XD12;
                            break;
                        default:
                            CantHappen();
                    }
                    break;
                case 3:		    
		    /* Special case 32-bit RGBA interleaved source, 2 bit dest,
		     * identity transform, no transfer function
		     */
                    if(source->bitspersample == 8 && source->interleaved &&
		       im->unused && (!im->transfer || !im->transfer->white)) {
			Mtx *m = im->info.mtx;
			if (m->b == 0.0 && m->c == 0.0 &&
			    m->a < 1.0001 && m->a > 0.9999 && 
			    m->d < 1.0001 && m->d > 0.9999) {
			    improc = ImIdent32;
			    break;
			}
                    }
                    improc = ImSXXD12;
		    break;
                case 4:
                    improc = ImSXXD12;
		    break;
            }
	    break;
	case 16:	/* dest bitsPerPixel */
        case 32:    	/* dest bitsPerPixel */
	    if (im->imagemask)
		improc = ImS11D1X;	/* crashes if NIL pattern */
	    else
		improc = Im110;		/* does imagemasks incorrectly */
            break;
        default: 
            CantHappen();
    }
    
    /* FIX: Temporary firewall against interleaved alphaimages for 2-bit dst */
    if(improc != ImIdent32 && source->interleaved && im->unused &&
       args->bitsPerPixel == 2)
        PSRangeCheck();

    (*improc)(items, t, run, args);
}

public procedure ImageTraps(t, nTraps, args)
  DevTrap *t; integer nTraps; ImageArgs *args; {
  ImageTrapsOrRun(nTraps, t, NULL, args);
  }

public procedure ImageRun(run, args)
  DevRun *run; ImageArgs *args; {
  ImageTrapsOrRun(1, NULL, run, args);
  }











