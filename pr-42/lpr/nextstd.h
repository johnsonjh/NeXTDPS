
/*
 *	nextstd.h
 *	Copyright 1987 NeXT, Inc.
 *	Written by Trey Matteson
 *
 *	This file has some standard macros useful in any program.
 */

#ifndef NEXTSTD_H
#define NEXTSTD_H

#include <math.h>

#ifndef MAX
#define  MAX(A,B)	((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define  MIN(A,B)	((A) < (B) ? (A) : (B))
#endif
#ifndef ABS
#define  ABS(A)		((A) < 0 ? (-(A)) : (A))
#endif

extern char *malloc();
#define  NX_MALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) malloc( (unsigned)(NUM)*sizeof(TYPE) )) 

extern char *realloc();
#define  NX_REALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) realloc((char *)(VAR), (unsigned)(NUM)*sizeof(TYPE)))

#define  NX_FREE( PTR )	free( (char *) (PTR) );

#ifndef NBITSCHAR
#define NBITSCHAR	8
#endif NBITSCHAR

#ifndef NBITSINT
#define NBITSINT	(sizeof(int)*NBITSCHAR)
#endif NBITSINT

#ifndef TRUE
#define TRUE		1
#endif TRUE
#ifndef FALSE
#define FALSE		0
#endif FALSE

#ifndef NX_ASSERT
#ifdef DEBUG
#define NX_ASSERT(exp,str)		{if(!(exp)) fprintf( stderr, "Assertion failed: %s\n", str );}
#else DEBUG
#define NX_ASSERT(exp,str) {}
#endif DEBUG
#endif NX_ASSERT

#define NX_PSDEBUG	 {}		/* ??? temp disable */
#ifndef NX_PSDEBUG
#ifdef DEBUG
#define NX_PSDEBUG		{if (DPSDebugPSOn) DPSPrintf( DPSGetCurrentContext(),"\n%% *** Debug *** Object:%d Class:%s Method:%s\n", ((int)self), ((char *)[self name]), selName(_cmd));}
#else DEBUG
#define NX_PSDEBUG {}
#endif DEBUG
#endif NX_PSDEBUG

#endif
