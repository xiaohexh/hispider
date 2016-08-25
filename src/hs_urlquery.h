#ifndef _HS_URL_QUERY_H_
#define _HS_URL_QUERY_H_

#include "hs_event.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#define RDWR_BUF_SIZE	16384

void send_resp_to_client(aeEventLoop *el, int fd, void *privdata, int mask);
void read_query_from_client(aeEventLoop *el, int fd, void *privdata, int mask);
void accept_conn_from_client(aeEventLoop *el, int fd, void *privdata, int mask);

#ifdef __cplusplus
}
#endif

#endif
