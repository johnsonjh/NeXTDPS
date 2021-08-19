#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    list.c

    This file has the routines to manage linked lists of things.
    Items in the linked list should have a _DPSListItem as the first
    element of their struct.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"


/* either mallocs a new item or gets one off the free list, and puts in
   in the front of the list.  Its important to put new items at the
   front of the list so people looping through the list arent
   affected.  Returns new item.
*/
void *_DPSNewListItem(_DPSList *list, int size, NXZone *zone)
{
    _DPSListItem *new;

    if( list->free ) {		/* try to get one from the free list */
	new = list->free;
	list->free = new->next;
    } else
	new = NXZoneMalloc(zone ? zone : NXDefaultMallocZone(), size);
    new->next = list->head;
    list->head = new;
    return new;
}


/* adds a new item to the front of the list.  Its important to put
   new items at the front of the list so people looping through the
   list arent affected.  Returns new item.
*/
void _DPSAddListItem(_DPSList *list, _DPSListItem *newItem)
{
    newItem->next = list->head;
    list->head = newItem;
}


/* returns element with matching key, or NULL if not found */
void *_DPSFindListItemByKey(_DPSList *list, int key)
{
    _DPSListItem *curr;
    
    for( curr = list->head; curr; curr = curr->next )
	if( curr->key == key )
	    break;
    return curr;
}


/* returns element with matching address, or NULL if not found */
void *_DPSFindListItemByItem(_DPSList *list, void *itemArg)
{
    _DPSListItem *curr;
    _DPSListItem *item = itemArg;
    
    for( curr = list->head; curr; curr = curr->next )
	if( curr == item )
	    break;
    return curr;
}


/* removes item from list.  Returns whether item was found.  Optionally put
   the item on the free list for later use.  The free list only has a
   size of one item.
 */
int _DPSRemoveListItem(_DPSList *list, void *item, int save)
{
    _DPSListItem **currPtr;
    _DPSListItem *curr;

    for( currPtr = &(list->head); *currPtr; currPtr = &((*currPtr)->next) )
	if( *currPtr == item )
	    break;
    if( *currPtr ) {
	curr = *currPtr;
	*currPtr = curr->next;		/* remove him from this list */
	if( save ) {
	    if( !list->free ) {		/* if empty free list */
		curr->next = NULL;	/* add him to the free list */
		list->free = curr;
	    } else
		FREE( curr );
	}
	return TRUE;
    }
	return FALSE;
}



