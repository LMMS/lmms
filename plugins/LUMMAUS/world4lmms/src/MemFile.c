#include "MemFile.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

MemFile *OpenMemFile(const char *filename, const char *mode)
{
    MemFile *fp = NULL;
    size_t fsize = 0;

    FILE *f = fopen(filename, mode);
    if (f == NULL)
    {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);

    fp = (MemFile *)malloc(sizeof(MemFile));
    if (fp == NULL)
    {
        fclose(f);
        return NULL;
    }

    fp->filename = strdup(filename);
    fp->size = fsize;
    fp->data = malloc(sizeof(uint8) * fsize);
    fp->cur = fp->data;

    size_t ret = fread(fp->data, 1, fsize, f);
    fclose(f);

    if (ret != fsize)
    {
        free(fp->filename);
        free(fp->data);
        free(fp);
        return NULL;
    }

    return fp;
}

size_t ReadMemFile(void *pbuf, size_t size, size_t nitem, MemFile *fp)
{
    if (!fp)
    {
        return -1;
    }

    if (fp->cur >= (fp->data + fp->size))
    {
        return 0;
    }

    size_t nread = 0;

    if ((fp->cur + size * nitem) >= (fp->data + fp->size))
    {
        nread = fp->size - (fp->cur - fp->data);
    }
    else
    {
        nread = size * nitem;
    }

    memcpy(pbuf, fp->cur, nread);

    fp->cur += nread;

    return nread;
}

int SeekMemFile(MemFile *fp, size_t size, int mode)
{
    if (mode == SEEK_CUR)
    {
        if (fp->cur + size <= fp->data + fp->size)
        {
            fp->cur += size;
            return 0;
        }
        return -1;
    }
    else if (mode == SEEK_END)
    {
        if (size <= fp->size) {
            fp->cur = fp->data + fp->size - size;
            return 0;
        }
        return -1;
    }
    else if (mode == SEEK_SET)
    {
        if (size <= fp->size) {
            fp->cur = fp->data + size;
            return 0;
        }
        return -1;
    }

    return -1;
}

int CloseMemFile(MemFile *fp)
{
    free(fp->filename);
    free(fp->data);
    free(fp);
    return 0;
}