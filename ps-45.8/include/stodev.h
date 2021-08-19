/*
  stodev.h

Copyright (c) 1986, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Don Andrews: October 1986
Edit History:
Jim Sandman: Fri Mar 11 11:52:09 1988
Dick Sweet: Mon Dec 1 10:31:03 PST 1986
Don Andrews: Tue Jan 6 17:17:00 PST 1987
Ed Taft: Thu May  5 09:43:29 1988
Ivor Durham: Tue Oct 25 11:26:10 1988
End Edit History.
*/

#ifndef	STODEV_H
#define	STODEV_H

#include PUBLICTYPES
#include STREAM

/*
 * Storage Device interface
 */

/*
  The PostScript system supports one or more Storage Devices (StoDev,
  so named to distinguish them from Device, which is a display device).
  A StoDev defines a collection of operations on files in secondary
  storage. These operations are invoked by the implementations of the
  PostScript file-related operators, such as "file", "deletefile", etc.
  The stodev interface is concerned mainly with operations on named
  files; once a file has been opened, reading and writing the contents
  of the file is handled through the stream interface.

  Different StoDevs have different properties, and not all StoDevs
  support all operations. The StoDev record describes the properties
  of a particular storage device. The StoDevProcs record contains
  pointers to a set of procedures that implement the operations for
  a particular class of storage device.

  The StoDev abstraction defines a uniform convention for naming
  devices, but says nothing about how files in a given device are named.
  A complete file name is in the form "%device%file", where "device"
  is the storage device name and "file" is the name of the file within
  the device. When a complete file name is presented to the StoDev
  machinery, it uses the "device" portion to select the appropriate
  storage device but leaves the "file" portion to be interpreted by a
  device dependent procedure that implements the requested operation.

  When a file name is presented without a "%device%" prefix, a search
  rule determines which storage device is selected. Each StoDev has
  an integer "searchOrder" field. The StoDevs are consulted in
  increasing order of searchOrder; the requested operation is
  performed for each StoDev in turn until it succeeds. Only StoDevs
  that are "searchable" are considered.

  Certain StoDevs have special characteristics. A StoDev can represent
  "removable" media; if so, it supports the Mount and Dismount
  operations; other operations can be performed only if the StoDev
  is "mounted". A StoDev normally supports named files, as described
  above; however, if "hasNames" is false, the StoDev contains a
  single unnamed file that is referenced by the complete name
  "%device%" (such a StoDev cannot be "searchable").

  There are no "standard" StoDevs; the environment can register one
  or more StoDevs for different classes of secondary storage devices
  (by calling RgstStoDev). However, there are several conventions for
  the definition of specific StoDevs:

  1. There may be a StoDev that represents the complete file system
  provided by the underlying operating system (Unix or whatever).
  If so, by convention, that StoDev's name should be "os" so that
  complete file names are in the form "%os%file", where "file" conforms
  to underlying file system conventions. (Any restrictions on "file",
  such as whether names may be rooted or relative, are StoDev specific.)
  This StoDev should have its searchable flag set true so that a
  PostScript program can refer to "file" without the "%os%" prefix.
  (If there is more than one StoDev that works this way, the choice of
  device name is arbitrary; all such devices should be searchable.)

  2. There may be a StoDev that represents fonts that may be loaded
  dynamically by the "findfont" operator. If so, by convention, that
  StoDev's name should be "font" so that complete file names are in
  the form "%font%file", where "file" is the specific font name;
  for example, "%font%Palatino-BoldItalic". Note that this naming
  convention does not necessarily have anything to do with how font
  files are actually named in the underlying file system; the "font"
  device is logically decoupled from the "os" device. This StoDev
  should not have its searchable flag set, thereby exclusing font
  names from consideration during searches for names that do not have
  a leading "%device%" prefix. This StoDev is only required to support
  reading of existing files; however, if possible, it should also
  support enumeration (allowing a PostScript program to obtain a list
  of available fonts) and writing (allowing a PostScript program to
  permanently install new fonts).

  Important: some of the procedures defined in this interface
  (including the ones in the StoDevProcs record) can raise exceptions,
  which some caller must catch. Exceptions may be raised only from
  those procedures whose descriptions say so explicitly. The exceptions
  are limited to the PostScript language level errors described in
  except.h; no device specific exceptions are permitted. In general,
  the meanings of the exceptions are:

    ecInvalidFileAccess -- invalid access mode or attempt to access
      file in an invalid way (e.g., write on read-only file);
    ecUndefFileName -- invalid file name or no file by that name exists;
    ecUndef -- requested operation undefined for this device;
    ecIOError -- unspecified other error or malfunction.
 */

/* Data Structures */

#define LDELIM '%'
#define RDELIM '%'

#define MAXNAMELENGTH 100
  /* Maximum length of complete file name including "%device%" prefix */

typedef struct _t_StoDevProcs { /* Storage Device Procedures record */

  boolean (*FindFile)(/* PStoDev dev, char *name */);
    /* Determines whether a file with the specified name (without
       "%device%" prefix) exists in the device; returns true if so,
       false if not. This procedure raises no exceptions.
     */

  Stm (*CreateDevStm)(/* PStoDev dev, char *name, *access */);
    /* Opens a file with the specified name (without "%device%"
       prefix) and creates a stream for it. Returns the stream
       if successful; either returns NIL or raises an exception
       if unsuccessful (details below).

       The meaning of access is device-dependent; see unixstream.h
       for a specific set of conventions for access strings.
       This operation may create a new file if access permits.

       An unsuccessful outcome is indicated in either of two ways.
       If the file is simply not found and the access does not specify
       that the file should be created, the procedure returns NIL.
       If some other failure occurs, an exception is raised.
     */

  procedure (*Delete)(/* PStoDev dev, char *name */);
    /* Deletes file identified by name (without "%device" prefix).
       Raises an exception if the operation fails.
     */

  procedure (*Rename)(/* PStoDev dev, char *oldname, *newname */);
    /* Renames file named oldname to be newname. Both names are relative
       to the specified StoDev and do not have a "%device%" prefix.
       Raises an exception if the operation fails.
     */

  procedure (*ReadAttributes)(/*
    PStoDev dev, char *name, StoDevFileAttributes *attributes */);
    /* Obtains attributes of file identified by name (without
       "%device%" prefix) and stores them into *attributes.
       Raises an exception if the operation fails.
     */

  int (*Enumerate)(/*
    PStoDev dev, 
    char *pattern, 
    int (*action)(char *name, *arg),
    char *arg */);
    /* Enumerates all files matching a pattern. For each file in the
       StoDev whose name matches the pattern, Enumerate calls
       (*action)(name, arg), where name is the matching file name
       and arg is the uninterpreted arg passed to Enumerate.
       If action returns a nonzero result, the enumeration terminates
       and Enumerate returns that result. If action returns zero,
       the enumeration continues; when all files have been exhausted,
       Enumerate returns zero. This procedure raises no exceptions
       of its own, but the action procedure might do so.

       The details of pattern matching are device dependent. However,
       a recommended convention is the one provided by the SimpleMatch
       procedure, defined below. Likewise, the set of files considered
       for matching is device dependent; for example, the "font" device
       might consider all font names whereas the "os" general filesystem
       device might consider only names in the current working directory.

       If *pattern does not begin with "%", it is matched against
       StoDev relative file names, i.e., without "%device%" prefix.
       When a match occurs, the name passed to the action procedure
       is likewise StoDev relative.

       If *pattern begins with "%", it should be matched against
       complete file names in the form "%device%file"; names passed
       to the action procedures should also be in this form.
       Note that pattern matching may be performed on both the
       "device" and "file" parts of the name; patterns such as
       "%*%foo" and "%font%*" are allowed. In particular, if the
       pattern cannot match any complete name having the StoDev's name
       as a prefix, Enumerate should immediately return zero without
       considering any files.

       The order of enumeration is unspecified and device dependent.
       There are no restrictions on what the action procedure can do.
       In particular, it is permitted to perform other operations
       on the same StoDev. However, if action causes new files to be
       created, it is unspecified whether or not those files will
       be encountered later in the same enumeration.
     */

  procedure (*DevAttributes)(/* PStoDev dev, StoDevAttributes *attributes */);
    /* Obtains attributes of device and stores them into *attributes.
       This information is defined only for devices that are writable
       and mounted. This procedure raises no exceptions.
     */

  procedure (*Format)(/* PStoDev dev, Int32 pages, format */);
    /* Causes the entire contents of the StoDev to be erased, destroying
       all files contained in it. The two integer arguments specify
       optional information whose precise meaning is device dependent;
       specifying zero for both arguments gives correct default behavior.
       A non-zero pages argument limits the available capacity of the
       StoDev to be less than its physical capacity. A non-zero format
       argument invokes device-dependent actions; typically, 1 means
       re-format the media; format > 1 means test the media (format - 1)
       times. These options are not implemented by all StoDevs.

       This procedure raises an exception if an unrecoverable error
       occurs or if the operation is not supported by the device.
     */

  procedure (*Mount)(/* PStoDev dev */);
    /* Notifies the StoDev that removable media have been installed.
       The caller asserts that the StoDev is removable and not already
       mounted. This procedure may raise an exception if it is unable
       to access the media; if this happens, the StoDev is not mounted.
     */

  procedure (*Dismount)(/* PStoDev dev */);
    /* Notifies the StoDev that the media may have been or may about
       to be removed. The caller asserts that the StoDev is removable
       and mounted. This procedure may raise an exception if a failure
       occurs; if this happens, the StoDev nevertheless becomes not
       mounted.
     */

} StoDevProcs, *PStoDevProcs;

  
typedef struct _t_StoDev {	/* Storage Device instance */
  struct _t_StoDev *next;	/* -> next registered StoDev */
  BitField removable: 1;	/* device is removable; can be referenced
				   only if mounted */
  BitField mounted: 1;		/* device is present (if removable) */
  BitField hasNames: 1;		/* device supports named files; false means
				   device contains one unnamed file */
  BitField writeable: 1;	/* device can be written upon */
  BitField searchable: 1;	/* device is included in search order
				   when no device name is specified;
				   hasNames must also be true */
  Int16 searchOrder;		/* determines search order (lowest first) */
  char *name;			/* device name (without "%" delimiters) */
  Int32 currentVolId;		/* if removable, id for this volume */
  char *currentVolName;		/* if removable, user readable device id */
  Int32 private1, private2, private3;	/* for device dependent stuff */
  PStoDevProcs procs;		/* -> procedures for this device */
} StoDev, *PStoDev;

typedef struct _t_StoDevFileAttributes {/* Result record for ReadAttributes */
  /* Note: interpretation of time fields is environment dependent;
     the only assumption a caller can make is that they are monotonic. */
  Card32 createTime;		/* time information in file was created */
  Card32 updateTime;		/* time file was last written */
  Card32 accessTime;		/* time file was last accessed (read|write) */
  Int32 byteLength;		/* logical length of file in bytes */
  Int32 pages;			/* space actually occupied by file, in
				   implementation dependent units */
} StoDevFileAttributes;

typedef struct _t_StoDevAttributes {/* Result record for DevAttributes */
  /* Note: a "page" is an implementation dependent unit of storage */
  Int32 size;			/* total size of storage device */
  Int32 freePages;		/* number of free pages in device */
} StoDevAttributes;


/* Exported Procedures */

extern PStoDev FndStoDev(/* char *name, **suffix */);
/* If the string *name is in the complete form "%device%file"
   and the "device" portion matches the name of an existing StoDev,
   returns a pointer to that StoDev. If there is no StoDev by that
   name or if *name does not have a "%device%" prefix, returns NIL.
   This procedure raises no exceptions.

   Additionally, if suffix is not NIL, stores in *suffix a pointer
   to the beginning of the StoDev specific portion of the name.
   If *name is in the form "%device%file", this is a pointer to
   the beginning of "file". If *name does not begin with "%", this
   is just a copy of the name pointer. However, *suffix is set to NIL
   if a device is specified but does not match any existing device
   (or if the second "%" is absent).
 */

extern PStoDev FndStoFile(/* char *name, **suffix */);
/* Finds an existing file whose name is *name and returns a pointer to
   the StoDev containing it, NIL if no such file is found.
   If the string *name is in the complete form "%device%file",
   this procedure searches only the StoDev whose name is "device".
   Otherwise, it enumerates searchable StoDevs, according to their
   searchOrder, until the file is found or all StoDevs have been searched.
   In any case, if a StoDev is removable, it is considered only if it
   is mounted. Existence of the file is determined by calling the StoDev's
   FindFile procedure (but if hasNames is false, the lookup succeeds
   without FindFile being called). This procedure raises no exceptions.

   If suffix is not NIL, this procedure stores a result in *suffix
   in the same fashion as FndStoDev.
 */

extern PStoDev StoDevGetNext(/* PStoDev dev */);
/* Enumerates registered StoDevs. If dev is NIL, this procedure returns
   the first StoDev in the enumeration. If dev is not NIL, it returns
   the next StoDev after the specified one; if dev is the last one,
   it returns NIL. The order of enumeration is ascending order of
   searchOrder fields; all StoDevs are enumerated, regardless of the
   state of searchable, removable, or mounted flags. This procedure
   raises no exceptions.
 */


/* Note: following procedures are not called from the PostScript kernel;
   they are defined here as a convenience to StoDev implementations.
 */

extern boolean SimpleMatch(/* char *name, *pattern */);
/* Matches *name against *pattern, returning true if they match,
   false if not. All characters in pattern are treated literally
   (and are case sensitive), except the following special characters:
     "*"  matches zero or more consecutive characters;
     "?"  matches exactly one character;
     "\"  causes the next character of the pattern to be treated
	  literally, even if it is "*", "?", or "\".
   This procedure is suitable as part of the implementation of
   a StoDev's Enumerate procedure. It raises no exceptions.
 */

extern procedure RgstStoDevice(/* PStoDev dev */);
/* Registers a StoDev, making it known to procedures such as FndStoDev
   and FndStoFile. The StoDev record must have been allocated and
   completely filled in by the caller. There must not already be
   a StoDev with the same device name. Note that this procedure
   does not perform any StoDev operations. It raises no exceptions.
 */

extern procedure UnRgstStoDevice(/* PStoDev dev */);
/* Unregisters a StoDev that was previously registered. Note that
   this procedure does not perform any StoDev operations; it has no
   effect on any existing open files for that device. It raises
   no exceptions.
 */

extern procedure StoDevInit();
/* Initializes the StoDev package; must be called before any other
   procedures in this interface. It raises no exceptions.
 */

#endif	STODEV_H
/* v010 durham Fri Apr 8 11:39:13 PDT 1988 */
/* v011 durham Wed Jun 15 14:24:02 PDT 1988 */
/* v012 durham Thu Aug 11 13:04:28 PDT 1988 */
/* v013 caro Thu Nov 3 14:18:35 PST 1988 */
/* v014 pasqua Thu Dec 15 11:13:05 PST 1988 */
/* v015 byer Tue May 16 11:19:12 PDT 1989 */
/* v016 sandman Mon Oct 16 17:04:32 PDT 1989 */
