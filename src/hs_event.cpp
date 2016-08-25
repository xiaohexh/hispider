#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "hs_event.h"

#include "hs_epoll.cpp"

#ifdef __cplusplus
extern "C"
{
#endif

aeEventLoop *aeCreateEventLoop(int setsize)
{
	aeEventLoop *eventLoop;
	int i;

	if ((eventLoop = (aeEventLoop *)malloc(sizeof(*eventLoop))) == NULL) goto err;
	eventLoop->events = (aeFileEvent *)malloc(sizeof(aeFileEvent) * setsize);
	if (eventLoop->events == NULL) goto err;
	eventLoop->fired = (aeFiredEvent *)malloc(sizeof(aeFiredEvent) * setsize);
	if (eventLoop->fired == NULL) goto err;
	eventLoop->setsize = setsize;
	eventLoop->lastTime = time(NULL);
	eventLoop->timeEventHead = NULL;
	eventLoop->timeEventNextId = 0;
	eventLoop->stop = 0;
	eventLoop->maxfd = -1;
	eventLoop->beforesleep = NULL;
	if (aeApiCreate(eventLoop) == -1) goto err;

	/* Events with mask == AE_NONE are not set. So let's initialize the
     * vector with it. */
	for (i = 0; i < setsize; i++) {
		eventLoop->events[i].mask = AE_NONE;
	}

	return eventLoop;

err:
	if (eventLoop) {
		free(eventLoop->fired);
		free(eventLoop->events);
		free(eventLoop);
	}

	return NULL;
}

int aeGetSetSize(aeEventLoop *eventLoop)
{
	return eventLoop->setsize;
}

int aeResizeSetSize(aeEventLoop *eventLoop, int setsize)
{
	int i;

	if (setsize == eventLoop->setsize) return AE_OK;
	if (eventLoop->maxfd >= setsize) return AE_ERR;
	if (aeApiResize(eventLoop, setsize) == -1) return AE_ERR;

	eventLoop->events = (aeFileEvent *)realloc(eventLoop->events, sizeof(aeFileEvent) * setsize);
	eventLoop->setsize = setsize;

	for (i = eventLoop->maxfd; i < setsize; i++) {
		eventLoop->events[i].mask = AE_NONE;
	}
	return AE_OK;
}

void aeDeleteEventLoop(aeEventLoop *eventLoop) 
{
	aeApiFree(eventLoop);
	free(eventLoop->fired);
	free(eventLoop->events);
	free(eventLoop);
}

void aeStop(aeEventLoop *eventLoop)
{
	eventLoop->stop = 1;
}

int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
					aeFileProc *proc, void *clientData)
{
	if (fd >= eventLoop->setsize) {
		errno = ERANGE;
		return AE_ERR;
	}

	aeFileEvent *fe = &eventLoop->events[fd];

	if (aeApiAddEvent(eventLoop, fd, mask) == -1) {
		return AE_ERR;
	}
	fe->mask |= mask;
	if (mask & AE_READABLE) fe->rfileProc = proc;
	if (mask & AE_WRITABLE) fe->wfileProc = proc;
	fe->clientData = clientData;
	if (fd > eventLoop->maxfd)
		eventLoop->maxfd = fd;

	return AE_OK;
}

void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask)
{
	if (fd >= eventLoop->setsize) return;
	aeFileEvent *fe = &eventLoop->events[fd];

	if (fe->mask == AE_NONE) return;
	fe->mask = fe->mask & (~mask);
	if (fd == eventLoop->maxfd && fe->mask == AE_NONE) {
		int j;

		/* update the max fd */
		for (j = eventLoop->maxfd - 1; j >= 0; j--) {
			if (eventLoop->events[j].mask != AE_NONE)
				break;
		}
		eventLoop->maxfd = j;
	}

	aeApiDelEvent(eventLoop, fd, mask);
}

int aeGetFileEvents(aeEventLoop *eventLoop, int fd)
{
	if (fd >= eventLoop->setsize) return 0;
	aeFileEvent *fe = &eventLoop->events[fd];

	return fe->mask;
}

static void aeGetTime(long *seconds, long *milliseconds)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	*seconds = tv.tv_sec;
	*milliseconds = tv.tv_usec / 1000;
}

static void aeAddMillisecondsToNow(long long milliseconds, long *sec, long *ms)
{
	long cur_sec, cur_ms, when_sec, when_ms;

	aeGetTime(&cur_sec, &cur_ms);
	when_sec = cur_sec + milliseconds / 1000;
	when_ms = cur_ms + milliseconds % 1000;
	if (when_ms >= 1000) {
		when_sec++;
		when_ms -= 1000;
	}

	*sec = when_sec;
	*ms = when_ms;
}

long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
		aeTimeProc *proc, void *clientData,
		aeEventFinalizerProc *finalizerProc)
{
	long long id = eventLoop->timeEventNextId++;
	aeTimeEvent *te;

	te = (aeTimeEvent *)malloc(sizeof(*te));
	if (te == NULL) return AE_ERR;
	te->id = id;
	aeAddMillisecondsToNow(milliseconds, &te->when_sec, &te->when_ms);
	te->timeProc = proc;
	te->finalizerProc = finalizerProc;
	te->clientData = clientData;
	te->next = eventLoop->timeEventHead;
	eventLoop->timeEventHead = te;
	return id;
}

int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id)
{
	aeTimeEvent *te, *prev= NULL;

	te = eventLoop->timeEventHead;
	while (te) {
		if (te->id == id) {
			if (prev == NULL) {
				eventLoop->timeEventHead = te->next;
			} else {
				prev->next = te->next;
			}
			if (te->finalizerProc) {
				te->finalizerProc(eventLoop, te->clientData);
			}
			free(te);
			return AE_OK;
		}
		prev = te;
		te = te->next;
	}

	return AE_ERR;
}

static aeTimeEvent *aeSearchNearestTimer(aeEventLoop *eventLoop)
{
	aeTimeEvent *te = eventLoop->timeEventHead;
	aeTimeEvent *nearest = NULL;

	while (te) {
		if (nearest == NULL || te->when_sec < nearest->when_sec ||
			(te->when_sec == nearest->when_sec &&
			 te->when_ms < nearest->when_ms)) {
			nearest = te;
		}
		te = te->next;
	}
	return nearest;
}

static int processTimeEvents(aeEventLoop *eventLoop)
{
	int processed = 0;
	aeTimeEvent *te;
	long long maxId;
	time_t now = time(NULL);

	if (now < eventLoop->lastTime) {
		te = eventLoop->timeEventHead;
		while (te) {
			te->when_sec = 0;
			te = te->next;
		}
	}

	eventLoop->lastTime = now;

	te = eventLoop->timeEventHead;
	maxId = eventLoop->timeEventNextId - 1;
	while (te) {
		long now_sec, now_ms;
		long long id;

		if (te->id > maxId) {
			te = te->next;
			continue;
		}

		aeGetTime(&now_sec, &now_ms);
		if (now_sec > te->when_sec ||
			(now_sec == te->when_sec && now_ms > te->when_ms)) {
			int retval;

			id = te->id;
			retval = te->timeProc(eventLoop, id, te->clientData);
			processed++;

			if (retval != AE_NOMORE) {
				aeAddMillisecondsToNow(retval, &te->when_sec, &te->when_ms);
			} else {
				aeDeleteTimeEvent(eventLoop, id);
			}

			/* restart from list head after process an event, cauz list may be changed */
			te = eventLoop->timeEventHead;
		} else {
			te = te->next;
		}
	}

	return processed;
}

/* Process every pengding time event, then every pending file event. */
int aeProcessEvents(aeEventLoop *eventLoop, int flags)
{
	int processed, numevents;

	if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS)) return 0;

	if (eventLoop->maxfd != -1 ||
		((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT))) {
		int j;
		aeTimeEvent *shortest = NULL;
		struct timeval tv, *tvp;

		if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT)) {
			shortest = aeSearchNearestTimer(eventLoop);
		}
		if (shortest) {
			long now_sec, now_ms;

			aeGetTime(&now_sec, &now_ms);
			tvp = &tv;
			tvp->tv_sec = shortest->when_sec-now_sec;
			if (shortest->when_ms < now_ms) {
				tvp->tv_usec = (shortest->when_ms + 1000 - now_ms) * 1000;
				tvp->tv_sec--;
			} else {
				tvp->tv_usec = (shortest->when_ms - now_ms) * 1000;
			}
			if (tvp->tv_sec < 0) tvp->tv_sec = 0;
			if (tvp->tv_usec < 0) tvp->tv_usec = 0;
		} else {
			if (flags & AE_DONT_WAIT) {
				tvp->tv_sec = 0;
				tvp->tv_usec = 0;
				tvp = &tv;
			} else {
				tvp = NULL;
			}
		}

		numevents = aeApiPoll(eventLoop, tvp);
		for (j = 0; j < numevents; j++) {
			aeFileEvent *fe = &eventLoop->events[eventLoop->fired[j].fd];
			int mask = eventLoop->fired[j].mask;
			int fd = eventLoop->fired[j].fd;
			int rfired = 0;

			if (fe->mask & mask & AE_READABLE) {
				rfired = 1;
				fe->rfileProc(eventLoop, fd, fe->clientData, mask);
			}
			if (fe->mask & mask & AE_WRITABLE) {
				if (!rfired || fe->wfileProc != fe->rfileProc)
					fe->wfileProc(eventLoop, fd, fe->clientData, mask);
			}
			processed++;
		}
	}

	if (flags & AE_TIME_EVENTS) {
		processed += processTimeEvents(eventLoop);
	}

	return processed;
}

void aeMain(aeEventLoop *eventLoop)
{
	eventLoop->stop = 0;
	while (!eventLoop->stop) {
		if (eventLoop->beforesleep != NULL) {
			eventLoop->beforesleep(eventLoop);
		}
		aeProcessEvents(eventLoop, AE_ALL_EVENTS);
	}
} 

char *aeGetApiName(void)
{
	return aeApiName();
}

void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep)
{
	eventLoop->beforesleep = beforesleep;
}

#ifdef __cplusplus
}
#endif
