/*
  qstatus() prints a line of status for the given printer queue onto
  stdout.
*/


#include "lp.h"

/*
 * Stuff for handling job specifications
 */
static char	tmp_buf[40];
static int	send_remote;	/* are we sending to a remote? */
static char err_buf[128];

static _output( char * );

qstatus( char *printer_name )
{
  register int i, fd;
  struct stat statb;
  FILE *fp;

  if ((i = pgetent(line, printer_name)) < 0)
    fatal("cannot open printcap");
  else if (i == 0)
    fatal("unknown printer");
  if ((LP = pgetstr("lp", &bp)) == NULL)
    LP = DEFDEVLP;
  if ((RP = pgetstr("rp", &bp)) == NULL)
    RP = DEFLP;
  if ((SD = pgetstr("sd", &bp)) == NULL)
    SD = DEFSPOOL;
  if ((LO = pgetstr("lo", &bp)) == NULL)
    LO = DEFLOCK;
  if ((ST = pgetstr("st", &bp)) == NULL)
    ST = DEFSTAT;
  RM = pgetstr("rm", &bp);

  /*
   * Figure out whether the local machine is the same as the remote 
   * machine entry (if it exists).  If not, then ignore the local
   * status information.
   */
  if (RM != (char *) NULL) {
    char name[256];
    struct hostent *hp;

    /* get the standard network name of the local host */
    gethostname(name, sizeof(name));
    name[sizeof(name)-1] = '\0';
    hp = gethostbyname(name);
    if (hp == (struct hostent *) NULL) {
      fatal("can't get network name for local machine %s\n", name);
    }
    else
      strcpy(name, hp->h_name);

    /* get the network standard name of RM */
    hp = gethostbyname(RM);
    if (hp == (struct hostent *) NULL) {
      fatal("can't get hostname for remote machine %s\n", RM);
    }

    /* if printer is not on local machine, ignore LP */
    if (strcmp(name, hp->h_name) != 0)
      *LP = '\0';
  }

  /*
   * If there is no local printer, then print the status on
   * the remote machine.
   */
  if (*LP == '\0') {
    register char *cp;
    char c;

    send_remote++;

    /* Send request to daemon for queue listing. */
    (void) sprintf(line, "%c%s\n", '\7', RP);
    fd = getport(RM);
    if (fd < 0) {
      if (from != host)
	sprintf( err_buf, "On machine %s the connection to machine %s is down",
		host, RM );
      sprintf( err_buf, "the connection to machine %s is down", RM );
      _output( err_buf );
      return( 0 );
    }
    else {
      i = strlen(line);
      if (write(fd, line, i) != i)
	fatal("Lost connection");
      while ((i = read(fd, line, sizeof(line))) > 0)
	(void) fwrite(line, 1, i, stdout);
      (void) close(fd);
      fflush( stdout );
    }
    return( 0 );
  }

  /*
   * Find all the control files in the spooling directory
   */
  if (chdir(SD) < 0)
    fatal("cannot chdir to spooling directory");
  
  if (stat(LO, &statb) >= 0) {
    /* Check to see if printing is disabled. */
    if (statb.st_mode & 0100) {
      sprintf( err_buf, "Printing is disabled", printer_name );
      _output( err_buf );
      return( 0 );
    }

    /* Check to see if queuing is disabled. */
    if (statb.st_mode & 010) {
      sprintf( err_buf, "%s queue is disabled", printer_name );
      _output( err_buf );
      return( 0 );
    }
  }

  /*
    The lock file contains two lines.  The first line
    contains the pid of the current spawned daemon.
    The second line contains the name of the control file
    that is currently being printed.
    */
  fp = fopen(LO, "r");
  
  if (fp == NULL){
    _output( "No active printing" );
    return( 0 );
  }
  else {
    register char *cp;
    
    /* 
      Get spawned daemon pid.  This is the pid of the spawned
      daemon that is currently running.
      */
    cp = tmp_buf;
    while ((*cp = getc(fp)) != EOF && *cp != '\n')
      cp++;
    *cp = '\0';
    i = atoi(tmp_buf);

    /*
      Make sure that the daemon is still alive.  If it isn't then
      there is some kind of problem.
      */
    if (i <= 0 || kill(i, 0) < 0){
      fclose( fp );
      _output( "No active printing" );
      return( 0 );
    }

    /* Print the status file. */
    fd = open(ST, O_RDONLY);
    if (fd >= 0) {
      flock(fd, LOCK_SH);
      while ((i = read(fd, line, sizeof(line))) > 0)
	(void) fwrite(line, 1, i, stdout);
      close(fd);	/* unlocks as well */
      fflush( stdout );
    }
    else{
      _output( "can't get status." );
      return( 0 );
    }
    fclose(fp);
  }
  fflush( stdout );
}

/*
 * Print a warning message if there is no daemon present.
 */
static
_output( char *s )
{
  if (send_remote)
    fprintf( stdout, "On machine %s ", host);
  fprintf( stdout, "%s\n", s );
  fflush( stdout );
}
