/*
  Usage:  Inform message [-b1 label] [-b2 label]
          Inform [-h host] -n code
	  
  Inform is a command line program that simply raise a window with
  the specified message and, optionally, up to two buttons.
  
  The -n option in used by npd to specify a particular message
  coded as a single integer.  The integer is used to match a 
  code in an array.  If the error condition related to the particular
  code is fixed then the panel is automatically removed.
*/	  
#import	<stddef.h>
#include <mach.h>
#include <sys/message.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <pwd.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <nextdev/npio.h>
#include <servers/netname.h>

#import <appkit/appkit.h>
#import "GrowAlert.h"

#define LOCK_FILE "/tmp/inform.lock"

#define OS_ERROR  0x10000000
#define SPECIAL_ERROR 0x20000000
#define MANUAL_FEED_TIMEOUT 0x1

extern int sys_nerr;
extern char *sys_errlist[];

struct AlertInfo {
  u_int flag;
  char alert;
  char *message;
  char *button1;
  char *button2;
};

struct AlertInfo prMessages[] = {
{NPDOOROPEN,1,"Printer cover open", "Continue", "Stop"},
{NPPAPERJAM,1,"Paper jammed in printer.  Open door and check paper path",
     "Continue", "Stop"},
{(NPMANUALFEED|NPNOPAPER),1,"Printer waiting for manual feed paper",
     "Continue", "Stop"},
{NPMANUALFEED,1,"Manual feed selected", "Continue", "Stop"},
{NPNOPAPER,1,"No paper in printer", "Continue", "Stop"},
{NPNOCARTRIDGE,1,"No toner cartridge in printer", "Continue", "Stop"},
{NPNOTONER,0,"Printer toner cartridge running low", "Continue", "Stop"},
{NPHARDWAREBAD,1,"Printer hardware failure", "Continue", "Stop"},
{NPPAPERDELIVERY,0,"Printing document", "Continue", "Stop"},
{NPDATARETRANS,0,"Retransmitting document", "Continue", "Stop"},
{NPCOLD,0,"Printer warming up", "Continue", "Stop"},
{0, 0 }
};

struct AlertInfo npErrorMessages[] = {
{(OS_ERROR | ENODEV),1,"No printer connected", "OK", 0 },
{(SPECIAL_ERROR | MANUAL_FEED_TIMEOUT),1,"Manual feed timeout;\nprint request canceled.", "OK", 0 },
{0, 0 }
};

static char DefaultServerName[] = "NextStep(tm) Window Server";

/* Back door name for the window server */
static char server_key[] = { 0x4e, 0x5e, 0x4e, 0x75, 0 };

static NXDefaultsVector informDefaults = {
    {"NXPSName", server_key },
    {NULL}
};

main( int argc, char **argv )
{
  int i = 1;
  char *cp;
  int flag = 0;
  char *button1 = 0, *button2 = 0, *message = 0;
  char buf[128];
  char *host = 0;
  kern_return_t kernResult;
  port_t dummyPort;
  const char *defaultsHost;
  char myHostName[MAXHOSTNAMELEN+1];
  struct passwd *user;
  
  openlog( "Inform", LOG_PID, LOG_LPR );

  if( argc < 2 )
    return;
    
  if( !checkLock() )
    exit( 0 );

  cp = argv[i++];
  while( cp ){
    if( *cp == '-' ){ 
      cp++;
      if( strcmp( cp, "b1" ) == 0 )
        button1 = argv[i++];
      else if( strcmp( cp, "b2" ) == 0 )
        button2 = argv[i++];
      else if( strcmp( cp, "h" ) == 0 )
        host = argv[i++];
      else if( strcmp( cp, "n" ) == 0 ){
        cp = argv[i++];
	if( strncmp( cp, "0x", 2 ) == 0 )
	  sscanf( cp, "0x%x", &flag );
	else
	  flag = atoi( cp );
      }
      else{
        i++;
      }
    }
    else if( !message )
      message = cp;
    else
      fprintf( stderr, "bad usage" );
    cp = argv[i++];
  }
  
  kernResult = netname_look_up(name_server_port, "", DefaultServerName, &dummyPort);
  if (kernResult != NETNAME_SUCCESS) {
    user = getpwuid(geteuid());
    if (strcmp(user->pw_name,"root")==0 || strcmp(user->pw_name,"daemon")==0) {
      NXRegisterDefaults("Inform", informDefaults);
      NXSetDefault("Inform", "NXPSName", (const char *)server_key);
    }
  }
   
  NXApp = [Application new];
  
  /* Make sure we are running on the local host */
  defaultsHost = NXGetDefaultValue("Inform", "NXHost");
  if (defaultsHost != NULL && *defaultsHost != '\0' && strcmp(defaultsHost,"localhost") != 0) {
    gethostname(myHostName, sizeof(myHostName));
    if (strcmp(defaultsHost,myHostName) != 0) {
      syslog( LOG_ERR, "Illegal to request remote host: %s", defaultsHost );
      exit( 1 );
      }
  }

  if( flag ){
    struct AlertInfo *ai;
    
    ai = npErrorMessages;
    while( ai->flag ){
      if( ai->flag == flag ){
        PrinterAlert( host, ai->message, ai->button1, ai->button2, 0 );
	exit( 0 );
      }
      ai++;
    }

    if( flag & OS_ERROR ){
      sprintf( buf, "Printer device error:\n%s\nJob aborted",
        sys_errlist[(flag & (~OS_ERROR))] );
      PrinterAlert( host, buf, "OK", 0,  0 );
      exit( 0 );
    }

    ai = prMessages;
    while( ai->flag ){
      if( (ai->flag & flag) == ai->flag ){
        PrinterAlert( host, ai->message, ai->button1, ai->button2, flag );
	exit( 0 );
      }
      ai++;
    }
    syslog( LOG_ERR, "unknown numeric flag" );
    exit( 0 );
  }
  
  PrinterAlert( host, message, button1, button2, 0 );

  [NXApp free];

  exit( 0 );
}


checkLock()
{
  struct stat st;
  int fd;

  if( stat( LOCK_FILE, &st ) < 0 ){
    if( errno != ENOENT ){
      syslog( LOG_ERR, "can't stat lock file: %m" );
      return( 0 );
    }
    if( creat( LOCK_FILE, 0660 ) < 0 ){
      perror( "creat" );
      return( 0 );
    }
  }

  if( (fd = open( LOCK_FILE, O_RDONLY, 0660 )) < 0 ){
    syslog( LOG_ERR, "can't open lock file: %m" );
    return( 0 );
  }

  if( flock( fd, LOCK_NB|LOCK_EX ) < 0 ){
    if( errno == EWOULDBLOCK ){
      return( 0 );
    }
    else
      syslog( LOG_ERR, "lock file locked" );
    return( 0 );
  }

  return( 1 );
}
