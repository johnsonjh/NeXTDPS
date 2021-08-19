
/* atm.h */

#ifndef ATM_H
#define ATM_H

/* for ATM, define ATM true */
/* for PostScript, define ATM false */

#define ATM (0)

#if ATM
#define GLOBALCOLORING (1)
#define DEBUG (0)
#define DPS_CODE (0)
#define PPS (0)
#define GSMASK (1)
#else

/* To support single source files for both all PS impls, the
   following switches are set based on the definition of
   DPS_CODE. For dps, DPS_CODE is 1 as defined in environment.h.
   For 2ps, DPS_CODE is 0.
   For 1ps (printer PS), DPS_CODE is 0 and PPS is 1.
   For 1ps with kanji, PPSkanji is 1.
*/

#define PPS 0
#define PPSkanji 1
#define DPS_CODE 0

#ifndef MERCURY
#define MERCURY ((!ATM) && (!PPS))
#else
/* 
 * Check that the earlier definition is consistent with
 * local state of ATM and PPS.
 */
#if MERCURY != ((!ATM) && (!PPS))
/* In a homogenous system (all DPS, PPS or MERCURY) would cause an error. */
#endif
#endif /* MERCURY */

#define GSMASK (0)

#if PPS || DPS_CODE
#define GLOBALCOLORING 0
#define DEBUG 0
#else /* 2ps definitions */
#define GLOBALCOLORING 1
#define DEBUG 0
#endif /* DPS_CODE */
#endif /* ATM */

#endif /* ATM_H */
