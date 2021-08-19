#include <mach.h>
#include <stdio.h>
#include <sys/features.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <nextdev/npio.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <syslog.h>


#define OS_ERROR  0x10000000
#define IGNORE_FLAGS (~(NPNOTONER | NPPAPERDELIVERY) )



int
printerStatus( int open_np )
{
  struct npop	op;
  u_int flags;
  int printer;

  /*
   * Open and initialize the printer.
   */
  if( open_np ){
#ifdef NeXT_DEBUG
  syslog( LOG_ERR, "trying to open np0" );
#endif    
    if ((printer = open("/dev/np0", O_WRONLY|O_NDELAY, 0777)) > 0){
#ifdef NeXT_DEBUG
  syslog( LOG_ERR, "succeeded" );
#endif    
      close( printer );
    }
    else{
#ifdef NeXT_DEBUG
  syslog( LOG_ERR, "failed" );
#endif
    }
  } 

  if ((printer = open("/dev/nps0", O_WRONLY)) == -1){
    syslog( LOG_ERR, "error opening /dev/np0, %m" );
    return( errno | OS_ERROR );
  }

  op.np_op = NPGETSTATUS;
  if(ioctl(printer, NPIOCPOP, &op) == -1){
    close( printer );
    return( errno | OS_ERROR );
  }
  flags = op.np_status.flags;
  op.np_op = NPGETSTATUS;
  if(ioctl(printer, NPIOCPOP, &op) == -1){
    close( printer );
    return( errno | OS_ERROR );
  }
  
  if( flags != op.np_status.flags ){
    syslog( LOG_ERR, "found mismatch on printer status, 0x%08x != 0x%08x",
      flags, op.np_status.flags );
    if( (flags & NPNOPAPER) || (op.np_status.flags & NPNOPAPER) )
      op.np_status.flags |= NPNOPAPER;
  }

  flags = op.np_status.flags;
  flags &= IGNORE_FLAGS;
  if( (flags & NPMANUALFEED) && (!(flags & NPNOPAPER)) )
    flags &= ~(NPMANUALFEED);

  close( printer );

  return( (int)flags );
}


 
    


