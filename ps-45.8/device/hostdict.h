/*
  hostdict.h

Copyright (c) 1988 Adobe Systems Incorporated.
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
Ivor Durham: Wed Sep 28 13:52:55 1988
End Edit History.
*/

#ifndef HOSTDICT_H
#define HOSTDICT_H

typedef struct _t_HostDictEntry {
  char   *key;
  char   *value;
  long    time;
  struct _t_HostDictEntry *next;
} HostDictEntry;

typedef struct _t_HostDict {
  int     length;
  int     items;
  int     strings;
  int     copy;
  void    (*purge) ();
  HostDictEntry *entries[1];
} HostDict;

HostDict *newHostDict(/* int, int, int, void (*)() */);
void disposeHostDict(/* HostDict * */);

void defHostDict(/* HostDict *, char *, char * */);
char *getHostDict(/* HostDict *, char * */);

void enumerateHostDict(/* HostDict *, void (*)() */);

#endif	HOSTDICT_H
