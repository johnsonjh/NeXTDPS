/*****************************************************************************
    miscops.c

    This file contains the otherwise uncategorizable
    operations involved in the NeXT DPS implementation.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 13Feb89 Leo from event.c
    
    Modified:
    18Feb89 Leo  Added playsound
    07Mar89 Jack Sync w/v009 & add check for nil pw
    10Mar89 Leo  setcorelimit operator
    16Mar89 Leo  Cache passwd entry in PSFilePathSearch
    26Apr89 Leo  Fixed bug in caching entry; cache only pw_dir
    13Jul89 Leo  Added setmallocdebug operator
    27Sep89 Ted  Added loaddrivers
    16Nov89 Dave Added operators to get and reset I/O byte counts
    16Oct89 Terry CustomOps conversion
    04Dec89 Ted  Integratathon!
    06Dec89 Ted  ANSI C Prototyping, reformatting.
    07Dec89 Ted  Left MAPLOG conditional in, but made default to "off".
    07Dec89 Ted  Removed MALLOC_DEBUG conditional; leave code installed
    23May90 Ted  Removed PSLoadDrivers

******************************************************************************/

#define TIMING 1
#define MAPLOG 1	/* Default on */

#import PACKAGE_SPECS
#import PUBLICTYPES
#import CUSTOMOPS
#import EXCEPT
#import POSTSCRIPT
#undef MONITOR
#import <sys/types.h>
#import <sys/file.h>
#import <sys/time.h>
#import <sys/resource.h>
#import <strings.h>
#import <pwd.h>
#import <mach.h>
#import <cthreads.h>
#import <errno.h>
#import <sound/utilsound.h>
#import "timelog.h"

/*****************************************************************************
    currentrusage <invol context switches>
	    <vol context switches> <signals>
	    <msg received> <msg sent>
	    <sys time> <user time> <real time>
    returns all sorts of statistics on timing
******************************************************************************/

private real TimevalToReal(struct timeval *then)
{
    return((float)(then->tv_sec - ((then->tv_sec/10000)*10000)) +
	(((float)then->tv_usec)/1000000.0));
}

private PushReal(float r)
{
     PSPushPReal(&r);
}

private procedure PSCurrentRUsage()
{
	struct rusage ru;
	struct timeval now;

	getrusage(0, &ru);
	gettimeofday(&now, NULL);
	PushReal(TimevalToReal(&now));
	PushReal(TimevalToReal(&ru.ru_utime));
	PushReal(TimevalToReal(&ru.ru_stime));
	PSPushInteger(ru.ru_msgsnd);
	PSPushInteger(ru.ru_msgrcv);
	PSPushInteger(ru.ru_nsignals);
	PSPushInteger(ru.ru_nvcsw);
	PSPushInteger(ru.ru_nivcsw);
}

extern char *strtok();

/*****************************************************************************
    myGetpwdir gets the home directory for the uid given.  It maintains a
    one-element cache of home directories indexed by uid.
******************************************************************************/

char *myGetpwdir(int uid)
{
    static int lastUid = -1;
    static char *lastDir;
    
    if (lastUid == uid)
	return(lastDir);
    else
    {
    	struct passwd *pw;
	
	lastUid = uid;
	pw = getpwuid(uid);
	if (pw) {
	    if (lastDir) free(lastDir);
	    if (pw->pw_dir) {
		lastDir = (char *) malloc(strlen(pw->pw_dir)+1);
		strcpy(lastDir, pw->pw_dir);
	    }
	    else
	    	lastDir = NULL;
	}
	else
	    lastDir = NULL;
	return(lastDir);
    }
}

int FilePathSearch(char *pathName, char *fileName, int fileNameLen,
    char filePath[], int size)
{
    char pathBuffer[256];
    char *dirName, *tempName;
    int	len, dirLen, fd;

    tempName = pathName;
    while (*tempName) {
	if ((len = (index(tempName,':')-tempName)) < 0)
	    len = os_strlen(tempName);
	/* Find dirName from tempName */
	if ((*tempName == '.') && (len == 1))
	{   /* working directory */
	    /* Get the working directory pathName */
	    if (!getwd(pathBuffer)) pathBuffer[0] = 0;
	    dirName = pathBuffer;
	    dirLen = strlen(pathBuffer);
	}
	else if ((*tempName=='~') &&((len == 1)  || (*(tempName+1)=='/'))) {
	    /* home directory */
	    char *hd;
    
	    if ((hd = myGetpwdir(geteuid())) && (*hd)) {
		/* Home directory is valid */
	        strcpy(pathBuffer, hd);
		strncat(pathBuffer, tempName+1, len-1);
		dirName = pathBuffer;
		dirLen = strlen(pathBuffer);
	    }
	    else
		goto continueLoop; /* No home directory */
		/* can't just use a continue statement, need to pick
		   up code at bottom of loop */
	} else {	/* Next component of path is a real directory name */
	    dirName = tempName;
	    dirLen = len;
	}
	if ((dirLen+1+fileNameLen) >= size)
	    goto continueLoop; /* won't fit in buffer, can't open */
	strncpy(filePath, dirName, dirLen);
	filePath[dirLen++] = '/';
	strncpy(filePath+dirLen, fileName, fileNameLen);
	filePath[dirLen+fileNameLen] = 0;
	if (access(filePath, R_OK) == 0) {
	    return(0);		
	}
continueLoop:
	tempName += len;
	if (*tempName == ':') tempName++;
    }
    return(-1);
}

#if MAPLOG

private	int totalMappedFiles;
public int mappedFileSizes[20];

public procedure AddMappedFile(int size)
{
    int i;
    
    totalMappedFiles += size;
    for (i=0; i<20; i++)
	if (mappedFileSizes[i] == 0) {
	    mappedFileSizes[i] = size;
	    return;
	}
}

public procedure RemoveMappedFile(int size)
{
    int i;
    
    totalMappedFiles -= size;
    for (i=0; i<20; i++)
	if (mappedFileSizes[i] == size) {
	    mappedFileSizes[i] = 0;
	    return;
	}
} 

private procedure DumpMappedFiles()
{
    int i;
    
    os_fprintf(os_stdout, "Mapped Files: ");
    for (i=0; i<20; i++)
    	if (mappedFileSizes[i])
	    os_fprintf(os_stdout, "%d ",mappedFileSizes[i]);
    os_fprintf(os_stdout,"\n Total mapped files %d bytes\n", totalMappedFiles);
}

#endif MAPLOG

private procedure PSMStats()
{
#if MAPLOG
    DumpMappedFiles();
#endif MAPLOG
    mstats();
}

private procedure PSFilePath()
{
    PSObject path, file, result;
    
    PSPopTempObject(dpsStrObj, &result);
    PSPopTempObject(dpsStrObj, &file);
    PSPopTempObject(dpsStrObj, &path);
    if (FilePathSearch((char *)path.val.strval, (char *)file.val.strval,
	    file.length, (char *)result.val.strval, result.length) >= 0)
	result.length = os_strlen(result.val.strval);
    else
	result.length = 0;
    PSPushObject(&result);
}

#define SND_SUFFIX ".snd"
#define SND_SUFFIX_LENGTH 4

private procedure PSPlaySound()
{
    char soundName[128], soundPathName[256];
    int soundLength, priority;
    
    priority = PSPopInteger();
    soundLength = PSStringLength();
    PSPopString(soundName, sizeof(soundName)-SND_SUFFIX_LENGTH-1);

    /* Add ".snd" suffix if there is room and it is not already present */
    if ((soundLength < SND_SUFFIX_LENGTH) ||
    (strcmp(SND_SUFFIX, soundName+soundLength-SND_SUFFIX_LENGTH) !=0 )) {
	strcat(soundName, SND_SUFFIX);
	soundLength += SND_SUFFIX_LENGTH;
    }
    
    /* If sound name is an absolute path, just play it.  Otherwise search
     * for the sound name in the three standard directories.  If not found,
     * just return. (SysRefMan/24_PostOps.wn)
     */
    if (soundName[0] == '/') {
	(void)SNDPlaySoundfile(soundName, priority);
    } else if (!FilePathSearch("~/Library/Sounds/:/LocalLibrary/Sounds/:"
	"/NextLibrary/Sounds/", soundName, soundLength,
	soundPathName, sizeof(soundPathName))) {
	    (void)SNDPlaySoundfile(soundPathName, priority);
    }
}

private procedure PSSetCoreLimit()
{
    struct rlimit rls;

    getrlimit(RLIMIT_CORE, &rls);
    rls.rlim_cur = PSPopInteger();
    setrlimit(RLIMIT_CORE, &rls);
}

private procedure PSSetMallocDebug()
{
    malloc_debug(PSPopInteger());
}

private procedure PSCurrentByteCount()
{
    /* push the output, then intput byte couns on the stack */
    extern unsigned int IPCbytesIn, IPCbytesOut;  /* in ipcstream.c */
    PSPushInteger(IPCbytesOut);
    PSPushInteger(IPCbytesIn);
}

private procedure PSResetByteCount()
{
    /* set the bytecounts to 0 */
    extern unsigned int IPCbytesIn, IPCbytesOut;  /* in ipcstream.c */
    IPCbytesIn = IPCbytesOut = 0;
}

private procedure PSUnixSignal()
{
    int signum = PSPopInteger();
    int pid = PSPopInteger();

    if(kill(pid, signum) == -1) {
	os_fprintf(os_stderr,"PSUnixSignal:kill error %s\n",
		   sys_errlist[cthread_errno()]);
    }

}

private readonly RgOpTable cmdMiscops = {
#if TIMELOG
    "printeventtimes", PrintTimedEvents,
    "initeventtimes", InitTimedEvents,
#endif TIMELOG
    "currentbytecount", PSCurrentByteCount,
    "currentrusage", PSCurrentRUsage,
    "filepath", PSFilePath,
    "mstats", PSMStats,
    "playsound", PSPlaySound,
    "resetbytecount", PSResetByteCount,
    "setcorelimit", PSSetCoreLimit,
    "setmallocdebug", PSSetMallocDebug,
    "unixsignal", PSUnixSignal,
    NIL};

public procedure MiscOpsInit(int reason)
{
   if (reason == 1) PSRgstOps(cmdMiscops);
}



