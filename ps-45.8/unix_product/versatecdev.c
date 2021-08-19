/*
  versatecdev.c

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

Original version: Doug Brotz: Sept. 14, 1983
Edit History:
Doug Brotz: Thu May 22 21:55:04 1986
Chuck Geschke: Thu Oct 10 06:44:35 1985
Ed Taft: Thu May 28 13:20:23 1987
Ivor Durham: Thu Jun 23 16:24:40 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include LANGUAGE
#include PSLIB
#include UNIXSTREAM
#include VM
#include <sys/vcmd.h>

#define SCANLENGTH 264
private int plotmd[] = {VPLOT, 0, 0};
private int prtmd[] = {VPRINT, 0, 0};


private procedure VPrintFile()
{
character devicename[50], filename[100];
Stm insh, outsh;
charptr buffer; char *vpbuf;
StrObj strobj;
PopPString(&strobj);
StringText(strobj, devicename);
PopPString(&strobj);
StringText(strobj, filename);
insh = os_fopen((char *)filename, "r");
if (insh == NULL) UndefFileName();
buffer = (charptr) NEW (SCANLENGTH, sizeof(character));
outsh = os_fopen((char *)devicename, "w");
if (outsh == NULL) UndefFileName();
vpbuf = (char*)NEW(BUFSIZ,1);
setbuf(outsh, vpbuf);					/* UNIX */
ioctl(fileno(outsh), VSETSTATE, plotmd);                /* UNIX */
while (fread(buffer,sizeof(char),SCANLENGTH,insh) != 0) /* UNIX */
  fwrite(buffer,sizeof(char),SCANLENGTH,outsh);         /* UNIX */
fflush(outsh);                                          /* UNIX */
ioctl(fileno(outsh), VSETSTATE, prtmd);                 /* UNIX */
fwrite("\f\f\0",sizeof(char),3,outsh);                  /* UNIX */
fflush(outsh);                                          /* UNIX */
fclose(outsh);                                          /* UNIX */
setbuf(outsh, (char *)NIL);					/* UNIX */
FREE(vpbuf);
fclose(insh);                                           /* UNIX */
FREE(buffer);
} /* end of VPrintFile */


public procedure VersatecDevInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
    break;
  case romreg:
    if (!MAKEVM) RgstExplicit("vprint", VPrintFile);
    break;
  endswitch}
} /* end of VersatecDevInit */
