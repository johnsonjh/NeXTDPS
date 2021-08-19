/*
  unix.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Andrew Shore: Fri Nov  4 14:09:38 1983
Edit History:
Chuck Geschke: Thu Oct 10 06:42:41 1985
Doug Brotz: Mon Jun  2 10:34:11 1986
Ed Taft: Fri Nov 24 09:59:12 1989
Andrew Shore: Fri Jun  8 08:48:03 1984
John Gaffney: Tue Jan  8 10:43:33 1985
Ivor Durham: Thu Jun 23 15:46:53 1988
Jim Sandman: Wed Mar  9 22:24:25 1988
Joe Pasqua: Mon Jan 16 14:21:26 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include UNIXSTREAM
#include VM

extern void os_exit();

#if STAGE==DEVELOP
/* this code provides unix outcalls in the style of the Unix-Emacs 
   "fast-filter-region" command */

/* This subroutine takes the FD whose integer number is given by "input",
   and uses it as standard input to the program named in *command[0],
   which is run with an argv of command[0]..command[n], such that
   command[n+1]==0.
   
   Upon successful completion this subroutine returns a pointer to a string
   that names the scratch file in which the output of the process has been
   stored. It is the responsibility of our caller to delete that file when
   he is done with it.
 */

private character *FilterRegion (input, command)
	int input; char **command;
 {
    int     fdpipe[2], fdscratch;
    int     status, n;
    static char tempfile[40];
    char    buf[1000];
    extern  char *mktemp();

    pipe (fdpipe);
#if	!(OS == os_aix || OS == os_xenix)
    if (vfork() == 0) {
#else	!(OS == os_aix || OS == os_xenix)
    if (fork() == 0) {
#endif	!(OS == os_aix || OS == os_xenix)
	close (0);
	close (1);
	close (2);

	dup (input);		/* This sets F#0=input */
	dup (fdpipe[1]);	/* This sets F#1=output pipe */
	dup (fdpipe[1]);	/* This sets F#2=same output pipe */
	close (fdpipe[1]);	/* We don't want to write the pipe */
	close (fdpipe[0]);	/* nobody wants to read it in child */
#if (OS == os_vms)
	execv(*command, command);
#else (OS == os_vms)
	execvp (*command, command);
#endif (OS == os_vms)
	write (1, "Couldn't execute the program!\n", 30);
	os_exit (-1);
    }
    close (fdpipe[1]);		/* Done with pipe output */

/* Now set up the file to receive the process' output */

    os_strcpy(tempfile,"/tmp/PS.XXXXXX");
    mktemp (tempfile);
    fdscratch = creat (tempfile, 0600);
    if (fdscratch < 0) {PSError(ioerror);}
    while ((n = read (fdpipe[0], buf, 1000)) > 0) {
          write (fdscratch, buf, n);
	}
    close (fdpipe[0]);
    close (fdscratch);
    chmod (tempfile, 0600);
    wait(&status);		/* Wait for child to complete */
    return ((character *)tempfile);
}

private character *Bang()
{
  AryObj cmdvec; StmObj instm;
  Object ob;				/* scratch object */
  Stm sh; character *sp;
  character cmdstrings[255]; character *varg[20];
  integer i; character *outfile;

  PopPArray(&cmdvec);
  PopPStream(&instm);
  sh = GetStream(instm);
  /* partition cmdvec into varg pointers */
  sp = cmdstrings;
  i = 0;
  while (cmdvec.length != 0)   /* for all array elements... */
  {
    varg[i++] = sp;
    VMCarCdr(&cmdvec, &ob);		/* get next array element */
    switch (ob.type){
      case strObj: {VMGetText(ob,sp); sp += ob.length + 1; break;}
      default: TypeCheck();}
  }
  varg[i] = NIL;
  outfile = FilterRegion( (int) os_fileno(sh), (char **) varg);
  CloseFile(instm,true);
  return outfile;
}  /* end of Bang */

private procedure PSUCall()
{
character *outfile;
outfile = Bang();
unlink( (char *) outfile);
}  /* end of PSUCall */

private procedure PSUClRead()
{
character *outfile; StmObj stm; StrObj name;
outfile = Bang();
AllocPString( (cardinal) StrLen(outfile), &name);
VMPutText(name,outfile);
CreateFileStream(name, (string) "r", &stm);
unlink( (char *) outfile);
PushP(&stm);
}  /* end of PSUClRead */
#endif

private procedure PSMkTemp()
{
StrObj str;  StrObj template;
character temp[61];
extern char *mktemp();

PopPString(&str);
PopPString(&template);
if (template.length > str.length) RangeCheck();
VMGetText(template,temp);
temp[template.length] = 0;
mktemp((char *) temp);
str.length = template.length;
VMPutText(str,temp);
PushP(&str);
}  /* end of PSMkTemp */

private procedure PSChdir()
{
  StrObj str; string s; integer result;
  PopPString(&str);
  s = (string)NEW(str.length+1,1);
  VMGetText(str,s);
  result = chdir( (char *) s);
  FREE(s);
  if (result == -1) UndefFileName();
}

#if	(OS != os_pharlap)
private procedure CVT()
{
integer t; StrObj str;
#if (OS == os_mpw)
char s[MAXtimeString+10];
#else (OS == os_mpw)
char* s;
#endif (OS == os_mpw)
PopPString(&str);
t = PopInteger();
if (str.length < MAXtimeString) RangeCheck();
#if (OS == os_mpw)
IUDateString(t, longDate, s);
#else (OS == os_mpw)
s = (char*)ctime(&t);
#endif (OS == os_mpw)
s[MAXtimeString] = NUL;
VMPutText(str,(string)s);
str.length = MAXtimeString;
PushP(&str);
} /* end of CVT */
#endif	(OS != os_pharlap)

private procedure PSDayTime()
{
  long    t;

#if	OS != os_mpw
  t = time (0);
#else	OS != os_mpw
  GetDateTime (&t);
#endif	OS != os_mpw
  PushInteger (t);
}

private readonly RgCmdTable unixCmds = {
#if STAGE==DEVELOP
  "ucallread", PSUClRead,
  "ucall", PSUCall,
#endif
  "chdir", PSChdir,
  "mktemp", PSMkTemp,
  "cvt", CVT,
  "daytime", PSDayTime,
  NIL
};

public procedure UnixInit(reason)
  InitReason reason;
{
  switch (reason){
    case init: {
      break;}
    case romreg: {
      if (!MAKEVM)
	RgstMCmds(unixCmds);
#if VMINIT
      else
        { /* Register daytime in internaldict so that it's available
	     for generating the buildtime in statusdict (see errs.ps) */
	Begin(rootShared->vm.Shared.internalDict);
	RgstExplicit("daytime", PSDayTime);
	End();
	}
#endif VMINIT
      break;}
    endswitch}
} /* end of UnixInit */
