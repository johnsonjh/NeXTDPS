/*
  unixstream.h

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Wed Mar 21 13:51:26 1984
Edit History:
Ed Taft: Tue May  3 10:03:31 1988
Ivor Durham: Wed Oct 26 08:07:04 1988
Jim Sandman: Wed Mar 16 15:46:55 1988
End Edit History.

This interface defines a facility for stream access to files provided
by an underlying operating system such as Unix. The main client of
this interface is the "stodev" package, which provides (among other
things) a "Unix" class of PostScript storage device, from which file
objects can be constructed. This interface is used by the PostScript
kernel only to access initial files.
*/

#ifndef UNIXSTREAM_H
#define UNIXSTREAM_H

#include STREAM


/* For the following procedures, "filename" is a null-terminated
   string that names a file. Its syntax is operating system dependent.

   "mode" is a null-terminated string that describes the mode in which
   the file is to be opened. A mode always begins with "r", "w", or "a",
   possibly followed by "+". Any additional characters provide operating
   system specific information. The modes have the following meanings:

   r   open for reading only; error if file doesn't already exist.
   w   open for writing only; create file if it doesn't already
       exist; truncate it if it does.
   a   open for writing only; create file if it doesn't already
       exist; append to it if it does.
   r+  open for reading and writing; error if file doesn't already
       exist.
   w+  open for reading and writing; create file if it doesn't
       already exist; truncate it if it does.
   a+  open for reading and writing; create file if it doesn't
       already exist; append to it if it does.

   "fd" is an integer "file descriptor" that represents an open file.
   It is used only when calling lower-level operating system primitives,
   not when accessing files at the stream level. Not all operating
   systems support file descriptors.
 */

extern procedure UnixStmInit();
/* Creates the standard streams os_stdin, os_stdout, and os_stderr
   by opening streams for the standard file descriptors 0, 1, and 2,
   which are assumed to exist already. StmInit must have been called
   previously.
 */

extern Stm os_fopen(/* char *filename, *mode */);
/* Opens a file named by filename in the specified mode, then creates
   and returns a Stm through which the file may be accessed.
   Returns NULL if unsuccessful; the reason for failure may have
   been stored in "errno" by the underlying file system primitives.
 */

extern Stm os_fdopen(/* int fd, char *mode */);
/* Creates and returns a Stm for an existing open Unix file designated
   by fd. The mode must be the same as was used to open the file.
   Returns NULL if unsuccessful.
 */

extern int os_fileno(/* Stm stm */);
/* Returns the file descriptor associated with stm; -1 if the underlying
   operating system doesn't support file descriptors. This is valid only
   if stm was created by os_fdopen or os_fopen; it does not work for
   arbitrary streams.
 */

#endif UNIXSTREAM_H
