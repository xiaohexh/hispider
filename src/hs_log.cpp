#include "hs_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <errno.h>

HSLog::HSLog()
	: m_fd(-1)
	, m_level(-1)
{
}

HSLog::~HSLog()
{
	_log_deinit();
}

HSLog * HSLog::instance()
{
	static HSLog * _ins  = 0;

    if(0 == _ins) {
    	_ins = new HSLog;
    }

    return _ins;
}

int HSLog::_log_init(int level, string name)
{
	int fd;
	fd = open(name.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
	if (fd < 0) {
		_log_stderr("open log file failed: %s", strerror(errno));
		return HS_ERROR;
	}

	m_fd = fd;
	m_name = name;
	m_level = level;

	return HS_OK;
}

int HSLog::_log_deinit()
{
	int status;

	if (m_fd > 0) {
		status = close(m_fd);
		if (status < 0) {
			_log_stderr("close log file failed: %s", strerror(errno));
			return HS_ERROR;
		}
	}

	return HS_OK;
}

void HSLog::log_level_up(int levelname)
{
}

void HSLog::log_level_down(int level)
{
}

int HSLog::_log_loggable(int level)
{
	return (level > m_level) ? 0 : 1;
}

void HSLog::_log(const char *name, int line, const char *fmt, ...)
{
	char buf[LOG_MAX_LEN];
	ssize_t n;
	int len, size;
	struct timeval tv;
	va_list args;

	len = 0;
	size = LOG_MAX_LEN;

	gettimeofday(&tv, NULL);
	len += strftime(buf + len, size - len, "[%Y-%m-%d %H:%M:%S]", localtime(&tv.tv_sec));
	len += snprintf(buf + len, size - len, "[%s %d] ", name, line);

	va_start(args, fmt);
	len += vsnprintf(buf + len, size - len, fmt, args);
	va_end(args);

	buf[len++] = '\n';

	n = write(m_fd, buf, len);
	if (n < 0) {
		_log_stderr("write log to log file failed: %s", strerror(errno));
	}
}

void HSLog::_log_stderr(const char *fmt, ...)
{
	char buf[LOG_MAX_LEN];
	int len, size;
	struct timeval tv;
	ssize_t n;
	va_list args;

	len = 0;
	size = LOG_MAX_LEN;

	gettimeofday(&tv, NULL);
	len += strftime(buf + len, size - len, "[%Y-%m-%d %H:%M:%S] ", localtime(&tv.tv_sec));

	va_start(args, fmt);
	len += vsnprintf(buf + len, size - len, fmt, args);
	va_end(args);

	buf[len++] = '\n';

	n = write(STDERR_FILENO, buf, len);
	if (n < 0) {
		printf("write failed: %s\n", strerror(errno));
	}
}
