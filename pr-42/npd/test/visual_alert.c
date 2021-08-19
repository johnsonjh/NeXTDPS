/*
 * visual_alert.c
 *
 * Copyright (c) 1999 by NeXT, Inc.  All rights reserved.
 *
 * This program tests the visual_alert request of an npd program.
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
    int		code;
} code_switch;


/*
 * Constants
 */
#define	OS_ERROR	0x10000000
code_switch	switches[] = {
    { "-dooropen", NPDOOROPEN },
    { "-paperjam", NPPAPERJAM },
    { "-manualfeed", NPMANUALFEED },
    { "-nopaper", NPNOPAPER },
    { "-nocartridge", NPNOCARTRIDGE },
    { "-notoner", NPNOTONER },
    { "-hardware", NPHARDWAREBAD },
    { "-paperdelivery", NPPAPERDELIVERY },
    { "-dataretrans", NPDATARETRANS },
    { "-npcold", NPCOLD },
    { "-oserror", (OS_ERROR|ENODEV) },
    { NULL, 0 }
};


/*
 * Global variables.
 */

/* Program data */
char	*programName = NULL;		/* The name of this program */
char	thisHost[64];			/* The name of this host */

/* Message data */
char	*message = NULL;		/* The alert message */
char	*button1 = NULL;		/* The alert button1 */
char	*button2 = NULL;		/* The alert button2 */
char	*host = NULL;			/* The host with printer to watch */
int	code = 0;			/* The alert code */
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
    code_switch	*switchp = switches;

    fprintf(stderr, "%s: [-t] [-h host] [-b1 button 1] [-b2 button2] "
	    "message\n", programName);
    fprintf(stderr, "%s: [-t] [-h host] <code switch>\n"
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
 * Function:	Cobble up a visual_alert_msg and send it off
 *		to npd.
 **********************************************************************/
sendRequest()
{
    npd1_visual_alert_msg	msg;	/* Message to send */
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
    msg.head.msg_size = sizeof( npd1_visual_alert_msg );
    msg.head.msg_id = NPD_1_VISUAL_ALERT;
    msg.head.msg_type = MSG_TYPE_NORMAL;
    msg.head.msg_simple = 1;

    /* Add the alert code */
    MSGTYPE_INT_INIT(msg.alert_code_type);
    msg.alert_code = code;

    /* Add the message */
    if (!message) {
	message = "";
    }
    MSGTYPE_CHAR_INIT(msg.message_type, sizeof(msg.message));
    strncpy(msg.message, message, sizeof(msg.message) - 1);

    /* Add button1 */
    if (!button1) {
	button1 = "";
    }	
    MSGTYPE_CHAR_INIT(msg.button1_type, sizeof(msg.button1));
    strncpy(msg.button1, button1, sizeof(msg.button1) - 1);

    /* Add button2 */
    if (!button2) {
	button2 = "";
    }
    MSGTYPE_CHAR_INIT(msg.button2_type, sizeof(msg.button2));
    strncpy(msg.button2, button2, sizeof(msg.button2) - 1);

    /* Add originating host */
    if (!host) {
	host = thisHost;
    }
    MSGTYPE_CHAR_INIT(msg.orig_host_type, sizeof(msg.orig_host));
    strncpy(msg.orig_host, host, sizeof(msg.orig_host) - 1);

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
    code_switch		*switchp;	/* pointer to a code switch */

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
	    /*
	     * This must be a message.  If we are not the last
	     * argument, print an error.
	     */
	    if (i != argc - 1) {
		usage();
		/* Doesn't return */
	    }

	    message = cp;
	    break;
	}

	/* Figure out just what this switch is */
	if (strcmp(cp, "-t") == 0) {
	    testPort++;
	} else  if (strcmp(cp, "-h") == 0) {
	    host = argv[++i];
	} else if (strcmp(cp, "-b1") == 0) {
	    button1 = argv[++i];
	} else if (strcmp(cp, "-b2") == 0) {
	    button2 = argv[++i];
	} else {
	    /* Perhaps it's a code switch */
	    for (switchp = switches; switchp->arg; switchp++) {
		if (strcmp(switchp->arg, cp) == 0) {
		    /* Found it */
		    code = switchp->code;
		    break;
		}
	    }
	    if (!switchp->arg) {
		usage();
		/* Doesn't return */
	    }
	}
    }

    /* We must have either a message or a code */
    if (!message && !code) {
	usage();
    }

    /* Send the request to the npd */
    sendRequest();
}

