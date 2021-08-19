/*
 * nptest_print.c
 *
 * This program prints test pages on the NeXT Personal Laser Printer.
 *
 * P.King	April 21, 1988.
 */

#define	DEFAULTPAGESTOPRINT	8
#define	CS_BUGFIX	1	/* This is because features.h isn't working */
#define CMUCS		1	/* This is to get useful decls in errno.h */

#include <sys/features.h>
#include <mach.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/features.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <nextdev/npio.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/time.h>

#define	WORDSIZE	32
#define	CELLSIZE	320

u_int		resolution = 300;
int		width;
int		height;
int	black = 0xffffffff;
char		*myname;

draw_cell(vaddr, x, y)
    unsigned int	*vaddr;
    int x,y;
{
    unsigned int 	*celladdr;
    register int	i;
    register int	j;
    
    
    celladdr = vaddr + (width * y / WORDSIZE) + (x / WORDSIZE);
    
    /*
     * Top line
     */
    for (i = 0; i < CELLSIZE/WORDSIZE; i++) {
	celladdr[i] = black;
    }
    
    for (j = 1; j < CELLSIZE; j ++) {
	unsigned int *lineaddr = 
	    &celladdr[j * width / WORDSIZE];
	/*
	 * Side line
	 */
	*lineaddr |= 0x80000000;
	
	/*
	 * Verticle bars
	 */
	if (j >= WORDSIZE && j < 9 * WORDSIZE) {
	    lineaddr[2] = lineaddr[3] =
		lineaddr[6] = lineaddr[7] = black;
	}
	
	/*
	 * Horizontal bars
	 */
	if (j >= 4 * WORDSIZE && j < 6 * WORDSIZE) {
	    lineaddr[4] = lineaddr[5] = black;
	}
    }
}


draw_page( vaddr)
    unsigned int	*vaddr;
{
    int i;
    int j;
    unsigned int	*lineaddr;
    
    /*
     * Draw Cells.
     */
    for (i = 0; i < width; i += CELLSIZE) {
	for (j = 0; j < height; j += CELLSIZE) {
	    draw_cell(vaddr, i, j);
	}
    }
    
    /*
     * Left line.
     */
    for (j = 0; j < height; j++) {
	vaddr[((width * (j+1) / WORDSIZE)- 1)] |= 0x01;
    }
    
    /*
     * Bottom line.
     */
    lineaddr = vaddr + ((height - 2) * width / WORDSIZE);
    for (i = 0; i < (width / WORDSIZE); i++);
    {
	lineaddr[i] = black;
    }
}

print_status(printer)
    int	printer;
{
    struct npop	op;
    register int		stflags;
    short	notfirst = 0;
    
    op.np_op = NPGETSTATUS;
    if (ioctl(printer, NPIOCPOP, &op) == -1) {
	printf("<GETSTATUS failed, error = %d>\n", errno);
	return;
    }
    
#define ADDSTRING(string) \
    if (notfirst) \
	putchar(','); \
	    printf string; \
		notfirst++; \
		    
		    stflags = op.np_status.flags;
    putchar('<');
    if (stflags & NPPAPERDELIVERY) {
	ADDSTRING(("paper delivery"));
    }
    if (stflags & NPDATARETRANS) {
	ADDSTRING(("retrans=%d", op.np_status.retrans));
    }
    if (stflags & NPCOLD) {
	ADDSTRING(("cold"));
    }
    if (stflags & NPNOCARTRIDGE) {
	ADDSTRING(("no cartridge"));
    }
    if (stflags & NPNOPAPER) {
	ADDSTRING(("no paper"));
    }
    if (stflags & NPPAPERJAM) {
	ADDSTRING(("paper jam"));
    }
    if (stflags & NPDOOROPEN) {
	ADDSTRING(("door open"));
    }
    if (stflags & NPNOTONER) {
	ADDSTRING(("toner low"));
    }
    if (stflags & NPHARDWAREBAD) {
	ADDSTRING(("hardware failure"));
    }
    if (stflags & NPMANUALFEED) {
	ADDSTRING(("manual feed"));
    }
    printf(">\n");
}

/*
 * Routine: ready_printer
 * Function:
 *	Wait for printer to become either ready, report errors
 *	periodically.  Note: If the power of the printer is off and is
 *	not in the process of being turned on, this routine will hang
 *	forever.
 */
ready_printer(printer)
    int	printer;
{
    int nfds;
    int writefds, excfds;
    
    
    while (TRUE) {
	/*
	 * Set up select bit masks.
	 */
	writefds = excfds = 1 << printer;
	printf("Calling select\n");
	if ((nfds = select(32, 0, &writefds,
			   &excfds, 0)) <= 0) {
	    /*
	     * Should never time out, so a
	     * return of 0 is bogus as well as -1.
	     */
	    fprintf(stderr, "%s: ready_printer select failed: ",myname);
	    perror(0);
	    exit(1);
	}
	
	if (nfds != 1) {
	    fprintf(stderr, "%s: select returned bogus value\n", myname);
	    exit(1);
	}
	
	if (writefds == (1<<printer)) {
	    printf("ready_printer select returns WRITE\n");
	    return(0);
	} else if (excfds == (1<<printer)) {
	    printf("Select returns EXCEPTION, status = ");
	    print_status(printer);
	    printf("Sleeping\n");
	    sleep(10);
	} else {
	    fprintf(stderr, "%s: select returned bogus masks\n", myname);
	    exit(1);
	}
    }
}


usage(argv)
    char 	**argv;
{
    fprintf(stderr, "usage: %s [300 | 400 | blank | handfeed]\n", myname);
    exit(1);
}

main(argc, argv)
    int	argc;
    char	**argv;
{
    vm_offset_t	image;
    int		image_size;
    char		blank = 0;
    struct npop	op;
    int		pages;
    struct timeval	tv;
    int		printer;
    int		pflags;
    char		handfeed = 0;
    int		pagestoprint = DEFAULTPAGESTOPRINT;
    
    /*
     * Check the arguments.
     */
    myname = argv[0];
    
    if (argc > 2) {
	usage(argv);
    }
    
    /*
     * Parse the arguments.
     */
    if (argc > 1) {
	if (strcmp(argv[1], "300") == 0) {
	    resolution = 300;
	} else if (strcmp(argv[1], "400") == 0) {
	    resolution = 400;
	} else if (strcmp(argv[1], "blank") == 0) {
	    blank++;
	} else if (strcmp(argv[1], "handfeed") == 0) {
	    handfeed++;
	} else {
	    usage(argv);
	}
    }
    
    /*
     * Open and initialize the printer.
     */
    if ((printer = open("/dev/np0", O_WRONLY|O_NDELAY)) == -1) {
	fprintf(stderr, "%s: printer open failed: ", myname);
	perror(0);
	exit(1);
    }
    
    /*
     * Set up the printer to be non-blocking and asynchronous.
     */
    if ((pflags = fcntl(printer, F_GETFL, 0)) == -1) {
	fprintf(stderr, "%s: F_GETFL failed: ", myname);
	perror(0);
	exit(1);
    }
    
    pflags |= (FNDELAY);
    if (fcntl(printer, F_SETFL, pflags) == -1) {
	fprintf(stderr, "%s: F_SETFL failed: ", myname);
	perror(0);
	exit(1);
    }
    
    /*
     * Initialize some parameters.
     */
    width = CELLSIZE * 7;
    height = CELLSIZE * 9;
    
    image_size = width * height / NBBY;
    while (image_size & 0x0f) { 
	/*
	 * Quad word align the end.
	 */
	image_size++;
    }
    
    /*
     * Get some memory for the image.
     */
    if (vm_allocate(task_self_, &image, image_size, TRUE) !=
	KERN_SUCCESS) {
	fprintf(stderr, "%s: vm_allocate failed trying to get image memory\n", myname);
	exit(1);
    }
    
    /*
     * Draw the image.
     */
    if (!blank) {
	draw_page(image);
    }
    
    /*
     * Since we are asynchonous, we need to wait for the printer
     * to become ready (i.e. powered up).
     */
    ready_printer(printer);
    
    /*
     * Set the page parameters.
     */
    op.np_op = NPSETRESOLUTION;
    if (resolution == 400) {
	op.np_resolution = DPI400;
    } else {
	op.np_resolution = DPI300;
    }
    
    /*
     * Set up page resolution
     */
    if (ioctl(printer, NPIOCPOP, &op) == -1) {
	fprintf(stderr, "%s: NPSETRESOLUTION failed: ", myname);
	perror(0);
	exit(1);
    }
    
    
    /*
     * Set up paper feeding parameters.
     */
    if (handfeed) {
	op.np_op = NPSETMANUALFEED;
	op.np_bool = TRUE;
	while (ioctl(printer, NPIOCPOP, &op) == -1) {
	    if (errno == EWOULDBLOCK || errno == EDEVERR) {
		printf("NPSETMANUALFEED failed, calling ready_printer()\n");
		ready_printer();
		continue;
	    }
	    fprintf(stderr, "%s: NPSETMANUALFEED failed: ", myname);
	    perror(0);
	    exit(1);
	}
    }
    
    op.np_op = NPSETMARGINS;
    if (resolution == 400) {
	op.np_margins.left = 200;
	op.np_margins.top = 200;
    } else {
	op.np_margins.left = 142;
	op.np_margins.top =150;
    }
    op.np_margins.width = width / (NBPW*NBBY); /* Longwords */
    op.np_margins.height = height;
    
    for (pages = 1; pages <= pagestoprint; pages++) {
	if (ioctl(printer, NPIOCPOP, &op) == -1) {
	    fprintf(stderr, "%s: NPSETMARGINS failed: ",myname);
	    perror(0);
	    exit(1);
	}     
	
	while (printf("Calling write\n"),
	       (write(printer, image, image_size) == -1)) {
	    if (errno == EWOULDBLOCK || errno == EDEVERR) {
		printf("Write failed page %d, calling ready_printer()\n", pages);
		ready_printer(printer);
		continue;
	    }
	    fprintf(stderr, "%s: write failed, error = %d: ",myname, errno);
	    perror(0);
	    exit(1);
	}
	printf("Write succeeded page %d\n", pages);
    }
    
    printf("Yahoo!\n");
    exit(0);
}

