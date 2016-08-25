#ifndef _HS_NET_H_
#define _HS_NET_H_

#ifdef __cplusplus
extern "C"
{
#endif

int set_tcpnodelay(int fd);
int set_reuseaddr(int fd);
int set_nonblocking(int fd);

#ifdef __cplusplus
}
#endif

#endif
