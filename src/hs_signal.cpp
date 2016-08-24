#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include "hispider.h"
#include "hs_signal.h"

static struct signal signals[] = {
    { SIGUSR1, "SIGUSR1", 0,                 signal_handler },
    { SIGUSR2, "SIGUSR2", 0,                 signal_handler },
    { SIGINT,  "SIGINT",  0,                 signal_handler },
    { SIGTERM, "SIGTERM", 0,               	 signal_handler },
    { SIGSEGV, "SIGSEGV", (int)SA_RESETHAND, signal_handler },
    { SIGPIPE, "SIGPIPE", 0,                 SIG_IGN },
    { 0,        NULL,     0,                 NULL }
};

int signal_init(void)
{
    struct signal *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        int status;
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sig->handler;
        sa.sa_flags = sig->flags;
        sigemptyset(&sa.sa_mask);

        status = sigaction(sig->signo, &sa, NULL);
        if (status < 0) {
            log_error("sigaction(%s) failed: %s", sig->signame,
                      strerror(errno));
            return HS_ERROR;
        }
    }

    return HS_OK;
}

void signal_handler(int signo)
{
    struct signal *sig;
    void (*action)(void);
    string actionstr;
    bool done;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }
    assert(sig->signo != 0);

    action = NULL;
    done = false;

    switch (signo) {
    case SIGUSR1:
		// reserved (can used to get stat)
        break;

    case SIGUSR2:
		// reserved (can used to get stat)
        break;

    case SIGINT:
    case SIGTERM:
        done = true;
        actionstr = ", exiting";
		action = hs_stop;
        break;

    case SIGSEGV:
        actionstr = ", core dumping";
        raise(SIGSEGV);
        break;

    default:
		break;
    }

    log_debug(LOG_INFO, "signal %d (%s) received%s", signo, sig->signame, actionstr.c_str());

    if (action != NULL) {
        action();
    }

    if (done) {
        exit(1);
    }
}
