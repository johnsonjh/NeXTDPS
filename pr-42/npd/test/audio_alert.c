/*
 * audio_alert.c
 *
 * Copyright (c) 1999 by NeXT, Inc.  All rights reserved.
 *
 * This program tests the audio_alert request of an npd program.
 */


/*
 * Include files.
 */
#import <errno.h>
#import <stdio.h>
#import <sys/message.h>
#import <sys/types.h>
#import <nextdev/npio.h>
#import "../npd_prot.h"
#import "../mach_ipc.h"


/*
 * Type definitions.
 */
typedef struct {
    char	*arg;
    char	*soundfile;
} sound_switch;


/*
 * Constants
 */
#define	OS_ERROR	0x10000000
sound_switch	switches[] = {
    { "-dooropen", "/usr/lib/NextPrinter/printeropen.snd" },
    { "-paperjam", "/usr/lib/NextPrinter/paperjam.snd" },
    { "-manualfeed", "/usr/lib/NextPrinter/manualfeed.snd" },
    { "-nopaper", "/usr/lib/NextPrinter/nopaper.snd" },
    { NULL, NULL }
};


/*
 * Global variables.
 */

/* Program data */
char	*programName = NULL;		/* The name of this program */
char	thisHost[64];			/* The name of this host */

/* Message data */
char	*soundfile = NULL;		/* The alert message */
int	testPort = 0;			/* Whether to use npd_test_port */


/**********************************************************************
 * Routine:	usage() - Print the program's usage on stderr
 *
 * Function:	Print out correct usage of the program and then
 *		exit(1).
 **********************************************************************/
void
usage()
{
    sound_switch	*switchp = switches;

    fprintf(stderr, "%s: [-t] <code switch>\n"
	    "\t<code switch> = %s\n",programName, switchp->arg);
    switchp++;
    while (switchp->arg) {
	fprintf(stderr, "\t\t\t%s\n", switchp->arg);
	switchp++;
    }
    exit(1);
}

/**********************************************************************
 * Routine:	sendRequest() - Send the request to npd
 *
 * Function:	Cobble up a audio_alert_msg and send it off
 *		to npd.
 **********************************************************************/
sendRequest()
{
    npd1_audio_alert_msg	msg;	/* Message to send */
    port_t			npdPort; /* Port to send to */
    msg_return_t		ret_code; /* Error code */
    char			*portName;

    /* Clear the message structure */
    bzero(&msg, sizeof (msg));

    /* Figure out which port to talk to */
    if (testPort) {
	portName = NPD_TEST_PORT;
    } else {
	portName = NPD_PUBLIC_PORT;
    }

    /* Get the port from the name server */
    if((npdPort = PortLookup(portName, NULL)) == PORT_NULL) {
	fprintf(stderr, "PortLookup of \"%s\" failed.\n", portName);
	exit(1);
    }

    /* Build the header */
    msg.head.msg_remote_port = npdPort;
    msg.head.msg_local_port = PORT_NULL;
    msg.head.msg_size = sizeof( npd1_audio_alert_msg );
    msg.head.msg_id = NPD_1_AUDIO_ALERT;
    msg.head.msg_type = MSG_TYPE_NORMAL;
    msg.head.msg_simple = 1;

    /* Add the soundfile name */
    MSGTYPE_CHAR_INIT(msg.soundfile_type, sizeof(msg.soundfile));
    strncpy(msg.soundfile, soundfile, sizeof(msg.soundfile) - 1);

    /* Send the message */
    if((ret_code = msg_send(&msg, SEND_TIMEOUT, 10000 /* 10 seconds */ ))
       != SEND_SUCCESS){
	fprintf(stderr, "msg_send failed: ret_code=%d\n", ret_code);
    }
}


/**********************************************************************
 * Routine:	main()	- The main program.
 **********************************************************************/
int
main(int argc, char *argv[])
{
    int			i;		/* generic counter */
    char		*cp;		/* generic character pointer */
    sound_switch	*switchp;	/* pointer to a code switch */

    /* Store our program name */
    programName = argv[0];

    /* Get our host name */
    bzero(thisHost, sizeof(thisHost));
    if (gethostname(thisHost, sizeof(thisHost) - 1)) {
	perror("gethostname");
	exit(1);
    }

    /* Parse the arguments */
    for (i = 1; i < argc; i++) {
	cp = argv[i];

	if (*cp != '-') {
	    usage();

	    /* Doesn't return */
	}

	/* Figure out just what this switch is */
	if (strcmp(cp, "-t") == 0) {
	    testPort++;
	} else {
	    /* It must be a code switch */
	    for (switchp = switches; switchp->arg; switchp++) {
		if (strcmp(switchp->arg, cp) == 0) {
		    /* Found it */
		    soundfile = switchp->soundfile;
		    break;
		}
	    }
	    if (!switchp->arg) {
		usage();
		/* Doesn't return */
	    }
	}
    }

    if (!soundfile) {
	usage();
	/* Doesn't return */
    }

    /* Send the request to the npd */
    sendRequest();
}

