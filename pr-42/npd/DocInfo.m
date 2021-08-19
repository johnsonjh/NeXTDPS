/*
 * DocInfo.m	- Implementation of the DocInfo object.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */


/*
 * Include files.
 */

#import "DocInfo.h"
#import "NpdDaemon.h"
#import "log.h"
#import "atomopen.h"

#import <ctype.h>
#import <libc.h>
#import <mach.h>
#import <stdlib.h>
#import <string.h>
#import <sys/file.h>
#import <sys/stat.h>

/*
 * Types.
 */
typedef struct {
    char	*string;
    version_t	version;
} PS_table_t;


/*
 * Constants.
 */

/* Available versions of postscript */
static PS_table_t PS_table[] = {
    { "PS-Adobe-1.0", PS_Adobe_10 },
    { "PS-Adobe-2.0", PS_Adobe_20 },
    { "PS-Adobe-2.1", PS_Adobe_20 },	/* Document revision of 2.0 */
    { "PS-Adobe-", PS_Adobe_New },
    { NULL, 0 },
};

/* error strings */
static struct string_entry _strings[] = {
    { "DocInfo_nomagic", "no magic number" },
    { "DocInfo_nopen", "couldn't open %s: %s" },
    { "DocInfo_nomap", "coundn't map file %s: error = %d" },
    { "DocInfo_vmdeallocate",
	  "DocInfo couldn't vm_deallocate mapped file, error = %d" },
    { "DocInfo_badbounds", "invalid bounding box: %s" },
    { "DocInfo_badpages", "invalid pages argument: %s" },
    { "DocInfo_badpageorder", "invalid page order: %s" },
    { "DocInfo_badres", "invalid resolution: %s" },
    { "DocInfo_badcustmpaper", "invalid custom paper comment: %s" },
    { "DocInfo_atendmismatch", "document (atend) comment left unresolved" },
    { NULL, NULL },
};


/*
 * Macros
 */
#define SKIPSPACE(cp) { \
	while(*(cp) && isspace(*(cp))) \
		(cp)++; \
}

#define	STRNDUP(cp, str, len) { \
	if (len <= 0) { \
		cp = (char *) malloc(1); \
		*cp = '\0'; \
	} else { \
		cp = (char *) malloc(len + 1); \
		cp[len] = '\0'; \
		strncpy(cp, str, len); \
	} \
}


/*
 * Local variables.
 */
static BOOL _initdone = NO;


/*
 * Local routines.
 */

/**********************************************************************
 * Routine:	_strMatch() - Match and skip a string.
 *
 * Function:	If string1 matches string2 skip over string1.
 *
 * Args:	string1: string to check and skip
 *		string2: string to match against.
 *
 * Returns:	pointer to character after match in string1.
 **********************************************************************/
static char *
_strMatch(char *string1, char *string2)
{
    int		len;			/* The length of the match */

    len =  strlen(string2);
    if (strncmp(string1, string2, len)) {
	return (NULL);
    }
    return (string1 + len);
}

/**********************************************************************
 * Routine:	_nextLine() - Skip to the next line in the data.
 *
 * Function:	Look for the beginning of the next line of data
 *		that starts before the maximum character.
 *
 * Args:	cp:	pointer to current position
 *		max:	pointer to first invalid character
 *
 * Returns	max:		if there were no more lines
 *		otherwise:	pointer to first character on next line
 **********************************************************************/
static char *
_nextLine(char *cp, char *max)
{
    while(cp < max && *cp != '\n') {
	cp++;
    }

    if( *cp == '\n' ) {
	cp++;
    }

    return (cp);
}

/**********************************************************************
 * Routine:	_skipBinary() - Skip to end of binary data.
 *
 * Function:	Parse %%BeginBinary: NBytes and skip that many bytes
 *
 * Args:	cp:	pointer to current position == %%BeginBinary
 *		max:	pointer to first invalid character
 *
 * Returns	max:		if there were no more lines
 *		otherwise:	pointer to first character past binary
 **********************************************************************/
static char *
_skipBinary(char *cp, char *max)
{
    char argument[257];
    int byteCount;
    
    byteCount = 0;
    cp += 14; /* strlen("%%BeginBinary:") */
    while(cp < max && *cp != '\n') {
        if (byteCount < 255)
	    argument[byteCount++] = *cp;
	cp++;
    }
    argument[byteCount] = '\0';
    byteCount = atoi(argument);
    cp += (byteCount+1);
    return (cp > max) ? max : cp;
}

/**********************************************************************
 * Routine:	_headerVersion() - Decode the PostScript version
 *
 * Function:	Parse the first line in the header, the one that
 *		starts with "%!" and determine which version of
 *		PostScript it is.
 *
 * Args:	cp:	pointer to the first line.
 *
 * Returns:	PostScript Version.
 *
 * Caveat:	Should be called within an NX_DURING block.
 **********************************************************************/
static version_t
_headerVersion(char *cp)
{
    PS_table_t	*ptp;

    if (strncmp(cp,"%!", 2)) {
	/* Ack! No magic number */
	NX_RAISE(NPDdocformat, [NXStringManager stringFor:"DocInfo_nomagic"],
				cp);
    }
    cp += 2;

    /* Scan the table for our version */
    for (ptp = PS_table; ptp->string; ptp++) {
	if (_strMatch(cp, ptp->string)) {
	    return (ptp->version);
	}
    }

    return (PS_NonConforming);
}
    
/**********************************************************************
 * Routine:	_readComplexProp() - Parse an open ended property
 *
 * Function:	This routine parses an open ended property.  The
 *		arguments can be grouped in tuples.  There is no
 *		limit on the length of the properties.  This routine
 *		will check for continuation lines.
 *
 * Args:	prop:	The property to add the arguments to.
 *		argp:	The current character pointer into the buffer.
 *		ntuple:	The size of the argument tuple: 3 == triplets.
 *
 * Returns:	Pointer to end of the last line parsed.
 **********************************************************************/
static char *
_readComplexProp(prop_t *prop, char *argp, char *eobuf, int ntuple)
{
    char	*eolp;			/* pointer to end of line */
    char	*cp;			/* generic pointer */
    char	**cpp;
    int		i;			/* generic counter */
    BOOL	inArg;
    int		nargs;			/* The number of new args */

    while (TRUE) {

	/* Get past the white-space */
	SKIPSPACE(argp);

	/* Find out where the end-of line is. */
	if ((eolp = strchr(argp, '\n')) == NULL) {
	    eolp = argp + strlen(argp);
	}

	if (_strMatch(argp, "(atend)")) {
	    prop->p_valsatend = YES;
	    return (eolp);
	}

	/* Count up the args */
	cp = argp;
	inArg = NO;
	i = 0;
	while (cp < eolp) {
	    if (!isspace(*cp) && !inArg) {
		i++;
		inArg = YES;
	    }
	    if (isspace(*cp) && inArg) {
		inArg = NO;
	    }
	    cp++;
	}

	/* Make more room for the args */
	nargs = (i + ntuple - 1) / ntuple;
	cpp = (char **) malloc((nargs + prop->p_nvals) * sizeof(char *));
	if (prop->p_vals) {
	    bcopy(prop->p_vals, cpp, prop->p_nvals * sizeof (char *));
	    free(prop->p_vals);
	}
	prop->p_vals = cpp;

	/* Copy in the args */
	for (cpp = &prop->p_vals[prop->p_nvals];
	     cpp < &prop->p_vals[prop->p_nvals + nargs]; cpp++) {
	    SKIPSPACE(argp);
	    cp = argp;
	    inArg = NO;
	    i = ntuple;
	    /* Find the end of this arg */
	    while ((i || inArg) && cp < eolp) {
		if (!isspace(*cp) && !inArg) {
		    i--;
		    inArg = YES;
		}
		if (isspace(*cp) && inArg) {
		    inArg = NO;
		}
		cp++;
	    }

	    /* Allocate a new argument */
	    *cpp = (char *) malloc(cp - argp + 1);
	    strncpy(*cpp, argp, cp - argp);
	    (*cpp)[cp - argp] = '\0';

	    argp = cp;
	}
	prop->p_nvals += nargs;

	/* Now check the next line to see if we want to use it */
	argp = _nextLine(eolp, eobuf);
	if ((argp = _strMatch(argp, "%%+")) == NULL) {
	    return (eolp);
	}
    }
    /* Never gets here */
}


/*
 * Method definitions.
 */

@implementation DocInfo : Object

/* Factory methods */
+ new
{
    if (!_initdone) {
	[NXStringManager loadFromArray:_strings];
	_initdone = YES;
    }

    self = [super new];
    lock = mutex_alloc();
    refcnt = 1;

    /* Initialize header information */
    title = NULL;
    creator = NULL;
    creationDate = NULL;
    forUser = NULL;
    routing = NULL;
    printerRequired = NULL;
    proofMode = PM_Unknown;
    bzero(&boundBox, sizeof (boundBox));
    pages = 0;
    pageOrder = 1;			/* Default to ascending order */
    pageOrderUnknown = 1;		/* But we don't really know */
    complexProps = NULL;
    atEnd = 0;
    bboxAtEnd = 0;
    pagesAtEnd = 0;
    pageOrderAtEnd = 0;

    /* Initialize default resource information */
    bzero(&paper, sizeof(paper));
    resolution = 400;
    manualfeed = NO;

    return (self);
}

+ newFromFile:(const char *) fileName size:(int) size
{
    int		fd;			/* file descriptor */
    char	*addr;
    char	*cp;
    char	*linep;
    char	*eobuf;
    char	*eolp;
    kern_return_t	ret_code;

    /* Open the file */
    if ((fd = atom_open(fileName, O_RDONLY, 0)) < 0) {
	LogWarning([NXStringManager stringFor:"DocInfo_noopen"],
		   fileName, strerror(cthread_errno()));
	return (nil);
    }

    /* Map the file into VM */
    if ((ret_code = map_fd(fd, 0, (vm_offset_t *)&addr, TRUE,
			   size)) != KERN_SUCCESS) {
	LogWarning([NXStringManager stringFor:"DocInfo_nomap"],
		   fileName, ret_code);
	(void) close(fd);
	return (nil);
    }
    (void) close(fd);

    /* Make sure this is a PostScript file */
    if (addr[0] != '%' || addr[1] != '!') {
	if ((ret_code = vm_deallocate(task_self(), (vm_offset_t) addr,
				      (vm_size_t) size))
	    != KERN_SUCCESS) {
	    LogError([NXStringManager stringFor:"DocInfo_vmdeallocate"],
		     ret_code);
	    [NXDaemon panic];
	}
	return (nil);
    }

    /* Instantiate our self */
    self = [self new];

    /*
     * Look for the end of the header which will be the beginning of the
     * first page or of the trailer.
     */
    linep = addr;
    eobuf = addr + size;
    while (linep < eobuf) {

	/* Get a pointer to the end of this line */
	if ((eolp = strchr(linep, '\n')) == NULL) {
	    eolp = linep + strlen(linep);
	}

	/* Check that this is a comment */
	if (strncmp(linep, "%%", 2)) {
	    /* No comment, skip */
	    linep = _nextLine(eolp, eobuf);
	    continue;
	}

	if (_strMatch(linep, "%%Page:") ||
	    _strMatch(linep, "%%Trailer")) {
	    /* This is the end of the header, break out of the loop */
	    break;
	}
	if (_strMatch(linep, "%%BeginBinary:")) /* skip binary data */
	     eolp = _skipBinary(linep,eobuf);
	linep = _nextLine(eolp, eobuf);
    }

    /* Parse the header */
    NX_DURING {
	[self parseHeaderFor:nil data:addr size: (linep - addr)];
    } NX_HANDLER {
	if ((ret_code = vm_deallocate(task_self(), (vm_offset_t) addr,
				      (vm_size_t) size))
	    != KERN_SUCCESS) {
	    LogError([NXStringManager stringFor:"DocInfo_vmdeallocate"],
		     ret_code);
	    [NXDaemon panic];
	}
	NX_RERAISE();
    } NX_ENDHANDLER;

    /* Now parse the trailer */
    [self parseTrailerFor:nil data:linep size:eobuf - linep];

    /* Free up the mapped file */
    if ((ret_code = vm_deallocate(task_self(), (vm_offset_t) addr,
				  (vm_size_t) size))
	!= KERN_SUCCESS) {
	LogError([NXStringManager stringFor:"DocInfo_vmdeallocate"],
		 ret_code);
	[NXDaemon panic];
    }

    return (self);
}

- parseHeaderFor:(id)printJob data:(char *)data size:(int) size
{
    char	*linep;			/* Pointer to the BOL */
    char	*argp;			/* Pointer to argument of comment */
    char	*eolp;			/* Pointer to end of current line */
    char	*eobuf;			/* Pointer to end of buffer */
    prop_t	*newProp;		/* pointer for creating props */
    char	*cp;			/* generic character pointer */
    int		filedepth = 0;		/* file recursion depth */
    float	tllx, tlly, turx, tury;	/* Temp bounds entries */

    /* Start at the very beginning */
    linep = data;
    eobuf = data + size;
    
    mutex_lock(lock);

    /* Figure out which version of PostScript */
    version = _headerVersion(linep);
    linep = _nextLine(linep, eobuf);

    /* If non-conforming, go check for richwill hacks */
    if (version == PS_NonConforming) {
	goto endcomments;
    }

    while (linep < eobuf) {

	/* Check that this is a comment */
	if (strncmp(linep, "%%", 2)) {
	    /* No comment, we must be at the end of the header */
	    break;
	}
	
	if (_strMatch(linep, "%%BeginBinary:")) { /* skip binary data */
	     linep = _nextLine(_skipBinary(linep,eobuf),eobuf);
	     continue;
	}
	
	linep += 2;			/* Skip the "%%" */
	
	/* Get a pointer to the end of this line */
	if ((eolp = strchr(linep, '\n')) == NULL) {
	    eolp = linep + strlen(linep);
	}
	
	/* Check for conforming comments */
	switch (version) {
	  case PS_Adobe_New:
	  case PS_Adobe_20:
	    
	    /* Check if we are entering an included file */
	    if (_strMatch(linep, "BeginDocument:") ||
		_strMatch(linep, "BeginFile:")) {
		filedepth++;
		break;
	    }
	    
	    if (_strMatch(linep, "EndDocument") ||
		_strMatch(linep, "EndFile")) {
		filedepth--;
		break;
	    }
	    
	    /* If we are in an included file, ignore this line */
	    if (filedepth) {
		break;
	    }
	    
	    /* Feedback routing */
	    if (argp = _strMatch(linep, "Routing:")) {
		if (!routing) {
		    SKIPSPACE(argp);
		    STRNDUP(routing, argp, eolp - argp);
		}
		break;
	    }
	    
	    /* Printer requirements */
	    if (argp = _strMatch(linep, "DocumentPrinterRequired:")) {
		if (!printerRequired) {
		    SKIPSPACE(argp);
		    STRNDUP(printerRequired, argp, eolp - argp);
		}
		break;
	    }
	    
	    /* Proof Mode */
	    if (argp = _strMatch(linep, "ProofMode:")) {
		if (proofMode == PM_Unknown) {
		    SKIPSPACE(argp);
		    if (strncmp(argp, "TrustMe", strlen("TrustMe")) == 0) {
			proofMode = PM_TrustMe;
		    } else if (strncmp(argp, "Substitute",
				       strlen("Substitute")) == 0) {
			proofMode = PM_Substitute;
		    } else if (strncmp(argp, "NotifyMe",
				       strlen("NotifyMe")) == 0) {
			proofMode = PM_NotifyMe;
		    }
		}
		break;
	    }
	    
	    /* Complex props with one element lists */
	    if ((argp = _strMatch(linep, "Requirements:")) ||
		(argp = _strMatch(linep, "DocumentNeededFonts:")) ||
		(argp = _strMatch(linep, "DocumentSuppliedFonts:")) ||
		(argp = _strMatch(linep, "DocumentNeededFiles:")) ||
		(argp = _strMatch(linep, "DocumentSuppliedFiles:")) ||
		(argp = _strMatch(linep, "DocumentPaperSizes:")) ||
		(argp = _strMatch(linep, "DocumentPaperForms:")) ||
		(argp = _strMatch(linep, "DocumentPaperColors:")) ||
		(argp = _strMatch(linep, "DocumentPaperWeights:")) ||
		(argp = _strMatch(linep, "DocumentProcessColors:")) ||
		(argp = _strMatch(linep, "DocumentCustomColors:"))) {
		
		STRNDUP(cp, linep, argp - linep);
		if (newProp = [self getComplexProp:cp]) {
		    /* We already have this property */
		    free(cp);
		} else {
		    /* Don't have it, create one */
		    newProp = (prop_t *) malloc(sizeof (*newProp));
		    bzero(newProp, sizeof(*newProp));
		    newProp->p_name = cp;
		    newProp->p_next = complexProps;
		    complexProps = newProp;
		}
		eolp = _readComplexProp(newProp, argp, eobuf, 1);
		if (newProp->p_valsatend) {
		    atEnd++;
		}
		break;
	    }
	    
	    /* Complex props with three element lists */
	    if ((argp = _strMatch(linep, "DocumentProcSets:")) ||
		(argp = _strMatch(linep, "DocumentNeededProcSets:")) ||
		(argp = _strMatch(linep, "DocumentSuppliedProcSets:"))) {
		
		STRNDUP(cp, linep, argp - linep);
		if (newProp = [self getComplexProp:cp]) {
		    /* We already have this property */
		    free(cp);
		} else {
		    newProp = (prop_t *) malloc(sizeof (*newProp));
		    bzero(newProp, sizeof (*newProp));
		    newProp->p_name = cp;
		    newProp->p_next = complexProps;
		    complexProps = newProp;
		}
		eolp = _readComplexProp(newProp, argp, eobuf, 3);
		if (newProp->p_valsatend) {
		    atEnd++;
		}
		break;
	    }
	    /* Adobe-2.0 is a superset of Adobe-1.0, fall through */
	    
	  case PS_Adobe_10:
	    if (argp = _strMatch(linep, "Title:")) {
		if (!title) {
		    SKIPSPACE(argp);
		    STRNDUP(title, argp, eolp - argp);
		}
		break;
	    }
	    
	    if (argp = _strMatch(linep, "Creator:")) {
		if (!creator) {
		    SKIPSPACE(argp);
		    STRNDUP(creator, argp, eolp - argp);
		}
		break;
	    }
	    
	    if (argp = _strMatch(linep, "CreationDate:")) {
		if (!creationDate) {
		    SKIPSPACE(argp);
		    STRNDUP(creationDate, argp, eolp - argp);
		}
		break;
	    }
	    
	    if (argp = _strMatch(linep, "For:")) {
		if (!forUser) {
		    SKIPSPACE(argp);
		    STRNDUP(forUser, argp, eolp - argp);
		}
		break;
	    }
	    
	    if (argp = _strMatch(linep, "BoundingBox:")) {
		/* Make sure we haven't read the bounding box already */
		if (!boundBox.llx && !boundBox.urx) {
		    SKIPSPACE(argp);
		    if ( _strMatch(argp, "(atend)") ) {
			bboxAtEnd = 1;
			atEnd++;
		    } else if (sscanf(argp, "%d %d %d %d", &boundBox.llx,
				      &boundBox.lly, &boundBox.urx,
				      &boundBox.ury) != 4) {

			/* The 1.0 AppKit output bounds as floats, sheesh! */
			if (sscanf(argp, "%f %f %f %f", &tllx, &tlly,
				   &turx, &tury) == 4) {
			    boundBox.llx = (int) tllx;
			    boundBox.lly = (int) tlly;
			    boundBox.urx = (int) turx;
			    boundBox.ury = (int) tury;
			} else {
			    LogWarning([NXStringManager
				      stringFor:"DocInfo_badbounds"],
				       linep - 2);
			    bzero(&boundBox, sizeof (boundBox));
			}
		    }
		}
		break;
	    }
	    
	    if (argp = _strMatch(linep, "Pages:")) {
		if (!pages) {
		    SKIPSPACE(argp);
		    if (cp = _strMatch(argp, "(atend)")) {
			argp = cp;
			pagesAtEnd = 1;
			atEnd++;
		    } else {
			if (sscanf(argp, "%d", &pages) != 1) {
			    LogWarning([NXStringManager
				    stringFor:"DocInfo_badpages"],
				     linep - 2);
			    pages = 0;
			}
			while (*argp && !isspace(*argp)) {
			    argp++;
			}
		    }
		    
		    SKIPSPACE(argp);
		    if (argp < eolp) {
			if (_strMatch(argp, "(atend)")) {
			    pageOrderAtEnd = 1;
			    atEnd++;
			} else {
			    if (sscanf(argp, "%d", &pageOrder) != 1) {
				LogWarning([NXStringManager
					  stringFor:"DocInfo_badpageorder"],
					   linep - 2);
				pageOrder = 0;
			    }
			}
			pageOrderUnknown = 0;
		    }
		}
		break;
	    }
	    
	    /* Complex props with single element lists */
	    if (argp = _strMatch(linep, "DocumentFonts:")) {
		STRNDUP(cp, linep, argp - linep);
		if (newProp = [self getComplexProp:cp]) {
		    /* We already have this property */
		    free(cp);
		} else {
		    newProp = (prop_t *) malloc(sizeof (*newProp));
		    bzero(newProp, sizeof(*newProp));
		    newProp->p_name = cp;
		    newProp->p_next = complexProps;
		    complexProps = newProp;
		}
		eolp = _readComplexProp(newProp, argp, eobuf, 1);
		if (newProp->p_valsatend) {
		    atEnd++;
		}
		break;
	    }
	    
	    if (_strMatch(linep, "EndComments")) {
		linep = _nextLine(eolp, eobuf);
		goto endcomments;
	    }
	    
	  default:
	    break;
	    /* The default is to ignore the comment */
	}
	
	/* Skip to the next line */
	linep = _nextLine(eolp, eobuf);
    }
    
endcomments:
    
    /*
     * FIXME: We scan the remainder of the prologue to get the
     *	  default requirements such as paper, resolution, and
     *	  manual feed.  These requirements should be handled
     *	  during the printing of the prologue by automatic
     *	  substitution of the comment with device-dependent
     *	  PostScript.
     */
    while (linep < eobuf) {
	
	/* Get a pointer to the end of this line */
	if ((eolp = strchr(linep, '\n')) == NULL) {
	    eolp = linep + strlen(linep);
	}
	
	/* Check that this is a comment */
	if (strncmp(linep, "%%", 2)) {
	    /* No comment, ignore this line */
	    linep = _nextLine(eolp, eobuf);
	    continue;
	}
	if (_strMatch(linep, "%%BeginBinary:")) { /* skip binary data */
	     eolp = _nextLine(_skipBinary(linep,eobuf),eobuf);
	     continue;
	}

	linep += 2;			/* Skip the "%%" */
	
	/* Check for conforming comments */
	switch (version) {
	  case PS_Adobe_New:
	  case PS_Adobe_20:
	    
	    /* Check if we are entering an included file */
	    if (_strMatch(linep, "BeginDocument:") ||
		_strMatch(linep, "BeginFile:")) {
		filedepth++;
		break;
	    }
	    
	    if (_strMatch(linep, "EndDocument") ||
		_strMatch(linep, "EndFile")) {
		filedepth--;
		break;
	    }
	    
	    /* If we are in an included file, ignore this line */
	    if (filedepth) {
		break;
	    }
	    
	    if (argp = _strMatch(linep, "PaperSize:")) {
		if (paper.p_name) {
		    free(paper.p_name);
		}
		SKIPSPACE(argp);
		STRNDUP(paper.p_name, argp, eolp - argp);
		paper.p_size.width = paper.p_size.height = 0.0;
		break;
	    }
	    
#ifdef	FIXME
	    /*
	     * We don't do anything with this right now because
	     * we want to scan the beginning of the script for
	     * the default setup.
	     */
	    if (_strMatch(linep, "EndProlog")) {
		mutex_unlock(lock);
		return (self);
	    }
#endif	FIXME
	    
	    /* Fall through */

	  case PS_Adobe_10:
	  case PS_NonConforming:
	    /*
	     * Check for Richard's hacks for manual feed and resolution.
	     */
	    if (argp = _strMatch(linep, "Feature: *ManualFeed ")) {
		if (strncmp(argp, "True", 4) == 0) {
		    manualfeed = YES;
		} else {
		    manualfeed = NO;
		}
		break;
	    }
	    
	    if (argp = _strMatch(linep, "Feature: *Resolution ")) {
		if (!sscanf(argp, "%d", &resolution)) {
		    LogWarning([NXStringManager stringFor:"DocInfo_badres"],
			     linep - 2);
		    resolution = 0;
		}
		break;
	    }
	    
	    if (argp = _strMatch(linep, "Feature: *CustomPaperSize ")) {
		if (paper.p_name) {
		    free(paper.p_name);

		    paper.p_name = NULL;
		}
		if (sscanf(argp, "%f %f", &paper.p_size.width,
			   &paper.p_size.height) !=
		    2) {
		    LogWarning([NXStringManager
			    stringFor:"DocInfo_badcustmpaper"], linep - 2);
		    paper.p_size.width = paper.p_size.height = 0;
		}
		break;
	    }
	    
	  default:
	    /* By default we ignore everything */
	    break;
	}
	
	/* Skip to next line */
	linep = _nextLine(eolp, eobuf);
    }

    mutex_unlock(lock);

    return (self);
}

- parseTrailerFor:(id)printJob data:(char *)data size:(int) size
{
    char	*linep;			/* Pointer to the BOL */
    char	*argp;			/* Pointer to argument of comment */
    char	*eolp;			/* Pointer to end of current line */
    char	*eobuf;			/* Pointer to end of buffer */
    char	*cp;			/* generic character pointer */
    BOOL	foundTrailer = NO;
    int		filedepth = 0;
    prop_t	*newProp;
    float	tllx, tlly, turx, tury;	/* Temp bounds entries */
    
    /* Start at the very beginning */
    linep = data;
    eobuf = data + size;

    /* Is this conforming PostScript? */
    if (version == PS_NonConforming) {
	return (self);
    }

    /* Yes, go to trailer */
    while (linep < eobuf) {
	if (_strMatch(linep, "%%BeginBinary:")) { /* skip binary data */
	     linep = _nextLine(_skipBinary(linep,eobuf),eobuf);
	     continue;
	}
	if (version >= PS_Adobe_20) {
	    /* Check if we are entering an included file */
	    if (_strMatch(linep, "BeginDocument:") ||
		_strMatch(linep, "BeginFile:")) {
		filedepth++;
	    } else if (_strMatch(linep, "EndDocument") ||
		       _strMatch(linep, "EndFile")) {
		filedepth--;
	    }
	    
	    if (filedepth) {
		linep = _nextLine(linep, eobuf);
		continue;
	    }
	}
	
	if (_strMatch(linep, "%%Trailer")) {
	    foundTrailer = YES;
	    break;
	}
	linep = _nextLine(linep, eobuf);
    }

    if (!foundTrailer) {
	return (self);
    }

    mutex_lock(lock);

    linep = _nextLine(linep, eobuf);
    while (linep < eobuf) {

	/* Quick check that this is a comment */
	if (strncmp(linep, "%%",2)) {
	    linep = _nextLine(linep, eobuf);
	    continue;
	}
	if (_strMatch(linep, "%%BeginBinary:")) { /* skip binary data */
	     linep = _nextLine(_skipBinary(linep,eobuf),eobuf);
	     continue;
	}
	linep += 2;		/* Skip the "%%" */

	/* Get a pointer to the end of this line */
	if ((eolp = strchr(linep, '\n')) == NULL) {
	    eolp = linep + strlen(linep);
	}

	/* Check for conforming comments */
	switch (version) {
	  case PS_Adobe_New:
	  case PS_Adobe_20:

	    /* Check if we are entering an included file */
	    if (_strMatch(linep, "BeginDocument:") ||
		_strMatch(linep, "BeginFile:")) {
		filedepth++;
		break;
	    }
	    
	    if (_strMatch(linep, "EndDocument") ||
		_strMatch(linep, "EndFile")) {
		filedepth--;
		break;
	    }
	    
	    /* If we are in an included file, ignore this line */
	    if (filedepth) {
		break;
	    }

	    /* Complex props with one element lists */
	    if ((argp = _strMatch(linep, "DocumentProcessColors:")) ||
		(argp = _strMatch(linep, "DocumentCustomColors:"))) {
		
		STRNDUP(cp, linep, argp - linep);
		if ((newProp = [self getComplexProp:cp]) == NULL) {
		    /* This wasn't deferred, so ignore it */
		    free(cp);
		    break;
		}
		free(cp);
		if (newProp->p_valsatend) {
		    newProp->p_valsatend = 0;
		    atEnd--;
		    eolp = _readComplexProp(newProp, argp, eobuf, 1);
		}
		break;
	    }
	    
	    /* Complex props with three element lists */
	    if (argp = _strMatch(linep, "DocumentProcSets:")) {
		
		STRNDUP(cp, linep, argp - linep);
		if ((newProp = [self getComplexProp:cp]) == NULL) {
		    /* This wasn't deferred, so ignore it */
		    free(cp);
		    break;
		}
		free(cp);
		if (newProp->p_valsatend) {
		    newProp->p_valsatend = 0;
		    atEnd--;
		    eolp = _readComplexProp(newProp, argp, eobuf, 3);
		}
		break;
	    }

	    /* Adobe-2.0 is a superset of Adobe-1.0, fall through */

	  case PS_Adobe_10:

	    if (argp = _strMatch(linep, "BoundingBox:")) {
		if (bboxAtEnd) {
		    SKIPSPACE(argp);
		    if (sscanf(argp, "%d %d %d %d", &boundBox.llx,
			       &boundBox.lly, &boundBox.urx,
			       &boundBox.ury) != 4) {

			/* The 1.0 AppKit output bounds as floats, sheesh! */
			if (sscanf(argp, "%f %f %f %f", &tllx, &tlly,
				   &turx, &tury) == 4) {
			    boundBox.llx = (int) tllx;
			    boundBox.lly = (int) tlly;
			    boundBox.urx = (int) turx;
			    boundBox.ury = (int) tury;
			} else {
			    LogWarning([NXStringManager
				      stringFor:"DocInfo_badbounds"],
				       linep - 2);
			    bzero(&boundBox, sizeof (boundBox));
			}
		    }
		    bboxAtEnd = 0;
		    atEnd--;
		}
		break;
	    }

	    if (argp = _strMatch(linep, "Pages:")) {
		SKIPSPACE(argp);
		if (pagesAtEnd) {
		    if (sscanf(argp, "%d", &pages) != 1) {
			LogWarning([NXStringManager
				stringFor:"DocInfo_badpages"],
				 linep - 2);
			pages = 0;
		    }
		    pagesAtEnd = 0;
		    atEnd--;
		}
		while (*argp && !isspace(*argp)) {
		    argp++;
		}
		    
		SKIPSPACE(argp);
		if (pageOrderAtEnd) {
		    if (sscanf(argp, "%d", &pageOrder) != 1) {
			LogWarning([NXStringManager
				stringFor:"DocInfo_badpageorder"],
				 linep - 2);
			pageOrder = 0;
		    }
		    pageOrderAtEnd = 0;
		    atEnd--;
		}
		break;
	    }

	    /* Complex props with single element lists */
	    if (argp = _strMatch(linep, "DocumentFonts:")) {
		STRNDUP(cp, linep, argp - linep);
		if ((newProp = [self getComplexProp:cp]) == NULL) {
		    /* This property wasn't deferred, so ignore it */
		    free(cp);
		    break;
		}
		free(cp);
		if (newProp->p_valsatend) {
		    newProp->p_valsatend = 0;
		    atEnd--;
		    eolp = _readComplexProp(newProp, argp, eobuf, 1);
		}
		break;
	    }

	  default:
	    /* By default, we ignore everything else */
	    break;
	}

	/* Skip to the next line */
	linep = _nextLine(eolp, eobuf);
    }

    /* Sanity check */
    if (atEnd) {
	LogWarning([NXStringManager stringFor:"DocInfo_atendmismatch"]);
    }

    mutex_unlock(lock);

    return (self);
}

- (prop_t *)getComplexProp:(char *)name
{
    prop_t	*pp;			/* A property pointer */

    for (pp = complexProps; pp; pp = pp->p_next) {
	if (strcmp(name, pp->p_name) == 0) {
	    return (pp);
	}
    }
    return (NULL);
}

- hold
{
    refcnt++;
    return (self);
}

- release
{
    if (--refcnt == 0) {
	/*
	 * No one else should have a reference to this object, so we can
	 * just free it.
	 */
	return([self free]);
    }

    return (self);
}

- lock
{
    mutex_lock(lock);
    return(self);
}

- unlock
{
    mutex_unlock(lock);
    return(self);
}

- free
{
    prop_t	*prop;
    prop_t	*nextProp;
    int		i;

    /* Free up the complex properties */
    prop = complexProps;
    while (prop) {
	nextProp = prop->p_next;
	for (i = 0; i < prop->p_nvals; i++) {
	    free(prop->p_vals[i]);
	}
	if (prop->p_vals) {
	    free(prop->p_vals);
	}
	free(prop->p_name);
	free(prop);
	prop = nextProp;
    }

    if (paper.p_name) {
	free(paper.p_name);
    }

    if (printerRequired) {
	free(printerRequired);
    }

    if (routing) {
	free(routing);
    }

    if (forUser) {
	free(forUser);
    }

    if (creationDate) {
	free(creationDate);
    }

    if (creator) {
	free(creator);
    }

    if (title) {
	free(title);
    }

    mutex_free(lock);

    return ([super free]);
}


@end
