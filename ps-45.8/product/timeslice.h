
/*
	timeslice.h
	There are two schemes for implementing the timeslicing
	used in faking up multiple contexts in the server:
	if there is some low-level routine in the server that
	is asynchronously called in the mousekeyboard package,
	it is asynchronous timeslicing; if it has to be done
	entirely in the main interpreter loop, it is synchronous.
	
	Asynchronous timeslicing should be done by having the
	asynchronous routine call CheckTimeslice every time
	it is invoked, and call CheckReschedule in the interpreter
	loop.
	
	Synchronous timeslicing should call SyncCheckReschedule
	every time around the main event loop.
	
	Both of these methods use the eventGlobals->VertRetraceClock
	provided by the MOUSEKEYBOARD interface to implement
	the timeslicing.
	*/

#ifndef TIMESLICE_H
#define TIMESLICE_H

/* List the OSes under which timeslicing is synchronous */
#define SYNCHRONOUS_TIMESLICE (OS == os_mach)

#if SYNCHRONOUS_TIMESLICE
extern procedure SyncCheckReschedule();
#else SYNCHRONOUS_TIMESLICE
extern procedure CheckTimeslice();
extern procedure CheckReschedule();
#endif SYNCHRONOUS_TIMESLICE

/*
	The TIMESLICE macro gives the number of VertRetraceClocks
	of time to allot one PostScript context before timeslicing
	it.
	*/
#if (OS == os_mach)
#define TIMESLICE 20
#endif (OS == os_mach)
#if (OS == os_sun)
#define TIMESLICE 10
#endif (OS == os_sun)

#endif TIMESLICE_H
