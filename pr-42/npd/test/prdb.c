/*
 * prdb.c	- A test of the printer database library calls.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 *
 * This program will print out the entire printer database or will print
 * out the database entries for the printers listed on the command line.
 */


/*
 * Include files.
 */

#import <stdio.h>
#import <printerdb.h>


/*
 * Constants
 */
#define	SCREENWIDTH	80		/* A hack, but this is a test prgm */


/*
 * Routines.
 */

/**********************************************************************
 * Routine:	printEntry() - print out a printerdb entry
 *
 * Function:	This prints out a printerdb entry.  It trys to do some
 *		minimal formatting of the properties so that can all be
 *		easily viewed.
 **********************************************************************/
void
printEntry(const prdb_ent *pe)
{
    char 		**cpp;		/* generic char pointer pointer */
    int			col;		/* The last column writen to */
    int			i;		/* generic counter */
    prdb_property	*pp;		/* pointer to printer property */

    /* Print the name */
    printf("\"%s\":", pe->pe_name[0]);
    if (pe->pe_name[1]) {
	printf(" aliases = ");
	for(cpp = pe->pe_name + 1; *cpp; cpp++) {
	    if (*(cpp+1)) {
		printf("\"%s\", ", *cpp);
	    } else {
		printf("\"%s\"", *cpp);
	    }
	}
    }

    /* Print out the properties in a minimally formated way */
    col = SCREENWIDTH;
    for (pp = pe->pe_prop, i = 0; i < pe->pe_nprops; i++, pp++) {
	/* Check that this will fit on this line */
	if (col + (strlen(pp->pp_key) + strlen(pp->pp_value) +
	    3 /* equal sign and two quote marks */) > SCREENWIDTH) {
	    printf("\n\t");
	    col = 8;			/* The size of a tab */
	}
	printf("%s=\"%s\"", pp->pp_key, pp->pp_value);
	col += strlen(pp->pp_key) + strlen(pp->pp_value) + 3;

	if (i < (pe->pe_nprops - 1) && col != SCREENWIDTH) {
	    printf(" ");
	    col++;
	}
    }

    /*  Skip a couple of lines */
    printf("\n\n");
}    
	

/**********************************************************************
 * Routine:	main() - The main program.
 **********************************************************************/
main(int argc, char **argv)
{
    int			i;		/* generic counter */
    const prdb_ent	*pe;		/* printerdb entry */

    if (argc > 1) {
	for (i = 1; i < argc; i++) {
	    if (pe = prdb_getbyname(argv[i])) {
		printEntry(pe);
	    } else {
		fprintf(stderr, "%s: unknown printer\n", argv[i]);
	    }
	}
    } else {
	/* Print all the printer entries */
	prdb_set(NULL);
	while (pe = prdb_get()) {
	    printEntry(pe);
	}
	prdb_end();
    }
}
