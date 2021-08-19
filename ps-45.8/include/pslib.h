/*
  pslib.h

	Copyright (c) 1987, 1988 Adobe Systems Incorporated.
			All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is stricly forbidden unless prior written
permission is obtained from Adobe.

      PostScript is a registered trademark of Adobe Systems Incorporated.
	Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Tue Jul  7 10:19:53 1987
Edit History:
Ed Taft: Wed May  4 17:06:37 1988
Ivor Durham: Tue Aug 23 17:04:33 1988
Jim Sandman: Wed Mar  9 14:24:49 1988
Perry Caro: Thu Nov  3 16:00:47 1988
End Edit History.

This package contains a collection of more-or-less unrelated library
facilities. Many of these are derived from or equivalent to the
standard C library facilities, but some are not. The ones that are
derived from the C library are renamed to have an "os_" prefix.
This enables them to be implemented either as a veneer over the
corresponding C library procedures or independently.

Some of the definitions here differ from standard C definitions in
declaring integer arguments or results as "long int" instead of "int".
Care may be required in configurations in which those two types are
not the same.
*/

#ifndef	PSLIB_H
#define	PSLIB_H

#include PUBLICTYPES

/*
 * Byte Array Interface
 */

/* Exported Procedures */

extern procedure os_bcopy(/* char *from, *to; long int count */);
/* Copies count consecutive bytes from a source block to a destination
   block. This procedure works correctly for all positive values of
   the count (even if greater than 2**16); it does nothing if count
   is zero or negative. This procedure works correctly even if the
   source and destination blocks overlap; that is, the result is
   as if all the source bytes are read before any destination bytes
   are written. */

extern procedure os_bzero(/* char *to; long int count */);
/* Sets count consecutive bytes to zero starting at the specified
   destination address. This procedure works correctly for all positive
   values of the count (even if greater than 2**16); it does nothing
   if count is zero or negative. */

extern procedure os_bvalue(/* char *to; long int count; char value */);
/* Sets count consecutive bytes to value starting at the specified
   destination address. This procedure works correctly for all positive
   values of the count (even if greater than 2**16); it does nothing
   if count is zero or negative. */


/*
 * String interface
 *
 * Note: in all the following procedures, a "string" is an indefinitely
 * long array of characters terminated by '\0' (a null character).
 */

/* Exported Procedures */

extern char *os_index(/* char *str; char ch */);
/* Returns a pointer to the first occurrence of the character ch
   in the string str, or zero if ch does not occur in str. */

extern int os_strlen(/* char *str */);
/* Returns the number of characters in the string, not counting
   the terminating null byte. */

extern int os_strcmp(/* char *str1, *str2 */);
/* Compares two strings lexicographically and returns a negative
   result if s1 is less than s2, zero if they are equal, and
   a positive result if s1 is greater than s2. s1 is
   lexicographically less than s2 if it is a substring or if
   the first non-matching character of s1 is less than the
   corresponding character of s2. */

extern char *os_strcpy(/* char *to, *from */);
/* Copies the contents of the second string (including the terminating
   null character) into the first string, overwriting its former
   contents. The number of characters is determined entirely by
   the position of the null byte in the source string and is not
   influenced by the former length of the destination string.
   The source and destination strings must not overlap. Returns
   the destination string pointer. */

extern char *os_strncpy(/* char *to, *from, *cnt */);
/* Copies cnt bytes from the contents of the second string to the first
   string, truncating or zero padding the first string as necessary.  The
   first string will be null terminated only if cnt is greater than the
   length of the second string. */

extern char *os_strcat(/* char *to, *from */);
/* Appends a copy of the second string (including its terminating
   null character) to the end of the first string, overwriting
   the first string's terminating null. It is the caller's
   responsibility to ensure adequate space beyond the end of
   the first string. The source and destination strings must not
   overlap. Returns the destination string pointer. */


/*
 * Miscellaneous Numerical Operations Interface
 */

extern long int
  os_max(/* long int a, b */),
  os_min(/* long int a, b */);
/* Returns the maximum or minimum, respectively, of two long integer
   arguments. Unlike the macros MAX and MIN, these are pure procedures;
   thus, it's OK for their arguments to be expressions with side-effects,
   and the calling sequence, though less effecient, is usually shorter. */


/*
 * Memory Allocator Interface
 */

extern char *os_malloc(/* long int size */);
/* Allocates size consecutive bytes of memory and returns a pointer
   to the first byte. The block is guaranteed to be aligned such
   that it may be used to store any C data type. The initial contents
   of the allocated memory are undefined. If os_malloc is unable
   to satisfy the request, it returns zero. Note: os_malloc's argument
   is a long int that may be larger than 2**16; this procedure
   is named "mlalloc" in some C libraries. */

extern char *os_sureMalloc(/* long int size */);
/* Like os_malloc, except that inability to allocate the requested
   storage causes a fatal error */

extern char *os_memalign(/* long int boundary, size */);
/* Same as os_malloc(size) except that the block is aligned to a multiple
   of boundary, which must be a power of 2. */

extern char *os_calloc(/* long int count, size */);
/* Allocates memory for an array of count elements, each of which occupies
   the specified number of bytes. The block is guaranteed to be aligned
   such that it may be used to store any C data type. The contents of
   the allocated memory are set to zero. If os_calloc is unable
   to satisfy the request, it returns zero. Note: os_calloc's arguments
   are long ints that may be larger than 2**16; this procedure
   is named "clalloc" in some C libraries. */

extern char *os_sureCalloc(/* long int count, size */);
/* Like os_calloc, except that inability to allocate the requested
   storage causes a fatal error */

extern char *os_realloc(/* char *ptr; long int size */);
/* Takes a pointer to a memory block previously allocated by os_malloc
   or os_calloc and changes its size, relocating it if necessary;
   returns a pointer to the new location. The contents within the
   minimum of the old and new sizes are preserved; if the new size is
   larger than the old, the extension has undefined contents.
   If the block must be relocated, the memory formerly occupied by it
   is released and the old pointer becomes invalid. If realloc is
   unable to satisfy the request, it returns zero; the contents of
   the existing block are undisturbed. Note: os_realloc's argument
   is a long int that may be larger than 2**16; this procedure
   is named "relalloc" in some C libraries. Note: the ability to
   reallocate the last block freed, provided in some C libraries,
   is not supported. Note: if the block was originally allocated
   by os_memalign, the original alignment is not necessarily preserved. */

extern char *os_valloc(/* long int size */);
/* os_valloc is equivalent to os_memalign (getpagesize(), size). That is,
   it returns a block of storage that is "page aligned", whatever
   that means in a given environment.
 */

extern procedure os_free(/* char *ptr */);
/* Takes a pointer to a memory block previously allocated by os_malloc
   or os_calloc and deallocates the memory, making it available for
   subsequent re-use. Note: the contents of the block should be
   assumed to become instantly invalid. */

/*
   Note: the following lower-level storage management procedure
   is not called by the PostScript kernel. It performs dynamic
   additions to the storage pool managed by os_malloc. In most
   environment, os_malloc is able to perform storage pool expansion
   on its own.
 */

extern procedure os_addmem(/* char *ptr; long int size */);
/* Adds a block of memory to the storage being managed by os_malloc.
   The arguments specify the base address and size in bytes; they
   have no special alignment requirements. The caller asserts that
   the new block is located at a higher address than any block
   already being managed by os_malloc. The caller makes no representations
   as to how the memory was obtained; it loses control over that
   memory irrevocably.
 */


/*
 * Linked List Interface
 */

/* Lists are circular and doubly-linked through the Links structure,
   which is used for the list header and as the first element of
   each item in the list. The head item in a list is header.next
   and the tail item is header.prev. An empty list is denoted by
   header.next = header.prev = &header.
*/

/* Data Structures */

typedef struct _t_Links {
  struct _t_Links *prev, *next;
  } Links;

/* Exported Procedures */

extern procedure InitLink(/* Links *link */);
/* Initializes an empty list. */

extern procedure RemoveLink(/* Links *link */);
/* Removes *link from its chain. */

extern procedure InsertLink(/* Links *where, *link */);
/* Inserts *link as the successor of *where. */

/* Inline Procedures */

/* Exported Data */

/*
 * Pool interface
 */

/* Data Structures */

typedef struct _t_Pool *Pool;

extern Pool os_newpool (/* int elementSize, elementsPerChunk, maxElements */);
 /*
   Create a new pool in which there are elementSize bytes in each element,
   each time the pool grows a chunk of elementsPerChunk elements is added
   to the pool, and the total number of elements in the pool must not
   exceed maxElements.  If elementsPerChunk is either -1 or 0, a default
   number is used.  If maxElements is -1 or 0, the pool size is unlimited.

   May raise ecLimitCheck if the pool or first chunk cannot be allocated.
   May raise ecRangeCheck if either elementsPerChunk or maxElements are bad.
  */

extern procedure os_freepool (/* Pool */);
 /*
   Free all of the elements in the pool and then the pool itself.
   pre: parameter is a valid Pool.
   post: parameter is invalid.
  */

extern char *os_newelement (/* Pool */);
 /*
   Allocate a new element from Pool.  May raise ecLimitCheck if the pool
   size limit would be exceeded or an attempt to grow the pool fails.
  */

extern procedure os_freeelement (/* Pool, char * */);
 /*
   Return element to pool.
   pre: char* must point to an element previously allocated from the given
        Pool.
   post: element is available in the Pool.
  */

/*
 * Timekeeping interface
 */

extern integer os_clock();
/* Returns the current value of a real time clock that represents
   number of milliseconds since an arbitrary starting time.
   Although the value represents milliseconds, the frequency at
   which it updates is implementation dependent.
 */

extern integer os_userclock();
/* Returns the current value of a clock that represents total
   execution time of the PostScript interpreter in milliseconds
   since an arbitrary starting time. To the extent possible, time
   spent in I/O wait, other processes, etc. should be subtracted
   out. Although the value represents milliseconds, the frequency
   at which it updates is implementation dependent.
 */


#endif	PSLIB_H
/* v004 durham Sun Feb 7 12:32:58 PST 1988 */
/* v005 sandman Wed Mar 9 14:14:19 PST 1988 */
/* v006 durham Fri Apr 8 11:38:11 PDT 1988 */
/* v007 durham Wed Jun 15 14:23:35 PDT 1988 */
/* v008 durham Thu Aug 11 13:01:05 PDT 1988 */
/* v009 caro Wed Nov 2 14:19:51 PST 1988 */
/* v010 bilodeau Wed Apr 26 12:48:06 PDT 1989 */
/* v011 byer Tue May 16 11:18:11 PDT 1989 */
/* v012 sandman Thu Jun 22 17:13:51 PDT 1989 */
/* v012 taft Thu Nov 23 15:01:42 PST 1989 */
