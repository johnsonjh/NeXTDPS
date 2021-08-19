/*
	defaults.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Bill Tschumy
	
	typedefs and external functions for the NeXT Window System
	defaults processing
*/

#ifndef DEFAULTS_H
#define DEFAULTS_H

/* globals for command line args */
extern int      NXArgc;
extern char   **NXArgv;

#import <db/db.h>

#define DEFAULTS_PATH ".:~/.NeXT:/bootdisk/system/conf:/bootdisk/system/ro/NeXT.sys/conf"

typedef struct _NXDefault {
  char *name;
  char *value;
} NXDefaultsVector[];


extern int NXRegisterDefaults(char *owner, NXDefaultsVector vector); 
extern char *NXGetDefaultValue(char *owner, char *name);
extern void NXSetDefault(char *owner, char *name, char *value);
extern int NXWriteDefault(char *owner, char *name, char *value);
extern int NXWriteDefaults(char *owner, NXDefaultsVector vector);
extern int NXRemoveDefault(char *owner, char *name);
extern char *NXReadDefault(char *owner, char *name);
extern void NXUpdateDefaults();
extern char *NXUpdateDefault(char *owner, char *name);

 /* Low level routines not intended for general use */

extern int NXFilePathSearch(char *envVarName, char *path, int leftToRight, 
	char *filename, int (*funcPtr)(), void *funcArg);

 /*
  * Used to look down a directory list for one or more files by a
  * certain name.  The directory list is obtained from the given
  * environment variable name, using the given default if not.  If
  * leftToRight is true, the list will be searched left to right;
  * otherwise, right to left.  In each such directory, if the file by the
  * given name can be accessed, then the given function is called with 
  * its first argument as the pathname of the file, and its second 
  * argument as the given value.  If the function returned zero, 
  * filePathSearch will then return with zero. If the function 
  * returned a negative value, filePathSearch will return
  * with the negative value. If the function returns a positive value,
  * filePathSearch will continue to traverse the driectory list and call
  * the function.  If it successfully reaches the end of the list, it
  * returns 0. 
  */
  

extern char *NXGetTempFilename(char *name, int pos);

#endif DEFAULTS_H

