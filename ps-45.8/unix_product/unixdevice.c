/* PostScript UNIX specific device module

	      Copyright 1983, 1984 -- Adobe Systems, Inc.
	    PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: Doug Brotz: Sept. 13, 1983
Edit History:
Doug Brotz: Fri Sep  5 10:04:33 1986
Chuck Geschke: Thu Oct 10 06:42:41 1985
Ed Taft: Thu Nov 23 15:49:19 1989
Ivor Durham: Tue Dec 13 11:39:43 1988
Perry Caro: Wed Nov  9 11:02:29 1988
Joe Pasqua: Thu Jan 19 15:21:51 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include DEVICE
#include ENVIRONMENT
#include ERROR
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include STREAM
#include UNIXSTREAM
#include VM

/* Cheat declarations to avoid importing internal device interface. */

extern PCard8 framebase;
extern integer framebytewidth, frameheight;

private procedure FrameToFile(base, size) PCard8 base; integer size;
{
Stm sh;
StmObj stm;
boolean lowBitFirst = PopBoolean();
extern procedure ReverseBits();
PopPStream(&stm);
if ((stm.access & wAccess) == 0) InvlAccess();
sh = GetStream(stm);
if (lowBitFirst != SWAPBITS)  ReverseBits(base, base + size);
fwrite(base, sizeof(character), size, sh);
if (ferror(sh) || feof(sh)) StreamError(sh);
if (lowBitFirst != SWAPBITS) ReverseBits(base, base + size);
} /* end of FrameToFile */

private procedure PSFrameToFile()
{
  FrameToFile(framebase, framebytewidth * frameheight);
} /* end of PSFrameToFile */

private readonly char hexMap[] = "0123456789abcdef";

private procedure WriteHexFile(sh, size)
  register Stm sh;  integer size;
{
register integer i, c, code, n;
register charptr cp, ccp, endcp;
charptr start;
endcp = (charptr)framebase + size;
i = 38;
start = (charptr)framebase;
code = n = 0;
for (cp = (charptr)framebase; cp < endcp; cp++)
  {
  c = *cp;
  if  (n==65535 || ((c==0) ? (code!=0) : (c==255) ? (code!=1) : (code!=2)))
    {
    putc(hexMap[code>>4], sh);    putc(hexMap[code&0xf], sh);
    putc(hexMap[n>>12], sh);      putc(hexMap[(n>>8)&0xf], sh);
    putc(hexMap[(n>>4)&0xf], sh); putc(hexMap[n&0xf], sh);
    i -= 3;
    if (i <= 0) {putc('\n', sh); i = 38;}
    if (code == 2)
      {
      for (ccp = start;  ccp < cp;  ccp++)
        {
        c = *ccp;
        putc(hexMap[c>>4], sh);  putc(hexMap[c&0xf], sh);
        if (--i == 0) {putc('\n', sh); i = 38;}
        }
      }
    c = *cp;
    code = (c==0) ? 0 : (c==255) ? 1 : 2;
    start = cp;
    n = 0;
    }
  n++;
  }
putc(hexMap[code>>4], sh);    putc(hexMap[code&0xf], sh);
putc(hexMap[n>>12], sh);      putc(hexMap[(n>>8)&0xf], sh);
putc(hexMap[(n>>4)&0xf], sh); putc(hexMap[n&0xf], sh);
i -= 3;
if (i <= 0) {putc('\n', sh); i = 38;}
if (code == 2)
  {
  for (ccp = start;  ccp < cp;  ccp++)
    {
    c = *ccp;
    putc(hexMap[c>>4], sh);  putc(hexMap[c&0xf], sh);
    if (--i == 0) {putc('\n', sh); i = 38;}
    }
  }
putc('\n', sh);
if (ferror(sh) || feof(sh)) StreamError(sh);
} /* end of WriteHexFile */


public readonly Card8 RevBitArray[256] = {
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
  0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
  0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
  0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
  0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
  0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
  0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
  0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
  0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
  0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
  0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
  0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
  0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
  0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
  0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
  0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
  0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF};


public procedure ReverseBits(start, endparm)  PCard8 start, endparm;
{
register PCard8 pb, end;
pb = start;
end = endparm - 8;
while (pb < end) {
  *pb = RevBitArray[*pb]; pb++;
  *pb = RevBitArray[*pb]; pb++;
  *pb = RevBitArray[*pb]; pb++;
  *pb = RevBitArray[*pb]; pb++;
  *pb = RevBitArray[*pb]; pb++;
  *pb = RevBitArray[*pb]; pb++;
  *pb = RevBitArray[*pb]; pb++;
  *pb = RevBitArray[*pb]; pb++;
  }
end = endparm;
while (pb < end) {*pb = RevBitArray[*pb]; pb++;}
}  /* end of ReverseBits */


private procedure PSFrameToHexFile()
{
integer framesize;
Stm sh;
StmObj stm;
boolean lowBitFirst = PopBoolean();
PopPStream(&stm);
if ((stm.access & wAccess) == 0) InvlAccess();
sh = GetStream(stm);
framesize = framebytewidth * frameheight;
if (lowBitFirst != SWAPBITS)
  ReverseBits(framebase, framebase + framesize);
WriteHexFile(sh, framesize);
if (lowBitFirst != SWAPBITS)
  ReverseBits(framebase, framebase + framesize);
} /* end of PSFrameToHexFile */

#if (false)


private procedure PSBandToFile()
{
integer bandsize, height;
Stm sh;
StmObj stm;
boolean lowBitFirst = PopBoolean();
PopPStream(&stm);
if ((stm.access & wAccess) == 0) InvlAccess();
sh = GetStream(stm);
height = (bdTop > frameheight) ? frameheight - bdBottom : bdheight;
bandsize = framebytewidth * height;
if (lowBitFirst != SWAPBITS)
  ReverseBits(framebase, framebase + bandsize);
fwrite(framebase, sizeof(character), bandsize, sh);
if (ferror(sh) || feof(sh)) StreamError(sh);
os_bzero((char *)framebase, (int)bandsize);
} /* end of PSBandToFile */


private procedure PSBandToHexFile()
{
integer bandsize;
Stm sh;
StmObj stm;
boolean lowBitFirst = PopBoolean();
PopPStream(&stm);
if ((stm.access & wAccess) == 0) InvlAccess();
sh = GetStream(stm);
bandsize = framebytewidth * bdheight;
if (lowBitFirst != SWAPBITS)
  ReverseBits(framebase, framebase + bandsize);
WriteHexFile(sh, bandsize);
os_bzero((char *)framebase, (int)bandsize);
} /* end of PSBandToHexFile */


#endif (false)

public procedure IniUxDev(reason)  InitReason reason;
{
switch (reason)
  {
  case romreg:
    if (!MAKEVM)
      {
      RgstExplicit("frametofile", PSFrameToFile);
      RgstExplicit("frametohexfile", PSFrameToHexFile);
      }
    break;
  }
}  /* end of IniUxDev */
