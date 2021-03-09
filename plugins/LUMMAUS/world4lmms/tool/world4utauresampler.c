#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>

#include "common.h"
#include "constant.h"
#include "dio.h"
#include "matlab.h"
#include "platinum.h"
#include "star.h"
#include "synthesis.h"
#include "wavread.h"
#include "timer.h"
#include "MemFile.h"

#define FRAMEPERIOD 5.80498866213152

//
void makeFilename(const char *filename, const char *ext, char *output)
{
    strcpy(output, filename);
    char *cp = strrchr(output, '.');
    if (cp)
        *cp = 0;
    strcat(output, ext);

    printf("%s\n", output);
}

int readDIOParam(const char *filename, double *p_t[], double *p_f0[], int *p_fs, int *p_siglen) //return tLen
{
    char fname1[512];
    int n = 0;
    int len = 0, sps = 0;
    int i;
    int siglen = 0, fs = 0, tLen = 0;
    double *t = 0;
    double *f0 = 0;

    makeFilename(filename, ".dio", fname1);

    printf("read .dio:\n");

    F_FILE *fp = F_OPEN(fname1, "rb");
    if (fp)
    {
        char d[9];
        F_READ(d, 8, 1, fp);
        if (strncmp(d, "wrld-dio", 8) != 0)
        {
            F_CLOSE(fp);
            printf(" bad file.\n");
            return 0;
        }
        F_READ(&siglen, sizeof(int), 1, fp);
        F_READ(&fs, sizeof(int), 1, fp);
        F_READ(&tLen, sizeof(int), 1, fp);
        if (tLen > 0)
        {
            t = (double *)malloc(tLen * sizeof(double));
            f0 = (double *)malloc(tLen * sizeof(double));
            if (t && f0)
            {
                for (i = 0; i < tLen; i++)
                {
                    F_READ(&(t[i]), sizeof(double), 1, fp);
                    F_READ(&(f0[i]), sizeof(double), 1, fp);
                }
            }
            else
            {
                SAFE_FREE(t);
                SAFE_FREE(f0);
                t = 0;
                f0 = 0;
                tLen = 0;
                fprintf(stderr, " メモリーが確保できません。\n");
            }
        }
        F_CLOSE(fp);
    }
    *p_t = t;
    *p_f0 = f0;
    *p_fs = fs;
    *p_siglen = siglen;
    return tLen;
}

int getDIOParam(double x[], int signalLen, int fs, double framePeriod, double *p_t[], double *p_f0[])
{
    printf("DIO:");
    int tLen = getSamplesForDIO(fs, signalLen, framePeriod);
    double *t = (double *)malloc(tLen * sizeof(double));
    double *f0 = (double *)malloc(tLen * sizeof(double));
    if (t && f0)
    {
        dio(x, signalLen, fs, framePeriod, t, f0);
    }
    else
    {
        fprintf(stderr, "\n");
        SAFE_FREE(t);
        SAFE_FREE(f0);
        t = 0;
        f0 = 0;
        tLen = 0;
    }
    *p_t = t;
    *p_f0 = f0;
    return tLen;
}

int writeDIOParam(int signalLen, int fs, int tLen, const char *filename, double t[], double f0[])
{
    char fname1[512];
    makeFilename(filename, ".dio", fname1);

    printf("write .dio\n");

    //FILE *ft = fopen("dio0.txt", "wt");
    FILE *f = fopen(fname1, "wb");
    if (f)
    {
        fwrite("wrld-dio", 1, 8, f);
        fwrite(&signalLen, sizeof(int), 1, f);
        fwrite(&fs, sizeof(int), 1, f);
        fwrite(&tLen, sizeof(int), 1, f);
        int i;
        for (i = 0; i < tLen; i++)
        {
            int un;
            FP_SUBNORMAL;
            // if ((un = _fpclass(f0[i]) & 0x0087) != 0) //NaN,+Inf,-Inf,denormalを除外する // NaN,+Inf,-Inf,denormal排除
            // FIXME: 这里需要看看_fpclass 和 fpclassify的返回值上有什么不同之处 [ruix]
            // if ((un = fpclassify(f0[i]) & 0x0087) != 0)
            un = fpclassify(f0[i]);
            if (un == FP_NAN || un == FP_INFINITE || un == FP_SUBNORMAL)
            {
#ifdef _DEBUG
                printf("un[%d]=%04x!\n", i, un);
#endif
                f0[i] = 0;
            }
            fwrite(&(t[i]), sizeof(double), 1, f);
            fwrite(&(f0[i]), sizeof(double), 1, f);
            //fprintf(ft, "%lf\t%lf\n", t[i], f0[i]);
        }
        fclose(f);
    }

    return 0;
}
double **readSTARParam(int signalLen, int fs, const char *filename, int tLen, int fftl)
{
    int i, j;
    char fname2[512];
    unsigned short tn = 0;
    unsigned short us = 0;
    int siglen = 0;
    int rate = 0;
    double **specgram = 0;

    makeFilename(filename, ".star", fname2);

    printf("read .star:\n");

    F_FILE *fp = F_OPEN(fname2, "rb");
    if (fp)
    {
        char st[9];
        F_READ(st, 1, 8, fp);
        if (strncmp(st, "wrldstar", 8) != 0)
        {
            F_CLOSE(fp);
            printf(" bad file.\n");
            return 0;
        }
        F_READ(&siglen, sizeof(int), 1, fp);
        F_READ(&rate, sizeof(int), 1, fp);
        F_READ(&tn, sizeof(unsigned short), 1, fp);
        F_READ(&us, sizeof(unsigned short), 1, fp);
        if (tn == tLen && us == (fftl / 2 + 1) && signalLen == siglen && fs == rate)
        {
            specgram = (double **)malloc(tLen * sizeof(double *));
            if (specgram)
            {
                for (i = 0; i < tLen; i++)
                {
                    specgram[i] = (double *)malloc((fftl / 2 + 1) * sizeof(double));
                    memset(specgram[i], 0, (fftl / 2 + 1) * sizeof(double));
                    if (specgram[i])
                    {
                        for (j = 0; j <= fftl / 2; j++)
                        {
                            unsigned short v;
                            F_READ(&v, sizeof(unsigned short), 1, fp);
                            //= (unsigned short)(log(specgram[i][j]*(2048.0*2048*2048)+1) * 512.0 + 0.5);
                            specgram[i][j] = (exp(v / 1024.0) - 1) * 1.16415321826935E-10; // /(2048.0*2048*2048);
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                if (i < tLen)
                {
                    for (j = 0; j < i; j++)
                    {
                        free(specgram[i]);
                    }
                    free(specgram);
                    specgram = 0;
                    fprintf(stderr, " メモリーが確保できません。%d\n", i);
                }
            }
            else
            {
                fprintf(stderr, " メモリーが確保できません。\n");
            }
        }
        else
        {
            tn = 0;
        }
        F_CLOSE(fp);
    }
    return specgram;
}
double **getSTARParam(double x[], int signalLen, int fs, double t[], double f0[], int tLen, int fftl)
{
    printf("STAR:");

    double **specgram = (double **)malloc(sizeof(double *) * tLen);
    if (specgram)
    {
        int i, j;
        for (i = 0; i < tLen; i++)
        {
            specgram[i] = (double *)malloc(sizeof(double) * (fftl / 2 + 1));
            memset(specgram[i], 0, sizeof(double) * (fftl / 2 + 1));
            if (specgram[i])
            {
                memset(specgram[i], 0, sizeof(double) * (fftl / 2 + 1));
            }
            else
            {
                break;
            }
        }
        if (i == tLen)
        {
            star(x, signalLen, fs, t, f0, specgram);
        }
        else
        {
            for (j = 0; j < i; j++)
            {
                free(specgram[i]);
            }
            free(specgram);
            specgram = 0;
            fprintf(stderr, " メモリーが確保できません。%d\n", i);
        }
    }
    else
    {
        fprintf(stderr, " メモリーが確保できません。\n");
    }
    return specgram;
}
void writeSTARParam(int signalLen, int fs, const char *filename, double *specgram[], int tLen, int fftl)
{
    unsigned short tn;
    unsigned short us;
    int i, j;
    char fname2[512];
    makeFilename(filename, ".star", fname2);

    printf("write .star:");

    short max = -32767, min = 32767;
    //FILE *ft = fopen("star0.txt", "wt");
    FILE *f1 = fopen(fname2, "wb");
    if (f1)
    {
        fwrite("wrldstar", 1, 8, f1);
        tn = (unsigned short)tLen;
        us = (unsigned short)fftl / 2 + 1;
        fwrite(&signalLen, sizeof(int), 1, f1);
        fwrite(&fs, sizeof(int), 1, f1);
        fwrite(&tn, sizeof(unsigned short), 1, f1);
        fwrite(&us, sizeof(unsigned short), 1, f1);
        for (i = 0; i < tLen; i++)
        {
            for (j = 0; j <= fftl / 2; j++)
            {
                int un;
                // if ((un = _fpclass(specgram[i][j]) & 0x0087) != 0)
                un = fpclassify(specgram[i][j]);
                if (un == FP_NAN || un == FP_INFINITE || un == FP_SUBNORMAL)
                {
                    specgram[i][j] = 0;
#ifdef _DEBUG
                    printf("un[%d][%d]=%04x!\n", i, j, un);
#endif
                }
                unsigned short v = (unsigned short)(log(specgram[i][j] * (2048.0 * 2048 * 2048) + 1) * 1024.0 + 0.5);
                fwrite(&v, sizeof(unsigned short), 1, f1);
                //fprintf(ft, "%0.9lf\t", specgram[i][j]*1000000.0);
                if (max < v)
                {
                    max = v;
                }
                if (min > v)
                {
                    min = v;
                }
            }
            //fprintf(ft, "\n");
        }
        fclose(f1);
    }
    //fclose(ft);
    printf("max = %d, min = %d\n", max, min);
}

double **readPlatinumParam(int signalLen, int fs, const char *filename, int tLen, int fftl)
{
    int i, j;
    unsigned short tn = 0;
    unsigned short us = 0;
    int siglen = 0;
    int rate = 0;
    double **residualSpecgram = 0;

    char fname3[512];
    makeFilename(filename, ".platinum", fname3);

    printf("read .platinum:\n");

    // FILE *fp = fopen(fname3, "rb");
    F_FILE *fp = F_OPEN(fname3, "rb");
    if (fp)
    {
        char b[9];
        F_READ(b, 1, 8, fp);
        if (strncmp(b, "platinum", 8) != 0)
        {
            F_CLOSE(fp);
            printf(" bad file.\n");
            return 0;
        }
        F_READ(&siglen, sizeof(int), 1, fp);
        F_READ(&rate, sizeof(int), 1, fp);
        F_READ(&tn, sizeof(unsigned short), 1, fp);
        F_READ(&us, sizeof(unsigned short), 1, fp);
        if (tn == tLen && (fftl + 1) == us && signalLen == siglen && fs == rate)
        {
            residualSpecgram = (double **)malloc(tLen * sizeof(double *));
            if (residualSpecgram)
            {
                for (i = 0; i < tLen; i++)
                {
                    residualSpecgram[i] = (double *)malloc((fftl + 1) * sizeof(double));
                    if (residualSpecgram[i])
                    {
                        memset(residualSpecgram[i], 0, (fftl + 1) * sizeof(double));
                        for (j = 0; j <= fftl; j++)
                        {
                            short v;
                            F_READ(&v, sizeof(short), 1, fp);
                            residualSpecgram[i][j] = v * 3.90625E-03; // /256.0;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                if (i < tLen)
                {
                    for (j = 0; j < i; j++)
                    {
                        free(residualSpecgram[i]);
                    }
                    free(residualSpecgram);
                    residualSpecgram = 0;
                    fprintf(stderr, " メモリーが確保できません。%d\n", i);
                }
            }
            else
            {
                fprintf(stderr, " メモリーが確保できません。\n");
            }
        }
        F_CLOSE(fp);
    }
    return residualSpecgram;
}

double **getPlatinumParam(double x[], int signalLen, int fs, double t[], double f0[], double *specgram[], int tLen, int fftl)
{
    printf("PLATINUM:");
    double **residualSpecgram = (double **)malloc(sizeof(double *) * tLen);
    if (residualSpecgram)
    {
        int i, j;
        for (i = 0; i < tLen; i++)
        {
            residualSpecgram[i] = (double *)malloc(sizeof(double) * (fftl + 1));
            memset(residualSpecgram[i], 0, sizeof(double) * (fftl + 1));
            if (residualSpecgram[i])
            {
                memset(residualSpecgram[i], 0, sizeof(double) * (fftl + 1));
            }
            else
            {
                break;
            }
        }
        if (i == tLen)
        {
            platinum(x, signalLen, fs, t, f0, specgram, residualSpecgram);
        }
        else
        {
            for (j = 0; j < i; j++)
            {
                free(residualSpecgram[i]);
            }
            free(residualSpecgram);
            residualSpecgram = 0;
            fprintf(stderr, " メモリーが確保できません。%d\n", i);
        }
    }
    else
    {
        fprintf(stderr, " メモリーが確保できません。\n");
    }
    return residualSpecgram;
}

void writePlatinumParam(int signalLen, int fs, const char *filename, double *residualSpecgram[], int tLen, int fftl)
{
    printf("write .platinum:");

    int i, j;
    unsigned short tn = 0;
    unsigned short us = 0;
    short max = -32767, min = 32767;

    char fname3[512];
    makeFilename(filename, ".platinum", fname3);

    FILE *f1 = fopen(fname3, "wb");
    if (f1)
    {
        fwrite("platinum", 1, 8, f1);
        tn = (unsigned short)tLen;
        us = (unsigned short)(fftl + 1);
        fwrite(&signalLen, sizeof(int), 1, f1);
        fwrite(&fs, sizeof(int), 1, f1);
        fwrite(&tn, sizeof(unsigned short), 1, f1);
        fwrite(&us, sizeof(unsigned short), 1, f1);
        for (i = 0; i < tLen; i++)
        {
            for (j = 0; j <= fftl; j++)
            {
                int un;
                // if ((un = _fpclass(residualSpecgram[i][j]) & 0x0087) != 0)
                un = fpclassify(residualSpecgram[i][j]);
                if (un == FP_NAN || un == FP_INFINITE || un == FP_SUBNORMAL)
                {
                    residualSpecgram[i][j] = 0;
#ifdef _DEBUG
                    printf("unr[%d][%d]=%04x!\n", i, j, un);
#endif
                }
                short v = (short)(residualSpecgram[i][j] * 256.0);
                //v = log(v * (2048.0*2048.0*2048.0) + 1) * 1024.0;
                if (v > 32767)
                {
                    v = 32767;
                }
                else if (v < -32768)
                {
                    v = -32768; //一応飽和計算しておく（したら不味いけど） //暂时计算饱和度（不好吃）
                }
                fwrite(&v, sizeof(short), 1, f1);
                if (max < v)
                {
                    max = v;
                }
                if (min > v)
                {
                    min = v;
                }
            }
        }
        fclose(f1);
    }
    printf("max = %d, min = %d\n", max, min);
}

void freeSpecgram(double **spec, int n)
{
    int i;
    if (spec && n)
    {
        for (i = 0; i < n; i++)
        {
            free(spec[i]);
        }
        free(spec);
    }
}

int generateParams(const char *wavfile)
{
    PROFILER_START(total);

    int i, j;
    FILE *fp;
    int fs, nbit = 16;

    double *x = 0, *f0 = NULL, *t = NULL, *y = NULL;
    double **specgram = NULL;
    double **residualSpecgram = NULL;
    int fftl = 0;

    int signalLen = 0;
    int tLen = 0;

    tLen = readDIOParam(wavfile, &t, &f0, &fs, &signalLen);
    if (tLen != 0)
    {
        fftl = getFFTLengthForStar(fs);
        specgram = readSTARParam(signalLen, fs, wavfile, tLen, fftl);
        if (specgram)
        {
            residualSpecgram = readPlatinumParam(signalLen, fs, wavfile, tLen, fftl);
            if (!residualSpecgram)
            {
                tLen = 0;
            }
        }
        else
        {
            tLen = 0;
        }
    }

    if (tLen == 0)
    {
        freeSpecgram(specgram, tLen);
        freeSpecgram(residualSpecgram, tLen);

        // Read wav file to memory
        x = wavread(wavfile, &fs, &nbit, &signalLen);
        if (x == NULL)
        {
            fprintf(stderr, "error: 指定されたファイルは存在しません．\n"); // 指定的文件不存在
            return 0;
        }

        // Get DIO param and write to file
        tLen = getDIOParam(x, signalLen, fs, FRAMEPERIOD, &t, &f0);
        if (tLen != 0)
        {
            writeDIOParam(signalLen, fs, tLen, wavfile, t, f0);
        }
        else
        {
            SAFE_FREE(x);
            fprintf(stderr, "error: DIO initialization failed．\n");
            return 0;
        }

        // Get STAR param and write to file
        fftl = getFFTLengthForStar(fs);
        specgram = getSTARParam(x, signalLen, fs, t, f0, tLen, fftl);
        if (!specgram)
        {
            SAFE_FREE(x);
            SAFE_FREE(t);
            SAFE_FREE(f0);
            fprintf(stderr, "error: STAR initialization failed．\n");
            return -1;
        }
        else
        {
            writeSTARParam(signalLen, fs, wavfile, specgram, tLen, fftl);
        }

        // Get Platinum param and write to file
        residualSpecgram = getPlatinumParam(x, signalLen, fs, t, f0, specgram, tLen, fftl);
        if (!residualSpecgram)
        {
            SAFE_FREE(x);
            SAFE_FREE(t);
            SAFE_FREE(f0);
            SAFE_FREE(specgram);
            fprintf(stderr, "error: Platinum initialization failed．\n"); // STAR参数创建失败
            return -1;
        }
        else
        {
            writePlatinumParam(signalLen, fs, wavfile, residualSpecgram, tLen, fftl);
        }
    }
    else
    {
        printf("[%s] is already processed.\n", wavfile);
    }

    SAFE_FREE(x);
    SAFE_FREE(t);
    SAFE_FREE(f0);
    SAFE_FREE(y);
    for (i = 0; i < tLen; i++)
    {
        SAFE_FREE(specgram[i]);
        SAFE_FREE(residualSpecgram[i]);
    }
    SAFE_FREE(specgram);
    SAFE_FREE(residualSpecgram);

    PROFILER_END(total);

    return 0;
}

int WORLD4UTAU_RESAMPLER_PROCESS(const char *inputFile,
                                 const char *outputFile,
                                 int note,
                                 double consonantVelocity,
                                 const char *flags,
                                 double offset,
                                 double scaledLength,
                                 double consonantLength,
                                 double cutoff,
                                 double intensity,
                                 double modulation,
                                 double tempo,
                                 double *pitchbend)
{
    printf("./tool");

    int ii = 1;
    while (ii < argc)
    {
        printf(" '%s'", argv[ii]);
        ii++;
    }
    printf("\n");

    if (argc != 2)
    {
        fprintf(stderr, "error: wrong params\n");
        return 0;
    }

    DIR *d = opendir(inputFile);
    struct dirent *entry = NULL;
    if (!d)
    {
        printf("open dir [%s] failed.\n", inputFile);
        return -1;
    }
    else
    {
        for (entry = readdir(d); entry != NULL; entry = readdir(d))
        {
            if (!strcmp(entry->d_name, "."))
                continue;

            if (!strcmp(entry->d_name, ".."))
                continue;

            if (DT_REG == entry->d_type)
            {
                if (entry->d_name[0] == '.')
                    continue;

                if (strstr(entry->d_name, ".wav"))
                {
                    printf("list file [%s]\n", entry->d_name);
                    char path[4096] = {0};
                    char tmp[1024] = {0};
                    sprintf(tmp, "%s/%s", inputFile, entry->d_name);
                    char * real = realpath(tmp, path);
                    generateParams(real);
                    printf("list file [%s]\n", real);
                }
            }
            else
            {
                printf("file [%s] is %d\n", entry->d_name, entry->d_type);
            }
        }
    }

    return 0;
}