/*
  init_table.c

Copyright (c) 1986, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ivor Durham: November 11, 1986
Edit History:
Ivor Durham: Sat Aug 27 14:17:17 1988
Ed Taft: Tue Jan  9 11:09:26 1990
Jim Sandman: Mon Nov  6 12:26:04 1989
Joe Pasqua: Wed Feb  8 14:42:40 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT

#include COPYRIGHT

extern void VMInit(),
  PostScriptInit(),
  LanguageInit(),
  StoDevSupportInit(),
  StoDevEditInit(),
  GraphicsInit(),
  PathExtraOpsInit(),
  FontsInit(),
  FontRunInit(),
#if ISP==isp_vax
  VersatecDevInit(),
#endif ISP==isp_vax
  UnixInit(),
#if STAGE==DEVELOP
  DebugInit(),
#endif STAGE==DEVELOP
  ProductInit(),
  UnixCustomOpsInit(),
  BitmapDevInit();
/*  IniUxDev();    */  /* writing frame to file. Device impl changed. */
/*  DownloadInit(); */

/*
 * packageInitProcedure an is (array of) (pointer to) (function returning) int.
 * Be careful if you change the ordering of the following.
 * Entries not commented as optional are mandatory.
 */

void    (*packageInitProcedure[]) ( /* InitReason */ ) = {
  VMInit,		/* must be first */
  LanguageInit,
  StoDevSupportInit,	/* optional storage device support */
  StoDevEditInit,	/* optional line editing device support */
  GraphicsInit,
  PathExtraOpsInit,	/* optional arcc operator */
  FontsInit,
  FontRunInit,		/* fontrun operator, required with filefindfont.ps */
  PostScriptInit,
#if ISP==isp_vax
  VersatecDevInit,	/* optional Versatec device support */
#endif ISP==isp_vax
  UnixInit,		/* optional Unix OS operators */
#if STAGE==DEVELOP
  DebugInit,
#endif STAGE==DEVELOP
  UnixCustomOpsInit,	/* Load customdict */
  BitmapDevInit,
  /* IniUxDev,	*/	/* frametofile, frametohexfile */
  ProductInit,		/* optional other product-specific initialization */
/*  BlackBoxInit,*/		/* optional black box initialization */
/*  DownloadInit,*/		/* download and ROM patch -- this should be last */
  0
};
