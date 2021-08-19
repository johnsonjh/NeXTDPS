/*
 * log.m	- Logging routines.
 *
 * Copyright (c) 1990 by NeXT, Inc.,  All rights reserved.
 */

/*
 * Include files.
 */
#import "log.h"
#import "Daemon.h"

#import <cthreads.h>
#import <errno.h>
#import	<syslog.h>
#import <stdarg.h>
#import <stdio.h>
#import <stdlib.h>
#import <string.h>

#include <sys/wait.h>
#include <objc/vectors.h>


/*
 * Constants.
 */
const char	*COREDIR = "/tmp/";	/* Directory to dump core in */
const char	*CORESUFFIX = ".core";	/* Suffix for core file */

#ifdef	GCORE
const char	*GCOREPATH = "/usr/bin/gcore"; /* Path to the gcore program */
#endif	GCORE

#define MALLOC_ERROR_VM_ALLOC 0
#define MALLOC_ERROR_VM_DEALLOC 1
#define MALLOC_ERROR_VM_COPY 2
#define MALLOC_ERROR_FREE_FREE 3
#define MALLOC_ERROR_HEAP_CHECK 4
#define MALLOC_ERROR_FREE_NOT_IN_HEAP 5


/*
 * Static routines.
 */

/**********************************************************************
 * Routine:	_logCommon - Log a debug/error/warning message.
 *
 * Function:	Log an information message with syslog unless we are
 *		in debug mode, in which case, print the error message
 *		on stderr.
 *
 * Args:	The arguments are the same that you would use for
 * 		syslog which are similar to those you would use for
 * 		printf with the following addition: "%m" in the format
 * 		string expands to the current error message based on
 * 		cthread_errno().
 *
 * FIXME:	Since there is no version of syslog that takes
 *		varargs, we have to place an arbitrary limit of 5
 *		parameters when calling syslog.  Sick I know, but
 *		you work with what you get.
 **********************************************************************/
static void
_logCommon(int priority, char *fmt, va_list ap)
{
	int		p0, p1, p2, p3, p4; /* args for syslog, ick! */
	char		*buf = NULL;	/* buffer for massaging error */
	char		*cp;		/* generic character pointer */
	int		olderrno = cthread_errno();

	if ([NXDaemon getDebugMode]) {

		/* If we use "%m" anywhere, we need to really work */
		if (cp = strstr(fmt, "%m")) {

			/* Allocate a buffer and transcribe the string */
			buf = (char *) malloc(strlen(fmt) + 80);
			strncpy(buf, fmt, (int)(cp - fmt));
			strcat(buf, strerror(olderrno));
			cp += 2;	/* skip "%m" */
			strcat(buf, cp);
			fmt = buf;
		}

		fprintf(stderr, "%s: ", [NXDaemon programName]);
		vfprintf(stderr, fmt, ap);

		/* Tack on a carriage return if needed */
		if (fmt[strlen(fmt) - 1] != '\n') {
			fprintf(stderr, "\n");
		}

		/* Free buffer if we used it */
		if (buf) {
			free(buf);
		}
	} else {
		/* Here comes the sick part */
		p0 = va_arg(ap, int);
		p1 = va_arg(ap, int);
		p2 = va_arg(ap, int);
		p3 = va_arg(ap, int);
		p4 = va_arg(ap, int);
		syslog(priority, fmt, p0, p1, p2, p3, p4);
	}
}

/**********************************************************************
 * Routine:	_logMessageError() - Handle a messaging error.
 *
 * Function:	This routine is a little more complex than your usual
 *		error routine.  It actually will log the error, and
 *		then cause the program to dump core if we are in debug
 *		mode.
 **********************************************************************/
static id
_logMessageError(id self, char *fmt, va_list ap)
{

    /* Log the error message */
    LogError("Objective-C error in object of class %s:",
	     NAMEOF(self));
    _logCommon(LOG_ERR, fmt, ap);

    /* Send the panic message to the daemon */
    [NXDaemon panic];

    /* Never gets here, but let's keep the compiler quite */
    return(self);
}

/*
 * External routines
 */

/**********************************************************************
 * Routine:	LogInit - Set up logging system.
 *
 * Function:	Calls openlog if we are not in debug mode.
 **********************************************************************/
void
LogInit()
{

    /* Set up syslog if appropriate */
    if (![NXDaemon getDebugMode]) {
	openlog([NXDaemon programName], LOG_CONS|LOG_PID, LOG_LPR);
    }

    /* Install our own ObjC message error handler */
    _error = _logMessageError;
}


/**********************************************************************
 * Routine:	LogWarning() - Log a warning
 *
 * Function:	Set up varargs and call _logCommon with the LOG_WARNING
 *		priority.
 **********************************************************************/
void
LogWarning(char *fmt, ... )
{
    va_list		ap;		/* vararg list */

    va_start(ap, fmt);
    _logCommon(LOG_WARNING, fmt, ap);
    va_end(ap);
}

/**********************************************************************
 * Routine:	LogError() - Log an error
 *
 * Function:	Set up varargs and call _logCommon with the LOG_ERR
 *		priority.
 **********************************************************************/
void
LogError(char *fmt, ... )
{
    va_list		ap;		/* vararg list */

    va_start(ap, fmt);
    _logCommon(LOG_ERR, fmt, ap);
    va_end(ap);
}

/**********************************************************************
 * Routine:	LogMallocDebug()	- Report a malloc error
 *
 * Function:	This is called when malloc gets an error.  It will print
 *		an appropriate error message, and if appropriate will
 *		set cthread_errno() to ENOMEM.
 **********************************************************************/
void
LogMallocDebug(int mallocError)
{
    static char *_mallocErrorMessages[] = {
	"vm_allocate failed",
	"vm_deallocate failed",
	"vm_copy failed",
	"attempt to free or realloc space already freed",
	"internal verification of memory heap failed",
	"attempt to free or realloc space not in heap"
    };

    /* Log the appropriate error message */
    if ((mallocError >= (sizeof _mallocErrorMessages) / sizeof (char *)) ||
	mallocError < 0) {
	LogError("memory allocation error");
    } else {
	LogError("memory allocation error: %s",
		 _mallocErrorMessages[mallocError]);
    }

    /* Set errno if appropriate */
    if ((mallocError == MALLOC_ERROR_VM_ALLOC) ||
	(mallocError == MALLOC_ERROR_FREE_FREE)) {
        cthread_set_errno_self(ENOMEM);
    }
}


/*
 * Conditionally compiled routines.
 */

#ifdef	DEBUG
/**********************************************************************
 * Routine:	LogDebug() - Log debugging information.
 *
 * Function:	Set up varargs and call _logCommon with the LOG_DEBUG
 *		priority.
 **********************************************************************/
void
LogDebug(char *fmt, ... )
{
    va_list		ap;		/* vararg list */

    if (![NXDaemon getDebugMode]) {
	return;
    }

    va_start(ap, fmt);
    _logCommon(LOG_DEBUG, fmt, ap);
    va_end(ap);
}
#else	DEBUG
/**********************************************************************
 * Routine:	LogDebug() - Log debugging information.
 *
 * Function:	Set up varargs and call _logCommon with the LOG_DEBUG
 *		priority.
 **********************************************************************/
void
LogDebug(char *fmt, ... )
{
}
#endif	DEBUG
