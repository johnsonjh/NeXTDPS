/*
				 vm_space.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All information contained  herein is the  property  of  Adobe Systems
Incorporated.  Many of   the intellectual  and  technical concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees  for  their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless  prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: 
Edit History:
Scott Byer: Wed May 17 09:52:56 1989
Ivor Durham: Sun Aug 28 08:46:30 1988
Ed Taft: Wed Jun 17 15:42:01 1987
Jim Sandman: Wed Mar  9 21:41:41 1988
Perry Caro: Mon Nov  7 13:14:08 1988
Joe Pasqua: Mon Jan  9 16:41:48 1989
End Edit History.
*/

/*
 * This module handles space allocation from the C-environment "malloc"
 * world.  Use of this module may be monitored by defining the symbol SPACE
 * to be true.
 */

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include PSLIB
#include STREAM
#include VM

/*
 * State
 */

#if SPACE

#if (OS != os_mpw)
/*-- BEGIN GLOBALS --*/
private FILE *spaceFile;
private integer Zchunks;
private integer NZchunks;
/*-- END GLOBALS --*/
#else (OS != os_mpw)

typedef struct {
 FILE	*g_spaceFile;
 integer	 g_Zchunks[256];
 integer	 g_NZchunks[256];
} GlobalsRec, *Globals;

private Globals globals;

#define spaceFile globals->g_spaceFile
#define Zchunks globals->g_Zchunks
#define NZchunks globals->g_NZchunks

#endif (OS != os_mpw)
#endif SPACE


#if	SPACE
private procedure PSPrintChunks()
{
  integer i,
          Zt,
          NZt;

  Zt = NZt = 0;
  os_printf ("\t%s%s\n", "non-zero", "  zero");

  for (i = 0; i < 16; i++) {
    os_printf ("%3d\t%8d%8d\n", i, NZchunks[i], Zchunks[i]);
    Zt += Zchunks[i];
    NZt += NZchunks[i];
  }

  for (i = 16; i < 256; i++) {
    os_printf ("%3d\t\t%8d\n", i, Zchunks[i]);
    Zt += Zchunks[i];
  }

  os_printf ("\t%8d%8d\n", NZt, Zt);
}				/* end of PSPrintChunks */
#endif	SPACE

public charptr MNew(n, size)  integer n, size;
{   /* NB! this proc allocates from unix subroutine package, not VM */  
  charptr c;
#if	SPACE
  register integer regpc; /* bugpc = d7(SUN),r11(VAX) */
  integer allocpc, amount;
#if	MC68K
  asm("	movl	a6@(4),d7");	/* pick up caller's PC */
#else	MC68K  /* assume VAX */
  asm ("	movl	16(fp),r11");	/* pick up caller's PC */
#endif	MC68K
  allocpc = regpc;
#endif	SPACE

  if ((c = (charptr) os_calloc ((long int) n, (long int) size)) == NIL)
    LimitCheck ();

#if	SPACE
  if (fwrite (&allocpc, 4, 1, spaceFile) != 1)
    CantHappen ();
  if (fwrite (&c, 4, 1, spaceFile) != 1)
    CantHappen ();

  amount = n * size;

  if (fwrite (&amount, 4, 1, spaceFile) != 1)
    CantHappen ();
#endif	SPACE

  return c;
}				/* end of MNew */

public charptr EXPAND(current, n, size)  charptr current; integer n, size;
{				/* NB! this proc allocates from unix
				 * subroutine package, not VM */
  charptr c;

  if ((c = (charptr) os_realloc(
    (char *) current, (long int)(n * size))) == NIL)
    VMERROR ();
  return c;
}				/* end of EXPAND */

public procedure MFree(ptr)  charptr ptr;
{
#if	SPACE
  register integer regpc; /* allocpc = d7(SUN),r11(VAX) */
  integer allocpc, amount = -1;
#if	MC68K
  asm("	movl	a6@(4),d7");	/* pick up caller's PC */
#else	MC68K  /* assume VAX */
  asm ("	movl	16(fp),r11");	/* pick up caller's PC */
#endif	MC68K
  allocpc = regpc;

  if (fwrite (&allocpc, 4, 1, spaceFile) != 1)
    CantHappen ();
  if (fwrite (&ptr, 4, 1, spaceFile) != 1)
    CantHappen ();
  if (fwrite (&amount, 4, 1, spaceFile) != 1)
    CantHappen ();
#endif	SPACE

  os_free ((char *) ptr);
}				/* end of MFree */

Init_VM_Space (reason)
  InitReason reason;
{
  switch (reason) {
  case init:
#if SPACE
#if (OS == os_mpw)
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    spaceFile = os_fopen ("space.log", "w");
#endif SPACE
    break;
  case romreg:
#if	SPACE
    RgstExplicit ("printchunks", PSPrintChunks);
#endif	SPACE
    break;
  endswitch
  }
}
