/*
 * PrintSpooler.m	- Implementation of PrintSpooler class.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

/*
 * Include files.
 */

#import "NpdDaemon.h"
#import "PrintJob.h"
#import "PrintSpooler.h"
#import "../lpr/lp.local.h"
#import "log.h"
#import "netinfo.h"
#import "npd_prot.h"
#import "mach_ipc.h"
#import "atomopen.h"

#import <objc/HashTable.h>


#import <ctype.h>
#import <cthreads.h>
#import <errno.h>
#import <libc.h>
#import <stddef.h>
#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import <printerdb.h>
#import <pwd.h>
#import <sys/types.h>
#import <sys/dir.h>
#import <sys/file.h>
#import <sys/stat.h>
#import <sys/un.h>

/*
 * Type definitions.
 */
typedef struct {
    uid_t	uid;
    uid_t	gid;
    char	*printer;
    char	*spoolfile;
    char	*jobname;
    int		copies;
} *lprargs_t;

typedef struct {
    port_t		port;
    queue_info_t	queueInfo;
} *infoargs_t;

/*
 * Static variables.
 */
static BOOL	_initdone = NO;
static id	_printerTable;
static int	daemonUid;
static int	daemonGid;

#ifdef	DEBUG
static int	_debugStopChild = 0;
#endif	DEBUG

/*
 * Constants.
 */
static char	*LPR = "/usr/ucb/lpr";
static char	*DAEMON = "daemon";
static int	DIRMODE = 0770;
static int	FREEDELAYTIME = 5 * 60;	/* 5 minutes */

static struct string_entry _strings[] = {
    { "PrintSpooler_execfailed", "could not exec %s: %m" },
    { "PrintSpooler_lprexit", "%s of %s exited status = %d" },
    { "PrintSpooler_lprterm", "%s of %s terminated signal = %d" },
    { "PrintSpooler_setuidfailed", "setuid %d failed: %m" },
    { "PrintSpooler_setgidfailed", "setgid %d failed: %m" },
    { "PrintSpooler_noopen", "PrintSpooler couldn't open %s: %m" },
    { "PrintSpooler_noflock", "PrintSpooler couldn't lock %s: %m" },
    { "PrintSpooler_notrunc", "PrintSpooler couldn't truncate %s: %m" },
    { "PrintSpooler_nowrite", "PrintSpooler write to %s failed: %m" },
    { "PrintSpooler_nocreate", "PrintSpooler couldn't create %s: %m" },
    { "PrintSpooler_nochown", "PrintSpooler couldn't chown %s: %m" },
    { "PrintSpooler_noclose", "PrintSpooler couldn't close %s: %m" },
    { "PrintSpooler_nolink", "PrintSpooler couldn't link %s to %s: %m" },
    { "PrintSpooler_nounlink", "PrintSpooler couldn't unlink %s: %m" },
    { "PrintSpooler_badseq", "PrintSpooler couldn't read sequence file %s" },
    { "PrintSpooler_nosocket", "PrintSpooler couldn't create socket: %m" },
    { "PrintSpooler_noconnect", "PrintSpooler couldn't connect to %s: %m" },
    { "PrintSpooler_nomkdir", "PrintSpooler couldn't create directory %s: %m"},
    { "PrintSpooler_statfailed", "PrintSpooler couldn't stat %s: %s" },
    { "PrintSpooler_qinfsendfail", "PrintSpooler couldn't send queue info" },
    { "PrintSpooler_acctopen",
	  "PrintSpooler couldn't open accounting file %s: %m" },
    { NULL, NULL },
};


/*
 * Local routines.
 */

/**********************************************************************
 * Routine:	_pstrDecode()	- Decode string parameter escapes.
 *
 * Function:	This function does the grunge work of decoding the
 *		string capability escapes.
 *
 * Args:	string:		The string to decode
 *
 * Returns:	Copy of decoded string.
 **********************************************************************/
static char *
_pstrDecode(char *string)
{
    char	*buf;			/* The new string buffer */
    char	*cp;			/* generic character pointer */
    char	c;

    /*
     * Allocate the new string.  Note that the string can only shrink
     * from being decoded.  We will just allocate the length of the
     * original string to keep the work simple.
     */
    buf = (char *) malloc(strlen(string) + 1);
    cp = buf;

    /* Scan through the string copying it */
    while (c = *string++) {

	switch (c) {

	  case '^':
	    /* Control character */
	    c = *string++ & 037;
	    break;

	  case '\\':
	    /* Escape sequence */
	    switch (c = *string++) {
	      case 'E':
		c = '\033';
		break;

	      case 'n':
		c = '\n';
		break;

	      case 'r':
		c = '\r';
		break;

	      case 't':
		c = '\t';
		break;

	      case 'b':
		c = '\b';
		break;

	      case 'f':
		c = '\f';
		break;

	      default:
		if (isdigit(c)) {
		    int i = 2;

		    c -= '0';
		    while (isdigit(*string) && i--) {
			c <<= 3;
			c |= (*string++ - '0');
		    }
		}
		break;
	    }
	}
	*cp++ = c;
    }
    *cp++ = 0;
    return (buf);
}
		

/**********************************************************************
 * Routine:	_pgetFlag()	- Get a flag parameter
 *
 * Function:	This looks to see if a flag option is set.
 *
 * Args:		pe:	Printer database entry
 *		key:	The option name;
 *
 * Returns:	YES:	Option exists.
 *		NO:	Option does not exist.
 **********************************************************************/
static BOOL
_pgetFlag(const prdb_ent *pe, const char *key)
{
    prdb_property	*pp;
    int			i;		/* generic counter */

    for (i = 0, pp = pe->pe_prop; i < pe->pe_nprops; i++, pp++) {
	if (strcmp(pp->pp_key, key) == 0) {
	    return (YES);
	}
    }
    return (NO);
}
    

/**********************************************************************
 * Routine:	_pgetStr()	- Get a string printer parameter.
 *
 * Function:	This copies a string parameter out of the printer
 *		database.
 *
 * Args:	pe:	The printers paramter list.
 *		key:	The key for the desired parameter.
 *
 * Returns:	NULL:	Parameter does not exist.
 *		Pointer to copy of value: success.
 **********************************************************************/
static char *
_pgetStr(const prdb_ent *pe, const char *key)
{
    int			i;		/* generic counter */
    prdb_property	*pp;		/* property pointer */

    for (i = 0, pp = pe->pe_prop; i < pe->pe_nprops; i++, pp++) {
	if (strcmp(pp->pp_key, key) == 0) {
	    return (_pstrDecode(pp->pp_value));
	}
    }
    return (NULL);
}

/**********************************************************************
 * Routine:	_createSpoolDir()
 *
 * Function:	Create the spool directory if it doesn't exist.
 **********************************************************************/
static void
_createSpoolDir(char *spoolDir)
{
    struct stat	stb;
    int		omask;

    /* First see if we can stat the directory */
    if (!stat(spoolDir, &stb)) {
	return;
    }

    /* Not there, let's try to create it */
    omask = umask(0);
    if (mkdir(spoolDir, DIRMODE) < 0) {
	LogError([NXStringManager stringFor:"PrintSpooler_nomkdir"],
		 spoolDir);
	(void) umask(omask);
	NX_RAISE(NPDspooling, NULL, NULL);
    }
    (void) umask(omask);

    /* Set it to be owned by daemon */
    if (chown(spoolDir, daemonUid, daemonGid)) {
	LogError([NXStringManager stringFor:"PrintSpooler_nochown"],
		 spoolDir);
	NX_RAISE(NPDspooling, NULL, NULL);
    }
    return;
}


/**********************************************************************
 * Routine:	_makeTempFiles()
 *
 * Function:	Make file names for "tf" and "cf" files in the spool
 *		directory.
 **********************************************************************/
static void
_makeTempFiles(char *spoolDir, char **tfname, char**cfname)
{
    int		fd;
    char	buf[1024];
    FILE	*FP;
    int		n;
    int		len;

    /* Open the sequence file */
    sprintf(buf, "%s/.seq", spoolDir);
    if ((fd = atom_open(buf, O_RDWR|O_CREAT, 0661)) < 0) {
	LogError([NXStringManager stringFor:"PrintSpooler_noopen"],
		buf);
	NX_RAISE(NPDspooling, NULL, NULL);
    }

    /* And lock it */
    if (flock(fd, LOCK_EX)) {
	LogError([NXStringManager stringFor:"PrintSpooler_noflock"],
		 buf);
	(void)close(fd);
	NX_RAISE(NPDspooling, NULL, NULL);
    }

    /* Read the current sequence number */
    FP = fdopen(fd, "r+");
    if (fscanf(FP, "%d", &n) != 1) {
	LogError([NXStringManager stringFor:"PrintSpooler_badseq"],
		 buf);
	goto errout;
    }
    
    len = strlen(spoolDir) + strlen(NXHostName) + 8;
    *tfname = (char *)malloc(len);
    sprintf(*tfname, "%s/tfA%03d%s", spoolDir, n, NXHostName);
    *cfname = (char *)malloc(len);
    sprintf(*cfname, "%s/cfA%03d%s", spoolDir, n, NXHostName);

    /* Bump the sequence number, and write it out */
    n = n + 1 % 1000;
    rewind(FP);
    fprintf(FP, "%03d\n", n);
    fclose(FP);				/* unlocks as well */
    return;

errout:
    fclose(FP);
    NX_RAISE(NPDspooling, NULL, NULL);
}
    

/**********************************************************************
 * Routine:	_wakeupLpd()
 *
 * Function:	This routine wakes up lpd to get it to look at
 *		the queue again.
 *
 * Caveat:	This code shamelessly stolen from the lpr stuff.
 **********************************************************************/
static void
_wakeupLpd(char *printer)
{
    struct sockaddr_un	skun;
    int 		sock;		/* the socket */
    int			n;		/* generic counter */
    char		buf[BUFSIZ];

    /* Create a socket */
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
	LogError([NXStringManager stringFor:"PrintSpooler_nosocket"]);
	return;
    }

    /* Build the address */
    skun.sun_family = AF_UNIX;
    strcpy(skun.sun_path, SOCKETNAME);
    if (connect(sock, (struct sockaddr *)&skun,
		strlen(skun.sun_path) + 2) < 0) {
	LogError([NXStringManager stringFor:"PrintSpooler_noconnect"],
		 SOCKETNAME);
	(void) close(sock);
	return;
    }

    (void) sprintf(buf, "\1%s\n", printer);
    n = strlen(buf);

    /* Write out the wakeup command */
    if (write(sock, buf, n) != n) {
	LogError([NXStringManager stringFor:"PrintSpooler_nowrite"],
		 SOCKETNAME);
    }

    /* Wait for some kind of response if it's coming */
    (void) read(sock, buf, 1);

    (void) close(sock);
    return;
}


/**********************************************************************
 * Routine:	_spoolChild() - Exec lpr program
 *
 * Function:	This routine is called in the child process to invoke
 *		lpr to spool a file.
 *
 * Args:	lprargs:	the structure with the relevant args.
 *
 * Returns:	1:		on exec failure.
 *		otherwise: 	doesn't return.
 **********************************************************************/
static int
_spoolChild(lprargs_t lprargs)
{
    char	buf[16];		/* Buffer to build integer argument */
    char	*argv[10];		/* Argv for new processes */
    char	**cpp = argv;
    extern int	setuid(uid_t uid);
    extern int	setgid(gid_t gid);

#if	DEBUG
    /* Stop ourselves so that we can attach a debugger */
    if (_debugStopChild) {
	kill(getpid(), SIGSTOP);
    }
#endif	DEBUG

    /* Set uid/gid to be that of the user */
    if (setgid(lprargs->gid)) {
	LogError([NXStringManager stringFor:"PrintSpooler_setgidfailed"],
		 lprargs->gid);
    }
    if (setuid(lprargs->uid)) {
	LogError([NXStringManager stringFor:"PrintSpooler_setuidfailed"],
		 lprargs->uid);
    }

    /* Clear argv */
    bzero(argv, sizeof(argv));

    /* Build the arguments */
    *cpp++ = LPR;			/* argv[0] */

    if (lprargs->copies > 1) {
	sprintf(buf, "-#%d", lprargs->copies);
	*cpp++ = buf;
    }

    *cpp++ = "-s";			/* use symbolic links */
    *cpp++ = "-r";			/* remove file when done */
    *cpp++ = "-J";			/* job name */
    *cpp++ = lprargs->jobname;
    *cpp++ = "-P";			/* printer name */
    *cpp++ = lprargs->printer;
    *cpp++ = lprargs->spoolfile;	/* file to spool */

    /* Exec the program */
    execv(LPR, argv);

    /* If we got here, then the exec failed. */
    LogError([NXStringManager stringFor:"PrintSpooler_execfailed"],
	     LPR);

    /* Free the lprargs */
    /* Free up allocated memory */
    free(lprargs->printer);
    free(lprargs->spoolfile);
    free(lprargs->jobname);
    free(lprargs);

    /* We return 0 here so that the child handler won't report an error */
    return (0);
}

/**********************************************************************
 * Routine:	_spoolChildHandler() - Handle death of spool child.
 *
 * Function:	This checks to see if lpr exited OK.  If not, it reports
 *		an error.
 */
static void
_spoolChildHandler(int pid, union wait *status, lprargs_t lprargs)
{
    if (status->w_retcode) {
	LogError([NXStringManager stringFor:"PrintSpooler_lprexit"],
		 LPR, lprargs->jobname, status->w_retcode);
    }

    if (status->w_termsig) {
	LogError([NXStringManager stringFor:"PrintSpooler_lprterm"],
		 LPR, lprargs->jobname, status->w_termsig);
    }

    /* Free up allocated memory */
    free(lprargs->printer);
    free(lprargs->spoolfile);
    free(lprargs->jobname);
    free(lprargs);
}

/**********************************************************************
 * Routine:	_queueEntryCompare()
 *
 * Function:	Compare the values of two queue entries and return
 *		an integer to be used by qsort().
 *
 * Returns:	-1:	entry1 is newer than entry2
 *		1:	entry2 is older than entry1
 **********************************************************************/
static int
_queueEntryCompare(queue_entry_t *entry1, queue_entry_t *entry2)
{
    if ((*entry1)->time > (*entry2)->time) {
	return (1);
    }
    return (-1);
}

/**********************************************************************
 * Routine:	_queueEntryLoadCFInfo()
 *
 * Function:	Read the cf file for any data that we might need
 *		to report the status of the queue.  Currently, that
 *		would be the name of the user that submitted the job
 *		as well as the names of the files being printed.
 **********************************************************************/
static void
_queueEntryLoadCFInfo(queue_entry_t qEntry, const char *fullFileName)
{
    FILE	*cfFile;
    char	buf[1024];
    char	*newFiles;

    if ((cfFile = atom_fopen(fullFileName, "r")) == NULL) {
	LogError([NXStringManager stringFor:"PrintSpooler_noopen"],
		 fullFileName);
	return;
    }

    /* Scan the file */
    while (fgets(buf, sizeof(buf), cfFile)) {
	/* Get rid of the eol */

	buf[strlen(buf) - 1] = '\000';
	switch (buf[0]) {
	  case 'P':			/* Get the owner of the job */
	    if (!qEntry->user) {
		qEntry->user = (char *) malloc(strlen(buf + 1) + 1);
		strcpy(qEntry->user, buf + 1);
	    }
	    break;

	  case 'N':			/* A name of a file */
	    if (qEntry->files) {
		newFiles = (char *) malloc(strlen(qEntry->files) +
					  strlen(buf + 1) +
					  3 /* space "," and NULL */);
		strcpy(newFiles, qEntry->files);
		strcat(newFiles, ", ");
		strcat(newFiles, buf + 1);
		free(qEntry->files);
		qEntry->files = newFiles;
	    } else {
		qEntry->files = (char *) malloc(strlen(buf + 1) + 1);
		strcpy(qEntry->files, buf + 1);
	    }
	    break;

	  default:
	    break;
	}
    }

    fclose(cfFile);
}

/**********************************************************************
 * Routine:	_queueInfoReport()
 *
 * Function:	Send a sequence of messages to the port describing
 *		each entry in the queueInfo array.  If the msg_send fails
 *		in any way, the whole process is aborted.  Also, a
 *		timeout is used because the main thread may block waiting
 *		for all the readers of the queueInfo to go away.
 **********************************************************************/
static void
_queueInfoReport(infoargs_t infoargs)
{
    int			i;			/* generic counter */
    queue_entry_t	qEntry;
    queue_info_t	queueInfo = infoargs->queueInfo;
    npd1_info_msg	msg;
    DocInfo		*docinfo;
    msg_return_t	ret_code;

    /* Initialize the message */
    bzero(&msg, sizeof(msg));
    MSGHEAD_SIMPLE_INIT(msg.head, sizeof(msg));
    msg.head.msg_local_port = PORT_NULL;
    msg.head.msg_remote_port = infoargs->port;
    msg.head.msg_id = NPD_HAVE_INFO;

    MSGTYPE_CHAR_INIT(msg.printer_type, sizeof(msg.printer));
    MSGTYPE_CHAR_INIT(msg.host_type, sizeof(msg.host));
    MSGTYPE_CHAR_INIT(msg.user_type, sizeof(msg.user));
    MSGTYPE_CHAR_INIT(msg.doc_type, sizeof(msg.doc));
    MSGTYPE_CHAR_INIT(msg.creator_type, sizeof(msg.creator));
    MSGTYPE_INT_INIT(msg.size_type);
    MSGTYPE_INT_INIT(msg.pages_type);
    MSGTYPE_INT_INIT(msg.feed_type);
    MSGTYPE_INT_INIT(msg.job_type);
    MSGTYPE_INT_INIT(msg.time_type);

    for (i = 0; i < queueInfo->nentries; i++) {
	qEntry = queueInfo->entries[i];

	/* If there is a docinfo structure, lock it */
	if (docinfo = qEntry->docinfo) {
	    [docinfo lock];
	}
	/* Copy the entry's host */
	strncpy(msg.host, qEntry->host, sizeof(msg.host) - 1);
	msg.host[sizeof(msg.host) - 1] = '\000';

	/* Copy the entry's user */
	strncpy(msg.user, qEntry->user, sizeof(msg.user) - 1);
	msg.user[sizeof(msg.user) - 1] = '\000';

	/* Copy the entry's title */
	if (docinfo && docinfo->title) {
	    strncpy(msg.doc, docinfo->title, sizeof(msg.doc) - 1);
	} else if (qEntry->files) {
	    strncpy(msg.doc, qEntry->files, sizeof(msg.doc) - 1);
	} else {
	    msg.doc[0] = '\000';
	}
	msg.doc[sizeof(msg.doc) - 1] = '\000';

	/* Copy the entry's creator */
	if (docinfo && docinfo->creator) {
	    strncpy(msg.creator, docinfo->creator,
		    sizeof(msg.creator) - 1);
	    msg.creator[sizeof(msg.creator) - 1] = '\000';
	} else {
	    msg.creator[0] = '\0';
	}

	/* Copy the entry's integer variables */
	msg.size = qEntry->size;
	if (docinfo && !docinfo->pagesAtEnd) {
	    msg.pages = docinfo->pages;
	} else {
	    msg.pages = 0;
	}
	if (docinfo) {
	    msg.feed = (int) docinfo->manualfeed;
	} else {
	    msg.feed = 0;
	}
	msg.job = qEntry->job;
	msg.time = qEntry->time;

	/* Unlock the docinfo */
	if (docinfo) {
	    [docinfo unlock];
	}

	/* Send the message with a 4 second timeout */
	if ((ret_code = msg_send(&msg, SEND_TIMEOUT, 4000)) != SEND_SUCCESS) {
	    LogWarning([NXStringManager
		      stringFor:"PrintSpooler_qinfosendfail"],
		       ret_code);
	    goto exit;
	}
    }

    /* Send the final message */
    msg.head.msg_id = NPD_NO_MORE_INFO;
    msg.host[0] = '\000';
    msg.user[0] = '\000';
    msg.doc[0] = '\000';
    msg.creator[0] = '\000';
    msg.size = 0;
    msg.pages = 0;
    msg.feed = 0;
    msg.job = 0;
    msg.time = 0;

    (void) msg_send(&msg, SEND_TIMEOUT, 4000);
    PortFree(infoargs->port);

exit:
    /* Wake up anything waiting for us to be done */
    mutex_lock(queueInfo->lock);
    queueInfo->readcount--;
    mutex_unlock(queueInfo->lock);
    condition_signal(queueInfo->readerdone);
    free(infoargs);
}

/**********************************************************************
 * Routine:	_freeSpooler()
 *
 * Function:	This routine is called as a result of a delayed free
 *		when the delay period has finally expired.  It will set
 *		delayedFree to NO and the invoke the free method.
 **********************************************************************/
static void
_freeSpooler(DPSTimedEntry teNumber, double now, id theSpooler)
{

    /* Bump the reference count to 1 so that free works */
    [theSpooler hold];
    [theSpooler setDelayedFree:NO];
    DPSRemoveTimedEntry([theSpooler freeTE]);
    [theSpooler free];
}



@implementation	PrintSpooler : Object

/* Factory methods */
+ newPrinter:(char *)printerName
{
    id			newSelf;	/* tmp id */
    const prdb_ent	*pe;		/* printer database entry */
    int			i;		/* generic counter */
    char		**cpp;		/* generic char pointer pointer */
    char		*cp;		/* generic char pointer */
    struct passwd	*pw;

    /* If we don't have one already, create a printer table */
    if (!_initdone) {
	_printerTable = [HashTable newKeyDesc:"*"];
	[NXStringManager loadFromArray:_strings];
	if (pw = getpwnam(DAEMON)) {
	    daemonUid = pw->pw_uid;
	    daemonGid = pw->pw_gid;
	} else {
	    daemonUid = DEFUID;
	    daemonGid = DEFUID;		/* XXX */
	}
	_initdone = YES;
    }

    /* If spooler already exists, bump its reference and return it */
    if (newSelf = [_printerTable valueForKey:printerName]) {
	self = newSelf;
	if (refcnt++ == 0) {
	    DPSRemoveTimedEntry(freeTE);
	}
	return ([self refresh]);
    }
    
     /* Get the printer information from the printer database (NetInfo) */
    pe = prdb_getbyname(printerName);
    if (pe == NULL) {
	return (nil);
    }

    /* Instantiate ourself */
    self = [super new];
    refcnt = 1;

    /* Copy all of the names */
    for (cpp = pe->pe_name, i = 1; *cpp; cpp++, i++) ;
    names = (char **) malloc(i * sizeof (char *));
    for (cpp = pe->pe_name, i = 0; *cpp; cpp++, i++) {
	names[i] = (char *) malloc(strlen(*cpp) + 1);
	strcpy(names[i], *cpp);
	[_printerTable insertKey:names[i] value:self];
    }
    names[i] = NULL;

    /* Load the parameters we are interested in */
    spoolDir = _pgetStr(pe, "sd");
    lockFile = _pgetStr(pe, "lo");
    accountFile = _pgetStr(pe, "af");
    devicePath = _pgetStr(pe, "lp");
    remoteHost = _pgetStr(pe, "rm");
    remotePrinter = _pgetStr(pe, "rp");
    inputFilter = _pgetStr(pe, "if");
    bannerBefore = _pgetStr(pe, "BannerBefore");
    bannerAfter = _pgetStr(pe, "BannerAfter");
    unavailable = _pgetFlag(pe, "_ignore");
    remoteAsNobody = _pgetFlag(pe, "RemoteAsNobody");

    /* Make sure we have a spoolDir */
    if (!spoolDir) {
	spoolDir = (char *)malloc(strlen(DEFSPOOL) + 1);
	strcpy(spoolDir, DEFSPOOL);
    }

    /* Make sure we have a lock file */
    if (!lockFile) {
	lockFile = (char *) malloc(strlen(spoolDir) + strlen(DEFLOCK) +
				   2 /* a slash and the null terminator */);
	sprintf(lockFile, "%s/%s", spoolDir, DEFLOCK);
    }

    /* Make sure the lock file is an absolute path */
    if (*lockFile != '/') {
	cp = (char *) malloc(strlen(spoolDir) + strlen(lockFile) +
			     2 /* a slash and the null terminator */);
	sprintf(cp, "%s/%s", spoolDir, lockFile);
	free(lockFile);
	lockFile = cp;
    }

    /* Make sure the accounting file is an absolute path */
    if (accountFile && *accountFile != '/') {
	cp = (char *) malloc(strlen(spoolDir) + strlen(accountFile) +
			     2 /* a slash and the null terminator */);
	sprintf(cp, "%s/%s", spoolDir, accountFile);
	free(accountFile);
	accountFile = cp;
    }

    /* Initialize the rest of the instance variables */
    cfname = NULL;
    entryCache = nil;
    currentDocInfo = nil;
    cacheTime = 0;
    queueInfo = NULL;
    delayedFree = YES;

    return (self);
}


/* Instance methods */

- setDelayedFree:(BOOL)dFree
{
    delayedFree = dFree;
    return (self);
}

- (DPSTimedEntry) freeTE
{
    return (freeTE);
}

- refresh
{
    const prdb_ent	*pe;		/* printer database entry */
    char		**cpp;		/* generic string pointer */
    char		*cp;		/* generic char pointer */
    int			i;		/* generic counter */

    pe = prdb_getbyname(*names);
    if (pe == NULL) {
	delayedFree = NO;
	[self free];
	return (nil);
    }

    /* FIXME:  This stuff should really be shared with the + new... method. */

    /* Free the old names */
    for (cpp = names; *cpp; cpp++) {
	[_printerTable removeKey:*cpp];
	free(*cpp);
    }
    free(names);

    /* Copy the new names */
    for (cpp = pe->pe_name, i = 1; *cpp; cpp++, i++) ;
    names = (char **) malloc(i * sizeof (char *));
    for (cpp = pe->pe_name, i = 0; *cpp; cpp++, i++) {
	names[i] = (char *) malloc(strlen(*cpp) + 1);
	strcpy(names[i], *cpp);
	[_printerTable insertKey:names[i] value:self];
    }
    names[i] = NULL;

    /* Refresh the parameters we are interested in */
    if (spoolDir) {
	free(spoolDir);
    }
    spoolDir = _pgetStr(pe, "sd");
    
    if (lockFile) {
	free(lockFile);
    }
    lockFile = _pgetStr(pe, "lo");

    if (accountFile) {
	free(accountFile);
    }
    accountFile = _pgetStr(pe, "af");

    if (devicePath) {
	free(devicePath);
    }
    devicePath = _pgetStr(pe, "lp");

    if (remoteHost) {
	free(remoteHost);
    }
    remoteHost = _pgetStr(pe, "rm");

    if (remotePrinter) {
	free(remotePrinter);
    }
    remotePrinter = _pgetStr(pe, "rp");

    if (inputFilter) {
	free(inputFilter);
    }
    inputFilter = _pgetStr(pe, "if");

    if (bannerBefore) {
	free(bannerBefore);
    }
    bannerBefore = _pgetStr(pe, "BannerBefore");

    if (bannerAfter) {
	free(bannerAfter);
    }
    bannerAfter = _pgetStr(pe, "BannerAfter");

    unavailable = _pgetFlag(pe, "_ignore");
    remoteAsNobody = _pgetFlag(pe, "RemoteAsNobody");

    /* Make sure we have a spoolDir */
    if (!spoolDir) {
	spoolDir = (char *)malloc(strlen(DEFSPOOL) + 1);
	strcpy(spoolDir, DEFSPOOL);
    }

    /* Make sure we have a lock file */
    if (!lockFile) {
	lockFile = (char *) malloc(strlen(spoolDir) + strlen(DEFLOCK) +
				   2 /* a slash and the null terminator */);
	sprintf(lockFile, "%s/%s", spoolDir, DEFLOCK);
    }

    /* Make sure the lock file is an absolute path */
    if (*lockFile != '/') {
	cp = (char *) malloc(strlen(spoolDir) + strlen(lockFile) +
			     2 /* a slash and the null terminator */);
	sprintf(cp, "%s/%s", spoolDir, lockFile);
	free(lockFile);
	lockFile = cp;
    }

    /* Make sure the accounting file is an absolute path */
    if (accountFile && *accountFile != '/') {
	cp = (char *) malloc(strlen(spoolDir) + strlen(accountFile) +
			     2 /* a slash and the null terminator */);
	sprintf(cp, "%s/%s", spoolDir, accountFile);
	free(accountFile);
	accountFile = cp;
    }

    return (self);
}

- (BOOL)isNamed:(const char *)theName
{
    char	**cpp;

    for (cpp = names; *cpp; cpp++) {
	if (!strcmp(*cpp, theName)) {
	    return (YES);
	}
    }
    return (NO);
}

- (BOOL)isUnavailable
{
    return (unavailable);
}

- (BOOL)remoteAsNobody
{
    return (remoteAsNobody);
}

- (queue_entry_t) _queueEntryLookup:(const char *)cfFile
{
    queue_entry_t	qEntry = NULL;
    struct stat		stb;
    char		*fullFileName;
    char		*cp;		/* generic character pointer */

    /* Create a queue cache if there isn't one */
    if (entryCache == nil) {
	entryCache = [HashTable newKeyDesc:"*" valueDesc:"i"];
    } else {
	qEntry = [entryCache valueForKey:cfFile];
    }

    /* If we found one, bump its reference count and return */
    if (qEntry) {
	qEntry->refcnt++;
	return(qEntry);
    }

    /* Stat the file to get its modify time */
    fullFileName = (char *)malloc(strlen(cfFile) + strlen(spoolDir) + 2);
    sprintf(fullFileName, "%s/%s", spoolDir, cfFile);
    if (stat(fullFileName, &stb) == -1) {
	/* file may have disappeared, just return */
	free(fullFileName);
	return (NULL);
    }

    qEntry = (queue_entry_t) malloc(sizeof *qEntry);
    bzero(qEntry, sizeof (*qEntry));

    qEntry->refcnt = 1;
    qEntry->controlFile = (char *) malloc(strlen(cfFile) + 1);
    strcpy(qEntry->controlFile, cfFile);
    qEntry->host = (char *) malloc(strlen(cfFile + 6) + 1);
    strcpy(qEntry->host, cfFile + 6);
    qEntry->job = atoi(cfFile + 3);
    qEntry->time = stb.st_mtime;

    _queueEntryLoadCFInfo(qEntry, fullFileName);

    /*
     * If this is the current file being printed then we can use the
     * currentDocInfo, otherwise, we have to parse the conforming
     * comments out of the data file.  This is really ugly, I know,
     * but backwards compatability is the burden one has to bear.
     *
     * Christ!  At this point I would love to launch into an essay on
     * software engineering and the need for up-front DESIGN.  But
     * these are merely comments in the code--not an appropriate place
     * for a diatribe.
     */
    if (cfname && !strcmp(fullFileName, cfname)) {
	qEntry->docinfo = [currentDocInfo hold];
    } else {
	/* Cobble up a name for the data file */
	cp = strrchr(fullFileName, '/');
	*++cp = 'd';

	/* Stat it to get the size */
	if (stat(fullFileName, &stb) != -1) {
	    qEntry->size = stb.st_size;
	}
	
	NX_DURING {
	    qEntry->docinfo = [DocInfo newFromFile:fullFileName
			     size:stb.st_size];
	} NX_HANDLER {
	    if (NXLocalHandler.code != NPDdocformat) {
		NX_RERAISE();
	    }
	    qEntry->docinfo = nil;
	    /* Not PostScript, get as much info as possible from cf file. */
	} NX_ENDHANDLER;
    }
    free(fullFileName);

    /* Add ourself to the entry cache */
    [entryCache insertKey:qEntry->controlFile value:(void *)qEntry];

    return(qEntry);
}

- (void) _queueEntryRelease:(queue_entry_t) qEntry
{
    if (--qEntry->refcnt == 0) {
	[entryCache removeKey:qEntry->controlFile];
	free(qEntry->controlFile);
	free(qEntry->host);
	if (qEntry->user) {
	    free(qEntry->user);
	}
	if (qEntry->files) {
	    free(qEntry->files);
	}
	if (qEntry->docinfo) {
	    [qEntry->docinfo release];
	}
	free(qEntry);
    }
}

- (queue_info_t) _queueInfoRead
{
    queue_info_t	newQueue;
    DIR			*dirp;
    struct direct	*direct;
    int			arraysize = 2;	/* Size of the entries array */
    queue_entry_t	*newArray;
    queue_entry_t	newEntry;
    int			i;		/* generic counter */

    /* Create the new queue_info_struct and initialize it */
    newQueue = (queue_info_t)malloc(sizeof *newQueue);
    newQueue->lock = mutex_alloc();
    newQueue->readerdone = condition_alloc();
    newQueue->readcount = 0;
    newQueue->nentries = 0;
    newQueue->entries = (queue_entry_t *)
	malloc(sizeof(queue_entry_t) * arraysize);

    /* Scan the directory for control files */
    if ((dirp = opendir(spoolDir)) == NULL) {
	LogError([NXStringManager stringFor:"PrintSpooler_noopen"],
		 spoolDir);
	goto errout;
    }

    while (direct = readdir(dirp)) {

	/* If not a control file, skip */
	if (direct->d_name[0] != 'c' || direct->d_name[1] != 'f') {
	    continue;
	}

	if (!(newEntry = [self _queueEntryLookup:direct->d_name])) {
	    continue;
	}

	/* Increase the size of the entries array if we need to */
	if (++newQueue->nentries > arraysize) {
	    newArray = (queue_entry_t *)
		malloc(sizeof(queue_entry_t) * arraysize * 2);
	    for (i = 0; i < arraysize; i++) {
		newArray[i] = newQueue->entries[i];
	    }
	    free(newQueue->entries);
	    newQueue->entries = newArray;
	    arraysize *= 2;
	}

	/* Get a queue entry for the control file */
	newQueue->entries[newQueue->nentries - 1] = newEntry;
    }
    closedir(dirp);

    /* Now sort the queue if appropriate */
    if (newQueue->nentries) {
	qsort(newQueue->entries, newQueue->nentries,
	      sizeof(queue_entry_t),
	      (int (*)(const void *, const void *)) _queueEntryCompare);
    }

    return(newQueue);

errout:
    mutex_free(newQueue->lock);
    condition_free(newQueue->readerdone);
    free(newQueue->entries);
    free(newQueue);
    return (NULL);
}

- (void) _queueInfoFree:(queue_info_t) qInfo
{
    int		i;			/* generic counter */

    /* Release all of the queue entries */
    for (i = 0; i < qInfo->nentries; i++) {
	[self _queueEntryRelease:qInfo->entries[i]];
    }

    /* Free up other data structures */
    mutex_free(qInfo->lock);
    condition_free(qInfo->readerdone);
    free(qInfo->entries);
    free(qInfo);
}

- (BOOL)lockForPrinting
{
    struct stat	stb;			/* stat buffer */
    int		pid = getpid();
    char	buf[64];

    /* Make sure there is a spooling directory */
    _createSpoolDir(spoolDir);

    /* Open/Create a lock file */
    if ((lockFd = atom_open(lockFile, O_WRONLY|O_CREAT, 0644)) < 0) {
	LogError([NXStringManager stringFor:"PrintSpooler_noopen"],
		 lockFile);
	return (NO);
    }

    /*
     * See if printing is disabled, we use the lpd convention of setting
     * the owner execute bit on the file
     */
    if (fstat(lockFd, &stb) == 0 && (stb.st_mode & 0100)) {
	goto failed;
    }

    /* Get a lock on the lock file. */
    if (flock(lockFd, LOCK_EX|LOCK_NB) < 0) {
	if (cthread_errno() != EWOULDBLOCK) {	/* active daemon present */
	    LogError([NXStringManager stringFor:"PrintSpooler_noflock"],
		     lockFile);
	}
	goto failed;
    }

    /* Truncate the file */
    if (ftruncate(lockFd, 0)) {
	LogError([NXStringManager stringFor:"PrintSpooler_notrunc"],
		 lockFile);
	goto failed;
    }

    /* Write our pid for others to know us by */
    sprintf(buf,"%u\n", pid);
    lockOffset = strlen(buf);
    if (write(lockFd, buf, lockOffset) != lockOffset) {
	LogError([NXStringManager stringFor:"PrintSpooler_nowrite"],
		 lockFile);
	goto failed;
    }

    return (YES);

failed:
    (void) close(lockFd);
    lockFd = -1;
    return (NO);
}
    
- fakeQueueEntry:(id)job
{
    PrintJob	*Job = (PrintJob *)job;
    int		fd;
    FILE	*FP;
    char	*tfname;
    int		oldumask;
    DocInfo	*docinfo = Job->docinfo;
    char	*cp;
    char	*titlestr;
    char	ochar;
    char	buf[64];

    /*
     * This really sucks, but we want consistancy with lpd until there
     * is a real spooling system.
     */

    /* Cobble up the weird file names used to build a cf file */
    _makeTempFiles(spoolDir, &tfname, &cfname);

    oldumask = umask(0);
    fd = creat(tfname, FILMOD);
    (void) umask(oldumask);

    if (fd < 0) {
	LogError([NXStringManager stringFor:"PrintSpooler_nocreate"],
		  tfname);
	free(tfname);
	free(cfname);
	cfname = NULL;
	NX_RAISE(NPDspooling, NULL, NULL);
    }

    /* Change the owner to daemon for protection */
    if (fchown(fd, daemonUid, -1) < 0) {
	LogError([NXStringManager stringFor:"PrintSpooler_nochown"],
		 tfname);
	goto errout;
    }

    FP = fdopen(fd, "w");
    fprintf(FP, "P%s\n", Job->user);

    /* Make the first word in the title the file name */
    titlestr = docinfo->title;
    if (!titlestr) titlestr = "(No_Title)";		/* Cope with no title in job */
    if (cp = strpbrk(titlestr, " \t")) {
	ochar = *cp;
	*cp = '\0';
    }
    fprintf(FP, "f%s\n", titlestr);
    fprintf(FP, "N%s\n", titlestr);
    if (cp) {
	*cp = ochar;
    }

    if (fclose(FP) == EOF) {
	LogError([NXStringManager stringFor:"PrintSpooler_noclose"],
		 tfname);
	goto unlinkerrout;
    }

    /* Now link it to the real control file */
    if (link(tfname, cfname) < 0) {
	LogError([NXStringManager stringFor:"PrintSpooler_nolink"],
		 cfname, tfname);
	goto unlinkerrout;
    }

    if (unlink(tfname) < 0) {
	/* We'll report the error, but forget about it. */
	LogError([NXStringManager stringFor:"PrintSpooler_nounlink"],
		 tfname);
    }

    /* Now write the "cf" name in the lock file */
    cp = strrchr(cfname, '/');
    cp++;
    sprintf(buf, "%s\n", cp);
    lseek(lockFd, lockOffset, L_SET);
    write(lockFd, buf, strlen(buf));

    currentDocInfo = docinfo;

    free(tfname);

    return (self);

errout:
    (void)close(fd);
unlinkerrout:
    (void)unlink(tfname);
    free(cfname);
    cfname = NULL;
    free(tfname);
    NX_RAISE(NPDspooling, NULL, NULL);
}
	
- unlockAndLog:(id)job pages:(int)pages
{
    char	**cpp;

    [self logUsage:job pages:pages];

    currentDocInfo = nil;
    /* Free up the fake control file */
    if (cfname) {
	if (unlink(cfname)) {
	    LogError([NXStringManager stringFor:"PrintSpooler_nounlink"],
		     cfname);
	}
	free(cfname);
	cfname = NULL;
    }

    /* Close the lock file, this will let the spooler in */
    (void) close (lockFd);

    /* Find the most used name of the printer */
    for (cpp = names; *(cpp + 1); cpp++)
	;

    _wakeupLpd(*cpp);

    /* FIXME: Implement accounting */
    return (self);
}

- logUsage:(id)job pages:(int)pages
{
    PrintJob	*Job = (PrintJob *)job;
    FILE	*afp;

    /*
     * Make sure accounting file is specified and exists and that we can
     * write it.
     */
    if (!accountFile ||
	access(accountFile, F_OK) || access(accountFile, W_OK)) {
	return (self);
    }

    if ((afp = atom_fopen(accountFile, "a")) == NULL) {
	LogError([NXStringManager stringFor:"PrintSpooler_acctopen"],
		 accountFile);
	return (self);
    }

    fprintf(afp, "%7.2f\t%s:%s\n", (float)pages, Job->host, Job->user);

    (void) fclose(afp);

    return (self);
}

- (BOOL)queueEnabled
{
    struct stat		stb;		/* Stat buffer */

    /*
     * The convention used by lpd for disabling the queue is to set the
     * execute but of the lock file.  A hack, we know.
     */
    if (stat(lockFile, &stb)) {
	LogDebug("couldn't stat lock file: %m");
    } else if (stb.st_mode & 010) {
	return (NO);
    }
    return (YES);
}

- spool:(id)job fromFile:(char *)filePath user:(int)uid group:(int)gid
{
    DocInfo	*docinfo;
    lprargs_t	lprargs;

    docinfo = (DocInfo *)[job docinfo];
    lprargs = (lprargs_t) malloc(sizeof(*lprargs));
    lprargs->uid = uid;
    lprargs->gid = gid;
    lprargs->printer = (char *) malloc(strlen(names[0]) + 1);
    strcpy(lprargs->printer, names[0]);
    lprargs->spoolfile = (char *)malloc (strlen(filePath) + 1);
    strcpy(lprargs->spoolfile, filePath);
    if (docinfo->title) {
	lprargs->jobname = (char *) malloc(strlen(docinfo->title) + 1);
	strcpy(lprargs->jobname, docinfo->title);
    } else {
	lprargs->jobname = (char *) malloc(sizeof (char));
	lprargs->jobname[0] = '\0';
    }
    lprargs->copies = [job copies];

    [NXDaemon spawnChildProc:(NXChildProc)_spoolChild data:(void *)lprargs
		handler:(NXChildHandler)_spoolChildHandler
		handlerData:(void *)lprargs];

    return (self);
}

- sendNPD1InfoTo:(port_t)port
{
    struct stat		stb;		/* stat buffer */
    infoargs_t		infoArgs;	/* thread arguments */
    queue_info_t	newQueueInfo;	/* new queue info */

    /* Create the arg strucuture */
    infoArgs = (infoargs_t)malloc(sizeof(*infoArgs));
    infoArgs->port = port;

    /* Stat the spool directory to see if it has changed */
    if (stat(spoolDir, &stb) == -1) {
	LogError([NXStringManager stringFor:"PrintSpooler_statfailed"],
		 spoolDir, strerror(cthread_errno()));

	/* No directory, so no queue info */
	infoArgs->queueInfo = NULL;
    } else {

	/*
	 * If the spool directory has changed, we need to read in the new
	 * queue information.
	 */
	if (queueInfo) {
	    if (cacheTime != stb.st_mtime) {
		/* Wait for all current readers to finish */
		mutex_lock(queueInfo->lock);
		while (queueInfo->readcount) {
		    condition_wait(queueInfo->readerdone, queueInfo->lock);
		}
		mutex_unlock(queueInfo->lock);
		newQueueInfo = [self _queueInfoRead];
		[self _queueInfoFree:queueInfo];
		queueInfo = newQueueInfo;
		cacheTime = stb.st_mtime;
	    }
	} else {
	    queueInfo = [self _queueInfoRead];
	}

	/*
	 * Increment the reference count on the queueInfo.
	 */
	mutex_lock(queueInfo->lock);
	queueInfo->readcount++;
	mutex_unlock(queueInfo->lock);

	infoArgs->queueInfo = queueInfo;
    }

    /*
     * Spawn a thread to send this info back to to port.  _queueInfoReport
     * will decrement the queueInfo readcount when it is done.
     */
    cthread_detach(cthread_fork((cthread_fn_t)_queueInfoReport, infoArgs));
    
    return(self);
}

- hold
{
    refcnt++;

    return (self);
}

- free
{
    char		**cpp;		/* generic character pointer pointer */

    if (--refcnt == 0) {

	if (delayedFree) {
	    freeTE = DPSAddTimedEntry(FREEDELAYTIME,
				      (DPSTimedEntryProc) _freeSpooler,
				      self, 0);
	    return (nil);
	}

	/* Remove ourself from the printer table and free up names */
	for (cpp = names; *cpp; cpp++) {
	    [_printerTable removeKey:*cpp];
	    free((void *)*cpp);
	}

	/* Free previously allocated storages */
	free(names);
	if (spoolDir) {
	    free(spoolDir);
	}
	if (lockFile) {
	    free(lockFile);
	}
	if (accountFile) {
	    free(accountFile);
	}
	if (devicePath) {
	    free(devicePath);
	}
	if (remoteHost) {
	    free(remoteHost);
	}
	if (remotePrinter) {
	    free(remotePrinter);
	}
	if (inputFilter) {
	    free(inputFilter);
	}
	if (bannerBefore) {
	    free(bannerBefore);
	}
	if (bannerAfter) {
	    free(bannerAfter);
	}

	if (cfname) {
	    free(cfname);
	}

	if (queueInfo) {
	    /* Wait for all readers to be done */
	    mutex_lock(queueInfo->lock);
	    while (queueInfo->readcount) {
		condition_wait(queueInfo->readerdone, queueInfo->lock);
	    }
	    mutex_unlock(queueInfo->lock);
	    [self _queueInfoFree:queueInfo];
	}

	if (entryCache) {
	    [entryCache free];
	}

	return ([super free]);
    }
    return (nil);
}

@end

