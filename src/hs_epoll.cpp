#include <sys/epoll.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct aeApiState {
	int epfd;
	struct epoll_event *events;
}aeApiState;

static int aeApiCreate(aeEventLoop *eventLoop)
{
	aeApiState *state = (aeApiState *)malloc(sizeof(aeApiState));

	if (state == NULL) return -1;

	state->events = (epoll_event *)malloc(sizeof(struct epoll_event) * eventLoop->setsize);
	if (state->events == NULL) {
		free(state);
		return -1;
	}

	state->epfd = epoll_create(1024);
	if (state->epfd == -1) {
		free(state->events);
		free(state);
		return -1;
	}

	eventLoop->apidata = state;
	return 0;
}

static int aeApiResize(aeEventLoop *eventLoop, int setsize)
{
	aeApiState *state = (aeApiState *)eventLoop->apidata;

	state->events = (epoll_event *)realloc(state->events, sizeof(struct epoll_event) * setsize);
	return 0;
}

static void aeApiFree(aeEventLoop *eventLoop)
{
	aeApiState *state = (aeApiState *)eventLoop->apidata;

	close(state->epfd);
	free(state->events);
	free(state);
}

static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask)
{
	aeApiState *state = (aeApiState *)eventLoop->apidata;
	struct epoll_event ee;

	int op = eventLoop->events[fd].mask == AE_NONE ?
		EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events = 0;
	mask |= eventLoop->events[fd].mask;
	if (mask & AE_READABLE) ee.events = (EPOLLIN | EPOLLET);
	if (mask & AE_WRITABLE) ee.events = (EPOLLOUT | EPOLLET);
	ee.data.u64 = 0;
	ee.data.fd = fd;
	if (epoll_ctl(state->epfd, op, fd, &ee) == -1) return -1;
	return 0;
}

static void aeApiDelEvent(aeEventLoop *eventLoop, int fd, int delmask)
{
	aeApiState *state = (aeApiState *)eventLoop->apidata;
	struct epoll_event ee;
	int mask = eventLoop->events[fd].mask & (~delmask);

	ee.events = 0;
	if (mask & AE_READABLE) ee.events = (EPOLLIN | EPOLLET);
	if (mask & AE_WRITABLE) ee.events = (EPOLLOUT | EPOLLET);
	ee.data.u64 = 0;
	ee.data.fd = fd;
	if (mask != AE_NONE) {
		epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
	} else {
		epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
	}
}

static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp)
{
	aeApiState *state = (aeApiState *)eventLoop->apidata;
	int retval, numevents = 0;

	int timeout = (tvp != NULL) ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : -1;
	retval = epoll_wait(state->epfd, state->events, eventLoop->setsize, timeout);
	if (retval > 0) {
		int j;

		numevents = retval;
		for (j = 0; j < numevents; j++) {
			int mask = 0;
			struct epoll_event *e = state->events + j;

			if (e->events & EPOLLIN) mask |= AE_READABLE;
			if (e->events & EPOLLOUT) mask |= AE_WRITABLE;
			if (e->events & EPOLLERR) mask |= AE_WRITABLE;
			if (e->events & EPOLLHUP) mask |= AE_WRITABLE;

			eventLoop->fired[j].fd = e->data.fd;
			eventLoop->fired[j].mask = mask;
		}
	}

	return numevents;
}

static char *aeApiName(void) {
	return "epoll";
}

#ifdef __cplusplus
}
#endif
