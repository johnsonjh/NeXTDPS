/*
  memalign.c

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Ivor Durham: Sat Feb 13 10:42:36 1988
Ed Taft: Sun May  1 16:39:33 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include PSLIB
#include "malloc.h"
#include <errno.h>

private int Split_malloc_Block (Low_Block,High_Block)
    char *Low_Block, *High_Block;
/*
 * This operation splits a malloc'd block (Low_Block) into two blocks.
 *
 * pre:	 Low_Block is a valid block previously allocated by malloc &
 *	 High_Block == address of desired block within the Low_Block block.
 *
 * post: (Invalid(Low_Block) v Invalid(High_Block)) => (RESULT = 0)  &
 *	 (Valid(Low_Block) & Valid(High_Block)) => (RESULT = 1)
 *
 * Make the pointer field of the High block from the pointer field of the
 * Low block and replaced the pointer in the Low block with one to the new
 * High block.
 *
 */

{
  union store *Low = (union store *)Low_Block;
  union store *High = (union store *)High_Block;

  MASSERT((Low > clearbusy(allocs[1].ptr)) && (Low <= alloct),Low);
  MASSERT(allock(),allocp);

  --Low;	/* Low now addresses pointer field preceding block data */
  --High;	/* High now address word to become pointer for new block*/

  if (! testbusy(Low->ptr))
    return 0;					/* Not a busy block! */
  else if ((High <= Low) || (High >= clearbusy(Low->ptr)))
    return 0;					/* Beyond end of block */

  High->ptr = Low->ptr;				/* Copy (busy) pointer */
  Low->ptr = setbusy(High);			/* Link Low block to High */

  return 1;
}

/*
 * The following implementation of "os_memalign" is based on the original
 * Sun Microsystems implementation (Copyright (c) 1984 Sun Microsystems),
 * but adapted to the Adobe "os_malloc" storage allocator.
 */

#define	MALLOC_OVERHEAD	sizeof(union store)

#define	Misaligned(x)	((long int)(x) & 1)

/* macro form of Roundup tingles compiler register allocation bug on Sun 2 */
private long int Roundup(x, y)
  long int x, y;
{
  return ((((x) + ((y) - 1)) / (y)) * (y));
}

public char *os_memalign (boundary, size)
  long int boundary, size;
/*
 * Allocate a block of "size" bytes on a boundary of "boundary" bytes.
 *
 * pre:  true
 *
 * post: (Success => RESULT == address) &
 *	 ((size == 0 || Misaligned(boundary) || heap corrupted) =>
 *		(RESULT == NULL & errno == EINVAL)) &
 *	 ((can't allocate memory) => (RESULT == NULL & errno == ENOMEM))
 *
 */
{
  long int  Enclosing_Block_Size;
  union store	*Temp;
  char		*New_Block,
  		*Aligned_Block,
		*Tail_Block;

  /* Check arguments */

  if ((size == 0) || Misaligned(boundary)) {
#if MALLOC_DEBUG
    os_eprintf ("<- memalign: Invalid parameter(s)--size == 0 or boundary odd\n");
#endif MALLOC_DEBUG

    errno = EINVAL;
    return NULL;
  }

  /*
   * Compute the size of the smallest block to "malloc" to ensure that a
   * block of the required size on the required boundary is within the
   * allocated block.
   */

  size = Roundup (size, WORD);	/* Allocate in word units only */

  Enclosing_Block_Size = size + boundary + MALLOC_OVERHEAD;

  /* Allocate the big block. */

  New_Block = os_malloc (Enclosing_Block_Size);

  if (New_Block == NULL) {
#if MALLOC_DEBUG
    os_eprintf ("<- memalign: malloc failed to allocate requested block.\n");
#endif MALLOC_DEBUG

    errno = ENOMEM;
    return NULL;
  }

  /* Identify address of required aligned block */

  Aligned_Block = (char *)Roundup((INT)New_Block, boundary);

  /* Return space in big block below boundary address, if any. */

  if ((Aligned_Block - New_Block) >= MALLOC_OVERHEAD) {
    if (Split_malloc_Block (New_Block, Aligned_Block))
      os_free (New_Block);
    else {
      /*
       * Split_malloc_Block failed in spite of check.
       * Prevent propagation by reporting failure.
       */
#if MALLOC_DEBUG
      os_eprintf ("<- memalign: FAILURE! Split_malloc_Block failed (low block).\n");
#endif MALLOC_DEBUG

      errno = EINVAL;
      return NULL;
    }
  }
  else if (Aligned_Block != New_Block) {
    /*
     * Heap corrupted because New_Block must precede Aligned_Block by
     * at least MALLOC_OVERHEAD ("sizeof(union store)") if they are not
     * equal.
     */
#if MALLOC_DEBUG
    os_eprintf ("<- memalign: Heap apparently corrupted!\n");
#endif MALLOC_DEBUG

    errno = EINVAL;
    return NULL;
  }
  else {
   /* New_Block == Aligned_Block: already aligned properly. */
  }

  /* Return space beyond end of required block, if possible. */

  Tail_Block = (char *)((INT)Aligned_Block + size + MALLOC_OVERHEAD);

  Temp = (union store *)((INT)Aligned_Block - MALLOC_OVERHEAD);

  if ((INT)Tail_Block < (INT)(clearbusy(Temp->ptr))) {
    if (Split_malloc_Block (Aligned_Block, Tail_Block))
      os_free (Tail_Block);
    else {
      /*
       * Split_malloc_Block failed, but the failure is benign here.
       * We simply return a larger block than was originally requested.
       */
#if MALLOC_DEBUG
      os_eprintf ("memalign: Split_malloc_Block failed (high block).\n");
#endif MALLOC_DEBUG
    }
  }
  else {
#if MALLOC_DEBUG
    os_eprintf ("memalign: Could not free tail block.\n");
#endif MALLOC_DEBUG
  }

  /*
   * Done.  Aligned_Block is an address on a suitable "boundary" for a block
   * of at least "size" bytes.
   */

  return Aligned_Block;
}
