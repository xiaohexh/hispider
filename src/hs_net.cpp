#include <fcntl.h>
#include <linux/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" 
{
#endif

int set_tcpnodelay(int fd) 
{
    int nodelay;
    socklen_t len;

    nodelay = 1;
    len = sizeof(nodelay);

    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, len);
}

int set_reuseaddr(int fd) 
{
    int reuse;
    socklen_t len;

    reuse = 1;
    len = sizeof(reuse);

    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, len);
}

int set_nonblocking(int fd) 
{
    int flag;

    flag = fcntl(fd, F_GETFL);
    if (flag < 0) {
        return -1; 
    }   

    return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

#ifdef __cplusplus
}
#endif
