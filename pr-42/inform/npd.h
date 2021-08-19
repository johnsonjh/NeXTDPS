#define NEW_APPKIT

#define NPD_PUBLIC_PORT   "npd_port"


/* message IDs for connect messages */
#define NPD_INFO		100
#define NPD_REMOVE		101
#define NPD_CONNECT		102
#define NPD_AUDIO_ALERT		103
#define NPD_VISUAL_ALERT	104
#define NPD_CONNECT_FROM_LPD	105
#define NPD_PRINTER_STATUS	106

/* message IDs for info transactions */
#define NPD_NO_MORE_INFO	10
#define NPD_HAVE_INFO		20

/* message IDs for receive messages */
#define NPD_SEND_HEADER  1
#define NPD_SEND_PAGE    2
#define NPD_SEND_TRAILER 3

typedef struct npd_npcomm_con_msg {
  msg_header_t   head;
  msg_type_t     printer_type;
  char           printer[1024];
  msg_type_t     user_type;
  char           user[1024];
  msg_type_t     copies_type;
  int            copies;
  msg_type_t     orig_host_type;
  char           orig_host[1024];
} npd_npcomm_con_msg;

#ifdef NEW_APPKIT
typedef struct npd_con_msg {
  msg_header_t   head;
  msg_type_t     printer_type;
  char           printer[1024];
  msg_type_t     orig_host_type;
  char           orig_host[1024];
  msg_type_t     user_type;
  char           user[1024];
  msg_type_t     copies_type;
  int            copies;
} npd_con_msg;
#else

typedef struct npd_con_msg {
  msg_header_t   head;
  msg_type_t     printer_type;
  char           printer[1024];
  msg_type_t     user_type;
  char           user[1024];
  msg_type_t     copies_type;
  int            copies;
} npd_con_msg;
#endif

typedef struct npd_audio_alart_msg {
  msg_header_t   head;
  msg_type_t     soundfile_type;
  char           soundfile[1024];
} npd_audio_alert_msg;

typedef struct npd_visual_alart_msg {
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
} npd_visual_alert_msg;


typedef struct npd_info_msg {
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
} npd_info_msg;

typedef struct npd_receive_msg {
  msg_header_t   head;
  msg_type_long_t type;
  char *data;
} npd_receive_msg;

