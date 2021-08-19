/*
  malloc.c

Copyright (c) 1986, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

This module is derived from a C library module provided to Adobe
under a Unix source license from AT&T Bell Laboratories, U. C. Berkeley,
and/or Sun Microsystems. Under the terms of the source license,
this module cannot be further redistributed in source form.

Original version:
Edit History:
Ivor Durham: Wed Apr 13 13:25:00 1988
Leo Hourvitz: Wed 06Oct87 malloc_verify and malloc_debug; all debugging
	code under STAGE==DEVELOP flags instead of 'debug'
Ed Taft: Sun May  1 16:37:31 1988
Paul Rovner: Fri Nov 24 10:32:48 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PSLIB
#include FOREGROUND
#include "malloc.h"

#if MALLOC_DEBUG
int allocDebugLevel;
#else
static
#endif
  union store	 allocs[2],	/*initial arena*/
		*allocp,	/*search ptr*/
		*alloct,	/*arena top*/
		*allocx;	/*for benefit of realloc*/

public char *os_malloc(nbytes)
long int nbytes;
{
	register union store *p, *q;
	register long int nw;
	static long int temp;	/*coroutines assume no auto*/

	FGEnterMonitor();

	if(allocs[0].ptr==0) {	/*first time*/
		allocs[0].ptr = setbusy(&allocs[1]);
		allocs[1].ptr = setbusy(&allocs[0]);
		alloct = &allocs[1];
		allocp = &allocs[0];
#if MALLOC_DEBUG
		if (allocDebugLevel == 0)
			allocDebugLevel = 1;
#endif MALLOC_DEBUG
	}
	nw = (nbytes+WORD+WORD-1)/WORD;
	MASSERT(allocp>=allocs && allocp<=alloct,allocp);
	MASSERT(allock(),allocp);
#if MALLOC_DEBUG
	if (allocDebugLevel > 1)
		MASSERT(malloc_verify(),0);
#endif MALLOC_DEBUG
	for(p=allocp; ; ) {
		for(temp=0; ; ) {
			if(!testbusy(p->ptr)) {
				while(!testbusy((q=p->ptr)->ptr)) {
					MASSERT(q>p&&q<alloct,q);
					p->ptr = q->ptr;
				}
				if(q>=p+nw && p+nw>=p)
					goto found;
			}
			q = p;
			p = clearbusy(p->ptr);
			if(p>q)
				MASSERT(p<=alloct,p);
			else if(q!=alloct || p!=allocs) {
				MASSERT(q==alloct&&p==allocs,q);
				FGExitMonitor();
				return(NULL);
			} else if(++temp>1)
				break;
		}
		temp = ((nw+BLOCK/WORD)/(BLOCK/WORD))*(BLOCK/WORD);
		q = (union store *)sbrk(0);
		if(q+temp+GRANULE < q) {
			FGExitMonitor();
			return(NULL);
		}
		q = (union store *)sbrk(temp*WORD);
		if((INT)q == (INT)-1) {
			FGExitMonitor();
			return(NULL);
		}
		os_addmem((char *)q, temp*WORD);
	}
found:
	allocp = p + nw;
	MASSERT(allocp<=alloct,allocp);
	if(q>allocp) {
		allocx = allocp->ptr;
		allocp->ptr = p->ptr;
	}
	p->ptr = setbusy(allocp);
	FGExitMonitor();
	return((char *)(p+1));
}

/* addmem(ptr, nbytes) adds the memory in the range [ptr, ptr+nbytes) to
   the storage allocator. The caller asserts that ptr is above the
   region of memory already managed by the storage allocator. */

public procedure os_addmem(ptr, nbytes)
char *ptr; long int nbytes;
{
	register union store *p = (union store *)ptr;
	long int temp = (nbytes & (-WORD))/WORD;
	FGEnterMonitor();
	MASSERT(p>alloct,p);
	alloct->ptr = p;
	if(p!=alloct+1)
		alloct->ptr = setbusy(alloct->ptr);
	alloct = p->ptr = p+temp-1;
	alloct->ptr = setbusy(allocs);
	FGExitMonitor();
}

/*	freeing strategy tuned for LIFO allocation
*/
public procedure os_free(ap)
register char *ap;
{
	register union store *p = (union store *)ap;
	FGEnterMonitor();

#if MALLOC_DEBUG
	if (allocDebugLevel > 1)
		MASSERT(malloc_verify(),0);
#endif MALLOC_DEBUG
	MASSERT(p>clearbusy(allocs[1].ptr)&&p<=alloct,p);
	MASSERT(allock(),allocp);
	--p;
	if (p < allocp) allocp = p;  /* make allop point to lowest free blk */
	MASSERT(testbusy(p->ptr),p);
	p->ptr = clearbusy(p->ptr);
	MASSERT(p->ptr > allocp && p->ptr <= alloct,p);
	FGExitMonitor();
}

/*	os_realloc(p, nbytes) reallocates a block obtained from os_malloc()
 *	and freed since last call of os_malloc()
 *	to have new size nbytes, and old content
 *	returns new location, or 0 on failure
*/

public char *os_realloc(p, nbytes)
register union store *p;
long int nbytes;
{
	register union store *q;
	register long int nw;
	long int onw;
	FGEnterMonitor();

#if MALLOC_DEBUG
	if (allocDebugLevel > 1)
		MASSERT(malloc_verify(),0);
#endif MALLOC_DEBUG
	if(testbusy(p[-1].ptr))
		os_free((char *)p);
	onw = p[-1].ptr - p;
	q = (union store *)os_malloc(nbytes);
	if(q==NULL || q==p) {
		FGExitMonitor();
		return((char *)q);
		}
	nw = (nbytes+WORD-1)/WORD;
	if(nw<onw)
		onw = nw;
	os_bcopy((char *) p, (char *) q, onw * WORD);
	if(q<p && q+nw>=p)
		(q+(q+nw-p))->ptr = allocx;
	FGExitMonitor();
	return((char *)q);
}


public char *os_sureMalloc(size)
  long int size;
{
  char *p = (char *)os_malloc(size);
  if (!p) CantHappen();
  return p;
}

/*
	Leo 07Oct87 SunOS-compatible debugging functions for os_malloc
	*/

#if MALLOC_DEBUG

int malloc_debug(level) int level; {
  int oldLevel;
  FGEnterMonitor();
  oldLevel = allocDebugLevel;
  allocDebugLevel = level;
  FGExitMonitor();
  return(oldLevel);
  } /* malloc_debug */

static malloc_failmsg(p,msg,info)
union store *p,*info;
char *msg;
{
	os_eprintf("malloc_verify:failing:at %x:ptr is %x",
		p,clearbusy(p->ptr));
	if (msg)
		os_eprintf(":%s %x.\n",msg,info);
	else
		os_eprintf(".\n");
}

int malloc_verify()
{
	extern	etext;
	register union store *lowp,*highp,*lastp;
	register union store *p;
	
	FGEnterMonitor();
	lowp = (union store *)&etext;
	highp = (union store *)sbrk(0);
	lastp = alloct;
	p = &allocs[0];
	while(1)
	{
		if (clearbusy(p->ptr) < lowp)
		{
		  if (allocDebugLevel)
		    malloc_failmsg(p,"etext at",lowp);
		  FGExitMonitor();
		  return(0);
		}
		if (clearbusy(p->ptr) > highp)
		{
		  if (allocDebugLevel)
		    malloc_failmsg(p,"sbrk(0) at",lowp);
		  FGExitMonitor();
		  return(0);
		}
		if (((int)clearbusy(p->ptr))&(sizeof(ALIGN)-1))
		{
		  if (allocDebugLevel)
		    malloc_failmsg(p,NULL,0);
		  FGExitMonitor();
		  return(0);
		}
		if (p != lastp)
		  if (p >= clearbusy(p->ptr))
		  {
		    if (allocDebugLevel)
		      malloc_failmsg(p,NULL,0);
		    FGExitMonitor();
		    return(0);
		  }
		if (p == lastp)
			break;
		p = clearbusy(p->ptr);
	}
	FGExitMonitor();
	return(1);
} /* malloc_verify */

malloc_printverify()
{
	os_eprintf("malloc_verify = %d\n",malloc_verify());
} /* malloc_printverify */


allock()
{
#ifdef longdebug
	register union store *p;
	long int x;
	x = 0;
	FGEnterMonitor();
	for(p= &allocs[0]; clearbusy(p->ptr) > p; p=clearbusy(p->ptr)) {
		if(p==allocp)
			x++;
	}
	MASSERT(p==alloct,p);
	FGExitMonitor();
	return(x==1|p==allocp);
#else
	return(1);
#endif
}

malloc_botch(s,p)
union store *p;
char *s;
{
	os_eprintf("alloc:assertion '%s' failed for pointer at %x\n",s,p);
	CantHappen();
}


#endif MALLOC_DEBUG
