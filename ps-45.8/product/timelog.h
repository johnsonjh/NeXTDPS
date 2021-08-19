/*
	timelog.h
	defines the procedures used to keep timing logs
	of various things happening in the PostScript
	server.  These routines are defined in timelog.c
	and the operators that call them in event.c;
	this header is used to control their inclusion.
	
	Created 25Sep88 Leovitch
	
	Modified:
*/

#if STAGE == EXPORT
#define TIMELOG 0
#endif

#ifndef TIMELOG
#define TIMELOG 1	/* Define this to 1 to insert timing, 0 to remove */
#endif

#if TIMELOG

extern void TimedEvent();
extern void InitTimedEvents();
extern void PrintTimedEvents();

#else

#define TimedEvent(n)
#define InitEventTimes()
#define PrintEventTimes()

#endif

