/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)startdaemon.c	5.1 (Berkeley) 6/6/85";
#endif not lint

/*
 * Tell the printer daemon that there are new files in the spool directory.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>
#include "lp.local.h"

static perr();

startdaemon(printer)
	char *printer;
{
	extern char *name;
	struct sockaddr_un skun;
	register int s, n;
	char buf[BUFSIZ];

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perr("socket");
		return(0);
	}
	skun.sun_family = AF_UNIX;
	strcpy(skun.sun_path, SOCKETNAME);
	if (connect(s, (struct sockaddr *)&skun,
	    strlen(skun.sun_path) + 2) < 0) {
		perr("connect", skun.sun_path);
		(void) close(s);
		return(0);
	}
	(void) sprintf(buf, "\1%s\n", printer);
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
	fflush (stdout);
	(void) close(s);
	return(0);
}

static
perr(msg, msg2)
	char *msg, *msg2;
{
	extern char *name;
	extern int sys_nerr;
	extern char *sys_errlist[];
	extern int errno;

	/* Added debugging code...   DJA */
	syslog (LOG_ERR, "Name %s Msg %s \"%s\" %m", 
		name, msg, (msg2 != NULL ? msg2 : "" ) );
	printf("%s: %s: ", name, msg);
	fputs(errno < sys_nerr ? sys_errlist[errno] : "Unknown error" , stdout);
	putchar('\n');
}
