#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>

#include <evdns.h>
#include <event2/util.h>
#include <event2/event.h>

#include <iostream>
using std::cout;
using std::endl;

#include "hispider.h"
#include "hs_urlparser.h"

UrlParser::UrlParser()
{
    pthread_mutex_init(&m_lock, NULL);
    pthread_cond_init(&m_cond, NULL);
}

UrlParser *UrlParser::instance()
{
    static UrlParser * _ins  = 0;

    if(0 == _ins) {
        _ins = new UrlParser;
    }
    return _ins;
}

//DNS解析回调函数
static void dns_callback(int result, char type,
						 int count, int ttl,
						 void *addresses, void *arg)
{
    ourl * o_url = (ourl *)arg;
    struct in_addr *addrs = (in_addr *)addresses;

    if (result != 0 || count == 0) {
        printf("Dns resolve fail: %s", o_url->domain.c_str());
    } else {
        char * ip = inet_ntoa(addrs[0]);
        printf("Dns resolve OK: %s -> %s\n", o_url->domain.c_str(), ip);
		UrlParser::instance()->add2map(o_url->domain, ip);
    }
    event_loopexit(NULL); // not safe for multithreads
}

void UrlParser::add2map(const string &domain, const string &ip)
{
	host_ip_m[domain] = ip;
}

void *UrlParser::url_parse(void *arg)
{
    surl *s_url = NULL;
    ourl *o_url = NULL;

	cout << "enter UrlParser::url_parse" << endl;

    pthread_mutex_lock(&m_lock);
    while (surl_q.empty()) {
        pthread_cond_wait(&m_cond, &m_lock);
    }
    s_url = surl_q.front();
    surl_q.pop();
    pthread_mutex_unlock(&m_lock);

    o_url = surl2ourl(s_url);
    if (o_url == NULL) {
        return NULL;
    }

    map<string, string>::iterator iter = host_ip_m.find(o_url->domain);
    if (iter == host_ip_m.end()) {
        event_base * base = event_init();
        evdns_init();
        evdns_resolve_ipv4(o_url->domain.c_str(), 0, dns_callback, o_url);
        event_dispatch();
        event_base_free(base);
    }

	delete s_url;

    return NULL;
}

ourl *UrlParser::surl2ourl(surl *s_url)
{
    ourl *o_url = new ourl();
    if (o_url == NULL) {
        return NULL;
    }

    string::size_type pos;
    // get domain & path
    if ((pos = s_url->url.find_first_of("/")) == string::npos) { // no path
        o_url->domain = s_url->url;
    } else {
        o_url->domain = s_url->url.substr(0, pos);
        o_url->path = s_url->url.substr(pos + 1);;
    }

    // get port number
    if ((pos = s_url->url.find_first_of(":")) == string::npos) {
        o_url->port = 80;
    } else {
        o_url->port = atoi(s_url->url.substr(pos + 1).c_str());
        if (o_url->port == 0) {
            o_url->port = 80;
        }
    }

    // get level
    o_url->level = s_url->level;

    return o_url;
}

const string &UrlParser::get_ip(const string& url)
{
	/*
    pthread_mutex_lock(&m_lock);

	surl *s_url = new surl();
    s_url->url = url;
    surl_q.push(s_url);
    pthread_cond_signal(&m_cond);

    pthread_mutex_unlock(&m_lock);
	*/
	host_ip_m["www.baidu.com"] = "115.239.211.112";
	if (host_ip_m.find(url) != host_ip_m.end()) {
    	return host_ip_m[url];
	}
	return string("");
}

void *parse_loop(void *arg)
{
	cout << "parse thread begin loop" << endl;
	struct instance *hsi = (struct instance *)arg;
	while (!hsi->stop) {
		UrlParser::instance()->url_parse(NULL);
	}
}
