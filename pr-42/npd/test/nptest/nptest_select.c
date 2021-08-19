/*
 * nptest_select.c
 *
 * This program hammers on the select and GETSTATUS ioctl system calls.
 *
 * P.King	March 21, 1990.
 */

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

int	printer;

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

main(int argc, char **argv)
{
    int		nfds;
    int		writefds;
    int		excfds;
    char	*myname = argv[0];
    int		pflags;

    /*
     * Open and initialize the printer.
     */
    if ((printer = open("/dev/np0", O_WRONLY)) == -1) {
	fprintf(stderr, "%s: printer open failed: ", myname);
	perror(0);
	exit(1);
    }

    /*
     * Set up the printer to be non-blocking.
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
	    printf("ready_printer select returns WRITE, sleeping\n");
	    sleep(10);
	} else if (excfds == (1<<printer)) {
	    printf("Select returns EXCEPTION, status = ");
	    print_status(printer);
	} else {
	    fprintf(stderr, "%s: select returned bogus masks\n", myname);
	    exit(1);
	}
    }

}
