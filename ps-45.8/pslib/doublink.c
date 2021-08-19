/*
  doublink.c

Copyright (c) 1985, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Tue Feb 26 10:51:25 1985
Edit History:
Ed Taft: Sun May  1 17:17:14 1988
Ivor Durham: Sun Feb  7 16:36:07 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include EXCEPT
#include PSLIB

/* Linked list implementation */

procedure InitLink(link)
  Links *link;
  {
  link->prev = link->next = link;
  }

procedure RemoveLink(link)
  register Links *link;
  {
  Assert(link->next != 0);
  link->prev->next = link->next;
  link->next->prev = link->prev;
  link->prev = 0;
  link->next = 0;
  }

procedure InsertLink(where, link)
  register Links *where, *link;
  {
  Assert(link->next == 0);
  link->next = where->next;
  link->next->prev = link;
  link->prev = where;
  where->next = link;
  }
