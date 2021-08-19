/*
 * paper.c	- Implementation of paper information routines.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 *
 * This module holds information about the paper types specified in
 * the Adobe PostScript Printer Descriptions specification, Version 3.0.
 */

#import <string.h>

#import "paper.h"


/*
 * Constants.
 */

static struct paperinfo paperTypes[] = {
    {
	"letter",
	612, 792,			/* 8.5 X 11 inches */
	612, 792,			/* 8.5 X 11 inches */
    },{
	"lettersmall",
	553, 732,			/* 553 X 731.5 points */
	612, 792,			/* 8.5 X 11 inches */
    },{
	"tabloid",
	792, 1224,			/* 11 X 17 inches */
	792, 1224,			/* 11 X 17 inches */
    },{
	"ledger",
	1224, 792,			/* 17 X 11 inches */
	1224, 792,			/* 17 X 11 inches */
    },{
	"legal",
	612, 1008,			/* 11 X 17 inches */
	612, 1008,			/* 11 X 17 inches */
    },{
	"statement",
	396, 612,			/* 5.5 X 8.5 inches */
	396, 612,			/* 5.5 X 8.5 inches */
    },{
	"executive",
	540, 720,			/* 7.5 X 10 inches */
	540, 720,			/* 7.5 X 10 inches */
    },{
	"a3",
	842, 1190,			/* 297 X 420 mm */
	842, 1190,			/* 297 X 420 mm */
    },{
	"a4",
	595, 842,			/* 210 X 297 mm */
	595, 842,			/* 210 X 297 mm */
    },{
	"a4small",
	538, 782,			/* 7.47 X 10.85 inches */
	595, 842,			/* 210 X 297 mm */
    },{
	"a5",
	420, 595,			/* 148 X 210 mm */
	420, 595,			/* 148 X 210 mm */
    },{
	"b4",
	729, 1032,			/* 257 X 364 mm */
	729, 1032,			/* 257 X 364 mm */
    },{
	"b5",
	516, 729,			/* 182 X 257 mm */
	516, 729,			/* 182 X 257 mm */
    },{
	"envelope",
	297, 792,			/* 4.125 X 11 inches */
	297, 792,			/* 4.125 X 11 inches */
    },{
	"folio",
	567, 904,			/* 567 X 903.5 points */
	612, 936,			/* 8.5 X 13 inches */
    },{
	"quarto",
	567, 744,
	610, 780,
    },{
	"10x14",
	720, 1008,			/* 10 X 14 inches */
	720, 1008,			/* 10 X 14 inches */
    },{
	NULL,
	0, 0,
	0, 0,
    }
};


/*
 * Exported routines.
 */
paperinfo_t
PaperLookup(const char *name)
{
    char	*namebuf;
    int		i;			/* generic counter */
    paperinfo_t	pip;			/* dedicated to faster printing */

    /* Convert the paper name to lower case */
    namebuf = (char *) malloc(strlen(name) + 1);
    for (i = 0; name[i]; i++) {
	if (isupper(name[i])) {
	    namebuf[i] = tolower(name[i]);
	} else {
	    namebuf[i] = name[i];
	}
    }

    /* Now look for it in the list */
    for (pip = paperTypes; pip->name; pip++) {
	if (strcmp(pip->name, namebuf) == 0) {
	    free(namebuf);
	    return(pip);
	}
    }

    /* None found */
    free(namebuf);
    return(NULL);
}


	
    
    
	
