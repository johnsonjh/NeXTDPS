/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)lpd.c	5.4 (Berkeley) 5/6/86";
#endif not lint

/*
 * lpd -- line printer daemon.
 *
 * Listen for a connection and perform the requested operation.
 * Operations are:
 *	\1printer\n
 *		check the queue for jobs and print any found.
 *	\2printer\n
 *		receive a job from another machine and queue it.
 *	\3printer [users ...] [jobs ...]\n
 *		return the current state of the queue (short form).
 *	\4printer [users ...] [jobs ...]\n
 *		return the current state of the queue (long form).
 *	\5printer person [users ...] [jobs ...]\n
 *		remove jobs from the queue.
 *
 * Strategy to maintain protected spooling area:
 *	1. Spooling area is writable only by daemon and spooling group
 *	2. lpr runs setuid root and setgrp spooling group; it uses
 *	   root to access any file it wants (verifying things before
 *	   with an access call) and group id to know how it should
 *	   set up ownership of files in the spooling area.
 *	3. Files in spooling area are owned by root, group spooling
 *	   group, with mode 660.
 *	4. lpd, lpq and lprm run setuid daemon and setgrp spooling group to
 *	   access files and printer.  Users can't get to anything
 *	   w/o help of lpq and lprm programs.
 */

#include "lp.h"

#define SET_STACK_LIMIT_HUGE
#ifdef SET_STACK_LIMIT_HUGE
#include <sys/time.h>
#include <sys/resource.h>
#endif

int	lflag;				/* log requests flag */

int	reapchild();
int	mcleanup();

#ifdef NeXT_MOD
#define INFORM "/usr/lib/NextPrinter/Inform"
static unsigned char server_key[] = { 0x4e, 0x5e, 0x4e, 0x75, 0 };

#endif

#include <printerdb.h>
#include <objc/hashtable.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <grp.h>
#include <text/pathutil.h>
#include <text/fileutil.h>
#include <libc.h>

main(argc, argv)
	int argc;
	char **argv;
{
#if	NeXT_MOD
	int f, funix, finet, options, fromlen;
	fd_set defreadfds;
	int debug_flag = 0;
#else	NeXT_MOD
	int f, funix, finet, options, defreadfds, fromlen;
#endif	NeXT_MOD
	struct sockaddr_un skun, fromunix;
	struct sockaddr_in sin, frominet;
	int omask, lfd;

	gethostname(host, sizeof(host));
	name = argv[0];

	while (--argc > 0) {
		argv++;
		if (argv[0][0] == '-')
			switch (argv[0][1]) {
			case 'd':
				options |= SO_DEBUG;
				break;
#if		NeXT_MOD
			case 'D':
				debug_flag = 1;
				break;
#endif		NeXT_MOD
			case 'l':
				lflag++;
				break;
			}
	}

#ifdef SET_STACK_LIMIT_HUGE
  {
    struct rlimit rlim;

    /* Set the stack limit huge so that alloca (particularly stringtab
     * in dbxread.c) does not fail. */
    getrlimit (RLIMIT_STACK, &rlim);
    rlim.rlim_cur = rlim.rlim_max;
    setrlimit (RLIMIT_STACK, &rlim);
  }
#endif /* SET_STACK_LIMIT_HUGE */

#ifndef DEBUG
	/*
	 * Set up standard environment by detaching from the parent.
	 */
	if (!debug_flag) {
	    if (fork())
		    exit(0);
	    for (f = 0; f < 5; f++)
		    (void) close(f);
	    (void) open("/dev/null", O_RDONLY);
	    (void) open("/dev/null", O_WRONLY);
	    (void) dup(1);
	    f = open("/dev/tty", O_RDWR);
	    if (f > 0) {
		    ioctl(f, TIOCNOTTY, 0);
		    (void) close(f);
	    }
	}
#endif

	openlog("lpd", LOG_PID, LOG_LPR);
	(void) umask(0);
	lfd = open(MASTERLOCK, O_WRONLY|O_CREAT, 0644);
	if (lfd < 0) {
		syslog(LOG_ERR, "%s: %m", MASTERLOCK);
		exit(1);
	}
	if (flock(lfd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)	/* active deamon present */
			exit(0);
		syslog(LOG_ERR, "%s: %m", MASTERLOCK);
		exit(1);
	}
	ftruncate(lfd, 0);
	/*
	 * write process id for others to know
	 */
	sprintf(line, "%u\n", getpid());
	f = strlen(line);
	if (write(lfd, line, f) != f) {
		syslog(LOG_ERR, "%s: %m", MASTERLOCK);
		exit(1);
	}
	/*
	 * remove stale spool directories
	 */
#ifdef	NeXT_MOD
	make_dir(NEXT_SPOOL);
	checkDirectories();
#endif
	signal(SIGCHLD, reapchild);
	/*
	 * Restart all the printers.
	 */
	startup();
	(void) unlink(SOCKETNAME);
	funix = socket(AF_UNIX, SOCK_STREAM, 0);
	if (funix < 0) {
		syslog(LOG_ERR, "socket: %m");
		exit(1);
	}
#define	mask(s)	(1 << ((s) - 1))
	omask = sigblock(mask(SIGHUP)|mask(SIGINT)|mask(SIGQUIT)|mask(SIGTERM));
	signal(SIGHUP, mcleanup);
	signal(SIGINT, mcleanup);
	signal(SIGQUIT, mcleanup);
	signal(SIGTERM, mcleanup);
	skun.sun_family = AF_UNIX;
	strcpy(skun.sun_path, SOCKETNAME);
	if (bind(funix, (struct sockaddr *)&skun,
	    strlen(skun.sun_path) + 2) < 0) {
		syslog(LOG_ERR, "ubind: %m");
		exit(1);
	}
	sigsetmask(omask);
#if	NeXT_MOD
	FD_ZERO(&defreadfds);
	FD_SET(funix, &defreadfds);
#else	NeXT_MOD
	defreadfds = 1 << funix;
#endif	NeXT_MOD
	listen(funix, 5);
	finet = socket(AF_INET, SOCK_STREAM, 0);
	if (finet >= 0) {
		struct servent *sp;

		if (options & SO_DEBUG)
			if (setsockopt(finet, SOL_SOCKET, SO_DEBUG, 0, 0) < 0) {
				syslog(LOG_ERR, "setsockopt (SO_DEBUG): %m");
				mcleanup();
			}
		sp = getservbyname("printer", "tcp");
		if (sp == NULL) {
			syslog(LOG_ERR, "printer/tcp: unknown service");
			mcleanup();
		}
		sin.sin_family = AF_INET;
		sin.sin_port = sp->s_port;
#ifdef	NeXT_MOD
		sin.sin_addr.s_addr = INADDR_ANY;
		bzero( sin.sin_zero, sizeof(sin.sin_zero));
		if (bind(finet, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
#else	NeXT_MOD
		if (bind(finet, &sin, sizeof(sin), 0) < 0) {
#endif	NeXT_MOD
		  perror( "lpd bind:" ); 
		  syslog(LOG_ERR, "bind: %m");
		  mcleanup();
		}
#ifdef	NeXT_MOD
		FD_SET(finet, &defreadfds);
#else	NeXT_MOD
		defreadfds |= 1 << finet;
#endif	NeXT_MOD
		listen(finet, 5);
	}
	/*
	 * Main loop: accept, do a request, continue.
	 */
	for (;;) {
#ifdef	NeXT_MOD
		int domain, nfds, s;
		fd_set readfds = defreadfds;
#else	NeXT_MOD
		int domain, nfds, s, readfds = defreadfds;
#endif	NeXT_MOD
		nfds = select(20, &readfds, 0, 0, 0);
		if (nfds <= 0) {
			if (nfds < 0 && errno != EINTR)
				syslog(LOG_WARNING, "select: %m");
			continue;
		}
#ifdef	NeXT_MOD
		if (FD_ISSET(funix, &readfds)) {
#else	NeXT_MOD
		if (readfds & (1 << funix)) {
#endif	NeXT_MOD
			domain = AF_UNIX, fromlen = sizeof(fromunix);
			s = accept(funix, (struct sockaddr *)&fromunix,
			    &fromlen);
#ifdef	NeXT_MOD
		} else if (FD_ISSET(finet, &readfds)) {
#else	NeXT_MOD
		} else if (readfds & (1 << finet)) {
#endif	NeXT_MOD
			domain = AF_INET, fromlen = sizeof(frominet);
			s = accept(finet, (struct sockaddr *)&frominet,
			    &fromlen);
		}
		if (s < 0) {
			if (errno != EINTR)
				syslog(LOG_WARNING, "accept: %m");
			continue;
		}
		if (fork() == 0) {
			signal(SIGCHLD, SIG_IGN);
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			signal(SIGTERM, SIG_IGN);
			(void) close(funix);
			(void) close(finet);
			dup2(s, 1);
			(void) close(s);
			if (domain == AF_INET)
				chkhost(&frominet);
			doit();
			exit(0);
		}
		(void) close(s);
	}
}

#ifdef	NeXT_MOD

#define	strsave(s)	strcpy(malloc(strlen(s) + 1), s)

checkDirectories()
{
	char **strPtr, *spoolName = NEXT_SPOOL;
	register struct direct *d;
	char fullName[MAXPATHLEN + 1], printName[MAXPATHLEN + 1];
	DIR *dirp;
	struct stat statbuf;
	prdb_ent *entry;
	NXHashTablePrototype prototype;
	NXHashTable *table;

	/*
	Set up a string hashing table that will free its contents
	*/
	bcopy(&NXStrPrototype, &prototype, sizeof(NXHashTablePrototype));
	prototype.free = NXReallyFree;
	table = NXCreateHashTable(prototype, 0, NULL);

	/*
	Now loop through the printcap entries, putting the printer names
	in the hash table.
	*/
	prdb_set(NULL);
	while (entry = (prdb_ent *) prdb_get())
	{
		for(strPtr = entry->pe_name; strPtr[1]; strPtr++)
			;

		(void) NXHashInsertIfAbsent(table, strsave(*strPtr));
	}

	prdb_end();
	
	/*
	Next, open the spool directory and for each subdirectory, 
	check for hash table entry. If not, delete recursively.
	*/
	if (! (dirp = opendir(spoolName)))
	{
		syslog(LOG_ERR, "%s: %m", spoolName);
		exit(1);
	}

	while (d = readdir(dirp))
	{
		if (d->d_name[0] == '.')
			continue;

		strncpy(printName, d->d_name, d->d_namlen);
		printName[d->d_namlen] = '\0';
		strncpy(fullName, fullPath(spoolName, printName), sizeof(fullName));
		fullName[sizeof(fullName) - 1] = '\0';
		if (lstat(fullName, &statbuf))
			continue;

		if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
		{
			if (! NXHashMember(table, printName)) {
				directoryUnlink(fullName, NULL, NULL, 1, 1, 1);
			}
		} else
		if ((statbuf.st_mode & S_IFMT) == S_IFLNK)
		{
			if (! NXHashMember(table, printName))
				unlink(fullName);
		}
	}

	closedir(dirp);
	NXFreeHashTable(table);
}

/*
 * Create spool directory
 */
make_dir( char *dir )
{
	struct stat statbuf;
	struct passwd *getpwnam(), *pwd;
	int omask;
  
	if( ! stat( dir, &statbuf ) )
		return;

	omask = umask( 0 );
	if( mkdir( dir, 0770 ) < 0 )
	{
		directoryError: syslog(LOG_ERR, "%s: %m", dir);
		exit(1);
	}

	umask( omask );
	if( pwd = getpwnam( "daemon" ) )
		chown( dir, pwd->pw_uid, pwd->pw_uid );
}

#endif

reapchild()
{
	union wait status;

	while (wait3(&status, WNOHANG, 0) > 0)
		;
}

mcleanup()
{
	if (lflag)
		syslog(LOG_ERR, "exiting");
	unlink(SOCKETNAME);
	exit(0);
}

/*
 * Stuff for handling job specifications
 */
char	*user[MAXUSERS];	/* users to process */
int	users;			/* # of users in user array */
int	requ[MAXREQUESTS];	/* job number of spool entries */
int	requests;		/* # of spool requests */
char	*person;		/* name of person doing lprm */

char	fromb[32];	/* buffer for client's machine name */
char	cbuf[BUFSIZ];	/* command line buffer */
char	*cmdnames[] = {
	"null",
	"printjob",
	"recvjob",
	"displayq short",
	"displayq long",
	"rmjob",
	"infoq",
	"qstatus",
	"display alert"
};

doit()
{
	register char *cp;
	register int n;

#ifdef	DEBUG
	kill(getpid(), SIGSTOP);
#endif	DEBUG

	for (;;) {
		cp = cbuf;
		do {
			if (cp >= &cbuf[sizeof(cbuf) - 1])
				fatal("Command line too long");
			if ((n = read(1, cp, 1)) != 1) {
				if (n < 0)
					fatal("Lost connection");
				return;
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = cbuf;
		if (lflag) {
			if (*cp >= '\1' && *cp <= 9)
				syslog(LOG_ERR, "%s requests %s %s",
					from, cmdnames[*cp], cp+1);
			else
				syslog(LOG_ERR, "bad request (%d) from %s",
					*cp, from);
		}
		switch (*cp++) {
		case '\1':	/* check the queue and print any jobs there */
			printer = cp;
			printjob();
			break;
		case '\2':	/* receive files to be queued */
			printer = cp;
			recvjob();
			break;
		case '\3':	/* display the queue (short form) */
		case '\4':	/* display the queue (long form) */
			printer = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			displayq(cbuf[0] - '\3');
			exit(0);
		case '\5':	/* remove a job from the queue */
			printer = cp;
			while (*cp && *cp != ' ')
				cp++;
			if (!*cp)
				break;
			*cp++ = '\0';
			person = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			rmjob();
			break;

		}
		fatal("Illegal service request %o", *(--cp) );
		syslog (LOG_ERR, "LPD Switch end: %m");
	}
}

/*
 * Make a pass through the printcap database and start printing any
 * files left from the last time the machine went down.
 */
startup()
{
	char buf[BUFSIZ], name[MAXPATHLEN + 1];
	register char *cp;
	int pid;
	struct queue **queue;
	int num;

	printer = buf;

	/*
	 * Restart the daemons.
	 */
	while (getprent(buf) > 0) {
		for (cp = buf; *cp; cp++)
			if (*cp == '|' || *cp == ':') {
				*cp = '\0';
				break;
			}
		bp = pbuf;
		if ((SD = pgetstr("sd", &bp)) == NULL)
			SD = DEFSPOOL;

#ifdef	NeXT_MOD
		make_dir(SD);
#endif
		if (chdir(SD) < 0) {
			syslog(LOG_ERR, "startup, chdir - %s: %m", SD);
			continue;
		}
		num = getq(0);
		if( !num )
		  continue;
		if ((pid = fork()) < 0) {
			syslog(LOG_WARNING, "startup: cannot fork");
			mcleanup();
		}
		if (!pid) {
#ifdef NeXT_MOD
			/* The signal calls are copied from just before the doit call
			   in the main loop.  Before these were added, printjob hung on
			   a wait() because SIGCHLD was hooked to the reapchild routine
			   by the init code.  */
			signal(SIGCHLD, SIG_IGN);
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			signal(SIGTERM, SIG_IGN);
#endif
			endprent();
			printjob();
		}
	}
}

#define DUMMY ":nobody::"

/*
 * Check to see if the from host has access to the line printer.
 */
chkhost(f)
	struct sockaddr_in *f;
{
	register struct hostent *hp;
	register FILE *hostf;
	register char *cp, *sp;
	char ahost[50];
	int first = 1;
	extern char *inet_ntoa();
	int baselen = -1;

	f->sin_port = ntohs(f->sin_port);
	if (f->sin_family != AF_INET || f->sin_port >= IPPORT_RESERVED)
		fatal("Malformed from address");
	hp = gethostbyaddr(&f->sin_addr, sizeof(struct in_addr), f->sin_family);
	if (hp == 0)
		fatal("Host name for your address (%s) unknown",
			inet_ntoa(f->sin_addr));

	strcpy(fromb, hp->h_name);
	from = fromb;
	if (!strcmp(from, host))
		return;

	sp = fromb;
	cp = ahost;
	while (*sp) {
		if (*sp == '.') {
			if (baselen == -1)
				baselen = sp - fromb;
			*cp++ = *sp++;
		} else {
			*cp++ = isupper(*sp) ? tolower(*sp++) : *sp++;
		}
	}
	*cp = '\0';
	hostf = fopen("/etc/hosts.equiv", "r");
again:
	if (hostf) {
		if (!_validuser(hostf, ahost, DUMMY, DUMMY, baselen)) {
			(void) fclose(hostf);
			return;
		}
		(void) fclose(hostf);
	}
	if (first == 1) {
		first = 0;
		hostf = fopen("/etc/hosts.lpd", "r");
		goto again;
	}
	fatal("Your host does not have line printer access");
}

alert_inform( char *message )
 {
    int childID;
    union wait status;
    
    if (!(childID = fork()))
    {   /* child */
        execl( INFORM, INFORM, message, 0 );
    }
    /* else we're in parent */
    else{
	while(wait(&status) != childID) {} /* sic (wait for child to exit) */
#ifdef NeXT_DEBUG	
	syslog( LOG_ERR, "Inform child exited with %d\n", status.w_retcode );
#endif
	if( status.w_retcode ){
            execl( INFORM, INFORM, "-PSName", server_key, message, 0 );
	}
    }

}
