/*
  arguments.c

Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ivor Durham: November 11, 1986
Edit History:
Ivor Durham: Wed Aug 24 16:25:25 1988
Linda Gass: Wed Jul  8 14:48:11 1987
Ed Taft: Tue Nov 28 16:09:31 1989
Paul Rovner: Friday, October 30, 1987 5:02:39 PM
Jim Sandman: Wed Mar  9 21:33:37 1988
Joe Pasqua: Tue Jan 17 13:03:19 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include EXCEPT
#include ORPHANS
#include PSLIB
#include STREAM

private int savedArgc;
private char **savedArgv;
private char *argsUsed;		/* boolean array parallel to argv */

public procedure BeginParseArguments (argc, argv)
  int argc;
  char *argv[];
{
  if (argsUsed != NIL || /* called BeginParseArgument multiple times */
      savedArgc > 0)	/* attempted to get arg after EndParseArguments */
    CantHappen ();

  savedArgc = argc;
  savedArgv = argv;
  argsUsed = os_sureCalloc((long int) argc + 1, 1);
}

public boolean EndParseArguments()
{
  register int argc;
  register char *used;
  boolean seen = false;

  if (argsUsed == NIL) return false;

  for (argc = savedArgc, used = argsUsed + 1;
       --argc > 0; )
    if (*used++ == 0)
      {
      if (! seen) os_eprintf("Warning: Unused command line arguments:");
      seen = true;
      os_eprintf(" %s", savedArgv[savedArgc - argc]);
      }

  if (seen) os_eprintf("\n");
  return seen;
}

public char *GetCArg(option)
  char option;
 /*
   If the specified option was present on the command line, returns
   that option's argument, otherwise returns NIL.
  */
{
  register int argc;
  register char **argv;
  register char *arg;

  if (argsUsed == NIL)
    BeginParseArguments (0, (char *) NIL);

  for (argc = savedArgc, argv = savedArgv + 1;
       --argc > 0; argv++)
    if ((arg = *argv) != NIL && arg[0] == '-')
      if (arg[1] == option)
        {
	argc = savedArgc - argc;
	argsUsed[argc] = 1;
	argsUsed[argc+1] = 1;
	return *++argv;
	}
      else
        {argc--; argv++;}

  return NIL;
}

private integer NumFromStr(str, radix, deflt)
  char *str; integer radix, deflt;
{
  register char *s = str;
  register integer x = 0;
  register integer d;

  if (s == NIL || *s == NUL)
    return deflt;

  while ((d = *s++) != NUL) {
    if (d >= '0' && d <= '9')
      d -= '0';
    else if (d >= 'A' && d <= 'F')
      d -= 'A' - 10;
    else if (d >= 'a' && d <= 'f')
      d -= 'a' - 10;
    else
      goto error;

    if (d >= radix)
      goto error;
    x = x * radix + d;
  }
  return x;

error:
  os_eprintf ("Invalid number \"%s\"\n", str);
  return deflt;
}

public integer NumCArg(option, radix, deflt)
  char option; integer radix, deflt;
 /*
   Converts a numeric option from the command line according to radix,
   and returns the result.  If the option is not present, returns deflt.
   If an error occurs, prints an error message and returns deflt.
  */
{
  return NumFromStr (GetCArg (option), radix, deflt);
}

public integer GetCSwitch(name, deflt)
  char *name; integer deflt;
{
  register int argc;
  register char **argv;
  register char *arg, *p1, *p2;

  if (argsUsed == NIL)
    BeginParseArguments (0, (char *) NIL);

  for (argc = savedArgc, argv = savedArgv + 1;
       --argc > 0; argv++)
    {
    if ((arg = *argv) != NIL)
      if (arg[0] == '-')
	{argc--; argv++;}
      else
        {
	for (p1 = arg, p2 = name; *p2 != '\0'; )
	  if (*p1++ != *p2++) goto mismatch;
	switch (*p1++)
	  {
	  case '=':
	    argsUsed[savedArgc - argc] = 1;
	    return NumFromStr(p1, (integer) 10, (integer) 0);
	  case '\0':
	    argsUsed[savedArgc - argc] = 1;
	    return 1;
	  }
	}
    mismatch: ;
    }

  return deflt;
}

