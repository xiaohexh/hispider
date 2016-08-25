#include "hs_urlquery.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>

#include <string>

#include "hispider.h"

#ifdef __cplusplus
extern "C" 
{
#endif

void send_resp_to_client(aeEventLoop *el, int fd, void *privdata, int mask)
{
    int n, status;
    char *resp = (char *)privdata;

    n = write(fd, resp, strlen(resp));
    if (n < 0) {
        if (errno == EAGAIN) {
            n = 0;
        } else {
            close(fd);
        }
        log_error("write to client failed!\n");
    }

    log_error("send to client success\n");

    aeDeleteFileEvent(el, fd, AE_WRITABLE);

    status = aeCreateFileEvent(el, fd, AE_READABLE, read_query_from_client, NULL);
    if (status < 0) {
        log_error("send_resp_to_client aeCreateFileEvent failed!\n");
    }
}

void read_query_from_client(aeEventLoop *el, int fd, void *privdata, int mask)
{
    log_error("read from client is triggered\n");

    char buf[RDWR_BUF_SIZE];

    int status;
    int n;

	memset(buf, 0, sizeof(buf));

    n = read(fd, buf, sizeof(buf));
    if (n == 0) {
        log_error("client close connection\n");
        close(fd);
        return;
    } else if (n < 0) {
        if (errno == EAGAIN) {
            n = 0;
        } else {
            log_error("read from client failed: %s\n", strerror(errno));
            close(fd);
            return;
        }
    }
    if (n > 0) {
        log_error("read from client: %s, read n:%d, size:%lu\n", buf, n, strlen(buf));
        aeDeleteFileEvent(el, fd, AE_READABLE);

		/* TODO:parse request from json/protobuf format 
		 *		and format response into json/protobuf
		 */
		std::string resq = std::string(buf).substr(0, strlen(buf) - 1);
		std::string ip;

		status = UrlParser::instance()->get_ip(resq, ip);
		if (status == HS_OK) {
			log_debug(LOG_DEBUG, "get url:%s ip:%s", resq.c_str(), ip.c_str());
		} else {
			log_error("get url:%s ip failed", resq.c_str());
		}

        status = aeCreateFileEvent(el, fd, AE_WRITABLE, send_resp_to_client, (void *)ip.c_str());
        if (status < 0) {
            log_error("read_query_from_client aeCreateFileEvent failed!\n");
        }
    }
}

void accept_conn_from_client(aeEventLoop *el, int fd, void *privdata, int mask)
{
    int status;
    int connfd;

    struct sockaddr_in clientaddr;

    socklen_t len = sizeof(clientaddr);

    connfd = accept(fd, (struct sockaddr *)&clientaddr, &len);
    if (connfd < 0) {
        log_error("accept failed: %s\n", strerror(errno));
        return;
    }

    status = set_nonblocking(connfd);
    if (status < 0) {
        log_error("set nonblocking failed: %s\n", strerror(errno));
        return;
    }

    status = set_tcpnodelay(connfd);
    if (status < 0) {
        log_error("set nodelay failed: %s\n", strerror(errno));
        return;
    }

    status = aeCreateFileEvent(el, connfd, AE_READABLE, read_query_from_client, NULL);
    if (status < 0) {
        log_error("accept_conn_from_client aeCreateFileEvent failed!\n");
    }
}

#ifdef __cplusplus
}
#endif
