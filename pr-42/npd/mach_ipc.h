/*
 * mach_ipc.h	- Interface to Mach-IPC routines.
 *
 * Copyright (c) 1990 by NeXT, Inc., All rights reserved.
 */

/*
 * Include files.
 */
#import	<mach.h>

/*
 * Macros.
 */

/* Macro to initialize a simple message header */
#define MSGHEAD_SIMPLE_INIT(header, size) { \
    (header).msg_simple = 1; \
    (header).msg_size = (size); \
    (header).msg_type = MSG_TYPE_NORMAL; \
}

/* Macro to initialize a msg_type_header for an integer */
#define MSGTYPE_INT_INIT(type) { \
    (type).msg_type_name = MSG_TYPE_INTEGER_32; \
    (type).msg_type_size = 32; \
    (type).msg_type_number = 1; \
    (type).msg_type_inline = 1; \
    (type).msg_type_longform = 0; \
    (type).msg_type_deallocate = 0; \
}

/* Macro to initialize a msg_type_header for a character array */
#define MSGTYPE_CHAR_INIT(type, size) { \
    (type).msg_type_name = MSG_TYPE_CHAR; \
    (type).msg_type_size = 8; \
    (type).msg_type_number = size; \
    (type).msg_type_inline = 1; \
    (type).msg_type_longform = 0; \
    (type).msg_type_deallocate = 0; \
}


/*
 * External definitions.
 */

/* Create a port with a given name */
port_t	PortCreate(const char *name);

/* Find a port with a given name on a given host */
port_t	PortLookup(const char *name, const char *host);

/* Free a port */
void	PortFree(port_t port);

/* Return the number of messages in a port */
int	PortMsgCount(port_t port);

