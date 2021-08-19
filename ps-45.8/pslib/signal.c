/*
  signal.c

Copyright (c) 1984, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

This module is derived from a C library module provided to Adobe
under a Unix source license from AT&T Bell Laboratories, U. C. Berkeley,
and/or Sun Microsystems. Under the terms of the source license,
this module cannot be further redistributed in source form.

Original version:
Edit History:
Ed Taft: Sat Sep 30 14:56:09 1989
End Edit History.

Implementation of "signal" for standalone configurations
*/

#include PACKAGE_SPECS
#include <signal.h>
#include <errno.h>

/* Compatibility hack for change in signal return type (grumble grumble) */
#ifdef __STDC__
typedef void signal_ret_type;	/* ANSI: always void */
#else
#ifdef _sys_signal_h
typedef void signal_ret_type;	/* SunOS 4.0: void */
#else
typedef int signal_ret_type;	/* SunOS 3.5 and non-Sun PCC: int */
#endif
#endif

typedef signal_ret_type (*sigtype)();

sigtype _sigfunc[NSIG];	/* functions to be executed on signals */

int	errno;

sigtype 
signal(sig, func)
	register sigtype func;
{
	register sigtype old;

	if (sig < 0 || sig >= NSIG)
	{
		errno = EINVAL;
		return((sigtype)-1);
	}
	old = _sigfunc[sig];
	_sigfunc[sig] = func;
	return(old);
}

signal_ret_type nullsig()
{
  return;
}

signal_ret_type unknownsig()
{
  CantHappen();
}

os_SigFunc(signum)
  int signum;
{
  (void) (*(_sigfunc[signum]))(signum);
}

os_signalinit()
{
  int i;
  for (i=0; i<NSIG; i++)
    _sigfunc[i] = unknownsig;
  _sigfunc[SIGINT] = nullsig;
  _sigfunc[SIGFPE] = nullsig;
}
