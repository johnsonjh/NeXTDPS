/*****************************************************************************

    nscontexts.c

    Support for different context types in the NextStep Scheduler.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Dave 14Feb89
    
    Modified:
    10Nov89 Terry CustomOps conversion
    04Dec89 Ted   Integratathon!
    07Dec89 Ted   ANSI C Prototyping, reformatting.
    14Feb90 Dave  Fixed to accomodate dynamic loading, made contextTable a
		  linked list.
    19Mar90 Ted   Added debugPath arg to DynaLoadFile.  Affected this file.
    05Apr90 Peter Moved <mach.h> to first include
    02May90 Dave  Added registration code for Package Dynaloading and cleaned
		  it up.
    04Jun90 Dave  Added NSNotifyPS proc to PS proc vector.
    05Jun90 Dave  moved PS-context type specific stuff to pscontext.c,
    		  upgraded to new NS call format.
******************************************************************************/

#import <string.h>
#import <mach.h>
#import <fcntl.h>
#import <libc.h>
#import <sys/types.h>
#import <sys/dir.h>
#import <sys/file.h>
#import <sys/syslog.h>
#import <sys/loader.h>
#import "ipcscheduler.h"
#import "nscontexts.h"
#import "wbcontext.h"

int (*DynaLoadFile(char *, char *, char *, struct mach_header **))();
void DynaUnloadFile(int);

/* procedure table for various types of contexts */
static NSContextType	*contextTypeList = NULL;

#define PKGSEGNAME	"GraphicsPackage"
#define PKGSECTNAME	"ID"
#define PKGDBGSECTNAME	"DebugFile"

/* list of all graphics packages that can be loaded by this server */
static struct pkgEnt
{
    int		id;
    char	*path;
    char	*debug; /* can be NULL */
    struct pkgEnt *next;
} *pkgEntryList = NULL;


/*
    RegisterGraphicsPackages() looks in the directory pkgDir for all the valid graphics
    packages.  For each package it finds, it gets the package's ID from a Mach-O
    segment in the file and caches that along with the path name for the package in
    a static list.  When the package needs to be loaded, this list is consulted for
    the path name of the package.  RegisterGraphicsPackages() is called during
    initialisation.
*/

void
RegisterGraphicsPackages( char *pkgDir )
{
    DIR			*pkgDirp;
    struct direct	*pkgFile;
    off_t		pkgSize;
    struct mach_header	*mach_header;
    const struct section *pkgSect;
    int			fd, pkgID;
    kern_return_t	r;
    vm_address_t	vmstart;
    vm_size_t		vmsize;
    char		*pkgPath, *debugPath;
    struct pkgEnt	*newEnt;

    if ((pkgDirp = opendir( pkgDir )) == NULL)
	return;
    for (pkgFile = readdir( pkgDirp ); pkgFile != NULL; pkgFile = readdir( pkgDirp ))
    {
	switch (pkgFile->d_namlen) {
	case 1:
	    if (*pkgFile->d_name == '.')
		continue;  /* weed out "." */
	    break;
	case 2:
	    if (!strcmp( pkgFile->d_name, ".." ))
		continue;  /* weed out "..", too */
	    break;
	default:
	    break;
	}
	pkgPath = malloc( strlen( pkgDir ) + pkgFile->d_namlen + 2 );
	strcpy( pkgPath, pkgDir );
	strcat( pkgPath, "/" );
	strcat( pkgPath, pkgFile->d_name );
	/* has to be a file, can't deal with directories */
	if ((fd = open( pkgPath, O_RDWR )) < 0 )
	    continue; /* can't open this one! */
	pkgSize = lseek( fd, (off_t)0, L_XTND );
	vmstart = vmsize = (vm_address_t)0;
	mach_header = (struct mach_header *)0;
	if ((r = map_fd( fd, 0, (vm_address_t *) &mach_header, 1,
			    (vm_size_t)pkgSize) ) != KERN_SUCCESS)
	    continue;
	close( fd ); fd = -1;    /* Close the file.  Don't need it anymore. */
	/* get the id from the section we think it's in */
	pkgSect = getsectbynamefromheader( mach_header, PKGSEGNAME, PKGSECTNAME );
	if (pkgSect == NULL)
	    goto sectErr;
	pkgID = *(int *)((unsigned long)mach_header + pkgSect->offset);
	/* now we have the ID of this package, first look in the list to see if
	   its already there; if it is, then overwrite the data; if not, then make
	   a new entry */
	for (newEnt = pkgEntryList; newEnt; newEnt = newEnt->next)
	{
	    if (newEnt->id == pkgID)
	    {
		newEnt->path = pkgPath;
		break;
	    }
	}
	if (newEnt == NULL)
	{
	    newEnt = (struct pkgEnt *)malloc( sizeof (struct pkgEnt ) );
	    newEnt->id = pkgID;
	    newEnt->path = pkgPath;
	    newEnt->next = pkgEntryList;
	    pkgEntryList = newEnt;
	}
	/* find out if there needs to be a debug file made for this package */
	pkgSect = getsectbynamefromheader( mach_header, PKGSEGNAME, PKGDBGSECTNAME );
	if (pkgSect == NULL)
	    goto sectErr;
	debugPath = (char *)((unsigned long)mach_header + pkgSect->offset);
	if (*debugPath)
	{
	    /* a file is required; if relative, make the whole path name */
	    if (*debugPath != '/')
	    {
		/* relative, prepend with current directory */
		newEnt->debug = malloc( strlen( pkgDir ) + strlen( debugPath ) + 2 );
		strcpy( newEnt->debug, pkgDir );
		strcat( newEnt->debug, "/" );
		strcat( newEnt->debug, debugPath );
	    }
	    else
	    {
		newEnt->debug = malloc( strlen( debugPath ) + 1 );
		strcpy( newEnt->debug, debugPath );
	    }
	}
	else
	    newEnt->debug = NULL;
sectErr:
	/* deallocate the file's space */
	if ((r = vm_deallocate(task_self(), (vm_address_t) mach_header,
				(vm_size_t)pkgSize)) != KERN_SUCCESS)
	    continue;
    }
    closedir( pkgDirp );
}

void NSAddContextType( NSContextType *c )
{
    /* Link the new context type into the list */
    c->next = contextTypeList;
    contextTypeList = c;
}


NSContextType *NSLoadContextType(int id)
{
    int			(*pkgInit)();
    NSContextType	*newct;
    char		*debug = NULL;
    struct stat 	sbuf;
    struct pkgEnt	*pep;
    struct mach_header	*header = NULL;

    /* fill in the blanks, these will come from a file eventually */
    pkgInit = (int (*)())0;
    switch (id) {
    case NXWB_CONTEXTID: /* windowbitmap is one gigantic hack */
	{
	    extern int WBmain();  /* in windowbitmap.c */
	    pkgInit = WBmain;
	}
	break;

    default:  /* Load the package */
	{
	    /* look up the package name in the list of packages */
	    for (pep = pkgEntryList; pep; pep = pep->next)
	    {
		if (pep->id == id)
		    break;
	    }
	    if (pep == NULL)
		return NULL;  /* package not available */
	    if ((pkgInit = DynaLoadFile( pep->path, pep->debug, "_Start", &header ))
			    == (int (*)())0)
		return NULL;
	}
	break;
    }

    /* tell the package to initialize itself */
    if ((newct = (NSContextType *)malloc( sizeof( NSContextType ) )) == NULL)
	return NULL; /* can't get the new space */
    
    if ((*pkgInit)( newct ))
    {
	/* a non-zero return value means an error */
	free( newct );
	return NULL;
    }

    NSAddContextType( newct );
    return newct;
}


NSContextType *NSGetContextType(int ctxtID)
{
    register NSContextType	*ctp;
    register int		j;
    
    for (ctp = contextTypeList; ctp; ctp = ctp->next)
    {
	if (ctp->ncids == 1)
	{
	    if ((int)ctp->cid == ctxtID)
		return ctp;
	}
	else
	{
	    for (j = 0; j < ctp->ncids; j++)
	    {
		if (ctp->cid[j] == ctxtID)
		    return ctp;
	    }
	}
    }
    
    return NULL;
}


/* routines to implement the typed-context machinery, see also nscontexts.h */
void
NSDestroyContext( NSContextType *t, PSSchedulerContext psc )
{
    (*((t)->destroyProc))( psc->context );
    if (psc->coroutine) DestroyCoroutine(psc->coroutine);
}

void
NSTermContext( NSContextType *t, PSSchedulerContext psc )
{
    (*((t)->termProc))( psc->context );
    psc->wannaRun = 1;
}
