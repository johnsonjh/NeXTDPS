/* copyright.h for 1990

Edit History:
Ed Taft: Sun Jan  7 11:21:23 1990
End Edit History.
*/

#ifndef PACKAGE_SPECS_H
#include PACKAGE_SPECS
#endif
#include ENVIRONMENT

#ifndef STUTTER
#if OS==os_ps
#define STUTTER (ISP==isp_mc68020? 4 : ISP==isp_mc68010? 2 : 1)
#else
#define STUTTER 1
#endif
#endif

#ifndef LINOTYPE
#define LINOTYPE 0
#endif

#ifndef BRIEF		/* BRIEF applies only when STUTTER==1 */
#define BRIEF 0
#endif

#if STUTTER==1
static char AdobeCpyrt[] =
#if BRIEF && LINOTYPE
"Copyright (c) 1984-1990 Adobe Systems Incorporated.";
#else BRIEF && LINOTYPE
"Copyright (c) 1984-1990 Adobe Systems Incorporated.\n\
All Rights Reserved.";
#endif BRIEF && LINOTYPE

#if LINOTYPE
static char LinoCpyrt[] =
#if BRIEF
"Typefaces Copyright (c) 1981 Linotype AG and/or its subsidiaries.\n\
All Rights Reserved.";
#else BRIEF
"The digitally encoded machine readable outline data for producing the \
Typefaces provided as part of this product is copyrighted (c) 1981 Linotype \
AG and/or its subsidiaries. All Rights Reserved. This data is the property \
of Linotype and may not be reproduced, used, displayed, modified, disclosed \
or transferred without the express written approval of Linotype.";
#endif BRIEF
#endif LINOTYPE
#endif STUTTER==1

#if STUTTER==2
/* Note that the string ends with one space so that the extra null at the
 * end leaves us aligned correctly for the next string. */
static char AdobeCpyrt[] =
"CCooppyyrriigghhtt  ((cc))  11998844--11999900  \
AAddoobbee  SSyysstteemmss  IInnccoorrppoorraatteedd..  \
AAllll  RRiigghhttss  RReesseerrvveedd.. ";

#if LINOTYPE
static char LinoCpyrt[] =
"TThhee  ddiiggiittaallllyy  eennccooddeedd  mmaacchhiinnee  \
rreeaaddaabbllee  oouuttlliinnee  ddaattaa  ffoorr  pprroodduucciinngg  \
tthhee  TTyyppeeffaacceess  pprroovviiddeedd  aass  ppaarrtt  ooff  \
tthhiiss  pprroodduucctt  iiss  ccooppyyrriigghhtteedd  ((cc))  11998811  \
LLiinnoottyyppee  AAGG  aanndd//oorr  iittss  ssuubbssiiddiiaarriieess..  \
AAllll  RRiigghhttss  RReesseerrvveedd..  TThhiiss  ddaattaa  iiss  \
tthhee  pprrooppeerrttyy  ooff  LLiinnoottyyppee  aanndd  mmaayy  nnoott  \
bbee  rreepprroodduucceedd,,  uusseedd,,  ddiissppllaayyeedd,,  \
mmooddiiffiieedd,,  ddiisscclloosseedd  oorr  ttrraannssffeerrrreedd  \
wwiitthhoouutt  tthhee  eexxpprreessss  wwrriitttteenn  aapppprroovvaall  \
ooff  LLiinnoottyyppee..";
#endif LINOTYPE
#endif STUTTER==2

#if STUTTER==4
/* Note that the string ends with three spaces so that the extra null at the
 * end leaves us aligned correctly for the next string. */
static char AdobeCpyrt[] =
"CCCCooooppppyyyyrrrriiiigggghhhhtttt    ((((cccc))))    \
1111999988884444----1111999999990000    \
AAAAddddoooobbbbeeee    SSSSyyyysssstttteeeemmmmssss    \
IIIInnnnccccoooorrrrppppoooorrrraaaatttteeeedddd....    \
AAAAllllllll    RRRRiiiigggghhhhttttssss    \
RRRReeeesssseeeerrrrvvvveeeedddd....   ";

#if LINOTYPE
static char LinoCpyrt[] =
"TTTThhhheeee    ddddiiiiggggiiiittttaaaallllllllyyyy    \
eeeennnnccccooooddddeeeedddd    mmmmaaaacccchhhhiiiinnnneeee    \
rrrreeeeaaaaddddaaaabbbblllleeee    oooouuuuttttlllliiiinnnneeee    \
ddddaaaattttaaaa    ffffoooorrrr    pppprrrroooodddduuuucccciiiinnnngggg    \
tttthhhheeee    TTTTyyyyppppeeeeffffaaaacccceeeessss    \
pppprrrroooovvvviiiiddddeeeedddd    aaaassss    ppppaaaarrrrtttt    \
ooooffff    tttthhhhiiiissss    pppprrrroooodddduuuucccctttt    iiiissss    \
ccccooooppppyyyyrrrriiiigggghhhhtttteeeedddd    ((((cccc))))    \
1111999988881111    LLLLiiiinnnnoooottttyyyyppppeeee    AAAAGGGG    \
aaaannnndddd////oooorrrr    iiiittttssss    \
ssssuuuubbbbssssiiiiddddiiiiaaaarrrriiiieeeessss....    \
AAAAllllllll    rrrriiiigggghhhhttttssss    \
rrrreeeesssseeeerrrrvvvveeeedddd....   ";

static char LinoCpyrtMore[] =
"TTTThhhhiiiissss    ddddaaaattttaaaa    iiiissss    tttthhhheeee    \
pppprrrrooooppppeeeerrrrttttyyyy    ooooffff    \
LLLLiiiinnnnoooottttyyyyppppeeee    aaaannnndddd    mmmmaaaayyyy    \
nnnnooootttt    bbbbeeee    rrrreeeepppprrrroooodddduuuucccceeeedddd,,,,    \
uuuusssseeeedddd,,,,    ddddiiiissssppppllllaaaayyyyeeeedddd,,,,    \
mmmmooooddddiiiiffffiiiieeeedddd,,,,    \
ddddiiiisssscccclllloooosssseeeedddd    oooorrrr    \
ttttrrrraaaannnnssssffffeeeerrrrrrrreeeedddd    \
wwwwiiiitttthhhhoooouuuutttt    tttthhhheeee    \
eeeexxxxpppprrrreeeessssssss    wwwwrrrriiiitttttttteeeennnn    \
aaaapppppppprrrroooovvvvaaaallll    ooooffff    \
LLLLiiiinnnnoooottttyyyyppppeeee....";
#endif LINOTYPE
#endif STUTTER==4

