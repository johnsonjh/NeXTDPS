/* implementation of Dev* routines dealing with halftone

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
Jim Sandman: Tue Apr  4 13:03:06 1989
Paul Rovner: Tue Nov 21 17:07:28 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICE
#include EXCEPT
#include DEVPATTERN
#include FOREGROUND

#include "devcommon.h"

public boolean DevIndependentColors () {
  boolean ans;
  FGEnterMonitor();
  DURING
    ans = IndependentColors();
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  return ans;
  }

public boolean DevCheckScreenDims (width, height) DevShort width, height; {
  boolean ans;
  FGEnterMonitor();
  DURING
    ans = CheckScreenDims(width, height);
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  return ans;
  }

public procedure DevFreeHalftone(h) DevHalftone *h; {
  if (h == NIL) return;
  FGEnterMonitor();
  DURING
    if (h->priv == (DevPrivate *)0)
      FlushHalftone(h);
    else
      h->priv = (DevPrivate *)(-((integer)(h->priv)));
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  }

public procedure DevAddHalftoneRef(h) DevHalftone *h; {
  if (h == NIL) return;
  FGEnterMonitor();
  if (((integer)(h->priv)) >= 0)
    h->priv = (DevPrivate *)(((integer)(h->priv)) + 1);
  else
    h->priv = (DevPrivate *)(((integer)(h->priv)) - 1);
  FGExitMonitor();
  }

public procedure DevRemHalftoneRef(h) DevHalftone *h; {
  if (h == NIL) return;
  FGEnterMonitor();
  DURING
    Assert(((integer)(h->priv)) != 0);
    if (((integer)(h->priv)) > 0)
      h->priv = (DevPrivate *)(((integer)(h->priv)) - 1);
    else {
      h->priv = (DevPrivate *)(((integer)(h->priv)) + 1);
      if (((integer)(h->priv)) == 0) FlushHalftone(h);
      }
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  }

public DevHalftone *DevAllocHalftone(
  wWhite, hWhite, wRed, hRed, wGreen, hGreen, wBlue, hBlue)
  DevShort wWhite, hWhite, wRed, hRed, wGreen, hGreen, wBlue, hBlue; {
  DevHalftone *h;
  FGEnterMonitor();
  DURING
    while (true) {
      h = AllocHalftone
        (wWhite, hWhite, wRed, hRed, wGreen, hGreen, wBlue, hBlue);
      if (h != NULL) {
        h->priv = (DevPrivate *)0;
	break;
	}
#if (!SAMPLEDEVICE)
      if (!AwaitPipelineProgress())
#endif
        break;
      }
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  return h;
  }

