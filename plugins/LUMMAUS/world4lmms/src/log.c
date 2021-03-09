#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "log.h"

int _log(int level, const char *format, ...)
{
#ifndef _DEBUG
	if (level >= DEBUG) {
		return 0;
	}
#endif

	static const char lvl[] = "ERROWARNINFODBUG";
	char tbuf[64] = {0}, buf[64] = {0};
	struct tm _tm;
	va_list arg;
	struct timeval tv;
	FILE *f;
	int done = 0;
	time_t t;
	
	gettimeofday(&tv, 0);
	t = tv.tv_sec;
	localtime_r(&t, &_tm);
	strftime(tbuf, 64, "%Y-%m-%d %H:%M:%S", &_tm);
	

	printf("%s.%03d [%.4s]  ", tbuf, (int)(tv.tv_usec / 1000), lvl + 4 * level);

	
	va_start(arg, format);
	vprintf(format, arg);
	va_end(arg);

	printf("\n");
    return 0;
}