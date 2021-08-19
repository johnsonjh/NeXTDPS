/*
  These functions list the status of the print daemon queue.

  displayq( char *printer ) will list the status of each
  file in the queue.  The function displays information
  to standard output.  The output is intended to be parsed by
  another the calling program.

  The output is in the following form.  Each entry is seperated
  by a newline.

  control file name       | char *
  last modification time  | long
  owner                   | char *
  file_name               | char *
  size                    | int
  doc_name                | char *
  doc_creator             | char *
  pages                   | int
  manual_feed             | int

  Errors are written to standard error.
*/


#include "lp.h"

/*
 * Stuff for handling job specifications
 */
static char	tmp_buf[40];
static long	size;	/* total print job size in bytes */
static int	send_remote;	/* are we sending to a remote? */
static char	file_buf[132];	/* print file name */
static char err_buf[128];
static int verbose_error;

static _warn( char * );
static _inform( struct queue * );
static char *_strmatch( char *, char * );

/*
 * Display the current state of the queue. Format = 1 if long format.
 */
infoq( char *printer_name, int ve )
{
	register struct queue *q;
	register int i, nitems, fd;
	struct queue **queue;
	struct stat statb;
	FILE *fp;

	size = 0;
	verbose_error = ve;

	if ((i = pgetent(line, printer_name)) < 0)
	  fatal("cannot open printcap");
	else if (i == 0)
	  fatal("infoq unknown printer");
	if ((LP = pgetstr("lp", &bp)) == NULL)
	  LP = DEFDEVLP;
	if ((RP = pgetstr("rp", &bp)) == NULL)
	  RP = DEFLP;
	if ((SD = pgetstr("sd", &bp)) == NULL)
	  SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
	  LO = DEFLOCK;
	RM = pgetstr("rm", &bp);

	/*
	 * Figure out whether the local machine is the same as the remote 
	 * machine entry (if it exists).  If not, then ignore the local
	 * queue information.
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
	 * If there is no local printer, then print the queue on
	 * the remote machine and then what's in the queue here.
	 * Note that a file in transit may not show up in either queue.
	 */
	if (*LP == '\0') {
	  register char *cp;
	  char c;

	  send_remote++;

	  /* Send request to daemon for queue listing. */
	  (void) sprintf(line, "%c%s\n", '\6', RP);
	  fd = getport(RM);
	  if (fd < 0) {
	    if (from != host)
	      sprintf( err_buf, "%s:  connection to %s is down", host, RM );
	    sprintf( err_buf, "connection to %s is down", RM );
	    _warn( err_buf );
	  }
	  else {
	    i = strlen(line);
	    if (write(fd, line, i) != i)
	      fatal("Lost connection");
	    while ((i = read(fd, line, sizeof(line))) > 0)
	      (void) fwrite(line, 1, i, stdout);
	    (void) close(fd);
	  }
	}

	/*
	 * Find all the control files in the spooling directory
	 */
	if (chdir(SD) < 0)
	  fatal("cannot chdir to spooling directory");
	if ((nitems = getq(&queue)) < 0)
	  fatal("cannot examine spooling area\n");

	if (stat(LO, &statb) >= 0) {
	  /* Check to see if printing is disabled. */
	  if (statb.st_mode & 0100) {
	    sprintf( err_buf, "%s is disabled", printer_name );
	    _warn( err_buf );
	  }

	  /* Check to see if queuing is disabled. */
	  if (statb.st_mode & 010) {
	    sprintf( err_buf, "%s queue is disabled", printer_name );
	    _warn( err_buf );
	  }
	}

	if (nitems == 0)
	  return(0);

	/*
	  The lock file contains two lines.  The first line
	  contains the pid of the current spawned daemon.
	  The second line contains the name of the control file
	  that is currently being printed.
	*/
	fp = fopen(LO, "r");

	if (fp == NULL)
	  _warn( "no daemon present" );
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
	  if (i <= 0 || kill(i, 0) < 0)
	    _warn( "no daemon present" );

	  (void) fclose(fp);
	}

	/*
	 * Now, examine the control files and print out the jobs to
	 * be done for each user.
	 */
	for (i = 0; i < nitems; i++){
	  q = queue[i];
	  _inform( q );
	  free( q );
	}
	free(queue);

	return( nitems );
}

/*
 * Print a warning message if there is no daemon present.
 */
static
_warn( char *s )
{
  if( !verbose_error )
    return;

  fprintf( stderr, "Warning" );
  if (send_remote)
    fprintf( stderr, "on %s:  ", host);
  else
    fprintf( stderr, ":  " );
  fprintf( stderr, "%s\n", s );
}

/*
  Examine the control file and display information about the job.
*/
static
_inform( q )
     struct queue *q;
{
  register int j, k;
  register char *cp;
  struct stat lbuf;
  FILE *cfp;

  /*
   * There's a chance the control file has gone away
   * in the meantime; if this is the case just keep going
   */
  if ((cfp = fopen( q->q_name, "r")) == NULL)
    return;

  file_buf[0] = '\0';
  size = 0;

  printf( "%s\n", q->q_name );
  printf( "%d\n", q->q_time );

  q->q_pages = 0;
  q->q_docName[0] = 0;
  q->q_creator[0] = 0;
  q->q_manualFeed = 0;

  while (getline(cfp)) {
    switch (line[0]) {
    case 'f': 
      {
	FILE *pf;

	/* Examine the file for postscript magic header. */
	if( (pf = fopen( line+1, "r" )) ){
	  char magic[11];

	  if( fread(magic,1,11,pf)> 0 && strncmp(magic,"%!PS-Adobe-",11) == 0){
	    int parseCount = 0;

	  examine:
	    parseCount++;
	    while( fgets( line, BUFSIZ, pf ) ){
	      char *lp;
	      
	      if( _strmatch( line, "%%EndComments" ) )
		break;

	      if( _strmatch( line, "%%" ) == 0 )
		continue;

	      if( (lp = _strmatch( line, "%%Pages:" )) )
		sscanf( lp, "%d", &q->q_pages );

	      else if( (lp=_strmatch( line, "%%Creator:" )) && !*q->q_creator){
		while( isspace( *lp ) ) lp++;
		strcpy( q->q_creator, lp );
		q->q_creator[strlen(lp)-1] = 0;
	      }

	      else if( (lp = _strmatch( line, "%%Title:" )) && !*q->q_docName){
		while( isspace( *lp ) ) lp++;
		strcpy( q->q_docName, lp );
		q->q_docName[strlen(lp)-1] = 0;
	      }

	    }

	    while( fgets( line, BUFSIZ, pf ) ){
	      char *lp;

	      if( _strmatch( line, "%%EndSetup" ) )
		break;

	      if( _strmatch( line, "%%" ) == 0 )
		continue;

	      else if( (lp = _strmatch( line, "%%Feature: *ManualFeed ")) ){
		if( strncmp( lp, "True", 4 ) == 0 )
		  q->q_manualFeed = 1;
		else
		  q->q_manualFeed = 0;
	      }
	    }

	    if( parseCount == 1 ){
	      fseek( pf, -400, 2 );
	      goto examine;
	    }

	  }
	  fclose( pf );
	}

      }
      continue;

    case 'P':   /* Look at the owner of the job. */
      printf("%s\n", line+1);
      continue;

    case 'N':
      if( *file_buf && strcmp( line+1, file_buf ) == 0 )
	j++;
      else
	j = 1;
      if ( stat( line+1, &lbuf) == 0 )
	size += j * lbuf.st_size;
      printf( "%s\n", line+1 );
      strcpy( file_buf, line+1 );
      continue;

    default:
      continue;
    }
  }

  fclose(cfp);

  printf("%d\n", size);

  if( *q->q_docName )
    printf( "%s\n", q->q_docName );
  else
    printf( "0\n" );
  if( *q->q_creator )
    printf( "%s\n", q->q_creator );
  else
    printf( "0\n" );
  printf( "%d\n", q->q_pages );
  printf( "%d\n", q->q_manualFeed );
  fflush( stdout );
}

static char *
_strmatch( char *s1, char *s2 )
{
  int l = strlen( s2 );
  return( strncmp( s1, s2, l ) == 0 ? s1+l : 0 );
}
    
