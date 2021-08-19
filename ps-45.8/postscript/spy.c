/*
  spy.c

Copyright (c) 1984, '87, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Sat Dec 15 14:05:28 1984
Edit History:
Ed Taft: Tue Oct 31 10:58:44 1989
Doug Brotz: Wed Jun  5 10:34:16 1985
End Edit History.

This program implements dynamic performance measurement by PC sampling.
It was implemented initially as a downloaded program, but is now built-in
because the download machinery is defunct. The spy facility is accessed
via two internaldict operators, startspy and stopspy.

  addresses histogram  startspy  --

The startspy operator takes two arrays and starts PC sampling. The
addresses array should contain integers which are the addresses of
interesting points in the code to be sampled (e.g., the starts of
procedures). These should be sorted in ascending order with no duplicates
and should be bracketed with "zero" and "infinity" addresses.

The histogram array must be the same size as the addresses array.
Startspy initializes it to all zeroes. Subsequently, during each
PC sample, the sampling routine increments the histogram entry
corresponding to the greatest address less than or equal to the
PC sampled.

Note: it is dangerous for the PostScript program to store into
the addresses or histogram array while sampling is active.

  --  stopspy  --

The stopspy operator simply ends PC sampling. The caller may then
examine the results in the histogram.

The implementation of the Spy is in three parts: a PostScript program,
a C program, and an assembly program. The PostScript program (spyps.ps)
is responsible for setting up the arrays, invoking the startspy and
stopspy operators, and interpreting the results. The C program (spy.c)
implements those operators. The assembly program (spyintsun.s) does the
actual PC sampling and histogram updating; it is the only program that is
in any sense machine-dependent.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include CLOCK
#include LANGUAGE
#include ORPHANS
#include VM

private Timer spyTimer; /* NIL means not spying */
private Level spyLevel;

extern struct {			/* initial register values (see spyint.s) */
  integer l, r;			/* left and right search limits */
  integer stackDisplacement;	/* stack displacement for interrupt PC */
  IntObj *addresses, *histogram; /* pointers to the arrays */
  } spyVars;

extern procedure SpyInterrupt();
extern integer FindDisplacement();
private procedure PSStopSpy();

private procedure PSStartSpy()	/* addresses histogram startspy -- */
  {
  Object ob;
  AryObj addresses, histogram;
  integer i;
  PSStopSpy();
  PopPArray(&histogram);
  PopPArray(&addresses);
  if (addresses.length != histogram.length || addresses.length < 2)
    RangeCheck();
  for (i = 0; i < addresses.length; i++)
    if (VMGetElem(addresses, i).type != intObj) TypeCheck();

  /* Initialize histogram to all zeroes. Doing this here ensures not only
   * that the histogram is properly initialized but also that any
   * required saving of the former values is done. */
  LIntObj(ob, 0);
  for (i = 0; i < histogram.length; i++) VMPutElem(histogram, i, ob);
  spyVars.addresses = addresses.val.arrayval;
  spyVars.histogram = histogram.val.arrayval;

  /* Perform the experiment to find the correct stack displacement
   * for obtaining the interrupt PC during a timer interrupt. */
  if ((spyVars.stackDisplacement = FindDisplacement()) == 0)
    LimitCheck();

  /* All possibility of PostScript-level errors is now past. Now start
   * the PC sampling machinery. */
  spyVars.l = 0;
  spyVars.r = (histogram.length - 1) * sizeof(IntObj);
  spyLevel = LEVEL;
  spyTimer = TimerQ(5, SpyInterrupt);  /* every 5 ms */
  }

private procedure PSStopSpy()	/* -- stopspy -- */
  {
  if (spyTimer) {
    TimerDQ(spyTimer);
    spyTimer = NIL;
    }
  }

private procedure SpyRestore(level)
  Level level;
  {if (level < spyLevel) PSStopSpy();}

public procedure SpyInit()
  {
  Begin(rootShared->vm.Shared.internalDict);
  RgstExplicit("startspy", PSStartSpy);
  RgstExplicit("stopspy", PSStopSpy);
  End();
  spyTimer = NIL;
  RgstRstrProc(SpyRestore);
  }
