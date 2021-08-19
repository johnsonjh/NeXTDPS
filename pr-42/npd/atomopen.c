/*
 * atomopen.c	- Implementation of "atomic open" for use with threads and exec's.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 *
 * This module implements a way of getting around the UNIX misfeature that file
 * descriptors stay open across execv's.  One can get fd's to close on an execv by
 * using fcntl(fd, F_SETFD, 1).  If there are multiple threads in the process, there
 * is still a race condition between the time the file gets opened and the time the F_SETFD
 * bit gets set.  So this module contains a mutex variable that should be used by open
 * operations and fork operations to make sure everything is clean.
 */


/* I have not implemented all open functions here.
   If you need more, go ahead and add them. */

#import <libc.h>
#import <fcntl.h>
#import <cthreads.h>
#import <stdio.h>
#import "atomopen.h"


mutex_t openLock;


/* Init the module */
void init_atom_open()
{
    openLock = mutex_alloc();
}


int atom_open(const char *path, int flags, int mode)
{
    int result;
    
    mutex_lock(openLock);
    result = open(path, flags, mode);
    if (result >= 0)
	fcntl(result, F_SETFD, 1);
    mutex_unlock(openLock);
    return result;
}


FILE *atom_fopen(const char *filename, const char *mode)
{
    FILE *result;
    
    mutex_lock(openLock);
    result = fopen(filename, mode);
    if (result != NULL)
	fcntl(fileno(result), F_SETFD, 1);
    mutex_unlock(openLock);
    return result;
}


/* We want to lock around the fork operation as well, so that the child does not get a
   file descriptor without the close-on-exec property.  Don't need to use this unless the
   child will eventually do an exec of some kind. */
int atom_fork(void)
{
    int pid;			/* Result */
    
    mutex_lock(openLock);
    pid = fork();
    if (pid != 0)		/* Do not unlock in child */
	mutex_unlock(openLock);
    return pid;
}

