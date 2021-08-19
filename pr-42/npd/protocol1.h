/*
 * protocol1.h	- Interface file to Protocol 1.0 handlers.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import "npd_prot.h"

/* Initialize the protocol handler */
extern void	NPD1_Init();

/* Process a message that arrives on the main port */
extern void	NPD1_Connect(npd1_con_msg *msg);

/* Display a visual alert */
extern void	NPD1_VisualMessage(const char *message, const char *button1,
				   const char *button2);
extern void	NPD1_RemoteVisualMessage(const char *host, const char *message,
					 const char *button1,
					 const char *button2);
extern void	NPD1_VisualCode(int alert_code);
extern void	NPD1_RemoteVisualCode(const char *host, int alert_code);
extern void	NPD1_AudioAlert(const char *soundFile);
extern void	NPD1_RemoteAudioAlert(const char *host, const char *soundFile);

/* Send ourself an abort message */
extern void	NPD1_AbortSelf();

