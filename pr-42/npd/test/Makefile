#
# Makefile for npd test programs.
#
# Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
#

CFLAGS = -g -O

PROGRAMS = visual_alert audio_alert prdb pserrtest postevent

all:	$(PROGRAMS)

visual_alert: visual_alert.o ../mach_ipc.o
	cc -o visual_alert visual_alert.o ../mach_ipc.o

audio_alert: audio_alert.o ../mach_ipc.o
	cc -o audio_alert audio_alert.o ../mach_ipc.o

prdb: prdb.o
	cc -o prdb prdb.o

pserrtest: pserrtest.o
	cc -o pserrtest pserrtest.o -lNeXT_s -lsys_s

postevent: postevent.o
	cc -o postevent postevent.o -lNeXT_s -lsys_s

clean:
	rm -f *.o $(PROGRAMS)
	rm -f #*# *~

