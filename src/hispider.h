#ifndef _HI_SPIDER_H_
#define _HI_SPIDER_H_

#include <string>
using std::string;

#include "hs_log.h"
#include "hs_config.h"
#include "hs_signal.h"
#include "hs_thread.h"
#include "hs_event.h"
#include "hs_net.h"
#include "hs_urlparser.h"
#include "hs_urlquery.h"

#define CRLF        "\x0d\x0a"

#define DEF_LOG_FILE    "../logs/hispider.log"
#define DEF_CFG_FILE    "../conf/hispider.ini"
#define DEF_PID_FILE    "./hispider.pid"

#define HS_OK       0
#define HS_ERROR    -1
#define HS_NEXIST	100

#define PORT    10001
#define BACKLOG	1024

struct instance {
	string conf_file;
	string log_file;
	int    log_level;
	string pid_file;
	pthread_t url_tid;
	int		stop;

    aeEventLoop *el;
    int listenfd;
};

void hs_stop(void);

#endif
