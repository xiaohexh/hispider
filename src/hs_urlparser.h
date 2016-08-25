#ifndef _HS_URL_PARSER_H_
#define _HS_URL_PARSER_H_

#include "hs_thread.h"

#include <string>
#include <map>
#include <queue>
using std::string;
using std::map;
using std::queue;

void *parse_loop(void *arg);

//维护url原始字符串
typedef struct surl {
    string  url;
    int     level;  // url抓取深度
    int     type;   // 抓取类型
} surl;

//解析后的
typedef struct ourl {
    string  domain; //域名
    string  path;   //路径
    string  ip;     //IP
    int     port;   //端口
    int     level;  //深度
} ourl;

class UrlParser
{
public:
	~UrlParser();

	static UrlParser* instance();

	ourl *surl2ourl(surl *s_url);
	void *url_parse(void *arg);

	void add2map(const string& domain, const string& ip);

	int get_ip(const string &url, string &ip);

private:

	UrlParser();
	UrlParser(const UrlParser& objLog);
	UrlParser& operator = (const UrlParser& objLog);

	pthread_mutex_t	m_lock;
	pthread_cond_t	m_cond;

	queue<surl *>   surl_q;
	map<string, string> host_ip_m;
};

#endif
