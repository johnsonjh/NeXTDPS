#import "buildafmdir.h"
#import "FontStringList.h"
#import "FontFamily.h"
#import "metrics.h"
#import <appkit/FontManager.h>
#import <objc/List.h>
#import <objc/HashTable.h>
#import <streams/streams.h>
#import <stdio.h>
#import <errno.h>
#import <stdlib.h>
#import <sys/dir.h>
#import <sys/time.h>
#import <sys/stat.h>
#import <pwd.h>
#import <sys/file.h>
#import <sys/param.h>
#import <string.h>
#import <libc.h>
#import <ctype.h>

/* This is DUPLICATED in FontFile.h in appkit_proj!! */

#define AFMDIR_FILENAME ".fontdirectory"

static id fontStringList = nil;	/* the strings in the file */
static id familyList = nil;	/* the list of FontFamily objects */
static id weightHT = nil;	/* weight name to number map */
static id weightBugsHT = nil;	/* special table of bugs in the afm file maps
				   incorrect weight names to proper ones */
static id duplicateTable = nil;	/* prevents duplicate font files */

const char *programName = "buildafmdir";

static void initHashTables()
{
    weightHT = [HashTable newKeyDesc:"*" valueDesc:"i"];
    
    [weightHT insertKey:"ultralight" value:(void *)1];
    [weightHT insertKey:"thin" value:(void *)2];
    [weightHT insertKey:"light" value:(void *)3];
    [weightHT insertKey:"extralight" value:(void *)3];
    [weightHT insertKey:"book" value:(void *)4];
    [weightHT insertKey:"regular" value:(void *)5];
    [weightHT insertKey:"plain" value:(void *)5];
    [weightHT insertKey:"roman" value:(void *)6];
    [weightHT insertKey:"medium" value:(void *)6];
    [weightHT insertKey:"demi" value:(void *)7];
    [weightHT insertKey:"demibold" value:(void *)7];
    [weightHT insertKey:"semibold" value:(void *)8];
    [weightHT insertKey:"bold" value:(void *)9];
    [weightHT insertKey:"extra" value:(void *)10];
    [weightHT insertKey:"extrabold" value:(void *)10];
    [weightHT insertKey:"heavy" value:(void *)11];
    [weightHT insertKey:"heavyface" value:(void *)11];
    [weightHT insertKey:"black" value:(void *)12];
    [weightHT insertKey:"ultra" value:(void *)13];
    [weightHT insertKey:"ultrablack" value:(void *)13];
    [weightHT insertKey:"fat" value:(void *)13];
    [weightHT insertKey:"extrablack" value:(void *)14];
    [weightHT insertKey:"obese" value:(void *)14];

  /* afm file bug-fixers */

    [weightHT insertKey:"titling" value:(void *)5];	  /* AGaramond-Titling */
    [weightHT insertKey:"normal" value:(void *)5];	  /* ArnoldBoecklin */
    [weightHT insertKey:"poster" value:(void *)5];	  /* Bodoni-Poster */

  /* more afm file bug-fixers for incorrect weight fields */

    weightBugsHT = [HashTable newKeyDesc:"*" valueDesc:"*"];

    [weightBugsHT insertKey:"BauerBodoni-Italic" value:"roman"];
    [weightBugsHT insertKey:"CooperBlack" value:"black"];
    [weightBugsHT insertKey:"CooperBlack-Italic" value:"black"];
    [weightBugsHT insertKey:"Eurostile-Demi" value:"demi"];
    [weightBugsHT insertKey:"Eurostile-DemiOblique" value:"demi"];
    [weightBugsHT insertKey:"Futura-CondensedExtraBold" value:"extrabold"];
    [weightBugsHT insertKey:"Futura-ExtraBold" value:"extrabold"];
    [weightBugsHT insertKey:"Futura-ExtraBoldOblique" value:"extrabold"];
    [weightBugsHT insertKey:"Futura-Heavy" value:"heavy"];
    [weightBugsHT insertKey:"Futura-HeavyOblique" value:"heavy"];
    [weightBugsHT insertKey:"Futura-Book" value:"book"];
    [weightBugsHT insertKey:"Futura-BookOblique" value:"book"];
    [weightBugsHT insertKey:"StoneInformal-Semibold" value:"semibold"];
    [weightBugsHT insertKey:"StoneInformal-SemiboldItalic" value:"semibold"];
    [weightBugsHT insertKey:"StoneSans-Semibold" value:"semibold"];
    [weightBugsHT insertKey:"StoneSans-SemiboldItalic" value:"semibold"];
    [weightBugsHT insertKey:"StoneSerif-Semibold" value:"semibold"];
    [weightBugsHT insertKey:"StoneSerif-SemiboldItalic" value:"semibold"];
    [weightBugsHT insertKey:"Tiffany-Demi" value:"demi"];
    [weightBugsHT insertKey:"Tiffany-HeavyItalic" value:"heavy"];
}

static void makeLC(char *string)
{
    if (string)
	for ( ; *string; string++)
	    if (isascii(*string) && isupper(*string))
		*string = tolower(*string);
}

/* cuts all spaces out of string */
static void stripSpaces(char *string)
{
    char *from, *to;

    if (!string)
	return;
    from = to = string;
    while (*from)
	if (*from != ' ' && *from != '\t')
	    *to++ = *from++;
	else
	    from++;
    *to = '\0';
}

/* cuts suffix off of string.  Returns if it found suffix. */
static BOOL cutSuffix(const char *suffix, char *string)
{
    int suffixLen, stringLen;

    if (string && suffix) {
	suffixLen = strlen(suffix);
	stringLen = strlen(string);
	if (stringLen >= suffixLen)
	    if (!strcmp(string+stringLen-suffixLen, suffix)) {
		string[stringLen-suffixLen] = '\0';
		return YES;
	    }
    }
    return NO;
}

static BOOL isPrefix(const char *prefix, const char *string)
{
    if (!string || !prefix) return NO;

    while (*string && *prefix == *string) {
	prefix++;
	string++;
    }

    return *prefix ? NO : YES;
}

static const char *separateWords(const char *word, char *buf)
{
    char c;
    char *s = buf;
    BOOL lastCWasSpace = YES;

    if (word) {
	s = buf;
	while (c = *word++) {
	    if ((c < '0' || c > '9') && c != '*' &&
		c != ':' && c != '#' && c != '.') {
		if ((c != ' ' && c != '	') || !lastCWasSpace) {
		    if (c >= 'A' && c <= 'Z' && !lastCWasSpace &&
			*word >= 'a' && *word <= 'z') {
			*s++ = ' ';
		    }
		    *s++ = c;
		    lastCWasSpace = (c == ' ' || c == '	');
		}
	    }
	}
	if (lastCWasSpace && s != buf) {
	    *--s = '\0';
	} else {
	    *s = '\0';
	}
	return buf;
    } else {
	return NULL;
    }
}

static const char *getFamilyName(const char *family, char *buf)
{
    if (isPrefix("ITC", family)) {
        return separateWords(family + 3, buf);
    }
    return separateWords(family, buf);
}

static const char *getFaceName(FontMetrics *fm, const char *familyName, char *buf)
{
    const char *name = fm->fullName;
    const char *family = familyName;

    if (*name++ == 'I' && *name++ == 'T' && *name++ == 'C') {
	while (*name == ' ') name++;
    } else {
	name = fm->fullName;
    }
    name = separateWords(name, buf);
    if (isPrefix("Linotype ", name)) {		/* 12 Linotype Centennial */
	name += 9;
    } else if (isPrefix("pt ", name)) {		/* 1[28] pt. Helvetica */
	name += 3;
    }
    while (*name && *name == *family) {
	family++;
	name++;
    }
    /* Take care of the TFC fonts. */
    while (*name == '-' || *name == ' ') {
	name++;
    }
    if (!*name || !strcmp(name, "Bold Face No")) {
	return fm->weight;
    } else {
	return name;
    }
}

static int getWeight(const char *name, const char *weightArg)
{
    BOOL changedString;
    char *weightCopy = NULL;
    char *weight;
    int weightVal;

    if ([weightBugsHT isKey:name]) {
	weight = [weightBugsHT valueForKey:name];
    } else {
	weight = weightCopy = NXCopyStringBuffer(weightArg);
	stripSpaces(weight);
	do {
	    changedString = NO;
	    changedString |= cutSuffix("Italic", weight);
	    changedString |= cutSuffix("Oblique", weight);
	    changedString |= cutSuffix("Alt", weight);
	    changedString |= cutSuffix("Alternate", weight);
	    changedString |= cutSuffix("Ext", weight);
	    changedString |= cutSuffix("Extended", weight);
	    changedString |= cutSuffix("Cond", weight);
	    changedString |= cutSuffix("Condensed", weight);
	    changedString |= cutSuffix("Compressed", weight);
	} while (changedString);
	makeLC(weight);
    }
    if ([weightHT isKey:weight]) {
	weightVal = (int)[weightHT valueForKey:weight];
    } else {
	weightVal = 5;
    }
    if (weightCopy)
	free(weightCopy);
    return weightVal;
}

static BOOL contains(const char *s, const char *substring)
{
    const char *sub = substring;

    while (*s && *s != *substring) {
	s++;
    }
    while (*s && *sub && *s == *sub) {
	s++;
	sub++;
    }
    if (*s) {
	return *sub ? contains(s, substring) : YES;
    } else {
	return *sub ? NO : YES;
    }
}

static int getFlags(FontMetrics *fm, char weight)
{
    int retval = 0;

    if (fm->italicAngle) {
	retval |= NX_ITALIC;
    }
    if (strcmp(fm->encodingScheme, STANDARD_ENCODING_SCHEME)) {
	retval |= NX_NONSTANDARDCHARSET;
    }
    if (contains(fm->name, "Expanded") || contains(fm->name, "Extended") ||
    contains(fm->fullName, "Expanded") || contains(fm->fullName, "Extended")) {
	retval |= NX_EXPANDED;
    }
    if (contains(fm->name, "Narrow") || contains(fm->fullName, "Narrow")) {
	retval |= NX_NARROW;
    }
    if (contains(fm->name, "Condensed") || contains(fm->fullName, "Condensed")) {
	retval |= NX_CONDENSED;
    }
    if (contains(fm->name, "Compressed") || contains(fm->fullName, "Compressed")) {
	retval |= NX_COMPRESSED;
    }
    if (contains(fm->name, "SmallCaps") || contains(fm->name, "SmCp") ||
	contains(fm->name, "AGaramondExp") || contains(fm->fullName, "Small Caps") || contains(fm->fullName, "SmallCaps")) {
	retval |= NX_SMALLCAPS;
    }
    if (contains(fm->name, "Poster") || contains(fm->fullName, "Poster")) {
	retval |= NX_POSTER;
    }

    return retval;
}

#define BUFLEN 1024

static void processFont(const char *name)
{
    int i, count;
    id family = nil;
    FontMetrics* fm;
    int familyIndex;
    int weight, flags, size;
    const char *familyName, *faceName;
    char familyBuf[BUFLEN];
    char faceBuf[BUFLEN];

    fm = readMetrics(name);

    if (fm) {
	familyName = getFamilyName(fm->familyName, familyBuf);
	if (familyName) {
	    faceName = (fm->faceName && *(fm->faceName)) ? fm->faceName : getFaceName(fm, familyName, faceBuf);
	    weight = (fm->weightValue > 0 && fm->weightValue < 15) ? fm->weightValue : getWeight(fm->name, fm->weight);
	    flags = getFlags(fm, weight);
	    size = 0;
	    count = [familyList count];
	    familyIndex = [fontStringList add:fm->familyName];
	    for (i = 0; i < count; i++) {
		family = [familyList objectAt:i];
		if ([family matches:familyIndex]) {
		    break;
		}
	    }
	    if (i == count) {
		family = [FontFamily newFamily:familyIndex];
		[familyList addObject:family];
	    }
	    [family addFace:[fontStringList add:faceName] weight:weight
		flags:flags size:size afm:[fontStringList add:fm->name]];
	}
    }

    freeFontMetrics(fm);
}

static int doafm(struct direct *entry)
{
    struct stat st;
    char *s, t[MAXPATHLEN+1];
    char outline[MAXPATHLEN+1];

    s = strrchr(entry->d_name, '.');
    if (s && !strcmp(s, ".afm")) {
	t[s - entry->d_name] = '\0';
	strncpy(t, entry->d_name, s - entry->d_name);
	strcpy(outline, "../outline/");
	strcat(outline, t);
	if (!strrchr(t, '.')) {
	    if (stat(outline, &st) < 0) {
		fprintf(stderr, "%s: no outline for %s.afm\n", programName, t);
	    } else {
		if (![duplicateTable isKey:entry->d_name]) {
		    processFont(entry->d_name);
		} else {
		    fprintf(stderr, "%s: old and new versions of %s found.\n",
			    programName, entry->d_name);
		}
	    }
	}
    }

    return 0;
}

static int fontlistfd = 0;

static int donewafm(struct direct *entry)
{
    char *dot;
    struct stat st;
    char t[MAXPATHLEN+1], curwd[MAXPATHLEN+1];

    strcpy(t, entry->d_name);
    dot = strrchr(t, '.');
    if (!dot || strcmp(dot, ".font")) return 0;

    if (fontlistfd > 0) write(fontlistfd, t, strlen(t)+1);

    if (!getwd(curwd)) return 0;
    if (chdir(t)) return 0;

    *dot = '\0';
    if (!stat(".", &st)) {
	strcat(t, ".afm");
	if (!stat(t, &st)) {
	    if (!duplicateTable) {
		duplicateTable = [HashTable newKeyDesc:"*" valueDesc:"i"];
	    }
	    [duplicateTable insertKey:NXCopyStringBuffer(t) value:0];
	    processFont(t);
	} else {
	    fprintf(stderr, "%s: no afm file for %s\n", programName, entry->d_name);
	}
    } else {
	fprintf(stderr, "%s: no outline for %s\n", programName, entry->d_name);
    }

    chdir(curwd);

    return 0;
}

static void dump(NXStream *stream)
{
    char filler;
    int i, count;
    int intsize, total = 0;

    intsize = sizeof(int);
    NXWrite(stream, &intsize, sizeof(int));
    i = sizeof(int);
    while (i < 4) {
	NXWrite(stream, &filler, 1);
	i++;
    }
    count = [familyList count];
    for (i = 0; i < count; i++) {
	total += [[familyList objectAt:i] count];
    }
    NXWrite(stream, &total, sizeof(int));
    for (i = 0; i < count; i++) {
	[[familyList objectAt:i] dump:stream];
    }
    [fontStringList dump:stream];
}

int main(int argc, char *argv[])
{
    struct stat st;
    struct direct **namelist;
    uid_t owner = -1;
    gid_t group = -1;
    int fd, result;
    NXStream *stream;
    struct timeval tvp[2];
    BOOL couldntBuild;
    char cwd[MAXPATHLEN+1], newadf[MAXPATHLEN+1];
    char adf[MAXPATHLEN+1], afm[MAXPATHLEN+1];

    if (!(programName = strrchr(argv[0], '/'))) programName = argv[0];

    if (argc != 2) {
	fprintf(stderr, "Usage: %s directory\n", programName);
	exit(1);
    }

    strcpy(adf, argv[1]);
    strcat(adf, "/");
    strcat(adf, AFMDIR_FILENAME);
    if (stat(adf, &st) < 0 && errno != ENOENT) {
	fprintf(stderr,"%s: invalid directory (%s)\n", programName, argv[1]);
	exit(errno);
    }
    
    strcpy(newadf, adf);
    strcat(newadf, ".new");

    fd = open(newadf, O_CREAT|O_EXCL|O_WRONLY, 0666);
    if (fd <= 0) {
	stat(newadf, &st);
	if (st.st_mtime - time(0) > STALE_LOCK_TIMEOUT) {
	    fd = open(newadf, O_CREAT|O_WRONLY, 0666);
	}
	if (fd <= 0) {
	    fprintf(stderr, "%s: couldn't create lock file\n", programName);
	    exit(errno);
	}
    }

    initHashTables();
    fontStringList = [FontStringList new];
    familyList = [List new];

    getwd(cwd);
    strcpy(afm, argv[1]);
    if (chdir(afm) || stat(".", &st) < 0) {
	fprintf(stderr, "%s: couldn't connect to Fonts directory\n",programName);
	couldntBuild = YES;
    } else {
	owner = st.st_uid;
        group = st.st_gid;
	fontlistfd = open(".fontlist", O_CREAT|O_WRONLY|O_TRUNC, 0666);
	result = scandir(".", &namelist, donewafm, NULL);
	if (fontlistfd > 0) close(fontlistfd);
	if (result == -1) {
	    fprintf(stderr, "%s: couldn't scan Fonts directory\n", programName);
	    couldntBuild = YES;
	} else {
	    couldntBuild = NO;
	}
    }
    strcat(afm, "/afm");
    if ((chdir(afm) || stat(".", &st) < 0) && couldntBuild) {
	fprintf(stderr, "%s: couldn't connect to afm directory\n",programName);
	unlink(newadf);
	exit(errno);
    } else {
	if (couldntBuild) {
	    owner = st.st_uid;
	    group = st.st_gid;
	}
	result = scandir(".", &namelist, doafm, NULL);
	if (result == -1 && couldntBuild) {
	    fprintf(stderr, "%s: couldn't scan afm directory\n", programName);
	    unlink(newadf);
	    exit(errno);
	}
    }
    chdir(cwd);

    stream = NXOpenFile(fd, NX_WRITEONLY);
    dump(stream);
    NXClose(stream);

    close(fd);

    unlink(adf);
    link(newadf, adf);
    unlink(newadf);
    chown(adf, owner, group);
    chmod(adf, 0644);

    if (!stat(argv[1], &st)) {
	tvp[0].tv_sec = st.st_mtime + 1;
	tvp[0].tv_usec = 0;
	tvp[1] = tvp[0];
	utimes(adf, tvp);
    }

    [familyList free];
    [fontStringList free];
    [weightHT free];
    [duplicateTable free];

    return 0;
}
