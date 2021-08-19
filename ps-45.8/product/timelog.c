/*
	timelog.c
	Code to keep timing profiles of events
	
	CONFIDENTIAL
	Copyright (c) 1988 NeXT, Inc. as an unpublished work.
	All Rights Reserved.

	Created: Leo 01Dec86
	Modified:
	
	10Dec86	Leo	Add InitTimedEvents
	
	timelog.c enables up to five events to be timed
	relative to each other.  This five events should
	each call their appropriate TimedEventN routine.
	These routines record the time at which they
	were called, for the first TIMEBUFLENGTH events.
	If WRAPEVENTS is true, then subsequent events will
	overwrite the initial ones.  Otherwise, subsequent
	events will be ignored.  The contents of the timing
	buffer can be printed with PrintTimedEvents. 

	Modified:
        26May90 Terry	Reduced code+data size from 488+1220 to 324+8
*/

#import <sys/time.h>
#import PACKAGE_SPECS
#import STREAM
#import PUBLICTYPES
#import POSTSCRIPT
#import "timelog.h"

#define TIMEBUFLENGTH	20
#define TIMEBUFCOLUMNS 5 /* Warning: this cannot be changed without also */
			/* editing the code */
	
#ifndef WRAPEVENTS
#define WRAPEVENTS 1
#endif WRAPEVENTS

/* This whole file is a big nop if TIMELOG in timelog.h is 0 */

#if TIMELOG

typedef struct timelog {
	struct timeval time;
	GenericID context;
} TimeLog;

static TimeLog (*timeLog)[TIMEBUFLENGTH];
static int *timeLogIndices;

void TimedEvent(int column)
{
    int index;
    TimeLog *t;
    
    if (timeLog && column<TIMEBUFCOLUMNS && 
        (index=timeLogIndices[column])<TIMEBUFLENGTH) 
    {
        t = &timeLog[column][index];
	gettimeofday(&t->time,NULL);
	if (currentPSContext)
		t->context = PSContextToID(currentPSContext);
	else
		t->context.stamp = 0;
#if WRAPEVENTS
	if (++index == TIMEBUFLENGTH) index = 0;
#endif WRAPEVENTS
	timeLogIndices[column] = index;
    }
}

void InitTimedEvents()
{
    if (timeLog) {
        free(timeLog);
	free(timeLogIndices);
    }
    timeLog = (TimeLog (*)[TIMEBUFLENGTH])
	    calloc(TIMEBUFCOLUMNS*TIMEBUFLENGTH*sizeof(TimeLog),1);
    timeLogIndices = (int *) calloc(TIMEBUFCOLUMNS*sizeof(int),1);
}

void PrintTimedEvents()
{
    int	i, j;

    for(i=0;i<TIMEBUFLENGTH;i++) {
	for(j=0;j<5;j++) {
	    int	secs,usecs,index;
	    
	    secs = timeLog[j][i].time.tv_sec;
	    usecs = timeLog[j][i].time.tv_usec;
	    index = timeLog[j][i].context.id.index;
	    secs = secs % 1000;
	    printf("%3d.%06d(%4d) ",secs,usecs,index);
	}
	printf("\n");
    }
}

#endif TIMELOG
