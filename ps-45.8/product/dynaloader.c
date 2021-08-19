/*****************************************************************************

    dynaloader.c

    These routines dynamically load low-level device drivers into the
    running windowserver.

    CONFIDENTIAL
    Copyright (c) 1989 NeXT, Inc.
    All Rights Reserved.

    Created 18Sep89 Ted

    Modified:

    29Sep89  Ted  Fully functioning dynaloading system in windowserver!
    02Oct89  Ted  Can map preloads or can relink driver object on demand.
    05Feb90  Dave Re-structured for more general audience.
    19Mar90  Ted  Added debugFile argument and support code.
		  Removed -r -d flags from ld line for executable output.
    04Apr90  Ted  Replaced code in favor of new rld_* commands by Mr. Enderby.
    05Apr90  Ted  Changed to "Dyna..."  Added DynaUnloadFile function.
    03Aug90  Ted  Finally replaced private rld defines with import <rld.h>.

******************************************************************************/

#import <mach.h>
#import <stdio.h>
#import <syslog.h>
#import <sys/stat.h>
#import <rld.h>

/*****************************************************************************
    DynaLoadFile
    
    This is a function which returns a pointer to a function that returns an
    int!

    This is called by anyone who wants to dynamically load a file into the
    WindowServer.  The required input is a full path name of the relocatable
    file and an entry symbol to lookup.  If you pass a non-null pointer in
    debugFile, this is taken as the filename of the debugging file you wish
    to create.  WARNING: The directory of the filename you provide must be
    writeable.

    This function returns the address of the entry point.
    
    Call DynaUnloadFile to unload the last file loaded by DynaLoadFile.
*****************************************************************************/
int (*DynaLoadFile(char *relocFile, char *debugFile, char *entryName,
    struct mach_header **header))()
{
    int (*entry)();
    NXStream *stream;
    char *object_filenames[2];
    struct stat relocStat;

    if (relocFile == NULL || entryName == NULL)
	return 0;
    if (stat(relocFile, &relocStat) < 0) {
	syslog(LOG_ERR, "Can't stat %s\n", relocFile);
	return 0;
    }
    object_filenames[0] = relocFile;
    object_filenames[1] = NULL;
    stream = NXOpenFile(fileno(stdout), NX_WRITEONLY);
    if (rld_load(stream, header, object_filenames, debugFile) == 0) {
	NXClose(stream);
	syslog(LOG_ERR, "Can't rld_load %s\n", relocFile);
    	return 0;
    }
    if (rld_lookup(stream, entryName, (unsigned long *) &entry) == 0) {
	NXClose(stream);
	syslog(LOG_ERR, "Can't rld_lookup %s\n", entryName);
    	return 0;
    }
    NXClose(stream);
    return entry;
}


/*****************************************************************************
    DynaUnloadFile
    
    Unloads the last file loaded by DynaLoadFile.  Passing 0 for unloadvm
    just unloads symbols for the file.  Passing 1 also deallocates its vm.
******************************************************************************/
void DynaUnloadFile(int unloadvm)
{
    NXStream *stream = NXOpenFile(fileno(stdout), NX_WRITEONLY);
    if (rld_unload_all(stream, unloadvm) == 0)
	syslog(LOG_ERR, "DynaUnloadFile: rld_unload_all error.\n");
    NXClose(stream);
}






