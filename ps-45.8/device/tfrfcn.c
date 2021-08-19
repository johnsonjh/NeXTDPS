/* implementation of Dev* routines dealing with transfer functions

Copyright (c) 1988, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Jim Sandman: Tue Apr  4 13:02:44 1989
Paul Rovner: Wed Nov 29 10:41:37 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICE
#include EXCEPT
#include PSLIB
#include FOREGROUND

private Pool tfrfcnPool;

#define GETTABLE(n) \
  (unsigned char *)os_malloc((long int)((n) * sizeof(unsigned char)))

#define GETUCRTABLE(n) \
  (DevShort *)os_malloc((long int)((n) * sizeof(DevShort)))

public DevTfrFcn *DevAllocTfrFcn (white, red, green, blue, ucr, bg)
  boolean white, red, green, blue, ucr, bg; {
  DevTfrFcn *t;
  boolean failure = false;

  FGEnterMonitor();
  DURING
    if (tfrfcnPool == NULL)
      tfrfcnPool = os_newpool(sizeof(DevTfrFcn), 3, 0);
    t = (DevTfrFcn *)os_newelement(tfrfcnPool);
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();

  os_bzero((char *)t, (long int)sizeof(DevTfrFcn));
  if (white) {
    if ((t->white = GETTABLE(256)) == NULL)
      failure |= true;
    }
  if (red) {
    if ((t->red = GETTABLE(256)) == NULL)
      failure |= true;
    }
  if (green) {
    if ((t->green = GETTABLE(256)) == NULL)
      failure |= true;
    }
  if (blue) {
    if ((t->blue = GETTABLE(256)) == NULL)
      failure |= true;
    }
  if (ucr) {
    if ((t->ucr = GETUCRTABLE(256)) == NULL)
      failure |= true;
    }
  if (bg) {
    if ((t->bg = GETTABLE(256)) == NULL)
      failure |= true;
    }
  if (failure) {
    DevFreeTfrFcn(t); t = NULL; }

  return t;
  }

private procedure ReclaimDevTfrFcn (t) DevTfrFcn *t; {
  if (t->white != NULL) os_free((char *)t->white);
  if (t->red != NULL) os_free((char *)t->red);
  if (t->green != NULL) os_free((char *)t->green);
  if (t->blue != NULL) os_free((char *)t->blue);
  if (t->ucr != NULL) os_free((char *)t->ucr);
  if (t->bg != NULL) os_free((char *)t->bg);
  os_freeelement(tfrfcnPool, (char *)t);
  }

public procedure DevFreeTfrFcn (t) DevTfrFcn *t; {
  FGEnterMonitor();
  if (t->priv == (DevPrivate *)0)
    ReclaimDevTfrFcn(t);
  else
    t->priv = (DevPrivate *)(-((integer)(t->priv)));
  FGExitMonitor();
  }

public procedure DevAddTfrFcnRef(t) DevTfrFcn *t; {
  if (t == NIL) return;
  FGEnterMonitor();
  if (((integer)(t->priv)) >= 0)
    t->priv = (DevPrivate *)(((integer)(t->priv)) + 1);
  else
    t->priv = (DevPrivate *)(((integer)(t->priv)) - 1);
  FGExitMonitor();
  }

public procedure DevRemTfrFcnRef(t) DevTfrFcn *t; {
  if (t == NIL) return;
  FGEnterMonitor();
  Assert(((integer)(t->priv)) != 0);
  if (((integer)(t->priv)) > 0)
    t->priv = (DevPrivate *)(((integer)(t->priv)) - 1);
  else {
    t->priv = (DevPrivate *)(((integer)(t->priv)) + 1);
    if (((integer)(t->priv)) == 0) ReclaimDevTfrFcn(t);
    }
  FGExitMonitor();
  }

