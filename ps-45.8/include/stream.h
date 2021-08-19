/*
  stream.h

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
Ed Taft: Tue May  3 08:52:05 1988
Doug Brotz: Mon May  5 11:09:13 1986
Ivor Durham: Sun Oct 30 12:53:15 1988
Jim Sandman: Mon Oct 16 16:52:16 1989
Perry Caro: Thu Nov  3 11:11:45 1988
Joe Pasqua: Thu Dec 15 10:59:03 1988
End Edit History.

Object-style streams; see stream.doc for complete explanation.
*/

#ifndef	STREAM_H
#define	STREAM_H

/* Constants */

#ifndef	NULL
#define NULL 0
#endif	NULL

#define EOF (-1)


/* Data Structures */

/* Following is the standard opaque handle for a stream. This is all
   that most clients should need. However, the concrete data structures
   must also be defined in this interface because they are needed by
   the macros for many of the stream access operations.
 */
typedef struct _t_StmRec *Stm;


/* Concrete stream representation, of interest mainly to stream
   implementations, not to clients.
 */

typedef struct _t_StmProcs {	/* Procedure record for a stream class */
  int (*FilBuf)(/* Stm stm */);	/* called by getc when buffer is empty */
  int (*FlsBuf)(/* int ch, Stm stm*/);/* called by putc when buffer is full */
  long int (*FRead)(/* char *ptr, long int itemSize, nItems, Stm stm */);
  				/* read block of data */
  long int (*FWrite)(/* char *ptr, long int itemSize, nItems, Stm stm */);
  				/* write block of data */
  int (*UnGetc)(/* int ch, Stm stm */); /* back up input stream */
  int (*FFlush)(/* Stm stm */);	/* flush dirty buffer if necessary */
  int (*FClose)(/* Stm stm */);	/* close stream */
  int (*FAvail)(/* Stm stm */);	/* number of chars immediately available */
  int (*FReset)(/* Stm stm */);	/* discard any buffered data */
  int (*FPutEOF)(/* Stm stm */);/* send end-of-file indication */
  int (*FSeek)(/* Stmstm, long int offset, int base */);
				/* set stream position */
  long int (*FTell)(/* Stm stm */); /* get stream position */
  char *type;			/* -> stream type name string */
  } StmProcs;

typedef struct _t_StmRec {	/* Generic stream instance */
  long int cnt;			/* chars remaining in buffer */
  char *ptr;			/* -> next char to get or put */
  char *base;			/* -> base of buffer (if any) */
  struct {			/* generic flags */
    unsigned read: 1;		/* ok to read */
    unsigned write: 1;		/* ok to write */
    unsigned readWrite: 1;	/* opened in read/write mode */
    unsigned eof: 1;		/* end-of-file has been read */
    unsigned error: 1;		/* error has occurred */
    unsigned reusable: 1;	/* stream is serially reusable */
    unsigned positionable: 1;	/* stream is positionable (fseek, ftell) */
    unsigned selectable: 1;	/* stream can be used for select */
    unsigned :4;		/* reserved for future generic flags */
    unsigned f1:1, f2:1, f3:1, f4:1; /* reserved for use by client */
    } flags;
  StmProcs *procs;		/* -> class-specific procedure record */
  struct {long int a, b;} data;	/* class-specific instance data */
  } StmRec; /* *Stm */


/*
 * Exported procedures for use by stream implementors only
 */

extern void StmInit();
/* Initializes the stream package. Must be called once, before any calls
   to the procedures below. Note: although os_stdin, os_stdout, and
   os_stderr are exported from the stream package, StmInit does not
   create the standard streams (see, for example, UnixStmInit).
 */

extern Stm StmCreate(/* StmProcs *procs; long int extraBytes; */);
  /* Allocates and returns a stream to which is attached the specified
   * StmProcs record. The initial cnt is zero, ptr and base are NULL,
   * and all the flags are false. Returns NULL if unsuccessful.
   * Note: it is the stream implementor's responsibility to properly set
   * flags such as read, write, and reusable.
   * Allocates extraBytes more bytes for the StmRec (to support subclassing).
   */

extern void StmDestroy(/* Stm stm */);
  /* Deallocates the stream object, without performing any stream-
   * dependent operations. */

extern void os_cleanup();
/* May be called prior to program exit to close all existing streams
   known to the stream package (i.e., created by StmCreate).
   It performs fclose on each stream and ignores the result.
 */

/*
  Generic implementations of certain stream operations, usable by
  stream implementations that have no special needs. These may be
  assigned to the appropriate fields of the StmProcs structure.
 */

extern long int StmFRead(/* char *ptr, long int itemSize, nItems, Stm stm */);
  /* Generic implementation of FRead */

extern long int StmFWrite(/* char *ptr, long int itemSize, nItems, Stm stm */);
  /* Generic implementation of FWrite */

extern int StmUnGetc(/* int ch, Stm stm */);
  /* Generic implementation of UnGetc */

extern int StmErr();		/* always returns EOF (as an int) */
extern long int StmErrLong();	/* always returns EOF (as a long int) */
extern long int StmZeroLong();	/* always returns zero (as a long int) */


/*
 * Exported procedures and macros for use by stream clients
 */

/* "Inline procedures" for invoking generic stream operations */

#define getc(stm) \
  (--(stm)->cnt >= 0? (unsigned char)*(stm)->ptr++ : \
    (*(stm)->procs->FilBuf)(stm))
  /* Returns the next character from stream, or EOF if end-of-file
   * has been reached or an error has occurred. */

#define ungetc(ch, stm) (*(stm)->procs->UnGetc)(ch, stm)
  /* Pushes the character ch onto the front of the input stream.
   * Returns ch normally, -1 if unsuccessful (impossible or not
   * implemented for this stream). */

#define putc(ch, stm) \
  (--(stm)->cnt >= 0 ? \
    ((unsigned char)(*(stm)->ptr++ = (unsigned char)(ch))) : \
    (*(stm)->procs->FlsBuf)((unsigned char)(ch), stm))
  /* Appends ch to stream, returning ch normally but EOF if an error
   * has occurred. If stream is buffered (as most are), ch may not
   * yet have been sent to the stream's destination (use fflush). */

#define fread(ptr, itemSize, nItems, stm) \
  (*(stm)->procs->FRead)(ptr, itemSize, nItems, stm)
  /* Reads nItems items of size itemSize from stream into memory starting
   * at ptr. Returns the number of items actually read, which can be less
   * than nItems if an error or end-of-file occurs. */

#define fwrite(ptr, itemSize, nItems, stm) \
  (*(stm)->procs->FWrite)(ptr, itemSize, nItems, stm)
  /* Writes nItems items of size itemSize to stream from memory starting
   * at ptr. Returns the number of items actually written, which can be
   * less than nItems if an error occurs. */

#define fflush(stm) (*(stm)->procs->FFlush)(stm)
  /* For an input stream, discards data until end-of-file. (For
   * positionable devices this is accomplished simply by positioning
   * to end-of-file; for other devices it requires actually reading
   * and discarding data until end-of-file is encountered.) Errors
   * are ignored during an input flush operation.
   * For an output or input/output stream, delivers any buffered
   * output characters to their destination. Returns 0 normally,
   * EOF if an error has occurred. */

#define fclose(stm) (*(stm)->procs->FClose)(stm)
  /* Delivers any buffered output, performs stream-dependent close
   * operations, and renders stream unusable for further operations.
   * Returns 0 normally, EOF if an error occurs during the close
   * (the stream is nevertheless closed in this case). */

#define favail(stm) (*(stm)->procs->FAvail)(stm)
  /* Returns the number of characters that may be immediately read
   * from stm without blocking, or -1 if this cannot be determined
   * or stm is at end-of-file. */

#define freset(stm) (*(stm)->procs->FReset)(stm)
  /* Discards buffered data. For an input stream, discards any data
   * that has been received but not yet read; for an output stream,
   * discards any data that has been written but not yet transmitted.
   * This operation never waits for I/O. It may be a no-op for "fast"
   * devices that don't ever wait indefinitely. Returns nothing. */

#define fputeof(stm) (*(stm)->procs->FPutEOF)(stm)
  /* Sends an end-of-file indication over the specified output stream.
   * Unlike fclose, fputeof does not destroy the stream but permits
   * more data (constitituting a new "file") to be sent over the same
   * stream. Returns 0 normally, EOF if the end-of-file cannot be sent
   * or if the stream is not serially reusable. */

#define fseek(stm, offset, base) (*(stm)->procs->FSeek)(stm, offset, base)
  /* Sets the stream's position in the underlying file. The new position
     is offset bytes relative to the specified base (0 = beginning,
     1 = current position, 2 = existing end). This operation is defined only
     for positionable streams. Returns 0 normally, EOF if unsuccessful. */

#define ftell(stm) (*(stm)->procs->FTell)(stm)
  /* Returns the current stream position in the underlying file as bytes
     relative to the beginning of the file. This operation is defined only
     for positionable streams. Returns EOF if unsuccessful. */


/* Class-independent and higher-level generic stream operations */

extern void os_clearerr(/* Stm stm */);
  /* Resets the end-of-file and error conditions. For an input stream,
   * if the stream is serially reusable, this permits the next file
   * to be read. */

#define feof(stm) ((stm)->flags.eof)
  /* Returns true if end-of-file has ALREADY been read, false otherwise.
   * A false result is no guarantee that the next getc will succeed. */

#define ferror(stm) ((stm)->flags.error)
  /* Returns true if an error has been encountered on stream, false
   * otherwise. An error condition persists until clearerr is called. */

extern int os_fgetc(/* Stm stm */);
  /* Same as getc, but a procedure call rather than a macro. */

extern char *os_fgets(/* char *str, int n, Stm stm */);
  /* Reads from stream to *str until n-1 characters have been received,
   * a newline character is read, or end-of-file is reached; then appends
   * a null to the string. Returns its str argument normally; NULL if
   * EOF or an error occurs before any characters have been read.
   * If a newline was read, it IS stored in the string. */

extern int os_fprintf(/* Stm stm, char *format, ... */);
  /* Performs formatted printing to stream as specified by the format
   * string and zero or more additional arguments. Returns the number of
   * characters written normally, EOF if an error has occurred. */

extern int os_fscanf(/* Stm stm, char *format, ... */);
  /* Performs scanning from stream as specified by the format string
   * and zero or more additional pointer arguments. Returns the number of items
   * converted normally, EOF if an error or end-of-file has occurred. */

extern int os_fputc(/* int ch, Stm stm */);
  /* Same as putc, but a procedure call rather than a macro. */

extern int os_fputs(/* char *str, Stm stm */);
  /* Copies null-terminated string to stream. Returns 0 normally, EOF
   * if an error has occurred. */

#define os_rewind(stm) (void) fseek((stm), 0L, 0)

/* Operations on standard I/O streams */

extern Stm os_stdin, os_stdout, os_stderr;
/* The "standard" input, output, and error streams used by the
   procedures and macros defined below. Note that these streams
   are ordinary pointer variables and may change at any time.
 */

#define getchar() getc(os_stdin)
/* Reads character from os_stdin; semantics otherwise equivalent to getc */

#define putchar(ch) putc(ch, os_stdout)
/* Writes character to os_stdout; semantics otherwise equivalent to putc */

#define	printf	os_printf
extern int os_printf(/* char *format, ... */);
/* Writes formatted output to os_stdout; semantics otherwise equivalent
   to os_fprintf.
 */

#define	sprintf	os_sprintf
extern char *os_sprintf(/* char *str, *format, ... */);
/* Writes formatted output, placing the result in the string *str,
   which had better be long enough (no error checking); returns
   str as its result. Semantics otherwise equivalent to os_fprintf.
 */

#define	scanf	os_scanf
extern int os_scanf(/* char *format, ... */);
/* Reads formatted input from os_stdin; semantics otherwise equivalent
   to os_fscanf.
 */

#define	sscanf	os_sscanf
extern int os_sscanf(/* char *str, *format, ... */);
/* Reads formatted input from the string *str; semantics otherwise
   equivalent to os_fscanf.
 */

#define	gets	os_gets
extern char *os_gets(/* char *str */);
/* Reads line of input from os_stdin into the string *str, which
   had better be long enough; semantics otherwise equivalent to fgets.
 */

#define	puts	os_puts
extern int os_puts(/* char *str */);
/* Writes line of output to os_stdout from the string *str; semantic
   otherwise equivalent to fputs.
 */

extern int os_eprintf(/* char *format, ... */);
/* Equivalent to os_fprintf(os_stderr, format, ...) followed
   by fflush(os_stderr). This is used only by low-level packages
   to report catastrophic errors.
 */

#endif	STREAM_H
/* v003 durham Sun Feb 7 12:43:22 PST 1988 */
/* v004 sandman Wed Mar 9 14:12:17 PST 1988 */
/* v005 durham Fri Apr 8 11:22:35 PDT 1988 */
/* v006 durham Wed Jun 15 14:24:35 PDT 1988 */
/* v007 durham Thu Aug 11 13:02:31 PDT 1988 */
/* v008 caro Thu Nov 3 13:24:46 PST 1988 */
/* v009 pasqua Thu Dec 15 11:00:41 PST 1988 */
/* v010 sandman Mon Oct 16 16:56:58 PDT 1989 */
