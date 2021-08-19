
/*
	NSWSd
	TCP front end for PostScript (deamon)
	
	This program listens for TCP/IP socket stream 
	connections on a given port.  It then connects
	to PostScript via Mach IPC on behalf of that
	port and continually translates between socket
	streams and Mach messages for the duration of that
	connection.
	
	Created Leovitch 08Jan88
	
	Modified:
	
	28Mar89	Leo  Modified so that this is auto-launched
		     from inetd.  In this configuration, the
		     socket to do accepts on is passed in as
		     socket 0.	Name changed from tcppsd to
		     NSWSd.
	
	*/

#import <stdio.h>
#import <mach.h>
#define c_plusplus 1
#import <cthreads.h>
#undef c_plusplus
#import <servers/netname.h>
#import <sys/types.h>
#import <sys/file.h>
#import <sys/stat.h>
#import <sys/errno.h>
#import <sys/socket.h>
#import <sys/notify.h>
#import <sys/message.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <syslog.h>


#define PSCONTEXTID 1000	/* msg id to get a PostScript context */
#define PS_NAME "NextStep(tm) Window Server" /* netname to look up */
#define	REC_ERR_RETRY 1000	/* Number of errors on msg_receive before */
				/*  giving up */
#define REC_ERR_PRINT 20	/* Nmber of errors we print out answers for */

/* Msg_id values for message between threads */
#define TCPPSD_EPIPE	(NOTIFY_LAST + 1)	/* EPIPE on write */
						/* Data is cthread_t */
#define TCPPSD_EOF	(NOTIFY_LAST + 2)	/* EOF on read */
						/* Data is cthread_t */
#define TCPPSD_NEWCONN	(NOTIFY_LAST + 3)	/* New connection accepted */
						/* Data is fd */

/* msg_id values for messages to PostScript */
#define STREAM_DATA_MSG_ID 3049
				/* The msg_id value for stream data */
#define EOF_MSG_ID 3050	/* The msg_id value indicating end of input */

#define DEBUG_OUTPUT 1			/* Enables debugging output */
#define DEBUG_SPIN 0			/* Causes daemon to spin on entry */
					/* so that you can attach w/debugger */

/* This is the structure holding all the global state for a connection */
typedef struct _connection {
	cthread_t	fromThread;	/* Thread sneding from client to PS */
	cthread_t	toThread;	/* Thread sending back to client */
	int		fd;		/* fd of the socket for the client */
	port_t		psPort;		/* port in PS to send to */
	port_t		myPort;		/* Port here that we receive msgs on */
	struct _connection *next;
} Connection;

static	char *myname = "tcppsd";
port_t		postScriptPort;	/* PostScript's listen port */
port_t		notifyPort;	/* My notify port */
int		abortFlag;	/* Shows that a thread_abort is in progress */
cthread_t	listenThread;	/* Cthread for the listener */
mutex_t		spawnMutex;	/* Used to synchronize newly created threads;
				   this is basically a lock on the connection
				   list, below */
struct mutex	mutexRec;	/* Above points to me */
Connection	*connections;	/* Linked list of active connections */

extern void ListenerFunc();	/* forward */
extern void ToFunc();		/* forward */
extern void FromFunc();		/* forward */
void ConnectionCreate();	/* forward */
void ConnectionTeardown();	/* forward */

#if DEBUG_OUTPUT
void PrintConn(conn)
Connection *conn;
{
    fprintf(stderr,"conn ft %d tt %d fd %d pP %d mP %d\n",
	conn->fromThread,conn->toThread,conn->fd,conn->psPort,conn->myPort);
} /* PrintConn */

void PrintMsg(m)
msg_header_t *m;
{
	msg_type_t *t;
	
	fprintf(stderr,"msg sim %d sz %d ty %d lp %d rp %d id %d ",
		m->msg_simple,m->msg_size,m->msg_type,m->msg_local_port,
		m->msg_remote_port,m->msg_id);
	t = (msg_type_t *)(((char *)m) + sizeof(msg_header_t));
	fprintf(stderr,"nm %d num %d il %d l %d da %d ",
		t->msg_type_name,t->msg_type_number,t->msg_type_inline,
		t->msg_type_longform,t->msg_type_deallocate);
	if (t->msg_type_name == MSG_TYPE_PORT)
	{
	    fprintf(stderr,"p %d",
	    	*(port_t *)(((char *)t)+sizeof(msg_type_t)) );
	}
	fprintf(stderr,"\n");
} /* PrintMsg */
#endif DEBUG_OUTPUT

main()
{
    int	i,res,j,recErrCount;
    int	*ip;
    struct	{
	    msg_header_t msgHeader;
	    msg_type_t msgType;
	    union {
		char	bufferSpace[128];
		cthread_t thread;
		int	fd;
	    } msgData;
    } inMsg;
    port_t *portP;
    
    cthread_init();
    mutex_init(spawnMutex = &mutexRec); /* sic */
    /* Get us a notify port */
    port_allocate(task_self_,&notifyPort);
    task_set_special_port(task_self_,TASK_NOTIFY_PORT,notifyPort);
    recErrCount = 0; /* Count of how many errors on msg_receive */
#if DEBUG_SPIN
    while(1) { if (open("/2451465",O_RDWR,0) > 0) break; }
#endif DEBUG_SPIN
    /* Lookup the PostScript server */
    if (res = netname_look_up(name_server_port,"",PS_NAME,&postScriptPort))
    {
	syslog(LOG_WARNING,"%s:Cannot look up WindowServer(%d)!\n",
		myname,res);
	exit(-1);
    }
    /* Fork the listener thread */
    listenThread = cthread_fork((cthread_fn_t)ListenerFunc,NULL);
    /* OK, now go into a loop receiving and acting on messages
       on the notify port */
#if DEBUG_OUTPUT
    syslog(LOG_INFO,"%s:main:listen thread forked.\n",myname);
#endif DEBUG_OUTPUT
    while(1) {
	inMsg.msgHeader.msg_local_port = notifyPort;
	inMsg.msgHeader.msg_size = sizeof(inMsg);
	if (res = msg_receive(&inMsg.msgHeader,MSG_OPTION_NONE,0))
	{
#if DEBUG_OUTPUT
	    fprintf(stderr,"%s:main:Rcv error for ",myname);
	    PrintMsg(&inMsg.msgHeader);
#endif DEBUG_OUTPUT
	    recErrCount++;
	    if (recErrCount > REC_ERR_RETRY)
	    {
	    	syslog(LOG_ERR,"%s: Exiting due to many receive errors!\n",
			myname);
	    	exit(-2);
	    }
	    if (recErrCount < REC_ERR_PRINT)
		syslog(LOG_WARNING,"%s:Error receiving message(%d)!\n",
			myname,res);
	}
	else
	    switch(inMsg.msgHeader.msg_id) {
	    case NOTIFY_PORT_DELETED:
	    	/* if this is any PostScript port, we should just
		   exit.  However, if PostScript died, we will eventually
		   get the notification for the PostScript listen port,
		   so wait for that one before exiting. */
		if (((notification_t *)&inMsg)->notify_port ==
							postScriptPort)
		{
		    syslog(LOG_WARNING,
		    "%s:WindowServer became unavailable!  Exiting!\n",myname);
		    exit(-1);
		}
	    	break;
	    case NOTIFY_MSG_ACCEPTED:
	    case NOTIFY_OWNERSHIP_RIGHTS:
	    case NOTIFY_RECEIVE_RIGHTS:
	    	/* Other known notify message are ignored */
		break;
	    default:
	    	syslog(LOG_WARNING,"%s: Received message with unknown id %d %s",
			myname,inMsg.msgHeader.msg_id,
			"on notify port.\n");
		/* Since the threads generate messages like that for many
		   unknown i/o errors, treat it like a EPIPE or EOF msg */
	    case TCPPSD_EPIPE:
	    	/* Some From thread has encountered an EPIPE on write */
	    case TCPPSD_EOF:
	    	/* Some To thread has encountered an EOF on read */
		ConnectionTeardown(inMsg.msgData.thread);
	    	break;
	    } /* switch(msg_id) */
    } /* while(1) */
} /* main */

void ConnectionCreate(newFd)
int	newFd;
{
	Connection *conn;
	int	i,j,n,res;
	static struct	{
		msg_header_t	mH;
		msg_type_t	mT;
		port_t		mP;
	} initMsg = {
		{ 0, 0, sizeof(initMsg), MSG_TYPE_NORMAL, 0, 0, PSCONTEXTID },
		{ MSG_TYPE_PORT, sizeof(port_t)*8, 1, 1, 0, 0 },
		0 }, psMsg;
	
	psMsg = initMsg;
	conn = (Connection *)malloc(sizeof(Connection));
	conn->fd = newFd;
	port_allocate(task_self_,&conn->myPort);
	psMsg.mH.msg_local_port = psMsg.mP = conn->myPort;
	psMsg.mH.msg_remote_port = postScriptPort;
	if ((res = msg_rpc(&psMsg,MSG_OPTION_NONE,sizeof(psMsg),0,0)) 
							!= KERN_SUCCESS)
	{
#if DEBUG_OUTPUT
	    fprintf(stderr,"%s:ConnectionCreate: rpc error for ",myname);
	    PrintMsg(&psMsg.mH);
#endif DEBUG_OUTPUT
	    syslog(LOG_ERR,
	    		"%s: msg_rpc to WindowServer failed(%d)!\n",myname,res);
	    port_deallocate(task_self_,conn->myPort);
	    free(conn);
	    return;
	}
	conn->psPort = psMsg.mP;
	/* OK, we've done all the prep work we can.  Now hold off
	   any other threads until we're all set up */
	mutex_lock(spawnMutex);
	conn->toThread = cthread_fork((cthread_fn_t)ToFunc,(any_t)conn);
	conn->fromThread = cthread_fork((cthread_fn_t)FromFunc,(any_t)conn);
	conn->next = connections;
	connections = conn;
	mutex_unlock(spawnMutex);
#if DEBUG_OUTPUT
	fprintf(stderr,"%s:ConnectionCreate:new ",myname);
	PrintConn(conn);
#endif DEBUG_OUTPUT
} /* ConnectionCreate */

/*
	ThreadDeath
	Used by ConnectionTeardown to destroy threads
	*/
static void ThreadDeath(joinThread,abortThread,conn,lc)
cthread_t joinThread,abortThread;
Connection *conn,*lc;
{
#if DEBUG_OUTPUT
    fprintf(stderr,"%s:ThreadDeath:killing ",myname); PrintConn(conn);
#endif DEBUG_OUTPUT
    cthread_join(joinThread);
    abortFlag = 1;
/*    thread_abort(cthread_thread(abortThread));
    cthread_join(abortThread);*/
    port_deallocate(task_self_,conn->myPort);
    port_deallocate(task_self_,conn->psPort);
    close(conn->fd);
    if (lc)
    	lc->next = conn->next;
    else
    	connections = conn->next;
} /* ThreadDeath */

void ConnectionTeardown(oldThread)
cthread_t oldThread;
{
    Connection *conn,*lc;
    int	i,j,n,res;
    
    /* Make sure that any connection creation is complete;
       then, keep thread creation out til after we're done */
    mutex_lock(spawnMutex);
    lc = NULL;
    for(conn = connections;conn;lc = conn, conn = conn->next)
	if (conn->toThread == oldThread)
	{
	    ThreadDeath(oldThread,conn->fromThread,conn,lc);
	    mutex_unlock(spawnMutex);
	    return;
	}
	else
	    if (conn->fromThread == oldThread)
	    {
	    	ThreadDeath(oldThread,conn->toThread,conn,lc);
		mutex_unlock(spawnMutex);
		return;
	    }
    mutex_unlock(spawnMutex);
    syslog(LOG_ERR,"%s: Failed to find thread %d sent in msg!\n",
    		myname,oldThread);
} /* ConnectionTeardown */

/*
	NotifyMainThread
	is called from either ToFunc or FromFunc to send
	a message back to the main thread indicating that
	the calling thread is exiting for some reason.
	After that, it exits the sending thread with that
	reason.
	*/
void NotifyMainThread(reason,thread)
int reason;
cthread_t thread;
{
	struct {
		msg_header_t H;
		msg_type_t T;
		cthread_t C;
	} msg;
	int res;
	
	msg.H.msg_size = sizeof(msg);
	msg.H.msg_simple = 1;
	msg.H.msg_local_port = 0;
	msg.H.msg_remote_port = notifyPort;
	msg.H.msg_type = MSG_TYPE_NORMAL;
	msg.H.msg_id = reason;
	msg.T.msg_type_name = MSG_TYPE_INTEGER_32;
	msg.T.msg_type_size = 32;
	msg.T.msg_type_number = 1;
	msg.T.msg_type_inline = 1;
	msg.T.msg_type_longform = 0;
	msg.T.msg_type_deallocate = 0;
	msg.C = thread;
	res = msg_send(&msg,MSG_OPTION_NONE,0);
#if DEBUG_OUTPUT
	if (res != KERN_SUCCESS)
	{
	    fprintf(stderr,"%s:NotifyMainThread:send error for ",myname);
	    PrintMsg(&msg.H);
	}
#endif DEBUG_OUTPUT
	cthread_exit((any_t)reason);
} /* NotifyMainThread */

/*
	FromFunc
	is the routine which runs the 'From' thread: the one
	that forwards responses from PostScript back to the
	client.
	*/
void FromFunc(conn)
Connection *conn;
{
    struct {
    	msg_header_t	mH;
	union {
	    struct {
	    	msg_type_t mT;
		unsigned char msgData[8192-sizeof(msg_type_t)];
	    } mS;
	    struct {
	    	msg_type_long_t mT;
		unsigned char msgData[8192-sizeof(msg_type_long_t)];
	    } mL;
	} m;
    } psMsg;
    int i,j,n,res,len;
    unsigned char *data;
    
    /* Make sure that any connection creation is complete */
    mutex_lock(spawnMutex);
    mutex_unlock(spawnMutex);
    /* This is the loop we stay in until somebody tells us to exit */
    while(1) {
    	/* Set up our local message buffer to get a new message */
    	psMsg.mH.msg_size = sizeof(psMsg);
    	psMsg.mH.msg_local_port = conn->myPort;
	/* Wait for a message from PostScript */
	res = msg_receive(&psMsg.mH,RCV_INTERRUPT,0);
	/* If we were interrupted and it was because the main thread is
	   telling us to abort, then exit cleanly */
	if (abortFlag)
	{
	    abortFlag = 0;
	    cthread_exit(0);
	}
	/* If there was any other error, we generally split.  However,
	   special case on RCV_PORT_CHANGE, which looks harmless */
	switch(res) {
	case RCV_PORT_CHANGE:
	case RCV_TIMED_OUT:
		continue;
	case KERN_SUCCESS:
		break;
	default:
	    syslog(LOG_ERR,"%s:thread %d: rcv error %d!\n",
		    myname,cthread_self(),res);
#if DEBUG_OUTPUT
	    fprintf(stderr,"%s:FromFunc:error on ",myname); PrintConn(conn);
#endif DEBUG_OUTPUT
#if DEBUG_OUTPUT
	    fprintf(stderr,"%s:FromFunc:rcv error for ",myname);
	    PrintMsg(&psMsg.mH);
#endif DEBUG_OUTPUT
	    NotifyMainThread(res,cthread_self());
	} /* switch(res) */
	/* OK, we've successfully gotten the message.  Set data
	   and len to reflect the data in the message. */
	if (psMsg.m.mS.mT.msg_type_longform)
	{
	    if (psMsg.m.mS.mT.msg_type_inline)
	    	data = &psMsg.m.mL.msgData[0];
	    else
	    	data = *(unsigned char **)(&psMsg.m.mL.msgData[0]);
	    len = psMsg.m.mL.mT.msg_type_long_size *
	    		 psMsg.m.mL.mT.msg_type_long_number / 8;
	}
	else
	{
	    if (psMsg.m.mS.mT.msg_type_inline)
	    	data = &psMsg.m.mS.msgData[0];
	    else
	    	data = *(unsigned char **)(&psMsg.m.mS.msgData[0]);
	    len = psMsg.m.mS.mT.msg_type_size *
	    		 psMsg.m.mS.mT.msg_type_number / 8;
	}
	/* Write the data to the socket stream */
	res = write(conn->fd,data,len);
	if (res == -1)
	{
#if DEBUG_OUTPUT
	    fprintf(stderr,"%s:error on write for ",myname); PrintConn(conn);
#endif DEBUG_OUTPUT
	    /* If we got an EPIPE, meaning the connection's gone down,
	       let the main thread know and exit */
	    if (errno == EPIPE)
		NotifyMainThread(TCPPSD_EPIPE,cthread_self());
	    /* If we were notified by the main thread, then just go cleanly */
	    if (abortFlag)
	    {
	        abortFlag = 0;
		cthread_exit(0);
	    }
	}
    } /* while(1) */
} /* FromFunc */

/*
	ToFunc
	is the function which runs the To thread, the one which
	reads data out of the socket stream and sends it on to
	PostScript.
	*/
struct _toMsg {
    	msg_header_t H;
	msg_type_long_t   T;
	unsigned char D[8192];
    };

void ToFunc(conn)
Connection *conn;
{
    int i,j,n,res,len;
    struct _toMsg msg;
    static msg_header_t initHeader = { 0, 1, sizeof(struct _toMsg), 
    			MSG_TYPE_NORMAL, 0, 0, STREAM_DATA_MSG_ID };
    static msg_type_long_t initType =  {{ MSG_TYPE_BYTE, 8, 0, 1, 1, 0 },
    		MSG_TYPE_BYTE, 8, 0 };
    
    /* Make sure that any connection creation is complete */
    mutex_lock(spawnMutex);
    mutex_unlock(spawnMutex);
    /* Set up the outoing message buffer */
    msg.H = initHeader;
    msg.T = initType;
    msg.H.msg_local_port = 0;
    msg.H.msg_remote_port = conn->psPort;
    /* Keep going until something bad happens */
    while(1) {
    	res = read(conn->fd,&msg.D[0],sizeof(msg.D));
	if (res <= 0)
	{
#if DEBUG_OUTPUT
	    fprintf(stderr,"%s:ToFunc:Error with ",myname); PrintConn(conn);
#endif DEBUG_OUTPUT
	    /* If we got an EOF on input, go back to the main thread
	       with this news. */
	    if (res == 0)
		NotifyMainThread(TCPPSD_EOF,cthread_self());
	    /* If we got interrupted, exit cleanly */
	    if ((res == -1)&&(errno == EINTR)&&abortFlag)
	    {
	    	abortFlag = 0;
		cthread_exit(0);
	    }
	    /* Otherwise, if we got an error, we got to blow this
	       popsicle stand */
	    syslog(LOG_ERR,"%s: thread %d: Got error %d on read!\n",
		    myname,cthread_self(),errno);
	    NotifyMainThread(errno,cthread_self());
	}
	/* OK, we got the data successfully.  Pass it on to PS */
	msg.T.msg_type_long_number = res;
	msg.H.msg_size = res + sizeof(msg_header_t)+sizeof(msg_type_long_t);
	res = msg_send(&msg,SEND_INTERRUPT,0);
#if DEBUG_OUTPUT
	if (res != SEND_SUCCESS)
	{
	    fprintf(stderr,"%s:ToFunc:send error for ",myname);
	    PrintMsg(&msg.H);
	}
#endif DEBUG_OUTPUT
	if ((res != SEND_SUCCESS)&&abortFlag)
	{
	    abortFlag = 0;
	    cthread_exit(0);
	}
    } /* while(1) */
} /* ToFunc */

/*
	ListenerFunc
	is the procedure which listens for new clients requesting
	connections.  When such a request arrives, it forwards the
	request to the main thread.
	*/
void ListenerFunc()
{
	int	newFd;
	struct sockaddr_in sin;
	int i,j,n,res;
	
	/* Accept a new connection on the socket inetd passed to us 
	   repeatedly, creating the two new threads each time */
#if DEBUG_OUTPUT
	syslog(LOG_INFO,"%s:listen:listen thread forked.\n",myname);
#endif DEBUG_OUTPUT
	while(1) {
	    if ((newFd = accept(0,(struct sockaddr *)&sin,&i)) == -1)
	    {
		syslog(LOG_ERR,"%s: listen thread: error on accept:",myname);
		perror("");
		exit(-1);
	    }
#if DEBUG_OUTPUT
	    syslog(LOG_INFO,"%s: connection accept on fd %d!\n",myname,newFd);
#endif DEBUG_OUTPUT
	    ConnectionCreate(newFd);
	} /* while(1) */	
} /* ListenerFunc */



