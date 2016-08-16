#ifndef _HS_LOG_H_
#define _HS_LOG_H_

#include "hispider.h"

#include <string>

using std::string;

#define LOG_EMERG   0   /* system in unusable */
#define LOG_ALERT   1   /* action must be taken immediately */
#define LOG_CRIT    2   /* critical conditions */
#define LOG_ERR     3   /* error conditions */
#define LOG_WARN    4   /* warning conditions */
#define LOG_NOTICE  5   /* normal but significant condition (default) */
#define LOG_INFO    6   /* informational */
#define LOG_DEBUG   7   /* debug messages */
#define LOG_VERB    8   /* verbose messages */
#define LOG_VVERB   9   /* verbose messages on crack */
#define LOG_VVVERB  10  /* verbose messages on ganga */
#define LOG_PVERB   11  /* periodic verbose messages on crack */

#define LOG_MAX_LEN 256 /* max length of log message */

#define log_init(_level, _name) 									\
	HSLog::instance()->_log_init(_level, _name);

#define log_debug(_level, ...) do {									\
	if (HSLog::instance()->_log_loggable(_level)) {					\
		HSLog::instance()->_log(__FILE__, __LINE__, __VA_ARGS__);	\
	}																\
} while (0)

#define log_warn(...) do {											\
	if (HSLog::instance()->_log_loggable(LOG_WARN)) {				\
		HSLog::instance()->_log(__FILE__, __LINE__, __VA_ARGS__);	\
	}																\
} while (0)

#define log_error(...) do {											\
	if (HSLog::instance()->_log_loggable(LOG_ERR)) {					\
		HSLog::instance()->_log(__FILE__, __LINE__, __VA_ARGS__);	\
	}																\
} while (0)

/* always write log */
#define loga(...) do {												\
	HSLog::instance()->_log(__FILE__, __LINE__, __VA_ARGS__);		\
} while (0)

/* write log to stderr*/
#define log_stderr(...) do {										\
	HSLog::instance()->_log_stderr(__VA_ARGS__);					\
} while (0)

class HSLog {
public:
	~HSLog();
	static HSLog* instance();

	int _log_init(int level, string name);
	int _log_deinit();
	void log_level_up(int levelname);
	void log_level_down(int level);
	int _log_loggable(int level);
	void _log(const char *name, int line, const char *fmt, ...);
	void _log_stderr(const char *fmt, ...);

protected:
	HSLog();
	HSLog(const HSLog&);
	HSLog& operator = (const HSLog&);

private:
	string m_name;
	int m_fd;
	int m_level;
};

#endif
