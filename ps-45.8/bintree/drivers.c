/*****************************************************************************

    drivers.c

    CONFIDENTIAL
    Copyright (c) 1989 NeXT, Inc.
    All Rights Reserved.

    Created 18Sep89 Ted

    Modified:

    29Sep89  Ted   Fully functioning dynaloading system in windowserver
    04Apr90  Ted   Now calls DynaLoadFile/DynaUnloadFile in dynaload.c    
    05Apr90  Ted   Complete rewrite to perform screen initialization
    11Apr90  Ted   Added dummy routine WSStartDriver() which developers can
    		   use to break at in gdb.  This is just before a loaded
		   driver is called.
    23Apr90  Ted   Placed string constants as macros in bintreetypes.h
    24Apr90  Ted   If no zero screen exists, adjusts alls screens to make one
    26Apr90  Ted   Optimizations and zero adjust screen code
    04May90  Ted   Fixed bug: used to netCount++ if netinfo error occured
    03Aug90  Ted   Forgot to increment driverCount. Fixed. Optimizations.
    20Sep90  Ted   Plugged leaks in LoadAllDrivers and LoadAllUnrefDrivers.
    20Sep90  Ted   Merged LoadAllUnrefDrivers and LoadAllDrivers to save space.
    08Oct90  Ted   Added PingAsyncDrivers().
    12Oct90  Ted   Added WriteDefaultNetInfo() to write MegaPixel entry.

******************************************************************************/

#import <string.h>
#import <sys/types.h>	/* for scandir() */
#import <sys/dir.h>	/* also for scandir() */
#import <sys/stat.h>
#import <sys/syslog.h>
#import <netinfo/ni.h>
#import <sys/loader.h>
#import PACKAGE_SPECS
#import CUSTOMOPS
#import PUBLICTYPES
#import "bintreetypes.h"

/*
 * Syslog message encoding for drivers.  Force msg into /usr/adm/messages for
 * use by Customer Support in debugging wierd problems.
 * In the ideal world, this would be LOG_NOTICE, but in spite of the
 * entry in /etc/syslog.conf, it must be LOG_ERR to work correctly.
 */
#define LOG_PS_DRIVER	(LOG_ERR)
#define MEGAPIXEL	"MegaPixel"

extern int (*DynaLoadFile(char *, char *, char *, struct mach_header **))();
extern void DynaUnloadFile(int);
extern int MPStart(NXDriver *);
extern int initialDepthLimit;

void NXStartDriver(char *name);

/* Drivers must fill in name, slot, unit, size and driver. */
typedef struct {
    char	*name;		/* product name */
    short	slot;		/* slot number */
    short	unit;		/* unit number within this slot */
    short	width;
    short	height;
    Bounds	b;		/* global bounds in device coordinates */
    NXDriver	*driver;	/* associated driver */
    unsigned	active:1;	/* whether or not monitor should be used */
    unsigned	matched:1;	/* if this has been matched up */
} NXMonitor;

static const char *displayPath = NEXTSTEP_DISPLAY_PATH;
static NXMonitor **mon, **net;
static int monCount, netCount;

/*****************************************************************************
    FreeMon - Frees a monitor array and all elements.  If the
    freeName flag is set, then the NXMonitor->name fields will be freed as
    well.  The mon reflects "reality" so to speak, and drivers will
    pass a pointer to a constant string which should not be deallocated, but
    the net mallocs string names.
******************************************************************************/
static void FreeMon(NXMonitor **array, int count, int freeName)
{
    int i;
    NXMonitor *m;

    for (i=0; i<count; i++) {
        m = array[i];
	if (freeName) free(m->name);
	free(m);
    }
    free(array);
}


/*****************************************************************************
    LoadSingleDriver - Attempts to dynamically load the driver specified.
    There must exist a directory by this name in /usr/lib/NextStep/Displays.
    A file "reloc" must be present in this directory.
******************************************************************************/
static int LoadSingleDriver(char *name)
{
    struct stat buf;
    NXDriver *driver;
    int error, (*start)(), resident = 0;
    static int residentloaded = 0;
    struct mach_header *header = NULL;
    char relocFile[256], debugFile[256], *debugPtr = NULL;

    if (strcmp(name, MEGAPIXEL) == 0) {
        if (residentloaded) return 0;
	resident = residentloaded = 1;
        start = MPStart;
	goto initdriver;
    }
    sprintf(relocFile, "%s/%s/reloc", displayPath, name);
    if (stat(relocFile, &buf)) return 1;  /* object file not found */

    sprintf(debugFile, "%s/%s/debugflag", displayPath, name);
    if (stat(debugFile, &buf) == 0) {
	sprintf(debugFile, "%s/%s/debug", displayPath, name);
	debugPtr = debugFile;
    }
    if ((start = DynaLoadFile(relocFile, debugPtr, START_SYMBOL, &header))
    == 0)
	return 1;	/* Error loading file */

initdriver:
    driver = (NXDriver *) calloc(1, sizeof(NXDriver));
    driver->header = header;
    /* If the loadable driver has the debugflag set, then inform the user
     * that the driver is loaded and ready for debugging.
     */
    if (debugPtr) {
	os_fprintf(os_stderr, "%s driver loaded.\n", name);
	NXStartDriver(name);
    }
    error = (*start)(driver);
    if (error) {
	syslog(	LOG_PS_DRIVER,
		"Problem starting the %s driver. Continuing.\n",
	    	name);
	free(driver);
    } else {
        driver->next = driverList;
	driverList = driver;
	driverCount++;
    }
    if (!resident) DynaUnloadFile(error ? true : false);
    return 0;
}


/*****************************************************************************
    FilterDirProc - Callback function used by scandirs to filter out
    non-directory entries.  Indirectly used by LoadAllDrivers.
******************************************************************************/
static int FilterDirProc(struct direct *d)
{
    char path[256];
    struct stat buf;

    if (d->d_name[0] == '.') return 0;
    strcpy(path, displayPath);
    strcat(path, "/");
    strcat(path, d->d_name);
    if (stat(path, &buf) < 0)
	return 0;
    return buf.st_mode & S_IFDIR;
}


/*****************************************************************************
    LoadAllDrivers - Attempts to load all drivers that can be found in the
    /usr/lib/NextStep/Displays directory.  No status returned.  If the unref
    flag is set, then only all unreferenced drivers will be loaded.
    SIDE EFFECT:  mon and monCount are updated.
******************************************************************************/
static void LoadAllDrivers(int unref)
{
    int i, j, dirs, found;
    struct direct **dirlist;

    /* Find out how many directories we need to search for drivers */
    dirs = scandir(displayPath, &dirlist, FilterDirProc, alphasort);

    if (dirs >= 0) {
	found = 0;
	/* For each directory, attempt to load a driver. */
	for (i=0; i<dirs; i++) {
	    if (unref) {
		for (j = found = 0; j<netCount; j++)
		    if (strcmp(net[j]->name, dirlist[i]->d_name) == 0) {
			found = 1;
			break;
		    }
	    }
	    if (!found)
		LoadSingleDriver(dirlist[i]->d_name);
	    free(dirlist[i]);
	}
	free(dirlist);
    }
}


/*****************************************************************************
    LoadActiveDrivers - Attempts to load all drivers specified in the
    net that are declared "active".  That is, if at least one screen
    of a given driver type is active, the driver is loaded, else it is not.
    SIDE EFFECT:  mon and monCount are updated.
******************************************************************************/
static int LoadActiveDrivers()
{
    NXDriver *d;
    NXMonitor *n;
    int i, skip, error=0;

    for (i=0; i<netCount; i++) {
        n = net[i];
	if (!n->active) continue;
	skip = 0;
	for (d=driverList; d; d=d->next)
	    if (strcmp(d->name, n->name) == 0) {
		skip = 1;
		break;
	    }
	if (!skip) error |= LoadSingleDriver(n->name);
    }
    return error;
}


/*****************************************************************************
    init_prop - support function for WriteDefaultNetInfo. Simply sets a
    netinfo property variable correctly.
******************************************************************************/
static void init_prop(ni_property *p, char *name, char **v, char *value)
{
    p->nip_name = name;
    p->nip_val.ni_namelist_len = 1;
    v[0] = value;
    p->nip_val.ni_namelist_val = v;
}

/*****************************************************************************
    WriteDefaultNetInfo - The WindowServer wants to make sure an entry exists
    for the MegaPixel whose active flag is turned off.  If the /screens
    directory doesn't exist, we first create one.  Then we create a MegaPixel
    screen entry if one isn't found.  This should not affect other screen
    entries in the netinfo database if the screens dir already exists.
    NOTE: This needs to run as root to actually affect the local domain.
******************************************************************************/
static void WriteDefaultNetInfo()
{	
    ni_status s;
    void *handle;
    ni_property prop;
    ni_proplist props;
    ni_id parent, screens_dir, mp_dir;
    char *name[1], *slot[1], *unit[1], *bounds[1], *active[1];

    if ((s = ni_open(NULL, ".", &handle)) != NI_OK) return;
    if ((s = ni_root(handle, &parent)) != NI_OK) goto done;
    s = ni_pathsearch(handle, &screens_dir, "/screens");
    if (s == NI_OK) {
	/* screens directory exists, now check for MegaPixel entry */
	s = ni_pathsearch(handle, &mp_dir, "/screens/MegaPixel");
	if (s != NI_NODIR) goto done;	/* found or error, bug out */
    } else if (s == NI_NODIR) {
	/* /screens doesn't exist, so create it */
	NI_INIT(&props);
	NI_INIT(&prop.nip_val);
	prop.nip_name = "name";
	ni_namelist_insert(&prop.nip_val, "screens", NI_INDEX_NULL);
	ni_proplist_insert(&props, prop, NI_INDEX_NULL);
	s = ni_create(handle, &parent, props, &screens_dir, 0);
	ni_proplist_free(&props);
    } else goto done;
    /* /screens/MegaPixel doesn't exist, so create its entry */
    props.ni_proplist_len = 5;
    props.ni_proplist_val = calloc(1, sizeof(ni_property) * 5);
    init_prop (&props.ni_proplist_val[0], "name", name, MEGAPIXEL);
    init_prop (&props.ni_proplist_val[1], "slot", slot, "0");
    init_prop (&props.ni_proplist_val[2], "unit", unit, "0");
    init_prop (&props.ni_proplist_val[3], "bounds", bounds, "0 1120 0 832");
    init_prop (&props.ni_proplist_val[4], "active", active, "0");
    s = ni_create(handle, &screens_dir, props, &mp_dir, 0);
done:	
    ni_free(handle);
}

/*****************************************************************************
    ReadNetInfo - Reads the NetInfo database for "screens".  It fills
    in the net. Status is returned: 0 for success, 1 for failure.
******************************************************************************/
static ni_status ReadNetInfo()
{
    ni_id parent, child;
    int j, temp, c, children;
    ni_status s;
    void *handle;
    NXMonitor *n;
    ni_idlist child_list;
    ni_proplist props;
    struct ni_property *p;

    if ((s = ni_open(NULL, ".", &handle)) != NI_OK) return s;
    if ((s = ni_root(handle, &parent)) != NI_OK) goto err;
    if ((s = ni_pathsearch(handle, &parent, "/screens")) != NI_OK) goto err;
    if ((s = ni_children(handle, &parent, &child_list)) != NI_OK) goto err;

    children = child_list.ni_idlist_len;
    for (c=0; c < children; c++) {
	child.nii_object = child_list.ni_idlist_val[c];
	ni_self(handle, &child);
	if ((s = ni_read(handle, &child, &props)) != NI_OK) {
	    os_fprintf(os_stderr, "PS error reading netinfo database. "
	    "Continuing.\n");
	    continue;
	}
	n = net[netCount++] = (NXMonitor *) calloc(1, sizeof(NXMonitor));
	
	for (j=0, p=props.ni_proplist_val; j<props.ni_proplist_len; j++, p++) {
	    if (ni_name_match(p->nip_name, "active"))
	    	n->active = (*p->nip_val.ni_namelist_val[0]=='1');
	    else if (ni_name_match(p->nip_name, "name"))
	        n->name = ni_name_dup(*p->nip_val.ni_namelist_val);
	    else if (ni_name_match(p->nip_name, "slot")) {
	        sscanf(*p->nip_val.ni_namelist_val, "%d", &temp);
		n->slot = (short) temp;
	    } else if (ni_name_match(p->nip_name, "unit")) {
	        sscanf(*p->nip_val.ni_namelist_val, "%d", &temp);
		n->unit = (short) temp;
	    } else if (ni_name_match(p->nip_name, "bounds")) {
	        int minx, maxx, miny, maxy;
	        sscanf(*p->nip_val.ni_namelist_val, "%d %d %d %d",
		    &minx, &maxx, &miny, &maxy);
		n->b.minx = (short) minx;
		n->b.maxx = (short) maxx;
		n->b.miny = (short) miny;
		n->b.maxy = (short) maxy;
	    }
	}
	ni_proplist_free(&props);
    }
    ni_idlist_free(&child_list);
err:
    ni_free(handle);
    return s;
}


/*****************************************************************************
    AssignBounds - Assign a monitor a reasonable global rectangle in the
    workspace. Place it just to the right of the rightmost monitor found.
******************************************************************************/
static void AssignBounds(NXMonitor *a)
{
    NXMonitor *m;
    int i, right, bottom;

    right = 0;
    bottom = a->height;
    /* Look for another monitor to sit next to */
    for (i=0; i<monCount; i++) {
	m = mon[i];
	if (!m->matched || m==a) continue;
	if (m->b.maxx > right) {
	    bottom = m->b.maxy;
	    right = m->b.maxx;
	}
    }
    a->b.minx = right;
    a->b.maxx = right + a->width;
    a->b.maxy = bottom;
    a->b.miny = bottom - a->height;
    a->matched = a->active = 1;
}


/*****************************************************************************
    AssignArrayBounds - Create a screen arrangement from the given monitors.
    	The use of the MegaPixel display is optional, and controlled by
	the UseMegaPixel parameter.
******************************************************************************/
static void AssignArrayBounds(int UseMegaPixel)
{
    int i;

    for (i=0; i<monCount; i++)
    {
    	if ( ! UseMegaPixel && strcmp(mon[i]->name, MEGAPIXEL) == 0 )
	    continue;
    	AssignBounds(mon[i]);
    }
}


/*****************************************************************************
    MatchMonitors - Matches reality against the desired screen configuration.
    First attempts exact matches.  Then matches common driver types.  Then
    assigns positions to those that aren't specified by the screen database.
******************************************************************************/
static void MatchMonitors()
{
    Point p;
    NXMonitor *m, *n;
    int i, j, matched, zerofound;

    p.x = INF;
    zerofound = 0;
    matched = monCount;
    /* Find exact matches for each registered monitor */
    for (i=0; i<monCount; i++) {
        m = mon[i];
	for (j=0; j<netCount; j++) {
	    n = net[j];
	    if (n->matched) continue;
	    if ((m->slot == n->slot) && (m->unit == n->unit) &&
	    (strcmp(m->name, n->name)==0)) {
		/* FOUND AN EXACT MATCH */
		m->matched = n->matched = 1;	/* set matched flag */
		m->b = n->b;			/* copy boudns */
		matched--;			/* one more matched screen */
		m->active = n->active;		/* copy active flag */
		if (m->active) {
		    if (!zerofound) {
			if (m->b.minx==0 && m->b.miny==0)
			    zerofound = 1;
			else
			    if (m->b.minx < p.x) {
				p.x = m->b.minx;
				p.y = m->b.miny;
			    }
		    }
		}
		break;	/* This matched; skip to next registered monitor. */
	    }
	}
    }
    /* If all monitors matched up, we're all done! */
    if (matched == 0) goto adjustscreens;
    
    /* Not all monitors matched so let's take take our best guess. */
    for (i=0; i<monCount; i++) {
        m = mon[i];
	if (m->matched) continue;
	for (j=0; j<netCount; j++) {
	    n = net[j];
	    if (n->matched) continue;
	    if (strcmp(m->name, n->name)==0) {
		m->matched = n->matched = 1;
		m->b = n->b;
		matched--;	/* One more down! */
		m->active = n->active;
		if (m->active) {
		    if (!zerofound)
			if (m->b.minx==0 && m->b.miny==0)
			    zerofound = 1;
			else
			    if (m->b.minx < p.x) {
				p.x = m->b.minx;
				p.y = m->b.miny;
			    }
		}
		break;
	    }
	}
    }
    if (matched == 0) goto adjustscreens;

    /* If best fit doesn't get them all, we'll just assign arbitrary bounds
       to the remaining screens and set them active. */
    for (i=0; i < monCount; i++) {
	m = mon[i];
	if (m->matched) continue;
	m->active = m->matched = 1;
	AssignBounds(m);
    }
    /* If a zero screen was found, then everything's kosher */
adjustscreens:
    if (zerofound) return;
    /* Otherwise, we need to adjust the screens so that the bottom-leftmost
     * screen is the zero screen.
     */
    for (i=0; i<monCount; i++) {
	m = mon[i];
	if (!(m->matched & m->active)) continue;
	OFFSETBOUNDS(m->b, -p.x, -p.y);
    }
}
/*****************************************************************************
    GuaranteeActiveScreen - Guarantee that the screen array contains 
    	at least one active screen.  Inspect the mon[] array until an
	active screen is found, or all screens have been checked.
	If no active screen is present, but an inactive MegaPixel screen
	was found, activate and make the MegaPixel screen the (0,0) 
	screen.
******************************************************************************/
static void GuaranteeActiveScreen()
{
    NXMonitor *m;
    int i;
    
    m = (NXMonitor *)0;	/* To be set to MegaPixel screen */
    for (i = 0; i < monCount; i++) {
    	if ( mon[i]->active )
	    return;		/* There is at least one active screen. */
	if ( m == (NXMonitor *)0 && strcmp(mon[i]->name, MEGAPIXEL) == 0 )
	    m = mon[i];
    }

    /* No active screens, but MegaPixel display is available. */
    if ( m != (NXMonitor *)0 ) {
    	m->active = 1;	/* Activate MegaPixel */
    	m->matched = 1;
	/* Set bounds for (0,0) screen */
	m->b.minx = 0;
	m->b.miny = 0;
	m->b.maxx = m->width;
	m->b.maxy = m->height;
	syslog(	LOG_PS_DRIVER,
		"No active screens? Using MegaPixel display by default.\n");
    }
}

/*****************************************************************************
    SanityCheck - The screen array is checked against the following criteria:
    	1. No overlaps allowed
	2. All screens must be adjacent to at least one screen
	3. All coordinates must be within +/- 16000
	4. Screen bounds must be non-empty and not inside-out
	5. At least one screen must be declared "active"
    NOTE: Only active screens are checked because inactive screens' boundaries
    are meaningless. Status returned: 0 for success, 1 for failure.
******************************************************************************/
static int SanityCheck()
{
    Bounds a, b;
    NXMonitor *n, *m;
    int active, adjacent, i, j;

    for (i=0, active=0; i<netCount; i++) active += net[i]->active;
    if (active==0) return 1;
    for (i=0; i<netCount; i++) {
        m = net[i];
	a = m->b;
	if (!m->active) continue;
	/* Bounds cannot be empty or inside-out */
	if (a.minx >= a.maxx || a.miny >= a.maxy) return 1;
	/* Coordinates must be in valid range */
	if (a.minx < -INF || a.maxx > INF || a.miny < -INF || a.maxy > INF)
	    return 1;
	if (active>1) {
	    for (j=0, adjacent=0; j<netCount; j++) {
	        Bounds dummy;
		n = net[j];
		if (i==j || !n->active) continue;
		b = n->b;
		/* Check Overlap */
		if (IntersectAndCompareBounds(&a, &b, &dummy) != outside)
		    return 1;
		/* Check Adjacency */
		if (a.maxx==b.minx)		/* right edge adjacent? */
		    { if (a.maxy<b.miny || a.miny>b.maxy) return 1; }
		else if (a.minx==b.maxx)	/* left edge adjacent? */
		    { if (a.maxy<b.miny || a.miny>b.maxy) return 1; }
		else if (a.maxy==b.miny)	/* bottom edge adjacent? */
		    { if (a.maxx<b.minx || a.minx>b.maxx) return 1; }
		else if (a.miny==b.maxy)	/* top edge adjacent? */
		    { if (a.maxx<b.minx || a.minx>b.maxx) return 1; }
		else continue;
		adjacent = 1;	/* found at least one adjacent edge */
	    }
	    if (!adjacent) return 1;
	}
    }
    return 0;
}


/*****************************************************************************
    NXRegisterScreen - Device drivers call this function to register all
    screens they are able to control.  It adds a screen to a running list of
    screens that are available for use by the windowserver.
******************************************************************************/
void NXRegisterScreen(NXDriver *driver, short slot, short unit,
    short width, short height)
{
    NXMonitor *m;

    mon[monCount++] = m = calloc(1, sizeof(NXMonitor));
    m->name   = driver->name;
    m->slot   = slot;
    m->unit   = unit;
    m->width  = width;
    m->height = height;
    m->driver = driver;
}

/* The following is a stub for driver writer to break at to debug their
 * drivers.  Allows them to insert breakpoints at this point.
 */
void NXStartDriver(char *name)
{
}

/* NXRegisterOps - Let's drivers register their own PostScript operators in
 * statusdict. DO NOT DELETE THIS ROUTINE.
 */
void NXRegisterOps(NXRegOpsVector r)
{
    for ( ; r->name; r++)
	PSRegisterStatusDict(r->name, r->proc);
}


/*****************************************************************************
    This should only be called ONCE during initialization of the WindowServer.
*****************************************************************************/
void DriverInit()
{
    NXMonitor *m;
    NXDevice *device;
    NXDriver *driver;
    int activeMonCount;
    int i, remapErr;
    ni_status s;

    remapErr = 1;
    deviceList = NULL;
    driverList = NULL;
    driverCount = 0;
    deviceCount = 0;
    monCount = 0;
    netCount = 0;
    mon = calloc(32, sizeof(NXMonitor *));
    net = calloc(32, sizeof(NXMonitor *));
    
    /*
     * Initialize the logging connection, so we can correctly 
     * record syslog LOG_NOTICE messages about non-fatal
     * driver load and configuration problems.
     */
    openlog("WindowServer", 0, LOG_DAEMON);
    
    /* FIX: We need to always initialize the MegaPixel driver before all
     * others, otherwise it fails (due to a tt1 clash). This will be changed
     * soon when we change everything to use pmap instead of tt.
     */
    WriteDefaultNetInfo();
    LoadSingleDriver(MEGAPIXEL);
    if ((s = ReadNetInfo()) || netCount==0 || SanityCheck()) {
    	/* Plan B: When netinfo read or consistency check fail, load
	 * all available drivers and assigns screen bounds arbitrarily.
	 *
	 * If there are screens other than the MegaPixel display, and there
	 * is no NetInfo database, disable the MegaPixel display, as there may
	 * be a Sound Box out there instead of a CRT.  The user can use
	 * Preferences to switch the MP display on.
	 *
	 * Assume all monitors reflected by monCount are to be used.
	 */
    	LoadAllDrivers(0);
	AssignArrayBounds( (monCount > 1) ? 0 : 1 ); /* 1 => Use MegaPixel */
    } else {
	/* Plan A: Things look good so far. Our heuristic is to load
	 * drivers that aren't specified in the NetInfo database and give
	 * them arbitrary bounds in the screen layout.
	 */
	LoadAllDrivers(1);
	/* Now try to load all drivers that were specified. If any errors
	 * returned, then assign arbitrary bounds to every screen.
	 */
	if (LoadActiveDrivers())
	    AssignArrayBounds(1);
	else
	    MatchMonitors();
    }

    /*
     * Scan the mon for matched or active screens to initialize.
     */
    GuaranteeActiveScreen();	/* Force at least 1 screen to be active. */
    initialDepthLimit = 0;
    for (i = 0; i < monCount; i++) {
	m = mon[i];
	/* If umatched or inactive screen, then punt on this entry */
	if (!m->matched || !m->active)
	    continue;
	/* Create a device structure and add it to the deviceList */
	device = (NXDevice *) calloc(1, sizeof(NXDevice));
	device->next = deviceList;
	deviceList = device;
	device->bounds = m->b;
	device->driver = m->driver;
	device->slot = m->slot;
	device->unit = m->unit;
	
	/* Call the device's driver to initialize it */
	(*device->driver->procs->InitScreen)(device);
	
	if (device->visDepthLimit > initialDepthLimit) {
	    initialDepthLimit = device->visDepthLimit;
	    holeDevice = device;
	}

	/* Set global flag if at least one driver has wants a Ping message */
	asyncDriversExist |= (int)device->driver->procs->Ping;
	/* Make room in wsBounds for the new device's bounds */
	if (m->b.minx == 0 && m->b.miny == 0) {
	    remapY = m->b.maxy;
	    remapErr = 0;
	}
	if (deviceCount++)
	    boundBounds(&device->bounds, &wsBounds, &wsBounds);
	else
	    wsBounds = device->bounds;
    }
    if (deviceCount == 0) {
	os_fprintf(os_stderr, "PS error: No screen drivers were loaded. "
	"Exiting.\n");
	exit(1);
    }
    if (remapErr) {
	os_fprintf(os_stderr, "PS error: No zero screen. Exiting.\n");
        exit(1);
    }
    if (holeDevice == NULL) holeDevice = device;
    EXInitialize();
    /* Free arrays of NXMonitor struct ptrs */
    FreeMon(mon, monCount, 0);
    FreeMon(net, netCount, 1);
}

/* Called from ipcstream.c to poll all framebuffers that respond to Ping */
void PingAsyncDrivers()
{
    void (*proc)();
    NXDevice *device;
    
    for (device = deviceList; device; device = device->next)
      if (proc = device->driver->procs->Ping)
          (*proc)(device);
}






