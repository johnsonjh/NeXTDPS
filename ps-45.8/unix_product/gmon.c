/*
  gmon.c

Copyright (c) 1984, '85, '86, '87 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: July 25, 1983
Edit History:
Scott Byer: Thu Jun  1 17:17:33 1989
Chuck Geschke: Thu Jul 19 10:08:02 1984
Ed Taft: Tue Oct  3 15:06:19 1989
Ivor Durham: Mon May 16 17:22:52 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include FP
#include VM
#include "gmon.h"

#ifdef register
#undef register
#endif register

/*
 *	froms is actually a bunch of unsigned shorts indexing tos
 */
static int		profiling;
static unsigned short	*froms;
static struct tostruct	*tos;
static long		tolimit;
static char		*s_lowpc;
static char		*s_highpc;
static unsigned long	s_textsize;
static char		*minsbrk;

static int	ssiz;
static int	*sbuf;

#define	MSG "No space for monitor buffer(s)\n"

/* NOTE: on the Sun you must patch "mcount" and "mcount+2" to nops
   (i.e.) 0x4e71 so that code falls through to this jmp instruction

#if MC68K
asm("	.text");
asm("	jmp mc");
#endif MC68K
*/

extern longcardinal highwatersp;

monstartup(lowpc, highpc)
    char	*lowpc;
    char	*highpc;
{
    int			monsize;
    char		*buffer;
    int			HF = HISTFRACTION;

	/*
	 *	round lowpc and highpc to multiples of the density we're using
	 *	so the rest of the scaling (here and in gprof) stays in ints.
	 */

    profiling = 3;
    tos = 0;
    tolimit = 0;
    minsbrk = 0;
#if MC68K
    while ((((unsigned)(highpc - lowpc))/HF + sizeof(struct phdr)) > 65000)
	{HF++;};
#endif MC68K
    lowpc = (char *)
	    ROUNDDOWN((unsigned)lowpc, HF*sizeof(HISTCOUNTER));
    s_lowpc = lowpc;
    highpc = (char *)
	    ROUNDUP((unsigned)highpc, HF*sizeof(HISTCOUNTER));
    s_highpc = highpc;
    s_textsize = highpc - lowpc;
    monsize = (s_textsize / HF) + sizeof(struct phdr);
    buffer = (char*)NEW(monsize,1);
    froms = (unsigned short *) NEW(s_textsize / HASHFRACTION,1);
    tolimit = s_textsize * ARCDENSITY / 100;
    if (tolimit < MINARCS) {
	tolimit = MINARCS;
    } else if (tolimit > 65534) {
	tolimit = 65534;
    }
    tos = (struct tostruct *) NEW(tolimit * sizeof(struct tostruct),1);
    tos[0].link = 0;
    monitor(lowpc , highpc , buffer , monsize , tolimit);
	/*
	 *	profiling is what mcount checks to see if
	 *	all the data structures are ready!!!
	 */
    profiling = 0;
}

gmoncleanup()
{
    int			fd;
    int			fromindex;
    int			endfrom;
    char		*frompc;
    int			toindex;
    struct rawarc	rawarc;

    fd = creat("gmon.out" , 0666);
    if (fd < 0) {
	perror("mcount: gmon.out");
	return;
    }
    write(fd , sbuf , ssiz);
    endfrom = s_textsize / (HASHFRACTION * sizeof(*froms));
    for (fromindex = 0 ; fromindex < endfrom ; fromindex++) {
	if (froms[fromindex] == 0) {
	    continue;
	}
	frompc = s_lowpc + (fromindex * HASHFRACTION * sizeof(*froms));
	for(toindex=froms[fromindex]; toindex!=0; toindex=tos[toindex].link) {
	    rawarc.raw_frompc = (unsigned long) frompc;
	    rawarc.raw_selfpc = (unsigned long) tos[toindex].selfpc;
	    rawarc.raw_count = tos[toindex].count;
	    write(fd , &rawarc , sizeof rawarc);
	}
    }
    close(fd);
}

#if __GNU__ /* If using Gnu CC */
/* Assumption is that this file is not compiled with -pg or -p */

asm(" .globl mcount;");
asm("mcount:");	/* point mcount at _mcount */
mcount()
{
	register char			*selfpc;
	register unsigned short		*frompcindex;
	register struct tostruct	*top;
	register struct tostruct	*prevtop;
	register long			toindex;
	static int s;

	if (profiling)
		goto out;
	/*
	 *	find the return address for mcount,
	 *	and the return address for mcount's caller.
	 */
	asm("	.text");	/* make sure we're in text space */
	asm("	movl a6@,%0" : "=a" (selfpc));	/* Get parents frame pointer */
	asm("	movl %1@(4),%0" : "=a" (frompcindex) : "a" (selfpc));
	asm("	movl a6@(4),%0" : "=a" (selfpc));
	/* Detect recursize calls */
	profiling++;
	/*
	 *	check that frompcindex is a reasonable pc value.
	 *	for example:	signal catchers get called from the stack,
	 *			not from text space.  too bad.
	 */
	frompcindex = (unsigned short *)((long)frompcindex - (long)s_lowpc);
	if ((unsigned long)frompcindex > s_textsize) {
		goto done;
	}
	frompcindex =
	    &froms[((long)frompcindex) / (HASHFRACTION * sizeof(*froms))];
	toindex = *frompcindex;
	if (toindex == 0) {
		/*
		 *	first time traversing this arc
		 */
		toindex = ++tos[0].link;
		if (toindex >= tolimit) {
			goto overflow;
		}
		*frompcindex = toindex;
		top = &tos[toindex];
		top->selfpc = selfpc;
		top->count = 1;
		top->link = 0;
		goto done;
	}
	top = &tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 *	arc at front of chain; usual case.
		 */
		top->count++;
		goto done;
	}
	/*
	 *	have to go looking down chain for it.
	 *	top points to what we are looking at,
	 *	prevtop points to previous top.
	 *	we know it is not at the head of the chain.
	 */
	for (; /* goto done */; ) {
		if (top->link == 0) {
			/*
			 *	top is end of the chain and none of the chain
			 *	had top->selfpc == selfpc.
			 *	so we allocate a new tostruct
			 *	and link it to the head of the chain.
			 */
			toindex = ++tos[0].link;
			if (toindex >= tolimit) {
				goto overflow;
			}
			top = &tos[toindex];
			top->selfpc = selfpc;
			top->count = 1;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
		/*
		 *	otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 *	there it is.
			 *	increment its count
			 *	move it to the head of the chain.
			 */
			top->count++;
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}

	}
done:
	profiling--;
out:

	return;

overflow:
	profiling = 3;
	printf("mcount: tos overflow\n");
	goto out;
}
#else __GNU__
#if !MC68K
asm("	.data");
asm("	.lcomm _sr11,4");
asm("	.lcomm _sr10,4");
asm("	.lcomm _sr9,4");
asm("	.lcomm _sr8,4");
asm("	.lcomm _sr7,4");
#else !MC68K
asm("	.bss");
asm("	.even");
asm("_sa5:	.skip 4");
asm("_sa4:	.skip 4");
asm("_sa3:	.skip 4");
asm("_sa2:	.skip 4");
asm("_sd7:	.skip 4");
#endif !MC68K


private priv_mcount()
{
	register char			*selfpc;	/* r11 , a5 */
	register unsigned short		*frompcindex;	/* r10 , a4  */
	register struct tostruct	*top;		/* r9  , a3 */
	register struct tostruct	*prevtop;	/* r8  , a2 */
	register long			toindex;	/* r7  , d7 */

	/*
	 *	find the return address for mcount,
	 *	and the return address for mcount's caller.
	 */
	asm("	.text");		/* make sure we're in text space */
asm("	.globl mcount");
asm("mcount:");
#if !MC68K

	asm("	movl r11,_sr11");	
	asm("	movl r10,_sr10");	
	asm("	movl r9,_sr9");	
	asm("	movl r8,_sr8");	
	asm("	movl r7,_sr7");	
	asm("	movl (sp), r11");	/* selfpc = ... (jsb frame) */
	asm("	movl 16(fp), r10");	/* frompcindex =     (calls frame) */
#else !MC68K
	asm("	movl a5,_sa5");	
	asm("	movl a4,_sa4");	
	asm("	movl a3,_sa3");	
	asm("	movl a2,_sa2");	
	asm("	movl d7,_sd7");	
	asm("	movl sp@, a5");	/* selfpc = ... (jsb frame) */
	asm("	movl a6@(4), a4");	/* frompcindex =     (calls frame) */
#endif !MC68K

	/*
	 *	check that we are profiling
	 *	and that we aren't recursively invoked.
	 */
#if !MC68K
	asm("	movl sp,r7"); /* r7 == toindex */
#else !MC68K
	asm("	movl sp,d7"); /* d7 == toindex */
#endif !MC68K
        if (highwatersp > (longcardinal)toindex)
 	   highwatersp = (longcardinal)toindex;
	if (profiling) {
		goto out;
	}
	profiling++;
	/*
	 *	check that frompcindex is a reasonable pc value.
	 *	for example:	signal catchers get called from the stack,
	 *			not from text space.  too bad.
	 */
	frompcindex = (unsigned short *)((long)frompcindex - (long)s_lowpc);
	if ((unsigned long)frompcindex > s_textsize) {
		goto done;
	}
	frompcindex =
	    &froms[((long)frompcindex) / (HASHFRACTION * sizeof(*froms))];
	toindex = *frompcindex;
	if (toindex == 0) {
		/*
		 *	first time traversing this arc
		 */
		toindex = ++tos[0].link;
		if (toindex >= tolimit) {
			goto overflow;
		}
		*frompcindex = toindex;
		top = &tos[toindex];
		top->selfpc = selfpc;
		top->count = 1;
		top->link = 0;
		goto done;
	}
	top = &tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 *	arc at front of chain; usual case.
		 */
		top->count++;
		goto done;
	}
	/*
	 *	have to go looking down chain for it.
	 *	top points to what we are looking at,
	 *	prevtop points to previous top.
	 *	we know it is not at the head of the chain.
	 */
	for (; /* goto done */;) {
		if (top->link == 0) {
			/*
			 *	top is end of the chain and none of the chain
			 *	had top->selfpc == selfpc.
			 *	so we allocate a new tostruct
			 *	and link it to the head of the chain.
			 */
			toindex = ++tos[0].link;
			if (toindex >= tolimit) {
				goto overflow;
			}
			top = &tos[toindex];
			top->selfpc = selfpc;
			top->count = 1;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
		/*
		 *	otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 *	there it is.
			 *	increment its count
			 *	move it to the head of the chain.
			 */
			top->count++;
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}

	}
done:
	profiling--;
	/* and fall through */
out:
#if !MC68K
	asm("	movl _sr11,r11");	
	asm("	movl _sr10,r10");	
	asm("	movl _sr9,r9");	
	asm("	movl _sr8,r8");	
	asm("	movl _sr7,r7");	
	asm("	rsb");
#else !MC68K
	asm("	movl _sa5,a5");
	asm("	movl _sa4,a4");
	asm("	movl _sa3,a3");
	asm("	movl _sa2,a2");
	asm("	movl _sd7,d7");
	asm("	rts");
#endif !MC68K

overflow:
	profiling++; /* halt further profiling */
#   define	TOLIMIT	"mcount: tos overflow\n"
	write(2, TOLIMIT, sizeof(TOLIMIT));
	goto out;
}
#endif __GNU__

/*VARARGS1*/
monitor(lowpc , highpc , buf , bufsiz , nfunc)
    char	*lowpc;
    char	*highpc;
    int		*buf, bufsiz;
    int		nfunc;	/* not used, available for compatability only */
{
    register o;

    if (lowpc == 0) {
	profiling++; /* halt profiling */
	profil((char *) 0 , 0 , 0 , 0);
	gmoncleanup();
	return;
    }
    sbuf = buf;
    ssiz = bufsiz;
    ((struct phdr *) buf) -> lpc = lowpc;
    ((struct phdr *) buf) -> hpc = highpc;
    ((struct phdr *) buf) -> ncnt = ssiz;
    o = sizeof(struct phdr);
    buf = (int *) (((int) buf) + o);
    bufsiz -= o;
    if (bufsiz <= 0)
	return;
    o = (((char *) highpc - (char *) lowpc));
    if(bufsiz < o)
	o = ((float) bufsiz / o) * 65536;
    else
	o = 65536;
    profil(buf , bufsiz , lowpc , o);
}

