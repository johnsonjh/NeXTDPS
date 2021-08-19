/*****************************************************************************

    listener.c

    The code to listen for new Mach IPC-based connections
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 11Sep88 Leo
    
    Modified:
    
    10Oct88 Leo  Login context, system contexts, etc.
    13Jan89 Jim  updated to v004
    17Jan89 Jack get contextExecutive from shareddict
    08Feb89 Dave added typed contexts
    27May89 Leo  Fixed state bug by expanding listenPublic
		 to a state variable (listenerState)
    07Nov89 Terry CustomOps conversion
    04Dec89 Ted   Integratathon!
    06Dec89 Ted   ANSI C Prototyping, reformatting.
    05Jun90 Dave  Moved CreateIPCStreams to ipcstream.c from here
    14Jun90 Pete  Added setbootstrapport
    24Jul90 Terry Set global bootstrap_port variable in setbootstrapport
    10Aug90 Terry Add call to setgroups() in SetEffectiveUser()

******************************************************************************/

#import PACKAGE_SPECS
#import CUSTOMOPS
#import EXCEPT
#import POSTSCRIPT
#import STREAM

#if (OS == os_mach) /* Leo 10Apr88 Get NeXT entensions */
#define NeXT 1
#endif

#import <sys/param.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sys/socket.h>
#import <sys/time.h>
#import <sys/un.h>
#import <sys/notify.h>
#import <pwd.h>
#import <sgtty.h>
#import <fcntl.h>
#import <signal.h>
#import <netinet/in.h>
#import <servers/netname.h>
#import "ipcstream.h"
#import "ipcscheduler.h"

extern port_t name_server_port;

#define PS_NAME "NextStep(tm) Window Server"
#define ROOT_UID 0
#define ROOT_GID 1
#define DAEMON_UID 1
#define DAEMON_GID 1

/* Values of ListenerState */
typedef enum _ListenerState {
    listenerPublic,	/* listen port registered under public name */
    listenerPrivate,	/* listen port reg'd under secret name */
    listenerTempname	/* listen port reg'd under temp name (just
			   after a resetuser) */
} ListenerState;

/* Private data */
static	PSContext listenContext;/* The PostScript context that listener is
				   run in */
static	port_t	signaturePort;	/* Port used as signature for netname */
public	port_t	listenPort;	/* Port on which we are listening */
				/* Exported so socketlistener can get it */
static	boolean	loginContextSet;/* Has someone become the login context */
static	boolean globalUserSet;	/* Is the global user set to some particular
				   user, or are we still root? */
static	ListenerState
		listenerState;	/* Current state of listener */
static	boolean listenRestart;	/* Used for synchronization */
static	char	portName[32];	/* Name under which our port is currently
				   registered */
public	uid_t	globalUid;	/* global user id of the server (usually
				   the user currently logged in) */
public	gid_t	globalGid;	/* same for group */
unsigned char privateName[] = { 0x4e, 0x5e, 0x4e, 0x75, 0, 0, 0, 0 };

/* Utility routines to register or unregister the listen port */
private procedure CheckInListenPort(char *name)
{
    if (netname_check_in(name_server_port, name, signaturePort, listenPort)
    != NETNAME_SUCCESS)
	PSLimitCheck();
    strcpy(portName, name);
}

private procedure CheckOutListenPort(char *name)
{
    netname_check_out(name_server_port, name ? name : portName, signaturePort);
}

private procedure PublicListenPort(int init)
{
    if (!init) CheckOutListenPort(NULL);
    CheckInListenPort(PS_NAME);
    listenerState = listenerPublic;
}

private procedure PrivateListenPort()
{
    CheckOutListenPort(NULL);
    CheckInListenPort((char *)privateName);
    listenerState = listenerPrivate;
}

/*****************************************************************************
    CreateNewContext takes a pointer to a request message containing a
    remote_port to be replied to and one bit of data which is the port to
    which future output from the context should be addressed.  It sets up
    a new context all primed to execute initString, and then replies
    to the requested port, sending along the input port that hasjust been
    created.
    
    Notice: this is not actually a notification message, it's just that
    there's this nicely declared structure for us that has the right
    information sitting there.
******************************************************************************/

private procedure CreateNewContext(notification_t *msg)
{
    /*
     *  Look up the context type in the context table.  If it's
     *  there, then call the appropriate routines.  If not, just
     *  politely say so and leave.
     *
     *  Later, we'll go around on the disk and look for a graphics
     *  package to load in here.  If the package can't be found, then
     *  we cack out.
     */

    NSContextType *i;
    PSSchedulerContext newPSC;
    extern PSSchedulerContext contextList;

    DURING
	if ((i = NSGetContextType(msg->notify_header.msg_id)) == NULL) {
	    if ((i = NSLoadContextType(msg->notify_header.msg_id)) == NULL)
		PSLimitCheck();  /* can't load context */
	}
	if (newPSC = NSCreateContext(i, msg)) {
	    /* validate the context type */
	    newPSC->type = i;
	}
	else
	    PSLimitCheck();  /* can't create new context */

    HANDLER
    {
	os_fprintf( os_stderr, "%s: Error in listener!  Errno %d.\n",
	    "Loading Graphics Package", Exception.Code );
    }
    END_HANDLER;
}

/*****************************************************************************
    LoginContextKilled is called by the scheduler whenever a login context is
    destroyed.
******************************************************************************/

public procedure LoginContextKilled()
{
    loginContextSet = false;
}

/*****************************************************************************
    SetGroups is called by SetEffectiveUser to make the window server belong to 
    the set of groups available to the specified user id.
    NOTE:  It assumes that we are seteuid root
    A 0 is returned on success, -1 on error, with a error code stored in errno.
******************************************************************************/

typedef struct {
    int groups[NGROUPS];
    int ngroups;
    unsigned short age; /* 0 if unused */
    uid_t uid;
} GroupCacheItem;

#define GROUPCACHESIZE 5
static GroupCacheItem GroupCache[GROUPCACHESIZE];
static unsigned short cacheclock;

SetGroups(uid_t uid, gid_t gid)
{
    int i, minage;
    struct passwd *pw;
    GroupCacheItem *slot, *gp;
    
    /* If uid is in groupcache, set the corresponding groups */
    for (i = 0, gp = GroupCache; i < GROUPCACHESIZE; i++, gp++)
	if (gp->age && uid == gp->uid) {
	    /* When this wraps, gp falls out of the cache, but so what */
	    gp->age = ++cacheclock;
	    return setgroups(gp->ngroups, gp->groups);
        }

    /* Cache lookup failed so find an empty or least recently used slot */
    for (i = 0, gp = GroupCache, minage = (1<<30); i < GROUPCACHESIZE; i++, gp++)
	if (gp->age < minage) { minage = gp->age; slot = gp; }
    
    /* If we can't find the uid's username, allow only their gid group */
    if (!(pw = getpwuid(uid)))
	return setgroups(1, &gid);
    else {
        int ret;
	
	/* Get the groups, put them in the slot, and set the groups */
	ret = initgroups(pw->pw_name, pw->pw_gid);
	slot->ngroups = getgroups(NGROUPS, slot->groups);
	slot->uid     = uid;
	slot->age     = ++cacheclock;
	return ret;
    }
}

/*****************************************************************************
    SetEffectiveUser sets the window server process to be executing as
    whatever user is identified in the call.  If will remember what is set,
    and not make any unnecessary system calls.  Note that, therefore, all uid
    changing calls must come through this routine.
******************************************************************************/

public procedure SetEffectiveUser(uid_t toUid, gid_t toGid)
{
    static currentEUid = 0;
    static currentEGid = 0;
    
    /* If we have to change... */
    if (toUid != currentEUid)
    {
    	/* If not root now, reset to root */
    	if (currentEUid != ROOT_UID) seteuid(ROOT_UID);

	/* Must do group hacking while user id is root! */
	if (toGid != currentEGid) {
	    if (currentEGid != ROOT_GID) setegid(ROOT_GID);
	    currentEGid = toGid;
	    if (toGid != ROOT_GID)
	        setegid(toGid);
	}
	
	/* Record new value */
	currentEUid = toUid;

        /* Now call setgroups() with the appropriate group list for our uid.
	 * This must be done while we are still euid root.
	 */
	SetGroups(toUid, toGid);
	
	/* If destination id isn't root, reset to final value */
	if (toUid != ROOT_UID) seteuid(toUid);
    }
}

/*****************************************************************************
    PSListener establishes this PostScript process as being in the state of
    listening for connections on a socket.  This operator will not return,
    instead sleeping and accepting more connections until some other Context
    calls the termsocketlistener operator.
******************************************************************************/

private procedure PSListener()
{
    struct _SchedulerMsg *msg;
    YieldReason because;

    listenContext = currentPSContext;
    if (port_allocate(task_self_, &listenPort) != KERN_SUCCESS)
	PSLimitCheck();
    if (port_allocate(task_self_, &signaturePort) != KERN_SUCCESS)
	PSLimitCheck();
    PublicListenPort(1);
    listenRestart = 0;
    loginContextSet = 0;
    globalUserSet = 0;
    /* Initialize msg and reason for that first call to ContextYield */
    msg = (struct _SchedulerMsg *) listenPort;
    because = yield_other;
    while(1) {
	ContextYield(because, &msg);
	if (listenRestart) {
	    /* This means we've allocated a new listen port.  Go back and
	       yield for yield_other reason to establish the new port. */
	    msg = (struct _SchedulerMsg *) listenPort;
	    because = yield_other;
	    listenRestart = 0;
	} else {
	    DURING
		CreateNewContext((notification_t *)msg);
	    HANDLER {
		os_fprintf(os_stderr,"%s: Error in listener!  Errno %d.\n",
		    "DPS", Exception.Code);
	    } END_HANDLER;
	    because = yield_stdin;
	}
    }
}

private procedure PSSetLoginContext()
{
    if (loginContextSet) PSInvlAccess();
    currentPSContext->scheduler->login = true;
    loginContextSet = true;
    MarkSystemContexts();
}

private procedure PSLoginExit()
{
    if (!currentPSContext->scheduler->login) PSInvlAccess();
    exit(1);
}

private procedure PSSetPublicListener()
{
    if (PSPopBoolean())
    	PublicListenPort(0);
    else
    	PrivateListenPort();
}

private procedure PSSetUser()
{
    int	uid, gid;
    
    if (!currentPSContext->scheduler->login) PSInvlAccess();
    gid = PSPopInteger();
    uid = PSPopInteger();
    if (globalUserSet) PSInvlAccess();
    if ((int) getpwuid(uid) <= 0) {	/* If user is not a valid user! */
    	uid = DAEMON_UID;
	gid = DAEMON_GID;
    }
    SetEffectiveUser(uid, gid);
    globalUid = uid;
    globalGid = gid;
    globalUserSet = true;
}

private procedure PSResetUser()
{
    char tempName[32];
    struct timeval t;
    PSObject str;
    
    if (!currentPSContext->scheduler->login) PSInvlAccess();
    if (!globalUserSet) PSInvlAccess();
    PSPopTempObject(dpsStrObj, &str);
    
    /* First, get rid of other contexts, and get rid of the current
    listen port (since some other contect may have it) */
    TerminateUserContexts();
    CheckOutListenPort(NULL);
    port_deallocate(task_self_, listenPort);
    
    /* OK, all contact with the outside world is severed.  Reset the ids */
    SetEffectiveUser(ROOT_UID, ROOT_GID);
    globalUid = ROOT_UID;
    globalGid = ROOT_GID;
    globalUserSet = false;
    
    /* OK, the hardest part.  Allocate a new listen port and
    get the listener context to restart (and thus start listening on it) */
    listenRestart = 1;
    port_allocate(task_self_, &listenPort);
    PSMakeRunnable(listenContext);
    
    /* Now check the new port in under a random name */
    gettimeofday(&t, NULL);
    sprintf(tempName,"NextStep(tm)%d", t.tv_usec);
    if (strlen(tempName) > str.length) tempName[str.length] = 0;
    CheckInListenPort(tempName);
    listenerState = listenerTempname;
    
    /* Now return the temp name */
    str.length = strlen(tempName);
    bcopy(tempName, str.val.strval, str.length);
    PSPushObject(&str);
}

private procedure PSCurrentUser()
{
    PSPushInteger(geteuid());
    PSPushInteger(getegid());
}

private procedure PSSetJobUser()
{
    int	toUid, toGid;
    PSSchedulerContext psc;
    
    psc = currentPSContext->scheduler;
    if (psc->userSet) return;
    toGid = PSPopInteger();
    toUid = PSPopInteger();
    if ((int) getpwuid(toUid) <= 0) {
    	toUid = DAEMON_UID;
	toGid = DAEMON_GID;
    }
    psc->userSet = true;
    psc->system = true;
    psc->uid = toUid;
    psc->gid = toGid;
    SetEffectiveUser(toUid, toGid);
}

/*****************************************************************************
    PSSetWriteBlock implements an operator allowing contexts to set whether
    writes block or fail. PSCurrentWriteBlock does the obvious.
******************************************************************************/

private void PSSetWriteBlock()
{
    currentPSContext->scheduler->writeBlock = PSPopBoolean();
}

private void PSCurrentWriteBlock()
{
    PSPushBoolean(currentPSContext->scheduler->writeBlock);
}

/* {set,current}nextstepcontext.  Provides a one-way door to turn off
   file-writing in a PostScript context
*/
private void PSSetNextStepContext()
{
    int writeProhibit = PSPopBoolean();
    if(!currentPSContext->scheduler->writeProhibited)
	currentPSContext->scheduler->writeProhibited = writeProhibit;
}

private void PSCurrentNextStepContext()
{
    PSPushBoolean(currentPSContext->scheduler->writeProhibited);
}

/* this routine is externed deep in the bowels of unixfopen.c to
 *  prevent secure contexts from opening files for writing
 */
public int IsContextWriteProhibited()
{
    return(currentPSContext && currentPSContext->scheduler->writeProhibited);
}

private void PSSetNextObjectFormat()
{
    switch (PSPopInteger()) {
	case 0:
	    currentPSContext->scheduler->objectFormat = 0;
	    break;
	case 1:
	case 3:
	    currentPSContext->scheduler->objectFormat = 1;
	    break;
	case 2:
	case 4:
	    currentPSContext->scheduler->objectFormat = 2;
	    break;
    }
}
private procedure PSSetBootstrapPort()
{
    char portName[256];
    port_t newBootstrapPort;
    PSObject str;
    
    if (!currentPSContext->scheduler->login) PSInvlAccess();
    PSPopTempObject(dpsStrObj, &str);
    
    /* Copy string into tempName and null-terminate */
    if (str.length > sizeof(portName)) PSRangeCheck();
    bcopy(str.val.strval,portName,str.length);
    portName[str.length] = 0;
    
    /* Get the new port */
    if (netname_look_up(name_server_port,"",portName,&newBootstrapPort)
    	== NETNAME_SUCCESS)
    {
    	/* Remove the name from the nmserver now that it's been gotten */
    	netname_check_out(name_server_port,portName,PORT_NULL);
	/* And make that new port our bootstrap port */
	if (   task_set_bootstrap_port(task_self(),newBootstrapPort)
	    != KERN_SUCCESS)
	    PSLimitCheck();
	/* Now we must set our process's global bootstrap_port variable. */
	bootstrap_port = newBootstrapPort;
    }
    else
    	PSLimitCheck();
} /* PSSetBootstrapPort */

private readonly RegOpEntry cmdListener[] = {
    "setbootstrapport", PSSetBootstrapPort,
    "listener", PSListener,
    "setlogincontext", PSSetLoginContext,
    "loginexit", PSLoginExit,
    "setpubliclistener", PSSetPublicListener,
    "setuser", PSSetUser,
    "resetuser", PSResetUser,
    "currentuser", PSCurrentUser,
    "setjobuser", PSSetJobUser,
    "setwriteblock", PSSetWriteBlock,
    "currentwriteblock", PSCurrentWriteBlock,
    "setnextobjectformat", PSSetNextObjectFormat,
    "setnextstepcontext", PSSetNextStepContext,
    "currentnextstepcontext", PSCurrentNextStepContext,
    NIL
    };

public procedure ListenerInit(int reason)
{
    switch (reason) {
	case 0:
	    loginContextSet = false;
	    globalUserSet = false;
	    break;
	case 1:
	    PSRgstOps(cmdListener);
	    break;
    }
}




