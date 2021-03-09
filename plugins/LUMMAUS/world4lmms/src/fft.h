//-----------------------------------------------------------------------------
// From https://github.com/mmorise/World/blob/master/src/world/fft.h
//-----------------------------------------------------------------------------

#ifndef _H_FFT_
#define _H_FFT_

// Commands for FFT (This is the same as FFTW)
#define FFT_FORWARD 1
#define FFT_BACKWARD 2
#define FFT_ESTIMATE 3

// Complex number for FFT
typedef double fft_complex[2];

// Struct used for FFT
typedef struct
{
    int n;
    int sign;
    unsigned int flags;
    fft_complex *c_in;
    double *in;
    fft_complex *c_out;
    double *out;
    double *input;
    int *ip;
    double *w;
} fft_plan;

fft_plan fft_plan_dft_1d(int n, fft_complex *in, fft_complex *out, int sign, unsigned int flags);
fft_plan fft_plan_dft_c2r_1d(int n, fft_complex *in, double *out, unsigned int flags);
fft_plan fft_plan_dft_r2c_1d(int n, double *in, fft_complex *out, unsigned int flags);
void fft_execute(fft_plan p);
void fft_destroy_plan(fft_plan p);


// Compatiable
#define FFTW_FORWARD FFT_FORWARD
#define FFTW_BACKWARD FFT_BACKWARD
#define FFTW_ESTIMATE FFT_ESTIMATE
typedef fft_complex fftw_complex;
typedef fft_plan fftw_plan;

#define fftw_plan_dft_1d fft_plan_dft_1d 
#define fftw_plan_dft_c2r_1d fft_plan_dft_c2r_1d 
#define fftw_plan_dft_r2c_1d fft_plan_dft_r2c_1d 
#define fftw_execute fft_execute
#define fftw_destroy_plan fft_destroy_plan

#endif
