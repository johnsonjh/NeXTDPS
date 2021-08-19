/*****************************************************************************

    nextmain.c
    
    This file is the mainline and initialization of the NeXT
    Display PostScript window server.
    
    CONFIDENTIAL
    Copyright (c) 1988 by NeXT, Inc as an unpublished work.
    All rights reserved.
    
    PostScript is a registered trademark of Adobe Systems Incorporated.
    Display PostScript is a trademark of Adobe Systems Incorporated.
    
    Original version: Leo Hourvitz, July 1988

    Edit History:

    16Jan89 Jack  new InitPostScript argument record
    03Mar89 Jack  fix open of /usr/adm/psout per Avie & Mike's instructions
    21Mar89 Leo   Bug upon clear: clear umask at startup
    02Jun89 Leo   signals handled on main stack via conditional USE_SIGSTACK
    17Nov89 Ted   Initialize alpha=255 (OPAQUE) in NextGStatesProc()
    14Nov89 Terry CustomOps conversion
    04Dec89 Ted   Integratathon!
    13Dec89 Ted   Moved NextGStatesProc() to gstates.c customization
    08Jan90 Terry Changed parms.realClock parameter back to using dummyClock
    28Feb90 Terry Added call to IniWindowImage for msgImageBitmap
    21Mar90 Jack  uncomment call to (D)PSDeviceInit (why'd we do that anyway?)
    30Mar90 Terry Changed font path list to not have "outline" in it
    27Apr90 Terry Added profiling toggle operator called prof
    23Jul90 Terry Place /NextLibrary/Fonts at start of font search path
    31Jul90 Terry Set thread priority and policy

******************************************************************************/

#import <mach.h>
#import <kern/sched.h>
#import <sys/file.h>
#import <signal.h>
#import <string.h>
#import <sys/types.h>
#import <sys/time.h>
#import PACKAGE_SPECS
#import DEVICE
#import DEVCREATE
#import POSTSCRIPT
#import PSLIB
#import STREAM
#import UNIXSTREAM
#import "ipcscheduler.h"

extern procedure NextGStatesProc();

static int dummyClock;

#define USE_SIGSTACK 0		/* Whether to use separate signal stack */
#define PROF 0
#define PROFINIT 0

#if USE_SIGSTACK
#define SIGNALSTACKSIZE 2048	/* Stack usage for signal handlers */
private char *signalStackArea;	/* Stack area for signal handlers */
#endif USE_SIGSTACK

/*****************************************************************************
    coSignal is an equivalent to the C library routine signal, but causes the
    signal to always be received on the separate signal stack.
******************************************************************************/

public integer coSignal(int sig, int (*func)())
{
#if USE_SIGSTACK
    struct sigvec myVec, oldVec;

    myVec.sv_handler = func;
    myVec.sv_mask = -1 & (~(1<<sig));
    myVec.sv_flags = SV_ONSTACK;
    sigvec(sig, &myVec, &oldVec);
    return((integer)oldVec.sv_handler);
#else USE_SIGSTACK
    return (integer) signal(sig, (void (*)())func);
#endif USE_SIGSTACK
}

#if PROF
static boolean mntring;
private procedure PSProf()
{
    
    if (!mntring){
	mntring = true;
	os_fprintf(os_stdout, "Performance monitoring -- on.\n"); 
	fflush(os_stdout);
	moninit();
	moncontrol(1);
    }
    else {
	moncontrol(0);
	monoutput ("gmon.out");
	os_fprintf(os_stdout, "Performance monitoring -- off.\n");
	fflush(os_stdout);
	mntring = false;
    }
}
#endif

private void NextCustomProc()
{
    int i;
    extern void DriverInit();
  
    for (i = 0; i<2; i++) {
#if (CONTROLLER_TYPE == cont_next1)
	MpdDevInit(i);
#endif (CONTROLLER_TYPE == cont_next1)
	ListenerInit(i);
	LayerInit(i);
	IniWindowOps(i);
	IniWindowGraphics(i);
	IniWindowImage(i);
	IniCoordinates(i);
	MouseInit(i);		/* Must be after IniWindowOps */
	MiscOpsInit(i);
	EventInit(i);		/* Now goes after MouseInit */
    }
    if (!GetCArg ('i')) {
	DriverInit();
	RegisterGraphicsPackages( "/usr/lib/NextStep/Packages" );
	/* this next call will overwrite any duplicate entries */
	RegisterGraphicsPackages( "/usr/local/lib/NextStep/Packages" );
    }
#if PROF
    PSRegister("prof", PSProf);
#endif
}

main(int argc, char *argv[])
{
    Stm	saveIn, saveOut, vmStm = NIL;
    PSSpace DPS_Space;
    char *iArgString, masterVMname[256];
    readonly char VMname[] = "PS.VM";
    PostScriptParameters parms;
    char *initString;
    PSContext ctx;
    kern_return_t kret;
#if (STAGE == DEVELOP)
    extern procedure RgstPARelocator ();
    extern procedure InitMakeVM ();
#endif (STAGE == DEVELOP)

#if PROF
    /* Turn off profiling */
    moncontrol(0); 
#endif
#if PROFINIT
    /* Profile initialization code */
    mntring = true;
    moninit();
    moncontrol(1);
#endif

    /* Set priority higher than normal priority and policy to interactive */
    kret = thread_priority(thread_self(), MAXPRI_USER - 2, FALSE);
    kret = thread_policy(thread_self(), POLICY_INTERACTIVE, 0);

#if USE_SIGSTACK
    {	/* Get signals off onto their own stack */
	struct sigstack newSigStack;

	signalStackArea = (char *) os_malloc(SIGNALSTACKSIZE);
	newSigStack.ss_sp = signalStackArea + SIGNALSTACKSIZE - sizeof(int);
	newSigStack.ss_onstack = 0;
	sigstack(&newSigStack, NULL);
    }
#endif USE_SIGSTACK

    /* Leo 28Nov88 */
    /* The init blocks SIGHUP.  For the 0.8 release, we just
       want SIGHUP to kill us.  For 0.9, this needs to be
       changed to properly to the same thing as the resetuser
       operator.  FIX!!!
       Leo 02Jun89 Second thoughts, shouldn't we just let the
       window server get relaunched in this case?  */
    signal(SIGHUP,SIG_DFL);
    /* Clear the $%$#@ umask! */
    (void)umask(0);
    {
    	/* Leo 11Nov88 */
    	/* If we're being launched from init, we have no stdin/out
	   streams.  If that's true, we need to open some.  We'll
	   test this by opening /dev/null.  If it is fd 0, we'll
	   assume we are being launched from init and open 0, 1, and 2.
	   */
	int fd;
	
	if ((fd = open("/dev/null", O_RDONLY, 0)) == 0)
	{
	    if (open("/usr/adm/psout", O_TRUNC|O_CREAT|O_WRONLY, 0666) == 1)
		dup2(1, 2);
	    else {
		/* If we can't create the file for some reason, do this */
	    	fd = open("/dev/null", O_WRONLY,0);
			dup2(fd, 2);
	    }
	}
	else
	    close(fd);
    }

    StmInit();
    UnixStmInit();
    FPInit();
    StoDevInit();
    UnixStoDevInit();
    /* Place /NextLibrary/Fonts at the start to reduce network lookup time.
     * This prevents people from overriding the fonts in NextLibrary, but
     * that has apparently been a rather dangerous action anyhow, and not
     * very common either. Or possibly not, pending FCC approval.
     * "~/Library/Fonts:/LocalLibrary/Fonts:/NextLibrary/Fonts"
     */
    FontStoDevInit("/NextLibrary/Fonts:~/Library/Fonts:/LocalLibrary/Fonts");
    BeginParseArguments(argc, argv);
    PSDeviceInit();  
    SchedulerInit();

    /* Leo 12Jul88 The stupid whole cloth init nukes os_stdin and os_stdout */
    saveIn = os_stdin; saveOut = os_stdout;

    /* We don't handle explicit VM file request via "-i filename"
	but use it to signal we're building VM and shouldn't read one */
    iArgString = (char *) GetCArg ('i');
    if (iArgString == NIL) {
	/* Look for a suitable master VM */
	FilePathSearch(".:/usr/lib/NextStep/", VMname, os_strlen(VMname),
	&masterVMname,sizeof(masterVMname));
	vmStm = os_fopen(masterVMname, "r");
    }
    /* Initialize PostScriptParameters
     * Use a dummy clock for this guy until events context
     * is initialized in PSInitEvents.
     */
    bzero((char *) &parms, sizeof(parms));
    parms.errStm = os_stderr;
    parms.vmStm = vmStm;
    parms.customProc = NextCustomProc;
    parms.gStateExtProc = NextGStatesProc;
    parms.writeVMProc = NIL;
    parms.realClock = (PCard32)&dummyClock;
#if (STAGE == DEVELOP)
    /* If we're making a PS.VM, set writeVMProc, otherwise leave it NULL,
     * so that DPS kernel will know we are not building one.
     */
    if (iArgString != NIL) {
	parms.writeVMProc = InitMakeVM;	/* registers makevm command */
	RgstPARelocator ();		/* register makevm callback */
   }
#endif (STAGE == DEVELOP)

    InitPostScript(&parms);

    PSSetTimeLimit(1);	/* Avoid timeouts before clock is running */
    os_stdin = saveIn;
    os_stdout = saveOut;

    /* Create a context in which initString can run */
    initString = (char *) GetCArg('e');
    if (initString == NULL) initString = "nextdict /windowserver get exec ";
    DPS_Space = CreatePSSpace();
    ctx = CreatePSContext(DPS_Space, os_stdin, os_stdout, CreateNullDevice(),
	initString);

    EndParseArguments();

    /* OK, whomsoever's ready can go */
    Scheduler();
}



