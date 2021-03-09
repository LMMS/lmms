#ifndef _H_UTIL_
#define _H_UTIL_

#include <stdio.h>
#include "types.h"

#if 1

#define F_FILE MemFile
#define F_OPEN(filename, mode) OpenMemFile(filename, mode)
#define F_READ(pbuf, size, nitem, fp) ReadMemFile(pbuf, size, nitem, fp)
#define F_SEEK(fp, size, mode) SeekMemFile(fp, size, mode)
#define F_CLOSE(fp) CloseMemFile(fp)

#else

#define F_FILE FILE
#define F_OPEN(filename, mode) fopen(filename, mode)
#define F_READ(pbuf, size, nitem, fp) fread(pbuf, size, nitem, fp)
#define F_SEEK(fp, size, mode) fseek(fp, size, mode)
#define F_CLOSE(fp) fclose(fp)

#endif

typedef struct mem
{
    uint8 *data;
    uint8 *cur;
    size_t size;
    char *filename;
} MemFile;

MemFile *OpenMemFile(const char *filename, const char *mode);
size_t ReadMemFile(void *pbuf, size_t size, size_t nitem, MemFile *fp);
int SeekMemFile(MemFile *fp, size_t size, int mode);
int CloseMemFile(MemFile *fp);

#endif