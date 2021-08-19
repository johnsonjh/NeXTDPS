/*
  pswdict.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Paul Rovner: Thursday, May 12, 1988 1:30:12 PM
Edit History:
Paul Rovner: Thursday, May 12, 1988 4:02:26 PM
Ivor Durham: Tue May 24 19:51:23 1988
Andrew Shore: Wed Jul 13 16:39:45 1988
Richard Cohn: Mon Sep 26 14:32:36 1988
End Edit History.
*/

#ifndef	PSWDICT_H
#define	PSWDICT_H

typedef struct _t_PSWDictRec *PSWDict;
/* Opaque designator for a dictionary */

typedef int PSWDictValue; /* non-negative */
typedef char *PSWAtom;

/* PROCEDURES */

/* NOTES 
   The name parameters defined below are NULL-terminated C strings.
   None of the name parameters are handed off, i.e. the caller is
   responsible for managing their storage. */

extern PSWDict CreatePSWDict(/* int nEntries */);
/* nEntries is a hint. Creates and returns a new dictionary */

extern void DestroyPSWDict(/* PSWDict dict */);
/* Destroys a dictionary */

extern PSWDictValue PSWDictLookup(/* PSWDict dict; char *name */);
/* -1 => not found. */

extern PSWDictValue PSWDictEnter
  (/* PSWDict dict; char *name; PSWDictValue value; */);
/*  0 => normal return (not found)
   -1 => found. If found, the old value gets replaced with the new one. */

extern PSWDictValue PSWDictRemove(/* PSWDict dict; char *name */);
/* -1 => not found. If found, value is returned. */

extern PSWAtom MakeAtom(/* char *name */);

#endif	PSWDICT_H
