/*
	defaults.c
	Copyright 1988, NeXT, Inc.
	Written by: Richard Williamson
	Responsibility: Bill Tschumy
	
	This file holds code to read a user's .NeXTdefaults file, and
	return requested info to the application.

*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <pwd.h>
#import <stdio.h>
#import <sys/file.h>
#import <sys/param.h>
#import <sys/times.h>
#import <sys/time.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <errno.h>
#import "defaults.h"

#ifdef SHLIB
#import "shlibextras.h"
#endif SHLIB

/* In libc.a(sprintf.o) */
#define sprintf (*_libappkit_sprintf)
extern int (*_libappkit_sprintf)();

extern int getuid(void);
extern int access(const char *path, int mode);
extern int mkdir(const char *name, int mode);

#ifdef DB_PROTO
extern dbDelete(Database *db, Data *data);
extern dbClose(Database *db);
extern dbFetch(Database *db, Data *data);
extern dbStore(Database *db, Data *data);
extern dbWaitLock(Database *db, int a, int b);
extern dbUnlock(Database *db);
#endif

#define READ_BUF_SIZE   1024


struct RegisteredDefault {
    time_t         mtime;
    char           *owner;
    char           *name;
    char           *value;
    struct RegisteredDefault *next;
};

static char    *defaultAlloc();
static void     cmdLineToDefaults();
static Database *_openDefaults();
static int _closeDefaults();
static int _registerDefault();
static _resetRegisteredDefault();
static struct RegisteredDefault *_isRegistered();
static char *_doReadDefault();
static char *_doWriteDefault();
static int _writeAndRegisterDefault();
static _lockDefaults();
static _unlockDefaults();
static _constructKey();
static _destroyKey();
static char *blockAlloc();

static Database *defaultsDB;
static char    *readBuf;

static struct RegisteredDefault *firstRD = 0;
static struct RegisteredDefault *lastRD = 0;

#define DEFAULTBLOCKSIZE 512
static char    *dBlock = 0;
static char    *dPos = 0;
static char   **firstBlock;
static char   **lastBlock;
static time_t db_mtime;
extern char *strcpy();

int
NXRegisterDefaults(owner, vector)
    char           *owner;
    NXDefaultsVector vector;

 /*
  * Registers the defaults in the vector.  Registration of defaults is
  * required and accomplishes two things.  (1) It specifies an initial
  * value for the default in the case when it doesn't appear in the
  * .NeXTdefaults database or on the command line.  (2) It allows more
  * efficient reading of the defaults from the defaults database.  This is
  * because all defaults specified in the vector are looked up at once
  * without having to open and close the database for each lookup. 
  * If you want to use the defaults mechanism don't expect to be able
  * to parse argv.  These functions remove items from the command line.
  * After the items have been removed and there should be NXArgc items
  * left in the argv array.
  *
  * A common place to register your defaults is in the initialize method of
  * the class that will use them. 
  * 
  * 0 is returned if the database couldn't be opened. Otherwise 1.
  */
{
    struct RegisteredDefault *rd;
    struct _NXDefault *d;
    char           *value;


 /* Register command line arguments.  */
    db_mtime = 0;
    cmdLineToDefaults( owner, vector );

    if (!_openDefaults())
	return (0);

 /*
  * The default gets its value with the following precedence: 
  *
  * 1)  Look for the default in the registration table with specified owner 
  *
  * 2)  Look for the default in the database with the specified owner 
  *
  * 3)  Look for the default in the registration table with no owner 
  *
  * 4)  Look for the default in the database with no owner 
  *
  * 5)  Use the initial default value from the vector 
  */
    for (d = vector; d->name; d++) {
	if (_isRegistered(owner, d->name))	/* Already registered */
	    continue;

	if (value = _doReadDefault(owner, d->name, 0))	/* Database w/ owner */
	    _registerDefault(owner, d->name, value);

	else if (rd = _isRegistered(0, d->name))	/* Table w/o owner */
	    _registerDefault(owner, d->name, rd->value);

	else if (value = _doReadDefault(0, d->name, 0))	/* Database w/o owner */
	    _registerDefault(owner, d->name, value);

	else {			/* inital value from vector */
	    if (d->value) {
		value = (char *)defaultAlloc(strlen(d->value) + 1);
		strcpy(value, d->value);
	    } else
		value = d->value;
	    _registerDefault(owner, d->name, value);
	}
    }
    _closeDefaults();
    return (1);
}


char           *
NXGetDefaultValue(owner, name)
    char           *owner;
    char           *name;
 /*
  * This routine returns a pointer to the default requested.  If the owner
  * is null then the default is a system wide default.  The defaults
  * database can be overridden by supplying command line arguments at
  * launch time.  This is done with the following syntax: 
  *
  * app -default1 value -default2 value. 
  *
  * If a the default doesn't exist for the given owner then a search is made
  * for that default as a system wide default. Memory is
  * allocated for the default value. 
  */
{
    struct RegisteredDefault *rd;
    char           *defaultString;


 /*
  * 1)  Look for the default in the registration table with specified
  * owner 
  *
  * 2)  Look for the default in the database with the specified owner 
  *
  * 3)  Look for the default in the registration table with no owner 
  *
  * 4)  Look for the default in the database with no owner 
  *
  * In any case if we find a default that isn't registered then register it
  * in the database. 
  */
    if (rd = _isRegistered(owner, name))
	return (rd->value);

    if (!defaultsDB) {
	if (!_openDefaults())
	    return (0);
    }
    if ((defaultString = _doReadDefault(owner, name, 0))) {
	;
    } else if ((rd = _isRegistered(0, name))) {
	defaultString = rd->value;
    } else
	defaultString = _doReadDefault(0, name, 0);
    _closeDefaults();
    _registerDefault(owner, name, defaultString);
    return (defaultString);
}


char           *
NXReadDefault(owner, name)
    char           *owner;
    char           *name;
 /*
  * This routine guarantees a read of the defaults database.  If the
  * exact owner name default is not found in the database then a NULL
  * is returned.  This routine does not reset the internal
  * cache and does not look for the named default as a system wide 
  * default.  You can update the internal cache by calling NXSetDefault
  * with the value returned from this functions.  Also you can look for
  * the system wide named default by send a NULL for the owner parameter.
  */
{
    char           *defaultString;

    if (!defaultsDB) {
	if (!_openDefaults())
	    return (0);
    }
    defaultString = _doReadDefault(owner, name, 0);
    _closeDefaults();
    return (defaultString);
}


void
NXUpdateDefaults()
 /*
  * This routine checks the last modification of the defaults database.
  * The timestamp of each internally registered default is checked 
  * against the time stamp of the database.  If the time stamp of the
  * database is greater than the timestamp of the default then the
  * database if re-read.  Note that this function will override the
  * defaults that have been set by the command line, or with
  * NXSetDefault().  This routine checks every cached default against
  * the time stamp of the database.
  */
{
    struct RegisteredDefault *rd = firstRD;
    char *value;

    if (!defaultsDB) {
	if (!_openDefaults())
	    return;
    }
    
    while (rd) {
        if (rd->mtime < db_mtime ) {
	    value = _doReadDefault( rd->owner, rd->name, 0 );
	    if (value)
	        _resetRegisteredDefault(rd, rd->owner, rd->name, value);
	}
	rd = rd->next;
    }

    _closeDefaults();

    return;
}

char *
NXUpdateDefault( char *owner, char *name )
 /*
  * This routine checks the last modification of the defaults database.
  * The time stamp of each internally registered default is checked 
  * against the time stamp of the database.  If the time stamp of the
  * database is greater than the timestamp of the default then the
  * database if re-read.  Note that this function will override the
  * defaults that have been set by the command line, or with
  * NXSetDefault().
  * This routine the new value if it is changed or NULL if it isn't
  * updated.
  */
{
    struct RegisteredDefault *rd = firstRD;
    char *value;

    if (!defaultsDB) {
	if (!_openDefaults())
	    return;
    }
    
    if ( rd = _isRegistered(owner, name)){
        if (rd->mtime < db_mtime ) {
            value = _doReadDefault( rd->owner, rd->name, 0 );
	    if (value)
	        _resetRegisteredDefault(rd, rd->owner, rd->name, value);
	}
    }
    else
        value = _doReadDefault( rd->owner, rd->name, 0 );

    _closeDefaults();

    return( value );
}


void
NXSetDefault(owner, name, value)
    char           *owner;
    char           *name;
    char           *value;

 /*
  * Sets the default's value for this process.  Future calls to
  * NXGetDefaultValue will return the string passed for the value.  This
  * does not write the default to the defaults database. 
  */
{
    struct RegisteredDefault *rd;

    if ( rd = _isRegistered(owner, name))
	_resetRegisteredDefault(rd, owner, name, value);
    else
	_registerDefault(owner, name, value);
}


int
NXWriteDefault(owner, name, value)
    char           *owner;
    char           *name;
    char           *value;

 /*
  * Writes a default and its value into the default database.  If the
  * owner is 0 then the default is system wide.  The newly written default
  * value for the owner and name will be returned if NXGetDefaultValue
  * is called with the same owner and name.
  *
  * 0 is returned if an error occured writing the default.  Otherwise a 1 is
  * returned. 
  */
{
    int           returnValue;

    if ((!defaultsDB) && (!_openDefaults()))
	return (0);

    returnValue = _writeAndRegisterDefault( owner, name, value );
    _closeDefaults();
    return (returnValue);
}


int
NXWriteDefaults( owner, vect )
    char *owner;
    NXDefaultsVector vect;
 /*
  * Writes the defaults in the vector into the default database.  If the
  * owner is 0 then the default is system wide.  Writing defaults overrides
  * any registered default.
  *
  * The number of successfully written defaults is returned.
  */
{
    register struct _NXDefault *d;
    register int num = 0;

    if ((!defaultsDB) && (!_openDefaults()))
	return (0);

    for (d = vect; d->name; d++)
      if( _writeAndRegisterDefault( owner, d->name, d->value))
	num++;

    _closeDefaults();
    return (num);
}
      


int
NXRemoveDefault(owner, name)
    char           *owner;
    char           *name;
 /*
  * Remove a default from the defaults database.  Returns 0 if the 
  * default could not be removed.  Otherwise a 1 is returned.
  */
{
    Data            deleteData;
    int             returnVal;

    if (!defaultsDB)
	if (!_openDefaults())
	    return (0);

    if (!name)
	return (0);
    _constructKey(&deleteData.k, owner, name);
    returnVal = dbDelete(defaultsDB, &deleteData);
    _destroyKey(&deleteData.k);

    _closeDefaults();
    return (returnVal);
}



/*** Private functions ***/

static Database       *
_openDefaults()
 /*
  * Open the defaults database.  If the ~/.NeXT directory doesn't exist
  * then this directory is created.  If the database doesn't exist then it
  * too is created.  This routine also scans the command line parameters.
  * Returns a pointer to the Database if successful, otherwise it returns
  * 0. 
  *
  * After calling _openDefaults you must call _closeDefaults. 
  */
{
    struct passwd  *pw;
    extern struct passwd  *getpwuid();
    struct stat stat_buf;
    int             dir;

    pw = getpwuid(getuid());

 /* Construct the path ~/.NeXT/.NeXTdefaults. */
    if (pw && (pw->pw_dir) && (*pw->pw_dir)) {

      /* Allocate a buffer used to read default data from the database.  */
	if (!(readBuf = (char *)malloc(READ_BUF_SIZE)))
	    return (0);
    /*
     * Use the readBuf as a temp buffer to construct the .NeXTdefaults
     * path.  This is nasty but we don't really cause any problems by
     * using the readBuf this way. 
     */
    /* First see if the ~/.NeXT directory exists. */
	strcpy(readBuf, pw->pw_dir);
	strcat(readBuf, "/.NeXT");

    /* If the dir doesn't exist then try to create it. */
	if ((dir = access(readBuf, F_OK)) < 0) {
	    if (errno == ENOENT) {
		if (mkdir(readBuf, 0777))
		    return (0);
	    } else
		return (0);
	}
	
    /*
     * Now that we know the ~/.NeXT dir exists we can proceed to open the
     * defaults database. 
     */
	strcat(readBuf, "/.NeXTdefaults");

	defaultsDB = dbOpen(readBuf);
	if (!defaultsDB) {
	    free(readBuf);
	    readBuf = 0;
	    return (0);
	}
	/*
         * Get the current time stamp for the database.
	 */
	strcat(readBuf, ".L" );
	stat( readBuf, &stat_buf );
	db_mtime = stat_buf.st_mtime;

	if (!_lockDefaults())
	    return (0);

	return (defaultsDB);
    }
    return (0);
}


static int
_closeDefaults()
 /*
  * Close the defaults database.  Deallocate MOST memory allocated. 
  */
{
    char          **nextBlock = 0;

    _unlockDefaults();
 /*
  * Close the database. 
  */
    if (defaultsDB) {
	dbClose(defaultsDB);
	defaultsDB = 0;
    }
 /*
  * Free memory used as temporary read buffer. 
  */
    if (readBuf) {
	free(readBuf);
	readBuf = 0;
    }
#ifdef DONT_FREE   /*  We need this memory to persistent across a _closeDefaults. */

 /*
  * Free memory used to store user default values. 
  */
    while (firstBlock) {
	nextBlock = (char **)*firstBlock;
	free(firstBlock);
	firstBlock = nextBlock;
    }
#endif
    return (1);
}


static int
_registerDefault(owner, name, value)
    char           *owner, *name, *value;

 /*
  * This could be changed to use a hash table. 
  */
{
    if (!firstRD)
	firstRD = lastRD = (struct RegisteredDefault *)
	      defaultAlloc(sizeof(struct RegisteredDefault));
    else {
	lastRD->next = (struct RegisteredDefault *)
	      defaultAlloc(sizeof(struct RegisteredDefault));
	lastRD = lastRD->next;
    }
    lastRD->next = 0;

    lastRD->owner = owner ? strcpy(defaultAlloc( strlen(owner)+1), owner ) : 0;
    lastRD->value = value ? strcpy(defaultAlloc( strlen(value)+1), value ) : 0;
    lastRD->name = name ? strcpy(defaultAlloc( strlen(name)+1), name ) : 0;
    lastRD->mtime = db_mtime;

    return (1);
}


static _resetRegisteredDefault(rd, owner, name, value)
    struct RegisteredDefault *rd;
    char           *owner, *name, *value;
{
    /*
     *  This code results in a memory leak.  The previously allocated
     *  memory for the RegisteredDefault isn't deallocated.
     */
    rd->value = value ? strcpy( defaultAlloc( strlen(value)+1), value ) : 0;
    rd->owner = owner ? strcpy( defaultAlloc( strlen(owner)+1), owner ) : 0;
    rd->name = name ? strcpy( defaultAlloc( strlen(name)+1), name ) : 0;
    rd->mtime = db_mtime;
}


static struct RegisteredDefault *
_isRegistered(owner, name)
    char           *owner, *name;
{
    struct RegisteredDefault *rd = firstRD;

    while (rd) {
	if (strcmp(rd->name, name) == 0) {
	    if (!(owner && rd->owner)) {	/* One owner is NULL */
		if (owner == rd->owner)
		    return (rd);
	    } else if (strcmp(rd->owner, owner) == 0)
		return (rd);
	}
	rd = rd->next;
    }
    return (0);
}
	      

static char           *
_doReadDefault(owner, name, buf)
    char           *owner;
    char           *name;
    char           *buf;

 /*
  * Read a default from the defaults database.  Owner should typically be
  * the application that uses the default.  If owner is null then the
  * default will be registered as a system default.  The name is the
  * default name. 
  *
  * The string pointer for the requested value is returned, or null if
  * the default isn't in the database.
  */
{
    Data            readData;
    char           *defaultString;


    if (!name)
	return (0);

 /*
  * Get the default value. 
  */
    _constructKey(&readData.k, owner, name);
    readData.c.s = readBuf;
    if (!dbFetch(defaultsDB, &readData)) {
#ifdef FUNCTION_SHOULD_LOOK_FOR_OWNERLESS_DEFAULT
    /*
     * Look for the default without the owner. 
     */
	_constructKey(&readData.k, (char *)0, name);
	if (!dbFetch(defaultsDB, &readData))
	    return (0);
#endif
	return (0);
    }
    _destroyKey(&readData.k);

    defaultString = defaultAlloc(readData.c.n);
    strcpy(defaultString, readData.c.s);

    return (defaultString);
}


static char           *
_doWriteDefault(owner, name, value)
    char           *owner;
    char           *name;
    char           *value;
{
    Data            writeData;


    if (!name)
	return (0);

    if (!value)
	value = "";

    _constructKey(&writeData.k, owner, name);
    writeData.c.s = value;
    writeData.c.n = strlen(value) + 1;

    if (!dbStore(defaultsDB, &writeData))
	return (0);

    _destroyKey(&writeData.k);

    return (value);
}


static int
_writeAndRegisterDefault( owner, name, value )
    char *owner;
    char *name;
    char *value;
{
    struct RegisteredDefault *rd;
    int rVal;

    if ((rVal = _doWriteDefault(owner, name, value) ? 1 : 0)) {
        if (rd = _isRegistered(owner, name))
	    _resetRegisteredDefault(rd, owner, name, value);
	else
	    _registerDefault(owner, name, value);
    }
    return( rVal );
}


static _lockDefaults()
{
    return (dbWaitLock(defaultsDB, 5, 30));
}



static _unlockDefaults()
{
    dbUnlock(defaultsDB);
}



static _constructKey(keyDatum, owner, name)
    Datum          *keyDatum;
    char           *owner, *name;
{
    char           *cptr;
    int             len;

    if (!owner)
	owner = "";

    len = strlen(owner) + strlen(name) + 1;
    keyDatum->s = cptr = (char *)malloc(len);
    keyDatum->n = len;
    while (*owner)
	*cptr++ = *owner++;
    *cptr++ = 0xff;
    while (*name)
	*cptr++ = *name++;
}


static _destroyKey(keyDatum)
    Datum          *keyDatum;
{
    if (keyDatum)
	free(keyDatum->s);
}


static char    *
defaultAlloc(chars)
    int             chars;

 /*
  * allocates a block of DEFAULTBLOCKSIZE characters at a time, then
  * dishes them out in response to calls. This is to avoid the
  * fragmentation that individual calls to malloc would cause. 
  */
{
    char           *place;


    if (chars > DEFAULTBLOCKSIZE)
	return (blockAlloc(chars));

    if ((!dBlock) || (chars > (dBlock + DEFAULTBLOCKSIZE - dPos)))
	dBlock = dPos = (char *)blockAlloc(DEFAULTBLOCKSIZE);
    place = dPos;
    dPos += chars;

    return (place);
}


static char           *
blockAlloc(size)
    int             size;
{
    char          **place = (char **)malloc(size + sizeof(char *));


    if (firstBlock) {
	*lastBlock = (char *)place;
	lastBlock = place++;
    } else
	firstBlock = lastBlock = place++;

    return ((char *)place);
}


static
_NXGobbleArgs(numToEat, argc, argv)
    register int    numToEat;
    int            *argc;
    register char **argv;

 /*
  * a little accessory routine to the command line arg parsing that
  * removes some args from the argv vector. 
  */
{
    register char **copySrc;

 /* copy the other args down, overwriting those we wish to gobble */
    for (copySrc = argv + numToEat; *copySrc; *argv++ = *copySrc++);
    *argv = 0;			/* leave the list NULL terminated */
    *argc -= numToEat;
}


static void
cmdLineToDefaults( char *owner, NXDefaultsVector vector )
 /*
  * Sucks arguments we need off the command line and places them the
  * parameters data. 
  */
{
    extern int      NXArgc;
    extern char   **NXArgv;
    register int   *argc = &NXArgc;
    register char **argv = NXArgv;
    struct RegisteredDefault *rd;
    struct _NXDefault *d;
    char           *value;

    argv++;			/* skip the command itself */
    while (*argv) {		/* while there are more args */
	if (**argv == '-') {	/* if its a flag (starts with a '=') */
	    /*
	     * If we have something of the form -default value then
	     * register it.
	     */
	    if ( argv[1] && *argv[1] != '-' ) {
	      /* See if this default has been requested. */
	      for (d = vector; d->name; d++) {
		if ( strcmp( argv[0]+1, d->name) == 0 )
		  break;
	      }
	      if( d->name ){
		_registerDefault(owner, argv[0] + 1, argv[1]);
		_NXGobbleArgs(2, argc, argv);
	      }
	      else
		argv++;
	    } 
	    /*
	     * If we have something of the form -default with no value
	     * or -default -default then set the default to null
	     */
	    else {
	      /* See if this default has been requested. */
	      for (d = vector; d->name; d++) {
		if ( strcmp( argv[0]+1, d->name) == 0 )
		  break;
	      }
	      if( d->name ){
		_registerDefault(owner, argv[0] + 1, "");
		_NXGobbleArgs(1, argc, argv);
	      }
	      else
		argv++;
	    }
	} else			/* else its not a flag at all */
	    argv++;
    }
}


static char    *firstArg;
static int      firstLen;

extern char *strtok();

static char    *
rstrtok(s1, s2)
    register char  *s1, *s2;
 /*
  * our right-to-left version of strtok.  Exception: rstrtok does not work if
  * s2 is of length > 1. 
  */
{
    register char  *cp;
    register int    s1len, clen;

    if (s1) {
	firstArg = s1;		/* Save away */
	firstLen = s1len = strlen(s1);
    } else {
	s1 = firstArg;		/* Cache in reg */
	s1len = firstLen;
    }
    clen = 0;
    cp = s1 + s1len - 1;
    while (cp >= s1) {
	if (*cp == *s2) {	/* Change this line to a search to */
	    if (clen) {		/* 1-char limit on s2. FIX */
		firstLen = cp - s1;
		cp++;
		*(cp + clen) = 0;
		return (cp);
	    } else {
		clen--;
	    }
	}
	cp--;
	clen++;
    }
    if (clen) {
	*(s1 + clen) = 0;
	firstLen = 0;
	return (s1);
    } else
	return (NULL);
} /* rstrtok */


#pragma CC_OPT_OFF



/*
Modifications (starting at 0.8):

1/10/89  rjw	Fixed dangling pointer bug.  When a default was registered
		using _registerDefault or _resetRegistedDefault the owner,
		name, and value weren't copied.  This occasionaly resulted in 
		problems when non-constant data was used.
2/17/89  rjw    Add NXUpdateDefaults().  Each default is time stamped.
                NXUpdateDefaults() will re-read the database if the timestamp
		of the database differs from the timestamp of the default.
*/

