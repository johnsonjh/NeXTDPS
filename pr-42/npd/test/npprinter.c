#include <stdlib.h>
#include <dpsclient/dpsclient.h>
#include <dpsclient/dpsNeXT.h>
#include <sys/file.h>
#include <text/text.h>
#include <varargs.h>    
#include <cthreads.h>
    
/*    
#if 0


#endif
*/


static void mytextproc();

char *Progname;

static int verbose = 0;

/* Progress Stuff -- send emacs-style processing messages to a file.
*/

#define pgf stdout

void
progress_message(va_alist)
va_dcl
{
    va_list args;
    char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
    vfprintf(pgf, fmt, args);
    fprintf(pgf, " . . . ");
    fflush(pgf);
    va_end(args);
}

void
progress_done()
{

    fprintf(pgf, "done.\n");
    fflush(pgf);
}
void
progress_end()
{
    fprintf(pgf,"\n");
    fflush(pgf);
}




/* --------- DPS client library callback for output from interpreter
*/

static void
mytextproc(ctxt, buf, cnt)
DPSContext ctxt;
char *buf;
unsigned long cnt;
{
  int k;
  unsigned char outbyte;

  printf("PS Output> %*s\n", cnt, buf);
}
#define erf stderr
/* error proc for callback from interpreter */
static void
myerrorproc(DPSContext ctxt, DPSErrorCode e, 
	unsigned long arg1, unsigned long arg2)
{
    fprintf(erf,"\nPostScript Error:\n");
    DPSPrintError(erf, (DPSBinObjSeq)arg1);
    exit (-1);
}

static double width_inches, height_inches;
static char *port_name;
static condition_t check_in_ok;
static port_t portId;
static int dpi = 400;
static int sender_done = 0;   


int
main(ac,av)
int ac;
char **av;
{
    DPSContext ps;

    char *psfile;
    int k;
    cthread_t printer_thread;
    
    Progname = basename(*av++); ac--;

    /* Parse arguments. */

    psfile = 0;
    
    for (k = 0; k < ac; k++, av++) {
	if (!strcmp(*av, "-pf")) {
	    psfile = *++av; ac--;
	    continue;
	}
	else if ( ! strcmp(*av, "-dpi")) {
	    dpi = atoi(*++av);
	    if(dpi != 400) dpi = 300;
	    ac--;
	    continue;
	} else {
	    printf("npprinter: Bad argument\n");
	    break;
	}
    }

    if( ! psfile) {
	printf("npprinter -pf <psfile>  [ -dpi <num> ] \n");
	exit(1);
    }
    /* hard coded variable for 8.5 x 11 page */
    width_inches = 8.5;
    height_inches = 11.0;
    port_name = "printerport";
#if 1
    setup_printer_port();
    
    /* now fork a page printer to receive the messages */
    {
	extern void operate_printer();
	printer_thread = cthread_fork(operate_printer, 0);
    }
#endif

    progress_message("connecting to PostScript");
    ps = DPSCreateContext((char *)0, (char *) 0, mytextproc,
			  myerrorproc);
    progress_done();
    
    progress_message("sending commands to PostScript");


    /* set up resolution in statusdict */
    DPSPrintf(ps, "statusdict /MachPortDeviceArgs %s put\n",
	      (dpi == 300) ? "NeXTLaser-300" : "NeXTLaser-400");
    
    /* create machportdevice */
    DPSPrintf(ps,
	      "%d %d [0 0 %d %d] [%g 0 0 %g 0 %d] () (%s) "
	      "machportdevice"
	      "\n",
	      (int) width_inches*dpi, (int) height_inches*dpi,
	      (int) width_inches*dpi, (int) height_inches*dpi,
	      dpi/72., -dpi/72., (int) height_inches*dpi,
	      port_name
	      );

    /* munge the file name into absolute form */
    { 
	char *abs_filename = (psfile[0] == '/') ? psfile :
	    fullPath(pwd(), psfile);
	
    
	/* finally, send the file */
	DPSPrintf(ps, "(%s) run \n", abs_filename);
    }
    progress_done();
    
    progress_message("waiting for PostScript to finish");
    DPSWaitContext(ps);
    progress_done();

    sender_done = 1;
    progress_message("waiting for printer thread to finish");
    printf(cthread_join(printer_thread) ? "[printer error] " :"[printer ok] ");
    progress_done();
    DPSDestroyContext(ps);
    progress_end();
    exit(0);
}




/*************** Support for printer device control */


#include <mach.h>
#include <sys/message.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <nextdev/npio.h>
#include <servers/netname.h>
#include "printmessage.h"





static int
npPowerOn(fd)
int fd;
{
    struct npop o;
    o.np_op = NPSETPOWER;
    o.np_power = 1;
    return(ioctl(fd, NPIOCPOP, &o));
}
static int
npPowerOff(fd)
int fd;
{
    struct npop o;
    o.np_op = NPSETPOWER;
    o.np_power = 0;
    return(ioctl(fd, NPIOCPOP, &o));
}

static int
npSetResolution(fd,dpi)
int fd;
int dpi;
{
    struct npop o;
    o.np_op = NPSETRESOLUTION;
    o.np_resolution = (dpi > 300)? DPI400 : DPI300;
    return(ioctl(fd, NPIOCPOP, &o));
}

static int
npSetMargins(fd, top, left, longs_per_scanline, numlines)
int fd, top, left, longs_per_scanline, numlines;
{
    struct npop o;

    o.np_op = NPSETMARGINS;
    o.np_margins.left = left;
    o.np_margins.top = top;
    o.np_margins.width = longs_per_scanline;
    o.np_margins.height = numlines;
    return(ioctl(fd, NPIOCPOP, &o));
}


static void
report_status(port)
port_t port;
{
    kern_return_t error;
    port_set_name_t enabled;
    int num_msgs, backlog;
    boolean_t owner, reciever;
    
    if ((error = port_status(task_self(), port, &enabled, &num_msgs,
			     &backlog, &owner, &reciever)) != KERN_SUCCESS)
	mach_error("server: port_status: ", error), exit(1);

    printf("Port Status Information:\n");
    printf("\tPort Name:       %d\n", port);
    printf("\tPort Set Name:   %d\n", enabled);
    printf("\tMessage Q'd:     %d\n", num_msgs);
    printf("\tBacklog:         %d\n", backlog);
    printf("\tOwner?:          %s\n", owner ? "true" : "false");
    printf("\tReceive Rights?: %s\n", reciever ? "true" : "false");
}


static int
num_queued_msgs(port)
port_t port;
{
    kern_return_t error;
    port_set_name_t enabled;
    int num_msgs, backlog;
    boolean_t owner, reciever;
    
    if ((error = port_status(task_self(), port, &enabled, &num_msgs,
			     &backlog, &owner, &reciever)) != KERN_SUCCESS){
	
	mach_error("server: port_status: ", error);
	return(-1);  	/* problem 0 is probably the safest return */
    }
    return(num_msgs);

}



void
setup_printer_port()
{
    kern_return_t error;
  
    /* first allocate a port */

    if ((error = port_allocate(task_self(), &portId)) != KERN_SUCCESS) {
	mach_error("server: port_allocate returned ", error);
	exit(1);
    }

    /* limit the ports backlog to 2 */

    if((error = port_set_backlog(task_self(), portId, 10)) != KERN_SUCCESS)
	mach_error("server: port_set_backlog: ", error), exit(1);

    /* report information about the port */

    report_status(portId);
	
    /* now, register that name with the name server */

    if ((error = netname_check_in( name_server_port, port_name,
				  (port_t) 0, portId)) !=
	NETNAME_SUCCESS) {
	mach_error("server: netname_check in returned ", error);
	exit(1);
    }
}

static int
isReasonablePrintMessage(msg)
PrintPageMessage *msg;
{
    return(msg->msgHeader.msg_id == PrintPageMsgId &&
	   msg->msgHeader.msg_size == sizeof(*msg) &&
	   msg->printPageVersion == PRINTPAGEVERSION && /* i.e. same
							  printmessage.h */
	   msg->lineBytes*msg->pixelsHigh == msg->PrinterDataLongWords*4
	   );
}


/* the actual printing thread routine.
 * The basic strategy is to open the printer and then wait for
 * printpage messages on the port.
 */ 

void
operate_printer()
{
    PrintPageMessage msg;
    msg_return_t msg_return;
    int npFd, e;
    int leftDots;

  
    /* open up the printer device */

    if((npFd = open("/dev/np0", O_WRONLY, 0)) < 0) 
	perror("opening printer"), exit(1);
    npPowerOn(npFd);
    npSetResolution(npFd, dpi);


    /* next, we wait for a message in the port */

    msg.msgHeader.msg_size = sizeof(msg);
    msg.msgHeader.msg_local_port = portId;
    msg.msgHeader.msg_remote_port = (port_name_t) 0;

    while(1) {
	if ( sender_done && num_queued_msgs(portId) <= 0) {
	    npPowerOff(npFd);
	    close(npFd);
	    cthread_exit(0);
	}
	switch(msg_return = msg_receive(&msg, RCV_TIMEOUT,
					(msg_timeout_t) 2000 /* millisecs*/)) {
	case RCV_SUCCESS:
	    /* sanity check the message */
	    if (! isReasonablePrintMessage(&msg)) {
		printf("printer_thread: bogus message!\n");
		break;
	    }

	    printf("printer_thread: message received for page %d. "
		   "[%d more queued]\n", msg.pageNum, num_queued_msgs(portId));

	    /* 648 is the nominal printer path width in points, we
	       attempt to center the bitmap in the printerpath */
	    leftDots = (648/72*dpi - msg.pixelsWide)/2;
	    leftDots = (leftDots > 511) ? 511 : (leftDots< 0) ? 0 : leftDots;

	    /* the 85 and 60 come from the old printerdevice.c */
	    
	    npSetMargins(npFd, (dpi == 400) ? 85 : 60, leftDots,
			 msg.lineBytes/4, msg.pixelsHigh);

	    /* write the data to the printer device.  This seems to
	       block until the printer is ready to go (i.e. heated up,
	       full of paper, etc.
	       */
	    e = write(npFd, (char *)msg.PrinterData,
		      msg.PrinterDataLongWords*sizeof(long));
	    if(e != msg.PrinterDataLongWords*sizeof(long))
		perror("printer_thread: write error");

	    /* we must deallocate the bitmap when we're done */
	    vm_deallocate(task_self(), msg.PrinterData,
			  msg.PrinterDataLongWords*sizeof(long));
	    break;
	case RCV_TIMED_OUT:
	    printf("printer_thread: timed out\n");
	    break;
	default:
	    mach_error(msg_return);
	    npPowerOff(npFd);
	    close(npFd);
	    cthread_exit(1);
	}
	    
    }
}

