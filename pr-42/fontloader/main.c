
/* fontloader.
   This program writes a PostScript job containing downloadable fonts to the standard output.
   The job will install the fonts in a non-NeXT printer.  It just exits the server loop,
   then defines each font.
   The fonts are each delimited with %%BeginFont: <name> and %%EndFont, so that the user may
   post-process the job.
   This program will normally be used by piping the output into lpr.  For instance:
       fontloader Optima | lpr -POurLaserWriter
   
   Usage:  fontloader [-p password] font-name ...
   -p is optional and specifies the printer password.  Default = "0".
   Other strings are assumed to be font names.  The corresponding font file is found using
   the standard system lookup rules.
 */


#include "libc.h"
#include "pwd.h"


#define DEFAULT_PASSWORD "0"		/* Printer password.  Works for many printers. */


/* Global variables */
static int createdJob;			/* True: We have created the PostScript job */
static char programName[MAXPATHLEN];	/* Name of the program we are running */


/* Got an error.  The caller should report the error on stderr. */
static void Error(void)
{
    exit(1);
}


/* Print out the usage line */
static void Usage(void)
{
    fprintf(stderr, "Usage for %s: [-p password] font ...\n", programName);
}

/* Get home directory.  Free this memory when you are done.  Returns NULL on error. */
static char *GetHomeDir(void)
{
    struct passwd *userInfo;
    char *result;
    
    userInfo = getpwuid(getuid());
    if (userInfo == NULL)
	return NULL;
    
    result = malloc(strlen(userInfo->pw_dir) + 1);
    strcpy(result, userInfo->pw_dir);
    return result;
}


/* Given a font name, return a file name.  Empty name means no font file found. */
static void FontNameToFileName(const char *fontName, char *fileName)
{
    char *home;
    
    sprintf(fileName, "/NextLibrary/Fonts/%s.font/%s", fontName, fontName);
    if (access(fileName, R_OK) == 0)
	return;

    home = GetHomeDir();
    if (home == NULL) {
	fprintf(stderr, "%s: Cannot determine home directory: %s\n",
	    programName, strerror(errno));
	Error();
    }
    sprintf(fileName, "%s/Library/Fonts/%s.font/%s", home, fontName, fontName);
    free(home);
    if (access(fileName, R_OK) == 0)
	return;
	
    sprintf(fileName, "/LocalLibrary/Fonts/%s.font/%s", fontName, fontName);
    if (access(fileName, R_OK) == 0)
	return;

    *fileName = '\0';
}


/* Write some bytes to the job file */
static void WriteToJob(const char *bytes, int count)
{
    int writeCount;
    
    writeCount = fwrite(bytes, 1, count, stdout);
    if (writeCount != count) {
	fprintf(stderr, "%s: Trouble writing output: %s\n", programName, strerror(errno));
	Error();
    }
}


/* Open the temp file for the job and write a header */
static void WriteHeader(const char *password)
{
    char buf[1000];
    
    createdJob = 1;
    
    /* File header */
    sprintf(buf, "\
%%!PS-Adobe-2.0 ExitServer
%%%%BeginExitServer: %s
serverdict begin
 %s      %% Password
exitserver
%%%%EndExitServer
", password, password);

    WriteToJob(buf, strlen(buf));
}


/* Add a new font file to the job we are making */
static void AddFileToJob(const char *password, const char *fontName, const char *fileName)
{
    int fontFile;			/* File descriptor for font file */
    int readCount;			/* Bytes read out of font file */
    char buf[1000];
    
    if (!createdJob) WriteHeader(password);
    
    fontFile = open(fileName, O_RDONLY, 0);
    if (fontFile < 0) {
	fprintf(stderr, "%s: Cannot open font file %s: %s\n", 
		fileName, programName, strerror(errno));
	Error();
    }
    
    /* Copy header to output */
    sprintf(buf, "%%%%BeginFont: %s\n", fontName);
    WriteToJob(buf, strlen(buf));
    
    /* Copy the font file */
    while (1) {
	readCount = read(fontFile, buf, sizeof(buf));
	if (readCount < 0) {
	    fprintf(stderr, "%s: Trouble reading font file %s: %s\n",
		programName, fileName, strerror(errno));
	    Error();
	}
	if (readCount == 0) break;
	WriteToJob(buf, readCount);
    }
    close(fontFile);
    
    /* Copy trailer to file */
    sprintf(buf, "%%%%EndFont\n");
    WriteToJob(buf, strlen(buf));
}


/*** MAIN ROUTINE ***/
int main(int argc, char **argv)
{
    char option;			/* Option letter from command line */
    char password[200];			/* Password for exitserver in printer */
    char printerName[200];		/* Name of printer if specified */
    char fileName[MAXPATHLEN];		/* Font file name */
    
    /* Defaults */
    createdJob = 0;
    strcpy(password, DEFAULT_PASSWORD);
    printerName[0] = '\0';
    if (argc > 0) strcpy(programName, argv[0]);
    
    /* Process the command options */
    while ((option = getopt(argc, argv, "p:")) != EOF) {
	switch(option) {
	
	    /* Password */
	    case 'p':
		strcpy(password, optarg);
		break;
		
	    /* Bad option */
	    default:
		fprintf(stderr, "%s: Bad command option: %c\n", programName, option);
		Usage();
		Error();
		break;
	}
    }
    
    /* Process font names */
    while (optind < argc) {
	FontNameToFileName(argv[optind], fileName);
	if (*fileName != '\0') {
	    AddFileToJob(password, argv[optind], fileName);
	} else {
	    fprintf(stderr, "%s: Font not found: %s\n", programName, argv[optind]);
	    Error();
	}
	optind++;
    }
    
    /* If we did nothing, tell them how to do something next time */
    if (!createdJob) {
	Usage();
    }
    
    return 0;
}
