/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = 	"@(#)recvjob.c	1.2 88/04/18 4.0NFSSRC SMI"; /* from UCB 5.4 6/6/86 */
#endif

/*
 * Receive printer jobs from the network, queue them and
 * start the printer daemon.
 */

#include "lp.h"
#ifdef	NeXT_MOD
#include <stdarg.h>
#undef	MAXUSERS
#endif	NeXT_MOD
#include <sys/vfs.h>

char	*sp = "";
#define ack()	(void) write(1, sp, 1);

static char    tfname[40];		/* tmp copy of cf before linking */
static char    dfname[40];		/* data files */
static int	minfree;		/* keep at least minfree blocks available */
static int	dfd;			/* file system device descriptor */

#ifdef	NeXT_MOD
/*
 * Forward declaration of functions.
 */
static int readjob();
static int readfile(char *file, int size);
static int noresponse();
static int rcleanup();
static int frecverr(char *msg, ...);
#endif	NeXT_MOD

recvjob()
{
	struct stat stb;
	char *bp = pbuf;
	int status, rcleanup();

	/*
	 * Perform lookup for printer name or abbreviation
	 */
	if ((status = pgetent(line, printer)) < 0)
		frecverr("cannot open printer description file");
	else if (status == 0)
		frecverr("recvjob unknown printer %s", printer);
	if ((LF = pgetstr("lf", &bp)) == NULL)
		LF = DEFLOGF;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;

	(void) close(2);			/* set up log file */
	if (open(LF, O_WRONLY|O_APPEND, 0664) < 0) {
		syslog(LOG_ERR, "%s: %m", LF);
		(void) open("/dev/null", O_WRONLY);
	}

#ifdef	NeXT_MOD
	make_dir(SD);
#endif
	if (chdir(SD) < 0)
		frecverr("%s: %s: %m", printer, SD);
	if (stat(LO, &stb) == 0) {
		if (stb.st_mode & 010) {
			/* queue is disabled */
			putchar('\1');		/* return error code */
			exit(1);
		}
	}

	/*
	 * Open the spooling directory, so that we have a descriptor on which
	 * to do an "fstatfs" to find out how much space is left on the file
	 * system containing that directory.
	 */
	if ((dfd = open(".", O_RDONLY)) < 0)
		frecverr("%s: %s: %m", printer, SD);
	minfree = read_number("minfree");
	signal(SIGTERM, rcleanup);
	signal(SIGPIPE, rcleanup);

	if (readjob())
		printjob();
}

/*
 * Read printer jobs sent by lpd and copy them to the spooling directory.
 * Return the number of jobs successfully transfered.
 */
static
readjob()
{
	register int size, nfiles;
	register char *cp;

	ack();
	nfiles = 0;
	for (;;) {
		/*
		 * Read a command to tell us what to do
		 */
		cp = line;
		do {
			if ((size = read(1, cp, 1)) != 1) {
				if (size < 0)
					frecverr("%s: Lost connection",printer);
				return(nfiles);
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = line;
		switch (*cp++) {
		case '\1':	/* cleanup because data sent was bad */
			rcleanup();
			continue;

		case '\2':	/* read cf file */
			size = 0;
			while (*cp >= '0' && *cp <= '9')
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			/*
			 * Security fix: make sure that the file name
			 * is relative to the current directory,
			 * otherwise someone can write /etc/passwd.
			 */
			if (index(cp, '/')) {
			    frecverr("security violation: "
				     "%s trying to write %s",
				     from, cp);
			}
			/*
			 * host name has been authenticated, we use our
			 * view of the host name since we may be passed
			 * something different than what gethostbyaddr()
			 * returns
			 */
			strcpy(cp + 6, from);
			strcpy(tfname, cp);
			tfname[0] = 't';
			if (!chksize(size)) {
				(void) write(1, "\2", 1);
				continue;
			}
			if (!readfile(tfname, size)) {
				rcleanup();
				continue;
			}
			if (link(tfname, cp) < 0)
				frecverr("%s: %m", tfname);
			(void) unlink(tfname);
			tfname[0] = '\0';
			nfiles++;
			continue;

		case '\3':	/* read df file */
			size = 0;
			while (*cp >= '0' && *cp <= '9')
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			/*
			 * Security fix: make sure that the file name
			 * is relative to the current directory,
			 * otherwise someone can write /etc/passwd.
			 */
			if (index(cp, '/')) {
			    frecverr("security violation: "
				     "%s trying to write %s",
				     from, cp);
			}
			if (!chksize(size)) {
				(void) write(1, "\2", 1);
				continue;
			}
			strcpy(dfname, cp);
			(void) readfile(dfname, size);
			continue;
		}
		frecverr("protocol screwup");
	}
}

/*
 * Read files send by lpd and copy them to the spooling directory.
 */
static
readfile(file, size)
	char *file;
	int size;
{
	register char *cp;
	char buf[BUFSIZ];
	register int i, j, amt;
	int fd, err;

	fd = open(file, O_WRONLY|O_CREAT, FILMOD);
	if (fd < 0)
		frecverr("%s: %m", file);
	ack();
	err = 0;
	for (i = 0; i < size; i += BUFSIZ) {
		amt = BUFSIZ;
		cp = buf;
		if (i + amt > size)
			amt = size - i;
		do {
			j = read(1, cp, amt);
			if (j <= 0)
				frecverr("Lost connection");
			amt -= j;
			cp += j;
		} while (amt > 0);
		amt = BUFSIZ;
		if (i + amt > size)
			amt = size - i;
		if (write(fd, buf, amt) != amt) {
			err++;
			break;
		}
	}
	(void) close(fd);
	if (err)
		frecverr("%s: write error", file);
	if (noresponse()) {		/* file sent had bad data in it */
		(void) unlink(file);
		return(0);
	}
	ack();
	return(1);
}

static
noresponse()
{
	char resp;

	if (read(1, &resp, 1) != 1)
		frecverr("Lost connection");
	if (resp == '\0')
		return(0);
	return(1);
}

/*
 * Check to see if there is enough space on the file system for size bytes.
 * 1 == OK, 0 == Not OK.
 */
chksize(size)
	int size;
{
	int spacefree;
	struct statfs statfs;

	if (fstatfs(dfd, &statfs) < 0)
		return(1);
	spacefree = (statfs.f_bavail * statfs.f_bsize + 1023) / 1024;
	size = (size + 1023) / 1024;
	if (minfree + size > spacefree)
		return(0);
	return(1);
}

read_number(fn)
	char *fn;
{
	char lin[80];
	register FILE *fp;

	if ((fp = fopen(fn, "r")) == NULL)
		return (0);
	if (fgets(lin, 80, fp) == NULL) {
		fclose(fp);
		return (0);
	}
	fclose(fp);
	return (atoi(lin));
}

/*
 * Remove all the files associated with the current job being transfered.
 */
static
rcleanup()
{

	if (tfname[0])
		(void) unlink(tfname);
	if (dfname[0])
		do {
			do
				(void) unlink(dfname);
			while (dfname[2]-- != 'A');
			dfname[2] = 'z';
		} while (dfname[0]-- != 'd');
	dfname[0] = '\0';
}

#ifdef	NeXT_MOD
static
frecverr(char *msg, ...)
{
	va_list		ap;
	int		a1, a2;

	rcleanup();
	va_start(ap, msg);
	a1 = va_arg(ap, int);
	a2 = va_arg(ap, int);
	syslog(LOG_ERR, msg, a1, a2);
	va_end(ap);
	putchar('\1');		/* return error code */
	exit(1);
}
#else	NeXT_MOD
static
frecverr(msg, a1, a2)
	char *msg;
{
	rcleanup();
	syslog(LOG_ERR, msg, a1, a2);
	putchar('\1');		/* return error code */
	exit(1);
}
#endif	NeXT_MOD
