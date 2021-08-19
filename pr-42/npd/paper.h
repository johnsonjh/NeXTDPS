/*
 * paper.h	- Interface file for paper information routines.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 *
 * The paperinfo structure defines the imageable area and size of a
 * particular paper type.  The actual size of the paper can be calculated
 * as the imageable area plus 2*margins.
 */


/*
 * Type definitions.
 */
typedef struct paperinfo {
    const char	*name;
    int		imageWidth;
    int		imageHeight;
    int		paperWidth;
    int		paperheight;
} *paperinfo_t;


/*
 * External routines.
 */

/* Lookup paper information based on name */
extern paperinfo_t	PaperLookup(const char *name);

