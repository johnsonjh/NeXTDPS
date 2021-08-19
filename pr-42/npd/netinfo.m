/*
 * netinfo.m	- Routines to deal with the NetInfo database.
 *
 * Copyright 1988, 1990 (c) by NeXT, Inc.  All rights reserved.
 */

/*
 * Include files.
 */
#import "NpdDaemon.h"
#import	"log.h"
#import "netinfo.h"

#import <stddef.h>
#import <string.h>
#import <netinfo/ni.h>

/*
 * Constants
 */

/* NetInfo directory constants */
static char *NIDIR_PRINTERS = "/printers";
static char *LOCAL_PRINTER = LOCALNAME;

/* Property names */
static char *NIPROP_IGNORE = IGNORE;

/* Default properties */
static char *DEFAULT_LP = "/dev/null";
static char *DEFAULT_TY = "NeXT 400 dpi Laser Printer";
static char *DEFAULT_SD = "/usr/spool/NeXT/Local_Printer";
static char *DEFAULT_IF = "/usr/lib/NextPrinter/npcomm";
static char *DEFAULT_GF = "/usr/lib/NextPrinter/psgf";
static char *DEFAULT_NF = "/usr/lib/NextPrinter/psnf";
static char *DEFAULT_TF = "/usr/lib/NextPrinter/pstf";
static char *DEFAULT_RF = "/usr/lib/NextPrinter/psrf";
static char *DEFAULT_VF = "/usr/lib/NextPrinter/psvf";
static char *DEFAULT_CF = "/usr/lib/NextPrinter/pscf";
static char *DEFAULT_DF = "/usr/lib/NextPrinter/psdf";
static char *DEFAULT_NOTE = "";

/* Error strings */
static struct string_entry	_strings[] = {
    { "NI_openfailed", "can't open local NetInfo domain: %s" },
    { "NI_pathfailed", "NetInfo error during pathsearch: %s" },
    { "NI_lupropfailed", "NetInfo error during lookupprop: %s" },
    { "NI_crpropfailed", "NetInfo error creating property: %s" },
    { "NI_readfailed", "NetInfo error reading directory: %s" },
    { "NI_propdstryfail", "NetInfo error destroying property: %s" },
    { "NI_createfailed", "NetInfo error creating directory: %s" },
    { "NI_wrpropfailed", "NetInfo error writing property: %s" },
    { NULL, NULL },
};

/*
 * Local variables.
 */
static BOOL	_initdone = NO;


/*
 * Static routines.
 */

/**********************************************************************
 * Routine:	_niInit() - Initialize this module.
 *
 * Function:	Loads the error strings into the StringManager.
 **********************************************************************/
static void
_niInit()
{
    if (!_initdone) {
	[NXStringManager loadFromArray:_strings];
	_initdone = YES;
    }
}

/**********************************************************************
 * Routine:	_propAdd() - Add a property to a proplist.
 *
 * Args:	pl:	The property list
 *		key:	The key of the new property
 *		val1:	The (optional) first value of the new property
 *		val2:	The (optional) second value of the new property.
 **********************************************************************/
static void
_propAdd(ni_proplist *pl, char *key, char *val)
{
    ni_property prop;
  
    NI_INIT(&prop);
    prop.nip_name = key;
    if (val != NULL) {
	ni_namelist_insert(&prop.nip_val, val, NI_INDEX_NULL);
    }
    ni_proplist_insert(pl, prop, NI_INDEX_NULL);
    ni_namelist_free(&prop.nip_val);
}

/**********************************************************************
 * Routine:	_proplistDefaultInit() - Load a proplist with default info.
 *
 * Function:	This routine loads a property list with the default properties
 *		of the local NeXT printer.
 *
 * Args:	pl:	The currently empty property list to be loaded.
 **********************************************************************/
static void
_proplistDefaultInit(ni_proplist *pl)
{

    /* Initialize the property list */
    NI_INIT(pl);

    /*
     * We put "Local_Printer" in twice because if someone changes
     * the name, we still want to be able to find this particular
     * printerdb entry easily.
     */
    _propAdd(pl, "name", LOCAL_PRINTER);
    _propAdd(pl, "lp", DEFAULT_LP);
    _propAdd(pl, "ty", DEFAULT_TY);
    _propAdd(pl, "sd", DEFAULT_SD);
    _propAdd(pl, "if", DEFAULT_IF);
    _propAdd(pl, "gf", DEFAULT_GF);
    _propAdd(pl, "nf", DEFAULT_NF);
    _propAdd(pl, "tf", DEFAULT_TF);
    _propAdd(pl, "rf", DEFAULT_RF);
    _propAdd(pl, "vf", DEFAULT_VF);
    _propAdd(pl, "cf", DEFAULT_CF);
    _propAdd(pl, "df", DEFAULT_DF);
    _propAdd(pl, "note", DEFAULT_NOTE);
}


/*
 * External routines.
 */

/**********************************************************************
 * Routine:	NIDirectoryName()
 *
 * Function:	Return a malloc'd version of the name property of
 *		a NetInfo directory in the local domain.  The directory
 *		is specified by a path.  If the directory doesn't
 *		exist, we return NULL.
 **********************************************************************/
char *
NIDirectoryName(const char *path)
{
    void	*ni;
    ni_id	id;
    ni_status	status;
    ni_namelist	namelist;
    const char	*cp;
    char	*ret;

    _niInit();

    /* Get a handle on the local NetInfo domain */
    if ((status = ni_open(NULL, ".", &ni)) != NI_OK) {
	LogError([NXStringManager stringFor:"NI_openfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }
    
    /* Search for the appropriate directory */
    if ((status = ni_pathsearch(ni, &id, path)) != NI_OK) {
	ni_free(ni);
	if (status != NI_NODIR) {
	    LogError([NXStringManager stringFor:"NI_pathfailed"],
		     ni_error(status));
	    NX_RAISE(NPDnetinfofailed, NULL, NULL);
	}
	return (NULL);
    }

    /* Get the name property */
    if ((status = ni_lookupprop(ni, &id, "name", &namelist)) != NI_OK) {
	ni_free(ni);
	LogError([NXStringManager stringFor:"NI_lupropfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }

    /* Make a copy of the last name in the list */
    cp = namelist.ninl_val[namelist.ninl_len - 1];
    ret = (char *) malloc(strlen(cp) + 1);
    strcpy(ret, cp);

    ni_namelist_free(&namelist);
    ni_free(ni);
    return (ret);
}

/**********************************************************************
 * Routine:	NINextPrinterCreate()
 *
 * Function:	Create a NetInfo directory for the local printer.
 *		Return a malloc'd version of the name.
 **********************************************************************/
char *
NINextPrinterCreate()
{
    void	*ni;
    ni_proplist pl;
    ni_status status;
    ni_id newid;
    ni_id neti_id;
    char	*cp;

    _niInit();

    /* Get a handle on the local NetInfo domain */
    if ((status = ni_open(NULL, ".", &ni)) != NI_OK) {
	LogError([NXStringManager stringFor:"NI_openfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }
    
    /* Get the id for /printers */
    if ((status = ni_pathsearch( ni, &neti_id, NIDIR_PRINTERS)) != NI_OK) {
	LogError([NXStringManager stringFor:"NI_pathfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }

    /* Build the property list */
    _proplistDefaultInit(&pl);

    /* Create the directory */
    if ((status = ni_create(ni, &neti_id, pl, &newid,
			    NI_INDEX_NULL)) != NI_OK) {
	ni_proplist_free(&pl);
	LogError([NXStringManager stringFor:"NI_createfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }
    ni_proplist_free(&pl);

    cp = (char *) malloc(strlen(LOCAL_PRINTER) + 1);
    strcpy(cp, LOCAL_PRINTER);
    return (cp);
}

/**********************************************************************
 * Routine:	NIEntryDisable() - Disable local printer NetInfo dir
 *
 * Function:	This routine sets the "_ignore" property in the
 *		NetInfo directory for the local printer with a given
 *		path.  This effectively makes it unavailable for use.
 *		If the directory doesn't exist, we consider it
 *		disabled.
 *
 * Caveat:	This should be called within an NX_DURING error handler.
 **********************************************************************/
void
NIPrinterDisable(const char *path)
{
    void	*ni;
    ni_id	id;
    ni_status	status;
    ni_namelist	namelist;
    ni_property	property;

    _niInit();

    /* Get a handle on the local NetInfo domain */
    if ((status = ni_open(NULL, ".", &ni)) != NI_OK) {
	LogError([NXStringManager stringFor:"NI_openfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }
    
    /* Search for the appropriate directory */
    if ((status = ni_pathsearch(ni, &id, path)) != NI_OK) {
	ni_free(ni);
	if (status != NI_NODIR) {
	    LogError([NXStringManager stringFor:"NI_pathfailed"],
		     ni_error(status));
	    NX_RAISE(NPDnetinfofailed, NULL, NULL);
	}
	/* No directory, consider it disabled */
	return;
    }

    /* See if the ignore property is already set */
    if ((status = ni_lookupprop(ni, &id, NIPROP_IGNORE, &namelist)) == NI_OK) {

	/* Already set, just return */
	ni_free(ni);
	ni_namelist_free(&namelist);
	return;
    }

    /* Was the error expected? */
    if (status != NI_NOPROP) {
	/* No, report it */
	ni_free(ni);
	LogError([NXStringManager stringFor:"NI_lupropfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }
    
    /* Create a new ignore property */
    NI_INIT(&property);
    property.nip_name = (char *) NIPROP_IGNORE;
    if ((status = ni_createprop(ni, &id, property, NI_INDEX_NULL)) != NI_OK) {
	ni_free(ni);
	LogError([NXStringManager stringFor:"NI_crpropfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }
    ni_free(ni);
}

/**********************************************************************
 * Routine:	NIEntryEnable() - Enable a local printer NetInfo dir
 *
 * Function:	This routine clears the "_ignore" property in the
 *		NetInfo directory for a local printer specified by
 *		path.
 *
 * Caveat:	This should be called within an NX_DURING error handler.
 **********************************************************************/
void
NIPrinterEnable(const char *path)
{
    void	*ni;
    ni_id	id;
    ni_status	status;
    ni_proplist	proplist;
    ni_index	where;

    _niInit();

    /* Get a handle on the local NetInfo domain */
    if ((status = ni_open(NULL, ".", &ni)) != NI_OK) {
	LogError([NXStringManager stringFor:"NI_openfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }
    
    /* Search for the appropriate directory */
    if ((status = ni_pathsearch(ni, &id, path)) != NI_OK) {
	ni_free(ni);
	LogError([NXStringManager stringFor:"NI_pathfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }

    /* Get its properties */
    if ((status = ni_read(ni, &id, &proplist)) != NI_OK) {
	ni_free(ni);
	LogError([NXStringManager stringFor:"NI_readfailed"],
		 ni_error(status));
	NX_RAISE(NPDnetinfofailed, NULL, NULL);
    }

    /* See if it has an ignore property */
    if ((where = ni_proplist_match(proplist, NIPROP_IGNORE, NULL)) !=
	NI_INDEX_NULL) {

	/* It does, destroy it */
	if ((status = ni_destroyprop(ni, &id, where)) != NI_OK) {
	    ni_free(ni);
	    ni_proplist_free(&proplist);
	    LogError([NXStringManager stringFor:"NI_propdstryfail"],
		     ni_error(status));
	    NX_RAISE(NPDnetinfofailed, NULL, NULL);
	}
    }
    ni_free(ni);
    ni_proplist_free(&proplist);
}

