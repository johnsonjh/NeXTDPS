/*
  foreground.h

	Copyright (c) 1989 Adobe Systems Incorporated.
			All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is stricly forbidden unless prior written
permission is obtained from Adobe.

      PostScript is a registered trademark of Adobe Systems Incorporated.
	Display PostScript is a trademark of Adobe Systems Incorporated.

This interface provides support for a PostScript implementation that has
two threads of control: a low priority 'background' thread in which the
PostScript interpreter runs; and a high priority 'foreground' thread in
which the output page scheduler controls the output device and schedules
and processes pages to be put out. The foreground thread originates in the
timer interrupt handler. One of the tasks of the foreground thread might be
to produce a framebuffer from a display list.

Tasks for the foreground thread get added by the background task to a data
structure shared by both threads. One such entry might identify the next
page to print. The details of this data structure are not described here.

NOTE: The design described below would be very different if the PostScript
software were built on a true multi-tasking kernel.
  
Original version: Paul Rovner (Thu Nov 16 15:49:40 1989)
Edit History:
Paul Rovner: Tue Dec 19 11:45:12 1989
End Edit History.

*/

#ifndef	FOREGROUND_H
#define	FOREGROUND_H

#include PUBLICTYPES

/*
  The procedures defined below manage a single monitor that provides
  mutually exclusive access by the two threads to shared data. A
  counter associated with the monitor supports nested entry to it.
 
  At any given time the foreground thread is either running or it is
  waiting either for the background thread to exit the monitor or for
  the next timer interrupt.

  The background thread is either running, or it is waiting to enter
  the monitor, or it is suspended, having been pre-empted by the
  timer-interrupt-driven foreground thread.
  
  When the foreground thread gets control from the interrupt handler,
  if the background thread is inside the monitor, the foreground thread
  either waits for the monitor to become free if the foreground thread's
  task list is non-empty, or waits for the next interrupt otherwise. If
  the monitor is free, the foreground thread enters it, performs some or
  all of its tasks, if any, then exits the monitor, and finally waits for
  the next interrupt. NOTE: technically, it is not necessary for the
  foreground thread to enter the monitor because the background thread
  remains suspended while the foreground thread performs its tasks. The
  background thread resumes as soon as the foreground thread waits.

  If the foreground thread is waiting to enter the monitor when the
  background thread exits the monitor, the background thread wakens the
  foreground thread.
  
  NOTE: only one thread will be running at any time.
  
 */

#if (OS == os_mach)

#define FGEnterMonitor()
#define FGExitMonitor()

#else

extern procedure FGInitialize(/* void (*proc)(); long int interval; */);
/* 
  Call FGInitialize when the first hybrid device is created.
 
  This establishes the foreground thread as a timer-driven
  interrupt handler with a period of 'interval' milliseconds.
 
  Whenever a wakeup occurs,
    if the monitor is free,
       the foreground thread enters the monitor, calls proc, then exits
       the monitor.
    if the monitor is not free, the foreground thread as marked as waiting
    to enter the monitor.
 */

extern procedure FGEnterMonitor();
/*
  If the current thread is already inside the monitor, this returns.
  Otherwise, this enters and then returns.  
  
  When this is called, either the current thread is inside the monitor
  or the current thread is the background thread and the monitor is free.
  
  Every call on FGEnterMonitor must be followed sometime later by a
  matching call on FGExitMonitor.
 */

extern procedure FGExitMonitor();
/* 
 Exit the monitor if this call matches the call that entered it.
 If the foreground thread is waiting to enter the monitor and this call
 exits the monitor then this invokes FGAwaken after exiting the monitor.
 */

extern procedure FGIdleBackground();
/* 
 To be called only from the background thread from within the monitor.

 This forces release of the monitor, then invokes FGAwaken, then restores
 the background threead's control of the monitor.
 
 This provides a way for the background thread to await completion of
 a crucial foreground task, for example freeing up display list buffer
 space, when it can exit the monitor temporarily.
 */

extern procedure FGAwaken();
/* 
 Noop if the foreground thread is running, otherwise this causes the
 foreground thread to run immediately. Note that if the background thread
 is inside the monitor, the foreground thread will not make progress but
 instead will be marked as waiting to enter the monitor.
 */
#endif

#endif	FOREGROUND_H
