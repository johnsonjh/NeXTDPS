/*
 * atomopen.h	- Implementation of "atomic open" for use with threads and exec's.
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
 
 
#import <stdio.h>
#import <cthreads.h>


/* The mutex we use to control things */
extern mutex_t openLock;


void init_atom_open();
int atom_open(const char *path, int flags, int mode);
FILE *atom_fopen(const char *filename, const char *mode);
int atom_fork(void);
