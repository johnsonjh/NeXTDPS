#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "lp.local.h"

static perr();

daemonmsg( char *machine, char *msg )
/*
  Send a message to the daemon.  If the machine is null then
  the message is sent to the local daemon, otherwise it is
  sent to the daemon on the remote machine.
*/
{
  struct sockaddr_un skun;
  register int s, n;
  char buf[BUFSIZ];

  if( machine ){
    s = getport( machine );
    if( !s )
      exit( 0 );
  }
  else {
    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
      perr("socket");
      return(0);
    }
    skun.sun_family = AF_UNIX;
    strcpy(skun.sun_path, SOCKETNAME);
    if (connect(s, (struct sockaddr *)&skun,
		strlen(skun.sun_path) + 2) < 0) {
      perr("connect");
      (void) close(s);
      return(0);
    }
  }

  (void) sprintf(buf, "%s\n", msg );
  n = strlen(buf);
  if (write(s, buf, n) != n) {
    perr("write");
    (void) close(s);
    return(0);
  }
  if (read(s, buf, 1) == 1) {
    if (buf[0] == '\0') {		/* everything is OK */
      (void) close(s);
      return(1);
    }
    putchar(buf[0]);
  }
  while ((n = read(s, buf, sizeof(buf))) > 0)
    fwrite(buf, 1, n, stdout);
  (void) close(s);

  return(0);
}

static
perr( char *msg )
{
  extern int sys_nerr;
  extern char *sys_errlist[];
  extern int errno;

  printf("%s: ", msg);
  fputs(errno < sys_nerr ? sys_errlist[errno] : "Unknown error" , stdout);
  putchar('\n');
}
