/*
  mappedstream.c
  Stream functions for streams based on Mach mapped files

CONFIDENTIAL
Copyright (c) 1988 NeXT, Inc.
All rights reserved.

Original version: Leo Hourvitz: Wed Aug 31 22:36:00 1988
Edit History:
Jack Newlin 16Oct90  now MFUnGetc increments cnt (as well as decrementing ptr)
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include ENVIRONMENT
#include PSLIB
#include STREAM
#undef MONITOR
#include <mach.h>


/* Mapped file class implementations of the stream operations.
   Since mapped files are read-only, these procs are unimplemented
   on the write side. */

/* Mapped file defines */
#define FileSize(stm) ((int)((stm)->data.b))
#define FileFd(stm) ((int)(stm->data.a))
#define MAPLOG 1
#if MAPLOG
extern procedure AddMappedFile(/*size*/);
extern procedure RemoveMappedFile(/*size*/);
#endif MAPLOG

private int MFFilBuf(stm)
  register Stm stm;
{
    if (stm->cnt <= 0) {
	stm->flags.eof = 1;
    	return(-1);
    }
    stm->cnt--;
    return((int)(*stm->ptr++));
}

private long int MFRead(ptr,itemSize,nItems,stm)
char *ptr;
long int itemSize, nItems;
Stm stm;
{
    unsigned int bytesToRead;
    
    bytesToRead = itemSize*nItems;
    if (bytesToRead > stm->cnt)
    {
	nItems = stm->cnt / itemSize;
	bytesToRead = nItems*itemSize;
    }
    os_bcopy(stm->ptr,ptr,bytesToRead);
    stm->ptr += bytesToRead;
    stm->cnt -= bytesToRead;
    return(nItems);
} /* MFRead */

private int MFUnGetc(ch,stm)
unsigned char ch;
Stm stm;
{
    if (stm->ptr > stm->base)
    {
    	stm->ptr--;
    	stm->cnt++;
	return(0);
    }
    return(-1);
} /* MFUnGetc */

private int MFFlush(stm)
  register Stm stm;
{
    stm->ptr += stm->cnt;
    stm->cnt = 0;
    return 0;
}

private int MFClose(stm)
  register Stm stm;
{
    int r;
    
    r = 0;
    if (vm_deallocate(task_self(),(vm_address_t)(stm->base),
    		(vm_size_t)FileSize(stm)) != KERN_SUCCESS)
        r = EOF;
#if MAPLOG
    RemoveMappedFile(FileSize(stm));
#endif MAPLOG
    if (close(FileFd(stm)) == -1)
        r = EOF;
    StmDestroy(stm);
    return r;
}

private int MFAvail(stm)
  register Stm stm;
{
    if (stm->flags.eof) return -1;
    if (stm->cnt >= 0) return stm->cnt;
    return -1;
}
private int MFSeek(stm, offset, origin)
  Stm stm; long int offset; int origin;
{
    switch(origin) {
    case 0: /* beginning */
    	break;
    case 1: /* current position */
    	offset = stm->ptr - stm->base + offset;
    	break;
    case 2: /* end */
    	offset = FileSize(stm) + offset;
    	break;
    } /* switch(origin) */
    if ((offset < 0)||(offset > FileSize(stm)))
    	return -1;
    stm->ptr = stm->base + offset;
    stm->cnt = FileSize(stm) - offset;
    return 0;
}

private long int MFTell(stm)
  register Stm stm;
{
    return(stm->ptr - stm->base);
}

public readonly StmProcs mappedStmProcs = {
  MFFilBuf, StmErr, MFRead, StmErrLong, MFUnGetc, MFFlush,
  MFClose, MFAvail, StmErr, StmErr, MFSeek, MFTell,
  "Mapped"};



