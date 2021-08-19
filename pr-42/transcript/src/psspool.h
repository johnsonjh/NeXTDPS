/* psspool.h
 *
 * Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See notice of rights in Notice file in TranScript
 * library directory -- probably /usr/lib/ps/Notice
 * RCSID: $Header: psspool.h,v 2.2 87/11/17 16:52:34 byron Rel $
 *
 * nice constants for printcap spooler filters
 *
 * RCSLOG:
 * $Log:	psspool.h,v $
 * Revision 2.2  87/11/17  16:52:34  byron
 * Release 2.1
 * 
 * Revision 2.1.1.2  87/11/12  13:42:02  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.1  87/04/23  10:26:50  byron
 * Copyright notice.
 * 
 * Revision 2.2  86/11/02  14:13:37  shore
 * Product Update
 * 
 * Revision 2.1  85/11/24  11:51:12  shore
 * Product Release 2.0
 * 
 * Revision 1.2  85/05/14  11:26:54  shore
 * 
 * 
 *
 */

#define PS_INT	'\003'
#define PS_EOF	'\004'
#define PS_STATUS '\024'

/* error exit codes for lpd-invoked processes */
#define TRY_AGAIN 1
#define THROW_AWAY 2

