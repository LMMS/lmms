#ifndef _H_LOG_
#define _H_LOG_

enum log_levels {
	ERROR,
    WARNING,
	INFO,
	DEBUG
};

extern int _log(int level, const char *format, ...);

#define log_err(...)   _log(ERROR   , __VA_ARGS__)
#define log_warn(...)  _log(WARNING , __VA_ARGS__)
#define log_info(...)  _log(INFO    , __VA_ARGS__)
#define log_debug(...) _log(DEBUG   , __VA_ARGS__)

#endif