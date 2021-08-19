/*
  os_macmalloc.c

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

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include <Errors.h>
#define Fixed MPWFixed
#include DPSMACSANITIZE
#include DPSPRIVATE
#include PSLIB

extern char *malloc(), *realloc();
extern void free();

public integer macAllocatedBytes, macFreedBytes, macReallocates;

private char *inneros_sureMalloc(size) integer size; {
  char *p;
  while (!(p = (char *)malloc(size))) {
    PSOutOfMemory();
    }
  return p;
  }

private char *inneros_malloc(size) integer size; {
  return inneros_sureMalloc(size);
  }

private char *inneros_realloc(ptr, size) char *ptr; integer size; {
  integer *q = (integer *)ptr;
  integer oldSize = 0;
  char *p;

  if (q) {
    q = (integer *)(ptr - sizeof(integer));
    oldSize = *q;
    }
  p = (char *)realloc((char *)q, size + sizeof(integer));
  if (p) {
    q = (integer *)p;
    *q = size;
    macAllocatedBytes += size;
    macFreedBytes += oldSize;
    macReallocates++;
    p += sizeof(integer);
    }
  else PSOutOfMemory();
  return p;
  }

public char *os_malloc(size) integer size; {
  char *p =
    (char *) DPSCall1Sanitized(inneros_malloc, size + sizeof(integer));
  if (p) {
    integer *q = (integer *)p;
    *q = size;
    macAllocatedBytes += size;
    p += sizeof(integer);
    }
  return p;
  }

public char *os_sureMalloc(size) integer size; {
  char *p =
    (char *) DPSCall1Sanitized(inneros_sureMalloc, size + sizeof(integer));
  if (p) {
    integer *q = (integer *)p;
    *q = size;
    macAllocatedBytes += size;
    p += sizeof(integer);
    }
  return p;
  }

public char *os_realloc(ptr, size) char *ptr; integer size; {
  return (char *) DPSCall2Sanitized(inneros_realloc, ptr, size);
  }

public procedure os_free (ptr) char *ptr; {
  integer *q = (integer *)(ptr - sizeof(integer));
  macFreedBytes += *q;
  (void) DPSCall1Sanitized(free, (char *)q);
  }

