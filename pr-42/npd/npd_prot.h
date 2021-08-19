/*
 * npd_prot.h
 * Copyright (c) 1990 by NeXT, Inc., All rights reserved.
 *
 */

#import <mach.h>

#define NPD_PUBLIC_PORT   "npd_port"
#define	NPD_TEST_PORT	  "npd_test_port"

/*
 * Protocol Version 1.0
 * NeXT Software Release 1.0
 */

/* Message IDs for messages to public port */
#define NPD_1_INFO		100
#define NPD_1_REMOVE		101
#define NPD_1_CONNECT		102
#define NPD_1_AUDIO_ALERT	103
#define NPD_1_VISUAL_ALERT	104
#define NPD_1_CONNECT_FROM_LPD	105
#define NPD_1_PRINTER_STATUS	106

/* Error flag for NPD_1_PRINTER_STATUS */
#define	NPD_1_OS_ERROR	0x10000000

/* Message IDs for info transactions */
#define NPD_NO_MORE_INFO	10
#define NPD_HAVE_INFO		20

/* Message IDs for receive messages */
#define NPD_SEND_HEADER  1
#define NPD_SEND_PAGE    2
#define NPD_SEND_TRAILER 3

typedef struct {
  msg_header_t   head;
  msg_type_t     printer_type;
  char           printer[1024];
  msg_type_t     orig_host_type;
  char           orig_host[1024];
  msg_type_t     user_type;
  char           user[1024];
  msg_type_t     copies_type;
  int            copies;
} npd1_con_msg;

typedef struct {
  msg_header_t   head;
  msg_type_t     soundfile_type;
  char           soundfile[1024];
} npd1_audio_alert_msg;

typedef struct {
  msg_header_t   head;
  msg_type_t     alert_code_type;
  int		 alert_code;
  msg_type_t     message_type;
  char           message[1024];
  msg_type_t     button1_type;
  char           button1[1024];
  msg_type_t     button2_type;
  char           button2[1024];
  msg_type_t     orig_host_type;
  char           orig_host[1024];
} npd1_visual_alert_msg;

typedef struct {
  msg_header_t   head;
  msg_type_t     printer_type;
  char printer[64];
  msg_type_t     host_type;
  char host[64];
  msg_type_t     user_type;
  char user[64];
  msg_type_t     doc_type;
  char doc[64];
  msg_type_t     creator_type;
  char creator[64];
  msg_type_t     size_type;
  int size;
  msg_type_t     pages_type;
  int pages;
  msg_type_t     feed_type;
  int feed;
  msg_type_t     job_type;
  int job;
  msg_type_t     time_type;
  int time;
} npd1_info_msg;

typedef struct {
  msg_header_t   head;
  msg_type_long_t type;
  char *data;
} npd1_receive_msg;

