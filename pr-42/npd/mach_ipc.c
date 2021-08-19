/*
 * mach_ipc.c	- Port and message handling routines.
 *
 * Copyright (c) 1990 by NeXT, Inc.,  All rights reserved.
 */

/*
 * Include files.
 */
#import "mach_ipc.h"
#import "log.h"

#import <mach_init.h>
#import <servers/netname.h>


/*
 * External routines
 */

/**********************************************************************
 * Routine:	PortCreate() - Create a port.
 *
 * Function:	Create a port and register it with the name server as
 *		"name".  If name is not set, don't try to register it.
 *
 * Args:	name	- the name of the port, null names will not be
 *			  registered.
 *
 * Returns:	PORT_NULL - failure
 *		new port - success
 **********************************************************************/
port_t
PortCreate(char *name)
{
    port_t newPort;

    /* Allocate a new port */
    if ( port_allocate(task_self(), &newPort) != KERN_SUCCESS ) {
	return (PORT_NULL);
    }

    /* Register it with the name server */
    if ( name && netname_check_in(name_server_port, name, PORT_NULL,
				  newPort) != KERN_SUCCESS ) {
	return (PORT_NULL);
    }

    return (newPort);
}

/**********************************************************************
 * Routine:	PortLookup() - Get a port from the name server.
 *
 * Function:	Get a port of a given name from a given host's name
 *		server.  If "host" is null then assume the localhost.
 *
 * Args:	name - the name of the port
 *		host - the name of the host, null for localhost
 *
 * Returns:	PORT_NULL - failure
 *		named port - success
 **********************************************************************/
port_t
PortLookup(char *name, char *host)
{
  port_t newPort;

  if ( !host ) {
      host = "";
  }
  
  if ( netname_look_up(name_server_port, host, name,
		       &newPort ) != KERN_SUCCESS ) {
      return (PORT_NULL);
  }

  return (newPort);
}

/**********************************************************************
 * Routine:	PortFree() - Free a port
 *
 * Args:	port - the port to be freed.
 **********************************************************************/
void
PortFree(port_t port)
{
    port_deallocate(task_self(), port);
}

/**********************************************************************
 * Routine:	PortMsgCount() - Count the messages in a port.
 *
 * Function:	Do a port_status and retreive the number of messages
 *		from it.
 **********************************************************************/
int
PortMsgCount(port_t port)
{
    kern_return_t	error;
    port_set_name_t	enabled;
    int			num_msgs;
    int			backlog;
    boolean_t		owner;
    boolean_t		receiver;

    if ((error = port_status(task_self(), port, &enabled, &num_msgs,
			     &backlog, &owner, &receiver)) != KERN_SUCCESS) {
	LogError("PostMsgCount: port_status failed, error = %d");
	return (0);
    }

    return(num_msgs);
}

				
