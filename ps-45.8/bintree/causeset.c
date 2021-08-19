/*****************************************************************************

    causeset.c
    Maintains lists of causes for divpiece divisions

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 03Jul86 Leo

    Modified:

    16Dec87 Leo   Made into routines
    17Jan88 Jack  Fix reversed args to bcopy in CSExpand
    17Feb88 Both  ...and in CSCopy (sheepishly)!
    16May89 Ted   ANSI C funtion prototypes.

******************************************************************************/

#import PACKAGE_SPECS
#import "bintreetypes.h"

#define CSGROWINC 2		/* Amount to expand by when full */
static char *csPool;		/* Blind ptr to pool for CauseSets */
static char *csCausePool;	/* Blind ptr to pool for initial causes */

/* Forward Declarations */
CauseSet *CSExpand(CauseSet *);
int CSContains(CauseSet *,int);	/* Externalize when useful */


/*****************************************************************************
    CSInitialize
    Initialize two cause set pools.
******************************************************************************/
void CSInitialize()
{
    csPool = (char *) os_newpool(sizeof(CauseSet), 0, 0);
    csCausePool = (char *) os_newpool(CSGROWINC*sizeof(int), 0, 0);
}


/*****************************************************************************
    CSNew
    Create a new cause set.
******************************************************************************/
CauseSet *CSNew()
{
    CauseSet *cs;
    
    cs = (CauseSet *) os_newelement(csPool);
    bzero(cs, sizeof(CauseSet));
    return CSExpand(cs);
}


/*****************************************************************************
    CSAdd
    Add a cause to the given cause set.
******************************************************************************/
CauseSet *CSAdd(CauseSet *cs, int newCause)
{
    int i;
    
    if (newCause == NOREASON)
	return cs;
    for (i=0; i<cs->length; i++)
	if (cs->causes[i] == newCause)
	    return cs;
    if (cs->length>=cs->capacity)
	cs = CSExpand(cs);
    cs->causes[cs->length] = newCause;
    cs->length++;
    return cs;
}


/*****************************************************************************
    CSAddSet
    Return the union of the cause sets.
******************************************************************************/
CauseSet *CSAddSet(CauseSet *cs, CauseSet *ocs)
{
    int i, j, origLength, addIt;

    /* First, make room for the new causes. */
    while (cs->capacity<(cs->length + ocs->length))
	cs = CSExpand(cs);
    /* Now, for each of the new causes, check to
       see if it was in the original set. */
    origLength = cs->length;
    for (i=0; i<ocs->length; i++) {
	addIt = 1;
	for (j=0; j<origLength; j++)
	    if (cs->causes[j] == ocs->causes[i]) {
		addIt = 0;
		break;
	    }
	if (addIt)
	    cs->causes[cs->length++] = ocs->causes[i];
    }
    return cs;
}


/*****************************************************************************
    CSContains
    Return boolean telling whether the cause is in the given cause set.
******************************************************************************/
/* Remove the #if..#endif if this routine is ever needed */
#if 0
static int CSContains(CauseSet *cs, int theCause)
{
    int i;

    for  (i=0; i<cs->length; i++)
	if (cs->causes[i] == theCause)
	    return true;
    return false;
}
#endif


/*****************************************************************************
    CSCopy
    Return a duplicate of the given cause set. Called from divpiece.c.
******************************************************************************/
CauseSet *CSCopy(CauseSet *ocs)
{
    CauseSet *cs;
    
    cs = CSNew();
    while (cs->capacity<ocs->length)
	cs = CSExpand(cs);
    bcopy(ocs->causes, cs->causes, ocs->length*sizeof(int));
    cs->length = ocs->length;
    return cs;
}


/*****************************************************************************
    CSExpand
    Expand the capacity of the given cause set.
******************************************************************************/
static CauseSet *CSExpand(CauseSet *cs)
{
    int *newCauses;
    
    if (cs->capacity)
	newCauses = (int *) malloc((cs->capacity+CSGROWINC)*sizeof(int));
    else
	newCauses = (int *) os_newelement(csCausePool);
    if (cs->length)
	bcopy(cs->causes, newCauses, cs->length*sizeof(int));
    if (cs->causes)
	if (cs->capacity == CSGROWINC)
	    os_freeelement(csCausePool, cs->causes);
	else
	    free(cs->causes);
    cs->capacity += CSGROWINC;
    cs->causes = newCauses;
    return cs;
}


/*****************************************************************************
    CSFree
    Free a cause set.
******************************************************************************/
void CSFree(CauseSet *cs)
{
    if (cs->capacity == CSGROWINC)
	os_freeelement(csCausePool, cs->causes);
    else
	free(cs->causes);
    os_freeelement(csPool, cs);
}


/*****************************************************************************
    CSPrintOn
    Print debugging info.
******************************************************************************/
void CSPrintOn(CauseSet *cs)
{
    int i, c;
    
    for (i=0; i<cs->length; i++) {
	if (i>0) os_fprintf(os_stdout, ",");
	c = STRIPCONVERTCAUSE(cs->causes[i]);
	os_fprintf(os_stdout, "%d", c);
    }
}


/*****************************************************************************
    CSRemove
    Remove the cause from the given cause set.
******************************************************************************/
int CSRemove(CauseSet *cs, int oldCause)
{
    int i, j;

    if (oldCause != NOREASON)
	for (i=0; i<cs->length; i++)
	    if (cs->causes[i] == oldCause) {
		for ( ; i<(cs->length-1); i++)
		    cs->causes[i] = cs->causes[i+1];
		cs->length--;
		return cs->length;
	    }
    return cs->length;
}


/*****************************************************************************
    CSSwapFor
    Exchange newCause for cause oldCause in the given cause set.
******************************************************************************/
void CSSwapFor(CauseSet *cs, int newCause, int oldCause)
{
    int	i;
    
    /* We should check to see that newCause isn't in the set already */
    for (i=0; i<cs->length; i++)
	if (cs->causes[i] == oldCause) {
	    cs->causes[i] = newCause;
	    return;
	}
}



