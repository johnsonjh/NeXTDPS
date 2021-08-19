/*
  controller.h

Copyright (c) 1986, 1988, 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: 
Edit History:
Ivor Durham: Tue Jan 27 17:13:24 1987
Ed Taft: Sat Jan  6 15:20:06 1990
Bill McCoy: Tue Jan 19 15:40:00 1988
Joel Sacks: Wed Apr 26 11:21:03 1989
End Edit History.
*/

#ifndef	CONTROLLER_H
#define	CONTROLLER_H

#include ENVIRONMENT

/*
 * Controller types
 *
 * The type cont_generic designates a generic "controller" implemented
 * entirely in software. It is used for those ISP-OS combinations that
 * provide no controller-specific hardware support. Implementations
 * that care can distinguish among "generic" controllers using ISP
 * and/or OS.
 *
 * The remaining types designate specific controller architectures, which
 * are combinations of an (implicit) ISP-OS pair and special hardware.
 *
 * Controller types greater than 1000 imply ORINGMEMORY (see below).
 */

#define	cont_generic		1

#define cont_garnet		2
#define cont_emerald		3


/* ORINGMEMORY controllers below here */

/* (cont_sandpiper no longer supported) */
/* (cont_redstone no longer supported) */
/* (cont_blustone no longer supported) */
#define	cont_scout		1004
/* (cont_yelstone no longer supported) */
#define	cont_atlas		1006
#define	cont_swift		1007
#define cont_lps20		1008
#define cont_rca		1009
#define cont_sga		1010
#define cont_spud		1011

#ifndef CONTROLLER_TYPE
ConfigurationError("Unknown controller type");
#endif

/*
 * ORINGMEMORY means that the controller's RAM is multiply mapped into the
 * address space of the CPU. Writing into the duplicate region causes the
 * data from the CPU to be "ored" into memory. (In many controllers,
 * the "oring" memory can be programmed to perform additional functions
 * such as shifting the data or applying a pattern; however, ORINGMEMORY
 * specifies only the basic "oring" capability, whether or not the
 * additional capabilities are present.)
 *
 * If ORINGMEMORY is true, the variable oringincr must contain the value
 * that is added to an ordinary RAM address to obtain the corresponding
 * "oring" memory address.
 */

#ifndef ORINGMEMORY
#define ORINGMEMORY (CONTROLLER_TYPE > 1000)
#endif

/*
 * CONTROLLER_DEFS is the name of the interface containing the hardware
 * definitions for the specific controller selected. (Note that generic
 * "controllers" do not have such definitions; in those cases,
 * CONTROLLER_DEFS will be undefined.)
 */

#if CONTROLLER_TYPE == cont_garnet
#define CONTROLLER_DEFS GARNET
#endif
#if CONTROLLER_TYPE == cont_emerald
#define CONTROLLER_DEFS EMERALD
#endif
#if CONTROLLER_TYPE == cont_scout
#define CONTROLLER_DEFS SCOUT
#endif
#if CONTROLLER_TYPE == cont_atlas
#define CONTROLLER_DEFS ATLAS
#endif
#if CONTROLLER_TYPE == cont_swift
#define CONTROLLER_DEFS SWIFT
#endif
#if CONTROLLER_TYPE == cont_lps20
#define CONTROLLER_DEFS LPS20
#endif
#if CONTROLLER_TYPE == cont_rca
#define CONTROLLER_DEFS RCA
#endif
#if CONTROLLER_TYPE == cont_sga
#define CONTROLLER_DEFS SGA
#endif
#if CONTROLLER_TYPE == cont_spud
#define CONTROLLER_DEFS SPUD
#endif

#endif	CONTROLLER_H
/* v004 burgett Tue May 31 06:59:33 PDT 1988 */
/* v005 lent Tue Aug 23 17:49:48 PDT 1988 */
/* v006 taft Tue Nov 22 09:47:35 PST 1988 */
/* v007 sacks Wed Apr 26 11:26:28 PDT 1989 */
/* v004 taft Tue Jun 27 16:10:49 PDT 1989 */
/* v006 taft Thu Nov 23 14:57:12 PST 1989 */
/* v007 taft Sat Jan 6 15:22:35 PST 1990 */
