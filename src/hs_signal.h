#ifndef _HS_SIGNAL_H_
#define _HS_SIGNAL_H_

#include "hispider.h"

#include <signal.h>

struct signal {
    int  signo;
    char *signame;
    int  flags;
    void (*handler)(int signo);
};

int signal_init(void);
void signal_handler(int signo);

#endif
