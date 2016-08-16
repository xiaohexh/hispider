#include <iostream>
#include <deque>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>

#include "hispider.h"

static int show_help;
static int show_version;
static int daemonize;

static struct option long_options[] = { 
    { "help",       no_argument,    NULL,   'h' },
    { "version",    no_argument,    NULL,   'v' },
    { "daemonize",  no_argument,    NULL,   'd' },
    { "cfg-file",  	no_argument,    NULL,   'c' },
    { "log-file",  	no_argument,    NULL,   'l' },
    { "pid-file",   no_argument,    NULL,   'p' },
    { NULL,         0,              NULL,   0   }   
};

static const char *short_options = "hvdc:l:p:";


int get_options(int argc, char **argv, struct instance *hsi)
{
    int c;

    for (;;) {
        c = getopt_long(argc, argv, short_options, long_options, NULL);
        if (c == -1) {
            break;
        }   

        switch (c) {
        case 'h':
            show_version = 1;
            show_help = 1;
            break;

        case 'v':
            show_version = 1;
            break;

        case 'd':
            daemonize = 1;
            break;

        case 'c':
            hsi->conf_file = optarg;
            break;

        case 'l':
            hsi->log_file = optarg;
            break;

        case 'p':
            hsi->pid_file = optarg;
            break;

        default:
            // errlog
            printf("invalid option '%c'\n", c);
            return -1;
        }
    }

    return 0;
}

void hs_daemonize(void)
{
    int fd; 

    if (fork() != 0) exit(0); /* parent exits */
    setsid();   /* create a new session */

    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO)
            close(fd);
    }
}

void create_pid_file(struct instance *hsi) {
  FILE *fp = fopen(hsi->pid_file.c_str(), "w");
  if (fp) {
    fprintf(fp,"%d\n",(int)getpid());
    fclose(fp);
  }
}

static void show_usage(void)
{
    printf(
        "Usage: HISpider [-hvd] [-c conf file] [-o output file]" CRLF
        "                 [-p pid file] [-m mbuf size]" CRLF
        "");
    printf(
        "Options:" CRLF
        "  -h, --help             : this help" CRLF
        "  -V, --version          : show version and exit" CRLF
        "  -d, --daemonize        : run as a daemon" CRLF);
    printf(
        "  -c, --cfg-file=S      : set configuration file (default: %s)" CRLF
        "  -l, --log-file=S      : set log file (default: %s)" CRLF
        "  -p, --pid-file=S       : set pid file (default: %s)" CRLF
        "",
        DEF_CFG_FILE,
		DEF_LOG_FILE,
        DEF_PID_FILE != NULL ? DEF_PID_FILE : "off"
        );
}

void set_default_options(struct instance *hsi)
{
    hsi->conf_file = DEF_CFG_FILE;
    hsi->log_file = DEF_LOG_FILE;
    hsi->log_level = LOG_ERR;
    hsi->pid_file = DEF_PID_FILE;
}

int init(struct instance *hsi)
{
	int status;

   	status = log_init(LOG_DEBUG, hsi->log_file);
   	if (status < 0) {
       	log_stderr("log_init failed");
    	return HS_ERROR;
   	}

  	HSConfig *hs_config = HSConfig::instance();
  	status = hs_config->loadConfigFile(hsi->conf_file.c_str());
  	if (status < 0) {
    	log_error("load config file '%s' failed\n", hsi->conf_file.c_str());
      	return HS_ERROR;
  	}

  	int level = hs_config->getConfigInt("log", "level");
    log_debug(LOG_DEBUG, "log level is %d", level);

	return HS_OK;
}

int main(int argc, char **argv) {

  int port = 9090;
  int status;

  struct instance hsi;

  set_default_options(&hsi);

  status = get_options(argc, argv, &hsi);
  if (status < 0) {
      fprintf(stderr, "parse common line params failed!\n");
	  exit(0);
  }

  if (show_version) {
      printf("hispider version 1.0.0\n");
      if (show_help) {
          show_usage();
      }
	  exit(0);
  }

  if (daemonize) hs_daemonize();

  status = init(&hsi);
  if (status != HS_OK) {
	  return 1;
  }

  if (daemonize) create_pid_file(&hsi);

  return 0;
}
