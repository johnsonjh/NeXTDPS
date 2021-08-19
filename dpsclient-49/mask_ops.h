
/*
    mask_ops.h

    This file has macros for operating on the fd_mask structures used
    refer to sets of fds.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/


/* macros to OR AND NAND masks together.  First one receives the data. */
#define FD_OR( m1, m2 )						\
{	fd_mask *m1ptr, *m2ptr;					\
	int i;							\
								\
	for( i = (sizeof(fd_set)/sizeof(fd_mask))+1, m1ptr = (m1)->fds_bits,  \
				m2ptr = (m2)->fds_bits; --i; )	\
	    *m1ptr++ |= *m2ptr++;				\
}


#define FD_AND( m1, m2 )					\
{	fd_mask *m1ptr, *m2ptr;					\
	int i;							\
								\
	for( i = (sizeof(fd_set)/sizeof(fd_mask))+1, m1ptr = (m1)->fds_bits,  \
				m2ptr = (m2)->fds_bits; --i; )	\
	    *m1ptr++ &= *m2ptr++;				\
}


#define FD_NAND( m1, m2 )					\
{	fd_mask *m1ptr, *m2ptr;					\
	int i;							\
								\
	for( i = (sizeof(fd_set)/sizeof(fd_mask))+1, m1ptr = (m1)->fds_bits,  \
				m2ptr = (m2)->fds_bits; --i; )	\
	    *m1ptr++ &= ~(*m2ptr++);				\
}


/* copys m2 to m1, setting flag if any bits are on */
#define FD_COPY_AND_TEST( m1, m2, flag )			\
{	fd_mask *m1ptr, *m2ptr;					\
	int i;							\
								\
	(flag) = FALSE;						\
	for( i = (sizeof(fd_set)/sizeof(fd_mask))+1, m1ptr = (m1)->fds_bits,  \
				m2ptr = (m2)->fds_bits; --i; )	\
	    if( *m1ptr++ = *m2ptr++ )				\
		(flag) = TRUE;					\
}


