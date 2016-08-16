#ifndef _HI_SPIDER_H_
#define _HI_SPIDER_H_

#include <string>
using std::string;

#include "hs_log.h"

#define CRLF        "\x0d\x0a"

#define DEF_LOG_FILE    "../logs/hispider.log"
#define DEF_CFG_FILE    "../conf/hispider.ini"
#define DEF_PID_FILE    "./hispider.pid"

#define HS_OK       0
#define HS_ERROR    -1

struct instance {
	string conf_file;
	string log_file;
	int    log_level;
	string pid_file;
};

#endif
