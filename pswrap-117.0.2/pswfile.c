/*
  pswfile.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Andrew Shore Wed May 25 10:04:05 1988
Edit History:
Andrew Shore: Fri Jul  1 10:23:38 1988
Richard Cohn: Fri Oct 21 15:38:01 1988
End Edit History.
*/

#include <stdio.h>
#include "pswversion.h"
#include <varargs.h>

extern int outlineno;		/* line number in output file */

extern FILE *header;
extern char headid[];
extern char *ifile;
extern char *hfile;
extern char *ofile;
#ifdef os_mach
extern char *shlibInclude;
#endif os_mach

#ifdef os_mpw
void SetFileStuff();
#endif

/* for debugging */
myprintf (va_alist)
  va_dcl
{
	printf(va_alist);
}

static int EmitVersion(f, infname, outfname)
    FILE *f;
    char *infname, *outfname;
{
    extern char *prog;
    fprintf(f,"/* %s generated from %s\n",outfname,infname);
    fprintf(f,"   by %s %s %s\n */\n\n",PSW_OS,prog,PSW_VERSION);
    return 4;  /* number of output lines */
}

InitHFile(){
    (void) EmitVersion(header, ifile, hfile);
    fprintf(header,"#ifndef %s\n#define %s\n",headid,headid);
}

FinishHFile() {
    fprintf(header,"\n#endif %s\n",headid);
#ifdef os_mpw
    SetFileStuff(header);
#endif
    fclose(header);
}

InitOFile() {
    outlineno += EmitVersion(stdout, ifile, ofile);
#ifdef os_mpw
    printf("#include <Memory.h>\n#include <Resources.h>\n\n");
    printf("#include PACKAGE_SPECS\n#include DPSFRIENDS\n\n");
	printf("void InitWrap();\n");
    printf("static struct {char *h; char *wb[1];} *wrap_gp;\n\n");
    outlineno += 9;  /* UPDATE this if you add more prolog */
#else
#ifdef os_mach
    if( shlibInclude ) {
 		printf("#ifdef SHLIB\n");
 		printf("#include \"%s\"\n", shlibInclude );
 		printf("#endif\n");
 		outlineno += 3;
    }
#endif os_mach
    printf("#include %s\n", FRIENDSFILE);
    printf("#include <string.h>\n\n");
    outlineno += 3;  /* UPDATE this if you add more prolog */
#endif
    printf("#line 1 \"%s\"\n",ifile);
    outlineno++;
}

#ifdef os_mpw
#include "macpswfile.c"
#endif
