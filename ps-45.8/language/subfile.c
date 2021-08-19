/* 
  subfile.c
 
Copyright (c) 1989 Adobe Systems Incorporated. 
All rights reserved.
 
NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.
 
PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.
 
Original version: Hans Obermeir: 
Edit History:
Jim Sandman: Fri Nov 10 09:34:13 1989
Ed Taft: Fri Dec  1 15:50:11 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include EXCEPT
#include LANGUAGE
#include PSLIB
#include STREAM

#include "streampriv.h"

typedef struct _t_SFDataRec
{
  FilterDataRec common;	/* subObj[0] is base StmObj */
  StrObj stringOb;	/* this is common.subObj[1] */
  long int count;
  char * buf;
  int bufSize;
  short int state;
  short int * altState;
  unsigned pendeof:1;
  unsigned penderror:1;
  unsigned nichts:(8-2);
} SFDataRec, *SFData;

#define GetSFData(stm)		((SFData) (stm + 1))
#define GetSFBaseStm(dp)	(GetStream((dp)->common.subObj[0]))
#define GetPattern(dp)		((dp)->stringOb.val.strval)
#define GetPatternLen(dp)	((dp)->stringOb.length)

private int SFFilBuf(stm)
  Stm stm;
{
  register SFData dp = GetSFData(stm);
  Stm baseStm = GetSFBaseStm(dp);
  char * buf = stm->base;
  char * pattern = (char *)GetPattern(dp);
  int patternLen = GetPatternLen(dp);

  if ( dp->pendeof || dp->penderror )
  {
    stm->flags.eof = true;
    stm->flags.error = dp->penderror;
    return EOF;
  }

  if ( patternLen == 0 )
  {
    int bytesRead = fread(buf, 1, os_min(dp->bufSize, dp->count), baseStm);

    dp->pendeof = ( ( (dp->count -= bytesRead) == 0)
                 || ( bytesRead < dp->bufSize ) );
    stm->ptr = stm->base;
    stm->cnt = bytesRead;
    goto schluss;
  }
  else
  {
    int count = dp->count;
    register int nstate = dp->state;
    register char ch;
    register short int * altState = dp->altState;
    int bytesRead;

    switch ( count )
    {
      case 0:
        for( ; ( nstate < patternLen ) ; )
        {
          if( (ch = getc(baseStm)) == EOF ) 
          {
            return EOF;
          }
          if ( ch == pattern[nstate] )
          {
            nstate++;
          }
          else
          {
            for( ; nstate > 0 ; )
            {
              nstate = altState[nstate-1] + 1;
              if ( ch == pattern[nstate] )
              {
                nstate++;
                break;
              } /* if */
            } /* for */
          } /* else */
        } /* for */

        dp->count = -1;

      case -1:
        bytesRead = fread( buf, 1, dp->bufSize, baseStm);
        dp->pendeof = ( bytesRead < dp->bufSize );
        stm->ptr = stm->base;
        stm->cnt = bytesRead;
        goto schluss;

      default:
      {
        int bytesToFill = dp->bufSize;

        for( ; ; )
        {
          if ( nstate == patternLen )
          {
            count--;
            nstate = 0;
          }
          if ( count == 0 )
          {
            dp->pendeof = true;
            stm->ptr = stm->base;
            stm->cnt = dp->bufSize - bytesToFill;
            goto schluss;
          }
          if ( bytesToFill == 0 )
          {
            dp->count = count;
            dp->state = nstate;
            stm->ptr = stm->base; 
            stm->cnt = dp->bufSize;
            goto schluss;
          }
          if ( (ch = getc(baseStm)) == EOF ) 
          { 
            return EOF;  
          }
          bytesToFill--;
          *buf++ = ch;
          if ( ch == pattern[nstate] )
          {
            nstate++;
          }
          else
          {
            for( ; nstate > 0 ; )
            {
              nstate = altState[nstate-1] + 1;
              if ( ch == pattern[nstate] )
              {  
                nstate++;
                break;
              } /* if */
            } /* for */
          } /* else */
        } /* for */
      } /* default */
    } /* switch */
  } /* else */

  schluss:
  if ( stm->cnt == 0 )
  {
    stm->flags.eof = true;
    return EOF;
  }
  else
  {
    stm->cnt--;
    return *stm->ptr++;
  }
}

private int SFClose(stm)
  register Stm stm;
{
  register SFData dp = GetSFData(stm);

  if ( dp->buf != NIL )
    os_free(dp->buf);
  if ( dp->altState != NIL )
    os_free(dp->altState);
  CloseIfNecessary(GetSFBaseStm(dp));
  StmDestroy(stm);
  return 0;
}

private int SFAvail(stm)
  Stm stm;
{
  return stm->cnt;
}

private int SFReset(stm)
  Stm stm;
{
  return 0;
}

private int SFFlush(stm)
  Stm stm;
{
  for( ; getc(stm) != EOF ; );
  return 0;
}

private readonly StmProcs SubfileStmProcs = {
  SFFilBuf, StmErr, StmFRead, StmZeroLong, StmUnGetc, SFFlush,
  SFClose, SFAvail, SFReset, StmErr, StmErr, StmErrLong,
  "!SubfileFilter" };

public Stm SubfileOpen()
{
  register Stm stm;
  register SFData dp;
  int patternLen;
  char * pattern;
  char *buff1 = NIL, *buff2 = NIL;

  if ( (stm = StmCreate(&SubfileStmProcs,
			(integer) sizeof(SFDataRec))) == NIL )
    LimitCheck();
  DURING
  dp = GetSFData(stm);

  PopPRString(&dp->stringOb);
  ConditionalInvalidateRecycler(&dp->stringOb);

  dp->count = PSPopInteger();

  pattern = (char *)GetPattern(dp);
  patternLen = GetPatternLen(dp);

  if ( ( dp->count < 0 ) 
    || ( ( patternLen == 0 ) && ( dp->count == 0 ) ) )
    RangeCheck();

  dp->bufSize = ( patternLen <= 10 ? 128 :
                  patternLen <= 100 ? 10 * patternLen :
                  patternLen <= 1000 ? 4 * patternLen :
                  2 * patternLen );
  if ( ( (buff1 = os_malloc(dp->bufSize)) == NIL )
    || ( ( patternLen > 0 ) 
      && ( (buff2 = os_calloc(dp->bufSize, sizeof(short int))) == NIL ) ) )
    LimitCheck();
  dp->buf = buff1;
  dp->altState = (short int *)buff2;
  stm->ptr = (
    stm->base = dp->buf);
  stm->cnt = 0;
  stm->flags.read = true;

  GetSrcOrTgt(&dp->common.subObj[0], true);
  dp->common.subObjCount = 2;

  dp->state = 0;

  if ( patternLen > 0 )
  {
    register int i;
    register int alti = 0;
    register short int * altState = dp->altState;

    altState[0] = -1;
    for( i = 1; i < patternLen; i++)
    {
      if ( pattern[i] == pattern[alti] )
      {
        altState[i] = altState[alti];
        alti++;
      }
      else
      {
        altState[i] = alti;
        alti = 0;
      }
    }
  }

  HANDLER {
    if (buff1 != NIL) os_free(buff1);
    if (buff2 != NIL) os_free(buff2);
    StmDestroy(stm);
    }
  END_HANDLER
  return stm;
}
