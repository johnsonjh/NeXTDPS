/*
 *  Copyright (c) 1988, 1989 NeXT Inc., All rights reserved.
 *
 *  Sept 1988
 *  Made major modification to redirect postscript output to NeXT
 *  Postscript Server instead of to the printer.
 *
 * is the communications filter for sending files to
 * the NeXT Window server.  The NeXT Window server then sends 
 * interpreted postscript to the NeXT printer.
 *
 * It observes (parts of) the PostScript file structuring conventions.
 * In particular, it distinguishes between PostScript files (beginning
 * with the "%!" magic number) -- which are shipped to the printer --
 * and text files (no magic number) which are formatted and listed
 * on the printer.  Files which begin with "%!PS-Adobe-" may be
 * page-reversed if the target printer has that option specified.
 *
 * npcomm gets called with:
 *	stdin	== the file to print (may be a pipe!)
 *	stdout	== the printer
 *	stderr	== the printer log file
 *	cwd	== the spool directory
 *	argv	== from lpd
 *	  filtername
 *			[-r]		(don't ever reverse)
 *			-n login
 *			-h host
 *                      -f filename     (filename to use instead of stdin)
 *
 * The exit code of this filter determines the behavior of lpd.
 * 0 - everything is OK
 * 1 - try to reprint
 * 2 - fatal error, don't reprint
 */

#include <mach.h>
#include <mach_error.h>
#include <sys/message.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdio.h>
#include <ctype.h>
#include "npd.h"


#ifdef NeXT_DEBUG
#define debugp(x) {fprintf x ; (void) fflush(stderr);}
#else
#define debugp(x)
#endif

#define NPD "/usr/lib/NextPrinter/npd"
#define DONE_TIMEOUT 30

static char *_strmatch( char *, char * );
static char *convert( char *, char *, int, int );
static void feeder( char *, char *, char * );
static void send_from_file( port_t, FILE *, long, int, int );
static long find_trailer( FILE *, long );
static long find_next_page( FILE *, long );
static long find_header( FILE * );
static port_t npd_connect( char *, char *, int );
static msg_return_t npd_send( port_t, char *, int, int );

extern char *asciiToPS( char *, int );

port_t npcomm_port;

int resolution = 0;
int manualfeed = 0;

char *host = 0;

int debug = 0;
#define npcomm_debug(x) { if( debug ) syslog x ; }
 

main( int argc, char **argv )
{
  char converted_file[128];
  int converted = 0;
  char print_file[128];
  int print_fd, i;
  char magic_header[32];
  int magic_length;
  int job_num, noReverse = 0, canReverse = 0;
  char *user_name = 0, *printer = 0;
  char *cp;
  
  openlog( "npcomm", LOG_PID, LOG_LPR );

  /* parse command-line arguments */
  for( i = 1; i < argc; i++ ){
    cp = argv[i];
    if( *cp == '-') {
      switch(*(cp + 1)){
      case 'f':
	strncpy( print_file, argv[++i], sizeof(print_file) );
	sscanf( print_file+3, "%d", &job_num );
	break;

      case 'd': debug = 1;			break;
      case 'R': resolution = atoi( argv[++i] );	break;
      case 'M': manualfeed = 1;			break;
      case 'p': printer = argv[++i];		break;
      case 'n': user_name = argv[++i];		break;
      case 'h': host = argv[++i];		break;
      case 'r':	noReverse = 1;			break;

      default:	/* unknown */
        npcomm_debug(( LOG_ERR, "unknown option received by npcomm" ));
	break;
      }
    }
  }

  if( !printer || !user_name || !print_file || !host ){
    syslog( LOG_ERR, "bad args" );
    exit( 0 );
  }

  npcomm_debug(( LOG_ERR, "printer %s, user %s, file %s, host %s",
    printer, user_name, print_file, host ));
    
  if( (print_fd = open( print_file, O_RDONLY )) < 0 ){
    syslog( LOG_ERR, "can't open input file, %s: %m", print_file );
    exit( 0 );
  }
  
  if ((magic_length = read( print_fd, magic_header, 11)) <= 0) {
    syslog( LOG_ERR, "can't read magic header, %m" );
    exit( 0 );
  }

  /* Do we have conforming PostScript input? */
  if( strncmp( magic_header,  "%!PS-Adobe-",11 ) == 0)
    canReverse = TRUE;

  /* Or regular PostScript input? */
  else if( strncmp( magic_header, "%!", 2 ) == 0)
    canReverse = FALSE;

  /*
   * Or something completely different?   In this case we
   * must convert the input to PostScript.
   */
  else{
    lseek( print_fd, 0L, L_SET );
    strcpy( print_file, convert( print_file, magic_header, magic_length, print_fd ) );
    converted = 1;
  }
    
  close( print_fd );
  
  feeder( printer, user_name, print_file );

  if( converted ){
    unlink( print_file );
  }

  exit(0);
}



static char *
convert( char *fname, char *id, int count, int fd )
/*
  Converts the stdin to PostScript
  Returns 1 if successfully converted, otherwise 0.
*/
{
  int i, cnt;
  /* here is where you might test for other file type
   * e.g., PRESS, imPRESS, DVI, Mac-generated, etc.
   */

  /* final sanity check on the text file, to guard
   * against arbitrary binary data
   */
  for (i = 0; i < count; i++) {
    if (!isascii( id[i]) || (!isprint( id[i]) && !isspace(id[i]))){
      /*
       * If the input is garbage then we print an error message
       * instead of printing the input.
       */
      syslog( LOG_ERR, "can't print binary file" );
      exit( 0 );
    }
  }
  
  return( asciiToPS( fname, fd ) );
}


static char *
_strmatch( char *s1, char *s2 )
{
  int l = strlen( s2 );
  return( strncmp( s1, s2, l ) == 0 ? s1+l : 0 );
}


static void
feeder( char *printer, char *user_name, char *print_file )
/*
  Send the PostScript to npd.
  */
{
    FILE *print_fp;
    char buf[1024];
    char *read_buf;
    int file_count = 0;
    long header = 0;
    long trailer = 0;
    long location = 0, new_location = 0, first_page = 0;
    port_t npd_port;
    msg_return_t ret_code;
    msg_header_t rec_msg;
    struct stat stat_buf;
    int max_length;
    int retries;
    
    print_fp = fopen( print_file, "r" );
    if( !print_fp ){
	syslog( LOG_ERR, "can't open %s PS file to print, %m", print_file );
	exit( 0 );
    }
    
    if( stat( print_file, &stat_buf ) < 0 ){
	syslog( LOG_ERR, "can't stat file %s, %m", print_file );
	exit( 0 );
    }
    max_length = stat_buf.st_size;
    
    if( !debug ){  
	/*
	  This loop will keep trying to connect to npd until it succeeds.
	  */
	retries = 0;
	while( !(npd_port = npd_connect( printer, user_name, 1 )) ){
	    retries++;
	    if (retries == 1) {
		syslog( LOG_ERR, "Cannot connect to npd, sleeping." );
	    }
	    sleep( 60 );
	}
	
	if (retries > 0) {
	    syslog( LOG_ERR, "Connection to npd established." );
	}
    }
    
    /* 
      Parse the postscript for conforming comments. We only need to
      find the header, each page thereafter, and the trailer.
      
      First, look for a %%EndSetup or a %%EndProlog.  This will delimit the
      end of the header.
      */
    header = find_header( print_fp );
    /*  See if we have found a header. */
    if( header ){
	if( !resolution && !manualfeed )
	    send_from_file( npd_port, print_fp, 0, header, NPD_SEND_HEADER );
	else{
	    char *header_buf, *hb;
	    
	    header_buf = (char *)malloc( header+1024 );
	    fread( header_buf, header, 1, print_fp );
	    hb = header_buf+strlen(header_buf);
	    while( *hb != '%' ) hb--;
	    hb--;
	    if( resolution )
		sprintf( hb, "%%%%Feature: *Resolution %d\n", resolution );
	    if( manualfeed )
		sprintf( hb+strlen(hb), "%%%%Feature: *ManualFeed %s\n",
			manualfeed ? "True" : "False" );
	    sprintf( hb+strlen(hb), "%%%%EndSetup\n" );
	    if( !debug )
		npd_send( npd_port, header_buf, strlen(header_buf), NPD_SEND_HEADER );
	    else
		fprintf( stderr, "%s", header_buf );
	    free( header_buf );
	}
    }    
    else{
	/* We don't have good conforming PostScript so we should send */
	/* a mock header. */
	fgets( buf, sizeof(buf), print_fp );
	if( !debug ){
	    if( resolution )
		sprintf( buf+strlen(buf), "%%%%Feature: *Resolution %d\n",
			resolution );
	    if( manualfeed )
		sprintf( buf+strlen(buf), "%%%%Feature: *ManualFeed %s\n",
			manualfeed ? "True" : "False" );
	    npd_send( npd_port, buf, strlen(buf), NPD_SEND_HEADER );
	}
	else{
	    if( resolution )
		sprintf( buf+strlen(buf), "%%%%Feature: *Resolution %d\n",
			resolution );
	    if( manualfeed )
		sprintf( buf+strlen(buf), "%%%%Feature: *ManualFeed %s\n",
			manualfeed ? "True" : "False" );
	    fprintf( stderr, "%s", buf );
	}
    }
    
    /* Now send the first page. */
    location = header;
    first_page = find_next_page( print_fp, location ? location-1 : 0 );
    if( first_page ){
	new_location = find_next_page( print_fp, first_page );
	if( new_location ){
	    send_from_file( npd_port, print_fp,
			   location, new_location-location, NPD_SEND_PAGE );
	    
	    location = new_location;
	    /* And every other page. */
	    while( new_location = find_next_page( print_fp, location ) ){
		send_from_file( npd_port, print_fp,
			       location, new_location-location, NPD_SEND_PAGE);
		location = new_location;
	    }
	}
	else
	    first_page = 0;
    }
    
    if( !first_page ){
	/* We can't find the conforming comment for a page so we should
	   just send everything up to the trailer. */
	trailer = find_trailer( print_fp, new_location );
	if( trailer )
	    send_from_file( npd_port, print_fp,
			   header, trailer - header, NPD_SEND_PAGE );
	else
	    send_from_file( npd_port, print_fp, 
			   header, max_length, NPD_SEND_PAGE );
    }
    if( !trailer )  
	trailer = find_trailer( print_fp, new_location );
    
    if( trailer )
	send_from_file( npd_port, print_fp, 
		       trailer, max_length, NPD_SEND_TRAILER );
    else{
	char *s = "%%Trailer\n";
	
	if( !debug )
	    npd_send( npd_port, s, strlen(s), NPD_SEND_TRAILER );
	else
	    fprintf( stderr, "%s", buf );
    }
    
    /* Wait for npd to finish. */
    if( !debug ){
#ifdef NeXT_DEBUG
	syslog( LOG_ERR, "waiting for npd to finish" );
#endif
	rec_msg.msg_local_port = npcomm_port;
	rec_msg.msg_size = sizeof( rec_msg );
	ret_code = msg_receive( &rec_msg, MSG_OPTION_NONE, 0 );
	if( ret_code != RCV_SUCCESS ){
	    syslog( LOG_ERR, "bad msg_receive, %d\n", ret_code );
	    exit( 0 );
	}
	
	DeletePort( npcomm_port );
	DeletePort( npd_port );
    }
    
}



static void
send_from_file( port_t npd_port, FILE *fp, long start, int length, int id )
{
  char *read_buf;
  int length_read;
  
  read_buf = (char *)malloc( length );
  if( !read_buf ){  
    syslog( LOG_ERR, "can't allocate read buffer of length %d", length );
    abort_connection( npd_port );
    exit( 0 );
  }
  if( fseek( fp, start, 0 ) == -1 ){
    syslog( LOG_ERR, "can't fseek to %d, %m", start );
    abort_connection( npd_port );
    exit( 0 );
  }
  
  if( (length_read = fread( read_buf, 1, length, fp )) == 0 ){
    syslog( LOG_ERR, "fread of %d bytes failed , %m", length );
    abort_connection( npd_port );
    exit( 0 );
  }
  
  if( !debug )
    npd_send( npd_port, read_buf, length_read, id );
  else{
    fprintf( stderr, "%% SEND START\n" );
    fprintf( stderr, "%s", read_buf );
  }
  free( read_buf );
}


abort_connection( port_t npd_port )
{
  msg_return_t ret_code;
  msg_header_t rec_msg;
 
  if( !debug ){ 
    npd_send( npd_port, "%%Trailer\n", strlen("%%Trailer\n"), NPD_SEND_TRAILER );
  #ifdef NeXT_DEBUG
    syslog( LOG_ERR, "waiting for npd to finish" );
  #endif
    rec_msg.msg_local_port = npcomm_port;
    rec_msg.msg_size = sizeof( rec_msg );
    ret_code = msg_receive( &rec_msg, RCV_TIMEOUT, DONE_TIMEOUT*1000 );
    if( ret_code != RCV_SUCCESS ){
      syslog( LOG_ERR, "bad msg_receive, %d\n", ret_code );
      exit( 0 );
    }
  }
}

static void skipBinary(char *bp,FILE *fp)
{
    int byteCount;
    
    byteCount = atoi(bp+14/*strlen("%%BeginBinary:")*/);
    if( byteCount < 0 || fseek( fp, byteCount, SEEK_CUR ) == -1 ){
	syslog( LOG_ERR, "skipBinary: fseek past binary failed, %m\n");
	exit( 0 );
    }
}
     
static long
find_trailer( FILE *fp, long location )
{
  char buf[1024];
  char *bp, *np;
  long trailer = 0;
  long prev_line;
  int file_count = 0;
  
  if( fseek( fp, location, 0 ) == -1 ){
    syslog( LOG_ERR, "find_trailer: fseek to %d failed, %m\n", location );
    exit( 0 );
  }
  
  prev_line = ftell( fp );
  while( fgets( buf, sizeof(buf), fp ) ){
    bp = buf;
    if( strncmp( buf, "%%", 2 ) == 0 ){
      if ( _strmatch(bp,"%%BeginBinary:"))
          skipBinary(bp,fp);
      else {
	    bp += 2;
	    if( _strmatch( bp, "BeginDocument:" ) ||
	        _strmatch( bp, "BeginFile:" ) )
	        file_count++;
	    else if( _strmatch( bp, "EndDocument" ) || 
	             _strmatch( bp, "EndFile" ) )
	        file_count--;
	    else if( file_count )
	        continue;
	    else if( _strmatch( bp, "Trailer" ) ){
		trailer = prev_line;
		break;
	    }
      }
    }
    prev_line = ftell( fp );
  }
  
  return( trailer );
}

static long
find_next_page( FILE *fp, long location )
{
  char buf[1024];
  char *bp, *np;
  long page = 0;
  long prev_line;
  int file_count = 0;
  
  if( fseek( fp, location, 0 ) == -1 ){
    syslog( LOG_ERR, "find_next_page: fseek to %d failed, %m\n", location );
    exit( 0 );
  }
  
  prev_line = ftell( fp );
  fgets( buf, sizeof(buf), fp );
  if( debug )
    fprintf( stderr, "%%Starting search at %d, %s", prev_line, buf );
  prev_line = ftell( fp );
  while( fgets( buf, sizeof(buf), fp ) ){
    bp = buf;
    if( strncmp( buf, "%%", 2 ) == 0 ){
      if ( _strmatch(bp,"%%BeginBinary:"))
          skipBinary(bp,fp);
      else {
	    bp += 2;
	    if( _strmatch( bp, "BeginDocument:" ) || 
	        _strmatch( bp, "BeginFile:" ) ){
		if( debug )
		    fprintf( stderr, 
		             "%% inserted document start at %d\n", prev_line );
		file_count++;
	    }
	    else if( _strmatch( bp, "EndDocument" ) || 
	             _strmatch( bp, "EndFile" ) ){
		if( debug )
		    fprintf( stderr, 
		            "%% inserted document end at %d\n", prev_line );
		file_count--;
	    }
	    else if( file_count )
	        continue;
	    else if( _strmatch( bp, "Page:" ) || 
	             _strmatch( bp, "Trailer" )){
		if( debug )
		    fprintf( stderr, 
		             "%% page match found at %d\n", prev_line );
		page = prev_line;
		break;
	    }
	}
    }
    prev_line = ftell( fp );
  }
  
  return( page );
}

static long
find_header( FILE *fp )
{
  char buf[1024];
  char *bp, *np;
  long header = 0;
  int file_count = 0;
  
  while( fgets( buf, sizeof(buf), fp ) ){
    bp = buf;
    if( strncmp( buf, "%%", 2 ) == 0 ){
      if ( _strmatch(bp,"%%BeginBinary:"))
          skipBinary(bp,fp);
      else {
	    bp += 2;
	    if( _strmatch( bp, "BeginDocument:" ) || 
	        _strmatch( bp, "BeginFile:" ) )
	        file_count++;
	    else if( _strmatch( bp, "EndDocument" ) || 
	             _strmatch( bp, "EndFile" ) )
	        file_count--;
	    if( file_count )
	        continue;
	    else if( (np = _strmatch( bp, "EndProlog" )) && !header)
	        header = ftell( fp );
	    else if( (np = _strmatch( bp, "EndSetup" )) ){
		header = ftell( fp );
		break;
	    } else if( (np = _strmatch( bp, "Page:" )) ){
	        break;
	    }
      }
    }
  }

  /* Reset the file pointer. */
  if( fseek( fp, 0, 0 ) == -1 ){
    syslog( LOG_ERR, "find_header: fseek to %d failed, %m\n", 0 );
    exit( 0 );
  }
  
  return( header );
}
  
static port_t
npd_connect( char *printer, char *user, int copies )
{
  msg_header_t rec_msg;
  msg_return_t ret_code;
  port_t npd_port;
  npd_con_msg *con_msg;

  npd_port = FindPort( NPD_PUBLIC_PORT );
  if( !npd_port )
    return( 0 );

  npcomm_port = CreatePort( );

  con_msg = (npd_con_msg *)malloc( sizeof(npd_con_msg) );
  con_msg->head.msg_remote_port = npd_port;
  con_msg->head.msg_local_port = npcomm_port;
  con_msg->head.msg_simple = 1;
  con_msg->head.msg_type = MSG_TYPE_NORMAL;
  con_msg->head.msg_id = NPD_CONNECT_FROM_LPD;
  con_msg->head.msg_size = sizeof( npd_con_msg ) + 4;

  con_msg->printer_type.msg_type_name = MSG_TYPE_CHAR;
  con_msg->printer_type.msg_type_size = 8;  
  con_msg->printer_type.msg_type_number = sizeof(con_msg->printer);
  con_msg->printer_type.msg_type_inline = 1;  
  con_msg->printer_type.msg_type_longform = 0;
  con_msg->printer_type.msg_type_deallocate = 0;
  strcpy( con_msg->printer, printer );

  con_msg->user_type.msg_type_name = MSG_TYPE_CHAR;
  con_msg->user_type.msg_type_size = 8;  
  con_msg->user_type.msg_type_number = sizeof(con_msg->user);
  con_msg->user_type.msg_type_inline = 1;  
  con_msg->user_type.msg_type_longform = 0;
  con_msg->user_type.msg_type_deallocate = 0;
  strcpy( con_msg->user, user );

  con_msg->orig_host_type.msg_type_name = MSG_TYPE_CHAR;
  con_msg->orig_host_type.msg_type_size = 8;  
  con_msg->orig_host_type.msg_type_number = sizeof(con_msg->orig_host);
  con_msg->orig_host_type.msg_type_inline = 1;  
  con_msg->orig_host_type.msg_type_longform = 0;
  con_msg->orig_host_type.msg_type_deallocate = 0;
  strcpy( con_msg->orig_host, host );

  con_msg->copies_type.msg_type_name = MSG_TYPE_INTEGER_32;
  con_msg->copies_type.msg_type_size = 32;  
  con_msg->copies_type.msg_type_number = 1;
  con_msg->copies_type.msg_type_inline = 1;  
  con_msg->copies_type.msg_type_longform = 0;
  con_msg->copies_type.msg_type_deallocate = 0;
  con_msg->copies = copies;

  ret_code = msg_send( con_msg, SEND_TIMEOUT, DONE_TIMEOUT*1000 );
  if( ret_code != SEND_SUCCESS ){
    syslog( LOG_ERR, "npd_connect send failed, %d", ret_code );
    DeletePort( npcomm_port );
    DeletePort( npd_port );
    return( 0 );
  }

  rec_msg.msg_local_port = npcomm_port;
  rec_msg.msg_size = sizeof( rec_msg );
  ret_code = msg_receive( &rec_msg, RCV_TIMEOUT, DONE_TIMEOUT*1000 );
  if( ret_code != RCV_SUCCESS ){
    syslog( LOG_ERR, "npd_connect receive failed, %d", ret_code );
    DeletePort( npcomm_port );
    DeletePort( npd_port );
    return( 0 );
  }

  DeletePort( npd_port );

  return( rec_msg.msg_remote_port );
}

static msg_return_t
npd_send( port_t p, char *data, int len, int id )
{
  npd_receive_msg *send_msg;
  msg_return_t ret_code;
  char *mem;


  send_msg = (npd_receive_msg *)malloc( sizeof(npd_receive_msg) );
  send_msg->head.msg_simple = 0;
  send_msg->head.msg_remote_port = p;
  send_msg->head.msg_local_port = npcomm_port;
  send_msg->head.msg_size = sizeof( npd_receive_msg );
  send_msg->head.msg_id = id;
  send_msg->head.msg_type = MSG_TYPE_NORMAL;
  send_msg->type.msg_type_header.msg_type_name = MSG_TYPE_CHAR;
  send_msg->type.msg_type_header.msg_type_deallocate = 1;
  send_msg->type.msg_type_header.msg_type_inline = 0;
  send_msg->type.msg_type_header.msg_type_longform = 1;
  send_msg->type.msg_type_long_size = 8;
  send_msg->type.msg_type_long_number = len;

  vm_allocate( task_self(), (vm_address_t *)&mem, len, 1 );
  send_msg->data = mem;
  bcopy( data, mem, len );
  ret_code = msg_send( send_msg, SEND_TIMEOUT, DONE_TIMEOUT*1000 );

  vm_deallocate( task_self(), (vm_address_t)mem, len );

  return( ret_code );
}


