
/* bmlocal.c
 * BMClass : LocalBMClass
 * abstract superclasses for all locally stored bitmaps

	Modified:
	
	12Aug90 Terry	Changed bPtr to boundsPrim in LBMMark()

 */

#import PACKAGE_SPECS
#undef MONITOR
#import <mach.h>
#import "bitmap.h"

extern DevProcs *wdProcs;
extern PMarkProcs wdMarkProcs;
extern PImageProcs fmImageProcs;
extern Card8 framelog2BD;
extern DevHalftone *defaultHalftone;

static LocalBMClass *current;

static Bitmap *LBMNew(BMClass *class, Bounds *b, int vmAllocate)
{
    LocalBitmap *lbm;
    int size, rowBytes;

    /* i.e. `super new' */
    lbm = (LocalBitmap *)(*bmClass->new)(class, b, vmAllocate);

    bm_sizeBits(class, b, &size, &rowBytes);
    lbm->rowBytes = rowBytes;
    size += 4;
    if (vmAllocate) {
	vm_allocate(task_self(), (vm_address_t *)&lbm->bits, size, true);
	lbm->freeMethod = LBM_VMDEALLOCATE;
    } else {
	lbm->bits = (unsigned int *)malloc(size);
	lbm->freeMethod = LBM_FREE;
    }
    lbm->byteSize = size;
    return((Bitmap *)lbm);
}

static Bitmap *LBMNewFromData(LocalBMClass *class, Bounds *b,
			      void *bits, void *abits, int byteSize,
			      int rowBytes, int vram, int freeMethod)
{
    LocalBitmap *lbm;
    
    /* i.e. `super new' */
    lbm = (LocalBitmap *)(*bmClass->new)((BMClass *)class, b, false);

    lbm->rowBytes = rowBytes;
    lbm->bits = bits;
    lbm->abits = abits;
    lbm->byteSize = byteSize;
    lbm->freeMethod = freeMethod;
    return((Bitmap *)lbm);
}

static void LBMNewAlpha(Bitmap *bm, int initialize)
{
    int size;
    unsigned int *ap;
    LocalBitmap *lbm = (LocalBitmap *)bm;

    if (bm->alpha || bm->vram) return;
    bm->alpha = 1;
    if(lbm->freeMethod == LBM_VMDEALLOCATE) {
	vm_allocate(task_self(), (vm_address_t *)&lbm->abits, lbm->byteSize,
		    true);
	ap = lbm->abits;
    } else
	ap = lbm->abits = (unsigned int *)malloc(lbm->byteSize);
    if (initialize) {
	size = lbm->byteSize >> 2;
	while(--size >= 0) *ap++ = 0xffffffff;
    }
}

static void LBMFree(Bitmap *bm)
{
    LocalBitmap *lbm = (LocalBitmap *)bm;

    switch(lbm->freeMethod) {
    case LBM_FREE:
	free(lbm->bits);
	if(lbm->abits) free(lbm->abits);
	break;
    case LBM_VMDEALLOCATE:
	vm_deallocate(task_self(), (vm_address_t)lbm->bits, lbm->byteSize);
	if(lbm->abits)
	    vm_deallocate(task_self(), (vm_address_t)lbm->abits,
			  lbm->byteSize);
	break;
    }

    /* i.e. `super free' */
    (*bmClass->_free)(bm);
}

static Bitmap *LBMMakePublic(Bitmap *bm, Bounds *hintBounds, int hintDepth)
{
    if (bm->type != NX_OTHERBMTYPE)
	/* We're already public; ignore hints and return a reference to
	 * ourselves.
	 */
	return(bm_dup(bm));

    /* If local bm's are not public, they should be overriding this method! */
    return (Bitmap *) 0;
}


static void LBMBecomePSDevice(LocalBitmap *lbm)
{
    /* install this bitmap as the PostScript rendering device */
	
    /* hack: for now, set a static variable to tell us what bitmap class
     the PostScript device code is currently configured for */

    current = (LocalBMClass *)((Bitmap *)lbm)->isa;

    /* now install our wakeup vector */
    wdProcs->Wakeup = ((LocalBMClass *) ((Bitmap *)lbm)->isa)->wakeup;

    /* and do it */
    (*wdProcs->Wakeup)(NULL);
}


/* PostScript related Device vectors (these folks are generic enough to be
 * defined in this class)
 */
static void LBMWakeUp(PDevice device)
{
    wdProcs->Proc1 = (PIntProc) current->setupMark;
    wdMarkProcs->SetupImageArgs = current->setupImageArgs;
    framelog2BD = current->log2bd;
    defaultHalftone = current->defaultHalftone;
}

static void LBMSetupImageArgs(PDevice device, ImageArgs *args)
{
    *args = current->bmImArgs;
}

static void LBMSetupMark(PDevice device, DevPrim **clip, MarkArgs *args)
{
    PNextGSExt ep;
    
    args->procs = wdMarkProcs;
    /* check to see if a special gray pattern is set up */
    if (args->markInfo->priv && (ep = *(PNextGSExt *)args->markInfo->priv) &&
	ep->graypatstate) {
	args->pattern = current->grayPattern;
    } else
	args->pattern = current->bmPattern;
    SetupPattern(args->pattern, args->markInfo, &args->patData);
}

static inline void LBMInitSinglePrim(DevPrim *self, DevPrimType type,
				     DevPrivate *value)
{
    self->type = type;
    self->next = NULL;
    self->items = self->maxItems = 1;
    self->value.value = value;
}

static void LBMAddTrap(DevTrap *t, TrapTrapInfo *tt)
{
    if (tt->trapsPrim.items == MAXTRAPS) {
	bm__mark(tt->bm, tt->mrec, &tt->trapsPrim);
	tt->trapsPrim.items = 0;
	EmptyDevBounds(&tt->trapsPrim.bounds);
    }
    if (t->l.xl < tt->trapsPrim.bounds.x.l)
	tt->trapsPrim.bounds.x.l = t->l.xl;
    if (t->g.xl > tt->trapsPrim.bounds.x.g)
	tt->trapsPrim.bounds.x.g = t->g.xl;
    if ((t->y.g - t->y.l) > 1) {  /* look at xg too */
	if (t->l.xg < tt->trapsPrim.bounds.x.l)
	    tt->trapsPrim.bounds.x.l = t->l.xg;
	if (t->g.xg > tt->trapsPrim.bounds.x.g)
	    tt->trapsPrim.bounds.x.g = t->g.xg;
    }
    if (t->y.l < tt->trapsPrim.bounds.y.l)
	tt->trapsPrim.bounds.y.l = t->y.l;
    if (t->y.g > tt->trapsPrim.bounds.y.g)
	tt->trapsPrim.bounds.y.g = t->y.g;
    tt->trapsPrim.value.trap[tt->trapsPrim.items++] = *t;
}

static void LBMMarkRun(DevRun *intcliprun, TrapTrapInfo *tt)
{
    DevPrim clipPrim;

    LBMInitSinglePrim(&clipPrim, runType, (DevPrivate *)intcliprun);
    clipPrim.bounds = intcliprun->bounds;
    bm__mark(tt->bm, tt->mrec, &clipPrim);
}

static void LBMMark(Bitmap *bm, MarkRec *mrec, Bounds *markBds, Bounds *bpBds)
{
    DevPrim *bPtr = NULL;
    DevTrap boundsTrap;
    DevPrim boundsPrim, *clip, *clipnext;
    DevPoint offset = mrec->info.offset;

    /* Customize device package */
    bm_becomePSDevice(bm);
    
    if (!withinBounds(markBds, bpBds)) { 
	bPtr = &boundsPrim; /* flags existence of boundary clipper */
	boundsTrap.y.l = bpBds->miny - mrec->info.offset.y;
	boundsTrap.y.g = bpBds->maxy - mrec->info.offset.y;
	boundsTrap.l.xl = boundsTrap.l.xg = bpBds->minx - mrec->info.offset.x;
	boundsTrap.g.xl = boundsTrap.g.xg = bpBds->maxx - mrec->info.offset.x;
	LBMInitSinglePrim(&boundsPrim, trapType, (DevPrivate *)&boundsTrap);
	boundsPrim.bounds.y = boundsTrap.y;
	boundsPrim.bounds.x.l = boundsTrap.l.xl;
	boundsPrim.bounds.x.g = boundsTrap.g.xl;
    } /* boundsTrap in window coords */

    mrec->info.offset.x -= bm->bounds.minx;
    mrec->info.offset.y -= bm->bounds.miny;

    /* now window coords plus offset yields bitmap coords */ 
    if (!bPtr)
	bm__mark(bm, mrec, mrec->clip);
    else if (!mrec->clip)
	bm__mark(bm, mrec, bPtr);
    else
	for (clip = mrec->clip; clip; clip = clip->next)
	    switch (clip->type) {
	    case noneType:
		break;
	    case trapType: {
		DevTrap *clipTrap = clip->value.trap;
		integer clipItems = clip->items;
		DevInterval inner, outer;
		DevTrap trapList[MAXTRAPS];
		TrapTrapInfo ttinfo;

		LBMInitSinglePrim(&ttinfo.trapsPrim, trapType,
				  (DevPrivate *)trapList);
		ttinfo.mrec = mrec;
		ttinfo.bm = bm;
		ttinfo.trapsPrim.items = 0;
		ttinfo.trapsPrim.maxItems = MAXTRAPS;
		EmptyDevBounds(&ttinfo.trapsPrim.bounds);
		for ( ; clipItems--; clipTrap++)
			switch (BoxTrapCompare(
			&boundsPrim.bounds, clipTrap, &inner, &outer, NULL)) {
			case outside:
			    continue;
			case inside: /* only one clip trap encloses bounds */
			    bm__mark(bm, mrec, &boundsPrim);
			    goto UnShift;
			default:
			    TrapTrapInt(clipTrap, &boundsTrap, NULL,
					LBMAddTrap, &ttinfo);
			    break;
			}
		if (ttinfo.trapsPrim.items > 0)
		    bm__mark(bm, mrec, &ttinfo.trapsPrim);
		break;
	    }
	    case runType:
		switch (DevBoundsCompare(&clip->bounds, &boundsPrim.bounds)) {
		case outside:
		    break;
		case inside: /* clipping run entirely within bounds */
		    clipnext = clip->next;
		    clip->next = NULL;
		    bm__mark(bm, mrec, clip);
		    clip->next =  clipnext;
		    break;
		default: {
		    int i;
		    TrapTrapInfo ttinfo;
		    ttinfo.mrec = mrec;
		    ttinfo.bm = bm;
		    for (i=0; i<clip->items; i++)
			QIntersectTrp(clip->value.run+i, &boundsTrap,
				      LBMMarkRun, &ttinfo);
		}
		    break;
		}
		break;
	    default:
		CantHappen();
		break;
	    }
 UnShift:
    mrec->info.offset = offset;
}

static void LBMOffset(Bitmap *bm, short dx, short dy)
{
    OFFSETBOUNDS(bm->bounds, dx, dy);
}

static int LBMSizeInfo(Bitmap *bm, int *localSize, int *remoteSize)
{
    *remoteSize = 0;
    *localSize = ((LocalBitmap *)bm)->byteSize + bm->isa->_size;
    if (((LocalBitmap *)bm)->abits)
	*localSize += ((LocalBitmap *)bm)->byteSize;
    return *remoteSize + *localSize;
}

static void LBMInitClassVars(BMClass *class)
{
    LocalBMClass *myclass = (LocalBMClass *) class;

    /* initialize superclass */
    (*bmClass->_initClassVars)(class);

    /* now selectively override superclasses' functions where needed */
    class->_free = LBMFree;
    class->_size = sizeof(LocalBitmap);
    class->makePublic = LBMMakePublic;
    class->mark = LBMMark;
    class->newAlpha = LBMNewAlpha;
    class->offset = LBMOffset;
    class->sizeInfo = LBMSizeInfo;
    
    /* now initialize our class by copying from the template below */
    bcopy((char *)localBM + sizeof(BMClass), (char *)class + sizeof(BMClass),
	  sizeof(LocalBMClass) - sizeof(BMClass));
}

/* this structure is filled out for use by subclasses needing a `super'
   method.  This structure is actually never used as the isa pointer of
   a bitmap (it's an abstract superclass)
   */
LocalBMClass _localBM = {
    { 
	0,			/* private initialized boolean */
	LBMInitClassVars,	/* subclass initialization method */
	LBMNew,			/* new method */
	LBMFree,		/* _free method */
	sizeof(LocalBitmap),	/* _size */
	0,			/* composite (subclass resp.) */
	0,			/* convertFrom (subclass resp.) */
	LBMMakePublic,
	LBMMark,
	LBMNewAlpha,
	LBMOffset,
	0,			/* sizeBits (subclass resp.) */
	LBMSizeInfo,
    },
    LBMNewFromData,
    LBMBecomePSDevice,
    0,				/* _mark (subclass resp.) */
    {
	/* bmImargs (subclass resp.) */
    },
    {
	/* bmMarkProcs (subclass resp.) */
    },
    0,	/* bmPattern (subclass resp.) */
    0,	/* grayPattern (subclass resp.) */
    0,	/* defaultHalftone (subclass resp.) */
    0,	/* log2bd (subclass resp.) */
    LBMWakeUp,
    0,	/* setupPattern (subclass resp.) */
    LBMSetupMark,
    LBMSetupImageArgs,
};
