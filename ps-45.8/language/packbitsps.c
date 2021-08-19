/* 
  packbitsps.c

1989, Adobe Systems Incorporated.
 
This module may be altered and copied without restriction. Adobe
Systems Incorporated neither expresses nor implies any warranties
concerning this module.
  
PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.
 
Original version: Hans Obermaier: 19 Oct 1989
Edit History:
End Edit History.
*/

/*
    packbitsps.c -- Compress Binary in "PackBits" (TIFF Data Compression
    Scheme #32773) Format for PostScript Level II 
    Adobe Systems, Inc., 1989

    This module may be altered and copied without restriction.  Adobe
    Systems makes no warranties about this module, expressed or
    implied.
*/

#ifndef INCLUDED

#include <stdio.h>

#define PBBUFSIZE 1024
#define BaseStmObj FILE *
#define BaseStm FILE *
#define private
#define boolean int

typedef struct _t_PBData
{
  char * bp;
  long int bytesToRead;
  BaseStmObj baseStm;
  int state;
  int recordSize;
} PBDataRec, *PBData;

typedef struct _t_Stm
{
  int cnt;
  char *ptr, base[PBBUFSIZE];
  PBDataRec data;
  struct
  {
    unsigned error: 1;
    unsigned nichts: 7;
  } flags;
} StmRec, *Stm;

#define GetPBData(stm)          ((PBData)&((stm)->data))
#define GetPBBaseStm(pd)	(pd->baseStm)

#endif /* INCLUDED */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

private int PBForceFlush(stm,pBase)
  register Stm stm;
  register char ** pBase;
{
  register PBData pd = GetPBData(stm);
  register BaseStm baseStm = GetPBBaseStm(pd);
  register char * bp = pd->bp;
  register char * base = ( pBase == NULL ? stm->base : *pBase );

  switch(pd->state)
  {
    case 0:
      break;
    case 1:
    case 4:
      {    
        /* if ( */ putc((unsigned char)(bp-base-1),baseStm) /* == EOF )
          return EOF */;
        for( ; base < bp ; )
          /* if ( */  putc((unsigned char)*base++,baseStm) /* == EOF )
            return EOF */;
        break;
      }  
    case 2:
      {
        if ( (bp - base) > 2 )
        {
          /* if ( */ putc((unsigned char)(bp-base-3),baseStm) /* == EOF )
            return EOF */;
          for( ; base < bp-2 ; )
            /* if ( */ putc((unsigned char)*base++,baseStm) /* == EOF )
              return EOF */;
        }
        /* if ( */ putc((int)(unsigned char)-1,baseStm) /* == EOF )
          return EOF */;
        /* if ( */ putc((unsigned char)*base++,baseStm) /* == EOF )
          return EOF */;
        base++;
        break;
      }   
    case 3:
      {
        /* if ( */ putc((int)(unsigned char)-(bp-base-1),baseStm) /* == EOF )
          return EOF */;
        /* if ( */ putc((unsigned char)*base,baseStm) /* == EOF )
          return EOF */;
        base = bp;
        break;
      }   
  }     
  if ( ferror(baseStm) )
  {
    stm->flags.error = 1;
    return EOF;
  }
  pd->state = 1;
  pd->bytesToRead = pd->recordSize;
  if ( pBase != NULL )
    *pBase = base;
  return 0;
}

private int PBFlush(stm)
  Stm stm;
{
  register PBData pd = GetPBData(stm);
  register int state = pd->state;
  register char * base = stm->base;
  register char * bp = pd->bp;
  register BaseStm baseStm = GetPBBaseStm(pd);
  int bytesToRead = pd->bytesToRead;
  int recordSize = pd->recordSize;
  boolean sameChar;
  int runLength;

  for( ; bp < stm->ptr ; bp++ )
  {
    if ( ferror(baseStm) )
    {
      stm->flags.error = 1;
      return EOF;
    }
    if ( recordSize && ( bytesToRead-- == 0 ) )
    {
      char * hBase = base;
      pd->state = state;
      pd->bp = bp;
      if ( PBForceFlush(stm,&hBase) == EOF )
        return EOF;
      state = pd->state;
      base = hBase;
      bytesToRead = pd->bytesToRead-1;
      continue;
    }
    sameChar = ( *bp == *(bp-1) );
    runLength = (bp - base);
    switch (state)
    {
      case 0:
        state = 1;
        break;
      case 1:
        if ( sameChar  )
        {
          if ( runLength == 128 )
          {
            /* if ( */ putc((unsigned char)126,baseStm) /* == EOF ) 
              return EOF */;
            for( ; base < (bp-1) ; )
              /* if ( */ putc((unsigned char)*base++,baseStm) /* == EOF )
                return EOF */;
          }
          state = 2;
        }
        else
        {
          if ( runLength > 127 )
          {
            char * base128 = base + 128;

            /* if ( */ putc((unsigned char)127,baseStm) /* == EOF )
              return EOF */;
            for( ; base < base128 ; )
              /* if ( */ putc((unsigned char)*base++,baseStm) /* == EOF )
                return EOF */;
          }
        }
        break;
      case 2:
        if ( sameChar )
        {
          if ( runLength > 2 )
          {
            /* if ( */ putc((unsigned char)(bp-base-3),baseStm) /* == EOF )
              return EOF */;
            for( ; base < (bp-2) ; )
              /* if ( */ putc((unsigned char)*base++,baseStm) /* == EOF )
                return EOF */;
          }
          state = 3;
        }
        else
          state = 4;
        break;
      case 3:
        if ( sameChar )
        {
          if ( runLength == 128 )
          {
            /* if ( */ ( putc((unsigned char)-127,baseStm) /* == EOF */ )
              || ( putc((unsigned char)*base,baseStm) /* == EOF ) */ )
              /* return EOF */;  
            base = bp;
            state = 1;
          }
        }
        else
        {
          /* if ( */ putc((int)(unsigned char)-(runLength-1),baseStm) /*== EOF )
            return EOF */;
          /* if ( */ putc((unsigned char)*base,baseStm) /* == EOF )
            return EOF */;
          base = bp;
          state = 1;
        }
        break;
      case 4:
        if ( sameChar )
        {
          if ( runLength > 3)
          {
            /* if ( */ putc((unsigned char)(runLength-4),baseStm) /* == EOF )
              return EOF */;
            for( ; base < (bp-3) ; )
              /* if ( */ putc((unsigned char)*base++,baseStm) /* == EOF )
                return EOF */;
          }
          /* if ( */ putc((int)(unsigned char)-1,baseStm) /* == EOF )
            return EOF */;
          /* if ( */ putc((unsigned char)*(bp-2),baseStm) /* == EOF )
            return EOF */;
          base = bp-1;
          state = 2;
        }
        else
        {
          if ( runLength == 3 )
          {
            /* if ( */ putc((int)(unsigned char)-1,baseStm) /* == EOF )
              return EOF */;
            /* if ( */ putc((unsigned char)*base,baseStm) /* == EOF )
              return EOF */;
            base = bp-1;
          }
          state = 1;
        }
        break;
    }
  }
  if ( base < stm->ptr )
  {
    for( bp = stm->base ; base < stm->ptr ; *bp++ = *base++ );
    pd->bp = bp;
    stm->ptr = bp;
    stm->cnt = PBBUFSIZE - (bp - stm->base);
  }
  pd->state = state;
  pd->bytesToRead = bytesToRead;
  return 0;
}

private int PBPutEOF(stm)
  register Stm stm;
{
  if ( ( PBFlush(stm) == EOF )
    || ( PBForceFlush(stm,NULL) == EOF ) )
    return EOF;
  return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INCLUDED

StmRec outStm[1];

main(argc,argv)
  int argc;
  char * argv[];
{
  int charsRead;
  PBData pd = GetPBData(outStm);

  if ( ( argc > 2 ) || ( argc > 1 && argv[1] == "?" ) )
  {
    fprintf(stderr,"usage: %s [recordsize]\n",argv[0]);
    exit(1);
  }
  pd->recordSize = ( argc != 2 ? 0 : atoi(argv[1]) );
  pd->bytesToRead = pd->recordSize;
  pd->baseStm = stdout;
  outStm->ptr = outStm->base;
  outStm->cnt = PBBUFSIZE;
  pd->bp = outStm->base;
  pd->state = 0;
  for ( ; ; )
  {
    charsRead = fread(outStm->ptr, 1, outStm->cnt , stdin);
    if (charsRead == 0)
    {
      if (feof(stdin))
      {
        if (PBPutEOF(outStm) == EOF)
          goto Failure;
        exit(0);
      }
      else
        goto Failure;
    }
    outStm->ptr += charsRead;
    outStm->cnt -= charsRead;
    if (PBFlush(outStm) == EOF)
      goto Failure;
  }
  
 Failure:
  fflush(stdout);
  fprintf(stderr, "packbitsps failed.\n");
  exit(1);
}

#endif /* INCLUDED */
