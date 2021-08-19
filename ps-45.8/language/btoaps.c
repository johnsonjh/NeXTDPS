/* 
  btoaps.c

1989, Adobe Systems Incorporated.
 
This module may be altered and copied without restriction. Adobe
Systems Incorporated neither expresses nor implies any warranties
concerning this module.
 
PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.
 
Original version: Ed McCreight: 1 Aug 1989
Edit History:
Ed McCreight: 14 Aug 1989
End Edit History.
*/

#ifndef INCLUDED

/*
      btoaps.c -- Binary to Base-85 printable ASCII for
        PostScript Level II
      Adobe Systems, Inc., 1989
      
      This module may be altered and copied without restriction.  Adobe
      Systems Incorporated neither expresses nor implies any warranties
      concerning this module.
      
*/

#include <stdio.h>

#ifdef DOS_OS2 /* Microsoft C */
#include <io.h>		/* for setmode */
#include <fcntl.h>	/* for setmode */
#endif /* DOS_OS2 */

#define STMBUFSIZE 64 /* should be a multiple of 4 */

#define BaseStm FILE *

typedef struct _t_PBtoaData
{
  BaseStm baseStm;
  int charsSinceNewline;
} PBtoaDataRec, *PBtoaData;

typedef struct _t_Stm
{
  int cnt;
  unsigned char *ptr, base[STMBUFSIZE];
  PBtoaDataRec data;
} StmRec, *Stm;

#define GetPBtoaData(stm)       ((PBtoaData)&((stm)->data))
#define GetPBtoaBaseStm(pd)	((BaseStm)((pd)->baseStm))

static int BAFlsBuf(/* int ch, Stm stm */);

#define os_fputc(x, y)		fputc(x, y)

int os_fflush(stm)
  register Stm stm;
{
  if (BAFlsBuf(0, stm) == EOF)
    return EOF;
  stm->cnt++;
  stm->ptr--;
  return 0;
}

#endif /* INCLUDED */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define NEWLINE_EVERY	64 /* characters */

static int BAFlsBuf(ch, stm)
  int ch;
  register Stm stm;
{
  int leftOver;
  register unsigned char *base;
  register BaseStm baseStm;
  register PBtoaData pd = GetPBtoaData(stm);
  baseStm = GetPBtoaBaseStm(pd);
  
  for (base = (unsigned char *)stm->base;
    base < (unsigned char *)stm->ptr-3; base += 4)
  {
    /* convert 4 binary bytes to 5 ASCII output characters */
    register unsigned long val;
    
    if (NEWLINE_EVERY <= pd->charsSinceNewline)
    {
      if (putc('\n', baseStm) == EOF)
        return EOF;
      pd->charsSinceNewline = 0;
    }
    
    val = (((unsigned long)(base[0] << 8) | base[1]) << 16) |
      ((base[2] << 8) | base[3]);
    if (val == 0)
    {
      if (putc('y', baseStm) == EOF)
        return EOF;
      pd->charsSinceNewline++;
    }
    else if (val == 0xFFFFFFFFL)
    {
      if (putc('z', baseStm) == EOF)
        return EOF;
      pd->charsSinceNewline++;
    }
    else
    {
      /* requires 2 semi-long divides (long by short), 2
        short divides, 4 short subtracts, 5 short adds,
        4 short multiplies.  It surely would be nice if C
        had an operator that gave you quotient and remainder
        in one machine operation.
      */
      unsigned char digit[5]; /* 0..84 */
      register unsigned long q = val/7225;
      register unsigned r = (unsigned)val-7225*(unsigned)q;
      register unsigned t;
      digit[3] = (t = r/85) + '!';
      digit[4] = r - 85*t + '!';
      digit[0] = (t = q/7225) + '!';
      r = (unsigned)q - 7225*t;
      digit[1] = (t = r/85) + '!';
      digit[2] = r - 85*t + '!';
      if (fwrite(digit, 1L, 5L, baseStm) != 5)
        return (EOF);
      pd->charsSinceNewline += 5;
    }
  }
  
  for (leftOver = 0; base < (unsigned char *)stm->ptr; )
    stm->base[leftOver++] = *base++;
    
  stm->ptr = stm->base+leftOver;
  stm->cnt = STMBUFSIZE-leftOver;
  
  stm->cnt--;
  return (unsigned char)(*stm->ptr++ = (unsigned char)ch);
}

static int BAPutEOF(stm)
  register Stm stm;
{
  BaseStm baseStm = GetPBtoaBaseStm(GetPBtoaData(stm));
  os_fflush(stm);
  if (stm->base < stm->ptr)
  {
    /*  There is a remainder of from one to three binary
    bytes.  The idea is to simulate padding with 0's
    to 4 bytes, converting to 5 bytes of ASCII, and then
    sending only those high-order ASCII bytes necessary to
    express unambiguously the high-order unpadded binary.  Thus
    one byte of binary turns into two bytes of ASCII, two to three,
    and three to four.  This representation has the charm that
    it allows arbitrary-length binary files to be reproduced exactly, and
    yet the ASCII is a prefix of that produced by the Unix utility
    "btoa" (which rounds binary files upward to a multiple of four
    bytes).
    
    This code isn't optimized for speed because it is only executed at EOF.
    */
    unsigned long val, power = 52200625L /* 85^4 */;
    unsigned long int i;
    for (i = 0; i < 4; i++)
      val = val << 8 |
        ((stm->base+i < stm->ptr)? (unsigned char)stm->base[i] :
        0 /* low-order padding */);
    for (i = 0; i <= (stm->ptr - stm->base); i++)
    {
      char q = val/power; /* q <= 84 */
      if (os_fputc(q+'!', baseStm) == EOF)
        return EOF;
      val = (val - q*power);
      power /= 85;
    }
  }
  return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INCLUDED

StmRec outStm[1];

int compress(in, out)
  FILE *in, *out;
{
  int bytesRead;
  GetPBtoaData(outStm)->baseStm = out;
  GetPBtoaData(outStm)->charsSinceNewline = NEWLINE_EVERY;
  do
  {
    bytesRead = fread(outStm->base, 1, STMBUFSIZE, in);
    if (bytesRead == 0 && ferror(in))
      return(-1);
    outStm->cnt = -1; /* bytes remaining in outStm->base */
    outStm->ptr = outStm->base+bytesRead;
    if (os_fflush(outStm) == EOF)
      return(-1);
  } while (bytesRead == STMBUFSIZE);
  if (BAPutEOF(outStm) == EOF)
    return(-1);
  return (0);
}

int main()
{
  FILE *in, *out;
  int prompt = 0;
  
#ifdef SMART_C
  /* Lightspeed C for Macintosh, where there's apparently no clean way to
  force stdin to binary mode.
  */
  prompt = 1;
#endif /* SMART_C */

#ifdef PROMPT
  prompt = PROMPT;
#endif /* PROMPT */

  if (prompt)
  {
    char inName[100], outName[100];
    fprintf(stdout, "Input binary file: ");
    fscanf(stdin, "%s", inName);
    in = fopen(inName, "rb");
    if (in == NULL)
    {
      fprintf(stderr, "Couldn't open binary input file \"%s\"\n", inName);
      exit(1);
    }
    fprintf(stdout, "Output text file: ");
    fscanf(stdin, "%s", outName);
    out = fopen(outName, "w");
    if (out == NULL)
    {
      fprintf(stderr, "Couldn't open ASCII output file \"%s\"\n", outName);
      exit(1);
    }
  }
  else
  {
  
#ifdef DOS_OS2 /* Microsoft C */
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_TEXT);
#endif /* ndef DOS_OS2 */

    in = stdin;
    out = stdout;
  }
  
  fputc('<', out);
  fputc('~', out);
  if (compress(in, out))
  {
    fclose(out);
    fprintf(stderr, "\nbtoaps compress failed.\n");
    exit(1);
  }
  fputc('~', out);
  fputc('>', out);
  
  fclose(out);
  fclose(in);
  fprintf(stderr, "\nbtoaps succeeded.\n");
  exit(0);
}

#endif /* INCLUDED */
